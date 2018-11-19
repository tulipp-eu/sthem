/***************************************************************************//**
 * @file
 * @brief NAND Flash example for EFM32GG_STK3700 development kit
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2015 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_common.h"
#include "bsp.h"
#include "retargetserial.h"
#include "bsp_trace.h"

#include "nandflash.h"
#include "flash_functions.h"

/***************************************************************************//**
 *
 * This example demonstrates use of NAND Flash on STK3700.
 *
 * Connect a terminal to the serialport (115200-N-8-1) on EXP port pins
 * 2 and 4.
 * Operations on the flash are initiated by issuing commands on the terminal.
 * Command "h" will print a help screen on the terminal.
 *
 ******************************************************************************/

#define PAGENUM_2_ADDR(x)  ( ( (x) * NANDFLASH_DeviceInfo()->pageSize) \
                             + NANDFLASH_DeviceInfo()->baseAddress)
#define BLOCKNUM_2_ADDR(x) ( ( (x) * NANDFLASH_DeviceInfo()->blockSize) \
                             + NANDFLASH_DeviceInfo()->baseAddress)
#define ADDR_2_PAGENUM(x)  ( ( (x) - NANDFLASH_DeviceInfo()->baseAddress) \
                             / NANDFLASH_DeviceInfo()->pageSize)
#define ADDR_2_BLOCKNUM(x) ( ( (x) - NANDFLASH_DeviceInfo()->baseAddress) \
                             / NANDFLASH_DeviceInfo()->blockSize)

#define DWT_CYCCNT  *(volatile uint32_t*)0xE0001004
#define DWT_CTRL    *(volatile uint32_t*)0xE0001000

/** Command line */
#define CMDBUFSIZE    80
#define MAXARGC       5

/** Input buffer */
static char cmdBuffer[CMDBUFSIZE + 1];
static int  argc;
static char *argv[MAXARGC];

/* NOTE: We assume that page size is 512 !! */
#define BUF_SIZ 512
SL_ALIGN(4)
static uint8_t buffer[2][BUF_SIZ] SL_ATTRIBUTE_ALIGN(4);

static bool blankCheckPage(uint32_t addr, uint8_t *buffer);
static void dump16(uint32_t addr, uint8_t *data);
static void dumpPage(uint32_t addr, uint8_t *data);
static void getCommand(void);
static void printHelp(void);
static void splitCommandLine(void);

/***************************************************************************//**
 * @brief main - the entrypoint after reset.
 ******************************************************************************/
uint8_t  buffer1[512];
uint8_t  buffer2[512];
int main2(void)
{

  /* Chip errata */
  //CHIP_Init();

  /* If first word of user data page is non-zero, enable Energy Profiler trace */
  BSP_TraceProfilerSetup();

  /* Select 48MHz clock. */
 CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

  /* Setup EBI for NAND Flash. */
  BSP_EbiInit();

  /* Enable DWT */
 CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  /* Make sure CYCCNT is running */
 DWT_CTRL |= 1;

  /* Initialize USART and map LF to CRLF */
  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  printf("\nEFM32GG_STK3700 NAND Flash example\n");
  printHelp();

  /* Initialize nand flash module, use DMA channel 5. */
  NANDFLASH_Init(5);

	int i;
for ( i = 0; i < BUF_SIZ; i++ ) {
     buffer1[i] = 0x12;
     buffer2[i] = 0x13;
	}
  test_flash(512,buffer1,buffer2);
  return(1);
  while (1) {
    //getCommand();
	  argv[0]="rp";
	 // argv[0][0]="f";
	 // argv[0][1]="i";

    /* Get misc. NAND flash info */
    if ( !strcmp(argv[0], "fi") ) {
    	 flash_info();
    }
    /* Read a page */
    else if ( !strcmp(argv[0], "rp") ) {
    	uint32_t pageNum;
    	pageNum = strtoul(argv[1], NULL, 0);
    	 Read_Page( pageNum, buffer1);
    }
    /* Blankcheck a page */
    else if ( !strcmp(argv[0], "bp") ) {
    	uint32_t pageNum;
    	pageNum = strtoul(argv[1], NULL, 0);
    	Blankcheck_page( pageNum, buffer1);
    }
    /* Blankcheck entire device */
    else if ( !strcmp(argv[0], "bd") ) {
    	Blankcheck_device(buffer1);
    }
    /* Check bad-block info */
    else if ( !strcmp(argv[0], "bb") ) {
    	check_badBlock_Info(buffer1);
    }
    /* Write a page */
    else if ( !strcmp(argv[0], "wp") ) {
    	uint32_t pageNum;
    	pageNum = strtoul(argv[1], NULL, 0);
    	Write_page( pageNum,  buffer1, BUF_SIZ);
    }
    /* Erase a block */
    else if ( !strcmp(argv[0], "eb") ) {
    	uint32_t blockNum;
    	blockNum = strtoul(argv[1], NULL, 0);
    	Erase_block( blockNum);
    }

    /* Check ECC algorithm */
       else if ( !strcmp(argv[0], "ecc") ) {

    	   uint32_t pageNum;
    	   pageNum = strtoul(argv[1], NULL, 0);
    	   Check_ECC( pageNum, buffer1, buffer2, BUF_SIZ);
       }
    /* Copy a page */
    else if ( !strcmp(argv[0], "cp") ) {

    	 uint32_t dstPageNum, srcPageNum;
    	  dstPageNum = strtoul(argv[2], NULL, 0);
    	  srcPageNum = strtoul(argv[1], NULL, 0);
    	  copy_page( dstPageNum, srcPageNum);
    }
    /* Mark block as bad */
    else if ( !strcmp(argv[0], "mb") ) {

    	uint32_t blockNum;
    	blockNum = strtoul(argv[1], NULL, 0);
    	Mark_badBlock( blockNum);
    }
    /* Display help */
    else if ( !strcmp(argv[0], "h") ) {
      printHelp();
    } else {
    	Unknown_command();
    }
  }
}

/***************************************************************************//**
 * @brief Print a help screen.
 ******************************************************************************/
static void printHelp(void)
{
  printf(
    "Available commands:\n"
    "\n    fi         : Show NAND flash device information"
    "\n    h          : Show this help"
    "\n    rp <n>     : Read page <n>"
    "\n    bp <n>     : Blankcheck page <n>"
    "\n    bd         : Blankcheck entire device"
    "\n    bb         : Check bad-block info"
    "\n    mb <n>     : Mark block <n> as bad"
    "\n    wp <n>     : Write page <n>"
    "\n    eb <n>     : Erase block <n>"
    "\n    ecc <n>    : Check ECC algorithm, uses page <n> and <n+1>"
    "\n    cp <m> <n> : Copy page <m> to page <n>"
    "\n");
}

/***************************************************************************//**
 * @brief Get a command from the terminal on the serialport.
 ******************************************************************************/
static void getCommand(void)
{
  int c;
  int index = 0;

  printf("\n>");
  while (1) {
    c = getchar();
    if (c > 0) {
      /* Output character - most terminals use CRLF */
      if (c == '\r') {
        cmdBuffer[index] = '\0';
        splitCommandLine();
        putchar('\n');
        return;
      } else if (c == '\b') {
        printf("\b \b");
        if ( index ) {
          index--;
        }
      } else {
        /* Filter non-printable characters */
        if ((c < ' ') || (c > '~')) {
          continue;
        }

        /* Enter into buffer */
        cmdBuffer[index] = c;
        index++;
        if (index == CMDBUFSIZE) {
          cmdBuffer[index] = '\0';
          splitCommandLine();
          return;
        }
        /* Echo char */
        putchar(c);
      }
    }
  }
}

/***************************************************************************//**
 * @brief Split a command line into separate arguments.
 ******************************************************************************/
static void splitCommandLine(void)
{
  int i;
  char *result = strtok(cmdBuffer, " ");

  argc = 0;
  for ( i = 0; i < MAXARGC; i++ ) {
    if ( result ) {
      argc++;
    }

    argv[i] = result;
    result = strtok(NULL, " ");
  }
}




