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

#ifndef PROFMODEL_H
#define PROFMODEL_H

#include <map>
#include <vector>
#include <string>
#include <QDomDocument>
#include <QAbstractTableModel>
#include <QModelIndex>
#include <QGraphicsPolygonItem>

#include "cfg/container.h"
#include "cfg/cfg.h"
#include "cfg/module.h"
#include "profile.h"

class ProfSort {

private:
  int column;
  Qt::SortOrder order;

public:
  ProfSort() {
    this->column = -1;
    this->order = Qt::AscendingOrder;
  }
  ProfSort(int column, Qt::SortOrder order) {
    this->column = column;
    this->order = order;
  }
  bool operator() (ProfLine *i, ProfLine *j) {
    if(column == -1) {
      return i->measurements.size() > j->measurements.size();
    }

    if(order == Qt::AscendingOrder) {
      if(column == 0) {
        if(!i->vertex) {
          return true;
        } else if(!j->vertex) {
          return false;
        } else {
          return i->vertex->id > j->vertex->id;
        }
      }
      switch(column-1) {
        case 0: return i->runtime > j->runtime;
        case 1: return i->power[Config::sensor] > j->power[Config::sensor];
        case 2: return i->energy[Config::sensor] > j->energy[Config::sensor];
      }
    }
    if(order == Qt::DescendingOrder) {
      if(column == 0) {
        if(!i->vertex) {
          return false;
        } else if(!j->vertex) {
          return true;
        } else {
          return i->vertex->id < j->vertex->id;
        }
      }
      switch(column-1) {
        case 0: return i->runtime < j->runtime;
        case 1: return i->power[Config::sensor] < j->power[Config::sensor];
        case 2: return i->energy[Config::sensor] < j->energy[Config::sensor];
      }
    }
    return false;
  }
};

class ProfModel : public QAbstractTableModel {
  Q_OBJECT

  std::vector<ProfLine*> table;  

public:  
  ProfModel(unsigned core, Cfg *cfg, QObject *parent = 0);
  ~ProfModel() {}

  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
};

#endif
