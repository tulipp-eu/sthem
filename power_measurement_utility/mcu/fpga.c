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

#include "lynsyn.h"
#include "fpga.h"

#include <stdio.h>
#include <string.h>
#include <em_usart.h>

static uint32_t initOk = false;

#ifdef USE_FPGA_JTAG_CONTROLLER

#define SPI_CMD_STATUS      0 // CMD/status - 0/status
#define SPI_CMD_MAGIC       1 // CMD/status - 0/data
#define SPI_CMD_JTAG_SEL    2 // CMD/status - sel/0
#define SPI_CMD_WR_SEQ      3 // CMD/status - size/0 - (tdidata/0 tmsdata/0)* 0/0
#define SPI_CMD_RDWR_SEQ    4 // CMD/status - size/0 - (tdidata/0 tmsdata/0)* 0/0
#define SPI_CMD_GET_DATA    5 // CMD/status - (0/data)*
#define SPI_CMD_STORE_SEQ   6 // CMD/status - size/0 size/0 (tdidata/0 tmsdata/0 readdata/0)*
#define SPI_CMD_EXECUTE_SEQ 7 // CMD/status - 0/0
#define SPI_CMD_JTAG_TEST   8 // CMD/status - 0/data
#define SPI_CMD_OSC_TEST    9 // CMD/status - 0/data

#define JTAG_EXT  0
#define JTAG_INT  1
#define JTAG_TEST 2

#define MAGIC 0xad

bool outputSpiCommands = false;

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

void writeSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData) {
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

void storeSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *readData) {
  if(size > MAX_STORED_SEQ_SIZE) panic("Size %d\n", size);
  if(size) {
    unsigned bytes = size/8;
    unsigned bits = size%8;

    if(bits) bytes++;

    startCmd(SPI_CMD_STORE_SEQ);
    transfer(size & 0xff);
    transfer((size >> 8) & 0xff);
    for(int i = 0; i < bytes; i++) {
      if(readData) transfer(*readData++);
      else transfer(0);
      transfer(*tdiData++);
      transfer(*tmsData++);
    }
    endCmd();
  }
}

void executeSeq(void) {
  startCmd(SPI_CMD_EXECUTE_SEQ);
  transfer(0);
  endCmd();
}

void readSeq(unsigned size, uint8_t *tdoData) {
  unsigned bytes = size/8;
  unsigned bits = size%8;
  if(bits) bytes++;

  waitStatus();

  startCmd(SPI_CMD_GET_DATA);

  for(int i = 0; i < bytes; i++) {
    uint8_t data = transfer((i == bytes-1) ? 0xff : 0);
    if((i == bytes-1) && bits) {
      *tdoData++ = data >> (8 - bits);
    } else {
      *tdoData++ = data;
    }
  }
  endCmd();
}

void readWriteSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *tdoData) {
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

void jtagInt(void) {
  GPIO_PinOutSet(JTAG_PORT, JTAG_SEL_BIT);
}

void jtagExt(void) {
  GPIO_PinOutClear(JTAG_PORT, JTAG_SEL_BIT);
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
  if(magic == MAGIC) {
    printf("Got FPGA magic number %x\n", magic);
  } else {
    printf("Got incorrect FPGA magic number '%x'\n", magic);
    initOk = FPGA_SPI_FAILED;
    return false;
  }

#else

  GPIO_PinModeSet(JTAG_PORT, JTAG_SEL_BIT, gpioModePushPull, JTAG_INT);

#endif

  //////////////////////////////////////////////////////////////////////////////
  // jtag mux

  jtagExt();

  return true;
}

uint32_t fpgaInitOk(void) {
  return initOk;
}
