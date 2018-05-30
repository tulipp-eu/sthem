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

#ifndef PROFLINE_H
#define PROFLINE_H

#include "measurement.h"

class Vertex;

class ProfLine {

  static bool lessThan(const Measurement &e1, const Measurement &e2) {
    return e1.time < e2.time;
  }

public:
  Vertex *vertex;
  double runtime;
  double power[LYNSYN_SENSORS];
  double energy[LYNSYN_SENSORS];
  QVector<Measurement> measurements;

  ProfLine() {}

  void init(Vertex *vertex, double runtime, double power[], double energy[]) {
    this->vertex = vertex;
    this->runtime = runtime;
    for(unsigned i = 0; i < LYNSYN_SENSORS; i++) {
      this->power[i] = power[i];
      this->energy[i]= energy[i];
    }
  }

  QVector<Measurement> *getMeasurements() {
    qSort(measurements.begin(), measurements.end(), lessThan);
    return &measurements;
  }

};

#endif
