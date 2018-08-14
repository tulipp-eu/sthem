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

#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <QVector>
#include <QFile>

#include "analysis_tool.h"
#include "project/pmu.h"

class BasicBlock;
class Cfg;
class Function;

class Measurement {
public:
  quint32 sequence;
  quint64 time;

  unsigned core;
  BasicBlock *bb;

  static unsigned counter[Pmu::MAX_CORES];

  Measurement() {}

  Measurement(uint64_t time, unsigned core, BasicBlock *bb) {
    this->time = time;
    this->core = core;
    this->bb = bb;
    sequence = Measurement::counter[core]++;
  }

};

#endif
