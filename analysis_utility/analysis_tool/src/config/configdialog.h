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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QtWidgets>
#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;

///////////////////////////////////////////////////////////////////////////////

class MainPage : public QWidget {
public:
  QLineEdit *workspaceEdit;

  MainPage(QWidget *parent = 0);
};

///////////////////////////////////////////////////////////////////////////////

class BuildPage : public QWidget {
public:
  QLineEdit *clangEdit;
  QLineEdit *clangppEdit;
  QLineEdit *llcEdit;
  QLineEdit *llvm_ir_parserEdit;
  QLineEdit *tulipp_source_toolEdit;

  QLineEdit *asEdit;
  QLineEdit *linkerEdit;
  QLineEdit *linkerppEdit;

  QLineEdit *asUsEdit;
  QLineEdit *linkerUsEdit;
  QLineEdit *linkerppUsEdit;

  BuildPage(QWidget *parent = 0);
};

///////////////////////////////////////////////////////////////////////////////

class CfgPage : public QWidget {
public:
  QCheckBox *allInstructionsCheckBox;
  QCheckBox *profDataCheckBox;
  QCheckBox *idCheckBox;

  CfgPage(QWidget *parent = 0);
};

///////////////////////////////////////////////////////////////////////////////

class ConfigDialog : public QDialog {
  Q_OBJECT

public:
  ConfigDialog();

protected:
  virtual void closeEvent(QCloseEvent *e);

public slots:
  void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
  MainPage *mainPage;
  BuildPage *buildPage;
  CfgPage *cfgPage;

  QListWidget *contentsWidget;
  QStackedWidget *pagesWidget;
};

#endif
