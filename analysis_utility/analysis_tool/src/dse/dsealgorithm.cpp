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

#include <QElapsedTimer>

#include "dsealgorithm.h"
#include "cfg/loop.h"
#include "unistd.h"

double DseAlgorithm::fitnessFunction(DseRun *run) {
  if(!run->failed) {
    if(run->project->getTimingOk()) {
      return getFitness(run->profile, run->project, fitnessChoice);
    }
  }
  return INT_MAX;
}

QMap<QString,QStringList> DseAlgorithm::getFilesToTransform(QVector<Loop*> loops, QVector<unsigned> genome) {
  QMap<QString,QStringList> hwFiles;

  for(int i = 0; i < loops.size(); i++) {
    for(auto l : getLoop(loops[i], genome[i])) {
      if(!hwFiles.contains(l->sourceFilename)) {
        hwFiles[l->sourceFilename] = QStringList();
      }
      QStringList list = hwFiles[l->sourceFilename];
      list.push_back(QString::number(l->sourceLineNumber) + "," + QString::number(l->sourceColumn));
      list.removeDuplicates();
      hwFiles[l->sourceFilename] = list;
    }
  }

  return hwFiles;
}

double DseAlgorithm::testGenome(std::ostream &outStream, QVector<Loop*> loops, QVector<unsigned> genome) {
  double fitness = INT_MAX;

  // see if it is already evaluated
  for(auto r : *dseRuns) {
    if(r.genome == genome) {
      if(!(r.failed && rerunFailed)) {
        return fitnessFunction(&r);
      }
    }
  }

  QElapsedTimer timer;
  timer.start();

  // create run
  Profile *profile = new Profile;
  Sdsoc *project = Sdsoc::copySdsoc(mainProject, profile);

  // clear and set temp directory
  QDir oldDir = QDir::current();
  QDir dir("dse");
  if(dir.exists()) dir.removeRecursively();
  dir.mkpath(".");
  QDir::setCurrent(dir.path());

  profile->connect();

  DseRun dseRun(genome, project, profile);

  // transform source files
  QMap<QString,QStringList> hwFiles = getFilesToTransform(loops, genome);

  for(auto f : hwFiles.toStdMap()) {
    QFileInfo fileInfo(f.first);

    int idx = dseRun.project->sources.indexOf(f.first);
    if(idx >= 0) {
      dseRun.project->sources[idx] = fileInfo.fileName();
    }

    for(int i = 0; i < dseRun.project->accelerators.size(); i++) {
      if(dseRun.project->accelerators[i].filepath == f.first) {
        dseRun.project->accelerators[i].filepath = fileInfo.fileName();
      }
    }

    QString opt;
    if(fileInfo.suffix() == "c") {
      opt = dseRun.project->cOptions + " " + dseRun.project->cSysInc;
    } else if((fileInfo.suffix() == "cpp") || (fileInfo.suffix() == "cc")) {
      opt = dseRun.project->cppOptions + " " + dseRun.project->cppSysInc;
    }

    dseRun.project->runSourceTool(f.first, fileInfo.fileName(), f.second, opt);
  }

  // build
  dseRun.project->makeBin();

  if(dseRun.project->errorCode) {
    delete dseRun.project;
    delete dseRun.profile;
    dseRun.failed = true;
    dseRun.time = (timer.elapsed() / (double)1000);

  } else {
    // profile
    dseRun.project->runProfiler();

    if(dseRun.project->errorCode) {
      delete dseRun.project;
      delete dseRun.profile;
      dseRun.failed = true;
      dseRun.time = (timer.elapsed() / (double)1000);

    } else {
      fitness = fitnessFunction(&dseRun);
      dseRun.time = (timer.elapsed() / (double)1000);
      dseRun.project->clear();
      dseRun.profile->clean();
    }
  }

  dseRuns->push_back(dseRun);
  outStream << true << '\n';
  outStream << dseRun << '\n';
  outStream.flush();

  // change back to standard dir
  QDir::setCurrent(oldDir.path());

  return fitness;
}

