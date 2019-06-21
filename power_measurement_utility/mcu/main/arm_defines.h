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

#ifndef ARM_DEFINES_H
#define ARM_DEFINES_H

///////////////////////////////////////////////////////////////////////////////
// JTAG-DP TAP

#define ARM_TAP_IDCODE 0x0ba00477

#define IDR_AHB_AP           0x04770001
#define IDR_APB_AP           0x04770002
#define IDR_AXI_AP           0x04770004

///////////////////////////////////////////////////////////////////////////////
// DP & AP

#define MAX_APS 10

#define AP_REG(x)   (APNDP | (x))
#define AP_CSW      AP_REG(0x00)
#define AP_TAR      AP_REG(0x04)
#define AP_TAR_HI   AP_REG(0x08)
#define AP_DRW      AP_REG(0x0C)
#define AP_BASE_HI  AP_REG(0xF0)
#define AP_CFG      AP_REG(0xF4)
#define AP_BASE     AP_REG(0xF8)
#define AP_IDR      AP_REG(0xFC)

#define APNDP       0x100

#define LOW_WRITE   0
#define LOW_READ    1

#define JTAG_ACK_OK   0x02
#define JTAG_ACK_WAIT 0x01

#define JTAG_IR_ABORT  0x8
#define JTAG_IR_DPACC  0xA
#define JTAG_IR_APACC  0xB

#define DP_IDCODE   0x0
#define DP_ABORT    0x0
#define DP_CTRLSTAT 0x4
#define DP_SELECT   0x8
#define DP_RDBUFF   0xC

#define DP_CTRLSTAT_CSYSPWRUPREQ	(1u << 30)
#define DP_CTRLSTAT_CSYSPWRUPACK	(1u << 31)
#define DP_CTRLSTAT_CDBGPWRUPACK	(1u << 29)
#define DP_CTRLSTAT_CDBGPWRUPREQ	(1u << 28)

#define DP_ABORT_DAPABORT		(1 << 0)

///////////////////////////////////////////////////////////////////////////////
// ARM V7-A

#define DSCR_HDBGEN           (1 << 14)
#define DSCR_ITREN            (1 << 13)
#define DSCR_MOE_MASK         (0xf << 2)
#define DSCR_RESTARTED        (1 << 1)
#define DSCR_MOE_BP           (0x1 << 2)

enum DBG_MOE_T {UNKNOWN=0, BREAKPOINT, NUM_MOE_TYPES};

#define DRCR_CSE              (1 << 2)
#define DRCR_RRQ              (1 << 1)

#define BCR_BAS_ANY           (0xf << 5)
#define BCR_EN                (1 << 0)

#define MCR 0xee000010
#define CPREG(coproc, opc1, rt, crn, crm, opc2) \
	(((opc1) << 21) | ((crn) << 16) | ((rt) << 12) | \
        ((coproc) << 8) | ((opc2) << 5) | (crm))
#define DTRTXint CPREG(14, 0, 0, 0, 5, 0)

#define ARMV7A_DIDR    0
#define ARMV7A_ITR     33
#define ARMV7A_PCSR    33
#define ARMV7A_DSCR    34
#define ARMV7A_DTRTX   35
#define ARMV7A_DRCR    36
#define ARMV7A_BVR(i)  (64+(i))
#define ARMV7A_BCR(i)  (80+(i))

///////////////////////////////////////////////////////////////////////////////
// ARM V8-A

#define ARMV8A_PCSR_L      40
#define ARMV8A_PCSR_H      43
#define ARMV8A_BVR_L(i)    (256+(16*(i)))
#define ARMV8A_BVR_H(i)    (257+(16*(i)))
#define ARMV8A_BCR(i)      (258+(16*(i)))
#define ARMV8A_DFR0        842
#define ARMV8A_SCR         34
#define ARMV8A_PRSR        197
#define ARMV8A_OSLAR       192
#define ARMV8A_AUTHSTATUS  1006

#define ARMV8A_ITR         33
#define ARMV8A_DTRTX       35
#define ARMV8A_DTRRX       32

#define ARMV8A_MRS_DLR(Rt)	(0xd53b4520 | (Rt))
#define ARMV8A_MSR_GP(System, Rt) (0xd5100000 | ((System) << 5) | (Rt))
#define SYSTEM_DBG_DBGDTR_EL0	0b1001100000100000

#define CHANNEL_0 1
#define CHANNEL_1 2

#define HALT_EVENT 0
#define HALT_EVENT_BIT 1

#define RESTART_EVENT 1
#define RESTART_EVENT_BIT 2

#define ARMV8A_CTICONTROL  (16384 + 0)
#define ARMV8A_CTIGATE     (16384 + 80)
#define ARMV8A_CTIINEN(i)  (16384 + 8 + (i))
#define ARMV8A_CTIOUTEN(i) (16384 + 40 + (i))
#define ARMV8A_CTIAPPPULSE (16384 + 7)
#define ARMV8A_CTIINTACK   (16384 + 4)
#define ARMV8A_CTIAPPCLEAR (16384 + 6)
#define ARMV8A_CTITRIGOUTSTATUS (16384 + 77)

#endif
