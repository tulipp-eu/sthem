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

#ifndef EXIT_H
#define EXIT_H

#include "analysis_tool.h"
#include "vertex.h"

class Exit : public Vertex {
public:
  Exit(QString id, Container *parent) : Vertex(id, "", parent) {}
  virtual ~Exit() {}
  virtual QColor getColor() {
    return EXIT_COLOR;
  }

  virtual QString getSourceFilename();
  virtual std::vector<unsigned> getSourceLineNumber();

  virtual void appendItems(QGraphicsItem *parent, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
    QPolygonF polygon;
    polygon << QPointF(INPUTOUTPUT_SIZE/2,INPUTOUTPUT_SIZE)
            << QPointF(0, 0)
            << QPointF(INPUTOUTPUT_SIZE, 0);

    baseItems.push(new QGraphicsPolygonItem(polygon, parent));

    getBaseItem()->setData(0, QVariant::fromValue((void*)this));
    getBaseItem()->setData(1, makeQVariant(callStack));

    width = INPUTOUTPUT_SIZE;
    height = INPUTOUTPUT_SIZE;

    lineSegments.clear();
  }

  virtual bool isExit() {
    return true;
  }
  virtual QString getTypeName() {
    return "Exit";
  }
  virtual unsigned getXIn() {
    return x + INPUTOUTPUT_SIZE/2;
  }
  virtual unsigned getYIn() {
    return y;
  }
  virtual unsigned getXOut(unsigned num) {
    return x + INPUTOUTPUT_SIZE/2;
  }
  virtual unsigned getYOut(unsigned num) {
    return y+height;
  }
};

#endif
