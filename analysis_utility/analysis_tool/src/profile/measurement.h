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

class BasicBlock;
class Cfg;
class Function;

class Measurement {
public:
  QVector<BasicBlock*> callStack;

  unsigned core;
  BasicBlock *bb;
  Function *func;
  quint64 time;
  quint64 timeSinceLast;
  double power[LYNSYN_SENSORS];

  quint32 sequence;

  static unsigned counter[LYNSYN_MAX_CORES];

  Measurement() {}

  Measurement(unsigned core, BasicBlock *bb, uint64_t time, uint64_t timeSinceLast, double power[]) {
    this->core = core;
    this->bb = bb;
    this->func = NULL;
    this->time = time;
    this->timeSinceLast = timeSinceLast;
    for(unsigned i = 0; i < LYNSYN_SENSORS; i++) {
      this->power[i] = power[i];
    }
    sequence = Measurement::counter[core]++;
  }

  Measurement(unsigned core, BasicBlock *bb, Function *func, uint64_t time, uint64_t timeSinceLast, double power[]) {
    this->core = core;
    this->bb = bb;
    this->func = func;
    this->time = time;
    this->timeSinceLast = timeSinceLast;
    for(unsigned i = 0; i < LYNSYN_SENSORS; i++) {
      this->power[i] = power[i];
    }
    sequence = Measurement::counter[core]++;
  }

  void read(QFile &file, Cfg *cfg);
  void write(QFile &file);

};

#endif
