/**************************************************************************//**
 * @file cdc.h
 * @brief CDC header file
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

extern volatile bool CDC_Configured;

#if defined( NO_RAMFUNCS )
#define __ramfunc
#endif

__ramfunc int  CDC_SetupCmd( const USB_Setup_TypeDef *setup );
__ramfunc void CDC_StateChange( USBD_State_TypeDef oldState, USBD_State_TypeDef newState );
