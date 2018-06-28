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

#ifndef CFGMODEL_H
#define CFGMODEL_H

#include <map>
#include <vector>
#include <string>
#include <QDomDocument>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QGraphicsPolygonItem>

#include "container.h"
#include "cfg.h"
#include "module.h"
#include "project/project.h"

class Project;

class CfgModel : public QAbstractItemModel {
  Q_OBJECT

public:  
  Cfg *top;

  CfgModel(QObject *parent = 0);
  ~CfgModel() {
    delete top;
  }

  QModelIndex index(int treeviewRow, int column, const QModelIndex &parent) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;

  void addModule(const QDomDocument &doc, Project &project);
  void clearColors();
  void collapseAll();
  Container *getMain();
  Cfg *getCfg() {
    return top;
  }
};

#endif
