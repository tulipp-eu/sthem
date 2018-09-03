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

  Analysis analysis;
  MainWindow *mainWin;

  mainWin = new MainWindow(&analysis);
  mainWin->show();

  return app.exec();
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
