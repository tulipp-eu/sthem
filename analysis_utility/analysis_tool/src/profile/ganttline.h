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

#ifndef GANTTLINE_H
#define GANTTLINE_H

#include <QGraphicsScene>
#include <QtWidgets>

#include "profile.h"

#define GANT_TEXT_SPACING 10

class GanttLine : public QGraphicsItem {

  QString id;
  unsigned textWidth;
  unsigned textHeight;
  unsigned maxTime;
  QVector<QPoint> lines;
  QFont font;
  QColor color;

public:
  GanttLine(QString id, QFont font, QColor color, QGraphicsItem *parent = NULL) : QGraphicsItem(parent) {
    this->id = id;
    this->font = font;
    this->color = color;

    maxTime = 0;

    QFontMetrics fm(font);
    textWidth = fm.width(id);
    textHeight = fm.height();
  }

  ~GanttLine() {}

  void addLine(unsigned start, unsigned stop) {
    lines.push_back(QPoint(start, stop));
    if(stop > maxTime) {
      maxTime = stop;
      prepareGeometryChange();
    }
  }

  QRectF boundingRect() const {
    return QRectF((int)(-textWidth-GANT_TEXT_SPACING), (int)-textHeight, (int)maxTime + textWidth + GANT_TEXT_SPACING, (int)textHeight*2);
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->setFont(font);
    painter->setPen(QPen(Qt::black));
    painter->setBrush(QBrush(color, Qt::SolidPattern));

    painter->drawText((int)(-textWidth-GANT_TEXT_SPACING), 0, id);

    for(auto line : lines) {
      painter->drawRect((int)line.x(), 0, (int)(line.y() - line.x()), (int)-(textHeight)/2);
    }
  }

};

#endif
