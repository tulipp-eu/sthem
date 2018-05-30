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

#ifndef ENTRY_H
#define ENTRY_H

#include "analysis_tool.h"
#include "vertex.h"

class Entry : public Vertex {
public:
  Entry(QString id, Container *parent) : Vertex(id, "", parent) {}
  virtual ~Entry() {}
  virtual QColor getColor() {
    return ENTRY_COLOR;
  }

  virtual QString getSourceFilename();
  virtual std::vector<unsigned> getSourceLineNumber();

  virtual bool isEntry() {
    return true;
  }
  virtual QString getTypeName() {
    return "Entry";
  }

  virtual void appendItems(QGraphicsItem *parent, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
    QPolygonF polygon;
    polygon << QPointF(INPUTOUTPUT_SIZE/2,0)
            << QPointF(0,INPUTOUTPUT_SIZE)
            << QPointF(INPUTOUTPUT_SIZE,INPUTOUTPUT_SIZE);

    baseItems.push(new QGraphicsPolygonItem(polygon, parent));

    getBaseItem()->setData(0, QVariant::fromValue((void*)this));
    getBaseItem()->setData(1, makeQVariant(callStack));

    width = INPUTOUTPUT_SIZE;
    height = INPUTOUTPUT_SIZE;

    lineSegments.clear();
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
