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

#ifndef FRAMELINE_H
#define FRAMELINE_H

#include <QGraphicsScene>
#include <QtWidgets>

#include "profile.h"

class FrameLine : public QGraphicsItem {

  int64_t time;
  unsigned lineHeight;
  unsigned lineDepth;
  QColor color;

public:
  FrameLine(int64_t time, unsigned lineHeight, unsigned lineDepth, QColor color, QGraphicsItem *parent = NULL) : QGraphicsItem(parent) {
    this->time = time;
    this->lineHeight = lineHeight;
    this->lineDepth = lineDepth;
    this->color = color;
  }

  ~FrameLine() {}

  QRectF boundingRect() const {
    return QRectF(time, -(int)lineHeight, 1, lineHeight + lineDepth);
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->setPen(QPen(color));
    painter->drawLine(QPoint(time, -lineHeight), QPoint(time, lineDepth));
  }

};

#endif
