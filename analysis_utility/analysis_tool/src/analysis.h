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

#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <map>
#include <fstream>

#include <QtWidgets>
#include <QTreeView>

#include "analysis_tool.h"

#include "project/project.h"
#include "project/customproject.h"
#include "project/sdsoc.h"
#include "cfg/cfgscene.h"
#include "profile/graphscene.h"
#include "cfg/vertex.h"
#include "profile/profmodel.h"
#include "cfg/cfgview.h"
#include "profile/graphview.h"
#include "textview.h"
#include "dse/dse.h"
#include "cfg/group.h"

class Analysis : public QObject {
  Q_OBJECT

public:
  Project *project;
  Profile *profile;
  Dse *dse;

  Analysis();
  ~Analysis();

  bool openProject(QString path, QString buildConfig);
  bool createProject(QString path);
  void closeProject();
  void loadProfFile(QString path);
  void load();
  void clean();
};

#endif
