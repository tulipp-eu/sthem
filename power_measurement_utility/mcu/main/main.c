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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <em_device.h>
#include <em_chip.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <em_emu.h>

#include "../common/swo.h"
#include "../common/usbprotocol.h"

#include "adc.h"
#include "config.h"
#include "usb.h"
#include "fpga.h"
#include "jtag.h"
#include "iic.h"

volatile bool sampleMode;
volatile bool samplePc;
volatile bool gpioMode;
volatile bool useStartBp;
volatile bool useStopBp;
volatile int64_t sampleStop;

volatile uint32_t lastLowWord = 0;
volatile uint32_t highWord = 0;

static struct SampleReplyPacket sampleBuf1[MAX_SAMPLES] __attribute__((__aligned__(4)));
static struct SampleReplyPacket sampleBuf2[MAX_SAMPLES] __attribute__((__aligned__(4)));

#ifndef USE_SWO

static int16_t i2cCurrent[7];

#endif

///////////////////////////////////////////////////////////////////////////////

void panic(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("\nPanic: ");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  jtagExt();
  while(true);
}

///////////////////////////////////////////////////////////////////////////////

int main(void) {
  sampleMode = false;

  // setup clocks
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_USB, true);
  CMU_ClockEnable(cmuClock_ADC0, true);
#ifndef USE_SWO
  CMU_ClockEnable(cmuClock_I2C0, true);
  CMU_ClockEnable(cmuClock_CORELE, true);
#endif

  printf("Lynsyn initializing...\n");

  // Enable cycle counter
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= 0x01; 
  DWT->CYCCNT = 0;

  configInit();

  printf("Hardware V%lx.%lx Firmware %s\n", (getUint32("hwver") & 0xf0) >> 4, getUint32("hwver") & 0xf, SW_VERSION_STRING);

  fpgaInit();
  adcInit();
  usbInit();
  jtagInit();

#ifndef USE_SWO
  GPIO_PinModeSet(TRIGGER_IN_PORT, TRIGGER_IN_BIT, gpioModeInput, 0);

  i2cInit();
#endif

  clearLed(0);
  clearLed(1);

  printf("Ready.\n");

  int samples = 0;

  unsigned currentSample = 0;

  struct SampleReplyPacket *sampleBuf = sampleBuf1;

  // main loop
  while(true) {
    if(sampleMode) {
      int64_t currentTime = calculateTime();
      struct SampleReplyPacket *samplePtr = &sampleBuf[currentSample];

      samplePtr->flags = 0;

      bool halted = false;

#ifndef USE_SWO
      bool send = !gpioMode;

      if(gpioMode && !GPIO_PinInGet(TRIGGER_IN_PORT, TRIGGER_IN_BIT)) {
        if(useStopBp) {
          halted = coreHalted(stopCore);
        } else {
          halted = currentTime >= sampleStop;
        }

      } else
#endif
      {
        samplePtr->time = currentTime;

        adcScan(samplePtr->current);
        if(samplePc) {
          halted = coreReadPcsrFast(samplePtr->pc);
          if(halted) {
            if(readPc(0) == frameBp) {
              coreClearBp(startCore, FRAME_BP);
              coresResume();
              halted = false;
              samplePtr->flags = SAMPLE_REPLY_FLAG_FRAME_DONE;
              coreSetBp(startCore, FRAME_BP, frameBp);
            }
          }
        } else {
          for(int i = 0; i < 4; i++) {
            samplePtr->pc[i] = 0;
          }
        }

        if(!useStopBp) {
          halted = currentTime >= sampleStop;
        }

        adcScanWait();

#ifndef USE_SWO
        if(GPIO_PinInGet(TRIGGER_IN_PORT, TRIGGER_IN_BIT)) {
          send = true;
        }
#endif
      }

      if(halted) {
#ifndef USE_SWO
        send = true;
#endif
        samplePtr->time = -1;
        sampleMode = false;

        if(useStopBp) {
          coreClearBp(stopCore, STOP_BP);
          coresResume();
        }

        jtagExt();

        clearLed(0);

        printf("Exiting sample mode, %d samples\n", samples);

        samples = 0;
      }

#ifndef USE_SWO
      if(send) {
#else
      {
#endif
        samples++;
        currentSample++;

        if(samplePtr->flags & SAMPLE_REPLY_FLAG_FRAME_DONE) samplePtr->pc[0] = calculateTime();

        if((currentSample >= MAX_SAMPLES) || halted) {
          sendSamples(sampleBuf, currentSample);
          if(sampleBuf == sampleBuf1) sampleBuf = sampleBuf2;
          else sampleBuf = sampleBuf1;
          currentSample = 0;
        }
      }
#ifndef USE_SWO
    } else {
      adcScan(i2cCurrent);
      adcScanWait();

      __disable_irq();
      for(int i = 0; i < 7; i++) {
        i2cCurrentAcc[i] += i2cCurrent[i];
        i2cSamplesSinceLast[i]++;
        i2cCurrentInstant[i] = i2cCurrent[i];
      }
      __enable_irq();
#endif      
    }
  } 
}

int64_t calculateTime() {
  uint32_t lowWord = DWT->CYCCNT;
  __disable_irq();
  if(lowWord < lastLowWord) highWord++;
  lastLowWord = lowWord;
  __enable_irq();
  return ((uint64_t)highWord << 32) | lowWord;
}

int _write(int fd, char *str, int len) {
#ifdef USE_SWO
  for (int i = 0; i < len; i++) {
    ITM_SendChar(str[i]);
  }
#endif
  return len;
}

