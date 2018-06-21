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

#include "config/config.h"
#include "mainwindow.h"
#include "cfg/cfgview.h"
#include "cfg/basicblock.h"
#include "analysis/analysismodel.h"
#include "config/configdialog.h"
#include "project/projectdialog.h"
#include "project/pmu.h"

QColor edgeColors[] = EDGE_COLORS;
QString colorNames[] = COLOR_NAMES;

///////////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow() {
  progDialog = NULL;

  project = NULL;
  profile = NULL;
  hwGroup = NULL;
  topGroup = NULL;
  profModel = NULL;
  dse = NULL;

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
    sensorBox->addItem(QString("Sensor ") + QString::number(i));
  }
  connect(sensorBox, SIGNAL(activated(int)), this, SLOT(changeSensor(int)));

  cfgModeBox = new QComboBox();
  cfgModeBox->addItem("Structural");
  cfgModeBox->addItem("Runtime");
  cfgModeBox->addItem("Power");
  cfgModeBox->addItem("Energy");
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
  cleanAct = new QAction("Clean", this);
  connect(cleanAct, SIGNAL(triggered()), this, SLOT(cleanEvent()));

  makeXmlAct = new QAction("Make CFG", this);
  connect(makeXmlAct, SIGNAL(triggered()), this, SLOT(makeXmlEvent()));

  makeBinAct = new QAction("Make bin", this);
  connect(makeBinAct, SIGNAL(triggered()), this, SLOT(makeBinEvent()));

  profileAct = new QAction("Profile", this);
  connect(profileAct, SIGNAL(triggered()), this, SLOT(profileEvent()));

  showProfileAct = new QAction("Summary", this);
  connect(showProfileAct, SIGNAL(triggered()), this, SLOT(showProfileSummary()));

  showDseAct = new QAction("Summary(DSE)", this);
  connect(showDseAct, SIGNAL(triggered()), this, SLOT(showDseSummary()));

  clearColorsAct = new QAction("Clear", this);
  connect(clearColorsAct, SIGNAL(triggered()), this, SLOT(clearColorsEvent()));

  topAct = new QAction("Top", this);
  connect(topAct, SIGNAL(triggered()), this, SLOT(topEvent()));

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

  openProjectAct = new QAction("Open custom project", this);
  connect(openProjectAct, SIGNAL(triggered()), this, SLOT(openCustomProject()));

  createProjectAct = new QAction("Create custom project", this);
  connect(createProjectAct, SIGNAL(triggered()), this, SLOT(createProject()));

  closeProjectAct = new QAction("Close project", this);
  connect(closeProjectAct, SIGNAL(triggered()), this, SLOT(closeProject()));

  // menus
  fileMenu = menuBar()->addMenu("File");
  projectMenu = fileMenu->addMenu("Open SDSoC project");
  connect(projectMenu, SIGNAL(triggered(QAction*)), this, SLOT(openProjectEvent(QAction*)));
  fileMenu->addAction(openProjectAct);
  fileMenu->addAction(createProjectAct);
  fileMenu->addAction(closeProjectAct);
  fileMenu->addAction(projectDialogAct);
  fileMenu->addSeparator();
  fileMenu->addAction(configDialogAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);

  menuBar()->addSeparator();

  helpMenu = menuBar()->addMenu("Help");
  helpMenu->addAction(aboutAct);

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
  cfgToolBar->addAction(hwAct);

  graphToolBar = addToolBar("GraphTB");
  graphToolBar->setObjectName("GraphTB");
  graphToolBar->addWidget(windowBox);

  projectToolBar = addToolBar("ProjectTB");
  projectToolBar->setObjectName("ProjectTB");
  projectToolBar->addAction(cleanAct);
  projectToolBar->addAction(makeXmlAct);
  projectToolBar->addAction(makeBinAct);
  projectToolBar->addAction(profileAct);
  projectToolBar->addAction(showProfileAct);
  projectToolBar->addAction(showDseAct);

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
  Config::workspace = settings.value("workspace", QDir::homePath() + "/workspace/").toString();
  Config::includeAllInstructions = settings.value("includeAllInstructions", false).toBool();
  Config::includeProfData = settings.value("includeProfData", true).toBool();
  Config::includeId = settings.value("includeId", false).toBool();
  Config::xilinxDir = settings.value("xilinxDir", "/opt/Xilinx/SDx/2017.2/").toString();
  Config::hwserver = settings.value("hwserver", "127.0.0.1").toString();
  Config::hwport = settings.value("hwport", 3121).toUInt();
  Config::clang = settings.value("clang", "clang").toString();
  Config::clangpp = settings.value("clangpp", "clang++").toString();
  Config::llc = settings.value("llc", "llc").toString();
  Config::llvm_ir_parser = settings.value("llvm_ir_parserPath", "llvm_ir_parser").toString();
  Config::tulipp_source_tool = settings.value("tulipp_source_toolPath", "tulipp_source_tool").toString();
  Config::as = settings.value("asPath", "aarch64-none-elf-as").toString();
  Config::linker = settings.value("linkerPath", "aarch64-none-elf-gcc").toString();
  Config::linkerpp = settings.value("linkerppPath", "aarch64-none-elf-g++").toString();
  Config::core = settings.value("core", 0).toUInt();
  Config::sensor = settings.value("sensor", 0).toUInt();
  Config::window = settings.value("window", 1).toUInt();
  Config::sdsocVersion = settings.value("sdsocVersion", 20172).toUInt();

  for(unsigned i = 0; i < Pmu::MAX_CORES; i++) {
    coreBox->addItem(QString("Core ") + QString::number(i));
  }

  coreBox->setCurrentIndex(Config::core);
  sensorBox->setCurrentIndex(Config::sensor);
  windowBox->setCurrentIndex(windowBox->findData(Config::window));

  buildProjectMenu();

  // create .tulipp directory
  QDir dir(QDir::homePath() + "/.tulipp");
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  tabWidget->setCurrentIndex(0);
  cfgToolBar->setEnabled(true);
  graphToolBar->setEnabled(false);

  projectDialogAct->setEnabled(false);
  closeProjectAct->setEnabled(false);

  Config::colorMode = Config::STRUCT;

  // open previous project
  QString prevProject = settings.value("currentProject", "").toString();
  QString prevBuildConfig = settings.value("currentBuildConfig", "").toString();
  if(prevProject != "") {
    openProject(prevProject, prevBuildConfig);
  }
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

    CustomProject *customProject = new CustomProject;

    QString path = dialog.selectedFiles()[0];
    if(customProject->openProject(path)) {
      project = customProject;
      connect(project, SIGNAL(advance(int, QString)), this, SLOT(advance(int, QString)), Qt::BlockingQueuedConnection);

      setWindowTitle(QString(APP_NAME) + " : " + project->name + " (custom)");
      loadFiles();

    } else {
      delete customProject;

      QMessageBox msgBox;
      msgBox.setText("Can't open project");
      msgBox.exec();
    }
  }
}

void MainWindow::createProject() {
  QFileDialog dialog(this, "Select project directory");
  dialog.setFileMode(QFileDialog::Directory);
  if(dialog.exec()) {
    closeProject();

    CustomProject *customProject = new CustomProject;

    QString path = dialog.selectedFiles()[0];
    if(customProject->createProject(path)) {
      project = customProject;
      connect(project, SIGNAL(advance(int, QString)), this, SLOT(advance(int, QString)), Qt::BlockingQueuedConnection);

      setWindowTitle(QString(APP_NAME) + " : " + project->name + " (custom)");
      loadFiles();

    } else {
      delete customProject;

      QMessageBox msgBox;
      msgBox.setText("Can't create project");
      msgBox.exec();
    }
  }
}

void MainWindow::closeProject() {
  delete project;
  project = NULL;

  if(dse) delete dse;
  dse = NULL;
  cfgView->setDse(dse);

  if(profile) delete profile;
  profile = NULL;

  if(hwGroup) delete hwGroup;
  hwGroup = NULL;
  if(topGroup) delete topGroup;
  topGroup = NULL;

  clearGui();

  projectDialogAct->setEnabled(false);
  closeProjectAct->setEnabled(false);
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

  if(profile) delete profile;
  profile = NULL;

  clearGui();

  project->loadFiles();

  if(dse) dse->setCfg(project->getCfg());
  treeView->setModel(project->cfgModel);
  QSettings settings;
  treeView->header()->restoreState(settings.value("treeViewState").toByteArray());

  QDir dir(".");
  dir.setFilter(QDir::Files);
  // read files from tulipp project dir
  {
    QStringList nameFilter;
    nameFilter << "*.prof" << "*.elf" << "*.dse";
    dir.setNameFilters(nameFilter);

    QFileInfoList list = dir.entryInfoList();
    for(auto fileInfo : list) {
      loadFile(fileInfo.filePath());
    }
  }

  topEvent();

  projectDialogAct->setEnabled(true);
  closeProjectAct->setEnabled(true);

  QApplication::restoreOverrideCursor();
}

void MainWindow::loadFile(const QString &fileName) {
  QFileInfo fileInfo(fileName);
  QString suffix = fileInfo.suffix();
  if(suffix == "xml") {
    project->loadXmlFile(fileName);
  } else if(suffix == "prof") {
    loadProfFile(fileName);
  } else if(suffix == "dse") {
    loadDseFile(fileName);
  } else {
    project->elfFile = fileName;
  }
}

void MainWindow::loadDseFile(const QString &fileName) {
  std::ifstream inputFile(fileName.toUtf8()); 
  inputFile >> *dse;
  inputFile.close();
}

void MainWindow::loadProfFile(const QString &fileName) {
  QFile file(fileName);

  if(!file.open(QIODevice::ReadOnly)) {
    QMessageBox msgBox;
    msgBox.setText("File not found");
    msgBox.exec();
    return;
  }

  QVector<Measurement> *measurements;

  measurements = project->parseProfFile(file);

  file.close();

  if(profile) delete profile;
  profile = new Profile();
  profile->setProfData(measurements);

  project->getCfg()->setProfile(profile);

  if(profModel) delete profModel;
  profModel = new ProfModel(Config::core, project->getCfg());

  tableView->setModel(profModel);
  tableView->sortByColumn(0, Qt::AscendingOrder);
  QSettings settings;
  tableView->horizontalHeader()->restoreState(settings.value("tableViewState").toByteArray());

  graphScene->drawProfile(Config::core, Config::sensor, project->getCfg(), profile);
}

void MainWindow::openProject(QString path, QString configType) {
  if(configType == "") {
    CustomProject *customProject = new CustomProject;

    if(customProject->openProject(path)) {
      project = customProject;
      connect(project, SIGNAL(advance(int, QString)), this, SLOT(advance(int, QString)), Qt::BlockingQueuedConnection);

      setWindowTitle(QString(APP_NAME) + " : " + project->name + " (custom)");
      loadFiles();

    } else {
      delete customProject;
    }

  } else {
    Sdsoc *sdsocProject = NULL;

    if(Config::sdsocVersion == 20162) {
      sdsocProject = new Sdsoc20162();

    } else if(Config::sdsocVersion == 20172) {
      sdsocProject = new Sdsoc20172();
    }

    if(sdsocProject->openProject(path, configType)) {
      project = sdsocProject;
      connect(project, SIGNAL(advance(int, QString)), this, SLOT(advance(int, QString)), Qt::BlockingQueuedConnection);

      dse = new Dse(sdsocProject);
      dse->setCfg(project->getCfg());
      cfgView->setDse(dse);

      setWindowTitle(QString(APP_NAME) + " : " + project->name + " (" + project->configType + ")");
      loadFiles();

    } else {
      delete sdsocProject;
    }
  }
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
  if(project) {
    settings.setValue("currentProject", project->path);
    settings.setValue("currentBuildConfig", project->configType);
  } else {
    settings.setValue("currentProject", "");
    settings.setValue("currentBuildConfig", "");
  }
  settings.setValue("xilinxDir", Config::xilinxDir);
  settings.setValue("hwserver", Config::hwserver);
  settings.setValue("hwport", Config::hwport);
  settings.setValue("clang", Config::clang);
  settings.setValue("clangpp", Config::clangpp);
  settings.setValue("llc", Config::llc);
  settings.setValue("llvm_ir_parserPath", Config::llvm_ir_parser);
  settings.setValue("tulipp_source_toolPath", Config::tulipp_source_tool);
  settings.setValue("asPath", Config::as);
  settings.setValue("linkerPath", Config::linker);
  settings.setValue("linkerppPath", Config::linkerpp);
  settings.setValue("core", Config::core);
  settings.setValue("sensor", Config::sensor);
  settings.setValue("window", Config::window);
  settings.setValue("sdsocVersion", Config::sdsocVersion);

  QMainWindow::closeEvent(event);
}

void MainWindow::about() {
  QMessageBox::about(this, "About Analysis Tool",
                     "Analysis Tool\n"
                     "A performance measurement and analysis utility\n"
                     "TULIPP EU Project, NTNU 2018");
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
  if(profile) {
    delete profile;
  }
}

void MainWindow::clearColorsEvent() {
  project->cfgModel->clearColors();
  cfgScene->redraw();
}

void MainWindow::topEvent() {
  project->cfgModel->collapseAll();
  project->cfgModel->clearColors();

  if(topGroup) delete topGroup;
  topGroup = new Group("Overview");
  Vertex *main = project->cfgModel->getMain();
  if(main) topGroup->appendChild(main);
  topGroup->appendChild(project->cfgModel->getCfg()->externalMod);
  
  topGroup->toggleExpanded();
  cfgScene->drawElement(topGroup);

  textView->loadFile(topGroup->getSourceFilename(), topGroup->getSourceLineNumber());
  cfgView->verticalScrollBar()->setValue(0);
  cfgView->horizontalScrollBar()->setValue(0);
}

void MainWindow::hwEvent() {
  project->cfgModel->collapseAll();
  project->cfgModel->clearColors();

  if(hwGroup) delete hwGroup;
  hwGroup = new Group("HW functions");

  for(auto acc : project->accelerators) {
    QFileInfo fileInfo(acc.filepath);
    Module *module = project->getCfg()->getModuleById(fileInfo.baseName());
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
  buildProjectMenu();
  cfgScene->redraw();
}

void MainWindow::projectDialog() {
  ProjectDialog dialog(project);
  dialog.exec();
  project->saveProjectFile();
  cfgScene->redraw();
}

void MainWindow::cleanEvent() {
  if(project) {
    project->clean();
    if(dse) dse->clear();
    loadFiles();
    if(profile) delete profile;
    profile = NULL;
  }
}

void MainWindow::advance(int step, QString message) {
  if(progDialog) {
    progDialog->setValue(step);
    progDialog->setLabelText(message);
  }
}

void MainWindow::makeXmlEvent() {
  if(project) {
    treeView->setModel(NULL);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    progDialog = new QProgressDialog("Making XML files...", QString(), 0, project->xmlBuildSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), project, SLOT (makeXml()));
    connect(project, SIGNAL(finished(int, QString)), this, SLOT(finishXml(int, QString)));
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
  disconnect(project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::makeBinEvent() {
  if(project) {
    treeView->setModel(NULL);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    progDialog = new QProgressDialog("Making binary...", QString(), 0, project->binBuildSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), project, SLOT (makeBin()));
    connect(project, SIGNAL(finished(int, QString)), this, SLOT(finishBin(int, QString)));
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
    project->elfFile = project->elfFilename();
    loadFiles();

    QMessageBox msgBox;
    msgBox.setText("Binary build completed");
    msgBox.exec();
  }

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::profileEvent() {
  if(project) {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    progDialog = new QProgressDialog("Profiling...", QString(), 0, project->profileSteps(), this);
    progDialog->setWindowModality(Qt::WindowModal);
    progDialog->setMinimumDuration(0);
    progDialog->setValue(0);

    thread.wait();
    project->moveToThread(&thread);
    connect(&thread, SIGNAL (started()), project, SLOT (runProfiler()));
    connect(project, SIGNAL(finished(int, QString)), this, SLOT(finishProfile(int, QString)));
    thread.start();
  }
}

void MainWindow::finishProfile(int error, QString msg) {
  delete progDialog;
  progDialog = NULL;

  QApplication::restoreOverrideCursor();

  if(error) {
    loadFiles();
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();

  } else {
    project->elfFile = project->elfFilename();
    loadFiles();

    QVector<Measurement> *measurements = project->parseProfFile();

    if(profile) delete profile;
    profile = new Profile();

    profile->setProfData(measurements);

    project->getCfg()->setProfile(profile);

    if(profModel) delete profModel;
    profModel = new ProfModel(Config::core, project->getCfg());

    tableView->setModel(profModel);
    tableView->sortByColumn(0, Qt::AscendingOrder);
    QSettings settings;
    tableView->horizontalHeader()->restoreState(settings.value("tableViewState").toByteArray());

    graphScene->clearScene();
    graphScene->drawProfile(Config::core, Config::sensor, project->getCfg(), profile);
    graphScene->update();

    cfgScene->redraw();

    QMessageBox msgBox;
    msgBox.setText("Profiling done");
    msgBox.exec();
  }

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(project, SIGNAL(finished(int, QString)), 0, 0);

  thread.quit();
}

void MainWindow::showDseSummary() {
  if(dse) {
    dse->showResults();

  } else {
    QMessageBox msgBox;
    msgBox.setText("No data to display");
    msgBox.exec();
  }
}

void MainWindow::showProfileSummary() {
  if(profile) {
    QString messageText;
    QTextStream messageTextStream(&messageText);

    messageTextStream << "<h4>Summary:</h4><table border=\"1\" cellpadding=\"5\">";
    messageTextStream << "<tr>";
    messageTextStream << "<td>Total runtime:</td><td>" << profile->getRuntime() << "s</td>";
    messageTextStream << "</tr>";
    for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
      messageTextStream << "<tr>";
      messageTextStream << "<td>Total energy " << QString::number(i) << ":</td><td>" << profile->getEnergy(i) << "J</td>";
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

  if(hwGroup) hwGroup->clearCachedProfilingData();
  if(topGroup) topGroup->clearCachedProfilingData();
  project->getCfg()->clearCachedProfilingData();

  cfgScene->redraw();

  graphScene->clearScene();
  graphScene->drawProfile(Config::core, Config::sensor, project->getCfg(), profile);

  if(profModel) delete profModel;
  profModel = new ProfModel(Config::core, project->getCfg());
  tableView->setModel(profModel);
  tableView->sortByColumn(0, Qt::AscendingOrder);
  QSettings settings;
  tableView->horizontalHeader()->restoreState(settings.value("tableViewState").toByteArray());
}

void MainWindow::changeSensor(int sensor) {
  Config::sensor = sensor;

  if(hwGroup) hwGroup->clearCachedProfilingData();
  if(topGroup) topGroup->clearCachedProfilingData();
  project->getCfg()->clearCachedProfilingData();

  cfgScene->redraw();

  graphScene->clearScene();
  graphScene->drawProfile(Config::core, Config::sensor, project->getCfg(), profile);

  tableView->reset();
}

void MainWindow::changeWindow(int window) {
  Config::window = windowBox->currentData().toUInt();
  graphScene->clearScene();
  graphScene->drawProfile(Config::core, Config::sensor, project->getCfg(), profile);
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
