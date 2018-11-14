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

 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_common.h"

#include "nandflash.h"

#include "usbprotocol.h"
#include "flash_functions.h"

#define DWT_CYCCNT  *(volatile uint32_t*)0xE0001004
#define PAGENUM_2_ADDR(x)  ( ( (x) * NANDFLASH_DeviceInfo()->pageSize) \
                             + NANDFLASH_DeviceInfo()->baseAddress)
#define BLOCKNUM_2_ADDR(x) ( ( (x) * NANDFLASH_DeviceInfo()->blockSize) \
                             + NANDFLASH_DeviceInfo()->baseAddress)
#define ADDR_2_BLOCKNUM(x) ( ( (x) - NANDFLASH_DeviceInfo()->baseAddress) \
                             / NANDFLASH_DeviceInfo()->blockSize)
#define ADDR_2_PAGENUM(x)  ( ( (x) - NANDFLASH_DeviceInfo()->baseAddress) \
                             / NANDFLASH_DeviceInfo()->pageSize)

static bool blankCheckPage(uint32_t addr, uint8_t *buffer);
static void dump16(uint32_t addr, uint8_t *data);
static void dumpPage(uint32_t addr, uint8_t *data);
/***************************************************************************//**
 * @brief Blankcheck a page in NAND flash.
 *
 * @param[in] addr
 *   The first address in the nand flash to start blankchecking.
 *
 * @param[in] buffer
 *   Page buffer to use when reading from nand flash.
 ******************************************************************************/
static bool blankCheckPage(uint32_t addr, uint8_t *buffer)
{
  uint32_t i;

  NANDFLASH_ReadPage(addr, buffer);
  for ( i = 0; i < NANDFLASH_DeviceInfo()->pageSize; i++ ) {
    if ( buffer[i] != 0xFF ) {
      printf(" ---> Blankcheck failure at address 0x%08lX (page %ld) <---\n",
             addr + i, ADDR_2_PAGENUM(addr) );
      return false;
    }
  }

  for ( i = 0; i < NANDFLASH_DeviceInfo()->spareSize; i++ ) {
    if ( NANDFLASH_DeviceInfo()->spare[i] != 0xFF ) {
      printf(" ---> Blankcheck failure in spare area for page at address 0x%08lX (page %ld), spare byte number %ld <---\n",
             addr, ADDR_2_PAGENUM(addr), i);
      return false;
    }
  }
  return true;
}

/***************************************************************************//**
 * @brief Print 16 bytes in both hex and ascii form on terminal.
 *
 * @param[in] addr
 *   The address to start at.
 * @param[in] data
 *   A buffer with 16 bytes to print.
 ******************************************************************************/
static void dump16(uint32_t addr, uint8_t *data)
{
  int i;

  printf("\n%08lX: ", addr);

  /* Print data in hex format */
  for ( i = 0; i < 16; i++ ) {
    printf("%02X ", data[i]);
  }
  printf("   ");
  /* Print data in ASCII format */
  for ( i = 0; i < 16; i++ ) {
    if ( isprint(data[i]) ) {
      printf("%c", data[i]);
    } else {
      printf(" ");
    }
  }
}

/***************************************************************************//**
 * @brief Dump page content on terminal.
 *
 * @param[in] addr
 *   The address to start at.
 * @param[in] data
 *   A buffer with page data.
 ******************************************************************************/
static void dumpPage(uint32_t addr, uint8_t *data)
{
  uint32_t i, j, lines;

  printf("\nEcc     : 0x%08lX", NANDFLASH_DeviceInfo()->ecc);
  printf("\nSpare   : ");

  for ( i = 0; i < NANDFLASH_DeviceInfo()->spareSize; i++ ) {
    printf("%02X ", NANDFLASH_DeviceInfo()->spare[i]);
  }
  putchar('\n');

  lines = NANDFLASH_DeviceInfo()->pageSize / 16;
  for ( i = 0, j = 0;
        i < lines;
        i++, j += 16, addr += 16   ) {
    dump16(addr, &data[j]);
  }
  putchar('\n');
}


/* Get misc. NAND flash info */
void flash_info()
{
		printf(" NAND flash device information:\n");
	    printf("\n  Manufacturer Code :  0x%02X", NANDFLASH_DeviceInfo()->manufacturerCode);
	    printf("\n  Device Code       :  0x%02X", NANDFLASH_DeviceInfo()->deviceCode);
	    printf("\n  Device size       :  %ld (%ldMB)", NANDFLASH_DeviceInfo()->deviceSize,
	            NANDFLASH_DeviceInfo()->deviceSize / 1024 / 1024);
	    printf("\n  Page size         :  %ld", NANDFLASH_DeviceInfo()->pageSize);
	    printf("\n  Spare size        :  %ld", NANDFLASH_DeviceInfo()->spareSize);
	    printf("\n  Block size        :  %ld", NANDFLASH_DeviceInfo()->blockSize);
	    putchar('\n');
}

/* Read a page */
int Read_Page(uint32_t pageNum,uint8_t *buffer)
{
	// read specific page 
	printf("Read page content,page%ld\n",pageNum);
	//return(1);
	int status;
	uint32_t addr,time;

   //   pageNum = strtoul(argv[1], NULL, 0);
	  addr = PAGENUM_2_ADDR(pageNum);

	  if ( !NANDFLASH_AddressValid(addr) ) {
		printf(" Read page content, page %ld is not a valid page\n", pageNum);
		return(-1);
	  } else {
		time = DWT_CYCCNT;
		status = NANDFLASH_ReadPage(addr, buffer);
		time = DWT_CYCCNT - time;
		if ( status == NANDFLASH_STATUS_OK ) {
		  printf(" Read page %ld content, %ld cpu-cycles used\n", pageNum, time);
		} else if(status==-5)
		{
			printf(" Read page %ld content, %ld cpu-cycles used\n", pageNum, time);
		}
		else {
		  printf(" Read page error %d, %ld cpu-cycles used\n", status, time);
		  return(-1);
		}
	  //  dumpPage(addr, buffer);
	      }
	return(1);
}

/* Blankcheck a page */
int Blankcheck_page(uint32_t pageNum,uint8_t *buffer)
{
	uint32_t addr;

//  pageNum = strtoul(argv[1], NULL, 0);
  addr = PAGENUM_2_ADDR(pageNum);

  if ( !NANDFLASH_AddressValid(addr) ) {
	printf(" Blankcheck page, page %ld is not a valid page\n", pageNum);
	return(-1);
  } else {
	printf(" Blankchecking page %ld\n", pageNum);
	if ( blankCheckPage(addr, buffer) ) {
	  printf(" Page %ld is blank\n", pageNum);
	  return(1);
	}
  }
  return -1 ;
}
/* Blankcheck entire device */
int Blankcheck_device(uint8_t * buffer)
{
	uint32_t addr;
  int i, pageSize, pageCount;

  pageSize  = NANDFLASH_DeviceInfo()->pageSize;
  pageCount = NANDFLASH_DeviceInfo()->deviceSize / pageSize;
  addr      = NANDFLASH_DeviceInfo()->baseAddress;

  printf(" Blankchecking entire device\n");
  for ( i = 0; i < pageCount; i++, addr += pageSize ) {
	if ( !blankCheckPage(addr, buffer) ) {
	  break;
	}
  }

  if ( i == pageCount ) {
	printf(" Device is blank\n");
	return 1 ;
  }
  return -1 ;
}

/* Check bad-block info */
int check_badBlock_Info(uint8_t * buffer)
{
	uint32_t addr;
  int i, blockSize, blockCount, badBlockCount = 0;

  blockCount = NANDFLASH_DeviceInfo()->deviceSize / NANDFLASH_DeviceInfo()->blockSize;
  blockSize  = NANDFLASH_DeviceInfo()->blockSize;
  addr       = NANDFLASH_DeviceInfo()->baseAddress;

  for ( i = 0; i < blockCount; i++, addr += blockSize ) {
	NANDFLASH_ReadSpare(addr, buffer);
	/* Manufacturer puts bad-block info in byte 6 of the spare area */
	/* of the first page in each block.                             */
	if ( buffer[NAND_SPARE_BADBLOCK_POS] != 0xFF ) {
	  printf(" ---> Bad-block at address 0x%08lX (block %ld) <---\n",
			 addr, ADDR_2_BLOCKNUM(addr) );
	  badBlockCount++;
	}
  }

  if ( badBlockCount == 0 ) {
	printf(" Device has no bad-blocks\n");
  }
  return badBlockCount ;
}

/* Write a page */
int Write_page(uint32_t pageNum,uint8_t * buffer,int BUF_SIZ)
{
	// write specific page 
	//return(1);
    int  status;
    uint32_t  time,addr;

   // pageNum = strtoul(argv[1], NULL, 0);
    addr = PAGENUM_2_ADDR(pageNum);

    if ( !NANDFLASH_AddressValid(addr) ) {
      printf(" Write page, page %ld is not a valid page\n", pageNum);
      return -1 ;
    } else {
      printf(" Write page %ld, ", pageNum);

    //  for ( i = 0; i < BUF_SIZ; i++ ) {
      //  buffer[i] = i;
     // }

      time = DWT_CYCCNT;
      status = NANDFLASH_WritePage(addr, buffer);
      time = DWT_CYCCNT - time;
      printf("ecc : 0x%08lX\n", NANDFLASH_DeviceInfo()->ecc);

      if ( status == NANDFLASH_STATUS_OK ) {
        printf(" Page written OK, %ld cpu-cycles used\n", time);
      } else if ( status == NANDFLASH_WRITE_ERROR ) {
        printf(" Page write failure, bad-block\n");
        return -1 ;
      } else {
        printf(" Page write error %d\n", status);
        return -1 ;
      }
    }
    return 1 ;
}

/* Erase a block */
int Erase_block(uint32_t blockNum)
{
	// erase specific block 

	//return(1);
    int status;
    uint32_t  addr;

   // blockNum = strtoul(argv[1], NULL, 0);
    addr = BLOCKNUM_2_ADDR(blockNum);

    if ( !NANDFLASH_AddressValid(addr) ) {
      printf(" Erase block, block %ld is not a valid block\n", blockNum);
      return -1 ;
    } else {
      printf(" Erase block %ld, ", blockNum);

      status = NANDFLASH_EraseBlock(addr);

      if ( status == NANDFLASH_STATUS_OK ) {
        printf(" Block erased OK\n");
      } else if ( status == NANDFLASH_WRITE_ERROR ) {
        printf(" Block erase failure, bad-block\n");
        return -1 ;
      } else {
        printf(" Block erase error %d\n", status);
        return -1 ;
      }
    }
    return 1;
}
/* Check ECC algorithm */
void Check_ECC(uint32_t pageNum,uint8_t * buffer1,uint8_t * buffer2,uint32_t BUF_SIZ)
{
    int i, status;
    uint32_t addr0, addr1, ecc0, ecc1;

    for ( i = 0; i < BUF_SIZ; i++ ) {
      buffer1[i] = i;
      buffer2[i] = i;
    }

  //  pageNum = strtoul(argv[1], NULL, 0);
    addr0   = PAGENUM_2_ADDR(pageNum);
    addr1   = PAGENUM_2_ADDR(pageNum + 1);

    if ( !NANDFLASH_AddressValid(addr0)
         || !NANDFLASH_AddressValid(addr1)    ) {
      printf(" Check ECC algorithm, page %ld or %ld is not a valid page\n",
             pageNum, pageNum + 1);
    } else {
      printf(" Checking ECC algorithm\n");
      status = NANDFLASH_WritePage(addr0, buffer1);
      if ( status != NANDFLASH_STATUS_OK ) {
        printf(" Write in page <n> failed\n");
      }
      ecc0 = NANDFLASH_DeviceInfo()->ecc; /* Get ECC generated during write*/

      /* Patch one bit in the second buffer. */
      buffer2[147] ^= 4;

      status = NANDFLASH_WritePage(addr1, buffer2);
      if ( status != NANDFLASH_STATUS_OK ) {
        printf(" Write in page <n+1> failed\n");
      }
      ecc1 = NANDFLASH_DeviceInfo()->ecc; /* Get ECC generated during write*/

      /* Try to correct the second buffer using ECC from first buffer */
      NANDFLASH_EccCorrect(ecc0, ecc1, buffer2);
      if ( memcmp(buffer1, buffer2, BUF_SIZ) == 0 ) {
        printf(" ECC correction succeeded\n");
      } else {
        printf(" ECC correction failed\n");
      }

      /* Byte 147 (addr 0x93) of page n+1 is now 0x97.                    */
      /* Single step next function call, stop when ECC is read, set the   */
      /* read ECC to ecc10 (0) and verify that 0x97 is corrected to 0x93! */
      NANDFLASH_ReadPage(addr1, buffer2);
    }
}

/* Copy a page */
int copy_page(uint32_t dstPageNum,uint32_t srcPageNum)
{
    int status;
    uint32_t dstAddr, srcAddr;

  //  dstPageNum = strtoul(argv[2], NULL, 0);
  //  srcPageNum = strtoul(argv[1], NULL, 0);
    dstAddr    = PAGENUM_2_ADDR(dstPageNum);
    srcAddr    = PAGENUM_2_ADDR(srcPageNum);

    if ( !NANDFLASH_AddressValid(dstAddr)
         || !NANDFLASH_AddressValid(srcAddr)    ) {
      printf(" Copy page, page %ld or %ld is not a valid page\n",
             srcPageNum, dstPageNum);
      return -1 ;
    } else {
      printf(" Copying page %ld to page %ld\n", srcPageNum, dstPageNum);

      status = NANDFLASH_CopyPage(dstAddr, srcAddr);

      if ( status == NANDFLASH_STATUS_OK ) {
        printf(" Page copied OK\n");
      } else if ( status == NANDFLASH_WRITE_ERROR ) {
        printf(" Page copy failure, bad-block\n");
        return -1 ;
      } else {
        printf(" Page copy error %d\n", status);
        return -1 ;
      }
    }
    return 1 ;
}

/* Mark block as bad */
int Mark_badBlock(uint32_t blockNum)
{
    uint32_t addr;

 //   blockNum = strtoul(argv[1], NULL, 0);
    addr = BLOCKNUM_2_ADDR(blockNum);

    if ( !NANDFLASH_AddressValid(addr) ) {
      printf(" Mark bad block, %ld is not a valid block\n", blockNum);
      return -1 ;
    } else {
      printf(" Marked block %ld as bad\n", blockNum);
      NANDFLASH_MarkBadBlock(addr);
    }
    return 1 ;
}


void Unknown_command()
{
	printf(" Unknown command");
}

void test_flash(int BUF_SIZ,uint8_t * buffer1,uint8_t * buffer2)
{
Write_page(10012,buffer1,BUF_SIZ);
Read_Page( 12287,buffer2);
Read_Page( 12288,buffer2);
Read_Page( 8192,buffer2);
if ( memcmp(buffer1, buffer2, BUF_SIZ) == 0 ) {
        printf(" ECC correction succeeded\n");
      } else {
        printf(" ECC correction failed\n");
      }
}
void NANDflashInit()
{
	// nand flash initialize
  //BSP_EbiInit();
	 NANDFLASH_Init(5);
}

void Fill_boot_reply_package( struct BootReplyPackage  * boot_reply_package,uint8_t *inBuffer )
{
	boot_reply_package->hardware_version=1202;        // hardware_version
	boot_reply_package->software_version=SW_VERSION;// software_version
	boot_reply_package->NANDflash_version=0x7520; // nan versian
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
 boot_reply_package->ready = 1 ;// raedy
 boot_reply_package->boot_version1 = res1 ;
 boot_reply_package->boot_version2 = res2 ;
 return ;
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

int check_flash_for_Framework(uint32_t Flash_ofset ,  uint8_t *inBuffer )
{
	// check for versian saved in nandflash
 int res  ,  i;
 uint32_t crc = 0 ;
 int nPage = 0 ;
 res =  Read_Page( Flash_ofset/512 ,inBuffer) ;
 if(res<1)return -1 ;// nand flash error
 struct Flash_Struct * header =( struct Flash_Struct * ) inBuffer ;
 uint32_t Header_dataCRC=header->DataCRC;
 uint16_t Header_version=header->version;
 if( header->version == 0xffff )
 {
	 // check version
  return -2;
 }
 //check  header->HeaderCRC
 crc = CRC32(  crc , (uint32_t * )inBuffer , sizeof(struct Flash_Struct)/sizeof(uint32_t )-1 ) ;
 if( crc != header->HeaderCRC )
 {
	 // check crc
  return -3 ;
 }
 if( header->Datalength > 0x200000 )
 {
	 //check data length
  return -4 ;
 }
 long length  = header->Datalength ;//+sizeof(struct Flash_Struct) ;
 nPage = (  length + 512 - 1  ) / 512 ;
 crc = 0 ;
// crc = CRC32( crc , (uint32_t * )inBuffer , 512  ) ;
 for( i  =  1 ; i<= nPage ; i++ )
 {
  res =  Read_Page( Flash_ofset/512 +  i  ,inBuffer) ;
  if(res<1)return -1 ;
  crc = CRC32( crc , (uint32_t * )inBuffer , 512/4  ) ;
 }
 if( crc != Header_dataCRC )
 {
	 // check crc header
  return -5 ;
 }
 return Header_version;
}

