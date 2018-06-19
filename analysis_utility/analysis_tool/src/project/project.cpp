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

#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <inttypes.h>

#include <QApplication>
#include <QTextStream>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QDirIterator>

#include <usbprotocol.h>

#include "analysis_tool.h"
#include "project.h"

///////////////////////////////////////////////////////////////////////////////
// makefile creation

void Project::writeSdsRule(QString compiler, QFile &makefile, QString path, QString opt) {
  QStringList options;

  options << opt.split(' ');
  options << "-sds-pf" << platform << "-target-os" << os << "-dmclkid" << QString::number(dmclkid);

  for(auto acc : accelerators) {
    options << "-sds-hw" << acc.name << acc.filepath << "-clkid" << QString::number(acc.clkid) << "-sds-end";
  }

  options << QString("-I") + this->path + "/src";

  QFileInfo fileInfo(path);

  makefile.write((fileInfo.baseName() + ".o : " + path + "\n").toUtf8());
  makefile.write((QString("\t") + compiler + " " + options.join(' ') + " -c $< -o $@\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());
}

void Project::writeCompileRule(QString compiler, QFile &makefile, QString path, QString opt) {
  QFileInfo fileInfo(path);

  QString clangTarget = ultrascale ? A53_CLANG_TARGET : A9_CLANG_TARGET;

  QStringList options;

  options << opt.split(' ');
  options << clangTarget;

  for(auto include : systemIncludes) {
    if(include.trimmed() != "") {
      options << QString("-I") + include;
    }
  }

  options << QString("-I") + this->path + "/src";

  makefile.write((fileInfo.baseName() + ".o : " + path + "\n").toUtf8());
  makefile.write((QString("\t") + compiler + " " + options.join(' ') + " -c $< -o $@\n\n").toUtf8());
}

void Project::writeClangRule(QString compiler, QFile &makefile, QString path, QString opt) {
  QFileInfo fileInfo(path);

  QString clangTarget = A9_CLANG_TARGET;
  QString llcTarget = A9_LLC_TARGET;
  QString asTarget = A9_AS_TARGET;

  if(ultrascale) {
    clangTarget = A53_CLANG_TARGET;
    llcTarget = A53_LLC_TARGET;
    asTarget = A53_AS_TARGET;
  }

  // .ll
  {
    QStringList options;

    options << opt.split(' ');

    if(cfgOptLevel >= 0) {
      options << QString("-O") + QString::number(cfgOptLevel);
    } else {
      options << QString("-Os");
    }
    options << clangTarget;

    for(auto include : systemIncludes) {
      if(include.trimmed() != "") {
        options << QString("-I") + include;
      }
    }

    options << QString("-I") + this->path + "/src";

    makefile.write((fileInfo.baseName() + ".ll : " + path + "\n").toUtf8());
    makefile.write((QString("\t") + compiler + " " + options.join(' ') + " -g -emit-llvm -S $<\n\n").toUtf8());
  }

  // .xml
  {
    makefile.write((fileInfo.baseName() + ".xml : " + fileInfo.baseName() + ".ll\n").toUtf8());
    makefile.write((QString("\t") + Config::llvm_ir_parser + " $< -xml $@\n\n").toUtf8());
  }

  // _2.ll
  {
    makefile.write((fileInfo.baseName() + "_2.ll : " + fileInfo.baseName() + ".ll\n").toUtf8());
    makefile.write((QString("\t") + Config::llvm_ir_parser + " $< -ll $@\n\n").toUtf8());
  }

  // .s
  {
    QStringList options;
    if(cppOptLevel >= 0) {
      options << QString("-O") + QString::number(cppOptLevel);
    } else {
      options << QString("-Os");
    }
    options << QString(llcTarget).split(' ');

    makefile.write((fileInfo.baseName() + ".s : " + fileInfo.baseName() + "_2.ll\n").toUtf8());
    makefile.write((QString("\t") + Config::llc + " " + options.join(' ') + " $< -o $@\n\n").toUtf8());
  }

  // .o
  bool buildIt = true;
  for(auto acc : accelerators) {
    QFileInfo info(acc.filepath);
    if(info.baseName() == fileInfo.baseName()) {
      buildIt = false;
      break;
    }
  }
  if(buildIt) {
    QStringList options;
    options << QString(asTarget).split(' ');

    makefile.write((fileInfo.baseName() + ".o : " + fileInfo.baseName() + ".s\n").toUtf8());
    makefile.write((QString("\t") + Config::as + " " + options.join(' ') + " $< -o $@\n\n").toUtf8());
  }

  makefile.write(QString("###############################################################################\n\n").toUtf8());
}

void Project::writeSdsLinkRule(QString linker, QFile &makefile, QStringList objects) {
  QStringList options;
  options << "-sds-pf" << platform << "-target-os" << os << "-dmclkid" << QString::number(dmclkid);
  for(auto acc : accelerators) {
    options << "-sds-hw" << acc.name << acc.filepath << "-clkid" << QString::number(acc.clkid) << "-sds-end";
  }

  makefile.write((name + ".elf : " + objects.join(' ') + "\n").toUtf8());
  makefile.write((QString("\t") + linker + " " + options.join(' ') + " $^ " + linkerOptions + " -o $@\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());
}

void Project::writeLinkRule(QString linker, QFile &makefile, QStringList objects) {
  makefile.write((name + ".elf : " + objects.join(' ') + "\n").toUtf8());
  makefile.write(QString("\t" + linker + " $^ " + linkerOptions + " -o $@\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());
}

bool Project::createXmlMakefile() {
  QStringList xmlFiles;

  QFile makefile("Makefile");
  bool success = makefile.open(QIODevice::WriteOnly);
  if(!success) {
    QMessageBox msgBox;
    msgBox.setText("Can't create Makefile");
    msgBox.exec();
    return false;
  }

  makefile.write(QString("###################################################\n").toUtf8());
  makefile.write(QString("# Autogenerated Makefile for TULIPP Analysis Tool #\n").toUtf8());
  makefile.write(QString("###################################################\n\n").toUtf8());

  makefile.write(QString(".PHONY : clean\n").toUtf8());
  makefile.write(QString("clean :\n").toUtf8());
  makefile.write(QString("\trm -rf *.ll *.xml *.s *.o *.elf *.bit sd_card _sds __tulipp__.* profile.prof\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());

  for(auto source : sources) {
    QFileInfo info(source);
    if(info.suffix() == "c") {
      writeClangRule(Config::clang, makefile, source, cOptions + " " + cSysInc);
      xmlFiles << info.baseName() + ".xml";
    } else if((info.suffix() == "cpp") || (info.suffix() == "cc")) {
      writeClangRule(Config::clangpp, makefile, source, cppOptions + " " + cppSysInc);
      xmlFiles << info.baseName() + ".xml";
    }
  }

  makefile.write(QString(".PHONY : xml\n").toUtf8());
  makefile.write((QString("xml : ") + xmlFiles.join(' ') + "\n").toUtf8());
  makefile.write(QString("\trm -rf profile.prof\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());

  makefile.close();

  return true;
}

bool Project::createMakefile() {
  QStringList objects;

  QFile makefile("Makefile");
  bool success = makefile.open(QIODevice::WriteOnly);
  if(!success) {
    QMessageBox msgBox;
    msgBox.setText("Can't create Makefile");
    msgBox.exec();
    return false;
  }

  makefile.write(QString("###################################################\n").toUtf8());
  makefile.write(QString("# Autogenerated Makefile for TULIPP Analysis Tool #\n").toUtf8());
  makefile.write(QString("###################################################\n\n").toUtf8());

  makefile.write(QString(".PHONY : binary\n").toUtf8());
  makefile.write((QString("binary : ") + name + ".elf\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());

  makefile.write(QString(".PHONY : clean\n").toUtf8());
  makefile.write(QString("clean :\n").toUtf8());
  makefile.write(QString("\trm -rf *.ll *.xml *.s *.o *.elf *.bit sd_card _sds __tulipp__.* profile.prof\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());

  // compile normal c files
  for(auto source : sources) {
    QFileInfo info(source);

    bool useClang = createBbInfo;
    Module *mod = cfgModel->getCfg()->getModuleById(info.baseName());
    if(mod && createBbInfo) useClang = !mod->hasHwCalls();

    if(info.suffix() == "c") {
      if(useClang) {
        writeClangRule(Config::clang, makefile, source, cOptions + " " + cSysInc);
      } else if(isSdSocProject) {
        writeSdsRule("sdscc", makefile, source, cOptions);
      } else {
        writeCompileRule(Config::clang, makefile, source, cOptions + " " + cSysInc);
      }
      objects << info.baseName() + ".o";

    } else if((info.suffix() == "cpp") || (info.suffix() == "cc")) {
      if(useClang) {
        writeClangRule(Config::clangpp, makefile, source, cppOptions + " " + cppSysInc);
      } else if(isSdSocProject) {
        writeSdsRule("sds++", makefile, source, cppOptions);
      } else {
        writeCompileRule(Config::clangpp, makefile, source, cppOptions + " " + cppSysInc);
      }
      objects << info.baseName() + ".o";

    }
  }

  // compile at least one file with sds
  if(isSdSocProject && createBbInfo) {
    if(isCpp) {
      makefile.write(QString("__tulipp__.cpp :\n").toUtf8());
      makefile.write(QString("\techo \"/* TULIPP dummy file */\" > __tulipp__.cpp\n").toUtf8());
      makefile.write(QString("\techo \"extern \\\"C\\\" __attribute__((weak)) void __assert_fail (const char *__assertion, const char *__file, unsigned int __line, const char *__function) {}\" > __tulipp__.cpp\n\n").toUtf8());

      makefile.write(QString("###############################################################################\n\n").toUtf8());

      writeSdsRule("sds++", makefile, "__tulipp__.cpp", cppOptions);

    } else {
      makefile.write(QString("__tulipp__.c :\n").toUtf8());
      makefile.write(QString("\techo \"/* TULIPP dummy file */\" > __tulipp__.c\n\n").toUtf8());
      makefile.write(QString("\techo \"__attribute__((weak)) void __assert_fail (const char *__assertion, const char *__file, unsigned int __line, const char *__function) {}\" > __tulipp__.c\n\n").toUtf8());

      makefile.write(QString("###############################################################################\n\n").toUtf8());

      writeSdsRule("sdscc", makefile, "__tulipp__.c", cOptions);
    }

    objects << "__tulipp__.o";
  }

  // compile accelerator files
  for(auto acc : accelerators) {
    QFileInfo info(acc.filepath);
    if(info.suffix() == "c") {
      writeSdsRule("sdscc", makefile, acc.filepath, cOptions);
      objects << info.baseName() + ".o";
    } else if((info.suffix() == "cpp") || (info.suffix() == "cc")) {
      writeSdsRule("sds++", makefile, acc.filepath, cppOptions);
      objects << info.baseName() + ".o";
    }
  }

  objects.removeDuplicates();

  if(isSdSocProject) {
    if(isCpp) {
      writeSdsLinkRule("sds++", makefile, objects);
    } else {
      writeSdsLinkRule("sdscc", makefile, objects);
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

///////////////////////////////////////////////////////////////////////////////
// make

void Project::makeXml() {
  emit advance(0, "Building XML");

  bool created = createXmlMakefile();
  if(created) errorCode = system(QString("make -j xml").toUtf8().constData());
  else errorCode = 1;

  if(!created || errorCode) {
    emit finished(errorCode, "Can't make XML");
  } else {
    emit finished(errorCode, "");
  }
}

void Project::makeBin() {
  emit advance(0, "Building XML");

  bool created = createXmlMakefile();
  if(created) errorCode = system(QString("make -j xml").toUtf8().constData());
  else errorCode = 1;

  if(!created || errorCode) {
    emit finished(errorCode, "Can't make XML");
    return;
  }

  loadFiles();

  emit advance(1, "Building binary");

  created = createMakefile();
  if(created) errorCode = system(QString("make -j binary").toUtf8().constData());
  else errorCode = 1;

  if(!created || errorCode) {
    emit finished(errorCode, "Can't make binary");
  } else {
    parseSynthesisReport();
    emit finished(errorCode, "");
  }
}

void Project::clean() {
  if(opened) {
    createXmlMakefile();
    errorCode = system("make clean");
  }
}

///////////////////////////////////////////////////////////////////////////////
// SDSoC project parsing

bool Project::readSdSocProject(QString path, QString configType) {
  close();

  this->path = path;
  this->configType = configType;

  // get list of sources
  QDirIterator dirIt(path + "/src", QDir::Files, QDirIterator::Subdirectories);
  while(dirIt.hasNext()) {
    QString source = dirIt.next();

    QFileInfo fileInfo(source);
    if((fileInfo.suffix() == "cpp") || (fileInfo.suffix() == "cc")) {
      isCpp = true;
    }

    sources.push_back(source);
  }

  // get project options
  if(Config::sdsocVersion == 20162) {
    QDomDocument doc;
    QFile file(path + "/userbuildcfgs/" + configType + "_project.sdsoc");
    bool success = file.open(QIODevice::ReadOnly);
    if(!success) {
      QMessageBox msgBox;
      msgBox.setText("Can't open project");
      msgBox.exec();
      return false;
    }

    success = doc.setContent(&file);
    Q_UNUSED(success);
    assert(success);
    file.close();

    QDomNodeList elements = doc.elementsByTagName("sdsproject:Project");
    assert(elements.size());
    QDomElement element = elements.at(0).toElement();
    assert(!element.isNull());

    name = element.attribute("name", path);
    platform = element.attribute("platform", "emc2dp");
    os = element.attribute("os", "FreeRTOS");

    elements = doc.elementsByTagName("options");
    assert(elements.size());
    element = elements.at(0).toElement();
    assert(!element.isNull());

    insertapm = element.attribute("insertapm", "false") != "false";
    genbitstream = element.attribute("genbitstream", "true") != "false";
    gensdcard = element.attribute("gensdcard", "true") != "false";
    dmclkid = element.attribute("dmclkid", "0").toUInt();
    enableHwSwTrace = element.attribute("enableHwSwTrace", "false") != "false";
    traceApplication = element.attribute("traceApplication", "false") != "false";

    QDomNode node = element.firstChild();
    while(!node.isNull()) {
      QDomElement childElement = node.toElement();
      if(!childElement.isNull()) {
        if(childElement.tagName() == "accelerator") {
          ProjectAcc acc;
                
          acc.name = childElement.attribute("name", name + "_hw");
          acc.filepath = path + "/" + childElement.attribute("filepath", acc.name + ".cpp");
          acc.clkid = childElement.attribute("clkid", "2").toUInt();

          accelerators.push_back(acc);
        }
      }
      node = node.nextSibling();
    }

  } else {
    QDomDocument doc;
    QFile file(path + "/project.sdx");
    bool success = file.open(QIODevice::ReadOnly);

    if(!success) {
      QMessageBox msgBox;
      msgBox.setText("Can't open project");
      msgBox.exec();
      return false;
    }

    success = doc.setContent(&file);
    assert(success);
    file.close();

    QDomNodeList elements = doc.elementsByTagName("sdsproject:SDSProject");
    assert(elements.size());
    QDomElement element = elements.at(0).toElement();
    assert(!element.isNull());

    name = element.attribute("name", path);
    platform = element.attribute("platform", "emc2dp");
    os = element.attribute("os", "standalone");

    elements = doc.elementsByTagName("configuration");
    assert(elements.size());

    for(int i = 0; i < elements.size(); i++) {
      element = elements.at(i).toElement();
      assert(!element.isNull());

      if(element.attribute("name", "") == configType) {
        QDomNode node2 = element.firstChild();
        while(!node2.isNull()) {
          QDomElement childElement2 = node2.toElement();
          if(!childElement2.isNull()) {
            if(childElement2.tagName() == "configBuildOptions") {
              dmclkid = childElement2.attribute("dmclkid", "2").toUInt();
              enableHwSwTrace = childElement2.attribute("enableHwSwTrace", "false") != "false";
              traceApplication = childElement2.attribute("traceApplication", "false") != "false";
              insertapm = childElement2.attribute("insertapm", "false") != "false";
              genbitstream = childElement2.attribute("genbitstream", "true") != "false";
              gensdcard = childElement2.attribute("gensdcard", "true") != "false";

              QDomNode node = childElement2.firstChild();
              while(!node.isNull()) {
                QDomElement childElement = node.toElement();
                if(!childElement.isNull()) {
                  if(childElement.tagName() == "accelerator") {
                    ProjectAcc acc;
                
                    acc.name = childElement.attribute("name", name + "_hw");
                    acc.filepath = path + "/" + childElement.attribute("filepath", acc.name + ".cpp");
                    acc.clkid = childElement.attribute("clkid", "2").toUInt();

                    accelerators.push_back(acc);
                  }
                }
                node = node.nextSibling();
              }
            }
          }
          node2 = node2.nextSibling();
        }
      }
    }
  }

  // get custom platforms
  {
    QString key;
    if(Config::sdsocVersion == 20172) {
      key = "sdx.custom.platform.repository.locations";
    } else if(Config::sdsocVersion == 20172) {
      key = "sdsoc.platform";
    }
        
    QDomDocument doc;
    QFile file(path + "/../.metadata/.plugins/com.xilinx.sdsoc.ui/dialog_settings.xml");
    bool success = file.open(QIODevice::ReadOnly);
    if(success) {
      success = doc.setContent(&file);
      assert(success);
      file.close();

      QDomNodeList elements = doc.elementsByTagName("item");
      assert(elements.size());
      for(int i = 0; i < elements.size(); i++) {
        QDomElement element = elements.at(i).toElement();
        assert(!element.isNull());
        if(element.attribute("key", "") == key) {
          QStringList locations = element.attribute("value", "").split(';');
          for(int j = 0; j < locations.size(); j++) {
            QFileInfo fileInfo(locations[j]);
            if(fileInfo.fileName() == platform) {
              platform = locations[j];
            }
          }
        }
      }
    }
  }

  // get compiler/linker options
  {
    QDomDocument doc;
    QFile file(path + "/.cproject");
    bool success = file.open(QIODevice::ReadOnly);
    if(success) {
      success = doc.setContent(&file);
      assert(success);
      file.close();

      QDomNodeList elements = doc.elementsByTagName("configuration");
      assert(elements.size());

      for(int i = 0; i < elements.size(); i++) {
        QDomNode node = elements.at(i);
        QDomElement element = node.toElement();
        if(!element.isNull()) {
          if(element.attribute("name", "") == configType) {
            QDomNodeList tools = element.elementsByTagName("tool");

            for(int i = 0; i < tools.size(); i++) {
              QDomNode toolNode = tools.at(i);
              QDomElement toolElement = toolNode.toElement();
              if(!toolElement.isNull()) {
                if(toolElement.attribute("name", "") == "SDSCC Compiler") {
                  cOptions = processCompilerOptions(toolElement, &cOptLevel);
                }
                if(toolElement.attribute("name", "") == "SDS++ Compiler") {
                  cppOptions = processCompilerOptions(toolElement, &cppOptLevel);
                }
                if(toolElement.attribute("name", "") == "SDS++ Linker") {
                  linkerOptions = processLinkerOptions(toolElement);
                }
              }
            }
          }
        }
      }
    }
  }

  opened = true;
  isSdSocProject = true;

  // create .tulipp project dir
  QDir dir(QDir::homePath() + "/.tulipp/" + name);
  if(!dir.exists()) {
    dir.mkpath(".");
  }

  // cd
  QDir::setCurrent(dir.path());

  // get system includes
  // TODO: This slows down startup, and is not very elegant.  Can we do it differently, or cache results?
  {
    { // C
      int ret = system("touch __tulipp_test__.c");
      QString command = "sdscc -v -sds-pf " + platform + " -target-os " + os +
        " -c __tulipp_test__.c -o __tulipp_test__.o > __tulipp_test__.out";
      ret = system(command.toUtf8().constData());
      cSysInc = processIncludePaths("__tulipp_test__.out");
      Q_UNUSED(ret);
    }
    { // C++
      int ret = system("touch __tulipp_test__.cpp");
      QString command = "sds++ -v -sds-pf " + platform + " -target-os " + os +
        " -c __tulipp_test__.cpp -o __tulipp_test__.o > __tulipp_test__.out";
      ret = system(command.toUtf8().constData());
      cppSysInc = processIncludePaths("__tulipp_test__.out");
      Q_UNUSED(ret);
    }
  }

  loadProjectFile();

  timingOk = false;
  brams = 0;
  luts = 0;
  dsps = 0;
  regs = 0;

  // read previous synthesis report (if exists)
  parseSynthesisReport();

  return true;
}

QString Project::processIncludePaths(QString filename) {
  QString options;

  QFile file(filename);

  if(file.open(QIODevice::ReadOnly)) {
    QTextStream in(&file);

    QString line;
    do {
      line = in.readLine();

      if(line.contains("#include <...> search starts here:", Qt::CaseSensitive)) {
        options = "";
        line = in.readLine();

        while(!line.contains("End of search list.", Qt::CaseSensitive)) {
          options += "-I" + line + " ";
          line = in.readLine();
        }
      }
    } while(!line.isNull());
    file.close();
  }

  return options;
}

QString Project::sdsocExpand(QString text) {
  text.replace(QRegularExpression("\\${workspace_loc:*(.*)}"), Config::workspace + "\\1");
  text.replace(QRegularExpression("\\${ProjName:*(.*)}"), name + "\\1");
  return text;
}

QString Project::processCompilerOptions(QDomElement &childElement, int *optLevel) {
  QString options;

  bool hasOpt = false;

  QDomNode grandChildNode = childElement.firstChild();
  while(!grandChildNode.isNull()) {
    QDomElement grandChildElement = grandChildNode.toElement();
    if(!grandChildElement.isNull()) {
      if(grandChildElement.tagName() == "option") {

        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.compiler.dircategory.includes") {
          QDomNode grandGrandChildNode = grandChildElement.firstChild();
          while(!grandGrandChildNode.isNull()) {
            QDomElement grandGrandChildElement = grandGrandChildNode.toElement();
            if(!grandGrandChildElement.isNull()) {
              if(grandGrandChildElement.tagName() == "listOptionValue") {
                options += "-I" + sdsocExpand(xmlPurify(grandGrandChildElement.attribute("value", ""))) + " ";
              }
            }
            grandGrandChildNode = grandGrandChildNode.nextSibling();
          }
        }

        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.compiler.symbols.defined") {
          QDomNode grandGrandChildNode = grandChildElement.firstChild();
          while(!grandGrandChildNode.isNull()) {
            QDomElement grandGrandChildElement = grandGrandChildNode.toElement();
            if(!grandGrandChildElement.isNull()) {
              if(grandGrandChildElement.tagName() == "listOptionValue") {
                options += "-D" + xmlPurify(grandGrandChildElement.attribute("value", "")) + " ";
              }
            }
            grandGrandChildNode = grandGrandChildNode.nextSibling();
          }
        }

        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.compiler.symbols.undefined") {
          QDomNode grandGrandChildNode = grandChildElement.firstChild();
          while(!grandGrandChildNode.isNull()) {
            QDomElement grandGrandChildElement = grandGrandChildNode.toElement();
            if(!grandGrandChildElement.isNull()) {
              if(grandGrandChildElement.tagName() == "listOptionValue") {
                options += "-U" + xmlPurify(grandGrandChildElement.attribute("value", "")) + " ";
              }
            }
            grandGrandChildNode = grandGrandChildNode.nextSibling();
          }
        }

        if(grandChildElement.attribute("name", "") == "Optimization Level") {
          if(grandChildElement.attribute("value", "") == "gnu.c.optimization.level.none") {
            *optLevel = 0;
          } else if(grandChildElement.attribute("value", "") == "gnu.c.optimization.level.optimize") {
            *optLevel = 1;
          } else if(grandChildElement.attribute("value", "") == "gnu.c.optimization.level.more") {
            *optLevel = 2;
          } else if(grandChildElement.attribute("value", "") == "gnu.c.optimization.level.most") {
            *optLevel = 3;
          } else if(grandChildElement.attribute("value", "") == "gnu.c.optimization.level.size") {
            *optLevel = -1;
          }
          hasOpt = true;
        }

        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.compiler.misc.ansi") {
          if(grandChildElement.attribute("value", "false") != "false") {
            options += "-ansi ";
          }
        }

      }
    }
    grandChildNode = grandChildNode.nextSibling();
  }

  if(!hasOpt) {
    if(configType == "SDDebug") {
      *optLevel = 0;
    } else {
      *optLevel = 3;
    }
  }

  return options;
}

QString Project::processLinkerOptions(QDomElement &childElement) {
  QString options;

  QDomNode grandChildNode = childElement.firstChild();
  while(!grandChildNode.isNull()) {
    QDomElement grandChildElement = grandChildNode.toElement();
    if(!grandChildElement.isNull()) {
      if(grandChildElement.tagName() == "option") {
        if((grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.link.option.nostart") &&
           (grandChildElement.attribute("value", "false") != "false")) {
          options += "-nostartfiles ";
        }
        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.link.option.ldflags") {
          options += grandChildElement.attribute("value", "") + " ";
        }
        if((grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.link.option.nodeflibs") &&
           (grandChildElement.attribute("value", "false") != "false")) {
          options += "-nodefaultlibs ";
        }
        if((grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.link.option.nostdlibs") &&
           (grandChildElement.attribute("value", "false") != "false")) {
          options += "-nostdlib ";
        }
        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.linker.option.lscript") {
          if(grandChildElement.attribute("value", "").trimmed() != "") {
            options += QString("-T ") + grandChildElement.attribute("value", "") + " ";
          }
        }
        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.link.option.other") {
          QDomNode grandGrandChildNode = grandChildElement.firstChild();
          while(!grandGrandChildNode.isNull()) {
            QDomElement grandGrandChildElement = grandGrandChildNode.toElement();
            if(!grandGrandChildElement.isNull()) {
              if(grandGrandChildElement.tagName() == "listOptionValue") {
                options += "-XLinker " + xmlPurify(grandGrandChildElement.attribute("value", "")) + " ";
              }
            }
            grandGrandChildNode = grandGrandChildNode.nextSibling();
          }
        }
        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.link.option.userobjs") {
          QDomNode grandGrandChildNode = grandChildElement.firstChild();
          while(!grandGrandChildNode.isNull()) {
            QDomElement grandGrandChildElement = grandGrandChildNode.toElement();
            if(!grandGrandChildElement.isNull()) {
              if(grandGrandChildElement.tagName() == "listOptionValue") {
                options += xmlPurify(grandGrandChildElement.attribute("value", "")) + " ";
              }
            }
            grandGrandChildNode = grandGrandChildNode.nextSibling();
          }
        }
        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.link.option.libs") {
          QDomNode grandGrandChildNode = grandChildElement.firstChild();
          while(!grandGrandChildNode.isNull()) {
            QDomElement grandGrandChildElement = grandGrandChildNode.toElement();
            if(!grandGrandChildElement.isNull()) {
              if(grandGrandChildElement.tagName() == "listOptionValue") {
                options += "-l" + xmlPurify(grandGrandChildElement.attribute("value", "")) + " ";
              }
            }
            grandGrandChildNode = grandGrandChildNode.nextSibling();
          }
        }
        if(grandChildElement.attribute("superClass", "") == "xilinx.gnu.c.link.option.paths") {
          QDomNode grandGrandChildNode = grandChildElement.firstChild();
          while(!grandGrandChildNode.isNull()) {
            QDomElement grandGrandChildElement = grandGrandChildNode.toElement();
            if(!grandGrandChildElement.isNull()) {
              if(grandGrandChildElement.tagName() == "listOptionValue") {
                options += "-L" + sdsocExpand(xmlPurify(grandGrandChildElement.attribute("value", ""))) + " ";
              }
            }
            grandGrandChildNode = grandGrandChildNode.nextSibling();
          }
        }

      }
    }
    grandChildNode = grandChildNode.nextSibling();
  }

  return options;
}

QStringList Project::getBuildConfigurations(QString path) {
  QStringList list;

  if(Config::sdsocVersion == 20162) {
    QDir dir(path + "/userbuildcfgs");
    dir.setFilter(QDir::Files);
    QFileInfoList files = dir.entryInfoList();
    for(auto fileInfo : files) {
      list << fileInfo.fileName().split("_").at(0);
    }
  } else {
    QDomDocument doc;

    QFile file(path + "/project.sdx");
    bool success = file.open(QIODevice::ReadOnly);
    if(!success) return QStringList();

    success = doc.setContent(&file);
    if(!success) return QStringList();

    file.close();
      
    QDomNodeList elements = doc.elementsByTagName("configuration");
    if(elements.size() == 0) return QStringList();

    for(int i = 0; i < elements.size(); i++) {
      list << elements.at(i).toElement().attribute("name", path);
    }
    
  }

  return list;
}

void Project::parseSynthesisReport() {
  QFile file("_sds/reports/sds.rpt");

  if(file.open(QIODevice::ReadOnly)) {
    QTextStream in(&file);

    QString line;
    do {
      line = in.readLine();
      if(line.contains("All user specified timing constraints are met", Qt::CaseSensitive)) {
        timingOk = true;
      }
      if(line.contains("| Block RAM Tile", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        brams = list[5].trimmed().toFloat();
      }
      if(line.contains("| Slice LUTs", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        luts = list[5].trimmed().toFloat();
      }
      if(line.contains("| DSPs", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        dsps = list[5].trimmed().toFloat();
      }
      if(line.contains("| Slice Registers", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        regs = list[5].trimmed().toFloat();
      }
    } while(!line.isNull());

    file.close();
  }
}

///////////////////////////////////////////////////////////////////////////////
// custom project support

bool Project::openProject(QString path) {
  close();

  this->path = path;
  QFileInfo fileInfo(path);
  name = fileInfo.baseName();

  opened = true;
  isSdSocProject = false;

  QDir::setCurrent(path);

  loadProjectFile();

  configType = "";

  timingOk = false;
  brams = 0;
  luts = 0;
  dsps = 0;
  regs = 0;

  return true;
}

bool Project::createProject(QString path) {
  return openProject(path);
} 

void Project::loadProjectFile() {
  QSettings settings("project.ini", QSettings::IniFormat);

  cfgOptLevel = settings.value("cfgOptLevel", "-1").toInt();
  createBbInfo = settings.value("createBbInfo", true).toBool();
  systemIncludes = settings.value("systemIncludeDirs", Config::defaultSystemIncludeDirs).toStringList();
  systemXmls = settings.value("systemXmls", Config::defaultSystemXmls).toStringList();
  systemBins = settings.value("systemBins", Config::defaultSystemBins).toStringList();
  for(int i = 0; i < LYNSYN_SENSORS; i++) {
    supplyVoltage[i] = settings.value("supplyVoltage" + QString::number(i), 5).toDouble();
  }
  rl[0] = settings.value("rl0", 0.025).toDouble();
  rl[1] = settings.value("rl1", 0.05).toDouble();
  rl[2] = settings.value("rl2", 0.05).toDouble();
  rl[3] = settings.value("rl3", 0.1).toDouble();
  rl[4] = settings.value("rl4", 0.1).toDouble();
  rl[5] = settings.value("rl5", 1).toDouble();
  rl[6] = settings.value("rl6", 10).toDouble();
  cores = settings.value("cores", LYNSYN_MAX_CORES).toUInt();
  ultrascale = settings.value("ultrascale", true).toBool();
  tcfUploadScript = settings.value("tcfUploadScript",
                                   "connect -url tcp:127.0.0.1:3121\n"
                                   "source /opt/Xilinx/SDx/2017.2/SDK/scripts/sdk/util/zynqmp_utils.tcl\n"
                                   "targets 8\n"
                                   "rst -system\n"
                                   "after 3000\n"
                                   "targets 8\n"
                                   "while {[catch {fpga -file $name.elf.bit}] eq 1} {rst -system}\n"
                                   "targets 8\n"
                                   "source _sds/p0/ipi/emc2_hdmi_ultra.sdk/psu_init.tcl\n"
                                   "psu_init\n"
                                   "after 1000\n"
                                   "psu_ps_pl_isolation_removal\n"
                                   "after 1000\n"
                                   "psu_ps_pl_reset_config\n"
                                   "catch {psu_protection}\n"
                                   "targets 9\n"
                                   "rst -processor\n"
                                   "dow sd_card/$name.elf\n").toString();
  useCustomElf = settings.value("useCustomElf", false).toBool();
  customElfFile = settings.value("customElfFile", "").toString();
  startFunc = settings.value("startFunc", "main").toString();
  startCore = settings.value("startCore", 0).toUInt();
  stopFunc = settings.value("stopFunc", "_exit").toString();
  stopCore = settings.value("stopCore", 0).toUInt();

  if(!isSdSocProject) {
    sources = settings.value("sources").toStringList();
    cOptLevel = settings.value("cOptLevel", 0).toInt();
    cOptions = settings.value("cOptions", "").toString();
    cppOptLevel = settings.value("cppOptLevel", 0).toInt();
    cppOptions = settings.value("cppOptions", "").toString();
    linkerOptions = settings.value("linkerOptions", "").toString();
  }
}

void Project::saveProjectFile() {
  QSettings settings("project.ini", QSettings::IniFormat);

  settings.setValue("cfgOptLevel", cfgOptLevel);
  settings.setValue("createBbInfo", createBbInfo);
  settings.setValue("systemIncludeDirs", systemIncludes);
  settings.setValue("systemXmls", systemXmls);
  settings.setValue("systemBins", systemBins);
  for(int i = 0; i < LYNSYN_SENSORS; i++) { 
    settings.setValue("supplyVoltage" + QString::number(i), supplyVoltage[i]);
    settings.setValue("rl" + QString::number(i), rl[i]);
  }
  settings.setValue("cores", cores);
  settings.setValue("ultrascale", ultrascale);
  settings.setValue("tcfUploadScript", tcfUploadScript);
  settings.setValue("useCustomElf", useCustomElf);
  settings.setValue("customElfFile", customElfFile);
  settings.setValue("startFunc", startFunc);
  settings.setValue("startCore", startCore);
  settings.setValue("stopFunc", stopFunc);
  settings.setValue("stopCore", stopCore);

  settings.setValue("sources", sources);
  settings.setValue("cOptLevel", cOptLevel);
  settings.setValue("cOptions", cOptions);
  settings.setValue("cppOptLevel", cppOptLevel);
  settings.setValue("cppOptions", cppOptions);
  settings.setValue("linkerOptions", linkerOptions);
}

///////////////////////////////////////////////////////////////////////////////
// addr2line

Addr2Line Project::addr2line(QString elfFile, uint64_t pc) {
  char buf[1024];
  FILE *fp;
  std::stringstream pcStream;
  std::string cmd;

  QString fileName;
  QString function;
  uint64_t lineNumber;

  if(elfFile == "") goto error;

  // create command
  pcStream << std::hex << pc;
  cmd = "addr2line -C -f -a " + pcStream.str() + " -e " + elfFile.toUtf8().constData();

  // run addr2line program
  if((fp = popen(cmd.c_str(), "r")) == NULL) goto error;

  // discard first output line
  if(fgets(buf, 1024, fp) == NULL) goto error;

  // get function name
  if(fgets(buf, 1024, fp) == NULL) goto error;
  function = QString::fromUtf8(buf).simplified();
  if(function == "??") function = "Unknown";

  // get filename and linenumber
  if(fgets(buf, 1024, fp) == NULL) goto error;
  {
    QString qbuf = QString::fromUtf8(buf);
    fileName = qbuf.left(qbuf.indexOf(':'));
    lineNumber = qbuf.mid(qbuf.indexOf(':') + 1).toULongLong();
  }

  // close stream
  if(pclose(fp)) goto error;

  {
    Addr2Line ret(fileName, function, lineNumber);
    addr2lineCache[pc] = ret;
    return ret;
  }

 error:
  Addr2Line failed("", "", 0);

  addr2lineCache[pc] = failed;

  return failed;
}

Addr2Line Project::cachedAddr2line(QString elfFile, uint64_t pc) {
  auto it = addr2lineCache.find(pc);
  if(it != addr2lineCache.end()) {
    return (*it).second;
  }

  Addr2Line a2l = addr2line(elfFile, pc);

  if(a2l.function == "Unknown") {
    for(auto elf : systemBins) {
      if(elf != "") {
        a2l = addr2line(elf, pc);
        if(a2l.function != "Unknown") {
          break;
        }
      }
    }
  }

  addr2lineCache[pc] = a2l;
  return a2l;
}

uint64_t Project::lookupSymbol(QString elfFile, QString symbol) {
  FILE *fp;
  char buf[1024];

  // create command line
  QString cmd = QString("nm ") + elfFile;

  // run program
  if((fp = popen(cmd.toUtf8().constData(), "r")) == NULL) goto error;

  while(fgets(buf, 1024, fp)) {
    QStringList line = QString::fromUtf8(buf).split(' ');
    if(line[2].trimmed() == symbol) {
      return line[0].toULongLong(0, 16);
    }
  }

 error:
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// transform source

int Project::runSourceTool(QString inputFilename, QString outputFilename, QStringList loopsToPipeline) {
  QStringList options;

  options << inputFilename;

  for(auto x : cOptions.split(' ')) {
    if(x.trimmed() != "") {
      options << QString("-extra-arg=") + x;
    }
  }

  for(auto include : systemIncludes) {
    if(include.trimmed() != "") {
      options << QString("-extra-arg=-I") + include;
    }
  }

  options << QString("-extra-arg=-I") + this->path + "/src";

  for(auto l : loopsToPipeline) {
    options << QString("-pipeloop=") + l;
  }

  options << QString("-output " + outputFilename);

  options << QString("--");

  return system((Config::tulipp_source_tool + " " + options.join(' ')).toUtf8().constData());
}

///////////////////////////////////////////////////////////////////////////////
// prof file parsing

QVector<Measurement> *Project::parseProfFile() {
  QFile file("profile.prof");
  int ret = file.open(QIODevice::ReadOnly);
  if(ret) {
    QVector<Measurement> *measurements = parseProfFile(file);
    file.close();
    return measurements;
  } else {
    return new QVector<Measurement>;
  }
}

QVector<Measurement> *Project::parseProfFile(QFile &file) {
  QVector<Measurement> *measurements = new QVector<Measurement>;

  while(!file.atEnd()) {
    Measurement m;
    m.read(file, cfgModel->getCfg());
    measurements->push_back(m);
  }

  return measurements;
}

///////////////////////////////////////////////////////////////////////////////
// usb and lynsyn

bool Project::initLynsyn() {

  libusb_device *lynsynBoard;

  int r = libusb_init(&usbContext);

  if(r < 0) {
    printf("Init Error\n"); //there was an error
    return false;
  }
	  
  libusb_set_debug(usbContext, 3); //set verbosity level to 3, as suggested in the documentation

  bool found = false;
  int numDevices = libusb_get_device_list(usbContext, &devs);
  for(int i = 0; i < numDevices; i++) {
    libusb_device_descriptor desc;
    libusb_device *dev = devs[i];
    libusb_get_device_descriptor(dev, &desc);
    if(desc.idVendor == 0x10c4 && desc.idProduct == 0x8c1e) {
      printf("Found Lynsyn Device\n");
      lynsynBoard = dev;
      found = true;
      break;
    }
  }

  if(!found) return false;

  int err = libusb_open(lynsynBoard, &lynsynHandle);

  if(err < 0) {
    printf("Could not open USB device\n");
    return false;
  }

  if(libusb_kernel_driver_active(lynsynHandle, 0x1) == 1) {
    err = libusb_detach_kernel_driver(lynsynHandle, 0x1);
    if (err) {
      printf("Failed to detach kernel driver for USB. Someone stole the board?\n");
      return false;
    }
  }

  if((err = libusb_claim_interface(lynsynHandle, 0x1)) < 0) {
    printf("Could not claim interface 0x1, error number %d\n", err);
    return false;
  }

  struct libusb_config_descriptor * config;
  libusb_get_active_config_descriptor(lynsynBoard, &config);
  if(config == NULL) {
    printf("Could not retrieve active configuration for device :(\n");
    return false;
  }

  struct libusb_interface_descriptor interface = config->interface[1].altsetting[0];
  for(int ep = 0; ep < interface.bNumEndpoints; ++ep) {
    if(interface.endpoint[ep].bEndpointAddress & 0x80) {
      inEndpoint = interface.endpoint[ep].bEndpointAddress;
    } else {
      outEndpoint = interface.endpoint[ep].bEndpointAddress;
    }
  }

  {
    struct RequestPacket initRequest;
    initRequest.cmd = USB_CMD_INIT;
    sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));

    struct InitReplyPacket initReply;
    getBytes((uint8_t*)&initReply, sizeof(struct InitReplyPacket));

    if(initReply.swVersion != USB_PROTOCOL_VERSION) {
      printf("Unsupported Lynsyn SW Version\n");
      return false;
    }

    if((initReply.hwVersion != HW_VERSION_2_0) && (initReply.hwVersion != HW_VERSION_2_1)) {
      printf("Unsupported Lynsyn HW Version: %x\n", initReply.hwVersion);
      return false;
    }

    hwVersion = initReply.hwVersion;

    for(int i = 0; i < LYNSYN_SENSORS; i++) {
      if((initReply.calibration[i] < 0.8) || (initReply.calibration[i] > 1.2)) {
        printf("Suspect calibration values\n");
        return false;
      }
      sensorCalibration[i] = initReply.calibration[i];
    }
  }

  return true;
}

void Project::releaseLynsyn() {
  libusb_release_interface(lynsynHandle, 0x1);
  libusb_attach_kernel_driver(lynsynHandle, 0x1);
  libusb_free_device_list(devs, 1);

  libusb_close(lynsynHandle);
  libusb_exit(usbContext);
}

void Project::sendBytes(uint8_t *bytes, int numBytes) {
  int remaining = numBytes;
  int transfered = 0;
  while(remaining > 0) {
    libusb_bulk_transfer(lynsynHandle, outEndpoint, bytes, numBytes, &transfered, 0);
    remaining -= transfered;
    bytes += transfered;
  }
}

void Project::getBytes(uint8_t *bytes, int numBytes) {
  int transfered = 0;
  int ret = 0;

  ret = libusb_bulk_transfer(lynsynHandle, inEndpoint, bytes, numBytes, &transfered, 100);
  if(ret != 0) {
    printf("LIBUSB ERROR: %s\n", libusb_error_name(ret));
  }

  assert(transfered == numBytes);

  return;
}

///////////////////////////////////////////////////////////////////////////////
// project files

void Project::loadFiles() {
  if(cfgModel) delete cfgModel;
  cfgModel = new CfgModel();

  QDir dir(".");
  dir.setFilter(QDir::Files);

  // read system XML files
  for(auto filename : systemXmls) {
    if(filename != "") loadXmlFile(filename);
  }

  // read XML files from tulipp project dir
  {
    QStringList nameFilter;
    nameFilter << "*.xml";
    dir.setNameFilters(nameFilter);

    QFileInfoList list = dir.entryInfoList();
    for(auto fileInfo : list) {
      loadXmlFile(fileInfo.filePath());
    }
  }
}

void Project::loadXmlFile(const QString &fileName) {
  QDomDocument doc;
  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly)) {
    QMessageBox msgBox;
    msgBox.setText("File not found");
    msgBox.exec();
    return;
  }
  if(!doc.setContent(&file)) {
    QMessageBox msgBox;
    msgBox.setText("Invalid XML file");
    msgBox.exec();
    file.close();
    return;
  }
  file.close();

  try {
    cfgModel->addModule(doc, *this);
  } catch (std::exception &e) {
    QMessageBox msgBox;
    msgBox.setText("Invalid CFG file");
    msgBox.exec();
    return;
  }
}

///////////////////////////////////////////////////////////////////////////////
// object construction, destruction and management

Project::Project() {
  cfgModel = NULL;
  close();
}

Project::Project(Project *p) {
  opened = p->opened;
  isCpp = p->isCpp;

  path = p->path;
  sources = p->sources;
  name = p->name;
  platform = p->platform;
  os = p->os;
  configType = p->configType;
  insertapm = p->insertapm;
  genbitstream = p->genbitstream;
  gensdcard = p->gensdcard;
  dmclkid = p->dmclkid;
  enableHwSwTrace = p->enableHwSwTrace;
  traceApplication = p->traceApplication;
  accelerators = p->accelerators;
  
  cOptions = p->cOptions;
  cOptLevel = p->cOptLevel;
  cppOptions = p->cppOptions;
  cppOptLevel = p->cppOptLevel;
  linkerOptions = p->linkerOptions;
  
  timingOk = p->timingOk;
  brams = p->brams;
  luts = p->luts;
  dsps = p->dsps;
  regs = p->regs;
  
  elfFile = p->elfFile;

  cfgModel = NULL;
}

Project::~Project() {
  delete cfgModel;
}

void Project::close() {
  opened = false;
  isCpp = false;
  path = "";
  clear();
}

void Project::clear() {
  sources.clear();
  accelerators.clear();
  if(cfgModel) delete cfgModel;
  cfgModel = new CfgModel();
}

void Project::print() {
  printf("Project: %s\n", name.toUtf8().constData());
  printf("Path: %s\n", path.toUtf8().constData());
  printf("Platform: %s\n", platform.toUtf8().constData());
  printf("OS: %s\n", os.toUtf8().constData());
  printf("Config: %s\n", configType.toUtf8().constData());
  printf("Insert APM: %d\n", insertapm);
  printf("Gen bitstream: %d\n", genbitstream);
  printf("Gen SD card: %d\n", gensdcard);
  printf("DM clk ID: %d\n", dmclkid);
  printf("Enable HW/SW trace: %d\n", enableHwSwTrace);
  printf("Trace application: %d\n", traceApplication);

  printf("Files:\n");
  for(auto source : sources) {
    printf("  %s\n", source.toUtf8().constData());
  }

  printf("Accelerators:\n");
  for(auto acc : accelerators) {
    acc.print();
  }

  printf("C options: %s\n", cOptions.toUtf8().constData());
  printf("C opt level: %d\n", cOptLevel);
  printf("C++ options: %s\n", cppOptions.toUtf8().constData());
  printf("C++ opt level: %d\n", cppOptLevel);
  printf("Linker options: %s\n", linkerOptions.toUtf8().constData());
}

///////////////////////////////////////////////////////////////////////////////
// profiling support

void Project::runProfiler() {
  emit advance(0, "Uploading binary");

  // init lynsyn
  bool lynsynInited = initLynsyn();
  if(!lynsynInited) {
    emit finished(1, "Can't connect to Lynsyn");
    return;
  }

  QFile tclFile("temp-lynsyn-prof.tcl");
  bool success = tclFile.open(QIODevice::WriteOnly);
  Q_UNUSED(success);
  assert(success);

  QString tcl = QString() + "set name " + name + "\n" + tcfUploadScript;
      
  tclFile.write(tcl.toUtf8());

  tclFile.close();

  int ret = system("xsct temp-lynsyn-prof.tcl");
  if(ret) {
    emit finished(1, "Can't upload binaries");
    releaseLynsyn();
    return;
  }

  emit advance(1, "Collecting samples");

  ///////////////////////////////////////////////////////////////////////////////

  {
    struct RequestPacket req;
    req.cmd = USB_CMD_JTAG_INIT;
    sendBytes((uint8_t*)&req, sizeof(struct RequestPacket));
  }

  {
    struct BreakpointRequestPacket req;
    req.request.cmd = USB_CMD_BREAKPOINT;
    req.core = startCore;
    req.bpType = BP_TYPE_START;
    if(useCustomElf) {
      req.addr = lookupSymbol(customElfFile, startFunc);
    } else {
      req.addr = lookupSymbol(elfFile, startFunc);
    }
    sendBytes((uint8_t*)&req, sizeof(struct BreakpointRequestPacket));
  }

  {
    struct BreakpointRequestPacket req;
    req.request.cmd = USB_CMD_BREAKPOINT;
    req.core = stopCore;
    req.bpType = BP_TYPE_STOP;
    if(useCustomElf) {
      req.addr = lookupSymbol(customElfFile, stopFunc);
    } else {
      req.addr = lookupSymbol(elfFile, stopFunc);
    }
    sendBytes((uint8_t*)&req, sizeof(struct BreakpointRequestPacket));
  }

  {
    struct RequestPacket req;
    req.cmd = USB_CMD_START_SAMPLING;
    sendBytes((uint8_t*)&req, sizeof(struct RequestPacket));
  }

  QFile profFile("profile.prof");
  success = profFile.open(QIODevice::WriteOnly);
  Q_UNUSED(success);
  assert(success);

  int remainingSpace = 128*1024*1024;
  uint8_t *buf = (uint8_t*)malloc(remainingSpace);
  assert(buf);
  uint8_t *ptr = buf;

  while(true) {
    getBytes(ptr, sizeof(struct SampleReplyPacket));

    ptr += sizeof(struct SampleReplyPacket);
    remainingSpace -= sizeof(struct SampleReplyPacket);

    if((unsigned)remainingSpace < sizeof(struct SampleReplyPacket)) {
      printf("Sample buffer full\n");
      break;
    }

    struct SampleReplyPacket *sample = (struct SampleReplyPacket *)(ptr - sizeof(struct SampleReplyPacket));
    if(sample->time == -1) {
      printf("Sampling done\n");
      break;
    }
  }

  emit advance(2, "Processing samples");

  struct SampleReplyPacket *sample = (struct SampleReplyPacket*)buf;
  int64_t lastTime = sample->time;
  sample++;

  while((uint8_t*)sample < ptr) {
    if(sample->time == -1) {
      break;
    }

    int64_t interval = sample->time - lastTime;
    lastTime = sample->time;

    for(unsigned core = 0; core < LYNSYN_MAX_CORES; core++) {
      double p[LYNSYN_SENSORS];

      for(unsigned sensor = 0; sensor < LYNSYN_SENSORS; sensor++) {
        switch(hwVersion) {
          case HW_VERSION_2_0: {
            double vo = (sample->current[sensor] * (double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE;
            double i = (1000 * vo) / (LYNSYN_RS * rl[sensor]);
            p[sensor] = i * supplyVoltage[sensor];
            break;
          }
          case HW_VERSION_2_1: {
            double v =
              (sample->current[sensor] * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * sensorCalibration[sensor];
            double vs = v / 20;
            double i = vs / rl[sensor];
            p[sensor] = i * supplyVoltage[sensor];
            break;
          }
        }
      }

      Addr2Line a2l;
      if(useCustomElf) {
        a2l = cachedAddr2line(customElfFile, sample->pc[core]);
      } else {
        a2l = cachedAddr2line(elfFile, sample->pc[core]);
      }

      if(!useCustomElf && a2l.isBb()) {
        Module *mod = cfgModel->getCfg()->getModuleById(a2l.getModuleId());
        BasicBlock *bb = NULL;
        if(mod) bb = mod->getBasicBlockById(QString::number(a2l.lineNumber));
        Measurement m(core, bb, sample->time, interval, p);
        m.write(profFile);

      } else {
        Function *func = cfgModel->getCfg()->getFunctionById(a2l.function);

        if(func) {
          // we don't know the BB, but the function exists in the CFG: add to first BB
          BasicBlock *bb = func->getFirstBb();
          if(bb) {
            Measurement m(core, bb, sample->time, interval, p);
            m.write(profFile);
          }
          
        } else {
          Module *mod = cfgModel->getCfg()->externalMod;
          func = mod->getFunctionById(a2l.function);
          BasicBlock *bb = NULL;
          if(func) {
            bb = static_cast<BasicBlock*>(func->children[0]);
          } else {
            func = new Function(a2l.function, mod, mod->children.size());
            mod->appendChild(func);

            bb = new BasicBlock(QString::number(mod->children.size()), func, 0);
            func->appendChild(bb);
          }
          Measurement m(core, bb, func, sample->time, interval, p);
          m.write(profFile);
        }
      }
    }

    sample++;
  }

  profFile.close();

  releaseLynsyn();

  emit finished(0, "");
}

///////////////////////////////////////////////////////////////////////////////

Cfg *Project::getCfg() {
  return cfgModel->getCfg();
}

