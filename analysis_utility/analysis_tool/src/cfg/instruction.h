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

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "analysis_tool.h"
#include "vertex.h"
#include "basicblock.h"

class Instruction : public Vertex {
public:
  QString target;
  bool recursive;
  bool isArray;
  bool arrayWithPtrToPtr;
  bool complexPtrCast;
  QString variable;

  Instruction(QString name, Container *parent,
              QString sourceFilename = "", unsigned sourceLineNumber = 1, unsigned sourceColumn = 1,
              QString variable = "", bool isArray=false, bool arrayWithPtrToPtr=false,
              bool complexPtrCast = false) : Vertex("", name, parent, sourceFilename, sourceLineNumber, sourceColumn) {
    recursive = false;
    this->variable = variable;
    this->isArray = isArray;
    this->arrayWithPtrToPtr = arrayWithPtrToPtr;
    this->complexPtrCast = complexPtrCast;
  }

  virtual ~Instruction() {}

  virtual QColor getColor() {
    return INSTRUCTION_COLOR;
  }

  virtual QString getTypeName() {
    return "Instruction";
  }

  virtual QString getCfgName() {
    if(name == INSTR_ID_CALL) {
      QStringList list = target.split("(");
      if(recursive) {
        return "Call " + list[0] + "() [recursive]";
      } else {
        return "Call " + list[0] + "()";
      }
    } else {
      return name;
    }
  }

  virtual void print(unsigned indent) {
    if(Config::includeAllInstructions) {
      Vertex::print(indent);
    }
  }

  virtual bool hasArrayWithPtrToPtr() {
    return arrayWithPtrToPtr;
  }

  virtual bool hasComplexPtrCast(QVector<BasicBlock*> callStack) {
    return complexPtrCast;
  }

  virtual bool hasHwCalls();

};

#endif
