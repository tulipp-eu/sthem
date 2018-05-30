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

#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include <QGraphicsScene>
#include <QPlainTextEdit>
#include <QFile>
#include <QTextStream>
#include <QTextBlock>

#include "cfg/container.h"

class TextView : public QPlainTextEdit {
  Q_OBJECT

  QString currentFile;

public:
  TextView(QObject *parent = 0) {
    setReadOnly(true);
  }
  ~TextView() {}

  void loadFile(QString filename, std::vector<unsigned> lineNumbers) {
    if(filename == "") {
      clear();
      return;

    } else if(currentFile != filename) {
      currentFile == filename;
      QFile file(filename);
      file.open(QIODevice::ReadOnly);
      QTextStream stream(&file);
      QString content = stream.readAll();
      file.close();
      setPlainText(content);
    }

    //std::sort(lineNumbers.begin(), lineNumbers.end());

    for(unsigned i : lineNumbers) {
      QTextCursor cursor(document()->findBlockByNumber(i-1));
      QTextBlockFormat blockFormat = cursor.blockFormat();
      blockFormat.setBackground(QColor(Qt::yellow));
      cursor.setBlockFormat(blockFormat);
      if(i == lineNumbers[0]) {
        setTextCursor(cursor);
        centerCursor();
      }
    }
  }
};

#endif
