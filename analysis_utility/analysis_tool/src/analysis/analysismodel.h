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

#ifndef ANALYSISMODEL_H
#define ANALYSISMODEL_H

#include <map>
#include <vector>
#include <string>
#include <QDomDocument>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QGraphicsPolygonItem>

class AnalysisElement {
public:
  QString text;
  AnalysisElement *parent;
  QVector<AnalysisElement*> children;
  QString sourceFilename;
  std::vector<unsigned> sourceLineNumbers;

  AnalysisElement(AnalysisElement *parent, QString text, QString sourceFilename, std::vector<unsigned> sourceLineNumbers) {
    this->parent = parent;
    this->text = text;
    this->sourceFilename = sourceFilename;
    this->sourceLineNumbers = sourceLineNumbers;
  }

  ~AnalysisElement() {
    for(auto child : children) {
      delete child;
    }
  }

  unsigned numChildren() { return children.size(); }
  unsigned getRow() {
    if(parent) {
      return parent->getRow(this);
    } else {
      return 0;
    }
  }
  unsigned getRow(AnalysisElement *el) {
    return children.indexOf(el);
  }
};

class AnalysisModel : public QAbstractItemModel {
  Q_OBJECT

  AnalysisElement *top;
  QString vertexName;

public:  
  AnalysisModel(QString vertexName, QObject *parent = 0) {
    this->vertexName = vertexName;
    top = new AnalysisElement(NULL, "Top", "", std::vector<unsigned>());
  }
  ~AnalysisModel() {
    delete top;
  }

  QModelIndex index(int treeviewRow, int column, const QModelIndex &parent) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;

  unsigned addType(QString text) {
    unsigned index = top->numChildren();
    top->children.push_back(new AnalysisElement(top, text, "", std::vector<unsigned>()));
    return index;
  }

  void addLine(QString text, QString sourceFilename, std::vector<unsigned> sourceLineNumber) {
    addLine(-1, text, sourceFilename, sourceLineNumber);
  }

  void addLine(int typeNum, QString text, QString sourceFilename, std::vector<unsigned> sourceLineNumber) {
    if(typeNum < 0) {
      top->children.push_back(new AnalysisElement(top, text, sourceFilename, sourceLineNumber));
    } else {
      AnalysisElement *type = top->children[typeNum];
      type->children.push_back(new AnalysisElement(type, text, sourceFilename, sourceLineNumber));
    }
  }

};

#endif
