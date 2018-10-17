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

#ifndef LOOP_H
#define LOOP_H

#include "analysis_tool.h"
#include "container.h"

class Loop : public Container {

  Entry *entryNode;
  std::vector<Exit*> exitNodes;

public:
  uint64_t count;

  Loop(QString id, Container *parent, unsigned treeviewRow, QString sourceFilename = "", unsigned sourceLineNumber = 1, unsigned sourceColumn = 1) : Container(id, id, parent, treeviewRow, sourceFilename, sourceLineNumber, sourceColumn) {
    entryNode = NULL;
  }
  virtual ~Loop() {
    if(entryNode) delete entryNode;
    for(auto exitNode : exitNodes) {
      delete exitNode;
    }
  }

  bool isLoop() {
    return true;
  }

  virtual bool isVisibleInTable() {
    return true;
  }

  virtual bool isVisibleInGantt() {
    return true;
  }

  virtual QString getTableName() {
    return getModule()->id + ":" + getFunction()->id + "():" + name;
  }

  virtual QString getGanttName() {
    return getTableName();
  }

  virtual QString getCfgName() {
    return "Loop";
  }

  virtual QColor getColor() {
    return LOOP_COLOR;
  }

  virtual QString getTypeName() {
    return getCfgName();
  }

  virtual Loop *getLoop() {
    return this;
  }

};

#endif
