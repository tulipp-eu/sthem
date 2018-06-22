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

ProjectMainPage::ProjectMainPage(Project *project, QWidget *parent) : QWidget(parent) {

  QGroupBox *xmlGroup = new QGroupBox("XML files to load at startup");

  xmlEdit = new QPlainTextEdit;
  QFontMetrics m(xmlEdit->font());
  xmlEdit->setFixedHeight((m.lineSpacing()+2) * 5);
  xmlEdit->setFixedWidth(m.maxWidth() * 20);

  xmlEdit->insertPlainText(project->systemXmls.join('\n'));

  QVBoxLayout *xmlLayout = new QVBoxLayout;
  xmlLayout->addWidget(xmlEdit);

  xmlGroup->setLayout(xmlLayout);

  //---------------------------------------------------------------------------

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(xmlGroup);
  mainLayout->addStretch(1);
  setLayout(mainLayout);
}

ProjectBuildPage::ProjectBuildPage(Project *project, QWidget *parent) : QWidget(parent) {

  QGroupBox *compGroup = new QGroupBox("Compiler Options");

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
  compLayout->addLayout(optLayout);
  compLayout->addWidget(createBbInfoCheckBox);

  if(!project->isSdSocProject()) {
    QLabel *cOptLabel = new QLabel("C optimization level:");

    cOptCombo = new QComboBox;
    cOptCombo->addItem("-O0");
    cOptCombo->addItem("-O1");
    cOptCombo->addItem("-O2");
    cOptCombo->addItem("-O3");
    cOptCombo->addItem("-Os");
    if(project->cOptLevel < 0) {
      cOptCombo->setCurrentIndex(4);
    } else {
      cOptCombo->setCurrentIndex(project->cOptLevel);
    }

    QHBoxLayout *cOptLayout = new QHBoxLayout;
    cOptLayout->addWidget(cOptLabel);
    cOptLayout->addWidget(cOptCombo);
    cOptLayout->addStretch(1);

    QLabel *cOptionsLabel = new QLabel("C flags:");
    cOptionsEdit = new QLineEdit(project->cOptions);
    QHBoxLayout *cOptionsLayout = new QHBoxLayout;
    cOptionsLayout->addWidget(cOptionsLabel);
    cOptionsLayout->addWidget(cOptionsEdit);

    QLabel *cppOptLabel = new QLabel("Cpp optimization level:");

    cppOptCombo = new QComboBox;
    cppOptCombo->addItem("-O0");
    cppOptCombo->addItem("-O1");
    cppOptCombo->addItem("-O2");
    cppOptCombo->addItem("-O3");
    cppOptCombo->addItem("-Os");
    if(project->cppOptLevel < 0) {
      cppOptCombo->setCurrentIndex(4);
    } else {
      cppOptCombo->setCurrentIndex(project->cppOptLevel);
    }

    QHBoxLayout *cppOptLayout = new QHBoxLayout;
    cppOptLayout->addWidget(cppOptLabel);
    cppOptLayout->addWidget(cppOptCombo);
    cppOptLayout->addStretch(1);

    QLabel *cppOptionsLabel = new QLabel("Cpp flags:");
    cppOptionsEdit = new QLineEdit(project->cppOptions);
    QHBoxLayout *cppOptionsLayout = new QHBoxLayout;
    cppOptionsLayout->addWidget(cppOptionsLabel);
    cppOptionsLayout->addWidget(cppOptionsEdit);

    QLabel *linkerOptionsLabel = new QLabel("Linker flags:");
    linkerOptionsEdit = new QLineEdit(project->linkerOptions);
    QHBoxLayout *linkerOptionsLayout = new QHBoxLayout;
    linkerOptionsLayout->addWidget(linkerOptionsLabel);
    linkerOptionsLayout->addWidget(linkerOptionsEdit);

    compLayout->addLayout(cOptLayout);
    compLayout->addLayout(cOptionsLayout);
    compLayout->addLayout(cppOptLayout);
    compLayout->addLayout(cppOptionsLayout);
    compLayout->addLayout(linkerOptionsLayout);
  }

  compGroup->setLayout(compLayout);

  //---------------------------------------------------------------------------

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(compGroup);

  if(!project->isSdSocProject()) {
    QGroupBox *sourcesGroup = new QGroupBox("Source files");

    {
      sourcesEdit = new QPlainTextEdit;
      QFontMetrics m(sourcesEdit->font());
      sourcesEdit->setFixedHeight((m.lineSpacing()+2) * 10);
      sourcesEdit->setFixedWidth(m.maxWidth() * 20);

      sourcesEdit->insertPlainText(project->sources.join('\n'));

      QVBoxLayout *sourcesLayout = new QVBoxLayout;
      sourcesLayout->addWidget(sourcesEdit);

      sourcesGroup->setLayout(sourcesLayout);
    }

    mainLayout->addWidget(sourcesGroup);
  }

  //---------------------------------------------------------------------------

  mainLayout->addStretch(1);

  setLayout(mainLayout);
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

  QLabel *zynqLabel = new QLabel("Zynq version:");
  zynqCombo = new QComboBox;
  zynqCombo->addItem("Zynq 7000");
  zynqCombo->addItem("Zynq Ultrascale+");
  if(project->ultrascale) {
    zynqCombo->setCurrentIndex(1);
  } else {
    zynqCombo->setCurrentIndex(0);
  }
  QHBoxLayout *zynqLayout = new QHBoxLayout;
  zynqLayout->addWidget(zynqLabel);
  zynqLayout->addWidget(zynqCombo);
  zynqLayout->addStretch(1);

  QVBoxLayout *targetLayout = new QVBoxLayout;
  targetLayout->addLayout(supplyVoltageLayout);
  targetLayout->addLayout(zynqLayout);
  targetLayout->addStretch(1);
  targetGroup->setLayout(targetLayout);

  //---------------------------------------------------------------------------

  QGroupBox *breakpointsGroup = new QGroupBox("Measurements");

  QHBoxLayout *customElfLayout = new QHBoxLayout;
  customElfCheckBox = new QCheckBox("Use custom elf");
  customElfCheckBox->setCheckState(project->useCustomElf ? Qt::Checked : Qt::Unchecked);
  customElfEdit = new QLineEdit(project->customElfFile);
  customElfLayout->addWidget(customElfCheckBox);
  customElfLayout->addWidget(customElfEdit);

  QHBoxLayout *startLayout = new QHBoxLayout;
  QLabel *startFuncLabel = new QLabel("Start at function:");
  startLayout->addWidget(startFuncLabel);
  startFuncEdit = new QLineEdit(project->startFunc);
  startLayout->addWidget(startFuncEdit);
  QLabel *startCoreLabel = new QLabel("On core:");
  startLayout->addWidget(startCoreLabel);
  startCoreEdit = new QLineEdit(QString::number(project->startCore));
  startCoreEdit->setValidator(new QIntValidator(0, Pmu::MAX_CORES, this));
  startLayout->addWidget(startCoreEdit);
  startLayout->addStretch(1);

  QHBoxLayout *stopLayout = new QHBoxLayout;
  QLabel *stopFuncLabel = new QLabel("Stop at function:");
  stopLayout->addWidget(stopFuncLabel);
  stopFuncEdit = new QLineEdit(project->stopFunc);
  stopLayout->addWidget(stopFuncEdit);
  QLabel *stopCoreLabel = new QLabel("On core:");
  stopLayout->addWidget(stopCoreLabel);
  stopCoreEdit = new QLineEdit(QString::number(project->stopCore));
  stopCoreEdit->setValidator(new QIntValidator(0, Pmu::MAX_CORES, this));
  stopLayout->addWidget(stopCoreEdit);
  stopLayout->addStretch(1);

  QVBoxLayout *breakpointsLayout = new QVBoxLayout;
  breakpointsLayout->addLayout(customElfLayout);
  breakpointsLayout->addLayout(startLayout);
  breakpointsLayout->addLayout(stopLayout);
  breakpointsLayout->addStretch(1);
  breakpointsGroup->setLayout(breakpointsLayout);

  //---------------------------------------------------------------------------

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tcfGroup);
  mainLayout->addWidget(lynsynGroup);
  mainLayout->addWidget(targetGroup);
  mainLayout->addWidget(breakpointsGroup);
  mainLayout->addStretch(1);
  setLayout(mainLayout);
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
  project->systemXmls = mainPage->xmlEdit->toPlainText().split("\n");

  if(buildPage->optCombo->currentIndex() == 4) {
    project->cfgOptLevel = -1;
  } else {
    project->cfgOptLevel = buildPage->optCombo->currentIndex();
  }

  project->createBbInfo = buildPage->createBbInfoCheckBox->checkState() == Qt::Checked;

  if(!project->isSdSocProject()) {
    project->sources = buildPage->sourcesEdit->toPlainText().split("\n");
  
    if(buildPage->cOptCombo->currentIndex() == 4) {
      project->cOptLevel = -1;
    } else {
      project->cOptLevel = buildPage->cOptCombo->currentIndex();
    }
    project->cOptions = buildPage->cOptionsEdit->text();

    if(buildPage->cppOptCombo->currentIndex() == 4) {
      project->cppOptLevel = -1;
    } else {
      project->cppOptLevel = buildPage->cppOptCombo->currentIndex();
    }
    project->cppOptions = buildPage->cppOptionsEdit->text();

    project->linkerOptions = buildPage->linkerOptionsEdit->text();
  }

  project->useCustomElf = profPage->customElfCheckBox->checkState() == Qt::Checked;
  project->customElfFile = profPage->customElfEdit->text();

  for(unsigned i = 0; i < project->pmu.numSensors(); i++) {
    project->pmu.supplyVoltage[i] = profPage->supplyVoltageEdit[i]->text().toDouble();
    project->pmu.rl[i] = profPage->rlEdit[i]->text().toDouble();
  }

  if(profPage->zynqCombo->currentIndex() == 1) project->ultrascale = true;
  else project->ultrascale = false;

  project->tcfUploadScript = profPage->tcfUploadScriptEdit->toPlainText();

  project->startFunc = profPage->startFuncEdit->text();
  project->startCore = profPage->startCoreEdit->text().toUInt();

  project->stopFunc = profPage->stopFuncEdit->text();
  project->stopCore = profPage->stopCoreEdit->text().toUInt();
}
