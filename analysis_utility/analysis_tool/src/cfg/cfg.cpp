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

#include "cfg.h"

Cfg::Cfg() : Container("", "", NULL, 0) {
  externalMod = new Module("__External__", this);
  appendChild(externalMod);

  profile = NULL;

  for(unsigned i = 0; i < Pmu::MAX_CORES; i++) {
    unknownProfLine[i] = NULL;
  }
}

Cfg::~Cfg() {}

void Cfg::buildProfTable(unsigned core, std::vector<ProfLine*> &table, bool forModel) {
  if(profile) {
    if(!unknownProfLine[core]) {
      unknownProfLine[core] = new ProfLine();

      double runtime;
      double energy[Pmu::MAX_SENSORS];

      profile->getProfData(core, NULL, &runtime, energy, QVector<BasicBlock*>(), &(unknownProfLine[core]->measurements));

      double power[Pmu::MAX_SENSORS];
      for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
        power[i] = 0;
        if(runtime) power[i] = energy[i] / runtime;
      }

      unknownProfLine[core]->init(NULL, runtime, power, energy);
    }

    table.push_back(unknownProfLine[core]);
    
    Container::buildProfTable(core, table, forModel);
  }
}

void Cfg::clearCachedProfilingData() {
  for(unsigned i = 0; i < Pmu::MAX_CORES; i++) {
    unknownProfLine[i] = NULL;
  }

  Container::clearCachedProfilingData();
}
