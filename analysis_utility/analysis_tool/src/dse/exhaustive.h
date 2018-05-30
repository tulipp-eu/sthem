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

#ifndef EXHAUSTIVE_H
#define EXHAUSTIVE_H

#include <QObject>

#include "dsealgorithm.h"

class Exhaustive : public DseAlgorithm {

private:
  int runCounter;

  void runAll(std::ostream &outStream, QVector<Loop*> loops, QVector<unsigned> loopDepths, int n, QVector<unsigned> genome);

public:
  Exhaustive(Project *mainProject, unsigned fitnessChoice, QVector<DseRun> *dseRuns, std::ostream *outStream,
             QVector<Loop*> loops, QVector<unsigned> loopDepths, bool rerunFailed) :
    DseAlgorithm(mainProject, fitnessChoice, dseRuns, outStream, loops, loopDepths, rerunFailed) {}
  ~Exhaustive() {
  }

public slots:
  void run();
};

#endif
