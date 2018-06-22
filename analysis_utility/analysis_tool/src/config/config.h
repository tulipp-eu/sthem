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

class Config {

public:

  enum ColorMode {
    STRUCT,
    RUNTIME,
    POWER,
    ENERGY
  };

  static QString workspace;
  static ColorMode colorMode;
  static bool includeAllInstructions;
  static bool includeProfData;
  static bool includeId;
  static QString clang;
  static QString clangpp;
  static QString llc;
  static QString llvm_ir_parser;
  static QString tulipp_source_tool;
  static QString as;
  static QString linker;
  static QString linkerpp;
  static QString xilinxDir;
  static QString hwserver;
  static unsigned hwport;
  static unsigned core;
  static unsigned sensor;
  static unsigned window;
  static unsigned sdsocVersion;
};

#endif
