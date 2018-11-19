/**************************************************************************//**
 * @file boot.h
 * @brief Bootloader boot functions
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

#ifndef _BOOT_H
#define _BOOT_H
#define NO_RAMFUNCS
#if defined( NO_RAMFUNCS )
#define __ramfunc
#endif

int BOOT_checkFirmwareIsValid(void);
__ramfunc void BOOT_boot(void);
__ramfunc void BOOT_jump(uint32_t sp, uint32_t pc);

#endif
