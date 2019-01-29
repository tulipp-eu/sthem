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

#include <QDomDocument>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <QtSql>

#include "config/config.h"
#include "mainwindow.h"
#include "cfg/cfgview.h"
#include "cfg/basicblock.h"
#include "cfg/loop.h"
#include "analysis/analysismodel.h"
#include "config/configdialog.h"
#include "project/projectdialog.h"
#include "project/pmu.h"

QColor edgeColors[] = EDGE_COLORS;
QString colorNames[] = COLOR_NAMES;

///////////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow(Analysis *analysis) {
  this->analysis = analysis;

  progDialog = NULL;

  hwGroup = NULL;
  topGroup = NULL;
  frameLoop = NULL;
  profModel = NULL;
  cfgModel = NULL;

  treeView = new QTreeView();

  analysisView = new QTreeView();

  tableView = new QTableView();
  tableView->setSortingEnabled(true);

  connect(analysisView, SIGNAL(activated(QModelIndex)), this, SLOT(analysisClicked(QModelIndex)));
  connect(treeView, SIGNAL(activated(QModelIndex)), this, SLOT(regionClicked(QModelIndex)));

  colorBox = new QComboBox();
  for(unsigned i = 0; i < sizeof(edgeColors)/sizeof(edgeColors[0]); i++) {
    colorBox->addItem(colorNames[i]);
  }

  windowBox = new QComboBox();
  windowBox->addItem("Filter windowsize 1", 1);
  windowBox->addItem("Filter windowsize 5", 5);
  windowBox->addItem("Filter windowsize 10", 10);
  windowBox->addItem("Filter windowsize 50", 50);
  windowBox->addItem("Filter windowsize 100", 100);
  windowBox->addItem("Filter windowsize 500", 500);
  windowBox->addItem("Filter windowsize 1000", 1000);
  windowBox->addItem("Filter windowsize 5000", 5000);
  windowBox->addItem("Filter windowsize 10000", 10000);
  connect(windowBox, SIGNAL(activated(int)), this, SLOT(changeWindow(int)));

  sensorBox = new QComboBox();
  for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
    sensorBox->addItem(QString("Sensor ") + QString::number(i+1));
  }
  connect(sensorBox, SIGNAL(activated(int)), this, SLOT(changeSensor(int)));

  cfgModeBox = new QComboBox();
  cfgModeBox->addItem("Structural");
  cfgModeBox->addItem("Runtime");
  cfgModeBox->addItem("Power");
  cfgModeBox->addItem("Energy");
  cfgModeBox->addItem("Runtime Frame");
  cfgModeBox->addItem("Power Frame");
  cfgModeBox->addItem("Energy Frame");
  connect(cfgModeBox, SIGNAL(activated(int)), this, SLOT(changeCfgMode(int)));

  coreBox = new QComboBox();
  connect(coreBox, SIGNAL(activated(int)), this, SLOT(changeCore(int)));

  textView = new TextView(this);

  cfgScene = new CfgScene(colorBox, textView, this);
  cfgScene->setSceneRect(QRectF(0, 0, 1024, 512));
  cfgView = new CfgView(analysisView, cfgScene);
  
  cfgSplitter2 = new QSplitter;
  cfgSplitter2->setOrientation(Qt::Vertical);
  cfgSplitter2->addWidget(textView);
  cfgSplitter2->addWidget(analysisView);

  cfgSplitter = new QSplitter;
  cfgSplitter->addWidget(treeView);
  cfgSplitter->addWidget(cfgView);
  cfgSplitter->addWidget(cfgSplitter2);

  graphScene = new GraphScene(this);
  graphView = new GraphView(graphScene);

  tabWidget = new QTabWidget;
  tabWidget->addTab(cfgSplitter, "CFG");
  tabWidget->addTab(graphView, "Profile Graph");
  tabWidget->addTab(tableView, "Profile Table");
  connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

  setCentralWidget(tabWidget);

  // actions
  refreshAct = new QAction("Refresh", this);
  connect(refreshAct, SIGNAL(triggered()), this, SLOT(refreshEvent()));

  cleanAct = new QAction("Clean", this);
  connect(cleanAct, SIGNAL(triggered()), this, SLOT(cleanEvent()));

  cleanBinAct = new QAction("Clean Bin", this);
  connect(cleanBinAct, SIGNAL(triggered()), this, SLOT(cleanBinEvent()));

  makeXmlAct = new QAction("Make CFG", this);
  connect(makeXmlAct, SIGNAL(triggered()), this, SLOT(makeXmlEvent()));

  makeBinAct = new QAction("Make bin", this);
  connect(makeBinAct, SIGNAL(triggered()), this, SLOT(makeBinEvent()));

  makeAct = new QAction("Make", this);
  connect(makeAct, SIGNAL(triggered()), this, SLOT(makeEvent()));

  cmakeAct = new QAction("CMake", this);
  connect(cmakeAct, SIGNAL(triggered()), this, SLOT(cmakeEvent()));

  runAct = new QAction("Run", this);
  connect(runAct, SIGNAL(triggered()), this, SLOT(runEvent()));

  profileAct = new QAction("Profile", this);
  connect(profileAct, SIGNAL(triggered()), this, SLOT(profileEvent()));

  showProfileAct = new QAction("Summary", this);
  connect(showProfileAct, SIGNAL(triggered()), this, SLOT(showProfileSummary()));

  showFrameAct = new QAction("Summary(Frame)", this);
  connect(showFrameAct, SIGNAL(triggered()), this, SLOT(showFrameSummary()));

  showDseAct = new QAction("Summary(DSE)", this);
  connect(showDseAct, SIGNAL(triggered()), this, SLOT(showDseSummary()));

  clearColorsAct = new QAction("Clear", this);
  connect(clearColorsAct, SIGNAL(triggered()), this, SLOT(clearColorsEvent()));

  topAct = new QAction("Top", this);
  connect(topAct, SIGNAL(triggered()), this, SLOT(topEvent()));

  frameAct = new QAction("Frame", this);
  connect(frameAct, SIGNAL(triggered()), this, SLOT(frameEvent()));

  hwAct = new QAction("HW", this);
  connect(hwAct, SIGNAL(triggered()), this, SLOT(hwEvent()));

  exitAct = new QAction("Exit", this);
  exitAct->setShortcuts(QKeySequence::Quit);
  connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

  projectDialogAct = new QAction("Project settings", this);
  connect(projectDialogAct, SIGNAL(triggered()), this, SLOT(projectDialog()));

  configDialogAct = new QAction("Settings", this);
  connect(configDialogAct, SIGNAL(triggered()), this, SLOT(configDialog()));

  aboutAct = new QAction("About", this);
  connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

  upgradeAct = new QAction("Upgrade PMU Firmware", this);
  connect(upgradeAct, SIGNAL(triggered()), this, SLOT(upgrade()));

  openProjectAct = new QAction("Open custom project", this);
  connect(openProjectAct, SIGNAL(triggered()), this, SLOT(openCustomProject()));

  openProfileAct = new QAction("Open profile", this);
  connect(openProfileAct, SIGNAL(triggered()), this, SLOT(openProfileEvent()));

  openGProfAct = new QAction("Open data file from instrumented run", this);
  connect(openGProfAct, SIGNAL(triggered()), this, SLOT(openGProfEvent()));

  createProjectAct = new QAction("Create custom project", this);
  connect(createProjectAct, SIGNAL(triggered()), this, SLOT(createProject()));

  closeProjectAct = new QAction("Close project", this);
  connect(closeProjectAct, SIGNAL(triggered()), this, SLOT(closeProject()));

  // menus
  fileMenu = menuBar()->addMenu("File");

  if(Config::sdsocVersion) {
    projectMenu = fileMenu->addMenu("Open SDSoC project");
    connect(projectMenu, SIGNAL(triggered(QAction*)), this, SLOT(openProjectEvent(QAction*)));
  }

  fileMenu->addAction(openProjectAct);
  fileMenu->addAction(createProjectAct);
  fileMenu->addAction(closeProjectAct);
  fileMenu->addAction(openProfileAct);
  fileMenu->addAction(openGProfAct);
  fileMenu->addAction(projectDialogAct);
  fileMenu->addSeparator();
  fileMenu->addAction(configDialogAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);

  menuBar()->addSeparator();

  helpMenu = menuBar()->addMenu("Help");
  helpMenu->addAction(aboutAct);
  helpMenu->addAction(upgradeAct);
  // toolbars
  mainToolBar = addToolBar("MainTB");
  mainToolBar->setObjectName("MainTB");
  mainToolBar->addWidget(coreBox);
  mainToolBar->addWidget(sensorBox);

  cfgToolBar = addToolBar("CfgTB");
  cfgToolBar->setObjectName("CfgTB");
  cfgToolBar->addWidget(cfgModeBox);
  cfgToolBar->addWidget(colorBox);
  cfgToolBar->addAction(clearColorsAct);
  cfgToolBar->addAction(topAct);
  cfgToolBar->addAction(frameAct);
  cfgToolBar->addAction(hwAct);

  graphToolBar = addToolBar("GraphTB");
  graphToolBar->setObjectName("GraphTB");
  graphToolBar->addWidget(windowBox);

  projectToolBar = addToolBar("ProjectTB");
  projectToolBar->setObjectName("ProjectTB");

  // statusbar
  statusBar()->showMessage("Ready");

  // title
  setWindowTitle(APP_NAME);

  // settings
  QSettings settings;
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("windowState").toByteArray(), VERSION);
  cfgSplitter->restoreState(settings.value("cfgSplitterState").toByteArray());
  cfgSplitter2->restoreState(settings.value("cfgSplitter2State").toByteArray());

  if(!Config::sdsocVersion) {
    QMessageBox msgBox;
    msgBox.setText("SDSoC not found, or unsupported version");
    msgBox.exec();
  }

  for(unsigned i = 0; i < Pmu::MAX_CORES; i++) {
    coreBox->addItem(QString("Core ") + QString::number(i));
  }

  coreBox->setCurrentIndex(Config::core);
  sensorBox->setCurrentIndex(Config::sensor);
  windowBox->setCurrentIndex(windowBox->findData(Config::window));

  if(Config::sdsocVersion) {
    buildProjectMenu();
  }

  tabWidget->setCurrentIndex(0);
  cfgToolBar->setEnabled(true);
  graphToolBar->setEnabled(false);

  projectDialogAct->setEnabled(false);
  closeProjectAct->setEnabled(false);

  Config::colorMode = Config::STRUCT;
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::buildProjectMenu() {
  projectMenu->clear();

  QDir workspaceDir(Config::workspace);
  workspaceDir.setFilter(QDir::Dirs);

  QFileInfoList workspaceList = workspaceDir.entryInfoList();
  for(auto fileInfo : workspaceList) {
    if(fileInfo.fileName()[0] != '.') {
      QDir projectDir(fileInfo.filePath());
      projectDir.setFilter(QDir::Files);
      QFileInfoList projectFiles = projectDir.entryInfoList();

      for(auto fileInfo2 : projectFiles) {
        if((fileInfo2.fileName() == "project.sdx") || (fileInfo2.fileName() == "project.sdsoc")) {
          QMenu *menu = projectMenu->addMenu(fileInfo.fileName());
          QStringList buildConfigs = Sdsoc::getBuildConfigurations(fileInfo.filePath());
          for(auto configType : buildConfigs) {
            QAction *action = new QAction(configType, this);
            QStringList data;
            data << fileInfo.filePath();
            data << configType;
            action->setData(QVariant::fromValue(data));
            menu->addAction(action);
          }
        }
      }
    }
  }
}

void MainWindow::openProjectEvent(QAction *action) {
  closeProject();
  openProject(action->data().toStringList().at(0), action->data().toStringList().at(1));
}

void MainWindow::openCustomProject() {
  QFileDialog dialog(this, "Select project directory");
  dialog.setFileMode(QFileDialog::Directory);
  if(dialog.exec()) {
    closeProject();

    QString path = dialog.selectedFiles()[0];
    if(!openProject(path, "")) {
      QMessageBox msgBox;
      msgBox.setText("Can't open project");
      msgBox.exec();
    }
  }
}

void MainWindow::openProfileEvent() {
  QFileDialog dialog(this, "Select profile file");
  if(dialog.exec()) {
    if(analysis->project) {
      QString path = dialog.selectedFiles()[0];
      analysis->loadProfFile(path);
      loadFiles();
    }
  }
}

void MainWindow::openGProfEvent() {
  if(analysis->project) {
    QString gprofPath;
    QString elfPath;

    QFileDialog gprofDialog(this, "Select tulipp.gmn file");
    gprofDialog.setNameFilter(tr("TULIPP gmn file (*.gmn)"));

    if(gprofDialog.exec()) {
      if(analysis->project->isSdSocProject()) {
        gprofPath = gprofDialog.selectedFiles()[0];
        elfPath = analysis->project->elfFilename(true);
      } else {
        QFileDialog elfDialog(this, "Select instrumented elf file");
        if(elfDialog.exec()) {
          gprofPath = gprofDialog.selectedFiles()[0];
          elfPath = elfDialog.selectedFiles()[0];
        }
      }
    }

    analysis->loadGProfFile(gprofPath, elfPath);
    loadFiles();
  }
}

void MainWindow::createProject() {
  QFileDialog dialog(this, "Select project directory");
  dialog.setFileMode(QFileDialog::Directory);
  if(dialog.exec()) {
    closeProject();

    if(analysis->createProject(dialog.selectedFiles()[0])) {
      connect(analysis->project, SIGNAL(advance(int, QString)), this, SLOT(advance(int, QString)), Qt::BlockingQueuedConnection);

      setWindowTitle(QString(APP_NAME) + " : " + analysis->project->name + " (custom)");
      loadFiles();

    } else {
      QMessageBox msgBox;
      msgBox.setText("Can't create project");
      msgBox.exec();
    }
  }
}

void MainWindow::closeProject() {
  clearGui();

  analysis->closeProject();

  cfgView->setDse(NULL);

  if(hwGroup) delete hwGroup;
  hwGroup = NULL;
  if(topGroup) delete topGroup;
  topGroup = NULL;
  if(frameLoop) frameLoop = NULL;

  projectDialogAct->setEnabled(false);
  closeProjectAct->setEnabled(false);
  projectToolBar->clear();

  setWindowTitle(QString(APP_NAME));
}

void MainWindow::clearGui() {
  // clear CFG view
  treeView->setModel(NULL);
  cfgScene->clearScene();
  textView->loadFile("", std::vector<unsigned>());
  analysisView->setModel(NULL);

  // clear gantt graph
  graphScene->clearScene();

  // clear profile table
  tableView->setModel(NULL);
}

void MainWindow::loadFiles() {
  QApplication::setOverrideCursor(Qt::WaitCursor);

  clearGui();

  analysis->load();

  graphScene->drawProfile(Config::core, Config::sensor, analysis->project->cfg, analysis->profile);

  if(profModel) delete profModel;
  profModel = new ProfModel(Config::core, analysis->project->cfg);
  tableView->setModel(profModel);
  tableView->sortByColumn(0, Qt::AscendingOrder);
  QSettings settings;
  tableView->horizontalHeader()->restoreState(settings.value("tableViewState").toByteArray());

  if(cfgModel) delete cfgModel;
  cfgModel = new CfgModel(analysis->project->cfg);
  treeView->setModel(cfgModel);

  treeView->header()->restoreState(settings.value("treeViewState").toByteArray());

  topEvent();

  projectDialogAct->setEnabled(true);
  closeProjectAct->setEnabled(true);

  QApplication::restoreOverrideCursor();
}

bool MainWindow::openProject(QString path, QString configType) {
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(analysis->openProject(path, configType)) {
    projectToolBar->clear();

    if(configType == "") {
      projectToolBar->addAction(cmakeAct);
      projectToolBar->addAction(makeAct);
      projectToolBar->addAction(refreshAct);
      projectToolBar->addAction(runAct);
      projectToolBar->addAction(profileAct);
      projectToolBar->addAction(showProfileAct);
      projectToolBar->addAction(showFrameAct);

      setWindowTitle(QString(APP_NAME) + " : " + analysis->project->name + " (custom)");

    } else {
      projectToolBar->addAction(cleanAct);
      projectToolBar->addAction(cleanBinAct);
      projectToolBar->addAction(makeXmlAct);
      projectToolBar->addAction(makeBinAct);
      projectToolBar->addAction(runAct);
      projectToolBar->addAction(profileAct);
      projectToolBar->addAction(showProfileAct);
      projectToolBar->addAction(showFrameAct);
      projectToolBar->addAction(showDseAct);

      setWindowTitle(QString(APP_NAME) + " : " + analysis->project->name + " (" + analysis->project->configType + ")");
    }
  }

  if(analysis->project) {
    if(analysis->project->opened) {
      connect(analysis->project, SIGNAL(advance(int, QString)), this, SLOT(advance(int, QString)), Qt::BlockingQueuedConnection);
      loadFiles();
    }
    cfgView->setDse(analysis->dse);
  }

  QApplication::restoreOverrideCursor();

  return analysis->project != NULL;
}

void MainWindow::closeEvent(QCloseEvent *event) {
  QSettings settings;
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState(VERSION));
  settings.setValue("cfgSplitterState", cfgSplitter->saveState());
  settings.setValue("cfgSplitter2State", cfgSplitter2->saveState());
  settings.setValue("treeViewState", treeView->header()->saveState());
  settings.setValue("analysisViewState", analysisView->header()->saveState());
  if(tableView->model()) {
    settings.setValue("tableViewState", tableView->horizontalHeader()->saveState());
  }
  settings.setValue("workspace", Config::workspace);
  settings.setValue("includeAllInstructions", Config::includeAllInstructions);
  settings.setValue("includeProfData", Config::includeProfData);
  settings.setValue("includeId", Config::includeId);
  if(analysis->project) {
    settings.setValue("currentProject", analysis->project->path);
    settings.setValue("currentBuildConfig", analysis->project->configType);
  } else {
    settings.setValue("currentProject", "");
    settings.setValue("currentBuildConfig", "");
  }
  settings.setValue("cmake", Config::cmake);
  settings.setValue("clang", Config::clang);
  settings.setValue("clangpp", Config::clangpp);
  settings.setValue("opt", Config::opt);
  settings.setValue("llc", Config::llc);
  settings.setValue("llvm_ir_parserPath", Config::llvm_ir_parser);
  settings.setValue("tulipp_source_toolPath", Config::tulipp_source_tool);
  settings.setValue("asPath", Config::as);
  settings.setValue("linkerPath", Config::linker);
  settings.setValue("linkerppPath", Config::linkerpp);
  settings.setValue("asUsPath", Config::asUs);
  settings.setValue("linkerUsPath", Config::linkerUs);
  settings.setValue("linkerppUsPath", Config::linkerppUs);
  settings.setValue("core", Config::core);
  settings.setValue("sensor", Config::sensor);
  settings.setValue("window", Config::window);
  settings.setValue("functionsInTable", Config::functionsInTable);
  settings.setValue("regionsInTable", Config::regionsInTable);
  settings.setValue("loopsInTable", Config::loopsInTable);
  settings.setValue("basicblocksInTable", Config::basicblocksInTable);

  QMainWindow::closeEvent(event);
}

void MainWindow::about() {
  QMessageBox::about(this, "About Analysis Tool",
                     QString("Analysis Tool V") + QString::number(VERSION) + "\n"
                     "A performance measurement and analysis utility\n"
                     "TULIPP EU Project, NTNU, 2018-2019");
}

void MainWindow::upgrade() {
  QFileDialog dialog(this, "Select PMU firmware file");
  dialog.setNameFilter(tr("Firmware binary (*.bin)"));
  if(dialog.exec()) {
    Pmu pmu;

    bool pmuInited = pmu.init();
    if(!pmuInited) {
      QMessageBox msgBox;
      msgBox.setText("Can't connect to PMU");
      msgBox.exec();
      return;
    }

    pmu.checkForUpgrade(dialog.selectedFiles()[0]);

    pmu.release();
  }
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::regionClicked(const QModelIndex &index) {
  Container *el = static_cast<Container*>(index.internalPointer());
  cfgScene->drawElement(el);
  textView->loadFile(el->getSourceFilename(), el->getSourceLineNumber());
}

void MainWindow::analysisClicked(const QModelIndex &index) {
  AnalysisElement *el = static_cast<AnalysisElement*>(index.internalPointer());
  textView->loadFile(el->sourceFilename, el->sourceLineNumbers);
}

MainWindow::~MainWindow() {
  if(profModel) {
    delete profModel;
  }
}

void MainWindow::clearColorsEvent() {
  cfgModel->clearColors();
  cfgScene->redraw();
}

void MainWindow::topEvent() {
  cfgModel->collapseAll();
  cfgModel->clearColors();

  if(topGroup) delete topGroup;
  topGroup = new Group("Overview", analysis->project->cfg);
  QVector<Function*> mainVector = analysis->project->cfg->getMain();
  for(auto main : mainVector) {
    topGroup->appendChild(main);
  }
  topGroup->appendChild(analysis->project->cfg->externalMod);
  
  topGroup->toggleExpanded();
  cfgScene->drawElement(topGroup);

  textView->loadFile(topGroup->getSourceFilename(), topGroup->getSourceLineNumber());
  cfgView->verticalScrollBar()->setValue(0);
  cfgView->horizontalScrollBar()->setValue(0);
}

void MainWindow::frameEvent() {
  cfgModel->collapseAll();
  cfgModel->clearColors();

  Cfg *cfg = analysis->project->cfg;

  BasicBlock *frameDoneBb = cfg->getFrameDoneBb(analysis->project->frameFunc);
  if(frameDoneBb) {
    frameLoop = frameDoneBb->getLoop();
    if(frameLoop) {
      frameLoop->toggleExpanded();
      cfgScene->drawElement(frameLoop);
      textView->loadFile(frameLoop->getSourceFilename(), frameLoop->getSourceLineNumber());
      cfgView->verticalScrollBar()->setValue(0);
      cfgView->horizontalScrollBar()->setValue(0);
    }
  }

  if(!frameLoop) cfgScene->clearScene();
}

void MainWindow::hwEvent() {
  cfgModel->collapseAll();
  cfgModel->clearColors();

  if(hwGroup) delete hwGroup;
  hwGroup = new Group("HW functions", analysis->project->cfg);

  for(auto acc : analysis->project->accelerators) {
    QFileInfo fileInfo(acc.filepath);
    Module *module = analysis->project->cfg->getModuleById(fileInfo.completeBaseName());
    QVector<Function*> functions = module->getFunctionsByName(acc.name);
    assert(functions.size() == 1);
    hwGroup->appendChild(functions.at(0));
  }

  hwGroup->toggleExpanded();

  cfgScene->drawElement(hwGroup);
}

void MainWindow::configDialog() {
  ConfigDialog dialog;
  dialog.exec();
  if(Config::sdsocVersion) buildProjectMenu();
  if(analysis->project) loadFiles();
}

void MainWindow::projectDialog() {
  ProjectDialog dialog(analysis->project);
  dialog.exec();
  analysis->project->saveProjectFile();
  cfgScene->redraw();
}

void MainWindow::refreshEvent() {
  if(analysis->project) {
    loadFiles();
  }
}

void MainWindow::cleanEvent() {
  if(analysis->project) {
    analysis->clean();
    loadFiles();
  }
}

void MainWindow::cleanBinEvent() {
  if(analysis->project) {
    analysis->cleanBin();
    loadFiles();
  }
}

void MainWindow::advance(int step, QString message) {
  if(progDialog) {
    progDialog->setValue(step);
    progDialog->setLabelText(message);
  }
}

void MainWindow::makeXmlEvent() {
  if(analysis->project) {
    treeView->setModel(NULL);

    if(analysis->profile) analysis->profile->clean();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    progDialog = new QProgressDialog("Making XML files...", QString(), 0, analysis->project->xmlBuildSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    analysis->project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), analysis->project, SLOT (makeXml()));
    connect(analysis->project, SIGNAL(finished(int, QString)), this, SLOT(finishXml(int, QString)));
    thread.start();
  }
}

void MainWindow::finishXml(int error, QString msg) {
  delete progDialog;
  progDialog = NULL;

  QApplication::restoreOverrideCursor();

  loadFiles();

  if(error) {
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();

  } else {
    QMessageBox msgBox;
    msgBox.setText("XML build completed");
    msgBox.exec();
  }

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(analysis->project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::makeBinEvent() {
  if(analysis->project) {
    treeView->setModel(NULL);

    if(analysis->profile) analysis->profile->clean();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    progDialog = new QProgressDialog("Making binary...", QString(), 0, analysis->project->binBuildSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    analysis->project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), analysis->project, SLOT (makeBin()));
    connect(analysis->project, SIGNAL(finished(int, QString)), this, SLOT(finishBin(int, QString)));
    thread.start();
  }
}

void MainWindow::finishBin(int error, QString msg) {
  delete progDialog;
  progDialog = NULL;

  QApplication::restoreOverrideCursor();

  if(error) {
    loadFiles();
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();

  } else {
    loadFiles();

    QMessageBox msgBox;
    msgBox.setText("Binary build completed");
    msgBox.exec();
  }

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(analysis->project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::makeEvent() {
  if(analysis->project) {
    treeView->setModel(NULL);

    if(analysis->profile) analysis->profile->clean();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    progDialog = new QProgressDialog("Making...", QString(), 0, analysis->project->makeSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    analysis->project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), analysis->project, SLOT (make()));
    connect(analysis->project, SIGNAL(finished(int, QString)), this, SLOT(finishMake(int, QString)));
    thread.start();
  }
}

void MainWindow::finishMake(int error, QString msg) {
  delete progDialog;
  progDialog = NULL;

  QApplication::restoreOverrideCursor();

  if(error) {
    loadFiles();
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();

  } else {
    loadFiles();

    QMessageBox msgBox;
    msgBox.setText("Build completed");
    msgBox.exec();
  }

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(analysis->project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::cmakeEvent() {
  if(analysis->project) {
    treeView->setModel(NULL);

    if(analysis->profile) analysis->profile->clean();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    progDialog = new QProgressDialog("Running CMake...", QString(), 0, analysis->project->cmakeSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    analysis->project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), analysis->project, SLOT (cmake()));
    connect(analysis->project, SIGNAL(finished(int, QString)), this, SLOT(finishCMake(int, QString)));
    thread.start();
  }
}

void MainWindow::finishCMake(int error, QString msg) {
  delete progDialog;
  progDialog = NULL;

  QApplication::restoreOverrideCursor();

  if(error) {
    loadFiles();
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();

  } else {
    loadFiles();

    QMessageBox msgBox;
    msgBox.setText("CMake completed");
    msgBox.exec();
  }

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(analysis->project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::runEvent() {
  if(analysis->project) {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    progDialog = new QProgressDialog("Running...", QString(), 0, analysis->project->runSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    analysis->project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), analysis->project, SLOT (runApp()));
    connect(analysis->project, SIGNAL(finished(int, QString)), this, SLOT(finishRun(int, QString)));
    thread.start();
  }
}

void MainWindow::finishRun(int error, QString msg) {
  delete progDialog;
  progDialog = NULL;

  QApplication::restoreOverrideCursor();

  if(error) {
    loadFiles();
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();

  } else {
    QMessageBox msgBox;
    msgBox.setText("Now running");
    msgBox.exec();
  }

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(analysis->project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::profileEvent() {
  if(analysis->project) {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    assert(analysis->profile);
    analysis->profile->clean();

    progDialog = new QProgressDialog("Profiling...", QString(), 0, analysis->project->profileSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    analysis->project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), analysis->project, SLOT (runProfiler()));
    connect(analysis->project, SIGNAL(finished(int, QString)), this, SLOT(finishProfile(int, QString)));
    thread.start();
  }
}

void MainWindow::finishProfile(int error, QString msg) {
  delete progDialog;
  progDialog = NULL;
  
  analysis->profile->update();

  QApplication::restoreOverrideCursor();

  if(error) {
    loadFiles();
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();

  } else {
    loadFiles();

    cfgScene->redraw();

    QMessageBox msgBox;
    msgBox.setText("Profiling done");
    msgBox.exec();
  }

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(analysis->project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::showDseSummary() {
  if(analysis->dse) {
    analysis->dse->showResults();

  } else {
    QMessageBox msgBox;
    msgBox.setText("No data to display");
    msgBox.exec();
  }
}

void MainWindow::showProfileSummary() {
  if(analysis->profile) {
    QString messageText;
    QTextStream messageTextStream(&messageText);

    messageTextStream << "<h4>Summary:</h4><table border=\"1\" cellpadding=\"5\">";
    messageTextStream << "<tr>";
    messageTextStream << "<td>Total runtime:</td><td>" << analysis->profile->getCycles() << " cycles</td>";
    messageTextStream << "</tr>";
    messageTextStream << "<tr>";
    messageTextStream << "<td>Total runtime:</td><td>" << analysis->profile->getRuntime() << "s</td>";
    messageTextStream << "</tr>";
    for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
      messageTextStream << "<tr>";
      messageTextStream << "<td>Total energy " << QString::number(i+1) << ":</td><td>" << analysis->profile->getEnergy(i) << "J</td>";
      messageTextStream << "</tr>";
    }
    if(analysis->project->isSdSocProject()) {
      Sdsoc *sdsoc = static_cast<Sdsoc*>(analysis->project);
      messageTextStream << "<tr>";
      messageTextStream << "<td>Timing:</td><td>" << (sdsoc->getTimingOk() ? "OK" : "Failed") << "</td>";
      messageTextStream << "</tr>";
      messageTextStream << "<tr>";
      messageTextStream << "<td>BRAMs:</td><td>" << sdsoc->getBrams() << "%</td>";
      messageTextStream << "</tr>";
      messageTextStream << "<tr>";
      messageTextStream << "<td>LUTs:</td><td>" << sdsoc->getLuts() << "%</td>";
      messageTextStream << "</tr>";
      messageTextStream << "<tr>";
      messageTextStream << "<td>DSPs:</td><td>" << sdsoc->getDsps() << "%</td>";
      messageTextStream << "</tr>";
      messageTextStream << "<tr>";
      messageTextStream << "<td>Registers:</td><td>" << sdsoc->getRegs() << "%</td>";
      messageTextStream << "</tr>";
    }
    messageTextStream << "</table>";
  
    QMessageBox msgBox;
    msgBox.setText(messageText);
    msgBox.exec();

  } else {
    QMessageBox msgBox;
    msgBox.setText("No data to display");
    msgBox.exec();
  }
}

void MainWindow::showFrameSummary() {
  if(analysis->profile) {
    QString messageText;
    QTextStream messageTextStream(&messageText);

    messageTextStream << "<h4>Frame Summary:</h4><table border=\"1\" cellpadding=\"5\">";

    messageTextStream << "<tr>";
    messageTextStream << "<td><b>Average frame rate</b></td><td><b>Min runtime</b></td><td><b>Average runtime</b></td><td><b>Max runtime</b></td>";
    messageTextStream << "</tr>";

    messageTextStream << "<tr>";
    messageTextStream << "<td>" << (1 / analysis->profile->getFrameRuntimeAvg()) << " Hz</td>";
    messageTextStream << "<td>" << analysis->profile->getFrameRuntimeMin() << " s</td>";
    messageTextStream << "<td>" << analysis->profile->getFrameRuntimeAvg() << " s</td>";
    messageTextStream << "<td>" << analysis->profile->getFrameRuntimeMax() << " s</td>";
    messageTextStream << "</tr>";

    messageTextStream << "<tr>";
    messageTextStream << "<td><b>Sensor</b></td>";
    messageTextStream << "<td><b>Min energy</b></td>";
    messageTextStream << "<td><b>Average energy</b></td>";
    messageTextStream << "<td><b>Max energy</b></td>";
    messageTextStream << "</tr>";

    for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
      double min = analysis->profile->getFrameEnergyMin(i);
      double avg = analysis->profile->getFrameEnergyAvg(i);
      double max = analysis->profile->getFrameEnergyMax(i);

      if(min < 0.05) min = 0;
      if(avg < 0.05) avg = 0;
      if(max < 0.05) max = 0;

      messageTextStream << "<tr>";
      messageTextStream << "<td>" << (i+1) << "</td>";
      messageTextStream << "<td>" << min << "J</td>";
      messageTextStream << "<td>" << avg << "J</td>";
      messageTextStream << "<td>" << max << "J</td>";
      messageTextStream << "</tr>";
    }
    messageTextStream << "</table>";
  
    QMessageBox msgBox;
    msgBox.setText(messageText);
    msgBox.exec();

  } else {
    QMessageBox msgBox;
    msgBox.setText("No data to display");
    msgBox.exec();
  }
}

void MainWindow::changeCore(int core) {
  Config::core = core;

  if(analysis->project) {
    if(hwGroup) hwGroup->clearCachedProfilingData();
    if(topGroup) topGroup->clearCachedProfilingData();
    analysis->project->cfg->clearCachedProfilingData();

    cfgScene->redraw();

    graphScene->clearScene();
    graphScene->drawProfile(Config::core, Config::sensor, analysis->project->cfg, analysis->profile, graphScene->minTime, graphScene->maxTime);

    if(profModel) delete profModel;
    profModel = new ProfModel(Config::core, analysis->project->cfg);
    tableView->setModel(profModel);
    tableView->sortByColumn(0, Qt::AscendingOrder);
    QSettings settings;
    tableView->horizontalHeader()->restoreState(settings.value("tableViewState").toByteArray());
  }
}

void MainWindow::changeSensor(int sensor) {
  Config::sensor = sensor;

  if(analysis->project) {
    if(hwGroup) hwGroup->clearCachedProfilingData();
    if(topGroup) topGroup->clearCachedProfilingData();
    analysis->project->cfg->clearCachedProfilingData();

    cfgScene->redraw();

    graphScene->clearScene();
    graphScene->drawProfile(Config::core, Config::sensor, analysis->project->cfg, analysis->profile, graphScene->minTime, graphScene->maxTime);

    tableView->reset();
  }
}

void MainWindow::changeWindow(int window) {
  Config::window = windowBox->currentData().toUInt();
  graphScene->clearScene();
  graphScene->drawProfile(Config::core, Config::sensor, analysis->project->cfg, analysis->profile, graphScene->minTime, graphScene->maxTime);
}

void MainWindow::changeCfgMode(int mode) {
  switch(mode) {
    case 0:
      Config::colorMode = Config::STRUCT;
      cfgScene->redraw();
      break;
    case 1:
      Config::colorMode = Config::RUNTIME;
      cfgScene->redraw();
      break;
    case 2:
      Config::colorMode = Config::POWER;
      cfgScene->redraw();
      break;
    case 3:
      Config::colorMode = Config::ENERGY;
      cfgScene->redraw();
      break;
    case 4:
      Config::colorMode = Config::RUNTIME_FRAME;
      cfgScene->redraw();
      break;
    case 5:
      Config::colorMode = Config::POWER_FRAME;
      cfgScene->redraw();
      break;
    case 6:
      Config::colorMode = Config::ENERGY_FRAME;
      cfgScene->redraw();
      break;
  }
}

void MainWindow::tabChanged(int tab) {
  switch(tab) {
    case 0:
      cfgToolBar->setEnabled(true);
      graphToolBar->setEnabled(false);
      break;
    case 1:
      cfgToolBar->setEnabled(false);
      graphToolBar->setEnabled(true);
      break;
    case 2:
      cfgToolBar->setEnabled(false);
      graphToolBar->setEnabled(false);
      break;
  }
}
