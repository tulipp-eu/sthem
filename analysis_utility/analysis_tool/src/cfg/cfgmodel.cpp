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

#include "cfgmodel.h"

#include "basicblock.h"
#include "function.h"

QModelIndex CfgModel::index(int treeviewRow, int column, const QModelIndex &parent) const {
  if (!hasIndex(treeviewRow, column, parent))
    return QModelIndex();

  Container *parentEl;

  if (!parent.isValid())
    parentEl = top;
  else
    parentEl = static_cast<Container*>(parent.internalPointer());

  assert(treeviewRow < (int)parentEl->getNumContainerChildren());

  Container *childEl = static_cast<Container*>(parentEl->children[treeviewRow]);
  return createIndex(treeviewRow, column, childEl);
}

QModelIndex CfgModel::parent(const QModelIndex &index) const {
  if (!index.isValid())
    return QModelIndex();

  Container *childEl = static_cast<Container*>(index.internalPointer());
  Container *parentEl = childEl->parent;

  if (parentEl == top) {
    return QModelIndex();
  }

  return createIndex(parentEl->treeviewRow, 0, parentEl);
}

int CfgModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    Container *container = static_cast<Container*>(parent.internalPointer());
    return container->getNumContainerChildren();
  } else {
    return top->getNumContainerChildren();
  }
}

int CfgModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return 2;
}

QVariant CfgModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  Container *el = static_cast<Container*>(index.internalPointer());

  switch(index.column()) {
    case 0:
      return el->getTypeName();
    case 1:
      return el->getListViewName();
  }

  return QVariant();
}

QVariant CfgModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch(section) {
      case 0: return QVariant("Type"); break;
      case 1: return QVariant("Name"); break;
    }
  }
  return QVariant();
}

Qt::ItemFlags CfgModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return 0;

  return QAbstractItemModel::flags(index);
}

CfgModel::CfgModel(Cfg *top, QObject *parent) : QAbstractItemModel(parent) {
  this->top = top;
}

void CfgModel::clearColors() {
  top->clearColors();
}

void CfgModel::collapseAll() {
  top->collapse();
}

