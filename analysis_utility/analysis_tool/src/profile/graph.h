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

#ifndef GRAPH_H
#define GRAPH_H

#include <QGraphicsScene>
#include <QtWidgets>

#include "profile.h"

#define GRAPH_TEXT_SPACING 20

class Graph : public QGraphicsItem {
  int64_t maxTime;
  int maxPower;
  QVector<QPoint> points;
  unsigned textWidth;
  unsigned textHeight;
  QFont font;
  double lowValue;
  double highValue;

public:
  Graph(QFont font, double lowValue, double highValue, QGraphicsItem *parent = NULL) : QGraphicsItem(parent) {
    maxTime = 0;
    maxPower = 0;

    this->font = font;
    this->lowValue = lowValue;
    this->highValue = highValue;

    QFontMetrics fm(font);
    unsigned w1 = fm.width(QString::number(lowValue) + "W");
    unsigned w2 = fm.width(QString::number(highValue) + "W");
    if(w1 > w2) textWidth = w1;
    else textWidth = w2;
    textHeight = fm.height();
  }
  ~Graph() {}

  void addPoint(int64_t time, unsigned value) {
    points.push_back(QPoint(time, value));
    if((int)value > maxPower) {
      maxPower = value;
      prepareGeometryChange();
    }
    if(time > maxTime) {
      maxTime = time;
      prepareGeometryChange();
    }
  }

  QRectF boundingRect() const {
    qreal penWidth = 1;
    return QRectF(-penWidth/2 - textWidth-GRAPH_TEXT_SPACING,
                  -(double)maxPower - (double)textHeight,
                  maxTime + penWidth/2 + textWidth + GRAPH_TEXT_SPACING,
                  maxPower + textHeight);
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->setPen(QPen(NTNU_BLUE));

    painter->drawLine(0, 0, 0, -maxPower);
    painter->drawLine(0, 0, maxTime, 0);

    painter->drawText((int)(-textWidth-GRAPH_TEXT_SPACING), 0, QString::number(lowValue) + "W");
    painter->drawText((int)(-textWidth-GRAPH_TEXT_SPACING), -maxPower, QString::number(highValue) + "W");

    painter->setPen(QPen(FOREGROUND_COLOR));
    QPoint prevPoint = QPoint(0,0);
    for(auto point : points) {
      painter->drawLine(QPoint(prevPoint.x(), -prevPoint.y()), QPoint(point.x(), -point.y()));
      prevPoint = point;
    }
  }

};

#endif
