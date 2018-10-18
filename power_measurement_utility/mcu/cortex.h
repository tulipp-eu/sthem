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

#ifndef CORTEX_H
#define CORTEX_H

///////////////////////////////////////////////////////////////////////////////
// ADIV5

#define ADIV5_MAX_APS 10

#define ADIV5_AP_REG(x)   (ADIV5_APnDP | (x))
#define ADIV5_AP_CSW      ADIV5_AP_REG(0x00)
#define ADIV5_AP_TAR      ADIV5_AP_REG(0x04)
#define ADIV5_AP_DRW      ADIV5_AP_REG(0x0C)
#define ADIV5_AP_IDR      ADIV5_AP_REG(0xFC)

#define ADIV5_APnDP       0x100

#define ADIV5_LOW_WRITE   0
#define ADIV5_LOW_READ    1

#define JTAG_ACK_OK   0x02
#define JTAG_ACK_WAIT 0x01

#define JTAG_IR_ABORT  0x8
#define JTAG_IR_DPACC  0xA
#define JTAG_IR_APACC  0xB

#define ADIV5_DP_IDCODE   0x0
#define ADIV5_DP_ABORT    0x0
#define ADIV5_DP_CTRLSTAT 0x4
#define ADIV5_DP_SELECT   0x8
#define ADIV5_DP_RDBUFF   0xC

#define ADIV5_DP_CTRLSTAT_CSYSPWRUPREQ	(1u << 30)
#define ADIV5_DP_CTRLSTAT_CSYSPWRUPACK	(1u << 31)
#define ADIV5_DP_CTRLSTAT_CDBGPWRUPACK	(1u << 29)
#define ADIV5_DP_CTRLSTAT_CDBGPWRUPREQ	(1u << 28)

#define ADIV5_DP_ABORT_DAPABORT		(1 << 0)

///////////////////////////////////////////////////////////////////////////////
// Cortex A9

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

#define A9_DIDR    0
#define A9_PCSR    33
#define A9_DSCR    34
#define A9_DRCR    36
#define A9_BVR(i)  (64+(i))
#define A9_BCR(i)  (80+(i))

///////////////////////////////////////////////////////////////////////////////
// Cortex A53

#define A53_PCSR_L      40
#define A53_PCSR_H      43
#define A53_BVR_L(i)    (256+(16*(i)))
#define A53_BVR_H(i)    (257+(16*(i)))
#define A53_BCR(i)      (258+(16*(i)))
#define A53_DFR0        842
#define A53_SCR         34
#define A53_PRSR        197
#define A53_OSLAR       192

#define A53_ITR         33
#define A53_DTRTX       35
#define A53_DTRRX       32

#define MCR 0xee000010
#define CPREG(coproc, opc1, rt, crn, crm, opc2) \
	(((opc1) << 21) | ((crn) << 16) | ((rt) << 12) | \
        ((coproc) << 8) | ((opc2) << 5) | (crm))
#define DTRTXint CPREG(14, 0, 0, 0, 5, 0)

#define CHANNEL_0 1
#define CHANNEL_1 2

#define HALT_EVENT 0
#define HALT_EVENT_BIT 1

#define RESTART_EVENT 1
#define RESTART_EVENT_BIT 2

#define A53_CTICONTROL  (16384 + 0)
#define A53_CTIGATE     (16384 + 80)
#define A53_CTIINEN(i)  (16384 + 8 + (i))
#define A53_CTIOUTEN(i) (16384 + 40 + (i))
#define A53_CTIAPPPULSE (16384 + 7)
#define A53_CTIINTACK   (16384 + 4)
#define A53_CTIAPPCLEAR (16384 + 6)
#define A53_CTITRIGOUTSTATUS (16384 + 77)

#endif
