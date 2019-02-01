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

#ifndef FPGA_H
#define FPGA_H

#include "../common/usbprotocol.h"

#define FPGA_CONFIGURE_TIMEOUT 96000000 // in cycles

#ifdef USE_FPGA_JTAG_CONTROLLER

#define MAX_SEQ_SIZE 256
#define MAX_STORED_SEQ_SIZE 65536

#else

#define MAX_SEQ_SIZE 256
#define MAX_STORED_SEQ_SIZE 65536

#endif

void writeSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData);
void readWriteSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *tdoData);
void storeSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *readData);
void executeSeq(void);
uint8_t *readSeq(unsigned size);

bool fpgaInit();
uint32_t fpgaInitOk(void);
void jtagInt(void);
void jtagExt(void);
int jtagTest(void);
bool oscTest(void);

#endif
