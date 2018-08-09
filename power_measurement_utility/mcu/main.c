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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <em_device.h>
#include <em_chip.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <em_emu.h>

#include "adc.h"
#include "config.h"
#include "usb.h"
#include "fpga.h"
#include "swo.h"
#include "jtag.h"
#include "usbprotocol.h"

volatile bool sampleMode;

static struct SampleReplyPacket sampleBuf1[MAX_SAMPLES] __attribute__((__aligned__(4)));
static struct SampleReplyPacket sampleBuf2[MAX_SAMPLES] __attribute__((__aligned__(4)));

///////////////////////////////////////////////////////////////////////////////

void clearLed(int led) {
  switch(led) {
    case 0:
      GPIO_PinOutSet(LED0_PORT, LED0_BIT);
      break;
    case 1:
      GPIO_PinOutSet(LED1_PORT, LED1_BIT);
      break;
  }
}

void setLed(int led) {
  switch(led) {
    case 0:
      GPIO_PinOutClear(LED0_PORT, LED0_BIT);
      break;
    case 1:
      GPIO_PinOutClear(LED1_PORT, LED1_BIT);
      break;
  }
}

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
  CHIP_Init();
   
  sampleMode = false;

  // setup clocks
  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_USB, true);
  CMU_ClockEnable(cmuClock_ADC0, true);

#ifdef TRIGGER_INPUT
  GPIO_PinModeSet(TRIGGER_IN_PORT, TRIGGER_IN_BIT, gpioModeInput, 0);
#else
  swoInit();
#endif

  printf("Lynsyn firmware %s initializing...\n", SW_VERSION_STRING);

  // setup LEDs
  GPIO_PinModeSet(LED0_PORT, LED0_BIT, gpioModePushPull, LED_ON);
  GPIO_PinModeSet(LED1_PORT, LED1_BIT, gpioModePushPull, LED_ON);

  // Enable cycle counter
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= 0x01; 
  DWT->CYCCNT = 0;

  configInit();
  fpgaInit();
  adcInit();
  usbInit();
  jtagInit();

  clearLed(0);
  clearLed(1);

  printf("Ready.\n");

  uint32_t lastLowWord = 0;
  uint32_t highWord = 0;

  int samples = 0;
  int64_t startTime = 0;

  unsigned currentSample = 0;

  struct SampleReplyPacket *sampleBuf = sampleBuf1;

  // main loop
  while(true) {
    uint32_t lowWord = DWT->CYCCNT;
    if(lowWord < lastLowWord) highWord++;
    lastLowWord = lowWord;

    struct SampleReplyPacket *samplePtr = &sampleBuf[currentSample];

    if(sampleMode) {
      bool halted = false;

#ifdef TRIGGER_INPUT
      bool send = false;

      if(!GPIO_PinInGet(TRIGGER_IN_PORT, TRIGGER_IN_BIT)) {
        halted = coreHalted();

      } else
#endif
      {
        samplePtr->time = ((uint64_t)highWord << 32) | lowWord;

        adcScan(samplePtr->current);
        halted = coreReadPcsrFast(samplePtr->pc);
        adcScanWait();

#ifdef TRIGGER_INPUT
        if(GPIO_PinInGet(TRIGGER_IN_PORT, TRIGGER_IN_BIT)) {
          send = true;
        }
#endif
      }

      if(halted) {
#ifdef TRIGGER_INPUT
        send = true;
#endif
        samplePtr->time = -1;
        clearLed(0);
        sampleMode = false;
        jtagExt();

        int64_t stopTime = ((uint64_t)highWord << 32) | lowWord;
        double totalTime = (stopTime - startTime) / CLOCK_FREQ;
        printf("Exiting sample mode, %d samples (%d per second)\n", samples, (unsigned)(samples/totalTime));

        samples = 0;
      }

#ifdef TRIGGER_INPUT
      if(send) {
#else
      {
#endif
        samples++;
        currentSample++;

        if((currentSample >= MAX_SAMPLES) || halted) {
          sendSamples(sampleBuf, currentSample);
          if(sampleBuf == sampleBuf1) sampleBuf = sampleBuf2;
          else sampleBuf = sampleBuf1;
          currentSample = 0;
        }
      }

    } else {
      startTime = ((uint64_t)highWord << 32) | lowWord;
      currentSample = 0;
    }
  }
}
