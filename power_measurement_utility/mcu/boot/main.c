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

#include "../common/swo.h"

#include "../external/flash.h"

///////////////////////////////////////////////////////////////////////////////

int Copy_UpgradeApplication()
{
	FLASH_init();
	uint8_t * pointer_internal= (uint8_t *)FLASH_BOOT_END;
	uint32_t i;
  //int res;
 //uint32_t crc = 0 ;
 // int nPage = 0 ;
 //res =  Read_Page( Flash_ofset/512 ,inBuffer) ;
// if(res<1)return -1 ;
 //NANDflashInit();
/* struct Flash_Struct * header =( struct Flash_Struct * ) inBuffer ;
 uint32_t Header_dataCRC=header->DataCRC;
 uint16_t Header_version=header->version;
 if( header->version == 0xffff )
 {
	 // versin not valid 
  return -2;
 }
 //check  header->HeaderCRC
 crc = CRC32(  crc , (uint32_t * )inBuffer , sizeof(struct Flash_Struct)/sizeof(uint32_t )-1 ) ;
 if( crc != header->HeaderCRC )
 {
	 // header crc not valid
  return -3 ;
 }
 if( header->Datalength > 0x200000 )
 {
	 // data length is too big , not support
  return -4 ;
 } */
// long length  = header->Datalength ;//+sizeof(struct Flash_Struct) ;
 //nPage = (  length + 512 - 1  ) / 512 ;
  // crc = 0 ;
 for(i=(uint32_t)pointer_internal;i<FLASH_NEW_APPLICATION_START;i+=4096)
 {
	 // erace internal flash
	 FLASH_eraseOneBlock(i);
 }
// crc = CRC32( crc , (uint32_t * )inBuffer , 512  ) ;
 //for( i  =  1 ; i<= nPage ; i++ )
  //{
   //res =  Read_Page( Flash_ofset/512 +  i  ,inBuffer) ;
  // if(res<1)return -1 ;
   uint32_t base_add = (uint32_t )FLASH_BOOT_END ;
   uint32_t *pointer_inbuffer = (uint32_t*)FLASH_NEW_APPLICATION_START ;
   for(int j = 0 ; j < (FLASH_NEW_APPLICATION_END-FLASH_NEW_APPLICATION_START)/4 ; j++ )
   {
	   // cope to internal flash
     FLASH_writeWord(base_add  + 4*j, pointer_inbuffer[j]);
   }
  // crc = CRC32( crc , (uint32_t * )inBuffer , 512/4  ) ;
 // }

// if( crc != Header_dataCRC )
// {
	 //check data crc 
 // return -5 ;
 //}
 //BOOT_boot();
 return 1;
}

int check_for_NewUpgrade()
{
	uint32_t *inBuffer = (uint32_t*)FLASH_NEW_APPLICATION_END;
 return inBuffer[0];
}

void boot(void) {
  uint32_t pc, sp;

  /* disable interrupts. */
  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICER[1] = 0xFFFFFFFF;

  /* set vector table pointer */
  SCB->VTOR = (uint32_t)FLASH_BOOT_END;

  /* read SP and PC from vector table */
  sp = *((uint32_t *)FLASH_BOOT_END);
  pc = *((uint32_t *)FLASH_BOOT_END + 1);

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
    printf("Upgrading firmware...\n");
		Copy_UpgradeApplication();
		uint32_t base_add =  FLASH_NEW_APPLICATION_END;
		FLASH_writeWord(base_add , 0);
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
