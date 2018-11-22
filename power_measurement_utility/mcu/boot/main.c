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

int Copy_UpgradeApplication() {
	uint8_t * pointer_internal= (uint8_t *)FLASH_APP_START;
	uint32_t i;

  // erase flash
  for(i=(uint32_t)pointer_internal;i<FLASH_NEW_APPLICATION_START;i+=4096) {
    MSC_ErasePage((void*)i);
  }
  uint32_t *base_add = (uint32_t *)FLASH_APP_START ;
  uint32_t *pointer_inbuffer = (uint32_t *)FLASH_NEW_APPLICATION_START ;

  // copy to internal flash
  MSC_WriteWord(base_add, pointer_inbuffer, FLASH_PARAMETERS_START-FLASH_NEW_APPLICATION_START);

  return 1;
}

int check_for_NewUpgrade()
{
	uint32_t *inBuffer = (uint32_t*)FLASH_PARAMETERS_START;
 return inBuffer[0];
}

void boot(void) {
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

void Select_Application()
{
	int32_t NewApp = check_for_NewUpgrade();
	if(NewApp==NEW_APP_MAGIC)
	{
    MSC_Init();

    printf("Upgrading firmware...\n");
		Copy_UpgradeApplication();
		uint32_t *base_add =  (uint32_t*)FLASH_NEW_APP_FLAG;

    MSC_ErasePage(base_add);
    MSC_WriteWord(base_add, 0, 4);

    MSC_Deinit();
	}
//	uint32_t App = check_for_Application();
	//if(App==0xffffffff)
	{
	//	return ;
	}
	//else
	{
		boot();
	}
}

int main(void) {
  CHIP_Init();
   
  // setup clocks
  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // setup LEDs
  GPIO_PinModeSet(LED0_PORT, LED0_BIT, gpioModePushPull, LED_ON);
  GPIO_PinModeSet(LED1_PORT, LED1_BIT, gpioModePushPull, LED_ON);

#ifdef SWO
  swoInit();
#endif

  printf("Lynsyn bootloader %s\n", BOOT_VERSION_STRING);

  Select_Application();
}
