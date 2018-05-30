/******************************************************************************
 *
 *  This file is part of the TULIPP Analysis Utility
 *
 *  Copyright 2018 Asbjørn Djupdal, NTNU, TULIPP EU Project
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "config.h"

const QStringList Config::defaultSystemIncludeDirs = {
  "/opt/Xilinx/SDx/2017.2/target/aarch64-none/include",
  "/opt/Xilinx/SDx/2017.2/Vivado_HLS/include",
  "/opt/Xilinx/SDx/2017.2/SDK/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/include/c++/6.2.1",
  "/opt/Xilinx/SDx/2017.2/SDK/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/include/c++/6.2.1/aarch64-none-elf",
  "/opt/Xilinx/SDx/2017.2/SDK/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/include/c++/6.2.1/backward",
  "/opt/Xilinx/SDx/2017.2/SDK/gnu/aarch64/lin/aarch64-none/lib/gcc/aarch64-none-elf/6.2.1/include",
  "/opt/Xilinx/SDx/2017.2/SDK/gnu/aarch64/lin/aarch64-none/lib/gcc/aarch64-none-elf/6.2.1/include-fixed",
  "/opt/Xilinx/SDx/2017.2/SDK/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/include",
  "/opt/Xilinx/SDx/2017.2/SDK/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/libc/usr/include"
};

const QStringList Config::defaultSystemXmls = {
};

const QStringList Config::defaultSystemBins = {
};

QString Config::workspace;
Config::ColorMode Config::colorMode;
bool Config::includeAllInstructions;
bool Config::includeProfData;
bool Config::includeId;
QString Config::clang;
QString Config::clangpp;
QString Config::llc;
QString Config::llvm_ir_parser;
QString Config::tulipp_source_tool;
QString Config::as;
QString Config::linker;
QString Config::linkerpp;
QString Config::xilinxDir;
QString Config::hwserver;
unsigned Config::hwport;
unsigned Config::core;
unsigned Config::sensor;
unsigned Config::window;
unsigned Config::sdsocVersion;
