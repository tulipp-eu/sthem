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

#include "measurement.h"
#include "cfg/basicblock.h"
#include "cfg/function.h"
#include "cfg/cfg.h"

#include <QDataStream>

unsigned Measurement::counter[LYNSYN_MAX_CORES] = {0};

void Measurement::read(QFile &file, Cfg *cfg) {
  QDataStream in(&file);

  bool existsInCfg;
  in >> existsInCfg;

  if(existsInCfg) {
    QString modId;
    in >> modId;
    Module *mod = cfg->getModuleById(modId);

    QString bbId;
    in >> bbId;
    if(mod) {
      bb = mod->getBasicBlockById(bbId);
    } else {
      bb = NULL;
    }

  } else {
    QString funcId;
    in >> funcId;

    if(funcId != "") {
      Module *mod = cfg->externalMod;
      func = mod->getFunctionById(funcId);
      if(func) {
        bb = static_cast<BasicBlock*>(func->children[0]);
      } else {
        func = new Function(funcId, mod, mod->children.size());
        mod->appendChild(func);

        bb = new BasicBlock(QString::number(mod->children.size()), func, 0);
        func->appendChild(bb);
      }
    }
  }

  in >> core;
  in >> time;
  in >> timeSinceLast;
  for(unsigned i = 0; i < LYNSYN_SENSORS; i++) {
    in >> power[i];
  }
  in >> sequence;
  if(sequence >= counter[core]) counter[core] = sequence + 1;
}

void Measurement::write(QFile &file) {
  QDataStream out(&file);
  if(func) {
    out << false;
    out << func->id;
  } else if(bb) {
    out << true;
    out << bb->getModule()->id;
    out << bb->id;
  } else {
    out << false;
    out << QString("");
  }
  out << core;
  out << time;
  out << timeSinceLast;
  for(unsigned i = 0; i < LYNSYN_SENSORS; i++) {
    out << power[i];
  }
  out << sequence;
}

