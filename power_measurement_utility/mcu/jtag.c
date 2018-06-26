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

#include "lynsyn.h"
#include "jtag.h"
#include "fpga.h"

///////////////////////////////////////////////////////////////////////////////

int numDevices = 0;
struct JtagDevice devices[MAX_JTAG_DEVICES];
uint32_t dpIdcode = 0;

unsigned numCores;
unsigned numEnabledCores;
struct Core cores[MAX_CORES];

bool zynqUltrascale;

///////////////////////////////////////////////////////////////////////////////

static unsigned getIrLen(uint32_t idcode) {

  if(idcode == CORTEXA9_TAP_IDCODE) {
    return CORTEXA9_TAP_IRLEN;
  }
  if(idcode == PL_TAP_IDCODE) {
    return PL_TAP_IRLEN;
  }

  if(idcode == CORTEXA53_TAP_IDCODE) {
    return CORTEXA53_TAP_IRLEN;
  }
  if(idcode == PL_US_TAP_IDCODE) {
    return PL_US_TAP_IRLEN;
  }

  if(idcode == PS_TAP_IDCODE) {
    return PS_TAP_IRLEN;
  }

  return 0;
}

static bool queryChain(void) {
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
        dev.irlen = getIrLen(dev.idcode);
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
  dpLowAccess(ADIV5_LOW_WRITE, addr, value);
}

static uint32_t dpRead(uint16_t addr) {
  dpLowAccess(ADIV5_LOW_READ, addr, 0);
  uint32_t ret = dpLowAccess(ADIV5_LOW_READ, ADIV5_DP_RDBUFF, 0);
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

  dpAbort(ADIV5_DP_ABORT_DAPABORT);

  uint32_t ctrlstat = dpRead(ADIV5_DP_CTRLSTAT);

  /* Write request for system and debug power up */
  dpWrite(ADIV5_DP_CTRLSTAT,
          ctrlstat |= ADIV5_DP_CTRLSTAT_CSYSPWRUPREQ |
          ADIV5_DP_CTRLSTAT_CDBGPWRUPREQ);

  /* Wait for acknowledge */
  while(((ctrlstat = dpRead(ADIV5_DP_CTRLSTAT)) &
         (ADIV5_DP_CTRLSTAT_CSYSPWRUPACK | ADIV5_DP_CTRLSTAT_CDBGPWRUPACK)) !=
        (ADIV5_DP_CTRLSTAT_CSYSPWRUPACK | ADIV5_DP_CTRLSTAT_CDBGPWRUPACK));
}

///////////////////////////////////////////////////////////////////////////////
// ARM AP

unsigned apSel = 0;

static void apWrite(uint16_t addr, uint32_t value) {
  dpWrite(ADIV5_DP_SELECT, ((uint32_t)apSel << 24)|(addr & 0xF0));
  dpWrite(addr, value);
}

static uint32_t apRead(uint16_t addr) {
  dpWrite(ADIV5_DP_SELECT, ((uint32_t)apSel << 24)|(addr & 0xF0));
  uint32_t ret = dpRead(addr);
  return ret;
}

static void apInit(unsigned ap) {
  apSel = ap;
}

///////////////////////////////////////////////////////////////////////////////
// ARM Core

void coreWriteReg(struct Core *core, uint16_t reg, uint32_t val) {
  uint32_t addr = core->baddr + 4*reg;
#ifdef DUMP_PINS
  printf("Write Reg %x = %x\n", (unsigned)addr, (unsigned)val);
#endif
  apWrite(ADIV5_AP_TAR, addr);
  dpWrite(ADIV5_AP_DRW, val);
}

uint32_t coreReadReg(struct Core *core, uint16_t reg) {
  uint32_t addr = core->baddr + 4*reg;
  apWrite(ADIV5_AP_TAR, addr);
  uint32_t ret = dpRead(ADIV5_AP_DRW);
#ifdef DUMP_PINS
  printf("Read Reg %x = %x\n", (unsigned)addr, (unsigned)ret);
#endif
  return ret;
}

uint64_t calcOffset(uint64_t dbgpcsr) {
  return dbgpcsr & ~3;
}

uint64_t coreReadPcsr(struct Core *core) {
  uint64_t dbgpcsr;

  if(zynqUltrascale) {
    if(core->enabled) {
      dbgpcsr = ((uint64_t)coreReadReg(core, A53_PCSR_H) << 32) | coreReadReg(core, A53_PCSR_L);
    } else {
      dbgpcsr = 0xffffffff;
    }
  } else {
    dbgpcsr = coreReadReg(core, A9_PCSR);
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
  if(zynqUltrascale) {
    coreWriteReg(&cores[core], A53_BVR_L(bpNum), addr & 0xfffffffc);
    coreWriteReg(&cores[core], A53_BVR_H(bpNum), (addr >> 32));
    coreWriteReg(&cores[core], A53_BCR(bpNum), (1 << 13) | (3 << 1) | BCR_BAS_ANY | BCR_EN);
		coreWriteReg(&cores[core], A53_SCR, coreReadReg(&cores[core], A53_SCR) | (0x1 << 14));
  } else {
    coreWriteReg(&cores[core], A9_BVR(bpNum), addr & ~3);
    coreWriteReg(&cores[core], A9_BCR(bpNum), BCR_BAS_ANY | BCR_EN);
  }
}

void coreClearBp(unsigned core, unsigned bpNum) {
  if(zynqUltrascale) {
    coreWriteReg(&cores[core], A53_BCR(bpNum), 0);
  } else {
    coreWriteReg(&cores[core], A9_BCR(bpNum), 0);
  }
}

void coresResume(void) {
  uint32_t dbgdscr;

  if(zynqUltrascale) {
    for(int i = 0; i < numCores; i++) {
      if(cores[i].enabled) {
        /* ack */
        coreWriteReg(&cores[i], A53_CTIINTACK, HALT_EVENT_BIT);
      }
    }

    /* Pulse Channel 0 */
    coreWriteReg(&cores[0], A53_CTIAPPPULSE, CHANNEL_0);

    /* Poll until restarted */
    while((coreReadReg(&cores[0], A53_SCR) & 0x3f) != 2);

  } else {
    // enable halting debug mode for controlling breakpoints
    dbgdscr = coreReadReg(&cores[0], A9_DSCR);
    dbgdscr |= DSCR_HDBGEN;
    dbgdscr &= ~DSCR_ITREN;
    coreWriteReg(&cores[0], A9_DSCR, dbgdscr);

    // clear sticky error and resume
    coreWriteReg(&cores[0], A9_DRCR, DRCR_CSE | DRCR_RRQ);

    // wait for restart
    while(1) {
      dbgdscr = coreReadReg(&cores[0], A9_DSCR);
      if(dbgdscr & DSCR_RESTARTED) {
        break;
      }
      coreWriteReg(&cores[0], A9_DRCR, DRCR_CSE | DRCR_RRQ);
    }
  }
}

bool coreHalted(void) {
  if(zynqUltrascale) {
    return coreReadReg(&cores[stopCore], A53_PRSR) & (1 << 4);
  } else {
    return coreReadPcsr(&cores[stopCore]) == 0xffffffff;
  }
}

struct Core coreInit(uint32_t baddr) {
  struct Core core;
  core.baddr = baddr;

  if(zynqUltrascale) {
    core.enabled = coreReadReg(&core, A53_PRSR) & 1;

    if(core.enabled) {
      /* enable CTI */
      coreWriteReg(&core, A53_CTICONTROL, 1);

      /* send channel 0 and 1 out to other PEs */
      coreWriteReg(&core, A53_CTIGATE, CHANNEL_1 | CHANNEL_0);

      /* generate channel 1 event on halt trigger */
      coreWriteReg(&core, A53_CTIINEN(HALT_EVENT), CHANNEL_1);

      /* halt on channel 1 */
      coreWriteReg(&core, A53_CTIOUTEN(HALT_EVENT), CHANNEL_1);

      /* restart on channel 0 */
      coreWriteReg(&core, A53_CTIOUTEN(RESTART_EVENT), CHANNEL_0);
    }

  } else {
    core.enabled = true;
  }

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

void jtagInitCores(void) {
  gotoResetThenIdle();

  if(!queryChain()) {
    return;
  }

  // test for zynq ultrascale+.  TODO: Bad test, should make this more general
  if((devices[0].idcode == PS_TAP_IDCODE) || 
     (devices[0].idcode == CORTEXA53_TAP_IDCODE)) {
    zynqUltrascale = true;
  } else {
    zynqUltrascale = false;
  }

  if(zynqUltrascale && (devices[0].idcode == PS_TAP_IDCODE)) {
    printf("Enabling DAP access\n");
    uint8_t jtagCtrl[4] = {(JTAG_CTRL_ARM_DAP | JTAG_CTRL_PL_TAP), 0, 0, 0};
    writeIr(PS_TAP_IDCODE, PS_JTAG_CTRL);
    readWriteDr(PS_TAP_IDCODE, (uint8_t*)&jtagCtrl, NULL, 32);
    queryChain();
  }

  {
    if(zynqUltrascale) {
      dpInit(CORTEXA53_TAP_IDCODE);
    } else {
      dpInit(CORTEXA9_TAP_IDCODE);
    }

    bool found = false;
    int apNum = 0;
    for(int i=0; i<ADIV5_MAX_APS; i++) {
      apInit(i);
      uint32_t idr = apRead(ADIV5_AP_IDR);
      if(idr) printf("Found AP %x\n", (unsigned)idr);
      if((idr & 0x0fffffff) == CORTEXA_AP_IDR) {
        found = true;
        apNum = i;
      }
    }
    if(!found) panic("AP not found");
    apInit(apNum);

    apWrite(ADIV5_AP_CSW, CORTEXA_CSW);
  }

  {
    uint32_t base;
    unsigned increment;

    if(zynqUltrascale) {
      numCores = CORTEXA53_CORES;
      base = CORTEXA53_CORE0_BASE;
      increment = CORTEXA53_INCREMENT;

      printf("Number of cores running: %d\n", numCores);
    } else {
      numCores = CORTEXA9_CORES;
      base = CORTEXA9_CORE0_BASE;
      increment = CORTEXA9_INCREMENT;
    }

    numEnabledCores = 0;
    for(int i = 0; i < numCores; i++) {
      cores[i] = coreInit(base);
      if(cores[i].enabled) numEnabledCores++;
      base += increment;
    }
  }

  printf("Number of cores: %d. Number of enabled cores: %d\n", numCores, numEnabledCores);

  coreReadPcsrInit();

  // get num breakpoints
  unsigned numBps;
  if(zynqUltrascale) {
    numBps = ((coreReadReg(&cores[0], A53_DFR0) >> 12) & 0xf) + 1;
  } else {
    numBps = ((coreReadReg(&cores[0], A9_DIDR) >> 24) & 0xf) + 1;
  }

  printf("Number of HW breakpoints: %d\n", numBps);

  // clear all breakpoints
  for(unsigned j = 0; j < numCores; j++) {
    if(zynqUltrascale) {
      if(cores[j].enabled) {
        for(unsigned i = 0; i < numBps; i++) {
          coreClearBp(j, i);
        }
      }
    } else {
      for(unsigned i = 0; i < numBps; i++) {
        coreClearBp(j, i);
      }
    }
  }
}
