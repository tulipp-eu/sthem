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
#include "dseresultdialog.h"
#include "cfg/loop.h"

double DseResultDialog::getTotalDseTime() {
  double time = 0;

  for(auto dseRun : *dseRuns) {
    time += dseRun.time;
  }

  return time;
}

DseRun DseResultDialog::getBestIndividual(int index, double *fitness) {
  *fitness = INT_MAX;
  DseRun r;
  r.failed = true;

  assert(dseRuns);
  assert(dseRuns->size());

  for(auto dseRun : *dseRuns) {
    double f = INT_MAX;
    if(!dseRun.failed) f = DseAlgorithm::getFitness(dseRun.profile, dseRun.project, index);
    if(f < *fitness) {
      *fitness = f;
      r = dseRun;
    }
  }

  return r;
}

void DseResultDialog::changeFitnessFunction(int index) {
  double fitness;
  DseRun best = getBestIndividual(index, &fitness);

  QString messageText;
  QTextStream messageTextStream(&messageText);

  messageTextStream << "<table border=\"1\" cellpadding=\"5\">";
  if(!best.failed) {
    messageTextStream << "<tr>";
    messageTextStream << "<td>Fitness:</td><td>" << QString::number(fitness) << "</td>";
    messageTextStream << "</tr>";
    messageTextStream << "<tr>";
    messageTextStream << "<td>Timing:</td><td>" << (best.project->getTimingOk() ? "OK" : "Failed") << "</td>";
    messageTextStream << "<td>BRAMs:</td><td>" << best.project->getBrams() << "%</td>";
    messageTextStream << "</tr>";
    messageTextStream << "<tr>";
    messageTextStream << "<td>LUTs:</td><td>" << best.project->getLuts() << "%</td>";
    messageTextStream << "<td>DSPs:</td><td>" << best.project->getDsps() << "%</td>";
    messageTextStream << "</tr>";
    messageTextStream << "<tr>";
    messageTextStream << "<td>Registers:</td><td>" << best.project->getRegs() << "%</td>";
    messageTextStream << "</tr>";
  }
  messageTextStream << "</table>";

  genomeText->setText(messageText);
}

void DseResultDialog::genSource() {
  QApplication::setOverrideCursor(Qt::WaitCursor);

  double fitness;
  DseRun dseRun = getBestIndividual(fitnessCombo->currentIndex(), &fitness);

  // transform source files
  QMap<QString,QStringList> hwFiles = DseAlgorithm::getFilesToTransform(loops, dseRun.genome);

  if(!hwFiles.size()) {
    QApplication::restoreOverrideCursor();

    QMessageBox msgBox;
    msgBox.setText("Identical to original, not copied");
    msgBox.exec();

  } else {
    for(auto f : hwFiles.toStdMap()) {
      QFileInfo fileInfo(f.first);

      // run source tool
      int ret = dseRun.project->runSourceTool(f.first, fileInfo.fileName(), f.second);
      Q_UNUSED(ret);
      assert(ret == 0);

      // backup old file
      bool r = QFile::copy(f.first, f.first + ".old");
      Q_UNUSED(r);
      assert(r);

      // remove old file
      r = QFile::remove(f.first);
      assert(r);

      // copy generated file
      r = QFile::copy(fileInfo.fileName(), f.first);
      assert(r);

      QApplication::restoreOverrideCursor();

      QMessageBox msgBox;
      msgBox.setText("Modified source file(s) copied to SDSoC project");
      msgBox.exec();
    }
  }
}

void DseResultDialog::changeAxes(int dummy) {
  QScatterSeries *series = new QScatterSeries();
  series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
  series->setMarkerSize(7.0);
  series->setBorderColor(Qt::transparent);

  double maxX = 0;
  double maxY = 0;

  double minX = INT_MAX;
  double minY = INT_MAX;

  QString xText;
  QString yText;

  for(auto r : *dseRuns) {
    if(!r.failed) {
      double x, y;

      x = DseAlgorithm::getFitness(r.profile, r.project, xCombo->currentIndex());
      xText = DseAlgorithm::getFitnessText(xCombo->currentIndex());

      y = DseAlgorithm::getFitness(r.profile, r.project, yCombo->currentIndex());
      yText = DseAlgorithm::getFitnessText(yCombo->currentIndex());

      *series << QPointF(x, y);
      if(x > maxX) maxX = x;
      if(x < minX) minX = x;
      if(y > maxY) maxY = y;
      if(y < minY) minY = y;
    }
  }

  chartView->chart()->removeAllSeries();
  chartView->chart()->addSeries(series);
  chartView->chart()->createDefaultAxes();

  double clearance = (maxX-minX) / 10;
  if(clearance == 0) clearance = maxX / 10;

  QAbstractAxis *x = chartView->chart()->axisX();
  x->setRange(minX-clearance,maxX+clearance);
  x->setTitleText(xText);

  clearance = (maxY-minY) / 10;
  if(clearance == 0) clearance = maxY / 10;

  QAbstractAxis *y = chartView->chart()->axisY();
  y->setRange(minY-clearance,maxY+clearance);
  y->setTitleText(yText);
}

DseResultDialog::DseResultDialog(QVector<DseRun> *dseRuns, unsigned fitness, QVector<Loop*> loops) {
  this->dseRuns = dseRuns;
  this->loops = loops;

  QGroupBox *ProcessGroup = new QGroupBox("DSE algorithm overview");

  QLabel *processText = new QLabel(this);

  QString messageText;
  QTextStream messageTextStream(&messageText);

  unsigned sec = getTotalDseTime();
  unsigned min = sec / 60;
  sec = sec % 60;
  unsigned hour = min / 60;
  min = min % 60;
  unsigned day = hour / 24;
  hour = hour % 24;

  messageTextStream << "<table border=\"1\" cellpadding=\"5\">";
  messageTextStream << "<tr>";
  messageTextStream << "<td>Total runtime:</td>";
  if(day) messageTextStream << "<td>" << QString::number(day) << "d</td>";
  if(hour || day) messageTextStream << "<td>" << QString::number(hour) << "h</td>";
  if(min || hour || day) messageTextStream << "<td>" << QString::number(min) << "m</td>";
  messageTextStream << "<td>" << QString::number(sec) << "s</td>";
  messageTextStream << "</tr>";
  messageTextStream << "</table>";
  processText->setText(messageText);

  QVBoxLayout *ProcessLayout = new QVBoxLayout;
  ProcessLayout->addWidget(processText);
  ProcessGroup->setLayout(ProcessLayout);

  ///////////////////////////////////////////////////////////////////////////////

  QGroupBox *bestGroup = new QGroupBox("Best individual");

  QLabel *fitnessText = new QLabel("Fitness", this);

  fitnessCombo = new QComboBox;
  addFitnessCombo(fitnessCombo);
  fitnessCombo->setCurrentIndex(fitness);

  connect(fitnessCombo, SIGNAL(activated(int)), this, SLOT(changeFitnessFunction(int)));

  QHBoxLayout *fitnessLayout = new QHBoxLayout;
  fitnessLayout->addWidget(fitnessText);
  fitnessLayout->addWidget(fitnessCombo);
  fitnessLayout->addStretch(1);

  genomeText = new QLabel(this);
  changeFitnessFunction(fitnessCombo->currentIndex());

  QPushButton *genButton = new QPushButton("Use this");
  connect(genButton, &QAbstractButton::clicked, this, &DseResultDialog::genSource);

  QVBoxLayout *bestLayout = new QVBoxLayout;
  bestLayout->addLayout(fitnessLayout);
  bestLayout->addWidget(genomeText);
  bestLayout->addWidget(genButton);
  bestGroup->setLayout(bestLayout);

  ///////////////////////////////////////////////////////////////////////////////

  QGroupBox *plotGroup = new QGroupBox("Plot");

  chartView = new QChartView;
  chartView->setRenderHint(QPainter::Antialiasing);
  chartView->chart()->setDropShadowEnabled(false);
  chartView->setMinimumSize(500,300);
  chartView->chart()->legend()->setVisible(false);

  QLabel *xText = new QLabel("X-axis", this);

  xCombo = new QComboBox;
  addFitnessCombo(xCombo);
  if(fitness == 9) xCombo->setCurrentIndex(0);
  else xCombo->setCurrentIndex(9);
  connect(xCombo, SIGNAL(activated(int)), this, SLOT(changeAxes(int)));

  QLabel *yText = new QLabel("Y-axis", this);

  yCombo = new QComboBox;
  addFitnessCombo(yCombo);
  yCombo->setCurrentIndex(fitness);
  connect(yCombo, SIGNAL(activated(int)), this, SLOT(changeAxes(int)));

  QHBoxLayout *xyLayout = new QHBoxLayout;
  xyLayout->addWidget(xText);
  xyLayout->addWidget(xCombo);
  xyLayout->addWidget(yText);
  xyLayout->addWidget(yCombo);
  xyLayout->addStretch(1);

  changeAxes(0);

  QVBoxLayout *plotLayout = new QVBoxLayout;
  plotLayout->addLayout(xyLayout);
  plotLayout->addWidget(chartView);
  plotGroup->setLayout(plotLayout);

  ///////////////////////////////////////////////////////////////////////////////

  QPushButton *okButton = new QPushButton("OK");
  okButton->setDefault(true);
  connect(okButton, &QAbstractButton::clicked, this, &QWidget::close);

  ///////////////////////////////////////////////////////////////////////////////

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(ProcessGroup);
  mainLayout->addWidget(bestGroup);
  mainLayout->addWidget(plotGroup);
  mainLayout->addWidget(okButton);

  setLayout(mainLayout);
  setWindowTitle("DSE Results");
}
