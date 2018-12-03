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

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QVector>

#define TCF_PROFILER 0
#define LYNSYN_PROFILER 1

#define DEFAULT_TCF_UPLOAD_SCRIPT \
  "connect\n"                                                       \
  "source [lindex [exec find _sds | grep sdk.*ps7_init.tcl] 0]\n"   \
  "targets 2\n"                                                     \
  "rst -system\n"                                                   \
  "after 3000\n"                                                    \
  "while {[catch {fpga -file $name.elf.bit}] eq 1} {rst -system}\n" \
  "ps7_init\n"                                                      \
  "ps7_post_config\n"                                               \
  "dow $name.elf\n"

#define DEFAULT_TCF_UPLOAD_SCRIPT_US \
  "connect\n" \
  "\n" \
  "targets -set -nocase -filter {name =~\"APU*\"} -index 1\n" \
  "rst -system\n" \
  "after 3000\n" \
  "\n" \
  "fpga -file $name.elf.bit\n" \
  "configparams force-mem-access 1\n" \
  "\n" \
  "targets -set -nocase -filter {name =~\"APU*\"} -index 1\n" \
  "source [lindex [exec find _sds | grep psu_init.tcl] 0]\n" \
  "psu_init\n" \
  "after 1000\n" \
  "psu_ps_pl_isolation_removal\n" \
  "after 1000\n" \
  "psu_ps_pl_reset_config\n" \
  "catch {psu_protection}\n" \
  "\n" \
  "targets -set -nocase -filter {name =~ \"*A53*0\"}\n" \
  "rst -processor\n" \
  "dow $name.elf\n" \
  "configparams force-mem-access 0\n" \
  "after 1000\n"

class Config {

public:

  enum ColorMode {
    STRUCT,
    RUNTIME,
    POWER,
    ENERGY,
    RUNTIME_FRAME,
    POWER_FRAME,
    ENERGY_FRAME
  };

  static QString workspace;
  static ColorMode colorMode;
  static bool includeAllInstructions;
  static bool includeProfData;
  static bool includeId;
  static QString clang;
  static QString clangpp;
  static QString opt;
  static QString llc;
  static QString llvm_ir_parser;
  static QString tulipp_source_tool;
  static QString as;
  static QString linker;
  static QString linkerpp;
  static QString asUs;
  static QString linkerUs;
  static QString linkerppUs;
  static unsigned core;
  static unsigned sensor;
  static unsigned window;
  static unsigned sdsocVersion;
  static QString extraCompileOptions;
  static QString projectDir;
  static int64_t overrideSamplePeriod;
  static bool overrideSamplePc;
  static bool overrideNoSamplePc;
};

#endif
