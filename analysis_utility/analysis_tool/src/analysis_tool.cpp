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

int main(int argc, char *argv[]) {
  Q_INIT_RESOURCE(application);

  QApplication app(argc, argv);
  app.setOrganizationName(ORG_NAME);
  app.setOrganizationDomain(ORG_DOMAIN);
  app.setApplicationName(APP_NAME);

  QCommandLineParser parser;
  parser.setApplicationDescription("Analysis Tool");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption batchOption("batch", QCoreApplication::translate("main", "Batch mode"));
  parser.addOption(batchOption);
  QCommandLineOption runOption("run", QCoreApplication::translate("main", "Run Application"));
  parser.addOption(runOption);
  QCommandLineOption profileOption("profile", QCoreApplication::translate("main", "Profile Application"));
  parser.addOption(profileOption);
  QCommandLineOption buildOption("build", QCoreApplication::translate("main", "Build Application"));
  parser.addOption(buildOption);
  QCommandLineOption cleanOption("clean", QCoreApplication::translate("main", "Clean Application"));
  parser.addOption(cleanOption);

  QCommandLineOption projectOption(QStringList() << "project",
                                   QCoreApplication::translate("main", "Open project"),
                                   QCoreApplication::translate("main", "project"));
  parser.addOption(projectOption);

  QCommandLineOption buildConfigOption(QStringList() << "build-option",
                                   QCoreApplication::translate("main", "Build Option"),
                                   QCoreApplication::translate("main", "build-option"));
  parser.addOption(buildConfigOption);

  QCommandLineOption loadProfileOption(QStringList() << "load-profile",
                                   QCoreApplication::translate("main", "Load profile"),
                                   QCoreApplication::translate("main", "file"));
  parser.addOption(loadProfileOption);

  parser.process(app);

  QSettings settings;
  QString project = settings.value("currentProject", "").toString();
  QString buildConfig = settings.value("currentBuildConfig", "").toString();

  Analysis analysis;

  if(parser.isSet(projectOption)) {
    project = parser.value(projectOption);
  }

  if(parser.isSet(buildConfigOption)) {
    buildConfig = parser.value(buildConfigOption);
  }

  if(parser.isSet(batchOption)) {
    if(!analysis.openProject(project, buildConfig)) {
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
      if(!analysis.project->runApp()) {
        printf("Can't run application\n");
        return -1;
      }
    } else if(parser.isSet(profileOption)) {
      printf("Profiling application\n");
      if(!analysis.project->runProfiler()) {
        printf("Can't run profiler\n");
      }
    }

  } else {
    MainWindow *mainWin = new MainWindow(&analysis);
    mainWin->show();

    // open project
    if(project != "") {
      mainWin->openProject(project, buildConfig);
    }

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
  return !filename.startsWith(Config::workspace);
}
