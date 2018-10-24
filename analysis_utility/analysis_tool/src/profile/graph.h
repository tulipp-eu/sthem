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

#define TOP_MARGIN 20

#define GRAPH_TEXT_SPACING 20

class Graph : public QGraphicsItem {
  QVector<QPoint> points;
  unsigned textWidth;
  unsigned textHeight;
  QFont font;

  unsigned lowPower;
  unsigned highPower;
  int64_t lowTime;
  int64_t highTime;

  double lowPowerValue;
  double highPowerValue;
  double lowTimeValue;
  double highTimeValue;

public:
  Graph(QFont font,
        unsigned lowPower, unsigned highPower, int64_t lowTime, int64_t highTime,
        double lowPowerValue, double highPowerValue, double lowTimeValue, double highTimeValue,
        QGraphicsItem *parent = NULL) : QGraphicsItem(parent) {

    this->font = font;

    this->lowPower = lowPower;
    this->highPower = highPower;
    this->lowTime = lowTime;
    this->highTime = highTime;

    this->lowPowerValue = lowPowerValue;
    this->highPowerValue = highPowerValue;
    this->lowTimeValue = lowTimeValue;
    this->highTimeValue = highTimeValue;

    QFontMetrics fm(font);
    unsigned w1 = fm.width(QString::number(lowPowerValue) + "W");
    unsigned w2 = fm.width(QString::number(highPowerValue) + "W");
    if(w1 > w2) textWidth = w1;
    else textWidth = w2;
    textHeight = fm.height();
  }
  ~Graph() {}

  void addPoint(int64_t time, unsigned value) {
    points.push_back(QPoint(time, value));
  }

  QRectF boundingRect() const {
    qreal penWidth = 1;
    return QRectF(-penWidth/2 - textWidth-GRAPH_TEXT_SPACING,
                  -(double)highPower - TOP_MARGIN,
                  highTime + penWidth/2 + textWidth + GRAPH_TEXT_SPACING,
                  highPower + textHeight + TOP_MARGIN);
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QString highTimeText = QString::number(highTimeValue) + "s";

    QFontMetrics fm(font);

    painter->setPen(QPen(NTNU_BLUE));

    painter->drawLine(0, 0, 0, -highPower);
    painter->drawLine(0, 0, highTime, 0);

    painter->drawText((int)(-textWidth-GRAPH_TEXT_SPACING), 0, QString::number(lowPowerValue) + "W");
    painter->drawText((int)(-textWidth-GRAPH_TEXT_SPACING), -highPower + textHeight, QString::number(highPowerValue) + "W");

    painter->drawText((int)0, textHeight, QString::number(lowTimeValue) + "s");
    painter->drawText((int)highTime - fm.width(highTimeText), textHeight, highTimeText);

    painter->setPen(QPen(FOREGROUND_COLOR));
    QPoint prevPoint = QPoint(0,0);
    for(auto point : points) {
      painter->drawLine(QPoint(prevPoint.x(), -prevPoint.y()), QPoint(point.x(), -point.y()));
      prevPoint = point;
    }
  }

};

#endif
