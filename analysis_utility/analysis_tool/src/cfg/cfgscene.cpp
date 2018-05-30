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

#include "cfgscene.h"

#include <QTextCursor>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsEllipseItem>

extern QColor edgeColors[];

CfgScene::CfgScene(QComboBox *colorBox, TextView *textView, QObject *parent) : QGraphicsScene(parent) {
  this->colorBox = colorBox;
  this->textView = textView;
  zvalue = 1;
  lastElement = NULL;
}

void CfgScene::drawElement(Container *element, QVector<BasicBlock*> callStack) {
  lastElement = element;
  lastCallStack = callStack;

  clear();

  QGraphicsLineItem *item = new QGraphicsLineItem();
  item->setPos(0,0);
  element->callStack = callStack;
  element->appendItems(item, element, callStack, 1);
  element->setPos(0,0);

  addItem(item);

  setSceneRect(QRectF(0, 0, element->width, element->height));

  element->closeItems();
}

void CfgScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) {
  if(mouseEvent->button() == Qt::LeftButton) {
    QGraphicsItem *item = itemAt(mouseEvent->scenePos(), QTransform());
    Vertex *el = (Vertex*)item->data(0).value<void*>();
    if(el) {
      textView->loadFile(el->getSourceFilename(), el->getSourceLineNumber());

      std::vector<LineSegment> lines = el->getLineSegments();
      for(auto line : lines) {
        QPen pen = line.item->pen();

        line.edge->color = colorBox->currentIndex();
        line.edge->zvalue = zvalue++;
        pen.setColor(edgeColors[line.edge->color]);

        line.item->setPen(pen);
        line.item->setZValue(line.edge->zvalue);
      }
    }
  }
}

void CfgScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent) {
  if (mouseEvent->button() == Qt::LeftButton) {
    QGraphicsItem *item = itemAt(mouseEvent->scenePos(), QTransform());
    Vertex *el = (Vertex*)item->data(0).value<void*>();
    Container *cont = dynamic_cast<Container*>(el);
    if(cont) {
      cont->toggleExpanded();
      redraw();
    }
  }
}
