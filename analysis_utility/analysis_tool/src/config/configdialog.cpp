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

#include "configdialog.h"
#include "config.h"
#include "analysis_tool.h"

MainPage::MainPage(QWidget *parent) : QWidget(parent) {
  QGroupBox *sdsocGroup = new QGroupBox("SDSoC configuration");

  QLabel *workspaceLabel = new QLabel("Workspace:");
  workspaceEdit = new QLineEdit(Config::workspace);
  QHBoxLayout *workspaceLayout = new QHBoxLayout;
  workspaceLayout->addWidget(workspaceLabel);
  workspaceLayout->addWidget(workspaceEdit);

  QVBoxLayout *xilinxLayout = new QVBoxLayout;

  xilinxLayout->addLayout(workspaceLayout);

  sdsocGroup->setLayout(xilinxLayout);

  //---------------------------------------------------------------------------

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(sdsocGroup);
  mainLayout->addStretch(1);
  setLayout(mainLayout);
}

BuildPage::BuildPage(QWidget *parent) : QWidget(parent) {

  QGroupBox *toolGroup = new QGroupBox("Tools");

  QLabel *clangLabel = new QLabel("clang:");
  clangEdit = new QLineEdit(Config::clang);
  QHBoxLayout *clangLayout = new QHBoxLayout;
  clangLayout->addWidget(clangLabel);
  clangLayout->addWidget(clangEdit);

  QLabel *clangppLabel = new QLabel("clang++:");
  clangppEdit = new QLineEdit(Config::clangpp);
  QHBoxLayout *clangppLayout = new QHBoxLayout;
  clangppLayout->addWidget(clangppLabel);
  clangppLayout->addWidget(clangppEdit);

  QLabel *optLabel = new QLabel("opt:");
  optEdit = new QLineEdit(Config::opt);
  QHBoxLayout *optLayout = new QHBoxLayout;
  optLayout->addWidget(optLabel);
  optLayout->addWidget(optEdit);

  QLabel *llcLabel = new QLabel("llc:");
  llcEdit = new QLineEdit(Config::llc);
  QHBoxLayout *llcLayout = new QHBoxLayout;
  llcLayout->addWidget(llcLabel);
  llcLayout->addWidget(llcEdit);

  QLabel *llvm_ir_parserLabel = new QLabel("llvm_ir_parser:");
  llvm_ir_parserEdit = new QLineEdit(Config::llvm_ir_parser);
  QHBoxLayout *llvm_ir_parserLayout = new QHBoxLayout;
  llvm_ir_parserLayout->addWidget(llvm_ir_parserLabel);
  llvm_ir_parserLayout->addWidget(llvm_ir_parserEdit);

  QLabel *tulipp_source_toolLabel = new QLabel("tulipp_source_tool:");
  tulipp_source_toolEdit = new QLineEdit(Config::tulipp_source_tool);
  QHBoxLayout *tulipp_source_toolLayout = new QHBoxLayout;
  tulipp_source_toolLayout->addWidget(tulipp_source_toolLabel);
  tulipp_source_toolLayout->addWidget(tulipp_source_toolEdit);

  QLabel *asLabel = new QLabel("Assembler (Zynq):");
  asEdit = new QLineEdit(Config::as);
  QHBoxLayout *asLayout = new QHBoxLayout;
  asLayout->addWidget(asLabel);
  asLayout->addWidget(asEdit);

  QLabel *linkerLabel = new QLabel("Linker (Zynq):");
  linkerEdit = new QLineEdit(Config::linker);
  QHBoxLayout *linkerLayout = new QHBoxLayout;
  linkerLayout->addWidget(linkerLabel);
  linkerLayout->addWidget(linkerEdit);

  QLabel *linkerppLabel = new QLabel("Linker (C++) (Zynq):");
  linkerppEdit = new QLineEdit(Config::linkerpp);
  QHBoxLayout *linkerppLayout = new QHBoxLayout;
  linkerppLayout->addWidget(linkerppLabel);
  linkerppLayout->addWidget(linkerppEdit);

  QLabel *asUsLabel = new QLabel("Assembler (Zynq Ultrascale+):");
  asUsEdit = new QLineEdit(Config::asUs);
  QHBoxLayout *asUsLayout = new QHBoxLayout;
  asUsLayout->addWidget(asUsLabel);
  asUsLayout->addWidget(asUsEdit);

  QLabel *linkerUsLabel = new QLabel("Linker (Zynq Ultrascale+):");
  linkerUsEdit = new QLineEdit(Config::linkerUs);
  QHBoxLayout *linkerUsLayout = new QHBoxLayout;
  linkerUsLayout->addWidget(linkerUsLabel);
  linkerUsLayout->addWidget(linkerUsEdit);

  QLabel *linkerppUsLabel = new QLabel("Linker (C++) (Zynq Ultrascale+):");
  linkerppUsEdit = new QLineEdit(Config::linkerppUs);
  QHBoxLayout *linkerppUsLayout = new QHBoxLayout;
  linkerppUsLayout->addWidget(linkerppUsLabel);
  linkerppUsLayout->addWidget(linkerppUsEdit);

  QVBoxLayout *toolLayout = new QVBoxLayout;
  toolLayout->addLayout(clangLayout);
  toolLayout->addLayout(clangppLayout);
  toolLayout->addLayout(optLayout);
  toolLayout->addLayout(llcLayout);
  toolLayout->addLayout(llvm_ir_parserLayout);
  toolLayout->addLayout(tulipp_source_toolLayout);
  toolLayout->addLayout(asLayout);
  toolLayout->addLayout(linkerLayout);
  toolLayout->addLayout(linkerppLayout);
  toolLayout->addLayout(asUsLayout);
  toolLayout->addLayout(linkerUsLayout);
  toolLayout->addLayout(linkerppUsLayout);

  toolGroup->setLayout(toolLayout);

  //---------------------------------------------------------------------------

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(toolGroup);
  mainLayout->addStretch(1);
  setLayout(mainLayout);
}

VisualisationPage::VisualisationPage(QWidget *parent) : QWidget(parent) {
  QGroupBox *cfgGroup = new QGroupBox("CFG");

  allInstructionsCheckBox = new QCheckBox("Show all instructions");
  allInstructionsCheckBox->setCheckState(Config::includeAllInstructions ? Qt::Checked : Qt::Unchecked);

  profDataCheckBox = new QCheckBox("Show profile data");
  profDataCheckBox->setCheckState(Config::includeProfData ? Qt::Checked : Qt::Unchecked);

  idCheckBox = new QCheckBox("Show ID");
  idCheckBox->setCheckState(Config::includeId ? Qt::Checked : Qt::Unchecked);

  QVBoxLayout *cfgLayout = new QVBoxLayout;
  cfgLayout->addWidget(allInstructionsCheckBox);
  cfgLayout->addWidget(profDataCheckBox);
  cfgLayout->addWidget(idCheckBox);
  cfgGroup->setLayout(cfgLayout);

  //---------------------------------------------------------------------------

  QGroupBox *tableGroup = new QGroupBox("Tables");

  functionsCheckBox = new QCheckBox("Show functions");
  functionsCheckBox->setCheckState(Config::functionsInTable ? Qt::Checked : Qt::Unchecked);

  regionsCheckBox = new QCheckBox("Show regions");
  regionsCheckBox->setCheckState(Config::regionsInTable ? Qt::Checked : Qt::Unchecked);

  loopsCheckBox = new QCheckBox("Show loops");
  loopsCheckBox->setCheckState(Config::loopsInTable ? Qt::Checked : Qt::Unchecked);

  basicblocksCheckBox = new QCheckBox("Show basicblocks");
  basicblocksCheckBox->setCheckState(Config::basicblocksInTable ? Qt::Checked : Qt::Unchecked);

  QVBoxLayout *tableLayout = new QVBoxLayout;
  tableLayout->addWidget(functionsCheckBox);
  tableLayout->addWidget(regionsCheckBox);
  tableLayout->addWidget(loopsCheckBox);
  tableLayout->addWidget(basicblocksCheckBox);
  tableGroup->setLayout(tableLayout);

  //---------------------------------------------------------------------------

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(cfgGroup);
  mainLayout->addWidget(tableGroup);
  mainLayout->addStretch(1);
  setLayout(mainLayout);
}

ConfigDialog::ConfigDialog() {
  contentsWidget = new QListWidget;
  contentsWidget->setViewMode(QListView::IconMode);
  contentsWidget->setIconSize(QSize(96, 84));
  contentsWidget->setMovement(QListView::Static);
  contentsWidget->setMaximumWidth(128);
  contentsWidget->setSpacing(12);
  contentsWidget->setMinimumWidth(128);

  pagesWidget = new QStackedWidget;

  mainPage = new MainPage;
  pagesWidget->addWidget(mainPage);

  buildPage = new BuildPage;
  pagesWidget->addWidget(buildPage);

  visualisationPage = new VisualisationPage;
  pagesWidget->addWidget(visualisationPage);

  QPushButton *closeButton = new QPushButton("Close");

  QListWidgetItem *mainButton = new QListWidgetItem(contentsWidget);
  mainButton->setText("Main");
  mainButton->setTextAlignment(Qt::AlignHCenter);
  mainButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  QListWidgetItem *buildButton = new QListWidgetItem(contentsWidget);
  buildButton->setText("Build");
  buildButton->setTextAlignment(Qt::AlignHCenter);
  buildButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  QListWidgetItem *visualisationButton = new QListWidgetItem(contentsWidget);
  visualisationButton->setText("Visualisation");
  visualisationButton->setTextAlignment(Qt::AlignHCenter);
  visualisationButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  connect(contentsWidget, &QListWidget::currentItemChanged, this, &ConfigDialog::changePage);

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

  setWindowTitle("Global Settings");
}

void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous) {
  if (!current) {
    current = previous;
  }

  pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

void ConfigDialog::closeEvent(QCloseEvent *e) {
  Config::workspace = mainPage->workspaceEdit->text();
  Config::includeAllInstructions = visualisationPage->allInstructionsCheckBox->checkState() == Qt::Checked;
  Config::includeProfData = visualisationPage->profDataCheckBox->checkState() == Qt::Checked;
  Config::includeId = visualisationPage->idCheckBox->checkState() == Qt::Checked;
  Config::clang = buildPage->clangEdit->text();
  Config::clangpp = buildPage->clangppEdit->text();
  Config::opt = buildPage->optEdit->text();
  Config::llc = buildPage->llcEdit->text();
  Config::llvm_ir_parser = buildPage->llvm_ir_parserEdit->text();
  Config::tulipp_source_tool = buildPage->tulipp_source_toolEdit->text();
  Config::as = buildPage->asEdit->text();
  Config::linker = buildPage->linkerEdit->text();
  Config::linkerpp = buildPage->linkerppEdit->text();
  Config::asUs = buildPage->asUsEdit->text();
  Config::linkerUs = buildPage->linkerUsEdit->text();
  Config::linkerppUs = buildPage->linkerppUsEdit->text();
  Config::functionsInTable = visualisationPage->functionsCheckBox->checkState() == Qt::Checked;
  Config::regionsInTable = visualisationPage->regionsCheckBox->checkState() == Qt::Checked;
  Config::loopsInTable = visualisationPage->loopsCheckBox->checkState() == Qt::Checked;
  Config::basicblocksInTable = visualisationPage->basicblocksCheckBox->checkState() == Qt::Checked;
}
