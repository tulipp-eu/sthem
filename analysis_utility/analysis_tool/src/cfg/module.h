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

#ifndef MODULE_H
#define MODULE_H

#include "analysis_tool.h"
#include "container.h"
#include "function.h"

class BasicBlock;

class Module : public Container {
public:
  std::map<QString,Vertex*> idToVertex;

  Module(QString id, Container *parent, QString sourceFilename = "") : Container(id, id, parent, 0, sourceFilename) {}
  virtual ~Module() {}

  virtual QColor getColor() {
    return MODULE_COLOR;
  }

  virtual QString getTypeName() {
    return "Module";
  }

  virtual Module *getModule() {
    return this;
  }

  bool isHw() {
    return false;
  }

  virtual BasicBlock *getBasicBlockById(QString id);
  virtual Function *getFunctionById(QString id);
  virtual QVector<Function*> getFunctionsByName(QString name);

  virtual Vertex *getVertexById(QString id);

  virtual void clearCallers() {
    for(auto child : children) {
      Function *func = static_cast<Function*>(child);
      func->clearCallers();
    }
  }
};

#endif
