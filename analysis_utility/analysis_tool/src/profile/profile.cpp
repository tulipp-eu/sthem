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

#include <QTextStream>
#include <QtWidgets>
#include <QTreeView>
#include <QMainWindow>

#include "profile.h"

Profile::Profile() {
  QSqlDatabase db = QSqlDatabase::database();
  db.setDatabaseName("profile.db3");
  db.open();

  QSqlQuery query;

  bool success = query.exec("CREATE TABLE IF NOT EXISTS core (core INT NOT NULL, time INT NOT NULL, pc INT, location INT, PRIMARY KEY(core, time))");
  assert(success);

  success = query.exec("CREATE TABLE IF NOT EXISTS sensor (sensor INT NOT NULL, time INT NOT NULL, timeSinceLast INT, current INT, power REAL, PRIMARY KEY(sensor, time))");
  assert(success);

  success = query.exec("CREATE TABLE IF NOT EXISTS location (id INT NOT NULL PRIMARY KEY, core INT NOT NULL, basicblock TEXT, function TEXT, module TEXT, runtime REAL, energy1 REAL, energy2 REAL, energy3 REAL, energy4 REAL, energy5 REAL, energy6 REAL, energy7 REAL)");
  assert(success);

  success = query.exec("CREATE TABLE IF NOT EXISTS meta (samples INT, mintime INT, maxtime INT, minpower1 REAL, minpower2 REAL, minpower3 REAL, minpower4 REAL, minpower5 REAL, minpower6 REAL, minpower7 REAL, maxpower1 REAL, maxpower2 REAL, maxpower3 REAL, maxpower4 REAL, maxpower5 REAL, maxpower6 REAL, maxpower7 REAL)");
  assert(success);
}

Profile::~Profile() {
  clear();
  QSqlDatabase db = QSqlDatabase::database();
  db.close();
}

void Profile::addMeasurement(Measurement measurement) {
  measurements.push_back(measurement);

  unsigned core = measurement.core;
  BasicBlock *bb = measurement.bb;
  std::vector<Measurement> *mments = NULL;

  auto it = measurementsPerBb[core].find(bb);

  if(it == measurementsPerBb[core].end()) {
    mments = new std::vector<Measurement>();
    measurementsPerBb[core][bb] = mments;
  } else {
    mments = it->second;
  }

  mments->push_back(measurement);
}

void Profile::clean() {
  clear();

  QSqlDatabase db = QSqlDatabase::database();
  QSqlQuery query = QSqlQuery(db);
  query.exec("DELETE FROM core");
  query.exec("DELETE FROM sensor");
  query.exec("DELETE FROM location");
  query.exec("DELETE FROM meta");
}

void Profile::setMeasurements(QVector<Measurement> *measurements) {
  for(unsigned core = 0; core < Pmu::MAX_CORES; core++) {
    measurementsPerBb[core].clear();
  }
  this->measurements.clear();
  for(auto measurement : *measurements) {
    addMeasurement(measurement);
  }
}

void Profile::getProfData(unsigned core, BasicBlock *bb, double *runtime, double *energy) {
  QSqlQuery query;
  QString queryString;

  if(bb->getTop()->externalMod == bb->getModule()) {
    queryString =
      QString() +
      "SELECT runtime,energy1,energy2,energy3,energy4,energy5,energy6,energy7" +
      " FROM location" +
      " WHERE core = " + QString::number(core) +
      " AND module = \"" + bb->getTop()->externalMod->id +
      "\" AND function = \"" + bb->getFunction()->id + "\"";

  } else {
    queryString =
      QString() +
      "SELECT runtime,energy1,energy2,energy3,energy4,energy5,energy6,energy7" +
      " FROM location" +
      " WHERE core = " + QString::number(core) +
      " AND module = \"" + bb->getModule()->id +
      "\" AND basicblock = \"" + bb->id + "\"";
  }

  query.exec(queryString);

  if(query.next()) {
    *runtime = query.value("runtime").toDouble();
    energy[0] = query.value("energy1").toDouble();
    energy[1] = query.value("energy2").toDouble();
    energy[2] = query.value("energy3").toDouble();
    energy[3] = query.value("energy4").toDouble();
    energy[4] = query.value("energy5").toDouble();
    energy[5] = query.value("energy6").toDouble();
    energy[6] = query.value("energy7").toDouble();

  } else {
    *runtime = 0;
    energy[0] = 0;
    energy[1] = 0;
    energy[2] = 0;
    energy[3] = 0;
    energy[4] = 0;
    energy[5] = 0;
    energy[6] = 0;
  }
}

void Profile::getMeasurements(unsigned core, BasicBlock *bb, QVector<Measurement> *measurements) {
  std::vector<Measurement> *mments;
  auto it = measurementsPerBb[core].find(bb);
  if(it == measurementsPerBb[core].end()) {
    return;
  } else {
    mments = it->second;
  }

  for(unsigned i = 1; i < mments->size(); i++) {
    Measurement m = (*mments)[i];
    measurements->push_back(m);
  }
}

void Profile::addExternalFunctions(Cfg *cfg) {
  Module *mod = cfg->externalMod;

  QSqlQuery query;

  QString queryString =
    QString() +
    "SELECT function" +
    " FROM location" +
    " WHERE module = \"" + mod->id + "\"";

  query.exec(queryString);

  while(query.next()) {
    QString funcId = query.value("function").toString();
    Function *func = mod->getFunctionById(funcId);
    if(!func) {
      func = new Function(funcId, mod, mod->children.size());
      mod->appendChild(func);

      BasicBlock *bb = new BasicBlock(QString::number(mod->children.size()), func, 0);
      func->appendChild(bb);
    }
  }
}

void Profile::clear() {
  for(unsigned core = 0; core < Pmu::MAX_CORES; core++) {
    for(auto const &it : measurementsPerBb[core]) {
      delete it.second;
    }
  }
}

