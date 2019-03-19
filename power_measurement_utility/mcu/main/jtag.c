/******************************************************************************
 *
 *  This file is part of the Lynsyn PMU firmware.
 *
 *  Copyright 2018 Asbjørn Djupdal, NTNU, TULIPP EU Project
 *
 *  Lynsyn is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Lynsyn is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Lynsyn.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <em_usart.h>

#include "jtag.h"
#include "fpga.h"

///////////////////////////////////////////////////////////////////////////////

int numDevices = 0;
struct JtagDevice devices[MAX_JTAG_DEVICES];
uint32_t dpIdcode = 0;

static unsigned apSelMem = 0;
static bool mem64 = false;
unsigned numCores;
unsigned numEnabledCores;
struct Core cores[MAX_CORES];

///////////////////////////////////////////////////////////////////////////////

static unsigned getIrLen(uint32_t idcode, struct JtagDevice *devlist) {

  for(int i = 0; i < MAX_JTAG_DEVICES; i++) {
    if(idcode == devlist[i].idcode) {
      return devlist[i].irlen;
    }
  }

  panic("Unknown IDCODE %x\n", idcode);

  return 0;
}

static bool queryChain(struct JtagDevice *devlist) {
  numDevices = getNumDevices();

  if((numDevices == 0) || (numDevices == MAX_JTAG_DEVICES)) {
    printf("No JTAG chain found\n");
    return false;

  } else {
    uint32_t idcodes[MAX_JTAG_DEVICES];
    getIdCodes(idcodes);
    for(int i = 0; i < numDevices; i++) {
      if(idcodes[i]) {
        struct JtagDevice dev;
        dev.idcode = idcodes[i];
        dev.irlen = getIrLen(dev.idcode, devlist);
        devices[i] = dev;
        printf("Device %d: idcode %x\n", i, (int)devices[i].idcode);
      } else {
        numDevices = i;
        break;
      }
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// ARM DP

static void dpWrite(uint16_t addr, uint32_t value) {
  dpLowAccess(LOW_WRITE, addr, value);
}

static uint32_t dpRead(uint16_t addr) {
  dpLowAccess(LOW_READ, addr, 0);
  uint32_t ret = dpLowAccess(LOW_READ, DP_RDBUFF, 0);
  return ret;
}

static void dpAbort(uint32_t abort) {
#ifdef DUMP_PINS
  printf("abort\n");
#endif

  uint8_t request_buf[8];
  memset(request_buf, 0, 8);

  request_buf[4] = (abort & 0xE0000000) >> 29;
  uint32_t tmp = (abort << 3);
  request_buf[3] = (tmp & 0xFF000000) >> 24;
  request_buf[2] = (tmp & 0x00FF0000) >> 16;
  request_buf[1] = (tmp & 0x0000FF00) >> 8;
  request_buf[0] = (tmp & 0x000000FF);

  writeIr(dpIdcode, JTAG_IR_ABORT);
  readWriteDr(dpIdcode, request_buf, NULL, 35);
}

static void dpInit(uint32_t idcode) {
  dpIdcode = idcode;

  dpAbort(DP_ABORT_DAPABORT);

  uint32_t ctrlstat = dpRead(DP_CTRLSTAT);

  /* Write request for system and debug power up */
  dpWrite(DP_CTRLSTAT,
          ctrlstat |= DP_CTRLSTAT_CSYSPWRUPREQ |
          DP_CTRLSTAT_CDBGPWRUPREQ);

  /* Wait for acknowledge */
  while(((ctrlstat = dpRead(DP_CTRLSTAT)) &
         (DP_CTRLSTAT_CSYSPWRUPACK | DP_CTRLSTAT_CDBGPWRUPACK)) !=
        (DP_CTRLSTAT_CSYSPWRUPACK | DP_CTRLSTAT_CDBGPWRUPACK));
}

///////////////////////////////////////////////////////////////////////////////
// ARM AP

static void apWrite(unsigned apSel, uint16_t addr, uint32_t value) {
  dpWrite(DP_SELECT, ((uint32_t)apSel << 24)|(addr & 0xF0));
  dpWrite(addr, value);
}

static uint32_t apRead(unsigned apSel, uint16_t addr) {
  dpWrite(DP_SELECT, ((uint32_t)apSel << 24)|(addr & 0xF0));
  uint32_t ret = dpRead(addr);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// ARM AXI

uint32_t axiReadMem(uint64_t addr) {
  apWrite(apSelMem, AP_TAR, addr & 0xffffffff);
  if(mem64) apWrite(apSelMem, AP_TAR_HI, addr >> 32);
  return dpRead(AP_DRW);
}

void axiWriteMem(uint64_t addr, uint32_t val) {
  apWrite(apSelMem, AP_TAR, addr & 0xffffffff);
  if(mem64) apWrite(apSelMem, AP_TAR_HI, addr >> 32);
  dpWrite(AP_DRW, val);
}

///////////////////////////////////////////////////////////////////////////////
// ARM Core

void coreWriteReg(struct Core *core, uint16_t reg, uint32_t val) {
  uint32_t addr = core->baddr + 4*reg;
#ifdef DUMP_PINS
  printf("Write Reg %x = %x\n", (unsigned)addr, (unsigned)val);
#endif
  apWrite(core->ap, AP_TAR, addr);
  dpWrite(AP_DRW, val);
}

uint32_t coreReadReg(struct Core *core, uint16_t reg) {
  uint32_t addr = core->baddr + 4*reg;
  apWrite(core->ap, AP_TAR, addr);
  uint32_t ret = dpRead(AP_DRW);
#ifdef DUMP_PINS
  printf("Read Reg %x = %x\n", (unsigned)addr, (unsigned)ret);
#endif
  return ret;
}

uint8_t coreReadStatus(unsigned core) {
  return coreReadReg(&cores[core], ARMV8A_SCR) & 0x3f;
}

uint64_t readPc(unsigned core) {
  if(cores[core].type == ARMV7A) {
    uint32_t dbgdscr;

    dbgdscr = coreReadReg(&cores[0], ARMV7A_DSCR);
    dbgdscr |= DSCR_ITREN;
    coreWriteReg(&cores[0], ARMV7A_DSCR, dbgdscr);

    coreWriteReg(&cores[core], ARMV7A_ITR, 0xe1a0000f);
    uint32_t regno = 0;
    uint32_t instr = MCR | DTRTXint | ((regno & 0xf) << 12);
    coreWriteReg(&cores[core], ARMV7A_ITR, instr);

    return coreReadReg(&cores[core], ARMV7A_DTRTX) - 8;

  } else if(cores[core].type == ARMV8A) {
    coreWriteReg(&cores[core], ARMV8A_ITR, ARMV8A_MRS_DLR(0));
    coreWriteReg(&cores[core], ARMV8A_ITR, ARMV8A_MSR_GP(SYSTEM_DBG_DBGDTR_EL0, 0));

    return (((uint64_t)coreReadReg(&cores[core], ARMV8A_DTRRX)) << 32) | (uint64_t)coreReadReg(&cores[core], ARMV8A_DTRTX);
  }

  return 0;
}

uint64_t calcOffset(uint64_t dbgpcsr) {
  return dbgpcsr & ~3;
}

uint64_t coreReadPcsr(struct Core *core) {
  uint64_t dbgpcsr = 0;

  if(core->type == ARMV7A) {
    dbgpcsr = coreReadReg(core, ARMV7A_PCSR);

  } else if(core->type == ARMV8A) {
    if(core->enabled) {
      uint64_t low = coreReadReg(core, ARMV8A_PCSR_L);
      uint64_t high = coreReadReg(core, ARMV8A_PCSR_H);
      dbgpcsr = (high << 32) | low;
    } else {
      dbgpcsr = 0xffffffff;
    }
  }

  if (dbgpcsr == 0xffffffff) {
    // This happens when
    // - Non-invasive debug is disabled
    // - Processor is in a mode or state where non-invasive debug is not permitted
    // - Processor is in Debug state.
    return dbgpcsr;

  } else {
    return calcOffset(dbgpcsr);
  }

  return 0;
}



void coreSetBp(unsigned core, unsigned bpNum, uint64_t addr) {
  if(cores[core].type == ARMV7A) {
    coreWriteReg(&cores[core], ARMV7A_BVR(bpNum), addr & ~3);
    coreWriteReg(&cores[core], ARMV7A_BCR(bpNum), BCR_BAS_ANY | BCR_EN);

  } else if(cores[core].type == ARMV8A) {
    coreWriteReg(&cores[core], ARMV8A_BVR_L(bpNum), addr & 0xfffffffc);
    coreWriteReg(&cores[core], ARMV8A_BVR_H(bpNum), (addr >> 32));
    coreWriteReg(&cores[core], ARMV8A_BCR(bpNum), (1 << 13) | (3 << 1) | BCR_BAS_ANY | BCR_EN);
    coreWriteReg(&cores[core], ARMV8A_SCR, coreReadReg(&cores[core], ARMV8A_SCR) | (0x1 << 14));
  }
}

void coreClearBp(unsigned core, unsigned bpNum) {
  if(cores[core].type == ARMV7A) {
    coreWriteReg(&cores[core], ARMV7A_BCR(bpNum), 0);
  } else if(cores[core].type == ARMV8A) {
    coreWriteReg(&cores[core], ARMV8A_BCR(bpNum), 0);
  }
}

void setBp(unsigned bpNum, uint64_t addr) {
  for(unsigned j = 0; j < numCores; j++) {
    if(cores[j].enabled) {
      coreSetBp(j, bpNum, addr);
    }
  }
}

void clearBp(unsigned bpNum) {
  for(unsigned j = 0; j < numCores; j++) {
    if(cores[j].enabled) {
      coreClearBp(j, bpNum);
    }
  }
}

void coresResume(void) {
  // FIXME: Does not work if there are several different types of cores in the system

  if(cores[0].type == ARMV7A) {
    uint32_t dbgdscr;

    // enable halting debug mode for controlling breakpoints
    dbgdscr = coreReadReg(&cores[0], ARMV7A_DSCR);
    dbgdscr |= DSCR_HDBGEN;
    dbgdscr &= ~DSCR_ITREN;
    coreWriteReg(&cores[0], ARMV7A_DSCR, dbgdscr);

    // clear sticky error and resume
    coreWriteReg(&cores[0], ARMV7A_DRCR, DRCR_CSE | DRCR_RRQ);

    // wait for restart
    while(1) {
      dbgdscr = coreReadReg(&cores[0], ARMV7A_DSCR);
      if(dbgdscr & DSCR_RESTARTED) {
        break;
      }
      coreWriteReg(&cores[0], ARMV7A_DRCR, DRCR_CSE | DRCR_RRQ);
    }

  } else if(cores[0].type == ARMV8A) {
    if(coreHalted(0)) {
      for(int i = 0; i < numCores; i++) {
        if(cores[i].enabled) {
          /* ack */
          coreWriteReg(&cores[i], ARMV8A_CTIINTACK, HALT_EVENT_BIT);
        }
      }

      /* Pulse Channel 0 */
      coreWriteReg(&cores[0], ARMV8A_CTIAPPPULSE, CHANNEL_0);

      /* Poll until restarted */
      while(!(coreReadReg(&cores[0], ARMV8A_PRSR) & (1<<11)));
    }
  }
}

bool coreHalted(unsigned core) {
  if(cores[core].type == ARMV7A) {
    return coreReadPcsr(&cores[core]) == 0xffffffff;

  } else if(cores[core].type == ARMV8A) {
    return coreReadReg(&cores[core], ARMV8A_PRSR) & (1 << 4);
  }

  return false;
}

struct Core coreInitA9(unsigned apSel, uint32_t baddr) {
  struct Core core;

  core.type = ARMV7A;
  core.core = CORTEX_A9;
  core.ap = apSel;
  core.baddr = baddr;
  core.enabled = true;
  printf("        Enabled\n");

  return core;
}

void coreInitArmV8(unsigned apSel, uint32_t baddr, struct Core *core) {
  core->type = ARMV8A;
  core->ap = apSel;
  core->baddr = baddr;
  core->enabled = coreReadReg(core, ARMV8A_PRSR) & 1;

  if(core->enabled) {
    printf("        Enabled\n");
    
    /* enable CTI */
    coreWriteReg(core, ARMV8A_CTICONTROL, 1);

    /* send channel 0 and 1 out to other PEs */
    coreWriteReg(core, ARMV8A_CTIGATE, CHANNEL_1 | CHANNEL_0);

    /* generate channel 1 event on halt trigger */
    coreWriteReg(core, ARMV8A_CTIINEN(HALT_EVENT), CHANNEL_1);

    /* halt on channel 1 */
    coreWriteReg(core, ARMV8A_CTIOUTEN(HALT_EVENT), CHANNEL_1);

    /* restart on channel 0 */
    coreWriteReg(core, ARMV8A_CTIOUTEN(RESTART_EVENT), CHANNEL_0);

  } else {
    printf("        Disabled\n");
  }
}

struct Core coreInitA53(unsigned apSel, uint32_t baddr) {
  struct Core core;

  core.core = CORTEX_A53;
  coreInitArmV8(apSel, baddr, &core);

  return core;
}

struct Core coreInitA57(unsigned apSel, uint32_t baddr) {
  struct Core core;

  core.core = CORTEX_A57;
  coreInitArmV8(apSel, baddr, &core);

  return core;
}

struct Core coreInitDenver2(unsigned apSel, uint32_t baddr) {
  struct Core core;

  core.core = DENVER_2;
  coreInitArmV8(apSel, baddr, &core);

  return core;
}

///////////////////////////////////////////////////////////////////////////////

void jtagInit(void) {
#ifdef USE_FPGA_JTAG_CONTROLLER

#else

  GPIO_PinModeSet(JTAG_PORT, JTAG_TMS_BIT, gpioModePushPull, 0);
  GPIO_PinModeSet(JTAG_PORT, JTAG_TCK_BIT, gpioModePushPull, 0);
  GPIO_PinModeSet(JTAG_PORT, JTAG_TDO_BIT, gpioModeInput, 0);
  GPIO_PinModeSet(JTAG_PORT, JTAG_TDI_BIT, gpioModePushPull, 0);

  GPIO_PortOutSetVal(JTAG_PORT, 0, (1 << JTAG_TCK_BIT) | (1 << JTAG_TDI_BIT) | (1 << JTAG_TMS_BIT));

#ifdef USE_USART
  ///////////////////////////////////////////////////////////////////////////////
  // USART

  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;

  USART_Reset(JTAG_USART);

  // enable clock
  CMU_ClockEnable(JTAG_USART_CLK, true);

  // configure
  init.baudrate = 10000000;
  init.msbf     = false;
  USART_InitSync(JTAG_USART, &init);
#endif

  ///////////////////////////////////////////////////////////////////////////////

#endif
}

void parseDebugEntry(unsigned apSel, uint32_t compBase) {
  
  // read CIDR1
  uint32_t addr = compBase + 0xff4;
  apWrite(apSel, AP_TAR, addr);
  uint32_t cidr1 = dpRead(AP_DRW);

  if((cidr1 & 0xf0) == 0x10) { // is a ROM
    bool done = false;
    uint32_t entryAddr = compBase;

    printf("  Got ROM at %x\n", (unsigned)compBase);

    while(!done) {
      // read entry
      apWrite(apSel, AP_TAR, entryAddr);
      uint32_t entry = dpRead(AP_DRW);

      if(!entry) done = true;

      // else
      if(entry & 1) { // entry is valid
        parseDebugEntry(apSel, compBase + (entry & 0xfffff000));
      }

      entryAddr += 4;
    }

  } else if((cidr1 & 0xf0) == 0x90) { // is debug component
    // read PIDR

    uint32_t addr0 = compBase + 0xfe0;
    apWrite(apSel, AP_TAR, addr0);
    uint32_t pidr0 = dpRead(AP_DRW);
                      
    uint32_t addr1 = compBase + 0xfe4;
    apWrite(apSel, AP_TAR, addr1);
    uint32_t pidr1 = dpRead(AP_DRW);
                      
    uint32_t addr2 = compBase + 0xfe8;
    apWrite(apSel, AP_TAR, addr2);
    uint32_t pidr2 = dpRead(AP_DRW);
                      
    uint32_t addr4 = compBase + 0xfd0;
    apWrite(apSel, AP_TAR, addr4);
    uint32_t pidr4 = dpRead(AP_DRW);
                      
    printf("    Got Entry at %x: %x %x %x %x\n", (unsigned)compBase, (unsigned)pidr0, (unsigned)pidr1, (unsigned)pidr2, (unsigned)pidr4);

    /* if(((pidr0 & 0xff) == 0x15) && // found a Cortex R5 */
    /*    ((pidr1 & 0xff) == 0xbc) && */
    /*    ((pidr2 & 0x0f) == 0xb) && */
    /*    ((pidr4 & 0x0f) == 0x4)) { */

    /*   cores[numCores] = coreInitR5(apSel, compBase); */
    /*   if(cores[numCores].enabled) numEnabledCores++; */

    /*   numCores++; */

    /* } else */

    if(((pidr0 & 0xff) == 0x9) && // found a Cortex A9
       ((pidr1 & 0xff) == 0xbc) &&
       ((pidr2 & 0x0f) == 0xb) &&
       ((pidr4 & 0x0f) == 0x4)) {

      printf("      Found Cortex A9\n");

      cores[numCores] = coreInitA9(apSel, compBase);
      if(cores[numCores].enabled) numEnabledCores++;

      numCores++;

    } else if(((pidr0 & 0xff) == 0x3) && // found a Cortex A53
             ((pidr1 & 0xff) == 0xbd) &&
             ((pidr2 & 0x0f) == 0xb) &&
             ((pidr4 & 0x0f) == 0x4)) {

      printf("      Found Cortex A53\n");

      cores[numCores] = coreInitA53(apSel, compBase);
      if(cores[numCores].enabled) numEnabledCores++;

      numCores++;

    } else if(((pidr0 & 0xff) == 0x7) && // found a Cortex A53
             ((pidr1 & 0xff) == 0xbd) &&
             ((pidr2 & 0x0f) == 0xb) &&
             ((pidr4 & 0x0f) == 0x4)) {

      printf("      Found Cortex A57\n");

      cores[numCores] = coreInitA57(apSel, compBase);
      if(cores[numCores].enabled) numEnabledCores++;

      numCores++;

    /* } else if(((pidr0 & 0xff) == 0x2) && // found a Denver 2 */
    /*          ((pidr1 & 0xff) == 0xb3) && */
    /*          ((pidr2 & 0x0f) == 0xe) && */
    /*          ((pidr4 & 0x0f) == 0x3)) { */

    /*   printf("      Found Denver 2\n"); */

    /*   cores[numCores] = coreInitDenver2(apSel, compBase); */
    /*   if(cores[numCores].enabled) numEnabledCores++; */

    /*   numCores++; */
    }
  }
}

bool jtagInitCores(struct JtagDevice *devlist) {
  gotoResetThenIdle();

  // query chain
  if(!queryChain(devlist)) {
    return false;
  }

  printf("Num devices: %d\n", numDevices);

  /* // attempt to enable debug access */
  /* if(devices[0].idcode == PS_TAP_IDCODE) { */
  /*   printf("Enabling DAP access\n"); */
  /*   uint8_t jtagCtrl[4] = {(JTAG_CTRL_ARM_DAP | JTAG_CTRL_PL_TAP), 0, 0, 0}; */
  /*   writeIr(PS_TAP_IDCODE, PS_JTAG_CTRL); */
  /*   readWriteDr(PS_TAP_IDCODE, (uint8_t*)&jtagCtrl, NULL, 32); */
  /*   queryChain(); */
  /* } */

  { // find TAP
    bool found = false;

    for(int i = 0; i < numDevices; i++) {
      if((devices[i].idcode & 0x0fffffff) == ARM_TAP_IDCODE) {
        dpInit(devices[i].idcode);
        found = true;
        break;
      }
    }

    if(!found) {
      printf("TAP not found\n");
      return false;
    }
  }

  { // find APs and cores
    numCores = 0;
    numEnabledCores = 0;

    for(int i = 0; i < MAX_APS; i++) {
      uint32_t idr = apRead(i, AP_IDR);
      uint32_t base = apRead(i, AP_BASE);
      uint32_t cfg = apRead(i, AP_CFG);

      if((idr & 0x0fffff0f) == IDR_APB_AP) {
        printf("Found APB AP (%d) idr %x base %x cfg %x\n", i, (unsigned)idr, (unsigned)base, (unsigned)cfg);

        if(base == ~0) { // legacy format, not present
          
        } else if((base & 0x3) == 0) { // legacy format, present
          apWrite(i, AP_CSW, 0x80000042);
          parseDebugEntry(i, base & 0xfffff000);

        } else if((base & 0x3) == 0x3) { // debug entry is present
          apWrite(i, AP_CSW, 0x80000042);
          parseDebugEntry(i, base & 0xfffff000);
        }

      } else if((idr & 0x0fffff0f) == IDR_AXI_AP) {
        printf("Found AXI AP (%d) idr %x base %x cfg %x\n", i, (unsigned)idr, (unsigned)base, (unsigned)cfg);

        apSelMem = i;
        mem64 = cfg & 2;
        if(mem64) {
          printf("  64 bit AXI memory space\n");
        }

      } else if((idr & 0x0fffff0f) == IDR_AHB_AP) {
        printf("Found AHB AP (%d)\n", i);

        apSelMem = i;
        mem64 = false;

      } else if(idr != 0) {
        printf("Found unknown AP %x (%d)\n", (unsigned)idr, i);
      }
    }
  }

  if(numCores == 0) {
    printf("No cores found\n");
    return false;
  }

  printf("Number of cores: %d. Number of enabled cores: %d\n", numCores, numEnabledCores);

  coreReadPcsrInit();

  // get num breakpoints
  unsigned numBps = 0;
  if(cores[0].type == ARMV7A) {
    numBps = ((coreReadReg(&cores[0], ARMV7A_DIDR) >> 24) & 0xf) + 1;
  } else if(cores[0].type == ARMV8A) {
    numBps = ((coreReadReg(&cores[0], ARMV8A_DFR0) >> 12) & 0xf) + 1;
  }

  printf("Number of HW breakpoints: %d\n", numBps);

  // clear all breakpoints
  for(unsigned j = 0; j < numCores; j++) {
    if(cores[j].enabled) {
      for(unsigned i = 0; i < numBps; i++) {
        coreClearBp(j, i);
      }
    }
  }

  printf("JTAG chain init done\n");

  return true;
}
