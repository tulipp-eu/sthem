
/******************************************************************************
 *
 *  This file is part of the TULIPP Analysis Utility
 *
 *  Copyright 2018 Fatemeh Ghasemi, NTNU, TULIPP EU Project
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


#ifndef CONFIGDIALOG_H2
#define CONFIGDIALOG_H2

#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;

class ConfigDialog2 : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog2();

private:

    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;
};

class UpdatePage : public QWidget
{
public:
    UpdatePage(QWidget *parent = 0);
    void startUpdate();
};

#endif
