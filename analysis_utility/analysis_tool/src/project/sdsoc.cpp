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

#include "sdsoc.h"

void Sdsoc::copy(Sdsoc *p) {
  Project::copy(p);

  platform = p->platform;
  os = p->os;
  insertapm = p->insertapm;
  genbitstream = p->genbitstream;
  gensdcard = p->gensdcard;
  dmclkid = p->dmclkid;
  enableHwSwTrace = p->enableHwSwTrace;
  traceApplication = p->traceApplication;

  timingOk = p->timingOk;
  brams = p->brams;
  luts = p->luts;
  dsps = p->dsps;
  regs = p->regs;
}

Sdsoc::Sdsoc(Sdsoc *p) {
  copy(p);
}

void Sdsoc20172::print() {
  Sdsoc::print();
  printf("sysConfig: %s\n", sysConfig.toUtf8().constData());
  printf("cpu: %s\n", cpu.toUtf8().constData());
}

void Sdsoc20174::print() {
  Sdsoc::print();
  printf("sysConfig: %s\n", sysConfig.toUtf8().constData());
  printf("cpu: %s\n", cpu.toUtf8().constData());
  printf("cpuInstance: %s\n", cpuInstance.toUtf8().constData());
  printf("genEmulationModel: %d\n", genEmulationModel);
}

void Sdsoc::print() {
  Project::print();

  printf("Platform: %s\n", platform.toUtf8().constData());
  printf("OS: %s\n", os.toUtf8().constData());
  printf("Config: %s\n", configType.toUtf8().constData());
  printf("Insert APM: %d\n", insertapm);
  printf("Gen bitstream: %d\n", genbitstream);
  printf("Gen SD card: %d\n", gensdcard);
  printf("DM clk ID: %d\n", dmclkid);
  printf("Enable HW/SW trace: %d\n", enableHwSwTrace);
  printf("Trace application: %d\n", traceApplication);
  printf("C system includes: %s\n", cSysInc.toUtf8().constData());
  printf("C++ system includes: %s\n", cppSysInc.toUtf8().constData());
}

void Sdsoc::writeSdsRule(QString compiler, QFile &makefile, QString path, QString opt) {
  QStringList options;

  options << QString("-I") + this->path + "/src";

  options << opt.split(' ');

  options << "-sds-pf" << platform << "-target-os" << os << "-dmclkid" << QString::number(dmclkid);

  for(auto acc : accelerators) {
    options << "-sds-hw" << acc.name << acc.filepath << "-clkid" << QString::number(acc.clkid) << "-sds-end";
  }

  QFileInfo fileInfo(path);

  makefile.write((fileInfo.baseName() + ".o : " + path + "\n").toUtf8());
  makefile.write((QString("\t") + compiler + " " + options.join(' ') + " -c $< -o $@\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());
}

void Sdsoc::writeSdsLinkRule(QString linker, QFile &makefile, QStringList objects, QString opt) {
  QStringList options;
  options << "-sds-pf" << platform << "-target-os" << os << "-dmclkid" << QString::number(dmclkid) << opt.split(' ');

  for(auto acc : accelerators) {
    options << "-sds-hw" << acc.name << acc.filepath << "-clkid" << QString::number(acc.clkid) << "-sds-end";
  }

  makefile.write((name + ".elf : " + objects.join(' ') + "\n").toUtf8());
  makefile.write((QString("\t") + linker + " " + options.join(' ') + " $^ " + linkerOptions + " -o $@\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());
}

bool Sdsoc::getPlatformOptions() {
  QString command = "sdscc -sds-pf-info " + platform + " > __tulipp_test__.out";
  int ret = system(command.toUtf8().constData());

  if(ret) {
    QMessageBox msgBox;
    msgBox.setText("Can't open project (Can't get platform info)");
    msgBox.exec();
    return false;
  }

  QFile file("__tulipp_test__.out");

  if(file.open(QIODevice::ReadOnly)) {
    QTextStream in(&file);

    bool done = false;

    QString line;
    do {
      line = in.readLine();

      if(line.contains("Architecture: zynquplus", Qt::CaseSensitive)) {
        ultrascale = true;
        done = true;
        break;
      }
      if(line.contains("Architecture: zynq", Qt::CaseSensitive)) {
        ultrascale = false;
        done = true;
        break;
      }

    } while(!line.isNull());
    file.close();

    if(!done) {
      QMessageBox msgBox;
      msgBox.setText("Unsupported architecture");
      msgBox.exec();
      return false;
    }

  } else {
    QMessageBox msgBox;
    msgBox.setText("Can't open project (Can't get platform info)");
    msgBox.exec();
    return false;
  }

  return true;
}

bool Sdsoc::openProject(QString path, QString configType) {
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
  if(!getProjectOptions()) return false;

  { // get custom platforms
    QDomDocument doc;
    QFile file(path + "/../.metadata/.plugins/com.xilinx.sdsoc.ui/dialog_settings.xml");
    bool success = file.open(QIODevice::ReadOnly);
    if(success) {
      success = doc.setContent(&file);
      assert(success);
      file.close();

      QDomNodeList elements = doc.elementsByTagName("item");

      for(int i = 0; i < elements.size(); i++) {
        QDomElement element = elements.at(i).toElement();
        assert(!element.isNull());
        if(element.attribute("key", "") == platformKey) {
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

  if(!getPlatformOptions()) return false;

  { // get compiler/linker options
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

QString Sdsoc::processIncludePaths(QString filename) {
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

QString Sdsoc::sdsocExpand(QString text) {
  text.replace(QRegularExpression("\\${workspace_loc:*(.*)}"), Config::workspace + "\\1");
  text.replace(QRegularExpression("\\${ProjName:*(.*)}"), name + "\\1");
  return text;
}

QString Sdsoc::processCompilerOptions(QDomElement &childElement, int *optLevel) {
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

QString Sdsoc::processLinkerOptions(QDomElement &childElement) {
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

QStringList Sdsoc::getBuildConfigurations(QString path) {
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

bool Sdsoc::createMakefile() {
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

  // compile normal c files
  for(auto source : sources) {
    QFileInfo info(source);

    bool tulippCompile = createBbInfo;
    Module *mod = cfgModel->getCfg()->getModuleById(info.baseName());
    if(mod && createBbInfo) tulippCompile = !mod->hasHwCalls();

    if(info.suffix() == "c") {
      if(tulippCompile) {
        writeTulippCompileRule(Config::clang, makefile, source, cOptions + " " + cSysInc);
      } else {
        writeSdsRule("sdscc", makefile, source, cOptions);
      }
      objects << info.baseName() + ".o";

    } else if((info.suffix() == "cpp") || (info.suffix() == "cc")) {
      if(tulippCompile) {
        writeTulippCompileRule(Config::clangpp, makefile, source, cppOptions + " " + cppSysInc);
      } else {
        writeSdsRule("sds++", makefile, source, cppOptions);
      }
      objects << info.baseName() + ".o";

    }
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

  // compile at least one file with sds
  if(createBbInfo && (accelerators.size() == 0)) {
    makefile.write(QString("__tulipp__.c :\n").toUtf8());
    makefile.write(QString("\techo \"/* TULIPP dummy file */\" > __tulipp__.c\n\n").toUtf8());

    makefile.write(QString("###############################################################################\n\n").toUtf8());

    writeSdsRule("sdscc", makefile, "__tulipp__.c", cOptions);

    objects << "__tulipp__.o";
  }

  objects.removeDuplicates();

  // link
  if(isCpp) {
    writeSdsLinkRule("sds++", makefile, objects);
  } else {
    writeSdsLinkRule("sdscc", makefile, objects);
  }

  makefile.close();

  return true;
}

void Sdsoc20162::parseSynthesisReport() {
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

bool Sdsoc20162::getProjectOptions() {
  QDomDocument doc;
  QFile file(path + "/userbuildcfgs/" + configType + "_project.sdsoc");
  bool success = file.open(QIODevice::ReadOnly);
  if(!success) {
    QMessageBox msgBox;
    msgBox.setText("Can't open project (Can't find project file)");
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

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void Sdsoc20172::writeSdsRule(QString compiler, QFile &makefile, QString path, QString opt) {
  Sdsoc::writeSdsRule(compiler, makefile, path, opt + " -sds-sys-config " + sysConfig + " -sds-proc " + cpu);
}

void Sdsoc20172::writeSdsLinkRule(QString linker, QFile &makefile, QStringList objects, QString opt) {
  Sdsoc::writeSdsLinkRule(linker, makefile, objects, opt + " -sds-sys-config " + sysConfig + " -sds-proc " + cpu);
}

void Sdsoc20172::parseSynthesisReport() {
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
      if(line.contains("| CLB LUTs", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        luts = list[5].trimmed().toFloat();
      }
      if(line.contains("| DSPs", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        dsps = list[5].trimmed().toFloat();
      }
      if(line.contains("| CLB Registers", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        regs = list[5].trimmed().toFloat();
      }
    } while(!line.isNull());

    file.close();
  }
}

bool Sdsoc20172::getProjectOptions() {
  QDomDocument doc;
  QFile file(path + "/project.sdx");
  bool success = file.open(QIODevice::ReadOnly);

  if(!success) {
    QMessageBox msgBox;
    msgBox.setText("Can't open project (Can't find project file)");
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
  platform = element.attribute("platform", "");
  os = element.attribute("os", "standalone");
  cpu = element.attribute("cpu", "");
  sysConfig = element.attribute("sysConfig", "standalone");

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

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void Sdsoc20174::writeSdsRule(QString compiler, QFile &makefile, QString path, QString opt) {
  Sdsoc::writeSdsRule(compiler, makefile, path, opt + " -sds-sys-config " + sysConfig + " -sds-proc " + cpu);
}

void Sdsoc20174::writeSdsLinkRule(QString linker, QFile &makefile, QStringList objects, QString opt) {
  Sdsoc::writeSdsLinkRule(linker, makefile, objects, opt + " -sds-sys-config " + sysConfig + " -sds-proc " + cpu);
}

void Sdsoc20174::parseSynthesisReport() {
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
      if(line.contains("| CLB LUTs", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        luts = list[5].trimmed().toFloat();
      }
      if(line.contains("| DSPs", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        dsps = list[5].trimmed().toFloat();
      }
      if(line.contains("| CLB Registers", Qt::CaseSensitive)) {
        QStringList list = line.split('|');
        regs = list[5].trimmed().toFloat();
      }
    } while(!line.isNull());

    file.close();
  }
}

bool Sdsoc20174::getProjectOptions() {
  QDomDocument doc;
  QFile file(path + "/project.sdx");
  bool success = file.open(QIODevice::ReadOnly);

  if(!success) {
    QMessageBox msgBox;
    msgBox.setText("Can't open project (Can't find project file)");
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
  os = element.attribute("os", "standalone");
  cpu = element.attribute("cpu", "");
  cpuInstance = element.attribute("cpuInstance", "");
  sysConfig = element.attribute("sysConfig", "standalone");

  {
    QString p = element.attribute("platform", "");
    QFileInfo fileInfo(p);
    platform = fileInfo.path();
  }

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
            genEmulationModel = childElement2.attribute("genEmulationModel", "false") != "false";

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

  return true;
}

