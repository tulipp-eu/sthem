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
#include "flash.h"
#include "boot.h"
#include <stdio.h>

void Flash_erase(uint32_t flash_address_start,uint32_t flash_address_end)
{
	// erace nand flash from flash_address_start to flash_address_end
	int i;
	for(i=flash_address_start/(32*512);i<=flash_address_end/(32*512);i++)
	{
		Erase_block(i);
	}
}
uint8_t inBuffer2[512];
int pointer=0;
uint32_t crc_mcu=0;
void reset_var()
{
	pointer=0;
	crc_mcu=0;
}
uint32_t last_flash_Address;
 int Flash_FIFO_IN(struct FlashBootPackage  * flash_boot_package)
 {
	 // save data to nand flash
	 int i,res=1;
	 for(i=0;i<USB_Data_length;i++)
	 {
		 // copy data to inBuffer2
		 inBuffer2[i+pointer]=flash_boot_package->Data[i];
	 }
	 pointer=USB_Data_length+pointer;
	 crc_mcu=CRC32(crc_mcu,(uint32_t * )flash_boot_package->Data,USB_Data_length/4);
	 last_flash_Address=flash_boot_package->flash_address;
	 if (pointer==512)
	 {
		 // if inBuffer2 full , write it to exter flash
		 res= Write_page((flash_boot_package->flash_address)/512,inBuffer2, 512);
		 pointer=0;
		// reset_var();
	 }
	 return(res);
 }
int Flash_half()
{
	// if any data left on inBuffer2 save it to external flash
	int res;
	if(pointer==0)
	{
		return 1;
	}
	else
	{
		int j;
		 for(j=pointer;j<512;j++)
		 {
		  inBuffer2[j]=0;
		 }
		 res= Write_page((last_flash_Address)/512,inBuffer2, 512);
		 pointer=0;
	 }
	 return(res);
	}
int SaveHeaderToFlash(struct FlashResetPackage inBuffer)
{
	// save data header file to nand flash 
  int i ,res = 1;
  uint32_t crc_mcu2 = 0 ;
  struct Flash_Struct  *fs ;
   for(i=0;i<512;i++)
   {
     inBuffer2[i]=0xff;
   }
   fs = (struct Flash_Struct *)inBuffer2 ;
   fs->version = inBuffer.version;
   fs->Reserve0 = 0 ;
   fs->Reserve1=0;
   fs->Reserve2=0;
   if( crc_mcu != inBuffer.crc )
   {
	   // chech data crc 
     return -1 ;
   }
   fs->DataCRC = inBuffer.crc;
   fs->Datalength= inBuffer.dataLength;
   fs->HeaderCRC=0;
   crc_mcu2 = 0 ;
  // inBuffer.request.cmd=0;
  crc_mcu2 = CRC32( crc_mcu2 , (uint32_t *)&inBuffer , sizeof(struct FlashResetPackage)/4 -1 );
  if(crc_mcu2 != inBuffer.crc_header)
  {
	  // che crc of data come from host
    return -2;
  }

   crc_mcu2 = 0 ;
   crc_mcu2 = CRC32( crc_mcu2 , (uint32_t *)fs , sizeof(struct Flash_Struct)/4 );
   fs->HeaderCRC = crc_mcu2 ;
    res= Write_page((inBuffer.flashAddress)/512,inBuffer2, 512);

   return(res);
}

void FlashTest1()
{
	// only test 
	struct BootReplyPackage  boot_reply_package;
	Fill_boot_reply_package(&boot_reply_package,inBuffer2);
}

