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

#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QApplication>

#include "graphscene.h"

class GraphView : public QGraphicsView {
  Q_OBJECT

protected:
  GraphScene *scene;
  int64_t beginTime;

  void mousePressEvent(QMouseEvent *mouseEvent) {
    if(mouseEvent->button() == Qt::LeftButton) {
      QPointF pos = mapToScene(mouseEvent->pos());
      beginTime = scene->posToTime(pos.x());
    } else if(mouseEvent->button() == Qt::RightButton) {
      scene->redrawFull();
      viewport()->update();
    }
  }

  void mouseReleaseEvent(QMouseEvent *mouseEvent) {
    if(mouseEvent->button() == Qt::LeftButton) {
      QPointF pos = mapToScene(mouseEvent->pos());
      int64_t endTime = scene->posToTime(pos.x());
      scene->minTime = beginTime;
      scene->maxTime = endTime;
      scene->redraw();
      viewport()->update();
    }
  }

  void wheelEvent(QWheelEvent *event) {
    if(event->modifiers() & Qt::ControlModifier) {
      if(event->delta() > 0) zoomInEvent();
      else zoomOutEvent();
      event->accept();
    } else {
      QGraphicsView::wheelEvent(event);
    }
  }

  void keyPressEvent(QKeyEvent *event) {
    if(event->modifiers() & Qt::ControlModifier) {
      if(event->key() == Qt::Key_Plus) {
        zoomInEvent();
      }
      if(event->key() == Qt::Key_Minus) {
        zoomOutEvent();
      }
    }
    event->accept();
  }

public:
  GraphView(GraphScene *scene) : QGraphicsView(scene) {
    this->scene = scene;
    setDragMode(QGraphicsView::ScrollHandDrag);
  }

public slots:
  void zoomInEvent() {
    if(scene->profile) {
      QPointF centerPoint = mapToScene(viewport()->width()/2, viewport()->height()/2);
    
      scene->scaleFactorTime *= SCALE_IN_FACTOR;
      if(scene->scaleFactorTime == 0) scene->scaleFactorTime = 10;
      scene->redraw();
      viewport()->update();

      centerOn(centerPoint.x() * SCALE_IN_FACTOR, centerPoint.y());
    }
  }
  void zoomOutEvent() {
    if(scene->profile) {
      QPointF centerPoint = mapToScene(viewport()->width()/2, viewport()->height()/2);

      scene->scaleFactorTime *= SCALE_OUT_FACTOR;
      scene->redraw();
      viewport()->update();

      centerOn(centerPoint.x() * SCALE_OUT_FACTOR, centerPoint.y());
    }
  }
};

#endif
