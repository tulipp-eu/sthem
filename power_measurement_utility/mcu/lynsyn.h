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

#ifndef LYNSYN_H
#define LYNSYN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#include "efm32gg332f1024.h"

#include "em_device.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "em_system.h"
#include "em_chip.h"
 
///////////////////////////////////////////////////////////////////////////////
// firmware settings

#define SWO

#define CAL_AVERAGE_SAMPLES 100

///////////////////////////////////////////////////////////////////////////////
// lynsyn settings

#define CLOCK_FREQ 48000000

#define BOOT_DELAY (3*CLOCK_FREQ)

#define I2C_ADDRESS  0xaa
#define I2C_SCL_PORT gpioPortF
#define I2C_SCL_PIN  1
#define I2C_SDA_PORT gpioPortF
#define I2C_SDA_PIN  0
#define I2C_LOCATION 5

#define TRIGGER_IN_BIT 2
#define TRIGGER_IN_PORT gpioPortF

#define LED_ON  0
#define LED_OFF 1

#define LED0_BIT 0
#define LED0_PORT gpioPortC

#define LED1_BIT 1
#define LED1_PORT gpioPortC

#define PROGRAM_B_BIT 1
#define PROGRAM_B_PORT gpioPortA

#define DONE_BIT 3
#define DONE_PORT gpioPortA

#define CLK_OUT_BIT 2
#define CLK_OUT_PORT gpioPortA

#define JTAG_USART USART0
#define JTAG_USART_CLK cmuClock_USART0
#define JTAG_USART_LOC USART_ROUTE_LOCATION_LOC0

#define JTAG_PORT gpioPortE

#define JTAG_TDI_BIT 10
#define JTAG_TDO_BIT 11
#define JTAG_TCK_BIT 12
#define JTAG_TMS_BIT 13
#define JTAG_SEL_BIT 14

#define JTAG_EXT 0
#define JTAG_INT 1

#define FPGA_USART JTAG_USART
#define FPGA_USART_CLK JTAG_USART_CLK
#define FPGA_USART_LOC JTAG_USART_LOC

#define FPGA_PORT JTAG_PORT

#define FPGA_TX_BIT  10
#define FPGA_RX_BIT  11
#define FPGA_CLK_BIT 12
#define FPGA_CS_BIT  13
#define FPGA_INT_BIT 14

///////////////////////////////////////////////////////////////////////////////
// global functions

void setLed(int led);
void clearLed(int led);

int64_t calculateTime();

void panic(const char *fmt, ...);

///////////////////////////////////////////////////////////////////////////////
// global variables

extern volatile bool sampleMode;
extern volatile bool samplePc;
extern volatile bool gpioMode;
extern volatile bool useStartBp;
extern volatile bool useStopBp;
extern volatile int64_t sampleStop;
extern uint8_t startCore;
extern uint8_t stopCore;

#endif
