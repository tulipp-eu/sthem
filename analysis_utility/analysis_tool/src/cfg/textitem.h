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

#ifndef TEXTITEM_H
#define TEXTITEM_H

#include <QGraphicsTextItem>
#include <QPainter>

class TextItem : public QGraphicsSimpleTextItem {

public:
  TextItem(const QString &text, QGraphicsPolygonItem *parent) : QGraphicsSimpleTextItem(text, parent) {
    if(parent->brush().color().lightness() >= 128) {
      setBrush(QBrush(Qt::black));
    } else {
      setBrush(QBrush(Qt::white));
    }
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
    //painter->setPen(Qt::NoPen);
    //painter->setBrush(poly->brush());
    //painter->drawRect(boundingRect());
    QGraphicsSimpleTextItem::paint(painter, o, w); 
  }   
};

#endif
