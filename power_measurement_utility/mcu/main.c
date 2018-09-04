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
#include <em_i2c.h>

#include "adc.h"
#include "config.h"
#include "usb.h"
#include "fpga.h"
#include "swo.h"
#include "jtag.h"
#include "usbprotocol.h"

volatile bool sampleMode;
volatile bool samplePc;
volatile bool gpioMode;
volatile bool useBp;
volatile int64_t sampleStop;

volatile uint32_t lastLowWord = 0;
volatile uint32_t highWord = 0;

static struct SampleReplyPacket sampleBuf1[MAX_SAMPLES] __attribute__((__aligned__(4)));
static struct SampleReplyPacket sampleBuf2[MAX_SAMPLES] __attribute__((__aligned__(4)));

#ifndef SWO

#define I2C_NO_CMD       0
#define I2C_MAGIC        1
#define I2C_READ_CURRENT 2

static uint8_t i2cCommand;
static uint8_t i2cSensors;

static int16_t i2cCurrent[7];
static int16_t i2cCurrentAvg[7];
static uint8_t *i2cSendBufPtr;

static int i2cIdx;

static int i2cSamplesSinceLast[7];
static uint64_t i2cCurrentAcc[7];

#endif

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

#ifndef SWO

void i2cInit(void) {
  CMU_ClockEnable(cmuClock_I2C0, true);  

  GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0;

  GPIO->ROUTE = GPIO->ROUTE & ~GPIO_ROUTE_SWCLKPEN;
  GPIO->ROUTE = GPIO->ROUTE & ~GPIO_ROUTE_SWDIOPEN;

  GPIO_PinModeSet(I2C_SCL_PORT, I2C_SCL_PIN, gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(I2C_SDA_PORT, I2C_SDA_PIN, gpioModeWiredAndPullUp, 1);

  I2C0->ROUTE = I2C_ROUTE_SDAPEN | I2C_ROUTE_SCLPEN |
    (I2C_LOCATION << _I2C_ROUTE_LOCATION_SHIFT);

  /* Initializing the I2C */
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
  i2cInit.master = false;
  I2C_Init(I2C0, &i2cInit);
  
  /* Setting up to enable slave mode */
  I2C0->SADDR = I2C_ADDRESS;
  I2C0->SADDRMASK = 0x7f;
  I2C0->CTRL |= I2C_CTRL_SLAVE | I2C_CTRL_AUTOACK;

  if(I2C0->STATE & I2C_STATE_BUSY) {
    I2C0->CMD = I2C_CMD_ABORT;
  }

  I2C_IntClear(I2C0, I2C_IEN_ADDR | I2C_IEN_RXDATAV | I2C_IEN_SSTOP | I2C_IEN_ACK);  
  I2C_IntEnable(I2C0, I2C_IEN_ADDR | I2C_IEN_RXDATAV | I2C_IEN_SSTOP | I2C_IEN_ACK);

  NVIC_EnableIRQ(I2C0_IRQn);    
}

void i2cSendData(void) {
  switch(i2cCommand) {

    case I2C_READ_CURRENT:
      while(!(i2cSensors & (1 << (i2cIdx/2))) && (i2cIdx < 14)) {
        i2cIdx += 2;
      }

      if(i2cIdx < 14) {
        I2C0->TXDATA = i2cSendBufPtr[i2cIdx++];
      } else {
        I2C0->TXDATA = 0;
      }
      break;

    case I2C_MAGIC:
      I2C0->TXDATA = 0xad;

    default:
    case I2C_NO_CMD:
      I2C0->TXDATA = 0;

      break;
  }
}

void I2C0_IRQHandler(void) {
  int status;
   
  status = I2C0->IF;

  if(status & I2C_IF_ADDR) {
    /* address match */ 
    uint8_t addr = I2C0->RXDATA;

    i2cIdx = 0;

    if(addr & 1) {
      if(i2cCommand == I2C_READ_CURRENT) {
        for(int i = 0; i < 7; i++) {
          i2cCurrentAvg[i] = i2cCurrentAcc[i] / i2cSamplesSinceLast[i];
          i2cCurrentAcc[i] = 0;
          i2cSamplesSinceLast[i] = 0;
        }

        i2cSendBufPtr = (uint8_t*)i2cCurrentAvg;
      }
      i2cSendData();
    }

    I2C_IntClear(I2C0, I2C_IFC_ADDR);

  } else if(status & I2C_IF_RXDATAV) {
    /* data received */
    switch(i2cIdx) {
      case 0:
        i2cCommand = I2C0->RXDATA;
        break;
      case 1:
        if(i2cCommand == I2C_READ_CURRENT) i2cSensors = I2C0->RXDATA;
        break;
      default:
        I2C0->RXDATA;
        break;
    }
    i2cIdx++;
  }

  if(status & I2C_IEN_ACK) {
    /* need to write data */
    i2cSendData();
    I2C_IntClear(I2C0, I2C_IFC_ACK);
  }

  if(status & I2C_IEN_SSTOP) {
    /* stop received */
    I2C_IntClear(I2C0, I2C_IEN_SSTOP);
  }
}

#endif

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
  CMU_ClockEnable(cmuClock_I2C0, true);
  CMU_ClockEnable(cmuClock_CORELE, true);

  // setup LEDs
  GPIO_PinModeSet(LED0_PORT, LED0_BIT, gpioModePushPull, LED_ON);
  GPIO_PinModeSet(LED1_PORT, LED1_BIT, gpioModePushPull, LED_ON);

#ifdef SWO
  swoInit();
#endif

  printf("Lynsyn firmware %s initializing...\n", SW_VERSION_STRING);

  // Enable cycle counter
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= 0x01; 
  DWT->CYCCNT = 0;

  configInit();
  fpgaInit();
  adcInit();
  usbInit();
  jtagInit();

#ifndef SWO
  GPIO_PinModeSet(TRIGGER_IN_PORT, TRIGGER_IN_BIT, gpioModeInput, 0);
  while(DWT->CYCCNT < BOOT_DELAY);
  i2cInit();
#endif

  clearLed(0);
  clearLed(1);

  printf("Ready.\n");

  int samples = 0;
  int64_t startTime = 0;

  unsigned currentSample = 0;

  struct SampleReplyPacket *sampleBuf = sampleBuf1;

  // main loop
  while(true) {
    int64_t currentTime = calculateTime();

    struct SampleReplyPacket *samplePtr = &sampleBuf[currentSample];

    if(sampleMode) {
      bool halted = false;

#ifndef SWO
      bool send = !gpioMode;

      if(gpioMode && !GPIO_PinInGet(TRIGGER_IN_PORT, TRIGGER_IN_BIT)) {
        if(samplePc && useBp) {
          halted = coreHalted();
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
          if(!useBp) {
            halted = currentTime >= sampleStop;
          }
        } else {
          for(int i = 0; i < 4; i++) {
            samplePtr->pc[i] = 0;
            halted = currentTime >= sampleStop;
          }
        }
        adcScanWait();

#ifndef SWO
        if(GPIO_PinInGet(TRIGGER_IN_PORT, TRIGGER_IN_BIT)) {
          send = true;
        }
#endif
      }

      if(halted) {
#ifndef SWO
        send = true;
#endif
        samplePtr->time = -1;
        sampleMode = false;
        jtagExt();

        clearLed(0);

        double totalTime = (currentTime - startTime) / CLOCK_FREQ;
        printf("Exiting sample mode, %d samples (%d per second)\n", samples, (unsigned)(samples/totalTime));

        samples = 0;
      }

#ifndef SWO
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
#ifndef SWO
    } else {
      adcScan(i2cCurrent);
      adcScanWait();

      __disable_irq();
      for(int i = 0; i < 7; i++) {
        i2cCurrentAcc[i] += i2cCurrent[i];
        i2cSamplesSinceLast[i]++;
      }
      __enable_irq();
#endif      
    }
  } 
}

int64_t calculateTime() {
  uint32_t lowWord = DWT->CYCCNT;
  if(lowWord < lastLowWord) highWord++;
  lastLowWord = lowWord;
  return ((uint64_t)highWord << 32) | lowWord;
}
