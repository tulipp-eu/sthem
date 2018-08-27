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

#include "customproject.h"

#include <QMessageBox>

void CustomProject::writeCompileRule(QString compiler, QFile &makefile, QString path, QString opt) {
  QFileInfo fileInfo(path);

  QString clangTarget = ultrascale ? A53_CLANG_TARGET : A9_CLANG_TARGET;

  QStringList options;

  options << QString("-I") + this->path + "/src";

  options << opt.split(' ');
  options << clangTarget;

  makefile.write((fileInfo.completeBaseName() + ".o : " + path + "\n").toUtf8());
  makefile.write((QString("\t") + compiler + " " + options.join(' ') + " -c $< -o $@\n\n").toUtf8());
}

bool CustomProject::openProject(QString path) {
  close();

  this->path = path;
  QFileInfo fileInfo(path);
  name = fileInfo.completeBaseName();

  opened = true;

  QDir::setCurrent(path);

  loadProjectFile();

  configType = "";

  return true;
}

bool CustomProject::createProject(QString path) {
  return openProject(path);
} 

bool CustomProject::createMakefile() {
  QStringList objects;

  QFile makefile("Makefile");
  bool success = makefile.open(QIODevice::WriteOnly);
  if(!success) {
    QMessageBox msgBox;
    msgBox.setText("Can't create Makefile");
    msgBox.exec();
    return false;
  }

  Project::createMakefile(makefile);

  // compile c files
  for(auto source : sources) {
    QFileInfo info(source);

    bool tulippCompile = createBbInfo;
    Module *mod = cfgModel->getCfg()->getModuleById(info.completeBaseName());
    if(mod && createBbInfo) tulippCompile = !mod->hasHwCalls();

    if(info.suffix() == "c") {
      if(tulippCompile) {
        writeTulippCompileRule(Config::clang, makefile, source, cOptions + " " + cSysInc);
      } else {
        writeCompileRule(Config::clang, makefile, source, cOptions + " " + cSysInc);
      }
      objects << info.completeBaseName() + ".o";

    } else if((info.suffix() == "cpp") || (info.suffix() == "cc")) {
      if(tulippCompile) {
        writeTulippCompileRule(Config::clangpp, makefile, source, cppOptions + " " + cppSysInc);
      } else {
        writeCompileRule(Config::clangpp, makefile, source, cppOptions + " " + cppSysInc);
      }
      objects << info.completeBaseName() + ".o";

    }
  }

  objects.removeDuplicates();

  // link
  if(ultrascale) {
    if(isCpp) {
      writeLinkRule(Config::linkerppUs, makefile, objects);
    } else {
      writeLinkRule(Config::linkerUs, makefile, objects);
    }
  } else {
    if(isCpp) {
      writeLinkRule(Config::linkerpp, makefile, objects);
    } else {
      writeLinkRule(Config::linker, makefile, objects);
    }
  }

  makefile.close();

  return true;
}

