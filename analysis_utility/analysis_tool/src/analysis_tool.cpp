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

#include <QApplication>

#include "mainwindow.h"
#include "analysis.h"
#include "cfg/loop.h"

///////////////////////////////////////////////////////////////////////////////

bool getProfData(Analysis &analysis,
                 QString moduleId, QString functionId, unsigned core, unsigned sensor,
                 double *runtime, double *energy, uint64_t *count) {
  Module *mod = analysis.project->cfg->getModuleById(moduleId);
  if(!mod) {
    printf("Can't find module %s\n", moduleId.toUtf8().constData());
    return false;
  }
  Function *func = mod->getFunctionById(functionId);
  if(!func) {
    printf("Can't find function %s\n", functionId.toUtf8().constData());
    return false;
  }

  double runtimeFrame;
  double energyAll[Pmu::sensors];
  double energyFrame[Pmu::sensors];

  func->getProfData(core, QVector<BasicBlock*>(), runtime, energyAll, &runtimeFrame, energyFrame, count);
  *energy = energyAll[sensor];

  return true;
}

int main(int argc, char *argv[]) {
  Q_INIT_RESOURCE(application);

  QApplication app(argc, argv);
  app.setOrganizationName(ORG_NAME);
  app.setOrganizationDomain(ORG_DOMAIN);
  app.setApplicationName(APP_NAME);
  app.setApplicationVersion(QString("V") + QString::number(VERSION));

  Analysis analysis;

  QCommandLineParser parser;
  parser.setApplicationDescription("Analysis Tool");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption runOption("run", QCoreApplication::translate("main", "Run Application"));
  parser.addOption(runOption);
  QCommandLineOption profileOption("profile", QCoreApplication::translate("main", "Profile Application"));
  parser.addOption(profileOption);
  QCommandLineOption exportOption("export", QCoreApplication::translate("main", "Export Measurements"));
  parser.addOption(exportOption);
  QCommandLineOption buildOption("build", QCoreApplication::translate("main", "Build Application"));
  parser.addOption(buildOption);
  QCommandLineOption cleanOption("clean", QCoreApplication::translate("main", "Clean Application"));
  parser.addOption(cleanOption);
  QCommandLineOption samplePcOption("sample-pc", QCoreApplication::translate("main", "Sample PC over JTAG"));
  parser.addOption(samplePcOption);
  QCommandLineOption noSamplePcOption("no-sample-pc", QCoreApplication::translate("main", "Do not sample PC over JTAG"));
  parser.addOption(noSamplePcOption);

  QCommandLineOption projectOption(QStringList() << "project",
                                   QCoreApplication::translate("main", "Open project"),
                                   QCoreApplication::translate("main", "project"));
  parser.addOption(projectOption);

  QCommandLineOption buildConfigOption(QStringList() << "build-config",
                                       QCoreApplication::translate("main", "Build config"),
                                       QCoreApplication::translate("main", "config"));
  parser.addOption(buildConfigOption);

  QCommandLineOption loadProfileOption(QStringList() << "load-profile",
                                       QCoreApplication::translate("main", "Load profile"),
                                       QCoreApplication::translate("main", "file"));
  parser.addOption(loadProfileOption);

  QCommandLineOption cflagsOption(QStringList() << "compile-flags",
                                  QCoreApplication::translate("main", "Extra compile flags"),
                                  QCoreApplication::translate("main", "flags"));
  parser.addOption(cflagsOption);

  QCommandLineOption getRuntimeOption(QStringList() << "get-runtime",
                                      QCoreApplication::translate("main", "Get runtime"),
                                      QCoreApplication::translate("main", "module,function,core"));
  parser.addOption(getRuntimeOption);

  QCommandLineOption getPowerOption(QStringList() << "get-power",
                                    QCoreApplication::translate("main", "Get power"),
                                    QCoreApplication::translate("main", "module,function,core,sensor"));
  parser.addOption(getPowerOption);

  QCommandLineOption getEnergyOption(QStringList() << "get-energy",
                                     QCoreApplication::translate("main", "Get energy"),
                                     QCoreApplication::translate("main", "module,function,core,sensor"));
  parser.addOption(getEnergyOption);

  QCommandLineOption getCountOption(QStringList() << "get-count",
                                    QCoreApplication::translate("main", "Get count"),
                                    QCoreApplication::translate("main", "module,function,core"));
  parser.addOption(getCountOption);

  QCommandLineOption getTotalEnergyOption(QStringList() << "get-total-energy",
                                     QCoreApplication::translate("main", "Get energy for entire run"),
                                     QCoreApplication::translate("main", "sensor"));
  parser.addOption(getTotalEnergyOption);

  QCommandLineOption projectDirOption(QStringList() << "project-dir",
                                         QCoreApplication::translate("main", "Project directory"),
                                         QCoreApplication::translate("main", "path"));
  parser.addOption(projectDirOption);

  QCommandLineOption periodOption(QStringList() << "period",
                                  QCoreApplication::translate("main", "Cycles to sample"),
                                  QCoreApplication::translate("main", "cycles"));
  parser.addOption(periodOption);

  QCommandLineOption dumpRoiOption(QStringList() << "dump-roi",
                                  QCoreApplication::translate("main", "Dump ROI data"),
                                  QCoreApplication::translate("main", "core,sensor"));
  parser.addOption(dumpRoiOption);

  parser.process(app);

  QSettings settings;
  QString project = settings.value("currentProject", "").toString();
  QString buildConfig = settings.value("currentBuildConfig", "").toString();

  if(parser.isSet(projectOption)) {
    project = parser.value(projectOption);
  }

  if(parser.isSet(buildConfigOption)) {
    buildConfig = parser.value(buildConfigOption);
  }

  if(parser.isSet(cflagsOption)) {
    Config::extraCompileOptions = parser.value(cflagsOption);
  }

  Config::overrideSamplePeriod = 0;
  if(parser.isSet(periodOption)) {
    Config::overrideSamplePeriod = parser.value(periodOption).toDouble();
  }

  Config::overrideSamplePc = false;
  Config::overrideSamplePc = parser.isSet(samplePcOption);

  Config::overrideNoSamplePc = false;
  Config::overrideNoSamplePc = parser.isSet(noSamplePcOption);

  if(parser.isSet(projectDirOption)) {
    Config::projectDir = parser.value(projectDirOption);
  } else {
    Config::projectDir = "";
  }

  bool batch =
    parser.isSet(getRuntimeOption) ||
    parser.isSet(getPowerOption) ||
    parser.isSet(getTotalEnergyOption) ||
    parser.isSet(getEnergyOption) ||
    parser.isSet(getCountOption) ||
    parser.isSet(cleanOption) || 
    parser.isSet(buildOption) || 
    parser.isSet(loadProfileOption) || 
    parser.isSet(runOption) || 
    parser.isSet(exportOption) || 
    parser.isSet(dumpRoiOption) || 
    parser.isSet(profileOption);

  bool compile =
    parser.isSet(cleanOption) || 
    parser.isSet(buildOption);

  if(batch) {
    if(!analysis.openProject(project, buildConfig, !compile)) {
      printf("Can't open project\n");
      return -1;
    }

    if(parser.isSet(cleanOption)) {
      printf("Cleaning application\n");
      if(!analysis.clean()) {
        printf("Can't clean project\n");
        return -1;
      }
    }

    if(parser.isSet(buildOption)) {
      printf("Building application\n");
      if(!analysis.project->makeBin()) {
        printf("Can't build project\n");
        return -1;
      }
    }

    if(parser.isSet(loadProfileOption)) {
      printf("Loading profile\n");
      if(!analysis.loadProfFile(parser.value(loadProfileOption))) {
        printf("Can't load profile\n");
        return -1;
      }
    }

    if(parser.isSet(runOption)) {
      printf("Running application\n");
      if(!analysis.runApp()) {
        printf("Can't run application\n");
        return -1;
      }
    } else if(parser.isSet(profileOption)) {
      printf("Profiling application\n");
      if(!analysis.profileApp()) {
        printf("Can't run profiler\n");
        return -1;
      }
    }

    if(parser.isSet(exportOption)) {
      if(analysis.profile) {
        printf("Exporting measurements to data.csv\n");
        if(!analysis.exportMeasurements("data.csv")) {
          printf("Can't export\n");
          return -1;
        }
      }
    }

    if(parser.isSet(dumpRoiOption)) {
      QStringList arg = parser.value(dumpRoiOption).split(',');
      unsigned core = arg[0].toUInt();
      unsigned sensor = arg[1].toUInt();
      analysis.dump(core, sensor);
    }

    if(parser.isSet(getRuntimeOption)) {
      double runtime, energy;
      uint64_t count;
      QStringList arg = parser.value(getRuntimeOption).split(',');
      if(!getProfData(analysis, arg[0], arg[1], arg[2].toUInt(), 0, &runtime, &energy, &count)) return -1;
      printf("%f\n", runtime);
    }

    if(parser.isSet(getPowerOption)) {
      double runtime, energy;
      uint64_t count;
      QStringList arg = parser.value(getRuntimeOption).split(',');
      if(!getProfData(analysis, arg[0], arg[1], arg[2].toUInt(), 0, &runtime, &energy, &count)) return -1;
      printf("%f\n", energy / runtime);
    }

    if(parser.isSet(getTotalEnergyOption)) {
      unsigned sensor = parser.value(getTotalEnergyOption).toUInt();
      printf("%f\n", analysis.profile->getEnergy(sensor));
    }

    if(parser.isSet(getEnergyOption)) {
      double runtime, energy;
      uint64_t count;
      QStringList arg = parser.value(getRuntimeOption).split(',');
      if(!getProfData(analysis, arg[0], arg[1], arg[2].toUInt(), 0, &runtime, &energy, &count)) return -1;
      printf("%f\n", energy);
    }

    if(parser.isSet(getCountOption)) {
      double runtime, energy;
      uint64_t count;
      QStringList arg = parser.value(getRuntimeOption).split(',');
      if(!getProfData(analysis, arg[0], arg[1], arg[2].toUInt(), 0, &runtime, &energy, &count)) return -1;
      printf("%ld\n", count);
    }

  } else {
    MainWindow *mainWin = new MainWindow(&analysis);

    // open project
    if(project != "") {
      mainWin->openProject(project, buildConfig);
    }

    mainWin->show();

    return app.exec();
  }
}

///////////////////////////////////////////////////////////////////////////////

QVariant makeQVariant(QVector<BasicBlock*> callStack) {
  // TODO: is this the only way to convert between callStack and QVariant?
  QVector<void*> cS;

  for(auto bb : callStack) {
    cS.push_back((void*)bb);
  }

  return QVariant::fromValue(cS);
}

QVector<BasicBlock*> fromQVariant(QVariant variant) {
  QVector<BasicBlock*> callStack;
  QVector<void*> cS = variant.value<QVector<void*> >();

  for(auto bb : cS) {
    callStack.push_back((BasicBlock*)bb);
  }

  return callStack;
}

void indent(unsigned in) {
  for(unsigned i = 0; i < in; i++) {
    printf(" ");
  }
}

QString xmlPurify(QString text) {
  text.replace("&amp;", "&");
  text.replace("&lt;", "<");
  text.replace("&gt;", ">");
  text.replace("&quot;", "\"");
  text.replace("&apos;", "\'");
  return text;
}

QString getLine(QString filename, unsigned lineNumber, unsigned column) {
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly)) {
    QString line;

    for(unsigned i = 0; i < lineNumber; i++) {
      line = file.readLine();
    }

    return line.mid(column-1);

  } else {
    return "";
  }
}

QVector<Loop*> getLoop(Loop *loop, unsigned depth) {
  QVector<Loop*> result;

  if(depth == 1) {
    result.push_back(loop);
  } else {
    QVector<Loop*> childLoops;
    loop->getAllLoops(childLoops, QVector<BasicBlock*>());

    for(auto l : childLoops) {
      result.append(getLoop(l, depth-1));
    }
  }

  return result;
}

void addFitnessCombo(QComboBox *combo) {
  combo->addItem("Runtime");
  combo->addItem("Energy 0");
  combo->addItem("Energy 1");
  combo->addItem("Energy 2");
  combo->addItem("Energy 3");
  combo->addItem("Energy 4");
  combo->addItem("Energy 5");
  combo->addItem("Energy 6");
  combo->addItem("BRAMs");
  combo->addItem("LUTs");
  combo->addItem("DSPs");
  combo->addItem("Registers");
}

bool isSystemFile(QString filename) {
  if(Config::sdsocProject) {
    return !filename.startsWith(Config::workspace);
  } else {
    return false;
  }
}
