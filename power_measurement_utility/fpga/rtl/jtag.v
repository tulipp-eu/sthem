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

`define ST_JTAG_IDLE               0
`define ST_JTAG_EXECUTE_INIT       1
`define ST_JTAG_EXECUTE_LOOP       2
`define ST_JTAG_EXECUTE_ACK        3
`define ST_JTAG_EXECUTE_DATA       4
`define ST_JTAG_EXECUTE_POST       5
`define ST_JTAG_EXECUTE_PUSH_DATA  6
`define ST_JTAG_EXECUTE_NEXT       7
`define ST_JTAG_FLUSH              8
`define ST_JTAG_STORE_SEQ          9
`define ST_JTAG_STORE_PROG1       10
`define ST_JTAG_STORE_PROG2       11
`define ST_JTAG_STORE_PROG3       12
`define ST_JTAG_STORE_PROG4       13
`define ST_JTAG_STORE_PROG5       14
`define ST_JTAG_STORE_PROG6       15
`define ST_JTAG_STORE_PROG7       16
`define ST_JTAG_STORE_PROG8       17

`define PUSH_IDLE          0
`define PUSH_ACK           1
`define PUSH_DATA          2
`define PUSH_FLUSH         3

module jtag_controller
  (
   input wire       clk,
   input wire       rst,

   output reg       jtag_tms,
   input wire       jtag_tck,
   input wire       jtag_tdo,
   output reg       jtag_tdi,
   output wire      jtag_trst,

   output reg       jtag_ce = 0,

   input wire       out_seq_empty,
   input wire [7:0] out_seq_tms,
   input wire [7:0] out_seq_tdi,
   input wire [2:0] out_seq_bits,
   input wire [4:0] out_seq_command,
   input wire [7:0] out_seq_read,
   output reg       out_seq_re,

   input wire       in_seq_full,
   output reg       in_seq_we, 
   output reg [7:0] in_seq_tdo,
   output reg       in_seq_flushed
   );

   assign jtag_trst = 1;

   reg              jtag_ce_i;

   reg [7:0]        read_seq;
   reg [7:0]        tdi_seq;
   reg [7:0]        tms_seq;

   reg [7:0]        jtag_cnt;
   reg [7:0]        tdo_cnt;
   reg [4:0]        push_cnt;

   reg [5:0]        jtag_state;
   reg [5:0]        next_jtag_state;

   reg [3:0]        push_state;
   reg [3:0]        next_push_state;

   reg [12:0]       words;
   reg [2:0]        bits;

   // sequence
   reg              write_enable_tdi;
   reg              write_enable_tms;
   wire             read_data_tdi;
   wire             read_data_tms;
   reg [14:0]       addr;
   reg [14:0]       addr_r;
   wire [7:0]       write_data_tdi;             
   wire [7:0]       write_data_tms;

   assign write_data_tdi = out_seq_tms;
   assign write_data_tms = out_seq_tms;

   reg              pos_write_enable;
   reg [5:0]        pos_addr;
   reg [5:0]        last_pos_addr;
   reg [60:0]       pos_read_data;
   wire [60:0]      pos_write_data;
   reg [60:0]       posram [63:0];

   reg [2:0]        ack;        
   reg [31:0]       data;
   reg              push_ack;
   reg              push_data;
   reg              push_flush;
   reg              push_unflush;
   reg              read_ack;
   reg              read_data;

   reg              readFlag;
   reg [14:0]       initpos;
   reg [14:0]       looppos;
   reg [14:0]       ackpos;
   reg [7:0]        endposL;

   wire [14:0]      initpos_read;
   wire [14:0]      looppos_read;
   wire [14:0]      ackpos_read;
   wire [14:0]      endpos_read;

   assign pos_write_data = {readFlag, initpos, looppos, ackpos, out_seq_tms[6:0], endposL};
   assign readFlag_read = pos_read_data[60];
   assign initpos_read = pos_read_data[59:45];
   assign looppos_read = pos_read_data[44:30];
   assign ackpos_read = pos_read_data[29:15];
   assign endpos_read = pos_read_data[14:0];

   always @(posedge clk) begin
      if(pos_write_enable)
        posram[pos_addr] <= pos_write_data;
   end
   always @(posedge clk) begin
      pos_read_data <= posram[pos_addr];
   end

   BRAM_SINGLE_MACRO #(
                       .BRAM_SIZE("18Kb"), // Target BRAM, "18Kb" or "36Kb"
                       .DEVICE("7SERIES"), // Target Device: "7SERIES"
                       .DO_REG(0), // Optional output register (0 or 1)
                       .WRITE_WIDTH(8), // Valid values are 1-72 (37-72 only valid when BRAM_SIZE="36Kb")
                       .READ_WIDTH(1), // Valid values are 1-72 (37-72 only valid when BRAM_SIZE="36Kb")
                       .WRITE_MODE("READ_FIRST") // "WRITE_FIRST", "READ_FIRST", or "NO_CHANGE"
                       ) tdiram (
                                 .DO(read_data_tdi), // Output data, width defined by READ_WIDTH parameter
                                 .ADDR(addr), // Input address, width defined by read/write port depth
                                 .CLK(clk), // 1-bit input clock
                                 .DI(write_data_tdi), // Input data port, width defined by WRITE_WIDTH parameter
                                 .EN(1), // 1-bit input RAM enable
                                 .REGCE(1), // 1-bit input output register enable
                                 .RST(rst), // 1-bit input reset
                                 .WE(write_enable_tdi) // Input write enable, width defined by write port depth
                                 );

   BRAM_SINGLE_MACRO #(
                       .BRAM_SIZE("18Kb"), // Target BRAM, "18Kb" or "36Kb"
                       .DEVICE("7SERIES"), // Target Device: "7SERIES"
                       .DO_REG(0), // Optional output register (0 or 1)
                       .WRITE_WIDTH(8), // Valid values are 1-72 (37-72 only valid when BRAM_SIZE="36Kb")
                       .READ_WIDTH(1), // Valid values are 1-72 (37-72 only valid when BRAM_SIZE="36Kb")
                       .WRITE_MODE("READ_FIRST") // "WRITE_FIRST", "READ_FIRST", or "NO_CHANGE"
                       ) tmsram (
                                 .DO(read_data_tms), // Output data, width defined by READ_WIDTH parameter
                                 .ADDR(addr), // Input address, width defined by read/write port depth
                                 .CLK(clk), // 1-bit input clock
                                 .DI(write_data_tms), // Input data port, width defined by WRITE_WIDTH parameter
                                 .EN(1), // 1-bit input RAM enable
                                 .REGCE(1), // 1-bit input output register enable
                                 .RST(rst), // 1-bit input reset
                                 .WE(write_enable_tms) // Input write enable, width defined by write port depth
                                 );

   // output on falling edge
   always @(negedge clk) begin
      jtag_tdi <= tdi_seq[0];
      jtag_tms <= tms_seq[0];
   end

   always @(posedge clk) begin
      if(rst) begin
         in_seq_tdo <= 0;
         tdo_cnt <= 8'h01;
         in_seq_we <= 0;
         push_state <= `PUSH_IDLE;
         push_cnt <= 0;
         in_seq_flushed <= 0;
         data <= 0;

      end else begin
         in_seq_we <= 0;

         case(push_state)

           `PUSH_IDLE: begin
              push_cnt <= 0;

              if(push_unflush) begin
                 in_seq_flushed <= 0;
              end

              if(jtag_ce && read_seq[0]) begin
                 in_seq_tdo <= {jtag_tdo,in_seq_tdo[7:1]};
                 tdo_cnt <= {tdo_cnt[6:0],tdo_cnt[7]};
                 if(tdo_cnt == 8'h80) begin
                    in_seq_we <= 1;
                 end
              end else if(jtag_ce && read_ack) begin
                 ack <= {jtag_tdo, ack[2:1]};
              end else if(jtag_ce && read_data) begin
                 data <= {jtag_tdo, data[31:1]};
              end else if(push_ack) begin
                 push_state <= `PUSH_ACK;
              end else if(push_data) begin
                 push_state <= `PUSH_DATA;
              end else if(push_flush) begin
                 push_state <= `PUSH_FLUSH;
              end
           end

           `PUSH_ACK: begin
              in_seq_tdo <= {ack[0],in_seq_tdo[7:1]};
              ack <= {1'b0,ack[2:1]};
              tdo_cnt <= {tdo_cnt[6:0],tdo_cnt[7]};
              push_cnt <= push_cnt + 1;
              if(tdo_cnt == 8'h80) begin
                 in_seq_we <= 1;
              end
              if(push_cnt == 2) begin
                 push_state <= `PUSH_IDLE;
              end
           end

           `PUSH_DATA: begin
              in_seq_tdo <= {data[0],in_seq_tdo[7:1]};
              data <= {1'b0,data[31:1]};
              tdo_cnt <= {tdo_cnt[6:0],tdo_cnt[7]};
              push_cnt <= push_cnt + 1;
              if(tdo_cnt == 8'h80) begin
                 in_seq_we <= 1;
              end
              if(push_cnt == 31) begin
                 push_state <= `PUSH_IDLE;
              end
           end

           `PUSH_FLUSH: begin
              if(tdo_cnt[0] == 0) begin
                 in_seq_we <= 1;
              end
              push_state <= `PUSH_IDLE;
              in_seq_flushed <= 1;
              tdo_cnt <= 8'h01;
           end

         endcase;
      end
   end      

   always @(posedge clk) begin
      if(rst) begin
         jtag_cnt <= 8'h80;
         read_seq <= 0;
         tdi_seq <= 0;
         tms_seq <= 0;
         jtag_ce <= 0;
         jtag_state <= `ST_JTAG_IDLE;
         addr_r <= -8;
         pos_addr <= 0;
         words <= 0;
         bits <= 0;
         push_unflush <= 0;

      end else begin
         jtag_ce <= jtag_ce_i;
         addr_r <= addr;
         jtag_state <= next_jtag_state;
         push_unflush <= 0;
         read_ack <= 0;
         read_data <= 0;

         case(jtag_state) 
 
           `ST_JTAG_IDLE: begin
              if(!out_seq_empty) begin
                 case(out_seq_command)
                   `FIFO_CMD_WR: begin
                      push_unflush <= 1;
                      if(jtag_cnt[7]) begin
                         tdi_seq <= out_seq_tdi;
                         tms_seq <= out_seq_tms;
                         read_seq <= {out_seq_read[0],out_seq_read[0],out_seq_read[0],out_seq_read[0],
                                      out_seq_read[0],out_seq_read[0],out_seq_read[0],out_seq_read[0]};
                         case(out_seq_bits)
                           0: jtag_cnt <= 8'h01;
                           1: jtag_cnt <= 8'h80;
                           2: jtag_cnt <= 8'h40;
                           3: jtag_cnt <= 8'h20;
                           4: jtag_cnt <= 8'h10;
                           5: jtag_cnt <= 8'h08;
                           6: jtag_cnt <= 8'h04;
                           7: jtag_cnt <= 8'h02;
                         endcase
                      end else begin
                         jtag_cnt <= {jtag_cnt[6:0],jtag_cnt[7]};
                         tdi_seq <= {1'h0,tdi_seq[7:1]};
                         tms_seq <= {1'h0,tms_seq[7:1]};
                      end
                   end
                   `FIFO_CMD_STORE_PROG: begin
                      readFlag <= out_seq_tms[0];
                   end
                   `FIFO_CMD_EXECUTE: begin
                      push_unflush <= 1;
                   end
                 endcase
              end
           end

           `ST_JTAG_STORE_PROG1: begin
              if(!out_seq_empty)
                initpos[7:0] <= out_seq_tms;
           end

           `ST_JTAG_STORE_PROG2: begin
              if(!out_seq_empty)
                initpos[14:8] <= out_seq_tms;
           end

           `ST_JTAG_STORE_PROG3: begin
              if(!out_seq_empty)
                looppos[7:0] <= out_seq_tms;
           end

           `ST_JTAG_STORE_PROG4: begin
              if(!out_seq_empty)
                looppos[14:8] <= out_seq_tms;
           end

           `ST_JTAG_STORE_PROG5: begin
              if(!out_seq_empty)
                ackpos[7:0] <= out_seq_tms;
           end

           `ST_JTAG_STORE_PROG6: begin
              if(!out_seq_empty)
                ackpos[14:8] <= out_seq_tms;
           end

           `ST_JTAG_STORE_PROG7: begin
              if(!out_seq_empty)
                endposL[7:0] <= out_seq_tms;
           end

           `ST_JTAG_STORE_PROG8: begin
              if(!out_seq_empty) begin
                 pos_addr <= pos_addr + 1;
                 last_pos_addr <= pos_addr + 1;
              end
           end

           `ST_JTAG_EXECUTE_INIT: begin
              tdi_seq[0] <= read_data_tdi;
              tms_seq[0] <= read_data_tms;
              read_seq[0] <= 0;
           end

           `ST_JTAG_EXECUTE_LOOP: begin
              tdi_seq[0] <= read_data_tdi;
              tms_seq[0] <= read_data_tms;
              read_seq[0] <= 0;
           end

           `ST_JTAG_EXECUTE_ACK: begin
              tdi_seq[0] <= read_data_tdi;
              tms_seq[0] <= read_data_tms;
              read_seq[0] <= 0;
              read_ack <= 1;
           end

           `ST_JTAG_EXECUTE_DATA: begin
              tdi_seq[0] <= read_data_tdi;
              tms_seq[0] <= read_data_tms;
              read_seq[0] <= 0;
              read_data <= 1;
           end

           `ST_JTAG_EXECUTE_POST: begin
              tdi_seq[0] <= read_data_tdi;
              tms_seq[0] <= read_data_tms;
              read_seq[0] <= 0;
              if((addr_r + 1) == endpos_read) begin
                 if(ack != 3'h01) begin
                    pos_addr <= pos_addr + 1;
                 end
              end
           end

           `ST_JTAG_EXECUTE_PUSH_DATA: begin
           end

           `ST_JTAG_FLUSH: begin
              pos_addr <= 0;
           end
           
           default:
             ;
         endcase

      end

   end

   always @* begin
      jtag_ce_i <= 0;
      out_seq_re <= 0;
      pos_write_enable <= 0;
      write_enable_tdi <= 0;
      write_enable_tms <= 0;
      push_ack <= 0;
      push_data <= 0;
      push_flush <= 0;

      addr <= addr_r;

      next_jtag_state <= jtag_state;

      case(jtag_state)

        `ST_JTAG_IDLE: begin
           if(!out_seq_empty) begin
              case(out_seq_command)
                `FIFO_CMD_WR: begin
                   if(jtag_cnt[6]) begin
                      out_seq_re <= 1;
                      jtag_ce_i <= 1;

                   end else if(jtag_cnt[7]) begin
                      if(!out_seq_empty) begin
                         jtag_ce_i <= 1;
                      end
                      if(out_seq_bits == 1) begin
                         out_seq_re <= 1;
                      end

                   end else begin
                      jtag_ce_i <= 1;
                   end
                end
                `FIFO_CMD_STORE_SEQ: begin
                   next_jtag_state <= `ST_JTAG_STORE_SEQ;
                   write_enable_tdi <= 1;
                   out_seq_re <= 1;
                   addr <= addr_r + 8;
                end
                `FIFO_CMD_STORE_PROG: begin
                   next_jtag_state <= `ST_JTAG_STORE_PROG1;
                   out_seq_re <= 1;
                end
                `FIFO_CMD_EXECUTE: begin
                   if(push_state == `PUSH_IDLE) begin
                      if(initpos_read == ackpos_read) begin
                         next_jtag_state <= `ST_JTAG_EXECUTE_ACK;
                      end else if(initpos_read == looppos_read) begin
                         next_jtag_state <= `ST_JTAG_EXECUTE_LOOP;
                      end else begin
                         next_jtag_state <= `ST_JTAG_EXECUTE_INIT;
                      end
                      out_seq_re <= 1;
                      addr <= initpos_read;
                   end
                end
                `FIFO_CMD_FLUSH: begin
                   next_jtag_state <= `ST_JTAG_FLUSH;
                   out_seq_re <= 1;
                   addr <= -8;
                end
              endcase
           end
        end

        `ST_JTAG_STORE_SEQ: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_IDLE;
              out_seq_re <= 1;
              write_enable_tms <= 1;
           end
        end

        `ST_JTAG_STORE_PROG1: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_STORE_PROG2;
              out_seq_re <= 1;
           end
        end

        `ST_JTAG_STORE_PROG2: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_STORE_PROG3;
              out_seq_re <= 1;
           end
        end

        `ST_JTAG_STORE_PROG3: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_STORE_PROG4;
              out_seq_re <= 1;
           end
        end

        `ST_JTAG_STORE_PROG4: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_STORE_PROG5;
              out_seq_re <= 1;
           end
        end

        `ST_JTAG_STORE_PROG5: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_STORE_PROG6;
              out_seq_re <= 1;
           end
        end

        `ST_JTAG_STORE_PROG6: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_STORE_PROG7;
              out_seq_re <= 1;
           end
        end

        `ST_JTAG_STORE_PROG7: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_STORE_PROG8;
              out_seq_re <= 1;
           end
        end

        `ST_JTAG_STORE_PROG8: begin
           if(!out_seq_empty) begin
              next_jtag_state <= `ST_JTAG_IDLE;
              out_seq_re <= 1;
              pos_write_enable <= 1;
           end
        end

        `ST_JTAG_EXECUTE_INIT: begin
           if((addr_r + 1) == looppos_read) begin
              next_jtag_state <= `ST_JTAG_EXECUTE_LOOP;
           end
           addr <= addr_r + 1;
           jtag_ce_i <= 1;
        end

        `ST_JTAG_EXECUTE_LOOP: begin
           if((addr_r + 1) == ackpos_read) begin
              next_jtag_state <= `ST_JTAG_EXECUTE_ACK;
           end
           addr <= addr_r + 1;
           jtag_ce_i <= 1;
        end

        `ST_JTAG_EXECUTE_ACK: begin
           if((addr_r + 1) == (ackpos_read + 3)) begin
              next_jtag_state <= `ST_JTAG_EXECUTE_DATA;
           end
           addr <= addr_r + 1;
           jtag_ce_i <= 1;
        end

        `ST_JTAG_EXECUTE_DATA: begin
           if((addr_r + 1) == (ackpos_read + 35)) begin
              next_jtag_state <= `ST_JTAG_EXECUTE_POST;
           end
           addr <= addr_r + 1;
           jtag_ce_i <= 1;
        end

        `ST_JTAG_EXECUTE_POST: begin
           if((addr_r + 1) == endpos_read) begin
              if(ack == 3'h01) begin
                 addr <= looppos_read;
                 if(looppos_read == ackpos_read) begin
                    next_jtag_state <= `ST_JTAG_EXECUTE_ACK;
                 end else begin
                    next_jtag_state <= `ST_JTAG_EXECUTE_LOOP;
                 end
              end else begin
                 next_jtag_state <= `ST_JTAG_EXECUTE_PUSH_DATA;
                 if(readFlag_read) push_data <= 1;
              end
           end else begin
              addr <= addr_r + 1;
           end
           jtag_ce_i <= 1;
        end

        `ST_JTAG_EXECUTE_PUSH_DATA: begin
           if(push_state == `PUSH_IDLE) begin
              push_ack <= 1;
              if(pos_addr == last_pos_addr) begin
                 next_jtag_state <= `ST_JTAG_IDLE;
              end else begin
                 next_jtag_state <= `ST_JTAG_EXECUTE_NEXT;
              end
           end
        end

        `ST_JTAG_EXECUTE_NEXT: begin
           addr <= initpos_read;
           if(push_state == `PUSH_IDLE) begin
              if(initpos_read == ackpos_read) begin
                 next_jtag_state <= `ST_JTAG_EXECUTE_ACK;
              end else if(initpos_read == looppos_read) begin
                 next_jtag_state <= `ST_JTAG_EXECUTE_LOOP;
              end else begin
                 next_jtag_state <= `ST_JTAG_EXECUTE_INIT;
              end
           end
        end

        `ST_JTAG_FLUSH: begin
           push_flush <= 1;
           if(push_state == `PUSH_IDLE) begin
              next_jtag_state <= `ST_JTAG_IDLE;
           end
        end

        default:
          ;
      endcase

   end

endmodule
