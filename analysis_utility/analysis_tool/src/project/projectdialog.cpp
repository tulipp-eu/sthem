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

#include "projectdialog.h"
#include "config/config.h"
#include "analysis_tool.h"

void ProjectProfPage::updateGui() {
  // sample pc
  if(runTcfCheckBox->checkState() != Qt::Checked) {
    samplePcCheckBox->setCheckState(Qt::Unchecked);
    samplePcCheckBox->setDisabled(1);
  } else {
    samplePcCheckBox->setEnabled(1);
  }

  // stop at combo
  if((samplePcCheckBox->checkState() != Qt::Checked) ||
     (runTcfCheckBox->checkState() != Qt::Checked)) {
    stopAtCombo->setCurrentIndex(STOP_AT_TIME);
    stopAtCombo->setDisabled(1);
  } else {
    stopAtCombo->setEnabled(1);
  }

  // start func
  if(runTcfCheckBox->checkState() != Qt::Checked) {
    startFuncEdit->setDisabled(1);
  } else {
    startFuncEdit->setEnabled(1);
  }

  // stop func
  if(stopAtCombo->currentIndex() == STOP_AT_BREAKPOINT) {
    stopFuncEdit->setEnabled(1);
  } else {
    stopFuncEdit->setDisabled(1);
  }

  // sample period
  if(stopAtCombo->currentIndex() == STOP_AT_BREAKPOINT) {
    samplePeriodEdit->setDisabled(1);
  } else {
    samplePeriodEdit->setEnabled(1);
  }
}

ProjectMainPage::ProjectMainPage(Project *project, QWidget *parent) : QWidget(parent) {

  QVBoxLayout *mainLayout = new QVBoxLayout;

  //---------------------------------------------------------------------------

  QGroupBox *xmlGroup = new QGroupBox("XML files to load at startup");

  xmlEdit = new QPlainTextEdit;
  QFontMetrics m(xmlEdit->font());
  xmlEdit->setFixedHeight((m.lineSpacing()+2) * 5);
  xmlEdit->setFixedWidth(m.maxWidth() * 20);

  xmlEdit->insertPlainText(project->systemXmls.join('\n'));

  QVBoxLayout *xmlLayout = new QVBoxLayout;
  xmlLayout->addWidget(xmlEdit);

  xmlGroup->setLayout(xmlLayout);

  //-----------------------------------------------------------------------------

  mainLayout->addWidget(xmlGroup);
  mainLayout->addStretch(1);
  setLayout(mainLayout);
}

ProjectBuildPage::ProjectBuildPage(Project *project, QWidget *parent) : QWidget(parent) {

  QVBoxLayout *mainLayout = new QVBoxLayout;

  if(project->isSdSocProject()) {
    QGroupBox *compGroup = new QGroupBox("Compiler Options");

    instrumentCheckBox = new QCheckBox("Instrument");
    instrumentCheckBox->setCheckState(project->instrument ? Qt::Checked : Qt::Unchecked);

    QLabel *optLabel = new QLabel("CFG view optimization level:");

    optCombo = new QComboBox;
    optCombo->addItem("-O0");
    optCombo->addItem("-O1");
    optCombo->addItem("-O2");
    optCombo->addItem("-O3");
    optCombo->addItem("-Os");
    if(project->cfgOptLevel < 0) {
      optCombo->setCurrentIndex(4);
    } else {
      optCombo->setCurrentIndex(project->cfgOptLevel);
    }

    QHBoxLayout *optLayout = new QHBoxLayout;
    optLayout->addWidget(optLabel);
    optLayout->addWidget(optCombo);
    optLayout->addStretch(1);

    createBbInfoCheckBox = new QCheckBox("Insert BB information in binary");
    createBbInfoCheckBox->setCheckState(project->createBbInfo ? Qt::Checked : Qt::Unchecked);

    QVBoxLayout *compLayout = new QVBoxLayout;
    compLayout->addWidget(instrumentCheckBox);
    compLayout->addLayout(optLayout);
    compLayout->addWidget(createBbInfoCheckBox);

    compGroup->setLayout(compLayout);

    mainLayout->addWidget(compGroup);

  } else {
    QGroupBox *buildGroup = new QGroupBox("Build Options");

    QLabel *cmakeLabel = new QLabel("Cmake arguments:");
    cmakeOptions = new QLineEdit(project->cmakeArgs);
    QHBoxLayout *cmakeLayout = new QHBoxLayout;
    cmakeLayout->addWidget(cmakeLabel);
    cmakeLayout->addWidget(cmakeOptions);
    
    buildGroup->setLayout(cmakeLayout);

    mainLayout->addWidget(buildGroup);
  }

  //---------------------------------------------------------------------------

  mainLayout->addStretch(1);

  setLayout(mainLayout);
}

void ProjectProfPage::setDefault(bool checked) {
  tcfUploadScriptEdit->clear();
  tcfUploadScriptEdit->insertPlainText(DEFAULT_TCF_UPLOAD_SCRIPT);
}

void ProjectProfPage::setDefaultUs(bool checked) {
  tcfUploadScriptEdit->clear();
  tcfUploadScriptEdit->insertPlainText(DEFAULT_TCF_UPLOAD_SCRIPT_US);
}

void ProjectProfPage::setDefaultHipperosUs(bool checked) {
  tcfUploadScriptEdit->clear();
  tcfUploadScriptEdit->insertPlainText(DEFAULT_TCF_UPLOAD_SCRIPT_HIPPEROS_US);
}

ProjectProfPage::ProjectProfPage(Project *project, QWidget *parent) : QWidget(parent) {
  QLabel label;
  QFontMetrics m(label.font());
  int labelWidth = m.width("Supply voltage:");
  int inputWidth = m.width("0.0000");

  //----------------------------------------------------------------------------

  QGroupBox *tcfGroup = new QGroupBox("Xilinx debugger settings");

  QLabel *tcfUploadScriptLabel = new QLabel("TCF upload script:");
  tcfUploadScriptEdit = new QPlainTextEdit;
  tcfUploadScriptEdit->setFixedHeight((m.lineSpacing()+2) * 10);
  tcfUploadScriptEdit->setFixedWidth(m.maxWidth() * 20);
  tcfUploadScriptEdit->insertPlainText(project->tcfUploadScript);
  QVBoxLayout *tcfUploadScriptLayout = new QVBoxLayout;
  tcfUploadScriptLayout->addWidget(tcfUploadScriptLabel);
  tcfUploadScriptLayout->addWidget(tcfUploadScriptEdit);

  QVBoxLayout *tcfLayout = new QVBoxLayout;
  tcfLayout->addLayout(tcfUploadScriptLayout);

  if(!project->isSdSocProject()) {
    QPushButton *defaultButton = new QPushButton("Default (Zynq)");
    connect(defaultButton, SIGNAL(clicked(bool)), this, SLOT(setDefault(bool)));

    QPushButton *defaultUsButton = new QPushButton("Default (Zynq Ultrascale+)");
    connect(defaultUsButton, SIGNAL(clicked(bool)), this, SLOT(setDefaultUs(bool)));

    QPushButton *defaultHipperosUsButton = new QPushButton("Default HIPPEROS (Zynq Ultrascale+)");
    connect(defaultHipperosUsButton, SIGNAL(clicked(bool)), this, SLOT(setDefaultHipperosUs(bool)));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(defaultButton);
    buttonLayout->addWidget(defaultUsButton);
    buttonLayout->addStretch(1);
    tcfLayout->addLayout(buttonLayout);

    QHBoxLayout *buttonLayoutHipperos = new QHBoxLayout;
    buttonLayoutHipperos->addWidget(defaultHipperosUsButton);
    buttonLayoutHipperos->addStretch(1);
    tcfLayout->addLayout(buttonLayoutHipperos);
  }

  tcfLayout->addStretch(1);
  tcfGroup->setLayout(tcfLayout);

  //---------------------------------------------------------------------------

  QGroupBox *lynsynGroup = new QGroupBox("Lynsyn settings");

  QHBoxLayout *rlLayout = new QHBoxLayout;
  QLabel *rlLabel = new QLabel("Rl:");
  rlLabel->setFixedWidth(labelWidth);
  rlLayout->addWidget(rlLabel);
  for(unsigned i = 0; i < project->pmu.numSensors(); i++) {
    rlEdit[i] = new QLineEdit(QString::number(project->pmu.rl[i]));
    rlEdit[i]->setFixedWidth(inputWidth);
    rlLayout->addWidget(rlEdit[i]);
  }
  rlLayout->addStretch(1);

  QVBoxLayout *lynsynLayout = new QVBoxLayout;
  lynsynLayout->addLayout(rlLayout);
  lynsynLayout->addStretch(1);
  lynsynGroup->setLayout(lynsynLayout);

  //---------------------------------------------------------------------------

  QGroupBox *targetGroup = new QGroupBox("Target settings");

  QHBoxLayout *supplyVoltageLayout = new QHBoxLayout;
  QLabel *supplyVoltageLabel = new QLabel("Supply voltage:");
  supplyVoltageLabel->setFixedWidth(labelWidth);
  supplyVoltageLayout->addWidget(supplyVoltageLabel);
  for(unsigned i = 0; i < project->pmu.numSensors(); i++) {
    supplyVoltageEdit[i] = new QLineEdit(QString::number(project->pmu.supplyVoltage[i]));
    supplyVoltageEdit[i]->setFixedWidth(inputWidth);
    supplyVoltageLayout->addWidget(supplyVoltageEdit[i]);
  }
  supplyVoltageLayout->addStretch(1);

  QVBoxLayout *targetLayout = new QVBoxLayout;
  targetLayout->addLayout(supplyVoltageLayout);
  targetLayout->addStretch(1);
  targetGroup->setLayout(targetLayout);

  //---------------------------------------------------------------------------

  QGroupBox *measurementsGroup = new QGroupBox("Measurements");

  QHBoxLayout *customElfLayout = new QHBoxLayout;
  QLabel *customElfLabel = new QLabel("Custom elf files:");
  customElfEdit = new QLineEdit(project->customElfFile);
  customElfLayout->addWidget(customElfLabel);
  customElfLayout->addWidget(customElfEdit);

  samplingModeGpioCheckBox = new QCheckBox("GPIO controlled sampling");
  samplingModeGpioCheckBox->setCheckState(project->samplingModeGpio ? Qt::Checked : Qt::Unchecked);

  runTcfCheckBox = new QCheckBox("Upload binary and reset");
  runTcfCheckBox->setCheckState(project->runTcf ? Qt::Checked : Qt::Unchecked);
  connect(runTcfCheckBox, SIGNAL(clicked(bool)), this, SLOT(updateGui()));

  samplePcCheckBox = new QCheckBox("Sample PC");
  samplePcCheckBox->setCheckState(project->samplePc ? Qt::Checked : Qt::Unchecked);
  connect(samplePcCheckBox, SIGNAL(clicked(bool)), this, SLOT(updateGui()));

  QHBoxLayout *startLayout = new QHBoxLayout;
  QLabel *startFuncLabel = new QLabel("Start location:");
  startLayout->addWidget(startFuncLabel);
  startFuncEdit = new QLineEdit(project->startFunc);
  startLayout->addWidget(startFuncEdit);
  startLayout->addStretch(1);

  QHBoxLayout *stopAtLayout = new QHBoxLayout;
  QLabel *stopAtLabel = new QLabel("Stop at:");
  stopAtCombo = new QComboBox();
  stopAtCombo->addItem("Location");
  stopAtCombo->addItem("Time");
  stopAtCombo->setCurrentIndex(project->stopAt);
  stopAtLayout->addWidget(stopAtLabel);
  stopAtLayout->addWidget(stopAtCombo);
  stopAtLayout->addStretch(1);
  connect(stopAtCombo, SIGNAL(activated(int)), this, SLOT(updateGui()));

  QHBoxLayout *stopLayout = new QHBoxLayout;
  QLabel *stopFuncLabel = new QLabel("Stop location:");
  stopLayout->addWidget(stopFuncLabel);
  stopFuncEdit = new QLineEdit(project->stopFunc);
  stopLayout->addWidget(stopFuncEdit);
  stopLayout->addStretch(1);

  QHBoxLayout *samplePeriodLayout = new QHBoxLayout;
  QLabel *samplePeriodLabel = new QLabel("Sample period (in seconds):");
  samplePeriodEdit = new QLineEdit(QString::number(project->samplePeriod));
  samplePeriodLayout->addWidget(samplePeriodLabel);
  samplePeriodLayout->addWidget(samplePeriodEdit);

  QHBoxLayout *frameLayout = new QHBoxLayout;
  QLabel *frameFuncLabel = new QLabel("Frame done location:");
  frameLayout->addWidget(frameFuncLabel);
  frameFuncEdit = new QLineEdit(project->frameFunc);
  frameLayout->addWidget(frameFuncEdit);
  frameLayout->addStretch(1);

  QVBoxLayout *measurementsLayout = new QVBoxLayout;
  measurementsLayout->addLayout(customElfLayout);
  measurementsLayout->addWidget(samplingModeGpioCheckBox);
  measurementsLayout->addWidget(runTcfCheckBox);
  measurementsLayout->addWidget(samplePcCheckBox);
  measurementsLayout->addLayout(startLayout);
  measurementsLayout->addLayout(stopAtLayout);
  measurementsLayout->addLayout(stopLayout);
  measurementsLayout->addLayout(samplePeriodLayout);
  measurementsLayout->addLayout(frameLayout);
  measurementsLayout->addStretch(1);
  measurementsGroup->setLayout(measurementsLayout);

  //---------------------------------------------------------------------------

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tcfGroup);
  mainLayout->addWidget(lynsynGroup);
  mainLayout->addWidget(targetGroup);
  mainLayout->addWidget(measurementsGroup);
  mainLayout->addStretch(1);
  setLayout(mainLayout);

  updateGui();
}

ProjectDialog::ProjectDialog(Project *project) {
  assert(project);

  this->project = project;

  contentsWidget = new QListWidget;
  contentsWidget->setViewMode(QListView::IconMode);
  contentsWidget->setIconSize(QSize(96, 84));
  contentsWidget->setMovement(QListView::Static);
  contentsWidget->setMaximumWidth(128);
  contentsWidget->setSpacing(12);
  contentsWidget->setMinimumWidth(128);

  pagesWidget = new QStackedWidget;

  mainPage = new ProjectMainPage(project);
  pagesWidget->addWidget(mainPage);

  buildPage = new ProjectBuildPage(project);
  pagesWidget->addWidget(buildPage);

  profPage = new ProjectProfPage(project);
  pagesWidget->addWidget(profPage);

  QPushButton *closeButton = new QPushButton("Close");

  QListWidgetItem *mainButton = new QListWidgetItem(contentsWidget);
  mainButton->setText("Main");
  mainButton->setTextAlignment(Qt::AlignHCenter);
  mainButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  QListWidgetItem *buildButton = new QListWidgetItem(contentsWidget);
  buildButton->setText("Build");
  buildButton->setTextAlignment(Qt::AlignHCenter);
  buildButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  QListWidgetItem *profButton = new QListWidgetItem(contentsWidget);
  profButton->setText("Profiler");
  profButton->setTextAlignment(Qt::AlignHCenter);
  profButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  connect(contentsWidget, &QListWidget::currentItemChanged, this, &ProjectDialog::changePage);

  contentsWidget->setCurrentRow(0);

  connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);

  QScrollArea *scrollArea = new QScrollArea;
  scrollArea->setWidget(pagesWidget);

  QHBoxLayout *horizontalLayout = new QHBoxLayout;
  horizontalLayout->addWidget(contentsWidget);
  horizontalLayout->addWidget(scrollArea, 1);

  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  buttonsLayout->addStretch(1);
  buttonsLayout->addWidget(closeButton);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(horizontalLayout);
  mainLayout->addStretch(1);
  mainLayout->addSpacing(12);
  mainLayout->addLayout(buttonsLayout);
  setLayout(mainLayout);

  setWindowTitle("Project Settings");
}

void ProjectDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous) {
  if (!current) {
    current = previous;
  }

  pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

void ProjectDialog::closeEvent(QCloseEvent *e) {
  // main page

  project->systemXmls = mainPage->xmlEdit->toPlainText().split("\n");

  // build page

  if(project->isSdSocProject()) {
    if(buildPage->optCombo->currentIndex() == 4) {
      project->cfgOptLevel = -1;
    } else {
      project->cfgOptLevel = buildPage->optCombo->currentIndex();
    }

    project->instrument = buildPage->instrumentCheckBox->checkState() == Qt::Checked;

    project->createBbInfo = buildPage->createBbInfoCheckBox->checkState() == Qt::Checked;
  }

  // prof page
  for(unsigned i = 0; i < project->pmu.numSensors(); i++) {
    project->pmu.supplyVoltage[i] = profPage->supplyVoltageEdit[i]->text().toDouble();
    project->pmu.rl[i] = profPage->rlEdit[i]->text().toDouble();
  }

  project->tcfUploadScript = profPage->tcfUploadScriptEdit->toPlainText();

  project->samplingModeGpio = profPage->samplingModeGpioCheckBox->checkState() == Qt::Checked;
  project->runTcf = profPage->runTcfCheckBox->checkState() == Qt::Checked;
  project->samplePc = profPage->samplePcCheckBox->checkState() == Qt::Checked;

  project->startFunc = profPage->startFuncEdit->text();

  project->stopAt = profPage->stopAtCombo->currentIndex();
  project->stopFunc = profPage->stopFuncEdit->text();
  project->samplePeriod = profPage->samplePeriodEdit->text().toDouble();

  project->frameFunc = profPage->frameFuncEdit->text();

  project->customElfFile = profPage->customElfEdit->text();

  project->cmakeArgs = buildPage->cmakeOptions->text();
}
