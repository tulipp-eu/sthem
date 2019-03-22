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
  "source [lindex [exec find _sds | grep ps7_init.tcl] 0]\n"   \
  "targets -set -nocase -filter {name =~\"ARM*#0\"} -index 0\n"                                                     \
  "rst -system\n"                                                   \
  "after 3000\n"                                                    \
  "while {[catch {fpga -file %name%.elf.bit}] eq 1} {rst -system}\n" \
  "ps7_init\n"                                                      \
  "ps7_post_config\n"                                               \
  "dow %name%.elf\n"

#define DEFAULT_TCF_UPLOAD_SCRIPT_US \
  "connect\n" \
  "\n" \
  "targets -set -nocase -filter {name =~\"APU*\"} -index 1\n" \
  "rst -system\n" \
  "\n" \
  "targets -set -nocase -filter {name =~\"RPU*\"} -index 1\n" \
  "rst -cores\n" \
  "\n" \
  "after 3000\n" \
  "\n" \
  "fpga -file %name%.elf.bit\n" \
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
  "dow %name%.elf\n" \
  "configparams force-mem-access 0\n" \
  "after 1000\n"

#define DEFAULT_TCF_UPLOAD_SCRIPT_HIPPEROS_US \
  "connect\n" \
  "\n" \
  "targets -set -nocase -filter {name =~ \"*A53*0\"}\n" \
  "rst -system\n" \
  "\n" \
  "after 1000\n" \
  "con\n" \
  "after 3000\n" \
  "stop\n" \
  "\n" \
  "dow -data bin/kernel.img 0x12100000\n" \
  "dow -data bin/uinitrd.img 0x14000000\n" \
  "\n" \
  "exec echo \"bootm 0x12100000 0x14000000\" > /dev/ttyUSB0\n" \
  "\n" \
  "con\n" \
  "after 1000\n" \
  "stop\n" \
  "\n" \
  "targets -set -nocase -filter {name =~ \"*A53*1\"}\n" \
  "con\n" \
  "after 10\n" \
  "targets -set -nocase -filter {name =~ \"*A53*2\"}\n" \
  "con\n" \
  "after 10\n" \
  "targets -set -nocase -filter {name =~ \"*A53*3\"}\n" \
  "con\n" \
  "after 10\n"

#define DEFAULT_UPLOAD_SCRIPT_LINUX \
  "echo \"hei %name%\"\n"

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
  static QString cmake;
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
  static double overrideSamplePeriod;
  static bool overrideSamplePc;
  static bool overrideNoSamplePc;
  static bool functionsInTable;
  static bool regionsInTable;
  static bool loopsInTable;
  static bool basicblocksInTable;
  static bool sdsocProject;
};

#endif
