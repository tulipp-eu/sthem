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

#ifndef ZYNQ_H
#define ZYNQ_H

#define IDR_AHB_AP           0x04770001
#define IDR_APB_AP           0x04770002
#define IDR_AXI_AP           0x04770004

#define CORTEXA9_TAP_IDCODE 0x4ba00477
#define CORTEXA53_TAP_IDCODE 0x5ba00477

#define JTAG_CTRL_ARM_DAP 2
#define JTAG_CTRL_PL_TAP  1

#define PS_JTAG_CTRL 0x824

#endif
