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

#ifndef GROUP_H
#define GROUP_H

#include "analysis_tool.h"
#include "container.h"

class Group : public Container {

public:
  Group(QString id, Container *parent) : Container(id, id, parent, 0) {
  }

  virtual ~Group() {
    children.clear();
  }

  virtual void appendChild(Vertex *e) {
    children.push_back(e);
  }

  virtual QColor getColor() {
    return GROUP_COLOR;
  }

  virtual QString getTypeName() {
    return name;
  }

  bool isHw() {
    return false;
  }
};

#endif
