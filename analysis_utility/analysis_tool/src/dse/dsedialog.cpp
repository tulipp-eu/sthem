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

#include "analysis_tool.h"
#include "dsedialog.h"

DseDialog::DseDialog(unsigned numberOfSolutions) {
  shouldRun = false;

  QGroupBox *compGroup = new QGroupBox("Components to explore");

  loopsCheckBox = new QCheckBox("Loop pipeline level");
  loopsCheckBox->setCheckState(Qt::Checked);

  QVBoxLayout *compLayout = new QVBoxLayout;
  compLayout->addWidget(loopsCheckBox);
  compGroup->setLayout(compLayout);

  ///////////////////////////////////////////////////////////////////////////////

  QGroupBox *algorithmGroup = new QGroupBox("Search algorithm");

  algCombo = new QComboBox;
  algCombo->addItem("Genetic Algorithm");
  algCombo->addItem("Exhaustive");
  algCombo->setCurrentIndex(1);

  QLabel *text = new QLabel(QString("Number of solutions: ") + QString::number(numberOfSolutions), this);

  QVBoxLayout *algorithmLayout = new QVBoxLayout;
  algorithmLayout->addWidget(algCombo);
  algorithmLayout->addWidget(text);
  algorithmGroup->setLayout(algorithmLayout);

  ///////////////////////////////////////////////////////////////////////////////

  QGroupBox *fitnessGroup = new QGroupBox("Search fitness");

  fitnessCombo = new QComboBox;
  addFitnessCombo(fitnessCombo);
  fitnessCombo->setCurrentIndex(0);

  QVBoxLayout *fitnessLayout = new QVBoxLayout;
  fitnessLayout->addWidget(fitnessCombo);
  fitnessGroup->setLayout(fitnessLayout);

  ///////////////////////////////////////////////////////////////////////////////

  QGroupBox *otherGroup = new QGroupBox("Other settings");

  rerunFailedCheckBox = new QCheckBox("Rerun failed individuals");
  rerunFailedCheckBox->setCheckState(Qt::Unchecked);

  QVBoxLayout *otherLayout = new QVBoxLayout;
  otherLayout->addWidget(rerunFailedCheckBox);
  otherGroup->setLayout(otherLayout);

  ///////////////////////////////////////////////////////////////////////////////

  QPushButton *cancelButton = new QPushButton("Cancel");
  cancelButton->setDefault(true);
  connect(cancelButton, &QAbstractButton::clicked, this, &QWidget::close);

  QPushButton *runButton = new QPushButton("Run DSE");
  connect(runButton, &QAbstractButton::clicked, this, &QWidget::close);
  connect(runButton, &QAbstractButton::clicked, this, &DseDialog::run);

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(cancelButton);
  buttonLayout->addWidget(runButton);

  ///////////////////////////////////////////////////////////////////////////////

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(compGroup);
  mainLayout->addWidget(algorithmGroup);
  mainLayout->addWidget(fitnessGroup);
  mainLayout->addWidget(otherGroup);
  mainLayout->addLayout(buttonLayout);
  mainLayout->addStretch(1);

  setLayout(mainLayout);

  setWindowTitle("Automatic DSE");
}

void DseDialog::run() {
  shouldRun = true;
}
