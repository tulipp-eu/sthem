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

#include "analysis.h"

Analysis::Analysis() {
  project = NULL;
  profile = NULL;
  dse = NULL;

  QSqlDatabase::addDatabase("QSQLITE");

  // settings
  QSettings settings;

  Config::workspace = settings.value("workspace", QDir::homePath() + "/workspace/").toString();
  Config::includeAllInstructions = settings.value("includeAllInstructions", false).toBool();
  Config::includeProfData = settings.value("includeProfData", true).toBool();
  Config::includeId = settings.value("includeId", false).toBool();
  Config::clang = settings.value("clang", "clang").toString();
  Config::clangpp = settings.value("clangpp", "clang++").toString();
  Config::llc = settings.value("llc", "llc").toString();
  Config::llvm_ir_parser = settings.value("llvm_ir_parserPath", "llvm_ir_parser").toString();
  Config::tulipp_source_tool = settings.value("tulipp_source_toolPath", "tulipp_source_tool").toString();
  Config::as = settings.value("asPath", "arm-none-eabi-as").toString();
  Config::linker = settings.value("linkerPath", "arm-none-eabi-gcc").toString();
  Config::linkerpp = settings.value("linkerppPath", "arm-none-eabi-g++").toString();
  Config::asUs = settings.value("asUsPath", "aarch64-none-elf-as").toString();
  Config::linkerUs = settings.value("linkerUsPath", "aarch64-none-elf-gcc").toString();
  Config::linkerppUs = settings.value("linkerppUsPath", "aarch64-none-elf-g++").toString();
  Config::core = settings.value("core", 0).toUInt();
  Config::sensor = settings.value("sensor", 0).toUInt();
  Config::window = settings.value("window", 1).toUInt();
  
  Config::sdsocVersion = Sdsoc::getSdsocVersion();

  // create .tulipp directory
  QDir dir(QDir::homePath() + "/.tulipp");
  if(!dir.exists()) {
    dir.mkpath(".");
  }
}

Analysis::~Analysis() {
  if(profile) {
    delete profile;
  }
}

void Analysis::load() {
  if(profile) delete profile;
  profile = new Profile;

  assert(project);
  project->loadFiles();

  project->cfg->setProfile(profile);

  if(dse) dse->setCfg(project->cfg);

  QDir dir(".");
  dir.setFilter(QDir::Files);
  // read files from tulipp project dir
  {
    QStringList nameFilter;
    nameFilter << "*.elf" << "*.dse";
    dir.setNameFilters(nameFilter);

    QFileInfoList list = dir.entryInfoList();
    for(auto fileInfo : list) {
      QString path = fileInfo.filePath();
      QString suffix = fileInfo.suffix();
      if(suffix == "dse") {
        std::ifstream inputFile(path.toUtf8()); 
        inputFile >> *dse;
        inputFile.close();
      } else if(suffix == "elf") {
        project->elfFile = path;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

bool Analysis::openProject(QString path, QString configType) {
  if(configType == "") {
    CustomProject *customProject = new CustomProject;

    if(customProject->openProject(path)) {
      project = customProject;
    } else {
      delete customProject;
    }

  } else {
    Sdsoc *sdsocProject = NULL;

    if(Config::sdsocVersion == 20162) {
      sdsocProject = new Sdsoc20162();
    } else if(Config::sdsocVersion == 20172) {
      sdsocProject = new Sdsoc20172();
    } else if(Config::sdsocVersion == 20174) {
      sdsocProject = new Sdsoc20174();
    } else {
      QApplication::restoreOverrideCursor();
      QMessageBox msgBox;
      msgBox.setText("Can't open SDSoC project without SDSoC");
      msgBox.exec();
      return false;
    }

    if(sdsocProject->openProject(path, configType)) {
      project = sdsocProject;

      dse = new Dse(sdsocProject);
      dse->setCfg(project->cfg);

    } else {
      delete sdsocProject;
    }
  }

  return project != NULL;
}

bool Analysis::createProject(QString path) {
  CustomProject *customProject = new CustomProject;

  if(customProject->createProject(path)) {
    project = customProject;

  } else {
    delete customProject;
    return false;
  }

  return true;
}

void Analysis::closeProject() {
  delete project;
  project = NULL;

  if(dse) delete dse;
  dse = NULL;

  if(profile) delete profile;
  profile = NULL;
}

bool Analysis::loadProfFile(QString path) {
  return project->parseProfFile(path, profile);
}

bool Analysis::clean() {
  if(!project->clean()) return false;
  if(dse) dse->clear();
  if(profile) profile->clean();
  return true;
}

