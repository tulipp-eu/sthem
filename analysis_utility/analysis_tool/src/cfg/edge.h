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

#ifndef EDGE_H
#define EDGE_H

#include "analysis_tool.h"
#include "vertex.h"

class Vertex;

class Edge {
public:
  bool isReversed;
  int color;
  unsigned zvalue;
  unsigned sourceNum;
  Vertex *source;
  Vertex *target;

  Edge(Vertex *source, Vertex *target, bool isReversed = false, unsigned sourceNum = 0) {
    this->isReversed = isReversed;
    this->sourceNum = sourceNum;
    this->source = source;
    this->target = target;
    color = -1;
    zvalue = 0;
  }

  void toggleReversed() {
    isReversed = !isReversed;
  }
};

#endif
