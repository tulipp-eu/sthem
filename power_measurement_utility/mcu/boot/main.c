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

#include "lynsyn_boot.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <em_chip.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <em_msc.h>

#include "../common/swo.h"

static uint32_t bootVersion __attribute__ ((section (".version"))) __attribute__ ((__used__)) = BOOT_VERSION;

///////////////////////////////////////////////////////////////////////////////

static void boot(void) {
  uint32_t pc, sp;

  /* disable interrupts. */
  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICER[1] = 0xFFFFFFFF;

  /* set vector table pointer */
  SCB->VTOR = (uint32_t)FLASH_APP_START;

  /* read SP and PC from vector table */
  sp = *((uint32_t *)FLASH_APP_START);
  pc = *((uint32_t *)FLASH_APP_START + 1);

  /* set MSP and PSP based on SP */
  asm("msr msp, %[sp]" :: [sp] "r" (sp));
  asm("msr psp, %[sp]" :: [sp] "r" (sp));

  /* jump */
  asm("mov pc, %[pc]" :: [pc] "r" (pc));
}

static void upgradeFirmware() {
	int32_t newApp = *(uint32_t*)FLASH_UPGRADE_FLAG;
	if(newApp == UPGRADE_MAGIC) {
    MSC_Init();

    printf("Upgrading firmware...\n");

    // erase flash
    for(unsigned i = FLASH_APP_START; i < FLASH_UPGRADE_START; i += 4096) {
      MSC_ErasePage((void*)i);
    }

    // copy to application flash area
    MSC_WriteWord((uint32_t *)FLASH_APP_START, (uint32_t *)FLASH_UPGRADE_START, FLASH_PARAMETERS_START - FLASH_UPGRADE_START);

    // reset upgrade flag
		uint32_t *upgrade =  (uint32_t*)FLASH_UPGRADE_FLAG;
    MSC_ErasePage(upgrade);
    MSC_WriteWord(upgrade, 0, 4);

    MSC_Deinit();
	}
}

///////////////////////////////////////////////////////////////////////////////

int _write(int fd, char *str, int len) {
#ifdef USE_SWO
  for (int i = 0; i < len; i++) {
    ITM_SendChar(str[i]);
  }
#endif
  return len;
}

int main(void) {
  CHIP_Init();
   
  // setup clocks
  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // setup LEDs
  GPIO_PinModeSet(LED0_PORT, LED0_BIT, gpioModePushPull, LED_ON);
  //GPIO_PinModeSet(LED1_PORT, LED1_BIT, gpioModePushPull, LED_ON);

#ifdef USE_SWO
  swoInit();
#endif

  // communicate version to main
  *(uint32_t*)FLASH_BOOT_VERSION = BOOT_VERSION;

  printf("Lynsyn bootloader %s\n", BOOT_VERSION_STRING);

  printf("Checking for new firmware...\n");
  upgradeFirmware();

  printf("Booting...\n");
  boot();
}
