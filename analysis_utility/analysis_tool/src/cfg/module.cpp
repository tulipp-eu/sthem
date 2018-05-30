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

#include "module.h"
#include "basicblock.h"

BasicBlock *Module::getBasicBlockById(QString id) {
  return dynamic_cast<BasicBlock*>(getVertexById(id));
}

Vertex *Module::getVertexById(QString id) {
  auto item = idToVertex.find(id);
  if(item != idToVertex.end()) {
    return item->second;
  }
  return NULL;
}

Function *Module::getFunctionById(QString id) {
  for(auto func : children) {
    Function *f = static_cast<Function*>(func);
    if(f->id == id) return f;
  }
  return NULL;
}

QVector<Function*> Module::getFunctionsByName(QString name) {
  QVector<Function*> functions;

  for(auto func : children) {
    Function *f = static_cast<Function*>(func);
    if(f->name == name) functions.push_back(f);
  }

  return functions;
}
