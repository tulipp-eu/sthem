/**************************************************************************//**
 * @file flash.h
 * @brief Bootloader flash writing functions.
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

#ifndef FLASH_H
#define FLASH_H
#define NO_RAMFUNCS
#if defined( NO_RAMFUNCS )
#define __ramfunc
#endif

/*
 * Flash programming hardware interface
 *
 */

/* Helper functions */
__ramfunc void FLASH_writeWord(uint32_t address, uint32_t data);
__ramfunc void FLASH_writeBlock(void *block_start,
                                uint32_t offset_into_block,
                                uint32_t count,
                                uint8_t const *buffer);
__ramfunc void FLASH_eraseOneBlock(uint32_t blockStart);
__ramfunc void FLASH_massErase( uint32_t eraseCmd );
void FLASH_init(void);
void FLASH_CalcFlashSize(void);

extern uint32_t flashSize;
#endif
