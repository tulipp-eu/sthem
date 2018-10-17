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

#include <assert.h>
#include <QBrush>
#include <QGraphicsRectItem>
#include <QPen>

#include "vertex.h"
#include "container.h"
#include "function.h"
#include "module.h"
#include "cfg.h"
#include "dummy.h"
#include "region.h"
#include "textitem.h"

void Vertex::reverseEdge(Edge *edge) {
  Vertex *target = edge->target;
  Vertex *source = edge->source;

  if((source->parent != this) || (target->parent != this)) {
    // this edge goes between hierarchical blocks
    // don't want to mess with the internals of another hierarchical block
    // insert two dummy nodes instead and break cycle there
    assert(dynamic_cast<Container*>(this));

    Container *myself = static_cast<Container*>(this);

    Dummy *dummy1 = new Dummy(source, target, myself);
    Dummy *dummy2 = new Dummy(source, target, myself);

    myself->appendChild(dummy1);
    myself->appendChild(dummy2);

    Edge *reverseEdge = new Edge(dummy1, dummy2, true);
    Edge *targetEdge = new Edge(dummy1, target);

    edge->target = dummy2;
    dummy1->edges.push_back(targetEdge);
    dummy1->edges.push_back(reverseEdge);

  } else {
    // edge between two local basic blocks
    edge->toggleReversed();
    edge->source = target;
    edge->target = source;
    target->edges.push_back(edge);
    source->edges.erase(std::find(source->edges.begin(), source->edges.end(), edge));
  }

}

void Vertex::appendItems(QGraphicsItem *parent, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
  unsigned xx = 0;
  unsigned yy = 0;

  lineSegments.clear();

  width = 0;
  height = 0;

  // create base item (rectangle)
  baseItems.push(new QGraphicsPolygonItem(parent));
  getBaseItem()->setPos(QPointF(x, y));
  getBaseItem()->setData(0, QVariant::fromValue((void*)this));
  getBaseItem()->setData(1, makeQVariant(callStack));

  double profData = 0;

  double runtimeTop;
  double energyTop[Pmu::MAX_SENSORS];
  uint64_t countTop;

  double runtime;
  double power[Pmu::MAX_SENSORS], energy[Pmu::MAX_SENSORS];
  uint64_t count;

  visualTop->getProfData(Config::core, visualTop->callStack, &runtimeTop, energyTop, &countTop);

  double powerMin = getTop()->getProfile()->getMinPower(Config::sensor);
  double powerMax = getTop()->getProfile()->getMaxPower(Config::sensor);

  getProfData(Config::core, callStack, &runtime, energy, &count);
  runtime *= scaling;
  energy[Config::sensor] *= scaling;
  if(runtime) power[Config::sensor] = energy[Config::sensor] / runtime;
  else power[Config::sensor] = 0;
  count *= scaling;

  assert(runtimeTop >= runtime);
  assert(energyTop[0] >= energy[0]);
  assert(energyTop[1] >= energy[1]);
  assert(energyTop[2] >= energy[2]);
  assert(energyTop[3] >= energy[3]);
  assert(energyTop[4] >= energy[4]);
  assert(energyTop[5] >= energy[5]);
  assert(energyTop[6] >= energy[6]);

  // if(runtimeTop < runtime) runtimeTop = runtime;
  // for(int i = 0; i < 7; i++) {
  //   if(energyTop[i] < energy[i]) energyTop[i] = energy[i];
  // }

  switch(Config::colorMode) {
    case Config::STRUCT:
      getBaseItem()->setBrush(getColor(callStack));
      break;
    case Config::RUNTIME:
      profData = runtime;
      if(runtimeTop) {
        int scale = 100*runtime/runtimeTop;
        if(!scale) scale = 1;
        getBaseItem()->setBrush(RUNTIME_COLOR.lighter(scale));
      } else {
        getBaseItem()->setBrush(Qt::black);
      }
      break;
    case Config::POWER:
      profData = power[Config::sensor];
      if(power[Config::sensor]) {
        int scale = 100*(power[Config::sensor]-powerMin)/(powerMax-powerMin);
        if(!scale) scale = 1;
        getBaseItem()->setBrush(POWER_COLOR.lighter(scale));
      } else {
        getBaseItem()->setBrush(Qt::black);
      }
      break;
    case Config::ENERGY:
      profData = energy[Config::sensor];
      if(energyTop[Config::sensor]) {
        int scale = 100*energy[Config::sensor]/energyTop[Config::sensor];
        if(!scale) scale = 1;
        getBaseItem()->setBrush(ENERGY_COLOR.lighter(scale));
      } else {
        getBaseItem()->setBrush(Qt::black);
      }
      break;
    default:
      getBaseItem()->setBrush(BACKGROUND_COLOR);
      break;
  }
    
  // create type/name text
  xx = TEXT_CLEARANCE;
  yy += TEXT_CLEARANCE;

  TextItem *text = new TextItem(getCfgName(), getBaseItem());
  text->setPos(QPointF(xx, yy));
  text->setData(0, QVariant::fromValue((void*)this));
  text->setData(1, makeQVariant(callStack));

  unsigned textwidth = text->boundingRect().width() + TEXT_CLEARANCE*2;
  if(textwidth > width) width = textwidth;
  yy += text->boundingRect().height();

  if(Config::includeId) {
    // create id text
    xx = TEXT_CLEARANCE;
    yy += TEXT_CLEARANCE;
    text = new TextItem(id, getBaseItem());
    text->setPos(QPointF(xx, yy));
    text->setData(0, QVariant::fromValue((void*)this));
    text->setData(1, makeQVariant(callStack));
    textwidth = text->boundingRect().width() + TEXT_CLEARANCE*2;
    if(textwidth > width) width = textwidth;
    yy += text->boundingRect().height();
  }

  if(Config::includeProfData && (Config::colorMode != Config::STRUCT) && (profData != 0)) {
    // create prof data text
    xx = TEXT_CLEARANCE;
    yy += TEXT_CLEARANCE;
    if(isFunction()) {
      text = new TextItem("Profile: " + QString::number(profData) + " Count: " + QString::number(count), getBaseItem());
    } else {
      text = new TextItem("Profile: " + QString::number(profData), getBaseItem());
    }
    text->setPos(QPointF(xx, yy));
    text->setData(0, QVariant::fromValue((void*)this));
    text->setData(1, makeQVariant(callStack));
    textwidth = text->boundingRect().width() + TEXT_CLEARANCE*2;
    if(textwidth > width) width = textwidth;
    yy += text->boundingRect().height();
  }

  QGraphicsPolygonItem *innerPoly = NULL;

  if(isContainer()) {
    innerPoly = new QGraphicsPolygonItem(getBaseItem());
    innerPoly->setPos(QPointF(LINE_CLEARANCE, yy + LINE_CLEARANCE));
    innerPoly->setData(0, QVariant::fromValue((void*)this));
    innerPoly->setData(1, makeQVariant(callStack));
    innerPoly->setBrush(BACKGROUND_COLOR);

    appendLocalItems(LINE_CLEARANCE, yy + LINE_CLEARANCE, visualTop, callStack, scaling);

  } else {
    appendLocalItems(0, yy, visualTop, callStack, scaling);
  }
    
  if(width % LINE_CLEARANCE) {
    width += LINE_CLEARANCE - (width % LINE_CLEARANCE);
  }

  // now the total node size is known, create the polygon for the node rectangle(s)

  if(isContainer()) {
    QPolygonF polygon;
    polygon << QPointF(0, 0)
            << QPointF(width, 0)
            << QPointF(width, height-yy-LINE_CLEARANCE)
            << QPointF(0, height-yy-LINE_CLEARANCE);
    innerPoly->setPolygon(polygon);
    width += 2*LINE_CLEARANCE;
    height += LINE_CLEARANCE;
  }

  QPolygonF polygon;
  polygon << QPointF(0, 0)
          << QPointF(width, 0)
          << QPointF(width, height)
          << QPointF(0, height);
  getBaseItem()->setPolygon(polygon);
}

Module *Vertex::getModule() {
  return parent->getModule();
}

Function *Vertex::getFunction() {
  return parent->getFunction();
}

Cfg *Vertex::getTop() {
  return parent->getTop();
}

void Vertex::buildEdgeList() {
  unsigned edgeNum = 0;

  for(auto edgeId : edgeIds) {
    if(this == getModule()->getVertexById(edgeId)) {
      this->selfLoop = true;
    } else {
      Edge *edge = new Edge(this, getModule()->getVertexById(edgeId), false, edgeNum);
      edges.push_back(edge);
    }
    edgeNum++;
  }
}

bool Vertex::isHw() {
  return getFunction()->isHw();
}

bool Vertex::isHw(QVector<BasicBlock*> callStack) {
  if(isHw()) return true;
  for(auto bb : callStack) {
    if(bb->isHw()) return true;
  }
  return false;
}

Loop *Vertex::getLoop() {
  if(parent) return parent->getLoop();
  else return NULL;
}
