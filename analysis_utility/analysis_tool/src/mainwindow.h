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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <map>
#include <fstream>

#include <QtWidgets>
#include <QTreeView>
#include <QMainWindow>

#include "analysis_tool.h"

#include "analysis.h"
#include "project/project.h"
#include "project/customproject.h"
#include "project/sdsoc.h"
#include "cfg/cfgscene.h"
#include "profile/graphscene.h"
#include "cfg/vertex.h"
#include "profile/profmodel.h"
#include "cfg/cfgview.h"
#include "profile/graphview.h"
#include "textview.h"
#include "dse/dse.h"
#include "cfg/group.h"
#include "cfg/cfgmodel.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

private:
  Analysis *analysis;

  QTreeView *treeView;
  QTreeView *analysisView;
  TextView *textView;
  CfgScene *cfgScene;
  CfgView *cfgView;
  GraphScene *graphScene;
  GraphView *graphView;
  QComboBox *colorBox;
  QComboBox *windowBox;
  QComboBox *coreBox;
  QComboBox *sensorBox;
  QComboBox *cfgModeBox;
  ProfModel *profModel;
  CfgModel *cfgModel;
  QTableView *tableView;
  QSplitter *cfgSplitter;
  QSplitter *cfgSplitter2;
  QTabWidget *tabWidget;
  Group *hwGroup;
  Group *topGroup;
  Loop *frameLoop;
  QMenu *fileMenu;
  QMenu *projectMenu;
  QMenu *viewMenu;
  QMenu *visualizationMenu;
  QMenu *helpMenu;
  QToolBar *mainToolBar;
  QToolBar *cfgToolBar;
  QToolBar *graphToolBar;
  QToolBar *projectToolBar;
  QAction *exitAct;
  QAction *aboutAct;
  QAction *clearColorsAct;
  QAction *topAct;
  QAction *frameAct;
  QAction *hwAct;
  QAction *configDialogAct;
  QAction *projectDialogAct;
  QAction *openProjectAct;
  QAction *openProfileAct;
  QAction *openGProfAct;
  QAction *createProjectAct;
  QAction *closeProjectAct;
  QAction *cleanAct;
  QAction *cleanBinAct;
  QAction *makeXmlAct;
  QAction *makeBinAct;
  QAction *profileAct;
  QAction *runAct;
  QAction *showProfileAct;
  QAction *showFrameAct;
  QAction *showDseAct;

  QThread thread;
  QProgressDialog *progDialog;

  void loadFiles();
  void buildProjectMenu();
  void clearGui();

private slots:
  void about();
  void regionClicked(const QModelIndex &index);
  void analysisClicked(const QModelIndex &index);
  void clearColorsEvent();
  void topEvent();
  void frameEvent();
  void hwEvent();
  void configDialog();
  void projectDialog();
  void openProjectEvent(QAction *action);
  void openProfileEvent();
  void openGProfEvent();
  void openCustomProject();
  void createProject();
  void closeProject();
  void cleanEvent();
  void cleanBinEvent();
  void makeXmlEvent();
  void makeBinEvent();
  void profileEvent();
  void runEvent();
  void showFrameSummary();
  void showProfileSummary();
  void showDseSummary();
  void finishXml(int error, QString msg);
  void finishBin(int error, QString msg);
  void finishProfile(int error, QString msg);
  void finishRun(int error, QString msg);
  void advance(int step, QString msg);
  void changeCore(int core);
  void changeSensor(int sensor);
  void changeWindow(int window);
  void changeCfgMode(int mode);
  void tabChanged(int tab);

protected:
  void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

public:
  MainWindow(Analysis *analysis);
  ~MainWindow();

  bool openProject(QString path, QString buildConfig);

  void resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    if(topGroup) {
      textView->loadFile(topGroup->getSourceFilename(), topGroup->getSourceLineNumber());
      cfgView->verticalScrollBar()->setValue(0);
      cfgView->horizontalScrollBar()->setValue(0);
    }
  }
};

#endif
