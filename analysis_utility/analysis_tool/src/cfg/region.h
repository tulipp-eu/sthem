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
  Region(QString id, Container *parent, unsigned treeviewRow) : Container(id, id, parent, treeviewRow) {
    entryNode = NULL;
  }
  virtual ~Region() {
    if(entryNode) delete entryNode;
    for(auto exitNode : exitNodes) {
      delete exitNode;
    }
  }

  virtual QString getCfgName() {
    return "Region";
  }

  virtual QColor getColor() {
    return REGION_COLOR;
  }

  virtual QString getTypeName() {
    return getCfgName();
  }

  virtual bool isVisibleInTable() {
    return Config::regionsInTable;
  }

  virtual bool isVisibleInGantt() {
    return Config::regionsInTable;
  }

};

#endif
