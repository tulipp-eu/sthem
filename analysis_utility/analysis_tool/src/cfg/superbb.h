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

#ifndef SUPERBB_H
#define SUPERBB_H

#include "analysis_tool.h"
#include "container.h"
#include "basicblock.h"

class SuperBB : public BasicBlock {

  Entry *entryNode;
  std::vector<Exit*> exitNodes;

public:
  SuperBB(QString id, Container *parent, unsigned treeviewRow) : BasicBlock(id, parent, treeviewRow) {
    entryNode = NULL;
  }
  virtual ~SuperBB() {
    if(entryNode) delete entryNode;
    for(auto exitNode : exitNodes) {
      delete exitNode;
    }
  }

  virtual QString getCfgName() {
    return "BasicBlock*";
  }

  virtual QColor getColor() {
    return BASICBLOCK_COLOR;
  }

  virtual QString getTypeName() {
    return getCfgName();
  }

  virtual void appendLocalItems(int startX, int yy, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling);

  virtual void calculateCallers() {
    for(auto child : children) {
      child->calculateCallers();
    }
  }
};

#endif
