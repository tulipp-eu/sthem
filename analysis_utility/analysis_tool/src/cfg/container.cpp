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

#include <unordered_set>
#include <assert.h>
#include <queue>
#include <QBrush>
#include <QPen>

#include "vertex.h"
#include "function.h"
#include "basicblock.h"
#include "region.h"
#include "instruction.h"
#include "loop.h"

extern QColor edgeColors[];
unsigned Container::exitNodeCounter = 0;

void Container::cycleRemoval() {
  for(auto child : children) {
    child->cycleRemoval();
  }

  for(auto entryNode : entries) {
    std::unordered_set<Vertex*> visitedNodes;
    visitedNodes.insert(entryNode);
    cycleRemoval(this, entryNode, visitedNodes);
  }
}

void Container::cycleRemoval(Vertex *baseNode, Vertex *node, std::unordered_set<Vertex*> &visitedNodes) {
  if(!node->acyclic) {
    std::vector<Edge*> targets;

    for(unsigned i = 0; i < node->getNumEdges(); i++) {
      targets.push_back(node->getEdge(i));
    }

    for(auto edge : targets) {
      Vertex *target = getLocalVertex(edge->target);

      if(target) {
        if(visitedNodes.find(target) != visitedNodes.end()) {
          // we have a cycle
          baseNode->reverseEdge(edge);

        } else {
          visitedNodes.insert(target);
          cycleRemoval(this, target, visitedNodes);
          visitedNodes.erase(target);
        }
      }
    }

    node->acyclic = true;
  }
}

/******************************************************************************
 * longest path layering algorithm
 * builds the layers bottom-up (layer 0 is bottom layer)
 *****************************************************************************/
void Container::layering() {
  layers.clear();

  std::vector<Vertex*> unassignedNodes = children;
  std::vector<Vertex*> assignedNodes;
  std::vector<Vertex*> nodesBelowCurrent;

  unsigned currentLayer = 0;

  // --------------------------------------------------------------------------
  // layer 0 set to all exits

  layers.push_back(new std::vector<Vertex*>);

  for(auto node : exits) {
    node->setRowCol(currentLayer, layers[currentLayer]->size());
    layers[currentLayer]->push_back(node);
    nodesBelowCurrent.push_back(node);
  }

  // new layer, we don't want more nodes in this layer
  currentLayer++;

  // --------------------------------------------------------------------------
  // layer 1 filled with nodes without outgoing edges

  {
    int unassigned = 0;
    for(auto node = unassignedNodes.begin(); node != unassignedNodes.end();) {
      if(!(*node)->getNumEdges()) {
        if(layers.size() != (currentLayer+1)) {
          layers.push_back(new std::vector<Vertex*>);
        }

        (*node)->setRowCol(currentLayer, layers[currentLayer]->size());

        layers[currentLayer]->push_back(*node);
        assignedNodes.push_back(*node);
        node = unassignedNodes.erase(node);

        unassigned++;
        if(unassigned >= MAX_UNCONNECTED_NODES_IN_A_ROW) {
          currentLayer++;
          nodesBelowCurrent.insert(nodesBelowCurrent.end(), assignedNodes.begin(), assignedNodes.end());
          assignedNodes.clear();
          unassigned = 0;
        }
      } else {
        node++;
      }
    }
  }

  // --------------------------------------------------------------------------
  // layers 1-n set to the rest of the nodes

  while(unassignedNodes.size()) {
    bool foundNode = false;
    // find node where all successors are in nodesBelowCurrent
    for(auto node = unassignedNodes.begin(); node != unassignedNodes.end();) {
      bool nodeIsEligible = true;
      // look through all outgoing edges of this node
      for(unsigned i = 0; i < (*node)->getNumEdges(); i++) {
        Vertex *successor = getLocalVertex((*node)->getEdge(i)->target);
        assert(successor);
        if(std::find(nodesBelowCurrent.begin(), nodesBelowCurrent.end(), successor) == nodesBelowCurrent.end()) {
          // outgoing edge is not going to a layer below this one, can't choose this node
          nodeIsEligible = false;
          break;
        }
      }
      if(nodeIsEligible) {
        // found eligible node
        foundNode = true;

        if(layers.size() != (currentLayer+1)) {
          layers.push_back(new std::vector<Vertex*>);
        }

        (*node)->setRowCol(currentLayer, layers[currentLayer]->size());

        layers[currentLayer]->push_back(*node);
        assignedNodes.push_back(*node);
        node = unassignedNodes.erase(node);
      } else {
        node++;
      }
    }
    if(!foundNode) {
      currentLayer++;
      nodesBelowCurrent.insert(nodesBelowCurrent.end(), assignedNodes.begin(), assignedNodes.end());
      assignedNodes.clear();
    }
  }

  currentLayer = layers.size()-1;

  // --------------------------------------------------------------------------
  // layer n+1 set to all entries

  currentLayer++;
  layers.push_back(new std::vector<Vertex*>);

  for(auto node : entries) {
    node->setRowCol(currentLayer, layers[currentLayer]->size());
    layers[currentLayer]->push_back(node);
  }
}

void Container::layerOrdering() {
  // TODO
}

/* recursive function that constructs the graph from XML DOM elements */
/* the DOM element represents one child of this object */
int Container::constructFromXml(const QDomElement &element, int treeviewRow, Project *project) {
  Container *child = this;

  // create child from DOM element
  QString childId = xmlPurify(element.attribute(ATTR_ID, ""));
  QString tagName = xmlPurify(element.tagName());
  QString file = xmlPurify(element.attribute(ATTR_FILE, ""));
  unsigned line = element.attribute(ATTR_LINE, "0").toUInt();
  unsigned col = element.attribute(ATTR_COLUMN, "0").toUInt();

  if(tagName == TAG_FUNCTION) {
    bool isStatic = element.attribute(ATTR_ISSTATIC, "false") != "false";
    bool isMember = element.attribute(ATTR_ISMEMBER, "false") != "false";
    bool ptrToPtrArg = element.attribute(ATTR_PTRTOPTRARG, "false") != "false";

    Function *func = new Function(childId, this, treeviewRow++, file, line, isStatic, isMember, ptrToPtrArg);
    appendChild(func);
    child = func;

  } else if(tagName == TAG_BASICBLOCK) {
    bool isEntry = element.attribute(ATTR_ENTRY, "false") != "false";
    child = new BasicBlock(childId, this, treeviewRow++);
    if(isEntry) {
      Vertex *p = this;
      while(p) {
        Function *f = dynamic_cast<Function*>(p);
        if(f) {
          f->entryNode->appendEdge(childId);
          break;
        }
        p = p->parent;
      }
    }
    appendChild(child);

  } else if(tagName == TAG_REGION) {
    bool isSuperBb = element.attribute(ATTR_SUPERBB, "false") != "false";
    child = new Region(childId, this, treeviewRow++, isSuperBb);
    appendChild(child);

  } else if(tagName == TAG_LOOP) {
    child = new Loop(childId, this, treeviewRow++, file, line, col);
    appendChild(child);

  } else if(tagName == TAG_INSTRUCTION) {
    QString variable = xmlPurify(element.attribute(ATTR_VARIABLE, ""));
    bool isArray = element.attribute(ATTR_ARRAY, "false") != "false";
    bool arrayWithPtrToPtr = element.attribute(ATTR_ARRAYWITHPTRTOPTR, "false") != "false";
    bool complexPtrCast = element.attribute(ATTR_COMPLEXPTRCAST, "false") != "false";

    Instruction *instr = new Instruction(childId, this, file, line, col, variable, isArray, arrayWithPtrToPtr, complexPtrCast);
    appendChild(instr);

    if(childId == INSTR_ID_CALL) {
      QString target = xmlPurify(element.attribute(ATTR_TARGET, ""));
      instr->target = target;

    } else if(childId == INSTR_ID_RET) {
      Vertex *p = this;
      while(p) {
        Function *f = dynamic_cast<Function*>(p);
        if(f) {
          appendEdge(f->exitNode->id);
          break;
        }
        p = p->parent;
      }
    }

  } else if(tagName == TAG_EDGE) {
    QString target = xmlPurify(element.attribute(ATTR_TARGET, ""));
    appendEdge(target);
  }

  // loop through all the DOM element children and construct grandchildren recursivly
  QDomNode n = element.firstChild();
  int i = 0;
  while(!n.isNull()) {
    QDomElement e = n.toElement();
    if(!e.isNull()) i = child->constructFromXml(e, i, project);
    n = n.nextSibling();
  }

  return treeviewRow;
}

void Container::buildEntryNodes() {
  std::vector<Edge*> entryEdges;

  for(auto node : entries) {
    Edge *edge = node->edges[0];
    if(getLocalVertex(edge->target)) {
      entryEdges.push_back(edge);
    }
  }
  for(auto node : children) {
    for(unsigned i = 0; i < node->getNumEdges(); i++) {
      Edge *edge = node->getEdge(i);
      if(getLocalVertex(edge->target)) {
        entryEdges.push_back(edge);
      }
    }
  }
  for(auto edge : entryEdges) {
    Vertex *target = edge->target;
    Container *localTarget = static_cast<Container*>(getLocalVertex(target));
    if((localTarget != target) && !(target->isEntry())) {
      // not local, build entry node
      Entry *entryNode = NULL;
      for(auto node : localTarget->entries) {
        if(node->edges[0]->target == target) {
          entryNode = node;
          break;
        }
      }
      if(!entryNode) {
        entryNode = new Entry(QString("entry") + QString::number(exitNodeCounter++), static_cast<Container*>(localTarget));
        Edge *entryEdge = new Edge(entryNode, target);
        entryNode->edges.push_back(entryEdge);
        localTarget->appendChild(entryNode);
      }

      edge->target = entryNode;
    }
  }
  for(auto child : children) {
    child->buildEntryNodes();
  }
}

void Container::buildExitNodes() {
  for(auto child : children) {
    child->buildExitNodes();
  }

  std::vector<Edge*> exitEdges;

  for(auto node : children) {
    for(unsigned i = 0; i < node->getNumEdges(); i++) {
      Edge *edge = node->getEdge(i);
      if(!getLocalVertex(edge->target)) {
        exitEdges.push_back(edge);
      }
    }
  }

  for(auto edge : exitEdges) {
    Vertex *target = edge->target;
    Exit *exitNode = NULL;
    for(auto node : exits) {
      if(node->edges[0]->target == target) {
        exitNode = node;
        break;
      }
    }
    if(!exitNode) {
      exitNode = new Exit(QString("exit") + QString::number(exitNodeCounter++), this);
      Edge *exitEdge = new Edge(exitNode, target);
      exitNode->edges.push_back(exitEdge);
      appendChild(exitNode);
    }
    edge->target = exitNode;
  }
}

void Container::printLayers() {
  printf("Layers of %s %s:\n", getTypeName().toUtf8().constData(), id.toUtf8().constData());
  for(auto layer = layers.rbegin(); layer != layers.rend(); layer++) {
    for(auto node : *(*layer)) {
      printf("  %s:%s ", node->getTypeName().toUtf8().constData(), node->id.toUtf8().constData());
    }
    printf("\n");
  }
}

void Container::appendLocalItems(int startX, int yy, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
  unsigned xx = startX + LINE_CLEARANCE;

  if(!expanded) {

    // entries
    if(entries.size()) {
      for(auto child : entries) {
        child->appendItems(getBaseItem(), visualTop, callStack, scaling);
        child->setPos(xx, yy);
        xx += child->width + LINE_CLEARANCE;
        child->closeItems();
      }
      if(xx > width) width = xx;
      yy += entries[0]->height + LINE_CLEARANCE;
      xx = startX + LINE_CLEARANCE;
    }
    
    // exits
    xx = 3*LINE_CLEARANCE/2;
    if(exits.size()) {
      for(auto child : exits) {
        child->appendItems(getBaseItem(), visualTop, callStack, scaling);
        child->setPos(xx, yy);
        xx += child->width + LINE_CLEARANCE;
        child->closeItems();
      }
      if(xx > width) width = xx;
      yy += exits[0]->height + LINE_CLEARANCE;
      xx = startX + 3*LINE_CLEARANCE/2;
    }

    height = yy + LINE_CLEARANCE;

  } else {

    //-----------------------------------------------------------------------------
    // build layers for this container

    if(!layers.size()) { // don't rebuild unnecessary
      // simplified sugiyama framework for graph layout
      layering();
      layerOrdering();
    }

    int rows = layers.size();

    //-----------------------------------------------------------------------------
    // build all vertex graphics items
    // need to do this now so that all vertex sizes are known during placement

    for(auto layer = layers.rbegin(); layer != layers.rend(); layer++) {
      for(auto vertex : *(*layer)) {
        vertex->appendItems(getBaseItem(), visualTop, callStack, scaling);
      }
    }

    //-----------------------------------------------------------------------------
    // calculate mesh positions for vertices and edges

    // find column widths
    std::vector<unsigned> columnWidths(0);
    for(auto layer = layers.rbegin(); layer != layers.rend(); layer++) {
      // this layer has more columns than previous layers, increase columnWidths vector
      if((*layer)->size() > columnWidths.size()) {
        columnWidths.resize((*layer)->size(), 0);
      }
      // go through all vertices in this layer and update columnWidths if necessary
      int i = 0;
      for(auto vertex : *(*layer)) {
        unsigned w = vertex->width;
        if(w > columnWidths[i]) columnWidths[i] = w + 2*LINE_CLEARANCE;
        i++;
      }
    }

    int columns = columnWidths.size();

    std::vector<unsigned> rowSpacing(rows, 0);
    std::vector<unsigned> columnSpacing(columns+1, 0);

    // find row and column spacing
    // this is based on the number of edges that must be routed here
    for(auto layer : layers) {
      for(auto vertex : *layer) {
        if(vertex->selfLoop) {
          unsigned row = getLocalVertex(vertex)->row;
          unsigned column = getLocalVertex(vertex)->column;

          rowSpacing[row+1] += LINE_CLEARANCE;
          rowSpacing[row] += LINE_CLEARANCE;
          columnSpacing[column] += LINE_CLEARANCE;
        }

        for(unsigned i = 0; i < vertex->getNumEdges(); i++) {
          Edge *edge = vertex->getEdge(i);
          Vertex *source = edge->source;
          Vertex *target = edge->target;

          if(getLocalVertex(target)) {
            unsigned sourceRow = getLocalVertex(source)->row;
            unsigned targetRow = getLocalVertex(target)->row;
            unsigned targetColumn = getLocalVertex(target)->column;

            if(edge->isReversed) {
              sourceRow++;
              targetRow--;
            }

            rowSpacing[sourceRow] += LINE_CLEARANCE;

            if((sourceRow - targetRow) > 1) {
              rowSpacing[targetRow+1] += LINE_CLEARANCE;
              columnSpacing[targetColumn] += LINE_CLEARANCE;
            }
          }
        }
      }
    }
    columnSpacing[columns] = 0;

    // vertical edge routing corridors
    currentRoutingXs.clear();
    currentRoutingXs.push_back(startX + columnSpacing[0]);
    for(unsigned i = 1; i < columnWidths.size(); i++) {
      currentRoutingXs.push_back(currentRoutingXs[i-1] + columnSpacing[i] + columnWidths[i-1]);
    }

    // horizontal edge routing corridors
    currentRoutingYs.clear();
    currentRoutingYs.resize(layers.size(), 0);

    //-----------------------------------------------------------------------------
    // position vertices

    xx = startX + LINE_CLEARANCE + columnSpacing[0];
    yy += LINE_CLEARANCE;

    unsigned row = rows-1;

    for(auto layer = layers.rbegin(); layer != layers.rend(); layer++) {
      unsigned maxHeight = 0;

      unsigned col = 0;
      currentRoutingYs[row] = yy - LINE_CLEARANCE;

      for(auto vertex : *(*layer)) {
        unsigned w = vertex->width;
        unsigned h = vertex->height;

        unsigned xpos = xx;
        if(vertex->isEntry()) xpos = xx;
        if(vertex->isExit()) xpos = xx + LINE_CLEARANCE/2;

        vertex->setPos(xpos, yy);

        xx += columnSpacing[col+1] + columnWidths[col];
        if((xpos + w) > width) width = xpos + w;
        if(h > maxHeight) maxHeight = h;
        col++;
      }

      xx = startX + LINE_CLEARANCE + columnSpacing[0];
      yy += LINE_CLEARANCE + rowSpacing[row] + maxHeight;

      row--;
    }

    //-----------------------------------------------------------------------------
    // create and display edges

    for(auto layer : layers) {
      for(auto vertex : *layer) {

        unsigned edgeNum = 0;

        if(vertex->selfLoop) {
          Edge edge(vertex, vertex);
          edge.isReversed = true;
          displayEdge(&edge, edgeNum++);
        }
        
        for(unsigned i = 0; i < vertex->getNumEdges(); i++) {
          Edge *edge = vertex->getEdge(i);
          displayEdge(edge, edgeNum++);
        }
      }
    }

    width += LINE_CLEARANCE;
    height = yy + LINE_CLEARANCE;

    //-----------------------------------------------------------------------------
    // close items

    for(auto layer = layers.rbegin(); layer != layers.rend(); layer++) {
      for(auto vertex : *(*layer)) {
        vertex->closeItems();
      }
    }
  }
}

void Container::displayEdge(Edge *edge, unsigned edgeNum) {
  Vertex *source = edge->source;
  Vertex *target = edge->target;

  if(getLocalVertex(target)) {
    unsigned sourceX;
    unsigned sourceY;
    unsigned targetX;
    unsigned targetY;
    unsigned sourceRow = getLocalVertex(source)->row;
    unsigned targetRow = getLocalVertex(target)->row;
    unsigned targetColumn = getLocalVertex(target)->column;

    if(edge->isReversed) {
      sourceX = source->getXIn();
      sourceY = source->getYIn();
      targetX = target->getXOut(edge->sourceNum);
      targetY = target->getYOut(edge->sourceNum);
      sourceRow++;
      targetRow--;
    } else {
      sourceX = source->getXOut(edgeNum);
      sourceY = source->getYOut(edgeNum);
      targetX = target->getXIn();
      targetY = target->getYIn();
    }

    if(source != getLocalVertex(source)) {
      sourceX += getLocalVertex(source)->x;
      sourceY += getLocalVertex(source)->y;
    }
    if(target != getLocalVertex(target)) {
      targetX += getLocalVertex(target)->x;
      targetY += getLocalVertex(target)->y;
    }

    std::vector<LineSegment> lines;

    if((sourceRow - targetRow) > 1) {
      // edge is spanning more than one row
      unsigned currentRoutingX = currentRoutingXs[targetColumn];
      unsigned currentRoutingYSource = currentRoutingYs[sourceRow-1];
      unsigned currentRoutingYTarget = currentRoutingYs[targetRow];

      lines.push_back(LineSegment(edge, new QGraphicsLineItem(sourceX, sourceY, sourceX, currentRoutingYSource, getBaseItem())));
      lines.push_back(LineSegment(edge, new QGraphicsLineItem(sourceX, currentRoutingYSource, currentRoutingX, currentRoutingYSource, getBaseItem())));
      lines.push_back(LineSegment(edge, new QGraphicsLineItem(currentRoutingX, currentRoutingYSource, currentRoutingX, currentRoutingYTarget, getBaseItem())));
      lines.push_back(LineSegment(edge, new QGraphicsLineItem(currentRoutingX, currentRoutingYTarget, targetX, currentRoutingYTarget, getBaseItem())));
      lines.push_back(LineSegment(edge, new QGraphicsLineItem(targetX, currentRoutingYTarget, targetX, targetY, getBaseItem())));
      
      currentRoutingXs[targetColumn] -= LINE_CLEARANCE;
      currentRoutingYs[sourceRow-1] -= LINE_CLEARANCE;
      currentRoutingYs[targetRow] -= LINE_CLEARANCE;

    } else {
      // edge is between neighbouring rows
      unsigned currentRoutingY = currentRoutingYs[sourceRow-1];

      lines.push_back(LineSegment(edge, new QGraphicsLineItem(sourceX, sourceY, sourceX, currentRoutingY, getBaseItem())));
      lines.push_back(LineSegment(edge, new QGraphicsLineItem(sourceX, currentRoutingY, targetX, currentRoutingY, getBaseItem())));
      lines.push_back(LineSegment(edge, new QGraphicsLineItem(targetX, currentRoutingY, targetX, targetY, getBaseItem())));
      
      currentRoutingYs[sourceRow-1] -= LINE_CLEARANCE;
    }

    for(auto line : lines) {
      QPen pen = line.item->pen();
      if(edge->color == -1) {
        pen.setColor(FOREGROUND_COLOR);
      } else {
        pen.setColor(edgeColors[line.edge->color]);
      }
      line.item->setPen(pen);
      line.item->setZValue(line.edge->zvalue);
    }

    if(edge->isReversed) {
      target->setLineSegmentsSource(edge->sourceNum, lines);
      source->setLineSegmentsTarget(lines);

    } else {
      source->setLineSegmentsSource(edgeNum, lines);
      target->setLineSegmentsTarget(lines);
    }

  }
}

void Container::setLineSegmentsSource(unsigned n, std::vector<LineSegment>lines) {
  for(auto it : exits) {
    if(n < it->getNumEdges()) {
      it->setLineSegments(lines);
    }
    n -= it->getNumEdges();
  }
}

Container::~Container() {
  for(auto it : children) {
    delete it;
  }
  for(auto it : entries) {
    delete it;
  }
  for(auto it : exits) {
    delete it;
  }
  for(auto it : layers) {
    delete it;
  }
  for(unsigned i = 0; i < Pmu::MAX_CORES; i++) {
    if(cachedProfLine[i]) {
      delete cachedProfLine[i];
    }
  }
}

Vertex *Container::getLocalVertex(Vertex *v) {
  if(v->parent == this) {
    return v;
  }
  for(auto child : children) {
    if(child->getLocalVertex(v)) {
      return child;
    }
  }
  return NULL;
}

void Container::appendChild(Vertex *e) {
  Entry *entryNode = dynamic_cast<Entry*>(e);
  Exit *exitNode = dynamic_cast<Exit*>(e);

  if(exitNode) {
    exits.push_back(exitNode);
  } else if(entryNode) {
    entries.push_back(entryNode);
  } else {
    children.push_back(e);
  }

  getModule()->idToVertex[e->id] = e;
}
  
void Container::buildEdgeList() {
  Vertex::buildEdgeList();
  for(auto entryNode : entries) {
    entryNode->buildEdgeList();
  }
  for(auto child : children) {
    child->buildEdgeList();
  }
  for(auto exitNode : exits) {
    exitNode->buildEdgeList();
  }
}

Edge *Container::getEdge(unsigned i, Vertex **source) {
  if(i < edges.size()) {
    if(source) *source = this;
    return edges[i];
  }
  i -= edges.size();
  if(i < exits.size()) {
    if(source) *source = exits[i];
    return exits[i]->getEdge(0);
  }
  return NULL;
}

void Container::print(unsigned indent) {
  if(indent <= 80) {
    Vertex::print(indent);
    for(auto child : entries) {
      child->print(indent+2);
    }
    for(auto child : children) {
      child->print(indent+2);
    }
    for(auto child : exits) {
      child->print(indent+2);
    }
  }
}

void Container::collapse() {
  expanded = false;
  for(auto child : children) {
    child->collapse();
  }
}

void Container::getProfData(unsigned core, QVector<BasicBlock*> callStack, double *runtime, double *energy) {
  if(cachedRuntime == INT_MAX) {
    cachedRuntime = 0;
    for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
      cachedEnergy[i] = 0;
    }

    for(auto child : children) {
      double runtimeChild;
      double energyChild[Pmu::MAX_SENSORS];
      child->getProfData(core, callStack, &runtimeChild, energyChild);
      cachedRuntime += runtimeChild;
      for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
        cachedEnergy[i] += energyChild[i];
      }
    }
  }

  *runtime = cachedRuntime;
  for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
    energy[i] = cachedEnergy[i];
  }
}

void Container::getMeasurements(unsigned core, QVector<BasicBlock*> callStack, QVector<Measurement> *measurements) {
  for(auto child : children) {
    child->getMeasurements(core, callStack, measurements);
  }
}

void Container::buildProfTable(unsigned core, std::vector<ProfLine*> &table, bool forModel) {
  for(auto child : children) {
    child->buildProfTable(core, table, forModel);
  }

  if(!forModel || isVisibleInTable()) {
    if(!cachedProfLine[core]) {
      double runtime;
      double energy[Pmu::MAX_SENSORS];

      cachedProfLine[core] = new ProfLine();

      getProfData(core, QVector<BasicBlock*>(), &runtime, energy);
      getMeasurements(core, QVector<BasicBlock*>(), &(cachedProfLine[core]->measurements));

      double power[Pmu::MAX_SENSORS];
      for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
        power[i] = 0;
        if(runtime) power[i] = energy[i] / runtime;
      }

      cachedProfLine[core]->init(this, runtime, power, energy);
    }

    table.push_back(cachedProfLine[core]);
  }
}

void Container::getAllLoops(QVector<Loop*> &loops, QVector<BasicBlock*> callStack) {
  for(auto child : children) {
    if(child->isLoop()) {
      if(!isSystemFile(child->getSourceFilename())) {
        QString line = getLine(child->getSourceFilename(), child->sourceLineNumber, child->sourceColumn);
        if(line.startsWith("for") || line.startsWith("while") || line.startsWith("do")) {
          loops.push_back(static_cast<Loop*>(child));
        }
      }
    } else {
      child->getAllLoops(loops, callStack);
    }
  }
}

bool Container::hasHwCalls() {
  for(auto child : children) {
    if(child->hasHwCalls()) return true;
  }
  return false;
}

