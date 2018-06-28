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

#ifndef DSERUN_H
#define DSERUN_H

#include <QVector>

#include "project/sdsoc.h"

class DseRun {
public:
  bool failed;
  QVector<unsigned> genome;
  Sdsoc *project;
  Profile *profile;
  double time;

  DseRun() {
    failed = false;
    project = NULL;
    profile = NULL;
  }

  DseRun(QVector<unsigned> genome, Sdsoc *project, Profile *profile) {
    this->failed = false;
    this->genome = genome;
    this->project = project;
    this->profile = profile;
  }

	friend std::ostream& operator<<(std::ostream &os, const DseRun &d) {
    os << d.genome.size() << '\n';
    for(auto g : d.genome) {
      os << g << '\n';
    }

    os << d.failed << '\n';
    os << d.time << '\n';

    if(!d.failed) {
      os << *d.project << '\n';
      os << *d.profile << '\n';
    }

    os << "****";

		return os;
	}

	friend std::istream& operator>>(std::istream &is, DseRun &d) {
    d.genome.clear();

    int size;
    is >> size;
    for(int i = 0; i < size; i++) {
      unsigned g;
      is >> g;
      d.genome.push_back(g);
    }

    is >> d.failed;
    is >> d.time;

    if(!d.failed) {
      if(!d.project) {
        if(Config::sdsocVersion == 20162) {
          d.project = new Sdsoc20162;
        } else if(Config::sdsocVersion == 20172) {
          d.project = new Sdsoc20172;
        } else if(Config::sdsocVersion == 20174) {
          d.project = new Sdsoc20174;
        }
      }
      if(!d.profile) d.profile = new Profile;

      is >> *d.project >> *d.profile;
    }

    std::string marker;
    while(marker != "****" && is.good()) {
      std::getline(is, marker);
    }

    return is;
	}
};

#endif
