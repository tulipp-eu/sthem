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

#include "lynsyn_main.h"
#include "fpga.h"
#include "jtag.h"

#include <stdio.h>
#include <string.h>
#include <em_usart.h>
#include <em_prs.h>
#include <stdlib.h>

static uint32_t initOk = false;

#define PRS_CHANNEL 1
#define PRS_CHANNEL_MASK 2

#define USE_USART

//#define DUMP_PINS

#ifdef USE_FPGA_JTAG_CONTROLLER

#define SPI_CMD_STATUS          0 // CMD/status - 0/status
#define SPI_CMD_MAGIC           1 // CMD/status - 0/data
#define SPI_CMD_JTAG_SEL        2 // CMD/status - sel/0
#define SPI_CMD_WR_SEQ          3 // CMD/status - size/0 - (tdidata/0 tmsdata/0)* 0/0
#define SPI_CMD_RDWR_SEQ        4 // CMD/status - size/0 - (tdidata/0 tmsdata/0)* 0/0
#define SPI_CMD_GET_DATA        5 // CMD/status - (0/data)* (ff/data)
#define SPI_CMD_STORE_SEQ       6 // CMD/status - size/0 (tdidata/0 tmsdata/0)* 0/0
#define SPI_CMD_STORE_PROG      7 // CMD/status - size/0 (initpos/0 looppos/0 ackpos/0 endpos/0)* 0/0
#define SPI_CMD_EXECUTE_SEQ     8 // CMD/status - 0/0
#define SPI_CMD_JTAG_TEST       9 // CMD/status - 0/data
#define SPI_CMD_OSC_TEST       10 // CMD/status - 0/data

#define JTAG_EXT  0
#define JTAG_INT  1
#define JTAG_TEST 2

#define MAGIC 0xad

bool outputSpiCommands = false;
uint8_t *tdoData = NULL;

///////////////////////////////////////////////////////////////////////////////

static inline uint8_t transfer(uint8_t data) {
  uint8_t data_out = USART_SpiTransfer(FPGA_USART, data);
  if(outputSpiCommands) {
    printf("spi_transfer(%d, data);\n", data);
  }
  return data_out;
}

static inline uint8_t startCmd(uint8_t cmd) {
  // assert CS */
  GPIO_PinOutClear(FPGA_PORT, FPGA_CS_BIT);

  if(outputSpiCommands) {
    switch(cmd) {
      case SPI_CMD_STATUS:      printf("spi_command(`SPI_CMD_STATUS, data);\n"); break;
      case SPI_CMD_MAGIC:       printf("spi_command(`SPI_CMD_MAGIC, data);\n"); break;
      case SPI_CMD_JTAG_SEL:    printf("spi_command(`SPI_CMD_JTAG_SEL, data);\n"); break;
      case SPI_CMD_WR_SEQ:      printf("spi_command(`SPI_CMD_WR_SEQ, data);\n"); break;
      case SPI_CMD_RDWR_SEQ:    printf("spi_command(`SPI_CMD_RDWR_SEQ, data);\n"); break;
      case SPI_CMD_GET_DATA:    printf("spi_command(`SPI_CMD_GET_DATA, data);\n"); break;
      case SPI_CMD_STORE_SEQ:   printf("spi_command(`SPI_CMD_STORE_SEQ, data);\n"); break;
      case SPI_CMD_STORE_PROG:  printf("spi_command(`SPI_CMD_STORE_PROG, data);\n"); break;
      case SPI_CMD_EXECUTE_SEQ: printf("spi_command(`SPI_CMD_EXECUTE_SEQ, data);\n"); break;
      case SPI_CMD_JTAG_TEST:   printf("spi_command(`SPI_CMD_JTAG_TEST, data);\n"); break;
      case SPI_CMD_OSC_TEST:    printf("spi_command(`SPI_CMD_OSC_TEST, data);\n"); break;
    }
    return USART_SpiTransfer(FPGA_USART, cmd);
  } else {
    return transfer(cmd);
  }
}

static inline void endCmd(void) {
  // release CS
  GPIO_PinOutSet(FPGA_PORT, FPGA_CS_BIT);
  if(outputSpiCommands) {
    printf("spi_done();\n\n");
  }
}

///////////////////////////////////////////////////////////////////////////////

uint8_t readStatus(void) {
  startCmd(SPI_CMD_STATUS);
  uint8_t status = transfer(0);
  endCmd();
  return status;
}

uint8_t readMagic(void) {
  startCmd(SPI_CMD_MAGIC);
  uint8_t magic = transfer(0);
  endCmd();
  return magic;
}

void jtagInt(void) {
  startCmd(SPI_CMD_JTAG_SEL);
  transfer(JTAG_INT);
  endCmd();
}

void jtagExt(void) {
  startCmd(SPI_CMD_JTAG_SEL);
  transfer(JTAG_EXT);
  endCmd();
}

static bool jtagTestSingle(uint8_t testVal) {
  startCmd(SPI_CMD_JTAG_SEL);
  transfer(JTAG_TEST | (testVal << 2));
  endCmd();

  startCmd(SPI_CMD_JTAG_TEST);
  uint8_t gotVal = transfer(0);
  endCmd();

  return gotVal == testVal;
}

int jtagTest(void) {
  int result = 0;

  if(!jtagTestSingle(1)) result |= 1;
  if(!jtagTestSingle(2)) result |= 2;
  if(!jtagTestSingle(4)) result |= 4;
  if(!jtagTestSingle(8)) result |= 8;
  if(!jtagTestSingle(0x10)) result |= 0x10;

  return result;
}

bool oscTest(void) {
  startCmd(SPI_CMD_OSC_TEST);
  uint8_t oscTest = transfer(0);
  endCmd();
  return oscTest == 0xf;
}

void waitStatus() {
  if(outputSpiCommands) {
    printf("data = 0;\n");
    printf("while((data & 3) != 3) begin\n");
    printf("  spi_command(`SPI_CMD_STATUS, data);\n");
    printf("  spi_transfer(0, data);\n");
    printf("  spi_done();\n");
    printf("end\n\n");
  }
  bool oldOutput = outputSpiCommands;
  outputSpiCommands = false;
  while((readStatus() & 3) != 3);
  outputSpiCommands = oldOutput;
}
    
///////////////////////////////////////////////////////////////////////////////

void storeSeq(uint16_t size, uint8_t *tdiData, uint8_t *tmsData) {
  if(size > 2048) panic("Size\n");
  if(size) {
    size *= 2;
    startCmd(SPI_CMD_STORE_SEQ);
    transfer(size & 0xff);
    transfer((size >> 8) & 0xff);

    for(int i = 0; i < size; i++) {
      transfer(*tdiData++);
      transfer(*tmsData++);
    }

    transfer(0);
    endCmd();
  }

  int bytes = size / 8;
  if(size % 8) bytes++;
  if(tdoData) free(tdoData);
  tdoData = (uint8_t*)malloc(bytes);
}

void storeProg(unsigned size, uint8_t *read, uint16_t *initPos, uint16_t *loopPos, uint16_t *ackPos, uint16_t *endPos) {
  if(size >= 64) panic("Stored program size is %d\n", size);
  if(size) {
    startCmd(SPI_CMD_STORE_PROG);
    transfer(size);

    for(int i = 0; i < size; i++) {
      transfer(read[i]);
      transfer(initPos[i] & 0xff);
      transfer((initPos[i] >> 8) & 0xff);
      transfer(loopPos[i] & 0xff);
      transfer((loopPos[i] >> 8) & 0xff);
      transfer(ackPos[i] & 0xff);
      transfer((ackPos[i] >> 8) & 0xff);
      transfer(endPos[i] & 0xff);
      transfer((endPos[i] >> 8) & 0xff);
    }

    transfer(0);
    endCmd();
  }
}

void executeSeq(void) {
  startCmd(SPI_CMD_EXECUTE_SEQ);
  transfer(0);
  endCmd();
}

///////////////////////////////////////////////////////////////////////////////

void writeSeq(unsigned start, unsigned size, uint8_t *tdiData, uint8_t *tmsData) {
  if(start != 0) panic("Invalid start");

  if(size > MAX_SEQ_SIZE) panic("Size %d\n", size);
  if(size) {
    unsigned bytes = size/8;
    if(size%8) bytes++;

    startCmd(SPI_CMD_WR_SEQ);
    transfer(size);
    for(int i = 0; i < bytes; i++) {
      transfer(*tdiData++);
      transfer(*tmsData++);
    }
    transfer(0);
    endCmd();
  }
}

uint8_t *readSeq(unsigned size) {
  uint8_t *ptr = tdoData;
  unsigned bytes = size/8;
  unsigned bits = size%8;
  if(bits) bytes++;

  waitStatus();

  startCmd(SPI_CMD_GET_DATA);

  for(int i = 0; i < bytes; i++) {
    uint8_t data = transfer((i == bytes-1) ? 0xff : 0);
    if((i == bytes-1) && bits) {
      *ptr++ = data >> (8 - bits);
    } else {
      *ptr++ = data;
    }
  }
  endCmd();

  return tdoData;
}

void readWriteSeq(unsigned start, unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *tdoData) {
  if(start != 0) panic("Invalid start");

  if(size > 255) panic("Size %d\n", size);
  if(size) {
    unsigned bytes = size/8;
    unsigned bits = size%8;

    if(bits) bytes++;

    startCmd(SPI_CMD_RDWR_SEQ);
    transfer(size);
    for(int i = 0; i < bytes; i++) {
      transfer(*tdiData++);
      transfer(*tmsData++);
    }
    transfer(0);
    endCmd();

    waitStatus();

    startCmd(SPI_CMD_GET_DATA);
    for(int i = 0; i < bytes; i++) {
      uint8_t data = transfer((i == bytes-1) ? 0xff : 0);
      if((i == bytes-1) && size%8) {
        *tdoData++ = data >> (8 - bits);
      } else {
        *tdoData++ = data;
      }
    }
    endCmd();
  }
}

///////////////////////////////////////////////////////////////////////////////

#else

static inline void jtagPinWrite(bool clk, bool tdi, bool tms) {
  GPIO_PortOutSetVal(JTAG_PORT,
                     (clk << JTAG_TCK_BIT) | (tdi << JTAG_TDI_BIT), 
                     (1 << JTAG_TCK_BIT) | (1 << JTAG_TDI_BIT));
  if(tms) GPIO_PinOutSet(TMS_PORT, TMS_BIT);
  else GPIO_PinOutClear(TMS_PORT, TMS_BIT);
}

static inline void usartOn(void) {
#ifdef USE_USART
  jtagPinWrite(false, false, false);

  JTAG_USART->ROUTE = (JTAG_USART->ROUTE & ~_USART_ROUTE_LOCATION_MASK) | JTAG_USART_LOC
    | USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN;

  TMS_USART->ROUTE = (TMS_USART->ROUTE & ~_USART_ROUTE_LOCATION_MASK) | TMS_USART_LOC
    | USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN;
#endif
}

static inline void usartOff(void) {
#ifdef USE_USART
  JTAG_USART->ROUTE = 0;
  TMS_USART->ROUTE = 0;
#endif
}

static inline unsigned readBit(void) {
  return GPIO_PinInGet(JTAG_PORT, JTAG_TDO_BIT);
}

static inline void writeBit(bool tdi, bool tms) {
  jtagPinWrite(false, tdi, tms);
  jtagPinWrite(true, tdi, tms);
}

static void readWriteSeqMask(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *tdoData) {
  int bytes = size / 8;
  int leftovers = size % 8;

  if(bytes) {
    for(int byte = 0; byte < bytes; byte++) {
#ifdef DUMP_PINS
      printf("%x %x = ", *tdiData, *tmsData);
#endif
#ifdef USE_USART
      JTAG_USART->CMD = USART_CMD_TXDIS;
      TMS_USART->CMD = USART_CMD_TXDIS;

      uint8_t txDataTdi = *tdiData++;
      JTAG_USART->TXDATA = (uint32_t)txDataTdi;

      uint8_t txDataTms = *tmsData++;
      TMS_USART->TXDATA = (uint32_t)txDataTms;

      PRS_PulseTrigger(PRS_CHANNEL_MASK);

      while(!(JTAG_USART->STATUS & USART_STATUS_RXDATAV));
      uint8_t val = (uint8_t)JTAG_USART->RXDATA;
#else
      uint8_t val = 0;

      for(int readPos = 0; readPos < 8; readPos++) {
        writeBit((*tdiData >> readPos) & 1, (*tmsData >> readPos) & 1);
        val |= readBit() << readPos;
      }
      tdiData++;
      tmsData++;

#endif
#ifdef DUMP_PINS
      printf("%x\n", val);
#endif
      if(tdoData) *tdoData++ = val;
    }
  }

  if(leftovers) {
    usartOff();

    uint8_t val = 0;
    for(int readPos = 0; readPos < leftovers; readPos++) {
      writeBit((*tdiData >> readPos) & 1, (*tmsData >> readPos) & 1);
      val |= readBit() << readPos;
    }

    if(tdoData) *tdoData = val;

    usartOn();
  }
}

static inline void readWriteSeqSpi(unsigned bytes, uint8_t *tdiData, uint8_t *tmsData, uint8_t *tdoData) {
  for(int byte = 0; byte < bytes; byte++) {
#ifdef DUMP_PINS
    printf("%x %x = ", *tdiData, *tmsData);
#endif

    JTAG_USART->CMD = USART_CMD_TXDIS;
    TMS_USART->CMD = USART_CMD_TXDIS;

    uint8_t txDataTdi = *tdiData++;
    JTAG_USART->TXDATA = (uint32_t)txDataTdi;

    uint8_t txDataTms = *tmsData++;
    TMS_USART->TXDATA = (uint32_t)txDataTms;

    PRS_PulseTrigger(PRS_CHANNEL_MASK);

    while(!(JTAG_USART->STATUS & USART_STATUS_RXDATAV));
    uint8_t val = (uint8_t)JTAG_USART->RXDATA;

#ifdef DUMP_PINS
    printf("%x\n", val);
#endif
    *tdoData++ = val;
  }
}

void jtagInt(void) {
  GPIO_PinOutSet(JTAG_PORT, JTAG_SEL_BIT);
}

void jtagExt(void) {
  GPIO_PinOutClear(JTAG_PORT, JTAG_SEL_BIT);
}

void writeSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData) {
  readWriteSeqMask(size, tdiData, tmsData, NULL);
}

void readWriteSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *tdoData) {
  readWriteSeqMask(size, tdiData, tmsData, tdoData);
}

uint8_t *storedTdi = NULL;
uint8_t *storedTms = NULL;

unsigned progSize;
uint32_t *words = NULL;
unsigned *ackPos = NULL;
bool *doRead = NULL;

unsigned *initSize = NULL;
uint8_t **initTdi = NULL;
uint8_t **initTms = NULL;

unsigned *loopSize = NULL;
uint8_t **loopTdi = NULL;
uint8_t **loopTms = NULL;

void storeSeq(uint16_t size, uint8_t *tdiData, uint8_t *tmsData) {
  if(storedTdi) free(storedTdi);
  storedTdi = malloc(size);

  if(storedTms) free(storedTms);
  storedTms = malloc(size);

  if(!storedTdi || !storedTms) panic("Out of memory (storeSeq)");

  memcpy(storedTdi, tdiData, size);
  memcpy(storedTms, tmsData, size);
}

void storeProg(unsigned size, uint8_t *read, uint16_t *initPos, uint16_t *loopPos, uint16_t *ap, uint16_t *endPos) {
  progSize = size;

  if(ackPos) free(ackPos);
  ackPos = malloc(size * sizeof(unsigned));

  if(doRead) free(doRead);
  doRead = malloc(size * sizeof(bool));

  if(initSize) free(initSize);
  initSize = malloc(size * sizeof(unsigned));
  if(initTdi) free(initTdi);
  initTdi = malloc(size * sizeof(uint8_t*));
  if(initTms) free(initTms);
  initTms = malloc(size * sizeof(uint8_t*));

  if(loopSize) free(loopSize);
  loopSize = malloc(size * sizeof(unsigned));
  if(loopTdi) free(loopTdi);
  loopTdi = malloc(size * sizeof(uint8_t*));
  if(loopTms) free(loopTms);
  loopTms = malloc(size * sizeof(uint8_t*));

  if(words) free(words);
  words = malloc(size * sizeof(uint32_t));

  if(!ackPos || !doRead || !initSize || !initTdi || !initTms || !loopSize || !loopTdi || !loopTms || !words) panic("Out of memory");

  for(int command = 0; command < size; command++) {
    doRead[command] = read[command];

    initSize[command] = (loopPos[command] - initPos[command]) / 8;
    int initByte = initPos[command] / 8;

    loopSize[command] = (endPos[command] - loopPos[command]) / 8;
    int loopByte = loopPos[command] / 8;

    ackPos[command] = ap[command] - loopPos[command];

    initTdi[command] = storedTdi + initByte;
    initTms[command] = storedTms + initByte;

    loopTdi[command] = storedTdi + loopByte;
    loopTms[command] = storedTms + loopByte;
  }

}

void executeSeq(void) {
  int wordCounter = 0;

  for(int command = 0; command < progSize; command++) {
    uint8_t ack;
    uint8_t tdo[32];

    readWriteSeqSpi(initSize[command], initTdi[command], initTms[command], tdo);

    do {
      readWriteSeqSpi(loopSize[command], loopTdi[command], loopTms[command], tdo);
      unsigned pos = ackPos[command];
      ack = extractAck(&pos, tdo);
      if(doRead[command] && (ack != JTAG_ACK_WAIT)) {
        uint32_t word = extractWord(&pos, tdo);
        words[wordCounter++] = word;
      }

    } while(ack == JTAG_ACK_WAIT);

    if(ack != JTAG_ACK_OK) panic("Invalid ACK\n");
  }
}

uint32_t *getWords() {
  return words;
}

#endif

///////////////////////////////////////////////////////////////////////////////

bool fpgaInit(void) {
  initOk = FPGA_INIT_OK;

  // set up pins
  GPIO_PinModeSet(PROGRAM_B_PORT, PROGRAM_B_BIT, gpioModePushPull, 0);
  GPIO_PinOutClear(PROGRAM_B_PORT, PROGRAM_B_BIT);
  GPIO_PinOutSet(PROGRAM_B_PORT, PROGRAM_B_BIT);

  GPIO_PinModeSet(DONE_PORT, DONE_BIT, gpioModeInput, 0);

  // wait until the FPGA is configured
  int done;
  DWT->CYCCNT = 0;
  do {
    done = GPIO_PinInGet(DONE_PORT, DONE_BIT);
    if(DWT->CYCCNT > FPGA_CONFIGURE_TIMEOUT) {
      initOk = FPGA_CONFIGURE_FAILED;
      printf("Can't configure the FPGA\n");
      return false;
    }
  } while(!done);

#ifdef USE_FPGA_JTAG_CONTROLLER
  // setup INT input from FPGA
  GPIO_PinModeSet(FPGA_PORT, FPGA_INT_BIT, gpioModeInput, 0);

  /////////////////////////////////////////////////////////////////////////////
  // setup spi
  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;

  // configure spi pins
  GPIO_PinModeSet(FPGA_PORT, FPGA_TX_BIT, gpioModePushPull, 1);
  GPIO_PinModeSet(FPGA_PORT, FPGA_RX_BIT, gpioModeInput, 0);
  GPIO_PinModeSet(FPGA_PORT, FPGA_CLK_BIT, gpioModePushPull, 0);
  GPIO_PinModeSet(FPGA_PORT, FPGA_CS_BIT, gpioModePushPull, 1);

  USART_Reset(FPGA_USART);

  // enable clock
  CMU_ClockEnable(FPGA_USART_CLK, true);

  // configure
  init.baudrate = 12000000;
  init.msbf     = true;
  USART_InitSync(FPGA_USART, &init);
  
  FPGA_USART->ROUTE = (FPGA_USART->ROUTE & ~_USART_ROUTE_LOCATION_MASK) |
                      FPGA_USART_LOC;

  FPGA_USART->ROUTE |= USART_ROUTE_TXPEN | USART_ROUTE_RXPEN |
                       USART_ROUTE_CLKPEN;

  //////////////////////////////////////////////////////////////////////////////
  // read magic

  uint8_t magic = readMagic();
  if(magic != MAGIC) {
    printf("Got incorrect FPGA magic number '%x'\n", magic);
    initOk = FPGA_SPI_FAILED;
    return false;
  }

#else

  GPIO_PinModeSet(JTAG_PORT, JTAG_SEL_BIT, gpioModePushPull, JTAG_INT);

  GPIO_PinModeSet(TMS_PORT, TMS_BIT, gpioModePushPull, 0);
  GPIO_PinModeSet(JTAG_PORT, JTAG_TCK_BIT, gpioModePushPull, 0);
  GPIO_PinModeSet(JTAG_PORT, JTAG_TDO_BIT, gpioModeInput, 0);
  GPIO_PinModeSet(JTAG_PORT, JTAG_TDI_BIT, gpioModePushPull, 0);

  GPIO_PortOutSetVal(JTAG_PORT, 0, (1 << JTAG_TCK_BIT) | (1 << JTAG_TDI_BIT));
  GPIO_PortOutSetVal(TMS_PORT, 0, (1 << TMS_BIT));

#ifdef USE_USART
  ///////////////////////////////////////////////////////////////////////////////
  // USART

  USART_Reset(JTAG_USART);
  USART_Reset(TMS_USART);

  // enable clock
  CMU_ClockEnable(JTAG_USART_CLK, true);
  CMU_ClockEnable(TMS_USART_CLK, true);
  CMU_ClockEnable(cmuClock_PRS, true);;

  // configure
  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;
  init.baudrate = 7000000;
  init.msbf     = false;
  USART_InitSync(JTAG_USART, &init);
  USART_InitSync(TMS_USART, &init);

  USART_PrsTriggerInit_TypeDef prsInit = USART_INITPRSTRIGGER_DEFAULT;
  prsInit.autoTxTriggerEnable = false;
  prsInit.rxTriggerEnable = true;
  prsInit.txTriggerEnable = true;
  prsInit.prsTriggerChannel = PRS_CHANNEL;
  USART_InitPrsTrigger(JTAG_USART, &prsInit);
  USART_InitPrsTrigger(TMS_USART, &prsInit);

  usartOn();
#endif

#endif

  //////////////////////////////////////////////////////////////////////////////
  // jtag mux

  jtagExt();

  return true;
}

uint32_t fpgaInitOk(void) {
  return initOk;
}
