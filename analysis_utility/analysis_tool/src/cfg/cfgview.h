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

#ifndef CFGVIEW_H
#define CFGVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QApplication>
#include <QTreeView>

#include "analysis_tool.h"
#include "vertex.h"
#include "analysis/analysismodel.h"
#include "dse/dse.h"

class CfgView : public QGraphicsView {

  Q_OBJECT

private:
  QTreeView *analysisView;
  Dse *dse;
  Vertex *contextVertex;
  QVector<BasicBlock*> contextCallStack;

protected:
  QAction *clearColorsAct;
  QAction *setTopAct;
  QAction *hlsAnalysisAct;
  QAction *dseAct;

  void mouseReleaseEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void contextMenuEvent(QContextMenuEvent *event);

public:
  CfgView(QTreeView *analysisView, Dse *dse, QGraphicsScene *scene);

public slots:
  void zoomInEvent() {
    scale(SCALE_IN_FACTOR, SCALE_IN_FACTOR);
  }
  void zoomOutEvent() {
    scale(SCALE_OUT_FACTOR, SCALE_OUT_FACTOR);
  }
  void clearColorsEvent();
  void setTopEvent();
  void hlsAnalysisEvent();
  void dseEvent();
};

#endif
