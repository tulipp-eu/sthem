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

#ifndef CFGSCENE_H
#define CFGSCENE_H

#include <QGraphicsScene>
#include <QtWidgets>

#include "textview.h"
#include "container.h"

class CfgScene : public QGraphicsScene {
  Q_OBJECT

  unsigned zvalue;
  TextView *textView;
  QComboBox *colorBox;
  Container *lastElement;
  QVector<BasicBlock*> lastCallStack;

public:
  explicit CfgScene(QComboBox *colorBox, TextView *textView, QObject *parent = 0);
  ~CfgScene() {}
  void drawElement(Container *element, QVector<BasicBlock*> callStack = QVector<BasicBlock*>());
  void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent);
  void redraw() {
    if(lastElement) {
      drawElement(lastElement, lastCallStack);
    }
  }
  void clearScene() {
    lastElement = NULL;
    clear();
    update();
  }
};

#endif
