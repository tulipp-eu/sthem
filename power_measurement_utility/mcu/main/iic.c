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

#include "iic.h"
#include "lynsyn_main.h"
#include "config.h"

static uint8_t i2cCommand;
static uint8_t i2cSensors;

static int16_t i2cCurrentAvg[7];
static double i2cCalData[14];
static uint8_t *i2cSendBufPtr;

static int i2cIdx;

uint64_t i2cCurrentAcc[7];
int i2cSamplesSinceLast[7];
int16_t i2cCurrentInstant[7];

void i2cInit(void) {
  DWT->CYCCNT = 0;
  while(DWT->CYCCNT < BOOT_DELAY);

  i2cCalData[0] = getDouble("offset0");
  i2cCalData[1] = getDouble("gain0");
  i2cCalData[2] = getDouble("offset1");
  i2cCalData[3] = getDouble("gain1");
  i2cCalData[4] = getDouble("offset2");
  i2cCalData[5] = getDouble("gain2");
  i2cCalData[6] = getDouble("offset3");
  i2cCalData[7] = getDouble("gain3");
  i2cCalData[8] = getDouble("offset4");
  i2cCalData[9] = getDouble("gain4");
  i2cCalData[10] = getDouble("offset5");
  i2cCalData[11] = getDouble("gain5");
  i2cCalData[12] = getDouble("offset6");
  i2cCalData[13] = getDouble("gain6");

  CMU_ClockEnable(cmuClock_I2C0, true);  

  GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0;

  GPIO->ROUTE = GPIO->ROUTE & ~GPIO_ROUTE_SWCLKPEN;
  GPIO->ROUTE = GPIO->ROUTE & ~GPIO_ROUTE_SWDIOPEN;

  GPIO_PinModeSet(I2C_SCL_PORT, I2C_SCL_PIN, gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(I2C_SDA_PORT, I2C_SDA_PIN, gpioModeWiredAndPullUp, 1);

  I2C0->ROUTE = I2C_ROUTE_SDAPEN | I2C_ROUTE_SCLPEN |
    (I2C_LOCATION << _I2C_ROUTE_LOCATION_SHIFT);

  /* initializing the I2C */
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
  i2cInit.master = false;
  I2C_Init(I2C0, &i2cInit);
  
  /* setting up to enable slave mode */
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

    case I2C_READ_CURRENT_AVG:
    case I2C_READ_CURRENT_INSTANT:
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
      break;

    case I2C_GET_CAL:
      I2C0->TXDATA = i2cSendBufPtr[i2cIdx++];
      break;

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

      if(i2cCommand == I2C_READ_CURRENT_AVG) {
        for(int i = 0; i < 7; i++) {
          i2cCurrentAvg[i] = i2cCurrentAcc[i] / i2cSamplesSinceLast[i];
          i2cCurrentAcc[i] = 0;
          i2cSamplesSinceLast[i] = 0;
        }

        i2cSendBufPtr = (uint8_t*)i2cCurrentAvg;

      } else if(i2cCommand == I2C_READ_CURRENT_INSTANT) {
        i2cSendBufPtr = (uint8_t*)i2cCurrentInstant;

      } else if(i2cCommand == I2C_GET_CAL) {
        i2cSendBufPtr = (uint8_t*)i2cCalData;
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
        if(i2cCommand == I2C_READ_CURRENT_AVG) i2cSensors = I2C0->RXDATA;
        if(i2cCommand == I2C_READ_CURRENT_INSTANT) i2cSensors = I2C0->RXDATA;
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

