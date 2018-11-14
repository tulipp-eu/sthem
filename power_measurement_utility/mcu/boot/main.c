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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <em_device.h>
#include <em_chip.h>
#include <em_cmu.h>
#include <em_emu.h>

#include "lynsyn_boot.h"
#include "swo.h"
#include "flash_functions.h"

///////////////////////////////////////////////////////////////////////////////

int main(void) {
  CHIP_Init();
   
  // setup clocks
  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  CMU_ClockEnable(cmuClock_GPIO, true);

  swoInit();

  printf("\nLynsyn bootloader %s initializing...\n", BOOT_VERSION_STRING);

  // setup LEDs
  GPIO_PinModeSet(LED0_PORT, LED0_BIT, gpioModePushPull, LED_ON);
  GPIO_PinModeSet(LED1_PORT, LED1_BIT, gpioModePushPull, LED_ON);

  clearLed(0);

  printf("Checking for new firmware...\n");

  printf("NOT IMPLEMENTED\n");

  printf("Booting firmware...\n\n");
}
