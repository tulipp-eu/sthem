/******************************************************************************
 *
 *  This file is part of the Lynsyn PMU firmware.
 *
 *  Copyright 2018 Fatemeh Ghasemi, NTNU, TULIPP EU Project
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

#include "usbprotocol.h"
#include "flash_functions.h"
#include <string.h>
#include "../Boot/flash.h"
#include "../Boot/boot.h"
#include <stdio.h>
#include <stdbool.h>
#include "config.h"

extern int Copy_fromNAND2Internal(uint32_t Flash_ofset ,  uint8_t *inBuffer );
extern void clearLed(int led);
extern void setLed(int led);

uint32_t CRC32(uint32_t CRC , uint32_t *data , int length )
{
 int i ;
 for( i = 0 ; i < length ; i++ )
 {
  CRC = CRC + data[i] ;
 }
 return CRC ;
}
/*
int Cmp_NANDflashInternal(uint32_t Flash_ofset ,  uint8_t *inBuffer )
{

	compare nand content with internal flash to check if its need copy or last data stand

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
*/
int Copy_UpgradeApplication()
{
	FLASH_init();
	uint8_t * pointer_internal= (uint8_t *)FLASH_BOOT_END;
	uint32_t i;
 int res;
 uint32_t crc = 0 ;
 int nPage = 0 ;
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
 crc = 0 ;
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
	uint32_t *inBuffer = FLASH_NEW_APPLICATION_END;
 return inBuffer[0];
}

int check_for_Application()
{
	uint32_t *inBuffer = FLASH_BOOT_END;
 return inBuffer[0];
}
void Fill_boot_reply_package( struct BootInitReplyPackage  * boot_reply_package)
{
	boot_reply_package->hwVersion=getUint32("hwver");        // hardware_version
	boot_reply_package->swVersion=SW_VERSION;// software_version
	//BootInit_reply_package->NANDflash_version=0x7520; // nan versian
	boot_reply_package->BlVersion=2102;
	/*
 int res1 = check_flash_for_Framework(Flash_offset1 ,  inBuffer );// check nand flash Flash_offset1 section
 int res2 = check_flash_for_Framework(Flash_offset2 ,  inBuffer );// check nand flash Flash_offset2 section
 if((res1 == -1)||(res2 == -1) )
 {
	 // if none of flash section valid , return not ready
  boot_reply_package->ready = 0 ;
  return ;
 }
 if(res1<0)
 {
	 res1=0;
 }
 if(res2<0)
 {
	 res2=0;
 }
 */
	boot_reply_package->ready = 1 ;// raedy
 //boot_reply_package->boot_version1 = res1 ;
// boot_reply_package->boot_version2 = res2 ;
 return ;
}

void Flash_erase()
{
	FLASH_init();
	int i;
	uint8_t * pointer_internal= (uint8_t *)FLASH_NEW_APPLICATION_START;
	for(i=(uint32_t)pointer_internal;i<FLASH_SIZE;i+=4096)
	 {
		 // erace internal flash
		 FLASH_eraseOneBlock(i);
	 }
	// erace nand flash from flash_address_start to flash_address_end
}
//uint8_t inBuffer2[512];
int pointer=0;
uint32_t crc_mcu=0;
void reset_var()
{
	pointer=0;
	crc_mcu=0;
}
//uint32_t last_flash_Address;
 int Flash_FIFO_IN(struct FlashBootPackage  * flash_boot_package)
 {
	 int i,res=1;
	 uint32_t base_add = flash_boot_package->flash_address ;
	 uint32_t *pointer_inbuffer = (uint32_t*)flash_boot_package->Data ;


	 for(int j = 0 ; j < USB_Data_length/4 ; j++ )
	    {
	 	   // copy to internal flash
	      FLASH_writeWord(base_add  + 4*j, pointer_inbuffer[j]);
	    }
	 crc_mcu=CRC32(crc_mcu,(uint32_t * )flash_boot_package->Data,USB_Data_length/4);
	 return(res);
 }
int SaveHeaderToFlash(struct ResetPackage inBuffer)
{
	// save data header file to nand flash 
  int res = 1;
 // uint32_t crc_mcu2 = 0 ;
//  struct Flash_Struct  *fs ;
  // for(i=0;i<512;i++)
  // {
 //    inBuffer2[i]=0xff;
 //  }
  // fs = (struct Flash_Struct *)inBuffer2 ;
 //  fs->version = inBuffer.version;
 //  fs->Reserve0 = 0 ;
 //  fs->Reserve1=0;
  // fs->Reserve2=0;
   if( crc_mcu != inBuffer.crc )
   {
	   // chech data crc 
     return -1 ;
   }
 //  fs->DataCRC = inBuffer.crc;
 //  fs->Datalength= inBuffer.dataLength;
 //  fs->HeaderCRC=0;
 //  crc_mcu2 = 0 ;
  // inBuffer.request.cmd=0;
 // crc_mcu2 = CRC32( crc_mcu2 , (uint32_t *)&inBuffer , sizeof(struct ResetPackage)/4 -1 );
 // if(crc_mcu2 != inBuffer.crc_header)
//  {
	  // che crc of data come from host
 //   return -2;
 // }

  // crc_mcu2 = 0 ;
  // crc_mcu2 = CRC32( crc_mcu2 , (uint32_t *)fs , sizeof(struct Flash_Struct)/4 );
  // fs->HeaderCRC = crc_mcu2 ;

  // uint32_t base_add = FLASH_NEW_APPLICATION_END ;
 //  uint32_t *pointer_inbuffer = (uint32_t*)inBuffer2;
  // pointer_inbuffer[0]=1;

   	// for(int j = 0 ; j < 512/4 ; j++ )
   	//    {
   	 	   // copy to internal flash
   	      FLASH_writeWord(FLASH_NEW_APPLICATION_END, 1);
   	 //   }

   return(res);
}

void FlashTest1()
{
	// only test 
	struct BootInitReplyPackage  boot_reply_package;
	Fill_boot_reply_package(&boot_reply_package);
}

void Select_Application()
{
	int32_t NewApp = check_for_NewUpgrade();
	if(NewApp==1)
	{
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
		BOOT_boot();
	}
}
