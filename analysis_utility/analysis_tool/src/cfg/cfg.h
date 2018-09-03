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

#ifndef CFG_H
#define CFG_H

#include "analysis_tool.h"
#include "container.h"
#include "function.h"
#include "module.h"
#include "profile/profile.h"
#include "project/pmu.h"

class Profile;

class Cfg : public Container {

private:
  ProfLine *unknownProfLine[Pmu::MAX_CORES];
  Profile *profile;

public:
  Module *externalMod;

  Cfg();
  virtual ~Cfg();

  virtual Cfg *getTop() {
    return this;
  }

  virtual QString getTypeName() {
    return "CFG";
  }

  virtual Module *getModuleById(QString id) {
    for(auto child : children) {
      Module *module = static_cast<Module*>(child);
      if(module->id == id) return module;
    }
    return NULL;
  }

  Function *getMain() {
    return getFunctionById("main");
  }

  virtual Function *getFunctionById(QString id) {
    for(auto child : children) {
      if(child != externalMod) {
        Module *module = static_cast<Module*>(child);
        Function *function = module->getFunctionById(id);
        if(function) return function;
      }
    }
    return NULL;
  }

  virtual void appendChild(Vertex *e) {
    children.push_back(e);
  }

  virtual void clearCallers() {
    for(auto child : children) {
      Module *module = static_cast<Module*>(child);
      module->clearCallers();
    }
  }

  virtual void setProfile(Profile *profile);

  virtual Profile *getProfile() {
    return profile;
  }

  virtual void clearCachedProfilingData();
};

#endif
