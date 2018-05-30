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

#ifndef DSEALGORITHM_H
#define DSEALGORITHM_H

#include <QObject>

#include "dserun.h"

#define FITNESS_RUNTIME   0
#define FITNESS_ENERGY_0  1
#define FITNESS_ENERGY_1  2
#define FITNESS_ENERGY_2  3
#define FITNESS_ENERGY_3  4
#define FITNESS_ENERGY_4  5
#define FITNESS_ENERGY_5  6
#define FITNESS_ENERGY_6  7
#define FITNESS_BRAMS     8
#define FITNESS_LUTS      9
#define FITNESS_DSPS     10
#define FITNESS_REGS     11

class DseAlgorithm : public QObject {
  Q_OBJECT

protected:
  QVector<DseRun> *dseRuns;

  std::ostream *outStream;
  QVector<Loop*> loops;
  QVector<unsigned> loopDepths;
  unsigned fitnessChoice;
  Project *mainProject;
  bool rerunFailed;

  double fitnessFunction(DseRun *run);
  double testGenome(std::ostream &outStream, QVector<Loop*> loops, QVector<unsigned> genome);

public:
  static QMap<QString,QStringList> getFilesToTransform(QVector<Loop*> loops, QVector<unsigned> genome);

  DseAlgorithm(Project *mainProject, unsigned fitnessChoice, QVector<DseRun> *dseRuns, std::ostream *outStream,
               QVector<Loop*> loops, QVector<unsigned> loopDepths, bool rerunFailed) {
    this->mainProject = mainProject;
    this->fitnessChoice = fitnessChoice;
    this->dseRuns = dseRuns;
    this->outStream = outStream;
    this->loops = loops;
    this->loopDepths = loopDepths;
    this->rerunFailed = rerunFailed;
  }
  ~DseAlgorithm() {
  }

  static QString getFitnessText(unsigned x) {
    switch(x) {
      default:
      case FITNESS_RUNTIME:  return QString("Runtime");
      case FITNESS_ENERGY_0: return QString("Energy 0");
      case FITNESS_ENERGY_1: return QString("Energy 1");
      case FITNESS_ENERGY_2: return QString("Energy 2");
      case FITNESS_ENERGY_3: return QString("Energy 3");
      case FITNESS_ENERGY_4: return QString("Energy 4");
      case FITNESS_ENERGY_5: return QString("Energy 5");
      case FITNESS_ENERGY_6: return QString("Energy 6");
      case FITNESS_BRAMS:    return QString("BRAMs");
      case FITNESS_LUTS:     return QString("LUTs");
      case FITNESS_DSPS:     return QString("DSPs");
      case FITNESS_REGS:     return QString("Registers");
    }
  }

  static double getFitness(Profile *profile, Project *project, unsigned x) {
    switch(x) {
      default:
      case FITNESS_RUNTIME:  return profile->getRuntime();
      case FITNESS_ENERGY_0: return profile->getEnergy(0);
      case FITNESS_ENERGY_1: return profile->getEnergy(1);
      case FITNESS_ENERGY_2: return profile->getEnergy(2);
      case FITNESS_ENERGY_3: return profile->getEnergy(3);
      case FITNESS_ENERGY_4: return profile->getEnergy(4);
      case FITNESS_ENERGY_5: return profile->getEnergy(5);
      case FITNESS_ENERGY_6: return profile->getEnergy(6);
      case FITNESS_BRAMS:    return project->getBrams();
      case FITNESS_LUTS:     return project->getLuts();
      case FITNESS_DSPS:     return project->getDsps();
      case FITNESS_REGS:     return project->getRegs();
    }
  }

public slots:
  virtual void run() = 0;

signals:
  void advance(int num);
  void finished();
};

#endif
