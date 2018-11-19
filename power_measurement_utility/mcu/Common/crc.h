/**************************************************************************//**
 * @file crc.h
 * @brief CRC16 routines for XMODEM and verification.
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

#ifndef _CRC_H
#define _CRC_H

#include <stdint.h>

#if defined( NO_RAMFUNCS )
#define __ramfunc
#endif

__ramfunc uint16_t CRC_calc(uint8_t *start, uint8_t *end);

#endif
