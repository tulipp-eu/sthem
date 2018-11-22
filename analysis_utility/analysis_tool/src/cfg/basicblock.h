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

#ifndef BASICBLOCK_H
#define BASICBLOCK_H

#include "analysis_tool.h"
#include "container.h"
#include "cfg.h"
#include "profile/measurement.h"
#include "instruction.h"
#include "profile/profile.h"

class Profile;

class BasicBlock : public Container {

protected:
  virtual void appendLocalItems(int xx, int yy, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling);

public:
  bool frameDone;
  Entry *entryNode;
  std::vector<Exit*> exitNodes;

  BasicBlock(QString id, Container *parent, unsigned treeviewRow, bool frameDone = false) : Container(id, id, parent, treeviewRow) {
    entryNode = new Entry("bb" + id + "entry", this);
    this->frameDone = frameDone;
  }
  virtual ~BasicBlock() {
    delete entryNode;
    for(auto exitNode : exitNodes) {
      delete exitNode;
    }
  }

  virtual QString getCfgName();

  virtual int appendInstructions(QGraphicsItem *parent, int xx, int yy, unsigned *width,
                                 Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling);

  void appendEdge(QString edgeId) {
    Vertex::appendEdge(edgeId);
    exitNodes.push_back(new Exit("bb" + id + "exit" + exitNodes.size(), this));
  }

  virtual QColor getColor() {
    return BASICBLOCK_COLOR;
  }

  virtual QString getSourceFilename() {
    for(auto child : children) {
      if(child->getSourceFilename() != "") return child->getSourceFilename();
    }
    return "";
  }

  unsigned getNumContainerChildren() {
    return 0;
  }

  virtual bool isBasicBlock() {
    return true;
  }

  virtual QString getTypeName() {
    return "BasicBlock";
  }

  virtual unsigned getXIn() {
    return x + entryNode->getXIn();
  }
  virtual unsigned getYIn() {
    return y + entryNode->getYIn();
  }
  virtual unsigned getXOut(unsigned num) {
    return x + exitNodes[num]->getXOut(0);
  }
  virtual unsigned getYOut(unsigned num) {
    return y + exitNodes[num]->getYOut(0);
  }

  void setLineSegmentsSource(unsigned n, std::vector<LineSegment>lines) {
    exitNodes[n]->setLineSegments(lines);
  }
  void setLineSegmentsTarget(std::vector<LineSegment>lines) {
    entryNode->setLineSegments(lines);
  }
  virtual void clearColors() {
    Vertex::clearColors();
  }

  virtual void getProfData(unsigned core, QVector<BasicBlock*> callStack,
                           double *runtime, double *energy, double *runtimeFrame, double *energyFrame, uint64_t *count);

  virtual void getMeasurements(unsigned core, QVector<BasicBlock*> callStack, QVector<Measurement> *measurements);

  virtual void calculateCallers();

  virtual QVector<AnalysisInfo> getRecursiveFunctions(QVector<BasicBlock*> callStack);

  virtual QVector<AnalysisInfo> getExternalCalls(QVector<BasicBlock*> callStack);

  virtual QVector<AnalysisInfo> getArraysWithPtrToPtr(QVector<BasicBlock*> callStack);

  virtual bool hasComplexPtrCast(QVector<BasicBlock*> callStack);

  void getAllLoops(QVector<Loop*> &loops, QVector<BasicBlock*> callStack, bool recursive = true);

  bool containsFunctionCall(Function *func);

  virtual BasicBlock *getFrameDoneBb() {
    if(frameDone) return this;
    else return NULL;
  }

};

#endif
