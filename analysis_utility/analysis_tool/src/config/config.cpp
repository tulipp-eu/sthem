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

QString Config::workspace;
Config::ColorMode Config::colorMode;
bool Config::includeAllInstructions;
bool Config::includeProfData;
bool Config::includeId;
QString Config::cmake;
QString Config::clang;
QString Config::clangpp;
QString Config::opt;
QString Config::llc;
QString Config::llvm_ir_parser;
QString Config::tulipp_source_tool;
QString Config::as;
QString Config::linker;
QString Config::linkerpp;
QString Config::asUs;
QString Config::linkerUs;
QString Config::linkerppUs;
unsigned Config::core;
unsigned Config::sensor;
unsigned Config::window;
unsigned Config::sdsocVersion;
QString Config::extraCompileOptions;
QString Config::projectDir;
double Config::overrideSamplePeriod;
bool Config::overrideSamplePc;
bool Config::overrideNoSamplePc;
bool Config::functionsInTable;
bool Config::regionsInTable;
bool Config::loopsInTable;
bool Config::basicblocksInTable;
bool Config::sdsocProject;
