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

#include <libusb.h>

#include "config/config.h"
#include "profile/measurement.h"
#include "cfg/cfgmodel.h"

class Addr2Line {
public:
  QString filename;
  QString function;
  uint64_t lineNumber;

  Addr2Line() {
    filename = "";
    function = "";
    lineNumber = 0;
  }
  Addr2Line(QString file, QString func, uint64_t l) {
    filename = file;
    function = func;
    lineNumber = l;
  }
  bool isBb() {
    return filename[0] == '@';
  }
  QString getModuleId() {
    return filename.right(filename.size()-1);
  }
};

class ProjectAcc {
public:
  QString name;
  QString filepath;
  unsigned clkid;

  void print() {
    printf("  %s %s %d\n", name.toUtf8().constData(), filepath.toUtf8().constData(), clkid);
  }
};

class CfgModel;

//////////////////////////////////////////////////////////////////////////////

class Project : public QObject {
  Q_OBJECT

private:
  std::map<uint64_t, Addr2Line> addr2lineCache;

	struct libusb_device_handle *lynsynHandle;
  uint8_t outEndpoint;
  uint8_t inEndpoint;
	struct libusb_context *usbContext;
	libusb_device **devs;
  uint8_t hwVersion;
  double sensorCalibration[LYNSYN_SENSORS];

  bool initLynsyn();
  void releaseLynsyn();
  void sendBytes(uint8_t *bytes, int numBytes);
  void getBytes(uint8_t *bytes, int numBytes);
  QString sdsocExpand(QString text);
  QString processCompilerOptions(QDomElement &childElement, int *optLevel);
  QString processLinkerOptions(QDomElement &childElement);
  QString processIncludePaths(QString filename);
  void writeSdsRule(QString compiler, QFile &makefile, QString path, QString opt);
  void writeClangRule(QString compiler, QFile &makefile, QString path, QString opt);
  void writeCompileRule(QString compiler, QFile &makefile, QString path, QString opt);
  void writeSdsLinkRule(QString linker, QFile &makefile, QStringList objects);
  void writeLinkRule(QString linker, QFile &makefile, QStringList objects);
  Addr2Line addr2line(QString elfFile, uint64_t pc);
  Addr2Line cachedAddr2line(QString elfFile, uint64_t pc);
  void parseSynthesisReport();
  bool createXmlMakefile();
  bool createMakefile();

public:
  bool opened;
  bool isCpp;
  bool isSdSocProject;

  QString path;

  // user specified settings
  QStringList systemXmls;
  QStringList systemBins;
  int cfgOptLevel;
  QStringList systemIncludes;
  QString tcfUploadScript;
  double rl[7];
  double supplyVoltage[7];
  unsigned cores;
  bool ultrascale;
  QString startFunc;
  unsigned startCore;
  QString stopFunc;
  unsigned stopCore;
  bool createBbInfo;
  bool useCustomElf;

  // settings from sdsoc project
  QString platform;
  QString os;
  QString configType;
  bool insertapm;
  bool genbitstream;
  bool gensdcard;
  unsigned dmclkid;
  bool enableHwSwTrace;
  bool traceApplication;
  QVector<ProjectAcc> accelerators;

  // settings from either sdsoc project or user
  QStringList sources;
  QString name;
  QString cOptions;
  int cOptLevel;
  QString cppOptions;
  int cppOptLevel;
  QString linkerOptions;

  bool timingOk;
  double brams;
  double luts;
  double dsps;
  double regs;

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

  bool openProject(QString path);
  bool createProject(QString path);
  bool readSdSocProject(QString path, QString configType);

  void print();
  int runSourceTool(QString inputFilename, QString outputFilename, QStringList loopsToPipeline);

  QVector<Measurement> *parseProfFile();
  QVector<Measurement> *parseProfFile(QFile &file);

  bool getTimingOk() { return timingOk; }
  double getBrams() { return brams; }
  double getLuts() { return luts; }
  double getDsps() { return dsps; }
  double getRegs() { return regs; }

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

  static QStringList getBuildConfigurations(QString path);

  static uint64_t lookupSymbol(QString elfFile, QString symbol);

  Cfg *getCfg();

	friend std::ostream& operator<<(std::ostream &os, const Project &p) {
    // only streams out the necessary parts for DSE
		os << p.timingOk << '\n';
		os << p.brams << '\n';
		os << p.luts << '\n';
		os << p.dsps << '\n';
		os << p.regs;
		return os;
	}

	friend std::istream& operator>>(std::istream &is, Project &p) {
		is >> p.timingOk >> p.brams >> p.luts >> p.dsps >> p.regs;
		return is;
	}

public slots:
  void makeXml();
  void makeBin();
  void runProfiler();

signals:
  void advance(int step, QString msg);
  void finished(int ret, QString msg);
};

#endif
