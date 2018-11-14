/**************************************************************************//**
 * @file boot.c
 * @brief Boot Loader
 * @author Silicon Labs
 * @version 1.22
 ******************************************************************************
 * @section License
 * <b>Copyright 2015 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "em_device.h"
#include "boot.h"
#include "flash.h"

#include "flash_functions.h"
#include "usbprotocol.h"

extern int Copy_fromNAND2Internal(uint32_t Flash_ofset ,  uint8_t *inBuffer );

uint8_t inBuffer2[512];

void BOOT_boot(void) {
  uint32_t pc, sp;

  /* Disable all interrupts. */
  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICER[1] = 0xFFFFFFFF;

  /* Set new vector table pointer */
  SCB->VTOR = (uint32_t)MAIN_START;

  /* Read new SP and PC from vector table */
  sp = *((uint32_t *)MAIN_START);
  pc = *((uint32_t *)MAIN_START + 1);

  /* Set new MSP, PSP based on SP */
  asm("msr msp, %[sp]" :: [sp] "r" (sp));
  asm("msr psp, %[sp]" :: [sp] "r" (sp));
  /* Jump to PC */
  asm("mov pc, %[pc]" :: [pc] "r" (pc));
}

int Cmp_NANDflashInternal(uint32_t Flash_ofset ,  uint8_t *inBuffer )
{
	/*
	compare nand content with internal flash to check if its need copy or last data stand
	*/
	FLASH_init();
	uint8_t * pointer_internal= (uint8_t *)0x10000;
	uint32_t i;
 int res;
 uint32_t crc = 0 ;
 int nPage = 0 ;
 res =  Read_Page( Flash_ofset/512 ,inBuffer) ;
 if(res<1)return -1 ;
 NANDflashInit();
 struct Flash_Struct * header =( struct Flash_Struct * ) inBuffer ;
 uint32_t Header_dataCRC=header->DataCRC;
 uint16_t Header_version=header->version;
 if( header->version == 0xffff )
 {
	 // no valid version find 
  return -2;
 }
 //check  header->HeaderCRC
 crc = CRC32(  crc , (uint32_t * )inBuffer , sizeof(struct Flash_Struct)/sizeof(uint32_t )-1 ) ;
 if( crc != header->HeaderCRC )
 {
	 // header crc not confirm
  return -3 ;
 }
 if( header->Datalength > 0x200000 )
 {
	 // data length is too big
  return -4 ;
 }
 long length  = header->Datalength ;//+sizeof(struct Flash_Struct) ;
 nPage = (  length + 512 - 1  ) / 512 ;
 crc = 0 ;
 for( i  =  1 ; i<= nPage ; i++ )
  {
   res =  Read_Page( Flash_ofset/512 +  i  ,inBuffer) ;
   if(res<1)return -1 ;
   uint32_t base_add = (uint32_t )pointer_internal+(i-1)*512 ;
  // uint32_t *pointer_inbuffer = (uint32_t*)inBuffer ;
   res=memcmp((uint32_t *)base_add, inBuffer, 512);
 //  FLASH_writeBlock(pointer_internal,(i-1)*512,512,inBuffer);
	if (res!=0)
	{
		// not equal
		//its need to copy from external 
		Copy_fromNAND2Internal(Flash_ofset,inBuffer);
		return 1;
	}
   crc = CRC32( crc , (uint32_t * )inBuffer , 512/4  ) ;
  }
 if( crc != Header_dataCRC )
 {
	 // crc error
  return -5 ;
 }
	if (res==0)
	{
		printf("*** same version is used \n" );
		BOOT_boot();
	}
 return Header_version;
}

int Copy_fromNAND2Internal(uint32_t Flash_ofset ,  uint8_t *inBuffer )
{
	FLASH_init();
	uint8_t * pointer_internal= (uint8_t *)0x10000;
	uint32_t i;
 int res;
 uint32_t crc = 0 ;
 int nPage = 0 ;
 res =  Read_Page( Flash_ofset/512 ,inBuffer) ;
 if(res<1)return -1 ;
 NANDflashInit();
 struct Flash_Struct * header =( struct Flash_Struct * ) inBuffer ;
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
 }
 long length  = header->Datalength ;//+sizeof(struct Flash_Struct) ;
 nPage = (  length + 512 - 1  ) / 512 ;
 crc = 0 ;
 for(i=(uint32_t)pointer_internal;i<0x100000;i+=4096)
 {
	 // erace internal flash
	 FLASH_eraseOneBlock(i);
 }
// crc = CRC32( crc , (uint32_t * )inBuffer , 512  ) ;
 for( i  =  1 ; i<= nPage ; i++ )
  {
   res =  Read_Page( Flash_ofset/512 +  i  ,inBuffer) ;
   if(res<1)return -1 ;
   uint32_t base_add = (uint32_t )pointer_internal+(i-1)*512 ;
   uint32_t *pointer_inbuffer = (uint32_t*)inBuffer ;
   for(int j = 0 ; j < 512/4 ; j++ )
   {
	   // cope to internal flash
     FLASH_writeWord(base_add  + 4*j, pointer_inbuffer[j]);
   }
   crc = CRC32( crc , (uint32_t * )inBuffer , 512/4  ) ;
  }

 if( crc != Header_dataCRC )
 {
	 //check data crc 
  return -5 ;
 }
 BOOT_boot();
 return Header_version;
}

void Select_Application()
{
	// select which Application is new
	struct BootReplyPackage  boot_reply_package;
	Fill_boot_reply_package( &boot_reply_package,inBuffer2 );
	printf("*** MCU boot( current Version) %i\n" ,boot_reply_package.boot_version1 );
	printf("*** MCU boot( current Version) %i\n" ,boot_reply_package.boot_version2 );
	if(boot_reply_package.boot_version2==boot_reply_package.boot_version1)
	{
		// if no version valid , just blink
		long j ;
		while(1) 
		{
		//off
		clearLed(0);
		setLed(1);
		for( j = 0 ; j < 1000000 ; j++ );
		setLed(0);
		//on
		clearLed(1);
		for( j = 0 ; j < 1000000 ; j++ );
		}
	}
	else if(boot_reply_package.boot_version2>boot_reply_package.boot_version1)
	{
		Cmp_NANDflashInternal(Flash_offset2,inBuffer2);
	}
	else
	{
		Cmp_NANDflashInternal(Flash_offset1,inBuffer2);
	}
}
