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

#ifndef DUMMY_H
#define DUMMY_H

#include "analysis_tool.h"
#include "vertex.h"

class Dummy : public Vertex {
  Vertex *source;
  Vertex *target;

public:
  Dummy(Vertex *source, Vertex *target, Container *parent) : Vertex("", "", parent) {
    this->source = source;
    this->target = target;
  }
  virtual ~Dummy() {}

  virtual QString getTypeName() {
    return "Dummy";
  }

  virtual void setLineSegmentsSource(unsigned n, std::vector<LineSegment>lines) {
    source->setLineSegmentsSource(n, lines);
  }

  virtual void setLineSegmentsTarget(std::vector<LineSegment>lines) {
    target->setLineSegmentsTarget(lines);
  }

  virtual void appendItems(QGraphicsItem *parent, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
    width = 0;
    height = 0;
    lineSegments.clear();
    baseItems.push(NULL);
  }
};

#endif
