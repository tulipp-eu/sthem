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

#ifndef DSERESULTDIALOG_H
#define DSERESULTDIALOG_H

#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLegendMarker>
#include <QtWidgets>
#include <QDialog>

#include "dse.h"

QT_CHARTS_USE_NAMESPACE

class DseResultDialog : public QDialog {
  Q_OBJECT

private:
  QLabel *genomeText;
  QVector<DseRun> *dseRuns;
  QVector<Loop*> loops;
  QComboBox *fitnessCombo;
  QScatterSeries *series;
  QChartView *chartView;
  QComboBox *xCombo;
  QComboBox *yCombo;

  DseRun getBestIndividual(int index, double *fitness);
  double getTotalDseTime();

private slots:
  void changeFitnessFunction(int index);
  void genSource();
  void changeAxes(int dummy);

public:
  DseResultDialog(QVector<DseRun> *dseRuns, unsigned fitness, QVector<Loop*> loops);
};

#endif
