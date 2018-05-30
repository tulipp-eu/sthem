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

double GraphScene::avgVector(double *window, int size) {
  double avg = 0;
  for(int i = 0; i < size; i++) {
    avg += window[i];
  }
  avg /= size;
  return avg;
}

void GraphScene::drawProfile(unsigned core, unsigned sensor, Cfg *cfg, Profile *profile) {
  clear();
  lines.clear();

  this->currentCore = core;
  this->currentSensor = sensor;
  this->cfg = cfg;
  this->profile = profile;

  if(profile) {
    if(profile->measurements.size()) {
      QVector<Point> points;

      minTime = LLONG_MAX;
      maxTime = 0;

      minPower = std::numeric_limits<double>::max();
      maxPower = 0;

      MovingAverage ma(Config::window);
      ma.initialize(profile->measurements[0].power[sensor]);

      for(int i = 0; i < profile->measurements.size(); i++) {
        auto measurement =  profile->measurements[i];
        double avg = ma.next(measurement.power[sensor]);
        points.push_back(Point(measurement.time, avg));
      }

      for(auto it : points) {
        if(it.power > maxPower) maxPower = it.power;
        if(it.power < minPower) minPower = it.power;
        if(it.time > maxTime) maxTime = it.time;
        if(it.time < minTime) minTime = it.time;
      }

      if(points.size()) {
        graph = new Graph(font(), minPower, maxPower);
        graph->setPos(0, GANTT_SIZE-GANTT_SPACING);
        addItem(graph);

        for(auto it : points) {
          addPoint(it.time, it.power);
        }

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
  }

  update();
}

void GraphScene::redraw() {
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

