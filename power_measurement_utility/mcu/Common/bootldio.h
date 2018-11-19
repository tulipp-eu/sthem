/**************************************************************************//**
 * @file bootldio.h
 * @brief IO code, USART or USB, for the EFM32 bootloader
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

#ifndef _BOOTLDIO_H
#define _BOOTLDIO_H

#if defined( NO_RAMFUNCS )
#define __ramfunc
#endif

__ramfunc void    BOOTLDIO_printHex(    uint32_t integer      );
__ramfunc void    BOOTLDIO_txByte(      uint8_t data          );
__ramfunc uint8_t BOOTLDIO_rxByte(      void                  );
__ramfunc void    BOOTLDIO_printString( const uint8_t *string );
__ramfunc bool    BOOTLDIO_usbMode(     void                  );
__ramfunc bool    BOOTLDIO_getPacket(   XMODEM_packet *p, int timeout );
void              BOOTLDIO_usartInit(   uint32_t clkdiv       );
void              BOOTLDIO_setMode(     bool usb              );

#endif
