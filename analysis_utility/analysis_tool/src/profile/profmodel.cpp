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

#include "profmodel.h"

ProfModel::ProfModel(unsigned core, Cfg *cfg, QObject *parent) : QAbstractTableModel(parent) {
  cfg->buildProfTable(core, table, true);
}

int ProfModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return table.size();
}

int ProfModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return 4;
}

QVariant ProfModel::data(const QModelIndex &index, int role) const {
  if (role == Qt::DisplayRole) {
    ProfLine *line = table[index.row()];
    if(index.column() == 0) {
      if(!line->vertex) {
        return "Unknown";
      } else {
        return line->vertex->getTableName();
      }
    }
    switch(index.column()-1) {
      case 0: return line->runtime;
      case 1: return line->power[Config::sensor];
      case 2: return line->energy[Config::sensor];
    }
  }
  return QVariant();
}

QVariant ProfModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch(section) {
      case 0: return QVariant("ID"); break;
      case 1: return QVariant("Runtime [s]"); break;
      case 2: return QVariant("Power [W]"); break;
      case 3: return QVariant("Energy [J]"); break;
    }
  }
  return QVariant();
}

void ProfModel::sort(int column, Qt::SortOrder order) {
  ProfSort profSort(column, order);
  std::sort(table.begin(), table.end(), profSort);
  emit dataChanged(createIndex(0, 0), createIndex(table.size()-1, 6));
}
