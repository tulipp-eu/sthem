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

#ifndef VERTEX_H
#define VERTEX_H

#include <map>
#include <vector>
#include <string>
#include <stack>

#include <assert.h>

#include <QDomDocument>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QGraphicsPolygonItem>

#include "analysis_tool.h"
#include "config/config.h"
#include "edge.h"
#include "linesegment.h"
#include "analysis_tool.h"
#include "profile/profline.h"

class Cfg;
class Module;
class Container;
class Function;
class BasicBlock;
class Loop;
class Instruction;

///////////////////////////////////////////////////////////////////////////////

class AnalysisInfo {
public:
  QString text;
  QString sourceFilename;
  std::vector<unsigned> sourceLineNumbers;

  AnalysisInfo() {}

  AnalysisInfo(QString text, QString sourceFilename, std::vector<unsigned> sourceLineNumbers) {
    this->text = text;
    this->sourceFilename = sourceFilename;
    this->sourceLineNumbers = sourceLineNumbers;
  }
};

///////////////////////////////////////////////////////////////////////////////

class Vertex {
  std::vector<QString> edgeIds;

protected:

  std::stack<QGraphicsPolygonItem*> baseItems;
  std::vector<LineSegment> lineSegments;

public:

  QString sourceFilename;
  unsigned sourceLineNumber;
  unsigned sourceColumn;

  // parent container
  Container *parent;

  // id and name (often the same, but id should be unique)
  QString id;
  QString name;

  // position and size, valid between calls to appendItems() and closeItems()
  unsigned x, y;
  unsigned width, height;
  unsigned row, column;

  // list of edges
  std::vector<Edge*> edges;
  bool selfLoop; // this vertex has an edge to itself

  // this is used to hold the callstack for the visible toplevel item
  // need the callstack to get correct profile data
  QVector<BasicBlock*> callStack;

  bool acyclic;

  Vertex(QString id, QString name, Container *parent, QString sourceFilename = "", unsigned sourceLineNumber = 1, unsigned sourceColumn = 1) {
    this->id = id;
    this->name = name;
    this->parent = parent;
    this->sourceFilename = sourceFilename;
    this->sourceLineNumber = sourceLineNumber;
    this->sourceColumn = sourceColumn;
    selfLoop = false;
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    row = 0;
    column = 0;
    acyclic = false;
  }

  virtual ~Vertex() {
    for(auto edge : edges) {
      delete edge;
    }
  }

  //---------------------------------------------------------------------------
  // information about this vertex

  virtual QColor getColor() {
    return VERTEX_COLOR;
  }

  virtual QColor getColor(QVector<BasicBlock*> callStack) {
    return getColor();
  }

  virtual QString getSourceFilename() {
    return sourceFilename;
  }

  virtual std::vector<unsigned> getSourceLineNumber() {
    std::vector<unsigned> lineNumbers;
    lineNumbers.push_back(sourceLineNumber);
    return lineNumbers;
  }

  virtual bool isContainer() {
    return false;
  }

  virtual bool isEntry() {
    return false;
  }

  virtual bool isExit() {
    return false;
  }

  virtual bool isFunction() {
    return false;
  }

  virtual bool isLoop() {
    return false;
  }

  virtual bool isVisibleInTable() {
    return false;
  }

  virtual bool isVisibleInGantt() {
    return false;
  }

  virtual QString getTypeName() = 0;

  virtual QString getListViewName() {
    return name;
  }

  virtual QString getTableName() {
    return getTypeName() + " " + name;
  }

  virtual QString getGanttName() {
    return getListViewName();
  }

  virtual QString getCfgName() {
    return getListViewName();
  }

  virtual void print(unsigned in = 0) {
    indent(in);
    printf("%s %s\n", getTypeName().toUtf8().constData(), id.toUtf8().constData());
    for(auto edge : edges) {
      indent(in+2);
      printf("Edge to %s%s\n", edge->target->id.toUtf8().constData(), edge->isReversed ? " (reversed)" : "");
    }
  }

  virtual bool isHw();

  virtual bool isHw(QVector<BasicBlock*> callStack);

  virtual void getAllLoops(QVector<Loop*> &loops, QVector<BasicBlock*> callStack, bool recursive = true) {}

  virtual bool hasHwCalls() { return false; }

  //---------------------------------------------------------------------------
  // information about the rest of the graph

  // get module this vertex belongs to
  virtual Module *getModule();

  // get Function this vertex belongs to
  virtual Function *getFunction();

  // get toplevel vertex
  virtual Cfg *getTop();

  // get basic block representing frame done
  virtual BasicBlock *getFrameDoneBb() {
    return NULL;
  }

  // get nearest surrounding loop
  virtual Loop *getLoop();

  //---------------------------------------------------------------------------
  // graph building

  virtual void appendEdge(QString edgeId) {
    edgeIds.push_back(edgeId);
  }

  virtual unsigned getNumEdges() {
    return edges.size();
  }

  virtual Edge *getEdge(unsigned i, Vertex **source = NULL) {
    if(source) {
      *source = this;
    }
    assert(i < edges.size());
    return edges[i];
  }

  // build all edges from the list of edgeIds
  virtual void buildEdgeList();

  // returns the forefather of v that is a direct child of this vertex, else NULL
  virtual Vertex *getLocalVertex(Vertex *v) {
    return NULL;
  }

  // removes cycles by reversing back edges
  virtual void cycleRemoval() {}

  // reverse given edge
  virtual void reverseEdge(Edge *edge);

  virtual void buildEntryNodes() {}

  virtual void buildExitNodes() {}

  virtual void calculateCallers() {}

  //---------------------------------------------------------------------------
  // graphics

  // appends QGraphicsItems representing this element to the given parent
  virtual void appendItems(QGraphicsItem *parent, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling);

  virtual void appendLocalItems(int xx, int yy, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
    height = yy + LINE_CLEARANCE;
  }

  // cleanup after appendItems()
  virtual void closeItems() {
    baseItems.pop();
  }

  // return current baseitem
  virtual QGraphicsPolygonItem *getBaseItem() {
    assert(baseItems.size());
    return baseItems.top();
  }

  // set scene position relative to baseitem
  virtual void setPos(unsigned x, unsigned y) {
    if(getBaseItem()) getBaseItem()->setPos(x, y);
    this->x = x;
    this->y = y;
  }

  // set container layers position
  virtual void setRowCol(unsigned row, unsigned column) {
    this->row = row;
    this->column = column;
  }

  // associate line segments with this vertex (as source)
  virtual void setLineSegmentsSource(unsigned n, std::vector<LineSegment>lines) {
    setLineSegments(lines);
  }

  // associate line segments with this vertex (as target)
  virtual void setLineSegmentsTarget(std::vector<LineSegment>lines) {
    setLineSegments(lines);
  }

  // associate line segments with this vertex
  virtual void setLineSegments(std::vector<LineSegment>lines) {
    lineSegments.insert(lineSegments.end(), lines.begin(), lines.end());
  }

  // get all line segments associated with this vertex
  virtual std::vector<LineSegment> getLineSegments() {
    return lineSegments;
  }

  // get input x coordinate
  virtual unsigned getXIn() {
    return x + width/2;
  }

  // get input y coordinate
  virtual unsigned getYIn() {
    return y;
  }

  // get output x coordinate
  virtual unsigned getXOut(unsigned num) {
    return x + width/2;
  }

  // get output y coordinate
  virtual unsigned getYOut(unsigned num) {
    return y + height;
  }

  // reset colors of all edges, for this vertex and all children
  virtual void clearColors() {
    for(auto edge : edges) {
      edge->color = -1;
      edge->zvalue = 0;
    }
  }

  // collapse this vertex and all children
  virtual void collapse() {}

  //---------------------------------------------------------------------------
  // profiling data

  virtual void getProfData(unsigned core, QVector<BasicBlock*> callStack,
                           double *runtime, double *energy, double *runtimeFrame, double *energyFrame, uint64_t *count) {
    *runtime = 0;
    for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
      energy[i] = 0;
    }
  }

  virtual void getMeasurements(unsigned core, QVector<BasicBlock*> callStack, QVector<Measurement> *measurements) {}

  virtual void buildProfTable(unsigned core, std::vector<ProfLine*> &table, bool forModel = false) {}

  virtual void clearCachedProfilingData() {}

  virtual uint64_t getCount();

  //---------------------------------------------------------------------------
  // HLS compatibility

  virtual QVector<AnalysisInfo> getRecursiveFunctions(QVector<BasicBlock*> callStack) {
    return QVector<AnalysisInfo>();
  }

  virtual QVector<AnalysisInfo> getExternalCalls(QVector<BasicBlock*> callStack) {
    return QVector<AnalysisInfo>();
  }

  virtual bool memFuncConst() {
    // TODO
    return true;
  }

  virtual QVector<AnalysisInfo> getArraysWithPtrToPtr(QVector<BasicBlock*> callStack) {
    return QVector<AnalysisInfo>();
  }

  virtual bool hasComplexPtrCast(QVector<BasicBlock*> callStack) {
    return false;
  }

};

#endif
