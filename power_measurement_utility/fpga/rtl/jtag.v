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

`define ST_JTAG_IDLE       0
`define ST_JTAG_STORE      1
`define ST_JTAG_EXECUTE    2
`define ST_JTAG_FLUSH      3

module jtag_controller
  (
   input wire       clk,
   input wire       rst,

   output reg       jtag_tms,
   input wire       jtag_tck,
   input wire       jtag_tdo,
   output reg       jtag_tdi,
   output wire      jtag_trst,

   output reg       jtag_ce,

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

   reg [1:0]        jtag_state;
   reg [1:0]        next_jtag_state;

   reg [12:0]       words;
   reg [2:0]        bits;

   // memory for stored sequence
   reg              write_enable;
   reg [12:0]       write_addr;
   wire [23:0]      write_data;
   reg [12:0]       read_addr;
   reg [23:0]       read_data;
   reg [23:0]       ram [8191:0];

   always @(posedge clk) begin
      if(write_enable)
        ram[write_addr] <= write_data;
   end
   always @(posedge clk) begin
      read_data <= ram[read_addr];
   end

   // output on falling edge
   always @(negedge clk) begin
      jtag_tdi <= tdi_seq[0];
      jtag_tms <= tms_seq[0];
   end

   always @(posedge clk) begin
      if(rst) begin
         jtag_cnt <= 8'h80;
         read_seq <= 0;
         tdi_seq <= 0;
         tms_seq <= 0;
         jtag_ce <= 0;
         jtag_state <= `ST_JTAG_IDLE;
         write_addr <= 0;
         read_addr <= 0;
         words <= 0;
         bits <= 0;
         tdo_cnt <= 8'h01;
         in_seq_flushed <= 0;
         in_seq_we <= 0;

      end else begin
         jtag_ce <= jtag_ce_i;

         in_seq_we <= 0;

         jtag_state <= next_jtag_state;

         if(jtag_ce && read_seq[0]) begin
            in_seq_tdo <= {jtag_tdo,in_seq_tdo[7:1]};
            tdo_cnt <= {tdo_cnt[6:0],tdo_cnt[7]};
            if(tdo_cnt == 8'h80) begin
               in_seq_we <= 1;
            end
         end

         case(jtag_state) 

           `ST_JTAG_IDLE: begin
              if(!out_seq_empty) begin
                 case(out_seq_command)
                   `FIFO_CMD_WR: begin
                      in_seq_flushed <= 0;
                      if(jtag_cnt[7]) begin
                         tdi_seq <= out_seq_tdi;
                         tms_seq <= out_seq_tms;
                         read_seq <= {out_seq_read[0],out_seq_read[0],out_seq_read[0],out_seq_read[0],
                                      out_seq_read[0],out_seq_read[0],out_seq_read[0],out_seq_read[0]};
                         case(out_seq_bits)
                           0: jtag_cnt <= 8'h01;
                           1: begin
                              jtag_cnt <= 8'h80;
                           end
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
                   `FIFO_CMD_STORE: begin
                      bits <= out_seq_tms[2:0];
                      if(out_seq_tms[2:0] == 0) words <= {out_seq_tdi[7:0],out_seq_tms[7:3]};
                      else words <= {out_seq_tdi[7:0],out_seq_tms[7:3]} + 1;
                   end
                   `FIFO_CMD_EXECUTE: begin
                      in_seq_flushed <= 0;
                   end
                 endcase
              end
           end

           `ST_JTAG_STORE: begin
              if(!out_seq_empty)
                write_addr <= write_addr + 1;
           end

           `ST_JTAG_EXECUTE: begin
              if(jtag_cnt[7]) begin
                 read_addr <= read_addr + 1;
                 read_seq <= read_data[7:0];
                 tdi_seq <= read_data[15:8];
                 tms_seq <= read_data[23:16];
                 if(read_addr == (words - 1)) begin
                    case(bits)
                      0: jtag_cnt <= 8'h01;
                      1: jtag_cnt <= 8'h80;
                      2: jtag_cnt <= 8'h40;
                      3: jtag_cnt <= 8'h20;
                      4: jtag_cnt <= 8'h10;
                      5: jtag_cnt <= 8'h08;
                      6: jtag_cnt <= 8'h04;
                      7: jtag_cnt <= 8'h02;
                    endcase
                 end else if(read_addr != words) begin
                    jtag_cnt <= 8'h01;
                 end
              end else begin
                 jtag_cnt <= {jtag_cnt[6:0],jtag_cnt[7]};
                 read_seq <= {1'h0,read_seq[7:1]};
                 tdi_seq <= {1'h0,tdi_seq[7:1]};
                 tms_seq <= {1'h0,tms_seq[7:1]};
              end
           end

           `ST_JTAG_FLUSH: begin
              if(tdo_cnt[0] == 0) begin
                 in_seq_we <= 1;
              end
              tdo_cnt <= 8'h01;
              in_seq_flushed <= 1;
              write_addr <= 0;
              read_addr <= 0;
           end
           
           default:
             ;
         endcase

      end

   end

   assign write_data = {out_seq_tms,out_seq_tdi,out_seq_read};

   always @* begin
      jtag_ce_i <= 0;
      out_seq_re <= 0;
      write_enable <= 0;

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
                `FIFO_CMD_STORE: begin
                   next_jtag_state <= `ST_JTAG_STORE;
                   out_seq_re <= 1;
                end
                `FIFO_CMD_EXECUTE: begin
                   next_jtag_state <= `ST_JTAG_EXECUTE;
                   out_seq_re <= 1;
                end
                `FIFO_CMD_FLUSH: begin
                   next_jtag_state <= `ST_JTAG_FLUSH;
                   out_seq_re <= 1;
                end
              endcase
           end
        end

        `ST_JTAG_STORE: begin
           if(!out_seq_empty) begin
              out_seq_re <= 1;
              write_enable <= 1;
              if(write_addr == (words - 1)) begin
                 next_jtag_state <= `ST_JTAG_IDLE;
              end
           end
        end

        `ST_JTAG_EXECUTE: begin
           jtag_ce_i <= 1;

           if(jtag_cnt[7]) begin
              if(read_addr == words) begin
                 jtag_ce_i <= 0;
                 next_jtag_state <= `ST_JTAG_IDLE;
              end
           end
        end

        `ST_JTAG_FLUSH: begin
           next_jtag_state <= `ST_JTAG_IDLE;
        end

        default:
          ;
      endcase

   end

endmodule
