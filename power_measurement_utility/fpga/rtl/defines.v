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

`define JTAG_EXT  0
`define JTAG_INT  1
`define JTAG_TEST 2

`define MAGIC 8'had

// SPI commands

`define SPI_CMD_STATUS          0 // CMD/status
`define SPI_CMD_MAGIC           1 // CMD/status - 0/data
`define SPI_CMD_JTAG_SEL        2 // CMD/status - sel/0
`define SPI_CMD_WR_SEQ          3 // CMD/status - size/0 - (tdidata/0 tmsdata/0)* 0/0
`define SPI_CMD_RDWR_SEQ        4 // CMD/status - size/0 - (tdidata/0 tmsdata/0)* 0/0
`define SPI_CMD_GET_DATA        5 // CMD/status - (0/data)*
`define SPI_CMD_STORE_SEQ       6 // CMD/status - size/0 (readdata/0 tdidata/0 tmsdata/0)* 0/0
`define SPI_CMD_EXECUTE_SEQ     7 // CMD/status - 0/0
`define SPI_CMD_JTAG_TEST       8 // CMD/status - 0/data
`define SPI_CMD_OSC_TEST        9 // CMD/status - 0/data

// SPI -> JTAG controller commands

`define FIFO_CMD_WR       5'h0
`define FIFO_CMD_STORE    5'h1
`define FIFO_CMD_EXECUTE  5'h2
`define FIFO_CMD_FLUSH    6'h3
