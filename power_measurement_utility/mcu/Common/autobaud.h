/**************************************************************************//**
 * @file autobaud.h
 * @brief Autobaud estimation for BOOTLOADER_USART
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

#ifndef _AUTOBAUD_H
#define _AUTOBAUD_H

#include <stdbool.h>

void AUTOBAUD_start(void);
void AUTOBAUD_stop( void );
bool AUTOBAUD_completed(void);

#if defined( USART_OVERLAPS_WITH_BOOTLOADER )
void GPIO_IRQHandler(void);
#endif

#endif
