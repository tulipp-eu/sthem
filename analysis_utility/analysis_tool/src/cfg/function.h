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

#ifndef FUNCTION_H
#define FUNCTION_H

#include <unordered_set>

#include "analysis_tool.h"
#include "container.h"
#include "entry.h"
#include "exit.h"

class Function : public Container {

public:
  Entry *entryNode;
  Exit *exitNode;
  int callers;
  bool funcIsStatic;
  bool funcIsMember;
  bool ptrToPtrArg;
  bool hw;
  QVector<BasicBlock*> caller;

  Function(QString id, Container *parent, unsigned treeviewRow, QString sourceFilename = "", unsigned sourceLineNumber = 1, bool funcIsStatic = false, bool funcIsMember = false, bool ptrToPtrArg = false) : Container(id, "", parent, treeviewRow, sourceFilename, sourceLineNumber) {
    QString entryId = "__" + id + "_entry";
    QString exitId = "__" + id + "_exit";

    entryNode = new Entry(entryId, this);
    exitNode = new Exit(exitId, this);

    appendChild(entryNode);
    appendChild(exitNode);

    callers = 0;

    hw = false;

    this->funcIsStatic = funcIsStatic;
    this->funcIsMember = funcIsMember;
    this->ptrToPtrArg = ptrToPtrArg;

    QStringList list = id.split("(");
    name = list[0];
  }
  virtual ~Function() {}

  virtual void addCaller(BasicBlock *c) {
    callers++;
    caller.push_back(c);
  }

  virtual QColor getColor() {
    if(isHw()) {
      return HW_FUNCTION_COLOR;
    } else {
      return FUNCTION_COLOR;
    }
  }

  virtual QColor getColor(QVector<BasicBlock*> callStack) {
    if(Vertex::isHw(callStack)) {
      return HW_FUNCTION_COLOR;
    } else {
      return FUNCTION_COLOR;
    }
  }

  virtual bool isHw() {
    return hw;
  }

  virtual Function *getFunction() {
    return this;
  }

  virtual bool isVisibleInTable() {
    return true;
  }

  virtual bool isVisibleInGantt() {
    return true;
  }

  bool isFunction() {
    return true;
  }

  virtual unsigned getNumEdges() {
    return 0;
  }

  virtual QString getTypeName() {
    if(isHw()) {
      return "HardWare";
    } else {
      return "Function";
    }
  }

  virtual QString getListViewName() {
    if(isHw()) {
      return name + " [HW]";
    } else {
      return name + "()";
    }
  }

  virtual QString getGanttName() {
    return getTableName();
  }

  virtual QString getTableName();

  virtual void clearCallers() {
    callers = 0;
    caller.clear();
  }

  bool isStatic() {
    return funcIsStatic;
  }

  bool hasTemplate() {
    return name.contains("<");
  }
  
  bool isMember() {
    return funcIsMember;
  }

  bool hasPointerToPointerArg() {
    return ptrToPtrArg;
  }

  BasicBlock *getFirstBb();

  uint64_t getCount() {
    return cachedCount;
  }

};

#endif
