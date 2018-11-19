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
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <libusb.h>

#include <stdlib.h>
#include<time.h>
#include "../mcu/Common/usbprotocol.h"
//#define USB_CMD_BootMode       'o'
//#define USB_CMD_FLASH_Save     'e'
//#define USB_CMD_RESET          'r'


bool initLynsyn(void) ;
void ledsOff(void);
void ledsOn(void) ;
void releaseLynsyn(void);
void sendBytes(uint8_t *bytes, int numBytes);
bool testUsb(void);
void getBytes(uint8_t *bytes, int numBytes);

uint32_t crc_pc;


void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

uint32_t CRC32(uint32_t CRC , uint32_t *data , int length )
{
 int i ;
 for( i = 0 ; i < length ; i++ )
 {
  CRC = CRC + data[i] ;
 }
 return CRC ;
}

int Send_bit_TO_MCU(char* Add,int only_show)//address for .bin ,)
{
	crc_pc=0;
	printf("*** Firmware Update.*** \n");
/*===========input file==========================*/
FILE *fp = fopen(Add, "rb");
	if(only_show==0)
{
	if(!fp) {
	  printf("Can't open file '%s'\nnot valid or access deny", Add);
	  exit(-2);
	}
	printf("*** File Check.*** \n");
}
/*============check board=========================*/
	if(!initLynsyn()) exit(-3);
	printf("*** lynsyn Check.*** \n");
	if(!testUsb()) exit(-4);
	printf("*** USB Check.*** \n");
	printf("*** LEDs D1 and D2 blink\n");
	long j ;
	ledsOn();
	for( j = 0 ; j < 1000000 ; j++ );
	ledsOff();
	for( j = 0 ; j < 1000000 ; j++ );
	ledsOn();
	for( j = 0 ; j < 1000000 ; j++ );
	ledsOff();
	for( j = 0 ; j < 1000000 ; j++ );
	ledsOn();
	for( j = 0 ; j < 1000000 ; j++ );
	ledsOff();
	for( j = 0 ; j < 1000000 ; j++ );
	ledsOn();
/*==========send first command ===========================*/
//send CMD1 to MCU , MCU <---> flash , return some information  ////send byte
	struct RequestPacket initRequest;
    initRequest.cmd = USB_CMD_BootMode;
    sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));

// get data from MCU /// get byte
    struct BootInitReplyPackage bootReply ;
    getBytes((uint8_t*)&bootReply, sizeof(struct BootInitReplyPackage));
    if( bootReply.ready !=1)
    {
    	printf("*** MCU is not ready to receiving firmware\n");
    	exit(-5);
    }
    printf("*** MCU is ready to receiving data\n");
   // printf("*** MCU boot( current Version) %i\n" ,bootReply.boot_version1 );
  //  printf("*** MCU boot( current Version) %i\n" ,bootReply.boot_version2 );
    printf("*** MCU boot( hardware_version) %i\n" ,bootReply.hwVersion );
    printf("*** MCU boot( software_version) %i\n" ,bootReply.swVersion );
    printf("*** MCU boot( BOOT_Loader_version) %i\n" ,bootReply.BlVersion );
if(only_show==1)
{
	return 1;
}
/*==========send flash erase ===========================*/
//send CMD(flash erase) to MCU , MCU <---> flash , return some information
    uint32_t Flash_offset;
   // struct FlashErasePackage Flash_erase_package;
    struct ResetPackage reset_package;
 //   Flash_erase_package.request.cmd = USB_CMD_FlashErase;
 //   if (bootReply.boot_version2>bootReply.boot_version1)
   // {
   // 	Flash_erase_package.flash_address_start=Flash_offset1;
    //	Flash_erase_package.flash_address_end=Flash_end1;
    	Flash_offset= FLASH_NEW_APPLICATION_START;
    /*	flash_reset_package.version=bootReply.boot_version2+1;
	printf("*** choose up\n");
    }
    else
    {
    	Flash_erase_package.flash_address_start=Flash_offset2;
    	Flash_erase_package.flash_address_end=Flash_end2;
    	Flash_offset=Flash_offset2+512;
    	flash_reset_package.version=bootReply.boot_version1+1;
	printf("*** choose down\n");
    }   
        sendBytes((uint8_t*)&Flash_erase_package, sizeof(struct FlashErasePackage));

    // get data from MCU /// get byte
        getBytes((uint8_t*)&bootReply, sizeof(struct BootReplyPackage));
        if( bootReply.ready !=1)
        {
        	printf("*** MCU is not ready to erasing flash\n");
        	exit(-6);
        }
        printf("*** MCU is ready to erasing flash\n");
        printf("*** MCU boot( current Version) %i\n" ,bootReply.boot_version1 );
        printf("*** MCU boot( current Version) %i\n" ,bootReply.boot_version2 );  */
/*==========send firmware data ===========================*/
 
	struct FlashBootPackage bootPack;
	//read from file(USB_Package_length=12(header)+USB_Data_length(data))  /// fread
	long i ;
	int r;
	//bootReply.flash_address
	fseek(fp, 0L, SEEK_END);
	long sz = ftell(fp);
	sz=((sz+USB_Data_length-1)/USB_Data_length)*USB_Data_length;
	fseek(fp, 0L, SEEK_SET);
	if(sz>10000)
	{
		printf("*** MCU boot Starting  (size: %1.1fkB) ...\n"  ,((float)sz)/1000 );
	}
	else
	{
		printf("*** MCU boot Starting  (size: %1.0fB) ...\n"  ,((float)sz) );
	}
	for(i=0;;i++)
	{
		bootPack.request.cmd=USB_CMD_FLASH_Save;
		bootPack.index=i+1;
		bootPack.flash_address=Flash_offset+i*USB_Data_length;//+i/21*8;
		bootPack.end=0;
		r = fread(bootPack.Data, 1, USB_Data_length, fp);
		if (r==0) break;
		crc_pc=CRC32(crc_pc,(uint32_t *)bootPack.Data,USB_Data_length/4);
		//send to MCU (CMD2) // send byte
		sendBytes((uint8_t*)&bootPack, USB_Package_length);
	  //  printf("*** MCU boot data send ,(%2.1f%s) completed\n" , (1.0f * i * 100*USB_Data_length) / sz , "%" );
	//wait for ACK //get byte
		/* struct FlashResultPackage flash_result;
		 getBytes((uint8_t*)&flash_result, sizeof(struct FlashResultPackage));
		if (flash_result.result!=1) 
			{
				printf("*** Save in flash is not succesful\n");
				exit(-7);
			}    */
//		if(flash_result.data_length!=USB_Data_length) exit(-7);
	}
    printf("*** MCU boot data send ,(%2.1f%s) completed\n" , 100.0 , "%" );

//MCU go to reset(CMD)

//reset
   // uint32_t crc_Header=0;
    reset_package.request.cmd = USB_CMD_RESET;
    reset_package.Reserve[0]=0;
    reset_package.Reserve[1]=0;
    reset_package.Reserve[2]=0;
    reset_package.crc=crc_pc;
   // flash_reset_package.dataLength=bootPack.flash_address-Flash_offset+USB_Data_length;
  //  flash_reset_package.flashAddress=Flash_offset-512;
 //   flash_reset_package.crc_header=CRC32(crc_Header,(uint32_t *)&flash_reset_package,sizeof(struct FlashResetPackage)/4-1);
    sendBytes((uint8_t*)&reset_package, sizeof(struct ResetPackage));

// get data from MCU
   /* struct ResetReplyPackage reset_reply_package;
    getBytes((uint8_t*)&reset_reply_package, sizeof(struct ResetReplyPackage));
    if(reset_reply_package.result!=1) 
	{
		printf("*** Upgrading is not completed\n");
		exit(-8);
	}*/
    printf("*** Upgrading was successful \n");
    releaseLynsyn();
    return 1 ;
}



int main( int argc , char *argv[]) {
  printf(" Frameware Update Ver 0.9 \n\n");

  if( argc == 1 )
  {
 	  // if you enter somthing like this:
	  ///    ./lynsyn_tester
	  Send_bit_TO_MCU(argv[1],1) ;
	  printf(" its need to enter input Application bin\n");
	  return -1;
  }
	  // if you enter somthing like this:
          //    ./lynsyn_tester Application.bin
  return Send_bit_TO_MCU(argv[1],0) ;

}
