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

#ifndef REGION_H
#define REGION_H

#include "analysis_tool.h"
#include "container.h"

class Region : public Container {

  Entry *entryNode;
  std::vector<Exit*> exitNodes;

public:
  bool isSuperBb;

  Region(QString id, Container *parent, unsigned treeviewRow, bool isSuperBb) : Container(id, id, parent, treeviewRow) {
    this->isSuperBb =isSuperBb;
    entryNode = NULL;
  }
  virtual ~Region() {
    if(entryNode) delete entryNode;
    for(auto exitNode : exitNodes) {
      delete exitNode;
    }
  }

  virtual QString getCfgName() {
    QString ret = "";

    if(isSuperBb) {
      ret = "BasicBlock*";
    } else {
      ret = "Region";
    }

    return ret;
  }

  virtual QColor getColor() {
    if(isSuperBb) {
      return BASICBLOCK_COLOR;
    } else {
      return REGION_COLOR;
    }
  }

  virtual QString getTypeName() {
    return getCfgName();
  }

  virtual void appendLocalItems(int startX, int yy, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling);

};

#endif
