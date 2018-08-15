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

#include "graphscene.h"
#include "profmodel.h"

#define GANTT_SPACING 20
#define GANTT_SIZE (scaleFactorPower + GANTT_SPACING)

GraphScene::GraphScene(QObject *parent) : QGraphicsScene(parent) {
  scaleFactorTime = 1000;
  scaleFactorPower = 100;
  profile = NULL;
  graph = NULL;
}

void GraphScene::addLineSegments(int line, QVector<Measurement> *measurements) {
  int64_t startTime = ~0;
  int64_t lastTime = ~0;
  uint64_t lastSeq = ~0;
  for(auto measurement : *measurements) {
    int64_t time = measurement.time;
    uint64_t seq = measurement.sequence;
    if(seq != lastSeq+1) {
      if(startTime != (int64_t)~0) {
        addLineSegment(line, startTime, lastTime);
      }
      startTime = time;
    }
    lastTime = time;
    lastSeq = seq;
  }
  if(measurements->size()) {
    addLineSegment(line, startTime, lastTime);
  }
}

void GraphScene::drawProfile(unsigned core, unsigned sensor, Cfg *cfg, Profile *profile, int64_t beginTime, int64_t endTime) {
  clear();
  lines.clear();

  this->currentCore = core;
  this->currentSensor = sensor;
  this->cfg = cfg;
  this->profile = profile;

  QVector<Measurement> *measurements = new QVector<Measurement>;

  if(profile) {
    QSqlQuery query;

    QString sensorString = QString::number(sensor+1);

    cfg->clearCachedProfilingData();

    query.exec(QString() + "SELECT mintime,maxtime,minpower" + sensorString + ",maxpower" + sensorString + ",samples FROM meta");

    if(query.next()) {
      if(beginTime < 0) minTime = query.value(0).toLongLong();
      else minTime = beginTime;

      if(endTime < 0) maxTime = query.value(1).toLongLong();
      else maxTime = endTime;

      minPower = query.value(2).toDouble();
      maxPower = query.value(3).toDouble();

      graph = new Graph(font(), minPower, maxPower);
      graph->setPos(0, GANTT_SIZE-GANTT_SPACING);
      addItem(graph);

      unsigned ticksPerSample = (query.value(1).toLongLong() - query.value(0).toLongLong()) / query.value(4).toULongLong();
      int64_t ticks = maxTime - minTime;
      uint64_t samples = ticks / ticksPerSample;
      int stride = samples / scaleFactorTime;
      if(stride < 1) stride = 1;

      QString queryString = QString() +
        "SELECT time,basicblock" + QString::number(core+1) +
        ",module" + QString::number(core+1) +
        ",power" + QString::number(sensor+1) +
        " FROM measurements" +
        " WHERE time BETWEEN " + QString::number(minTime) + " AND " + QString::number(maxTime) + 
        " AND rowid % " + QString::number(stride) + " = 0";

      query.exec(queryString);

      MovingAverage ma(Config::window);

      if(query.next()) {
        ma.initialize(query.value("power" + QString::number(sensor+1)).toDouble());
        
        do {
          int64_t time = query.value("time").toLongLong();
          double power = query.value("power" + QString::number(sensor+1)).toDouble();
          QString moduleId = query.value("module" + QString::number(core+1)).toString();
          QString bbId = query.value("basicblock" + QString::number(core+1)).toString();

          Module *mod = cfg->getModuleById(moduleId);
          assert(mod);
          BasicBlock *bb = mod->getBasicBlockById(bbId);

          double avg = ma.next(power);

          addPoint(time, avg);

          measurements->push_back(Measurement(time, core, bb));
          
        } while(query.next());
      }

      profile->setMeasurements(measurements);

      std::vector<ProfLine*> table;
      cfg->buildProfTable(core, table);
      ProfSort profSort;
      std::sort(table.begin(), table.end(), profSort);

      for(auto profLine : table) {
        if(!profLine->vertex) {
          int l = addLine("Unknown", Qt::black);
          addLineSegments(l, profLine->getMeasurements());
      
        } else {
          Vertex *vertex = profLine->vertex;

          if(vertex->isVisibleInGantt() && (profLine->measurements.size() > 0)) {
            int l = addLine(vertex->getGanttName(), vertex->getColor());
            addLineSegments(l, profLine->getMeasurements());
          }
        }
      }
    }
  }

  update();
}

void GraphScene::redraw() {
  drawProfile(currentCore, currentSensor, cfg, profile, minTime, maxTime);
}

void GraphScene::redrawFull() {
  drawProfile(currentCore, currentSensor, cfg, profile);
}

int GraphScene::addLine(QString id, QColor color) {
  int lineNum = lines.size();

  GanttLine *line = new GanttLine(id, font(), color);
  line->setPos(0, GANTT_SIZE + (lineNum+1) * LINE_SPACING);
  addItem(line);
  lines.push_back(line);

  return lineNum;
}

void GraphScene::addLineSegment(unsigned lineNum, int64_t start, int64_t stop) {
  lines[lineNum]->addLine(scaleTime(start), scaleTime(stop));
}

void GraphScene::addPoint(int64_t time, double value) {
  graph->addPoint(scaleTime(time), scalePower(value));
}

int64_t GraphScene::scaleTime(int64_t time) {
  return (int64_t)(scaleFactorTime * (double)(time - minTime) / (double)(maxTime - minTime));
}

double GraphScene::scalePower(double power) {
  return scaleFactorPower * (double)(power - minPower) / (double)(maxPower - minPower);
}

int64_t GraphScene::posToTime(double pos) {
  return (pos / scaleFactorTime) * (maxTime-minTime) + minTime;
}
