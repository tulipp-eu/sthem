/**************************************************************************//**
 * @file retargetdebug.h
 * @brief I/O retarget prototypes and definitions
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

#ifndef __RETARGETDEBUG_H
#define __RETARGETDEBUG_H

#if defined( BL_DEBUG )

void RETARGET_SerialInit(void);
int  RETARGET_ReadChar(void);
int  RETARGET_WriteChar(char c);

#endif
#endif
