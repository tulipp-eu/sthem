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

#ifndef CONTAINER_H
#define CONTAINER_H

#include <assert.h>
#include <unordered_set>

#include "analysis_tool.h"
#include "vertex.h"
#include "exit.h"
#include "entry.h"

class Loop;
class Instruction;

///////////////////////////////////////////////////////////////////////////////

class Container : public Vertex {

  std::vector<std::vector<Vertex*>*> layers;

  std::vector<unsigned> currentRoutingXs;
  std::vector<unsigned> currentRoutingYs;

  static unsigned exitNodeCounter;

  std::map<QString,Vertex*> idVertexCache;

  void layering();
  void layerOrdering();
  void displayEdge(Edge *edge, unsigned edgeNum);
  void cycleRemoval(Vertex *baseNode, Vertex *node, std::unordered_set<Vertex*> &visitedNodes);

protected:
  bool expanded; // this container is expanded and displays its children

  ProfLine *cachedProfLine[LYNSYN_MAX_CORES];
  double cachedRuntime;
  double cachedEnergy[LYNSYN_SENSORS];

public:
  std::vector<Vertex*> children;
  std::vector<Entry*> entries;
  std::vector<Exit*> exits;

  unsigned treeviewRow;

  Container(QString id, QString name, Container *parent, unsigned treeviewRow, QString sourceFilename = "", unsigned sourceLineNumber = 1, unsigned sourceColumn = 1) : Vertex(id, name, parent, sourceFilename, sourceLineNumber, sourceColumn) {
    expanded = false;
    this->treeviewRow = treeviewRow;
    for(unsigned i = 0; i < LYNSYN_MAX_CORES; i++) {
      cachedProfLine[i] = NULL;
    }
    cachedRuntime = INT_MAX;
    for(unsigned i = 0; i < LYNSYN_SENSORS; i++) {
      cachedEnergy[i] = INT_MAX;
    }
  }

  virtual ~Container();

  //---------------------------------------------------------------------------
  // information about this vertex

  virtual QString getSourceFilename() {
    if(sourceFilename != "") return sourceFilename;

    for(auto child : children) {
      if(child->getSourceFilename() != "") {
        return child->getSourceFilename();
      }
    }

    return "";
  }

  virtual std::vector<unsigned> getSourceLineNumber() {
    std::vector<unsigned> lineNumbers;

    if(sourceFilename != "") lineNumbers.push_back(sourceLineNumber);

    for(auto child : children) {
      if(child->getSourceFilename() == getSourceFilename()) {
        std::vector<unsigned> childNumbers = child->getSourceLineNumber();
        lineNumbers.insert(lineNumbers.end(), childNumbers.begin(), childNumbers.end());
      }
    }

    return lineNumbers;
  }

  virtual bool isContainer() {
    return expanded;
  }

  virtual QString getTypeName() = 0;

  virtual void print(unsigned indent = 0);

  virtual void printLayers();

  virtual void getAllLoops(QVector<Loop*> &loops, QVector<BasicBlock*> callStack);

  virtual bool hasHwCalls();

  //---------------------------------------------------------------------------
  // graph building


  virtual int constructFromXml(const QDomElement &element, int treeviewRow);

  virtual void appendChild(Vertex *e);
  
  virtual unsigned getNumContainerChildren() {
    return children.size();
  }

  virtual unsigned getNumEdges() {
    return edges.size() + exits.size();
  }

  virtual Edge *getEdge(unsigned i, Vertex **source = NULL);

  virtual void buildEdgeList();

  virtual Vertex *getLocalVertex(Vertex *v);

  virtual void cycleRemoval();

  virtual void buildEntryNodes();

  virtual void buildExitNodes();

  virtual void calculateCallers() {
    for(auto child : children) {
      child->calculateCallers();
    }
  }

  //---------------------------------------------------------------------------
  // graphics

  virtual void appendLocalItems(int xx, int yy, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling);

  virtual void setLineSegmentsSource(unsigned n, std::vector<LineSegment>lines);

  virtual void clearColors() {
    for(auto edge : edges) {
      edge->color = -1;
    }
    for(auto child : entries) {
      child->clearColors();
    }
    for(auto child : children) {
      child->clearColors();
    }
    for(auto child : exits) {
      child->clearColors();
    }
  }

  virtual void toggleExpanded() {
    expanded = !expanded;
  }

  virtual void collapse();

  //---------------------------------------------------------------------------
  // profiling data
  
  virtual void getProfData(unsigned core, QVector<BasicBlock*> callStack, double *runtime, double *energy, QVector<Measurement> *measurements = NULL);

  virtual void buildProfTable(unsigned core, std::vector<ProfLine*> &table, bool forModel = false);

  virtual void clearCachedProfilingData() {
    for(auto child : children) {
      child->clearCachedProfilingData();
    }
    for(unsigned i = 0; i < LYNSYN_MAX_CORES; i++) {
      cachedProfLine[i] = NULL;
    }
    cachedRuntime = INT_MAX;
    for(unsigned i = 0; i < LYNSYN_SENSORS; i++) {
      cachedEnergy[i] = INT_MAX;
    }
  }

  //---------------------------------------------------------------------------
  // HLS compatibility

  virtual QVector<AnalysisInfo> getRecursiveFunctions(QVector<BasicBlock*> callStack) {
    QVector<AnalysisInfo> recursiveCalls;

    for(auto child : children) {
      recursiveCalls += child->getRecursiveFunctions(callStack);
    }

    return recursiveCalls;
  }

  virtual QVector<AnalysisInfo> getExternalCalls(QVector<BasicBlock*> callStack) {
    QVector<AnalysisInfo> externalCalls;

    for(auto child : children) {
      externalCalls += child->getExternalCalls(callStack);
    }

    return externalCalls;
  }

  virtual QVector<AnalysisInfo> getArraysWithPtrToPtr(QVector<BasicBlock*> callStack) {
    QVector<AnalysisInfo> arraysWithPtrToPtr;

    for(auto child : children) {
      arraysWithPtrToPtr += child->getArraysWithPtrToPtr(callStack);
    }

    return arraysWithPtrToPtr;
  }

  virtual bool hasComplexPtrCast(QVector<BasicBlock*> callStack) {
    for(auto child : children) {
      if(child->hasComplexPtrCast(callStack)) {
        return true;
      }
    }
    return false;
  }

};

#endif
