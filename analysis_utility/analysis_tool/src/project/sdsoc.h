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

#ifndef SDSOC_H
#define SDSOC_H

#include <QDirIterator>
#include <QMessageBox>
#include <QTextStream>

#include "project.h"

class Sdsoc : public Project {

protected:
  QString platformKey;

  bool timingOk;
  double brams;
  double luts;
  double dsps;
  double regs;

  QString sdsocExpand(QString text);
  QString processCompilerOptions(QDomElement &childElement, int *optLevel);
  QString processLinkerOptions(QDomElement &childElement);
  QString processIncludePaths(QString filename);
  virtual void parseSynthesisReport() = 0;
  virtual bool getPlatformOptions();
  virtual bool getProjectOptions() = 0;
  virtual void writeSdsRule(QString compiler, QFile &makefile, QString path, QString opt);
  virtual void writeSdsLinkRule(QString linker, QFile &makefile, QStringList objects, QString opt = "");

public:
  QString platform;
  QString os;
  bool insertapm;
  bool genbitstream;
  bool gensdcard;
  unsigned dmclkid;
  bool enableHwSwTrace;
  bool traceApplication;

  void copy(Sdsoc *p);

  Sdsoc() {}
  Sdsoc(Sdsoc *p);

  bool openProject(QString path, QString configType, bool fast = false);

  bool makeBin() {
    bool ret = Project::makeBin();
    if(ret) parseSynthesisReport();
    return ret;
  }

  bool getTimingOk() const { return timingOk; }
  double getBrams() const { return brams; }
  double getLuts() const { return luts; }
  double getDsps() const { return dsps; }
  double getRegs() const { return regs; }

  virtual bool createMakefile();

  virtual bool isSdSocProject() { return true; }

  virtual void print();

  static QStringList getBuildConfigurations(QString path);

  static unsigned getSdsocVersion();

  static Sdsoc *createSdsoc(unsigned version);

	friend std::ostream& operator<<(std::ostream &os, const Sdsoc &p) {
    // only streams out the necessary parts for DSE
		os << p.getTimingOk() << '\n';
		os << p.getBrams() << '\n';
		os << p.getLuts() << '\n';
		os << p.getDsps() << '\n';
		os << p.getRegs();
		return os;
	}

	friend std::istream& operator>>(std::istream &is, Sdsoc &p) {
		is >> p.timingOk >> p.brams >> p.luts >> p.dsps >> p.regs;
		return is;
	}
};

///////////////////////////////////////////////////////////////////////////////

class Sdsoc20162 : public Sdsoc {

protected:
  virtual bool getProjectOptions();
  virtual void parseSynthesisReport();

public:
  Sdsoc20162() {
    platformKey = "sdsoc.platform";
  }
  Sdsoc20162(Sdsoc *p) {
    Sdsoc::copy(p);
  }

};

//-----------------------------------------------------------------------------

class Sdsoc20172 : public Sdsoc {

protected:
  QString sysConfig;
  QString cpu;

  virtual bool getProjectOptions();
  virtual void parseSynthesisReport();
  virtual void writeSdsRule(QString compiler, QFile &makefile, QString path, QString opt);
  virtual void writeSdsLinkRule(QString linker, QFile &makefile, QStringList objects, QString opt = "");

public:
  Sdsoc20172() {
    platformKey = "sdx.custom.platform.repository.locations";
  }
  Sdsoc20172(Sdsoc *p) {
    Sdsoc::copy(p);
  }

  virtual void print();
};

//-----------------------------------------------------------------------------

class Sdsoc20174 : public Sdsoc {

protected:
  QString sysConfig;
  QString cpu;
  QString cpuInstance;
  bool genEmulationModel;

  virtual bool getProjectOptions();
  virtual void parseSynthesisReport();
  virtual void writeSdsRule(QString compiler, QFile &makefile, QString path, QString opt);
  virtual void writeSdsLinkRule(QString linker, QFile &makefile, QStringList objects, QString opt = "");

public:
  Sdsoc20174() {
    platformKey = "sdx.custom.platform.repository.locations";
  }
  Sdsoc20174(Sdsoc *p) {
    Sdsoc::copy(p);
  }

  virtual void print();
};

//-----------------------------------------------------------------------------

class Sdsoc20182 : public Sdsoc20174 {

public:
  Sdsoc20182() {}
};

#endif
