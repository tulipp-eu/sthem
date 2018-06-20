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

#ifndef DSE_H
#define DSE_H

#include "cfg/container.h"
#include "dsedialog.h"
#include "project/project.h"
#include "dserun.h"
#include "dsealgorithm.h"

class Dse : public QObject {
  Q_OBJECT

private:
  QVector<DseRun> dseRuns;
  Sdsoc *mainProject;
  unsigned fitnessChoice;
  unsigned algorithm;
  QString containerId;
  QString moduleId;
  QVector<Loop*> loops;
  Cfg *cfg;
  QProgressDialog *progDialog;
  QThread thread;
  std::ofstream *outputFile; 
  DseAlgorithm *dseAlgorithm;

  unsigned loopDepth(Loop *loop);
  double fitnessFunction(DseRun *run);
  double testGenome(std::ostream &outStream, QVector<Loop*> loops, QVector<unsigned> genome);
  void runAll(std::ostream &outStream, QVector<Loop*> loops, QVector<unsigned> loopDepths, int n, QVector<unsigned> genome);
  void exhaustive(std::ostream &outStream, QVector<Loop*> loops, QVector<unsigned> loopDepths);
  void ga(std::ostream &outStream, QVector<Loop*> loops, QVector<unsigned> loopDepths);

public:
  Dse(Sdsoc *project) {
    mainProject = project;
    progDialog = NULL;
    dseAlgorithm = NULL;
  }
  
  ~Dse() {
    if(progDialog) delete progDialog;
    clear();
  }

  void setCfg(Cfg *cfg) {
    this->cfg = cfg;
  }

  void clear() {
    for(auto r : dseRuns) {
      if(!r.failed) {
        delete r.project;
        delete r.profile;
      }
    }
    dseRuns.clear();
  }

  void dialog(Container *cont);
  void showResults();

	friend std::ostream& operator<<(std::ostream &os, const Dse &d) {
    os << d.moduleId.toUtf8().constData() << '\n';
    os << d.containerId.toUtf8().constData() << '\n';
    os << d.algorithm << '\n';
    os << d.fitnessChoice;

    if(d.dseRuns.size()) os << '\n';

    for(int i = 0; i < d.dseRuns.size(); i++) {
      auto dseRun = d.dseRuns[i];
      os << true << '\n';
      os << dseRun;
      if(i < d.dseRuns.size()-1) os << '\n';
    }

		return os;
	}

	friend std::istream& operator>>(std::istream &is, Dse &d) {
    std::string id;
    std::getline(is, id);
    d.moduleId = QString::fromUtf8(id.c_str());
    std::getline(is, id);
    d.containerId = QString::fromUtf8(id.c_str());
    is >> d.algorithm;
    is >> d.fitnessChoice;

    bool hasMore = false;
    do {
      is >> hasMore;
      if(is.good()) {
        DseRun run;
        is >> run;

        // remove all previous failed attempts at the same genome
        for(auto r = d.dseRuns.begin(); r != d.dseRuns.end();) {
          if(((*r).genome == run.genome) && (*r).failed) {
            r = d.dseRuns.erase(r);
          } else {
            r++;
          }
        }

        d.dseRuns.push_back(run);
      }
    } while(hasMore && is.good());

		return is;
	}

private slots:
  void finished();

};

#endif
