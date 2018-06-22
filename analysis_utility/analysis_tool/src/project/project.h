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

#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QVector>
#include <QDir>
#include <QDomDocument>

#include "elfsupport.h"
#include "projectacc.h"
#include "config/config.h"
#include "profile/measurement.h"
#include "cfg/cfgmodel.h"
#include "pmu.h"

#define SAMPLEBUF_SIZE (128*1024*1024)

class CfgModel;

//////////////////////////////////////////////////////////////////////////////

class Project : public QObject {
  Q_OBJECT

protected:
  void writeTulippCompileRule(QString compiler, QFile &makefile, QString path, QString opt);
  void writeLinkRule(QString linker, QFile &makefile, QStringList objects);
  void writeCleanRule(QFile &makefile);

  bool createXmlMakefile();
  virtual bool createMakefile(QFile &makefile);
  virtual bool createMakefile() = 0;
  void copy(Project *p);

public:
  bool opened;
  bool isCpp;

  QString path;

  // user specified settings
  QStringList systemXmls;
  int cfgOptLevel;
  QString tcfUploadScript;
  Pmu pmu;
  bool ultrascale;
  QString startFunc;
  unsigned startCore;
  QString stopFunc;
  unsigned stopCore;
  bool createBbInfo;
  bool useCustomElf;

  // settings from either sdsoc project or user
  QStringList sources;
  QString name;
  QString cOptions;
  int cOptLevel;
  QString cppOptions;
  int cppOptLevel;
  QString linkerOptions;

  // settings from sdsoc project, unused otherwise
  QString configType;
  QVector<ProjectAcc> accelerators;
  QString cSysInc;
  QString cppSysInc;

  QString customElfFile;
  QString elfFile;

  CfgModel *cfgModel;

  int errorCode;

  Project();
  Project(Project *p);
  virtual ~Project();

  void close();
  void clear();

  void clean();

  virtual void print();
  int runSourceTool(QString inputFilename, QString outputFilename, QStringList loopsToPipeline);

  QVector<Measurement> *parseProfFile();
  QVector<Measurement> *parseProfFile(QFile &file);

  QString elfFilename() {
    return name + ".elf";
  }

  void loadFiles();
  void loadXmlFile(const QString &fileName);
  void loadProjectFile();
  void saveProjectFile();

  int xmlBuildSteps() { return 1; }
  int binBuildSteps() { return 2; }
  int profileSteps() { return 3; }

  virtual bool isSdSocProject() { return false; }

  Cfg *getCfg();

public slots:
  void makeXml();
  virtual void makeBin();
  void runProfiler();

signals:
  void advance(int step, QString msg);
  void finished(int ret, QString msg);
};

#endif
