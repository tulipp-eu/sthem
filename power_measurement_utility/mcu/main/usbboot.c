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
#include "lynsyn.h"
#include "usb.h"
#include "adc.h"
#include "config.h"

#include "descriptors.h"

#include <stdio.h>
#include <em_usb.h>

#include "jtag.h"
#include "fpga.h"

#include "core_cm3.h"

extern void Send_Data2PC(uint8_t *inBuffer, int length);
extern int UsbDataSent(USB_Status_TypeDef status, uint32_t xf, uint32_t remaining);
extern void Fill_boot_reply_package( struct BootReplyPackage  * boot_reply_package  ,  uint8_t *inBuffer) ;
extern void Flash_erase(uint32_t flash_address_start,uint32_t flash_address_end);
extern int Flash_FIFO_IN(struct FlashBootPackage  * flash_boot_package);
extern int Flash_half();
extern void reset_var();
extern int SaveHeaderToFlash(struct FlashResetPackage inBuffer);

uint8_t BufferFlash[512] ;
void USB_BootMode(struct RequestPacket *inBuffer)
{
	// ack to host command for Boot info and Version
	struct BootReplyPackage boot_reply_package;
	Fill_boot_reply_package( &boot_reply_package , BufferFlash  ) ;
	boot_reply_package.reply.cmd= USB_CMD_BootMode;
	Send_Data2PC((uint8_t *)&boot_reply_package,sizeof (struct BootReplyPackage));
	printf("*** send boot reply package to PC \n");
}
void USB_FlashErase(struct FlashErasePackage *inBuffer)
{
	// ack to host command for Boot info and Version
	struct BootReplyPackage boot_reply_package;
	//reset nand flash in specific range
	    Flash_erase(inBuffer->flash_address_start,inBuffer->flash_address_end);
	    reset_var();
	    Fill_boot_reply_package( &boot_reply_package , BufferFlash  ) ;
	    boot_reply_package.reply.cmd= USB_CMD_FlashErase;
		Send_Data2PC((uint8_t *)&boot_reply_package,sizeof (struct BootReplyPackage));
		printf("*** send flash erase reply package to PC \n");
}
void USB_FLASH_SAVE(struct FlashBootPackage *inBuffer)
{
	//save data to nand flash
	struct FlashResultPackage flash_result;
	flash_result.reply.cmd=USB_CMD_FLASH_Save;
	flash_result.result=Flash_FIFO_IN(inBuffer);
	flash_result.data_length=USB_Data_length;
	flash_result.flash_address=inBuffer->flash_address;
	Send_Data2PC((uint8_t *)&flash_result,sizeof (struct FlashResultPackage));
	printf("*** send data reply package to PC \n");
}

void USB_BootReset(struct FlashResetPackage *inBuffer)
{
	//finalize operation
	struct ResetReplyPackage reset_reply_package;
	// if any data remain in middle buffer save it to flash 
	Flash_half();
	// fill file header to nand flash
	int res=SaveHeaderToFlash(* inBuffer);
	reset_reply_package.reply.cmd=USB_CMD_RESET;
	reset_reply_package.result=res;
	// ack to host
	Send_Data2PC((uint8_t *)&reset_reply_package,sizeof (struct ResetReplyPackage));
	printf("*** send rest reply package to PC \n");
//	for( j = 0 ; j < 100000000 ; j++ );
// reset MCU 
	NVIC_SystemReset();
}
void Send_Data2PC(uint8_t *inBuffer, int length) {
	// send data to host 
  while(USBD_EpIsBusy(CDC_EP_DATA_IN));
  int ret = USBD_Write(CDC_EP_DATA_IN, inBuffer, length , UsbDataSent);
  if(ret != USB_STATUS_OK) printf("Data error send: %d\n", ret);
}
