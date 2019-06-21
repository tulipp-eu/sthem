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
#include "arm.h"
#include "jtag.h"
#include "jtag_lowlevel.h"

#include <stdio.h>
#include <string.h>
#include <em_usart.h>
#include <em_prs.h>
#include <stdlib.h>

#define PRS_CHANNEL 1
#define PRS_CHANNEL_MASK 2

#define USE_USART

//#define DUMP_PINS

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

void readWriteSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *tdoData) {
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

///////////////////////////////////////////////////////////////////////////////

#define LYNSYN_V2
#define FPGA_CONFIGURE_TIMEOUT 96000000 // in cycles

bool jtagInitLowLevel(void) {
#ifdef LYNSYN_V2
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
      printf("Can't configure the FPGA\n");
      return false;
    }
  } while(!done);

  GPIO_PinModeSet(JTAG_PORT, JTAG_SEL_BIT, gpioModePushPull, JTAG_INT);
#endif

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

  return true;
}
