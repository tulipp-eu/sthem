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

`include "defines.v"

`timescale 1ns/1ps

///////////////////////////////////////////////////////////////////////////////

module lynsyn 
  (
   // Oscillators
   input wire  MCU_OSC,
   input wire  JTAG_OSC,

   // MCU SPI
   output wire INT,
   input wire  SCLK,
   input wire  nSS,
   input wire  MOSI,
   output wire MISO,

   // JTAG_in
   input wire  JTAG_IN_TMS,
   input wire  JTAG_IN_TCK,
   output reg  JTAG_IN_TDO,
   input wire  JTAG_IN_TDI,
   input wire  JTAG_IN_TRST,

   // JTAG_out
   output reg  JTAG_OUT_TMS,
   output reg  JTAG_OUT_TCK,
   input wire  JTAG_OUT_TDO,
   output reg  JTAG_OUT_TDI,
   output reg  JTAG_OUT_TRST
   );

   wire        jtag_tms;
   wire        jtag_tck;
   reg         jtag_tdo;
   wire        jtag_tdi;
   wire        jtag_trst;
   wire [1:0]  jtag_sel;
   wire [4:0]  jtag_test_out;
   wire [4:0]  jtag_test_in;

   wire        jtag_ce;

   wire        fb;
   wire        clk;

   wire        locked;
   wire        rst;

   assign rst = !locked;

   LynsynTop u_lynsyn
     (
      .clk(clk),
      .rst(rst),
      .INT(INT),
      .SCLK(SCLK),
      .nSS(nSS),
      .MOSI(MOSI),
      .MISO(MISO),
      .jtag_tms(jtag_tms),
      .jtag_tck(jtag_tck),
      .jtag_tdo(jtag_tdo),
      .jtag_tdi(jtag_tdi),
      .jtag_trst(jtag_trst),
      .jtag_ce(jtag_ce),
      .jtag_sel(jtag_sel),
      .jtag_test_out(jtag_test_out),
      .jtag_test_in(jtag_test_in)
      );

   // JTAG MUX
   always @* begin
      case(jtag_sel)
        `JTAG_INT: begin
           JTAG_OUT_TMS <= jtag_tms;
           JTAG_OUT_TDI <= jtag_tdi;
           JTAG_OUT_TRST <= jtag_trst;
           JTAG_OUT_TCK <= jtag_tck;

           jtag_tdo <= JTAG_OUT_TDO;

           JTAG_IN_TDO <= 0;
        end
        
        `JTAG_EXT: begin
           JTAG_OUT_TMS <= JTAG_IN_TMS;
           JTAG_OUT_TDI <= JTAG_IN_TDI;
           JTAG_OUT_TRST <= JTAG_IN_TRST;
           JTAG_OUT_TCK <= JTAG_IN_TCK;

           jtag_tdo <= 0;

           JTAG_IN_TDO <= JTAG_OUT_TDO;
        end

        `JTAG_TEST: begin
           JTAG_OUT_TMS <= jtag_test_out[0];
           JTAG_OUT_TDI <= jtag_test_out[1];
           JTAG_OUT_TRST <= jtag_test_out[2];
           JTAG_OUT_TCK <= jtag_test_out[3];

           jtag_tdo <= 0;

           JTAG_IN_TDO <= jtag_test_out[4];
        end
      endcase
   end

   assign jtag_test_in[0] = JTAG_IN_TMS;
   assign jtag_test_in[1] = JTAG_IN_TDI;
   assign jtag_test_in[2] = JTAG_IN_TRST;
   assign jtag_test_in[3] = JTAG_IN_TCK;
   assign jtag_test_in[4] = JTAG_OUT_TDO;
   
   MMCME2_BASE #(.BANDWIDTH("OPTIMIZED"), // Jitter programming ("HIGH","LOW","OPTIMIZED")
                 .CLKFBOUT_MULT_F(30.0), // Multiply value for all CLKOUT (2.000-64.000).
                 .CLKFBOUT_PHASE(0.0),
                 .CLKIN1_PERIOD(50.0),
                 .CLKOUT0_DIVIDE_F(60.0), // Divide amount for CLKOUT0 (1.000-128.000).
                 // CLKOUT0_DUTY_CYCLE - CLKOUT6_DUTY_CYCLE: Duty cycle for each CLKOUT (0.01-0.99).
                 .CLKOUT0_DUTY_CYCLE(0.5),
                 // CLKOUT0_PHASE - CLKOUT6_PHASE: Phase offset for each CLKOUT (-360.000-360.000).
                 .CLKOUT0_PHASE(0.0),
                 .CLKOUT4_CASCADE("FALSE"),
                 .DIVCLK_DIVIDE(1),
                 .REF_JITTER1(0.0), // Reference input jitter in UI (0.000-0.999).
                 .STARTUP_WAIT("TRUE")
                 )
   MMCME2_BASE_inst (.CLKOUT0(clk),
                     .CLKOUT0B(),
                     .CLKOUT1(),
                     .CLKOUT1B(),
                     .CLKOUT2(),
                     .CLKOUT2B(),
                     .CLKOUT3(),
                     .CLKOUT3B(),
                     .CLKOUT4(),
                     .CLKOUT5(),
                     .CLKOUT6(),
                     .CLKFBOUT(fb),
                     .CLKFBOUTB(),
                     .LOCKED(locked),
                     .CLKIN1(JTAG_OSC),
                     .PWRDWN(),
                     .RST(1'b0),
                     .CLKFBIN(fb)
                     );

   // clocks
   BUFGCE jtag_bufg 
     (
      .I(clk),
      .O(jtag_tck),
      .CE(jtag_ce)
      );

endmodule

///////////////////////////////////////////////////////////////////////////////

module LynsynTop
  (
   input wire        clk,
   input wire        rst,

   // MCU SPI
   output wire       INT,
   input wire        SCLK,
   input wire        nSS,
   input wire        MOSI,
   output wire       MISO,

   // JTAG output
   output wire       jtag_tms,
   input wire        jtag_tck,
   input wire        jtag_tdo,
   output wire       jtag_tdi,
   output wire       jtag_trst,

   output wire       jtag_ce,

   // JTAG mux
   output wire [1:0] jtag_sel,
   output wire [4:0] jtag_test_out,
   input wire [4:0]  jtag_test_in
   );

   wire        out_seq_empty;
   wire [7:0]  out_seq_tms;
   wire [7:0]  out_seq_tdi;
   wire [7:0]  out_seq_read;
   wire [2:0]  out_seq_bits;
   wire [4:0]  out_seq_command;
   wire        out_seq_re;
   wire        in_seq_full;
   wire        in_seq_we;
   wire [7:0]  in_seq_tdo;
   wire        in_seq_flushed;
   wire        jtag_rst;

   spi_controller u_spi
     (
      .clk(clk),
      .rst(rst),
      .INT(INT),
      .SCLK(SCLK),
      .nSS(nSS),
      .MOSI(MOSI),
      .MISO(MISO),
      .out_seq_empty(out_seq_empty),
      .out_seq_tms(out_seq_tms),
      .out_seq_tdi(out_seq_tdi),
      .out_seq_read(out_seq_read),
      .out_seq_bits(out_seq_bits),
      .out_seq_command(out_seq_command),
      .out_seq_re(out_seq_re),
      .in_seq_full(in_seq_full),
      .in_seq_we(in_seq_we),
      .in_seq_tdo(in_seq_tdo),
      .in_seq_flushed(in_seq_flushed),
      .jtag_sel(jtag_sel),
      .jtag_test_out(jtag_test_out),
      .jtag_test_in(jtag_test_in),
      .jtag_rst(jtag_rst)
      );

   jtag_controller u_jtag
     (
      .clk(clk),
      .rst(rst | jtag_rst),
      .jtag_tms(jtag_tms),
      .jtag_tck(jtag_tck),
      .jtag_tdo(jtag_tdo),
      .jtag_tdi(jtag_tdi),
      .jtag_trst(jtag_trst),
      .jtag_ce(jtag_ce),
      .out_seq_empty(out_seq_empty),
      .out_seq_tms(out_seq_tms),
      .out_seq_tdi(out_seq_tdi),
      .out_seq_read(out_seq_read),
      .out_seq_bits(out_seq_bits),
      .out_seq_command(out_seq_command),
      .out_seq_re(out_seq_re),
      .in_seq_full(in_seq_full),
      .in_seq_we(in_seq_we),
      .in_seq_tdo(in_seq_tdo),
      .in_seq_flushed(in_seq_flushed)
      );

endmodule

///////////////////////////////////////////////////////////////////////////////
