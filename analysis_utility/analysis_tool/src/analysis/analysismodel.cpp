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

#include "analysismodel.h"

QModelIndex AnalysisModel::index(int treeviewRow, int column, const QModelIndex &parent) const {
  if (!hasIndex(treeviewRow, column, parent))
    return QModelIndex();

  AnalysisElement *parentEl;

  if (!parent.isValid())
    parentEl = top;
  else
    parentEl = static_cast<AnalysisElement*>(parent.internalPointer());

  AnalysisElement *childEl = static_cast<AnalysisElement*>(parentEl->children[treeviewRow]);
  return createIndex(treeviewRow, column, childEl);
}

QModelIndex AnalysisModel::parent(const QModelIndex &index) const {
  if(!index.isValid()) {
    return QModelIndex();
  }

  AnalysisElement *childEl = static_cast<AnalysisElement*>(index.internalPointer());
  AnalysisElement *parentEl = childEl->parent;

  if (parentEl == top) {
    return QModelIndex();
  }

  assert(parentEl);

  return createIndex(parentEl->getRow(), 0, parentEl);
}

int AnalysisModel::rowCount(const QModelIndex &parent) const {
  if(parent.isValid()) {
    AnalysisElement *container = static_cast<AnalysisElement*>(parent.internalPointer());
    return container->numChildren();
  } else {
    return top->numChildren();
  }
}

int AnalysisModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return 1;
}

QVariant AnalysisModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  AnalysisElement *el = static_cast<AnalysisElement*>(index.internalPointer());

  switch(index.column()) {
    case 0:
      return el->text;
  }

  return QVariant();
}

QVariant AnalysisModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch(section) {
      case 0: return QVariant("HLS Analysis for " + vertexName); break;
    }
  }
  return QVariant();
}

Qt::ItemFlags AnalysisModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return 0;

  return QAbstractItemModel::flags(index);
}

