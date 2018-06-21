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

#include <QTextStream>
#include <QtWidgets>
#include <QTreeView>
#include <QMainWindow>

#include "profile.h"

Profile::Profile() {
}

Profile::~Profile() {
  clear();
}

void Profile::addMeasurement(Measurement measurement) {
  measurements.push_back(measurement);

  unsigned core = measurement.core;
  BasicBlock *bb = measurement.bb;
  std::vector<Measurement> *mments = NULL;

  auto it = measurementsPerBb[core].find(bb);

  if(it == measurementsPerBb[core].end()) {
    mments = new std::vector<Measurement>();
    measurementsPerBb[core][bb] = mments;
  } else {
    mments = it->second;
  }

  mments->push_back(measurement);
}

void Profile::getProfData(unsigned core, BasicBlock *bb, double *runtime, double *energy, QVector<BasicBlock*> callStack, QVector<Measurement> *measurements) {
  *runtime = 0;
  for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
    energy[i] = 0;
  }
  
  std::vector<Measurement> *mments;
  auto it = measurementsPerBb[core].find(bb);
  if(it == measurementsPerBb[core].end()) {
    return;
  } else {
    mments = it->second;
  }

  for(unsigned i = 1; i < mments->size(); i++) {
    Measurement m = (*mments)[i];

    (*runtime) += Pmu::cyclesToSeconds(m.timeSinceLast);

    for(unsigned sensor = 0; sensor < Pmu::MAX_SENSORS; sensor++) {
      energy[sensor] += m.power[sensor] * Pmu::cyclesToSeconds(m.timeSinceLast);
    }

    if(measurements) measurements->push_back(m);
  }
}

void Profile::setProfData(QVector<Measurement> *measurements) {
  for(unsigned core = 0; core < Pmu::MAX_CORES; core++) {
    measurementsPerBb[core].clear();
  }
  this->measurements.clear();
  for(auto measurement : *measurements) {
    addMeasurement(measurement);
  }
  delete measurements;

  if(this->measurements.size()) {
    runtime = Pmu::cyclesToSeconds(this->measurements.last().time - this->measurements[0].time);
  }

  if(this->measurements.size()) {
    for(unsigned sensor = 0; sensor < Pmu::MAX_SENSORS; sensor++) {
      energy[sensor] = 0;

      for(int i = 1; i < this->measurements.size(); i++) {
        if(this->measurements[i].core == 0) {
          energy[sensor] += this->measurements[i].power[sensor] * Pmu::cyclesToSeconds(this->measurements[i].timeSinceLast);
        }
      }
    }
  }
}

void Profile::clear() {
  for(unsigned core = 0; core < Pmu::MAX_CORES; core++) {
    for(auto const &it : measurementsPerBb[core]) {
      delete it.second;
    }
  }
}
