/******************************************************************************
 *
 *  This file is part of the Lynsyn PMU firmware.
 *
 *  Copyright 2018 Asbjørn Djupdal, NTNU, TULIPP EU Project
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

#ifndef LYNSYN_MAIN_H
#define LYNSYN_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#include "lynsyn.h"

#include "em_device.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "em_system.h"
#include "em_chip.h"
 
///////////////////////////////////////////////////////////////////////////////
// firmware settings

#define CAL_AVERAGE_SAMPLES 1024

///////////////////////////////////////////////////////////////////////////////
// global functions

int64_t calculateTime();

///////////////////////////////////////////////////////////////////////////////
// global variables

extern volatile bool sampleMode;
extern volatile bool samplePc;
extern volatile bool gpioMode;
extern volatile bool useStartBp;
extern volatile bool useStopBp;
extern volatile int64_t sampleStop;
extern uint8_t startCore;
extern uint8_t stopCore;
extern uint64_t frameBp;

#endif
