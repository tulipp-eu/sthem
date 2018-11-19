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

#include "cfg/container.h"
#include "cfg/loop.h"

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
  Config::opt = settings.value("opt", "opt").toString();
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

bool Analysis::openProject(QString path, QString configType, bool fast) {
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

    if(sdsocProject->openProject(path, configType, fast)) {
      project = sdsocProject;

      dse = new Dse(sdsocProject);
      dse->setCfg(project->cfg);

    } else {
      delete sdsocProject;
    }
  }

  if(project != NULL) {
    load();
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

bool Analysis::loadGProfFile(QString gprofPath, QString elfPath) {
  return project->parseGProfFile(gprofPath, elfPath, profile);
}

bool Analysis::clean() {
  if(!project->clean()) return false;
  if(dse) dse->clear();
  if(profile) profile->clean();
  return true;
}

bool Analysis::cleanBin() {
  if(!project->cleanBin()) return false;
  if(dse) dse->clear();
  if(profile) profile->clean();
  return true;
}

bool Analysis::runApp() {
  return project->runApp();  
}

bool Analysis::profileApp() {
  assert(profile);
  profile->clean();
  
  return project->runProfiler();  
}

void dumpLoop(unsigned core, unsigned sensor, Function *function, Loop *loop) {
  double runtime;
  double runtimeFrame;
  double energy[LYNSYN_SENSORS];
  double energyFrame[LYNSYN_SENSORS];
  uint64_t dummyCount;

  loop->getProfData(core, QVector<BasicBlock*>(), &runtime, energy, &runtimeFrame, energyFrame, &dummyCount);
  if(runtime > 0) {
    printf("%-40s %10ld %8.3f %8.3f %8.3f %8.3f %8.3f\n",
           (function->id + ":" + loop->id).toUtf8().constData(),
           loop->parent->getCount(), runtime, energy[sensor],
           runtime / loop->parent->getCount(), energy[sensor] / loop->parent->getCount(), energy[sensor] / runtime);
  }

  QVector<Loop*> loops;
  loop->getAllLoops(loops, QVector<BasicBlock*>(), false);
  for(auto childLoop : loops) {
    dumpLoop(core, sensor, function, childLoop);
  }
}

void Analysis::dump(unsigned core, unsigned sensor) {
  Cfg *cfg = project->cfg;

  printf("ID count runtime energy runtime/count energy/count energy/runtime\n");

  for(auto cfgChild : cfg->children) {
    Module *module = static_cast<Module*>(cfgChild);
    for(auto modChild : module->children) {
      Function *function = static_cast<Function*>(modChild);

      double runtime;
      double runtimeFrame;
      double energy[LYNSYN_SENSORS];
      double energyFrame[LYNSYN_SENSORS];
      uint64_t count;

      function->getProfData(core, QVector<BasicBlock*>(), &runtime, energy, &runtimeFrame, energyFrame, &count);
      if(runtime > 0) { // && (count > 0)) {
        printf("%-40s %10ld %8.3f %8.3f %8.3f %8.3f %8.3f\n",
               function->id.toUtf8().constData(), count, runtime,
               energy[sensor], runtime / count, energy[sensor] / count, energy[sensor] / runtime);

        QVector<Loop*> loops;
        function->getAllLoops(loops, QVector<BasicBlock*>(), false);
        for(auto loop : loops) {
          dumpLoop(core, sensor, function, loop);
        }
      }
    }
  }
}
