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

#include "analysis_tool.h"
#include "project.h"
#include "location.h"

struct gmonhdr {
 uint64_t lpc; /* base pc address of sample buffer */
 uint64_t hpc; /* max pc address of sampled buffer */
 int32_t ncnt; /* size of sample buffer (plus this header) */
 int32_t version; /* version number */
 int32_t profrate; /* profiling clock rate */
 int32_t core;
 int32_t loops;
 int32_t spare; /* reserved */
};

struct rawarc {
 uint64_t raw_frompc;
 uint64_t raw_selfpc;
 int64_t raw_count;
};

///////////////////////////////////////////////////////////////////////////////
// makefile creation

void Project::writeTulippCompileRule(QString compiler, QFile &makefile, QString path, QString opt) {
  QFileInfo fileInfo(path);

  QString clangTarget = A9_CLANG_TARGET;
  QString llcTarget = A9_LLC_TARGET;
  QString asTarget = A9_AS_TARGET;

  QString as = Config::as;

  if(ultrascale) {
    clangTarget = A53_CLANG_TARGET;
    llcTarget = A53_LLC_TARGET;
    asTarget = A53_AS_TARGET;

    as = Config::asUs;
  }

  // .ll
  {
    QStringList options;

    options << QString("-I") + this->path + "/src";

    options << opt.split(' ');
    options << Config::extraCompileOptions.split(' ');

    if(cfgOptLevel >= 0) {
      options << QString("-O") + QString::number(cfgOptLevel);
    } else {
      options << QString("-Os");
    }
    options << clangTarget;

    makefile.write((fileInfo.completeBaseName() + ".ll : " + path + "\n").toUtf8());
    makefile.write((QString("\t") + compiler + " " + options.join(' ') + " -g -emit-llvm -S $<\n\n").toUtf8());
  }

  // .xml
  {
    makefile.write((fileInfo.completeBaseName() + ".xml : " + fileInfo.completeBaseName() + ".ll\n").toUtf8());
    makefile.write((QString("\t") + Config::llvm_ir_parser + " $< -xml $@\n\n").toUtf8());
  }

  // _2.ll
  {
    makefile.write((fileInfo.completeBaseName() + "_2.ll : " + fileInfo.completeBaseName() + ".ll\n").toUtf8());
    if(instrument) {
      makefile.write((QString("\t") + Config::llvm_ir_parser + " $< -ll $@ --instrument\n\n").toUtf8());
    } else {
      makefile.write((QString("\t") + Config::llvm_ir_parser + " $< -ll $@\n\n").toUtf8());
    }
  }

  // _3.ll
  {
    QStringList options;
    if(cppOptLevel >= 0) {
      options << QString("-O") + QString::number(cppOptLevel);
    } else {
      options << QString("-Os");
    }

    makefile.write((fileInfo.completeBaseName() + "_3.ll : " + fileInfo.completeBaseName() + "_2.ll\n").toUtf8());
    makefile.write((QString("\t") + Config::opt + " " + options.join(' ') + " $< -o $@\n\n").toUtf8());
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

    makefile.write((fileInfo.completeBaseName() + ".s : " + fileInfo.completeBaseName() + "_3.ll\n").toUtf8());
    makefile.write((QString("\t") + Config::llc + " " + options.join(' ') + " $< -o $@\n\n").toUtf8());
  }

  // .o
  bool buildIt = true;
  for(auto acc : accelerators) {
    QFileInfo info(acc.filepath);
    if(info.completeBaseName() == fileInfo.completeBaseName()) {
      buildIt = false;
      break;
    }
  }
  if(buildIt) {
    QStringList options;
    options << QString(asTarget).split(' ');

    makefile.write((fileInfo.completeBaseName() + ".o : " + fileInfo.completeBaseName() + ".s\n").toUtf8());
    makefile.write((QString("\t") + as + " " + options.join(' ') + " $< -o $@\n\n").toUtf8());
  }

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
  makefile.write(QString("\trm -rf *.ll *.xml *.s *.o *.elf *.bit sd_card _sds __tulipp__.* __tulipp_test__.* .Xil\n\n").toUtf8());

  makefile.write(QString(".PHONY : cleanbin\n").toUtf8());
  makefile.write(QString("cleanbin :\n").toUtf8());
  makefile.write(QString("\trm -rf *_2.ll *_3.ll *.s *.o *.elf __tulipp__.* __tulipp_test__.*\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());

  for(auto source : sources) {
    QFileInfo info(source);
    if(info.suffix() == "c") {
      writeTulippCompileRule(Config::clang, makefile, source, cOptions + " " + cSysInc);
      xmlFiles << info.completeBaseName() + ".xml";
    } else if((info.suffix() == "cpp") || (info.suffix() == "cc")) {
      writeTulippCompileRule(Config::clangpp, makefile, source, cppOptions + " " + cppSysInc);
      xmlFiles << info.completeBaseName() + ".xml";
    }
  }

  makefile.write(QString(".PHONY : xml\n").toUtf8());
  makefile.write((QString("xml : ") + xmlFiles.join(' ') + "\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());

  makefile.close();

  return true;
}

void Project::writeCleanRule(QFile &makefile) {
  makefile.write(QString(".PHONY : clean\n").toUtf8());
  makefile.write(QString("clean :\n").toUtf8());
  makefile.write(QString("\trm -rf *.ll *.xml *.s *.o *.elf *.bit sd_card _sds __tulipp__.* __tulipp_test__.* .Xil\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());
}

bool Project::createMakefile(QFile &makefile) {
  makefile.write(QString("###################################################\n").toUtf8());
  makefile.write(QString("# Autogenerated Makefile for TULIPP Analysis Tool #\n").toUtf8());
  makefile.write(QString("###################################################\n\n").toUtf8());

  makefile.write(QString(".PHONY : binary\n").toUtf8());
  makefile.write((QString("binary : ") + name + ".elf\n\n").toUtf8());

  makefile.write(QString("###############################################################################\n\n").toUtf8());

  writeCleanRule(makefile);

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// make

bool Project::makeXml() {
  emit advance(0, "Building XML");

  bool created = createXmlMakefile();
  if(created) errorCode = system(QString("make -j xml").toUtf8().constData());
  else errorCode = 1;

  if(!created || errorCode) {
    emit finished(errorCode, "Can't make XML");
  } else {
    emit finished(errorCode, "");
  }

  return errorCode == 0;
}

bool Project::makeBin() {
  emit advance(0, "Building XML");

  bool created = createXmlMakefile();
  if(created) errorCode = system(QString("make -j xml").toUtf8().constData());
  else errorCode = 1;

  if(!created || errorCode) {
    emit finished(errorCode, "Can't make XML");
    return false;
  }

  loadFiles();

  emit advance(1, "Building binary");

  created = createMakefile();
  if(created) errorCode = system(QString("make -j binary").toUtf8().constData());
  else errorCode = 1;

  if(!created || errorCode) {
    emit finished(errorCode, "Can't make binary");
  } else {
    emit finished(errorCode, "");
  }

  return errorCode == 0;
}

bool Project::clean() {
  if(opened) {
    createXmlMakefile();
    errorCode = system("make clean");
  }
  return true;
}

bool Project::cleanBin() {
  if(opened) {
    createXmlMakefile();
    errorCode = system("make cleanbin");
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////

void Project::loadProjectFile() {
  QSettings settings("project.ini", QSettings::IniFormat);

  cfgOptLevel = settings.value("cfgOptLevel", "-1").toInt();
  createBbInfo = settings.value("createBbInfo", true).toBool();
  systemXmls = settings.value("systemXmls").toStringList();
  for(unsigned i = 0; i < pmu.numSensors(); i++) {
    pmu.supplyVoltage[i] = settings.value("supplyVoltage" + QString::number(i), 5).toDouble();
  }
  pmu.rl[0] = settings.value("rl0", 0.025).toDouble();
  pmu.rl[1] = settings.value("rl1", 0.05).toDouble();
  pmu.rl[2] = settings.value("rl2", 0.05).toDouble();
  pmu.rl[3] = settings.value("rl3", 0.1).toDouble();
  pmu.rl[4] = settings.value("rl4", 0.1).toDouble();
  pmu.rl[5] = settings.value("rl5", 1).toDouble();
  pmu.rl[6] = settings.value("rl6", 10).toDouble();
  useCustomElf = settings.value("useCustomElf", false).toBool();
  samplePc = settings.value("samplePc", true).toBool();
  useBp = settings.value("useBp", true).toBool();
  samplingModeGpio = settings.value("samplingModeGpio", false).toBool();
  samplePeriod = settings.value("samplePeriod", 0).toLongLong();
  customElfFile = settings.value("customElfFile", "").toString();
  startFunc = settings.value("startFunc", "main").toString();
  startCore = settings.value("startCore", 0).toUInt();
  stopFunc = settings.value("stopFunc", "_exit").toString();
  stopCore = settings.value("stopCore", 0).toUInt();
  instrument = settings.value("instrument", false).toBool();

  if(!isSdSocProject()) {
    ultrascale = settings.value("ultrascale", true).toBool();
    sources = settings.value("sources").toStringList();
    cOptLevel = settings.value("cOptLevel", 0).toInt();
    cOptions = settings.value("cOptions", "").toString();
    cppOptLevel = settings.value("cppOptLevel", 0).toInt();
    cppOptions = settings.value("cppOptions", "").toString();
    linkerOptions = settings.value("linkerOptions", "").toString();
    tcfUploadScript = settings.value("tcfUploadScript", "").toString();

  } else {
    if(ultrascale) {
      tcfUploadScript = settings.value("tcfUploadScript", DEFAULT_TCF_UPLOAD_SCRIPT_US).toString();
    } else {
      tcfUploadScript = settings.value("tcfUploadScript", DEFAULT_TCF_UPLOAD_SCRIPT).toString();
    }
  }

  if(Config::overrideSamplePc) samplePc = true;
  if(Config::overrideNoSamplePc) samplePc = false;
  if(Config::overrideSamplePeriod) samplePeriod = Config::overrideSamplePeriod;
}

void Project::saveProjectFile() {
  QSettings settings("project.ini", QSettings::IniFormat);

  settings.setValue("cfgOptLevel", cfgOptLevel);
  settings.setValue("createBbInfo", createBbInfo);
  settings.setValue("systemXmls", systemXmls);
  for(unsigned i = 0; i < pmu.numSensors(); i++) { 
    settings.setValue("supplyVoltage" + QString::number(i), pmu.supplyVoltage[i]);
    settings.setValue("rl" + QString::number(i), pmu.rl[i]);
  }
  settings.setValue("ultrascale", ultrascale);
  settings.setValue("tcfUploadScript", tcfUploadScript);
  settings.setValue("useCustomElf", useCustomElf);
  settings.setValue("customElfFile", customElfFile);
  settings.setValue("samplePc", samplePc);
  settings.setValue("useBp", useBp);
  settings.setValue("samplingModeGpio", samplingModeGpio);
  settings.setValue("samplePeriod", (qint64)samplePeriod);
  settings.setValue("startFunc", startFunc);
  settings.setValue("startCore", startCore);
  settings.setValue("stopFunc", stopFunc);
  settings.setValue("stopCore", stopCore);
  settings.setValue("instrument", instrument);

  settings.setValue("sources", sources);
  settings.setValue("cOptLevel", cOptLevel);
  settings.setValue("cOptions", cOptions);
  settings.setValue("cppOptLevel", cppOptLevel);
  settings.setValue("cppOptions", cppOptions);
  settings.setValue("linkerOptions", linkerOptions);
}

///////////////////////////////////////////////////////////////////////////////
// transform source

int Project::runSourceTool(QString inputFilename, QString outputFilename, QStringList loopsToPipeline, QString opt) {
  QStringList options;

  options << inputFilename;

  options << QString("-extra-arg=-I") + this->path + "/src";

  for(auto x : opt.split(' ')) {
    if(x.trimmed() != "") {
      options << QString("-extra-arg=") + x;
    }
  }

  for(auto l : loopsToPipeline) {
    options << QString("-pipeloop=") + l;
  }

  options << QString("-output " + outputFilename);

  options << QString("--");

  return system((Config::tulipp_source_tool + " " + options.join(' ')).toUtf8().constData());
}

///////////////////////////////////////////////////////////////////////////////
// project files

void Project::loadFiles() {
  if(cfg) delete cfg;
  cfg = new Cfg();

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
    QDomElement element = doc.documentElement();
    QString moduleName = element.attribute(ATTR_ID, "");
    QString fileName = element.attribute(ATTR_FILE, "");

    Module *module = new Module(moduleName, cfg, fileName);

    module->constructFromXml(element, 0, this);
    module->buildEdgeList();
    module->buildExitNodes();
    module->buildEntryNodes();

    for(auto acc : accelerators) {
      QFileInfo fileInfo(acc.filepath);
      if(fileInfo.completeBaseName() == moduleName) {
        QVector<Function*> functions = module->getFunctionsByName(acc.name);
        assert(functions.size() == 1);
        functions.at(0)->hw = true;
      }
    }

    for(auto func : module->children) {
      Function *f = static_cast<Function*>(func);
      f->cycleRemoval();
    }

    cfg->appendChild(module);

    cfg->clearCallers();
    cfg->calculateCallers();

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
  cfg = NULL;
  close();
}

void Project::copy(Project *p) {
  opened = p->opened;
  isCpp = p->isCpp;

  path = p->path;

  systemXmls = p->systemXmls;
  cfgOptLevel = p->cfgOptLevel;
  tcfUploadScript = p->tcfUploadScript;
  //pmu = p->pmu;
  ultrascale = p->ultrascale;
  startFunc = p->startFunc;
  startCore = p->startCore;
  stopFunc = p->stopFunc;
  stopCore = p->stopCore;
  createBbInfo = p->createBbInfo;
  useCustomElf = p->useCustomElf;
  
  // settings from either sdsoc project or user
  sources = p->sources;
  name = p->name;
  cOptions = p->cOptions;
  cOptLevel = p->cOptLevel;
  cppOptions = p->cppOptions;
  cppOptLevel = p->cppOptLevel;
  linkerOptions = p->linkerOptions;

  // settings from sdsoc project, unused otherwise
  configType = p->configType;
  accelerators = p->accelerators;
  cSysInc = p->cSysInc;
  cppSysInc = p->cppSysInc;
  
  QString customElfFile;
  elfFile = p->elfFile;

  cfg = NULL;
}

Project::Project(Project *p) {
  copy(p);
}

Project::~Project() {
  delete cfg;
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

  if(cfg) delete cfg;
  cfg = new Cfg();
}

void Project::print() {
  printf("Project: %s\n", name.toUtf8().constData());
  printf("Path: %s\n", path.toUtf8().constData());

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

  printf("Architecture: %s\n", ultrascale ? "Zynq Ultrascale+" : "Zynq");
}

///////////////////////////////////////////////////////////////////////////////
// profiling support

Location *Project::getLocation(unsigned core, uint64_t pc, ElfSupport *elfSupport, std::map<BasicBlock*,Location*> *locations) {
  BasicBlock *bb = NULL;
  Function *func = NULL;

  if(!useCustomElf && elfSupport->isBb(pc)) {
    Module *mod = cfg->getModuleById(elfSupport->getModuleId(pc));
    if(mod) bb = mod->getBasicBlockById(QString::number(elfSupport->getLineNumber(pc)));

  } else {
    QString functionId;

    if(core != 0) {
      functionId = QString("CPU") + QString::number(core) + "_" + elfSupport->getFunction(pc);
    } else {
      functionId = elfSupport->getFunction(pc);
    }

    func = cfg->getFunctionById(functionId);

    if(func) {
      // we don't know the BB, but the function exists in the CFG: add to first BB
      bb = func->getFirstBb();
      func = NULL;

    } else {
      // the function does not exist in the CFG
      Module *mod = cfg->externalMod;
      func = mod->getFunctionById(functionId);

      if(func) {
        bb = static_cast<BasicBlock*>(func->children[0]);
      } else {
        func = new Function(functionId, mod, mod->children.size());
        mod->appendChild(func);

        bb = new BasicBlock(QString::number(mod->children.size()), func, 0);
        func->appendChild(bb);
      }
    }
  }

  QString bbText = "";
  QString modText = "";
  QString funcText = "";

  if(func) {
    funcText = func->id;
  }

  assert(bb);

  bbText = bb->id;
  modText = bb->getModule()->id;

  Location *location;

  if(locations) {
    auto it = locations->find(bb);
    if(it != locations->end()) {
      location = it->second;
    } else {
      location = new Location(modText, funcText, bbText, bb);
      (*locations)[bb] = location;
    }
  } else {
    location = new Location(modText, funcText, bbText, bb);
  }

  return location;
}

void Project::getLocations(unsigned core, std::map<BasicBlock*,Location*> *locations) {
  QSqlQuery query;
  query.exec("SELECT * FROM location WHERE core = " + QString::number(core));

  while(query.next()) {
    int id = query.value("id").toInt();

    QString modId = query.value("module").toString();
    QString funcId = query.value("function").toString();
    QString bbId = query.value("basicblock").toString();

    Module *mod = cfg->getModuleById(modId);
    assert(mod);
    BasicBlock *bb = mod->getBasicBlockById(bbId);
    assert(bb);

    Location *location = new Location(id, modId, funcId, bbId, bb);
    (*locations)[bb] = location;
  }

  query.exec("SELECT * FROM location ORDER BY id DESC LIMIT 1");
  if(query.next()) {
    Location::idCounter = query.value(0).toInt() + 1;
  }
}

bool Project::parseGProfFile(QString gprofFileName, QString elfFileName, Profile *profile) {
  QSqlQuery query;

  ElfSupport elfSupport;
  elfSupport.addElf(elfFileName);

  QFile file(gprofFileName);
  if(!file.open(QIODevice::ReadOnly)) {
    QMessageBox msgBox;
    msgBox.setText("File not found");
    msgBox.exec();
    return false;
  }

  query.exec("DELETE FROM arc");

  std::map<BasicBlock*,Location*> locations;

  QSqlDatabase::database().transaction();

  struct gmonhdr hdr;
  file.read((char*)&hdr, sizeof(struct gmonhdr));

  unsigned core = hdr.core;

  getLocations(core, &locations);

  for(int i = 0; i < hdr.loops; i++) {
    uint64_t pc;
    uint64_t count;

    file.read((char*)&pc, sizeof(uint64_t));
    file.read((char*)&count, sizeof(uint64_t));

    Location *loop = getLocation(core, pc, &elfSupport, &locations);
    loop->loopCount = count;
  }

  while(!file.atEnd()) {
    struct rawarc arc;
    file.read((char*)&arc, sizeof(struct rawarc));

    Location *from = getLocation(core, arc.raw_frompc-4, &elfSupport, &locations);
    Location *self = getLocation(core, arc.raw_selfpc, &elfSupport, &locations);

    if(!from->bb->containsFunctionCall(self->bb->getFunction())) {
      printf("Warning: Can't find arc %s:%s:%s - %s\n",
             from->bb->getModule()->id.toUtf8().constData(),
             from->bb->getFunction()->id.toUtf8().constData(),
             from->bb->id.toUtf8().constData(),
             self->bb->getFunction()->id.toUtf8().constData());
    }

    self->addCaller(from->id, arc.raw_count);
  }

  for(auto location : locations) {
    if(!location.second->inDb) {
      QSqlQuery query;

      query.prepare("INSERT INTO location (id,core,basicblock,function,module,runtime,energy1,energy2,energy3,energy4,energy5,energy6,energy7,loopcount) "
                    "VALUES (:id,:core,:basicblock,:function,:module,:runtime,:energy1,:energy2,:energy3,:energy4,:energy5,:energy6,:energy7,:loopcount)");

      query.bindValue(":id", location.second->id);
      query.bindValue(":core", core);
      query.bindValue(":basicblock", location.second->bbId);
      query.bindValue(":function", location.second->funcId);
      query.bindValue(":module", location.second->moduleId);
      query.bindValue(":runtime", location.second->runtime);
      query.bindValue(":energy1", location.second->energy[0]);
      query.bindValue(":energy2", location.second->energy[1]);
      query.bindValue(":energy3", location.second->energy[2]);
      query.bindValue(":energy4", location.second->energy[3]);
      query.bindValue(":energy5", location.second->energy[4]);
      query.bindValue(":energy6", location.second->energy[5]);
      query.bindValue(":energy7", location.second->energy[6]);
      query.bindValue(":loopcount", (qulonglong)location.second->loopCount);

      bool success = query.exec();
      assert(success);
    } else {
      QSqlQuery query;

      query.prepare("UPDATE location SET loopcount=:loopcount WHERE id=:id");

      query.bindValue(":loopcount", (qulonglong)location.second->loopCount);
      query.bindValue(":id", location.second->id);

      bool success = query.exec();
      assert(success);
    }

    for(auto it : location.second->callers) {
      QSqlQuery arcQuery;
      int caller = it.first;
      int count =  it.second;

      arcQuery.prepare("INSERT INTO arc (fromid,selfid,num) VALUES (:fromid,:selfid,:num)");

      arcQuery.bindValue(":fromid", caller);
      arcQuery.bindValue(":selfid", location.second->id);
      arcQuery.bindValue(":num", count);

      bool success = arcQuery.exec();
      assert(success);
    }

    delete location.second;
  }

  QSqlDatabase::database().commit();

  return true;
}

bool Project::parseProfFile(QString fileName, Profile *profile) {
  QSqlQuery query;

  ElfSupport elfSupport;
  if(!useCustomElf) elfSupport.addElf(elfFile);
  for(auto ef : customElfFile.split(',')) {
    elfSupport.addElf(ef);
  }

  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly)) {
    QMessageBox msgBox;
    msgBox.setText("File not found");
    msgBox.exec();
    return false;
  }

  profile->clean();

  std::map<BasicBlock*,Location*> locations;

  unsigned core = 0;
  unsigned sensor = 0;
  double calData[7];
  double period = 0;
  double unknownPower = 0;
  double unknownRuntime = 0;
  uint32_t count = 0;

  file.read((char*)&core, sizeof(uint8_t));
  file.read((char*)&sensor, sizeof(uint8_t));
  for(int i = 0; i < 7; i++) {
    file.read((char*)&calData[i], sizeof(double));
  }
  file.read((char*)&period, sizeof(double));
  file.read((char*)&unknownPower, sizeof(double));
  file.read((char*)&unknownRuntime, sizeof(double));
  file.read((char*)&count, sizeof(uint32_t));

  unknownPower = Pmu::currentToPower(sensor, unknownPower, pmu.rl, pmu.supplyVoltage, calData);

  QSqlDatabase::database().transaction();

  if((unknownPower != 0) || (unknownRuntime != 0)) {
    double energy[7] = {0, 0, 0, 0, 0, 0, 0};
    if(unknownRuntime) {
      energy[sensor] = unknownPower * unknownRuntime;
    }

    query.prepare("INSERT INTO location (id,core,basicblock,function,module,runtime,energy1,energy2,energy3,energy4,energy5,energy6,energy7,loopcount) "
                  "VALUES (:id,:core,:basicblock,:function,:module,:runtime,:energy1,:energy2,:energy3,:energy4,:energy5,:energy6,:energy7,:loopcount)");

    query.bindValue(":id", 0);
    query.bindValue(":core", core);
    query.bindValue(":basicblock", "uknown-bb");
    query.bindValue(":function", "Unknown");
    query.bindValue(":module", cfg->externalMod->id);
    query.bindValue(":runtime", unknownRuntime);
    query.bindValue(":energy1", energy[0]);
    query.bindValue(":energy2", energy[1]);
    query.bindValue(":energy3", energy[2]);
    query.bindValue(":energy4", energy[3]);
    query.bindValue(":energy5", energy[4]);
    query.bindValue(":energy6", energy[5]);
    query.bindValue(":energy7", energy[6]);
    query.bindValue(":loopcount", 0);

    bool success = query.exec();
    assert(success);
  }

  double totalRuntime = 0;
  double totalEnergy[7] = {0, 0, 0, 0, 0, 0, 0};

  for(uint32_t i = 0; i < count; i++) {
    uint64_t pc;
    double power;
    double runtime;

    file.read((char*)&pc, sizeof(uint64_t));
    file.read((char*)&power, sizeof(double));
    file.read((char*)&runtime, sizeof(double));

    Location *location = getLocation(core, pc, &elfSupport, &locations);

    power = Pmu::currentToPower(sensor, power, pmu.rl, pmu.supplyVoltage, calData);

    if(runtime) {
      location->energy[sensor] += power * runtime;
      location->runtime += runtime;

      totalEnergy[sensor] += power * runtime;
      totalRuntime += runtime;
    }
  }

  for(auto location : locations) {
    QSqlQuery query;

    query.prepare("INSERT INTO location (id,core,basicblock,function,module,runtime,energy1,energy2,energy3,energy4,energy5,energy6,energy7,loopcount) "
                  "VALUES (:id,:core,:basicblock,:function,:module,:runtime,:energy1,:energy2,:energy3,:energy4,:energy5,:energy6,:energy7,:loopcount)");

    query.bindValue(":id", location.second->id);
    query.bindValue(":core", core);
    query.bindValue(":basicblock", location.second->bbId);
    query.bindValue(":function", location.second->funcId);
    query.bindValue(":module", location.second->moduleId);
    query.bindValue(":runtime", location.second->runtime);
    query.bindValue(":energy1", location.second->energy[0]);
    query.bindValue(":energy2", location.second->energy[1]);
    query.bindValue(":energy3", location.second->energy[2]);
    query.bindValue(":energy4", location.second->energy[3]);
    query.bindValue(":energy5", location.second->energy[4]);
    query.bindValue(":energy6", location.second->energy[5]);
    query.bindValue(":energy7", location.second->energy[6]);
    query.bindValue(":loopcount", 0);

    bool success = query.exec();
    assert(success);

    delete location.second;
  }

  QSqlDatabase::database().commit();

  QSqlDatabase::database().transaction();

  query.prepare("INSERT INTO meta"
                " (samples,minTime,maxTime,minPower1,minPower2,minPower3,minPower4,minPower5,minPower6,minPower7,"
                "maxPower1,maxPower2,maxPower3,maxPower4,maxPower5,maxPower6,maxPower7,"
                "runtime,energy1,energy2,energy3,energy4,energy5,energy6,energy7)"
                " VALUES (:samples,:minTime,:maxTime,:minPower1,:minPower2,:minPower3,:minPower4,:minPower5,:minPower6,:minPower7,"
                ":maxPower1,:maxPower2,:maxPower3,:maxPower4,:maxPower5,:maxPower6,:maxPower7,"
                ":runtime,:energy1,:energy2,:energy3,:energy4,:energy5,:energy6,:energy7)");

  query.bindValue(":samples", 0);
  query.bindValue(":minTime", 0);
  query.bindValue(":maxTime", (qulonglong)pmu.secondsToCycles(totalRuntime));
  query.bindValue(":minPower1", 0);
  query.bindValue(":minPower2", 0);
  query.bindValue(":minPower3", 0);
  query.bindValue(":minPower4", 0);
  query.bindValue(":minPower5", 0);
  query.bindValue(":minPower6", 0);
  query.bindValue(":minPower7", 0);
  query.bindValue(":maxPower1", 0);
  query.bindValue(":maxPower2", 0);
  query.bindValue(":maxPower3", 0);
  query.bindValue(":maxPower4", 0);
  query.bindValue(":maxPower5", 0);
  query.bindValue(":maxPower6", 0);
  query.bindValue(":maxPower7", 0);
  query.bindValue(":runtime", totalRuntime);
  query.bindValue(":energy1", totalEnergy[0]);
  query.bindValue(":energy2", totalEnergy[1]);
  query.bindValue(":energy3", totalEnergy[2]);
  query.bindValue(":energy4", totalEnergy[3]);
  query.bindValue(":energy5", totalEnergy[4]);
  query.bindValue(":energy6", totalEnergy[5]);
  query.bindValue(":energy7", totalEnergy[6]);

  bool success = query.exec();
  assert(success);

  QSqlDatabase::database().commit();

  return true;
}

bool Project::runProfiler() {

  ElfSupport elfSupport;
  if(!useCustomElf) elfSupport.addElf(elfFile);
  for(auto ef : customElfFile.split(',')) {
    elfSupport.addElf(ef);
  }

  bool pmuInited = pmu.init();
  if(!pmuInited) {
    emit finished(1, "Can't connect to PMU");
    return false;
  }

  if(!useBp) {
    emit advance(0, "Skipping upload");

  } else {
    emit advance(0, "Uploading binary");

    // upload binaries
    QFile tclFile("temp-pmu-prof.tcl");
    bool success = tclFile.open(QIODevice::WriteOnly);
    Q_UNUSED(success);
    assert(success);

    QString tcl = QString() + "set name " + name + "\n" + tcfUploadScript;
      
    tclFile.write(tcl.toUtf8());

    tclFile.close();

    int ret = system("xsct temp-pmu-prof.tcl");
    if(ret) {
      emit finished(1, "Can't upload binaries");
      pmu.release();
      return false;
    }
  }

  uint64_t samples;
  int64_t minTime;
  int64_t maxTime;
  double minPower[LYNSYN_SENSORS];
  double maxPower[LYNSYN_SENSORS];
  double runtime;
  double energy[LYNSYN_SENSORS];

  // collect samples
  {
    emit advance(1, "Collecting samples");

    uint64_t startAddr = 0;
    uint64_t stopAddr = 0;

    if(useBp) {
      startAddr = elfSupport.lookupSymbol(startFunc);
      stopAddr = elfSupport.lookupSymbol(stopFunc);
    }

    bool usePeriod = samplePeriod != 0;

    bool useFrame = true;
    uint64_t frameAddr = elfSupport.lookupSymbol("__tulippFrameDone");

    pmu.collectSamples(useFrame, frameAddr, useBp, samplePc, samplingModeGpio, usePeriod, 
                       samplePeriod, startCore, startAddr, stopCore, stopAddr,
                       &samples, &minTime, &maxTime, minPower, maxPower, &runtime, energy);
  }

  pmu.release();

  int64_t frameRuntimeMin = 0;
  int64_t frameRuntimeMax = 0;
  int64_t frameRuntimeAvg = 0;

  // find all frames
  QVector<int64_t> frames;
  unsigned frameCount = 0;
  {
    int64_t lastTime = 0;
    QSqlQuery query;
    query.exec("SELECT time,delay FROM frames");
    while(query.next()) {
      int64_t time = query.value("time").toLongLong();
      int64_t delay = query.value("delay").toLongLong();
      frames.push_back(time);

      if(lastTime) {
        int64_t frameRuntime = time - lastTime - delay;
        if(frameRuntime > frameRuntimeMax) frameRuntimeMax = frameRuntime;
        if((frameRuntimeMin == 0) || (frameRuntime < frameRuntimeMin)) frameRuntimeMin = frameRuntime;
        frameRuntimeAvg += frameRuntime;
        frameCount++;
      }
      
      lastTime = time;
    }
  }

  if(frameCount) frameRuntimeAvg /= frameCount;
  
  double frameEnergyMin[LYNSYN_SENSORS];
  double frameEnergyMax[LYNSYN_SENSORS];
  double frameEnergyAvg[LYNSYN_SENSORS];
  double currentFrameEnergy[LYNSYN_SENSORS];

  {
    emit advance(2, "Processing samples");

    std::map<BasicBlock*,Location*> locations[LYNSYN_MAX_CORES];

    QSqlQuery query;
    query.setForwardOnly(true);
    query.exec("SELECT rowid,time,timeSinceLast,pc1,pc2,pc3,pc4,power1,power2,power3,power4,power5,power6,power7 FROM measurements");

    int counter = 0;

    QSqlDatabase::database().transaction();

    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE measurements SET"
                        " basicblock1=:basicblock1,module1=:module1"
                        ", basicblock2=:basicblock2,module2=:module2"
                        ", basicblock3=:basicblock3,module3=:module3"
                        ", basicblock4=:basicblock4,module4=:module4"
                        " WHERE rowid = :rowid");

    int currentFrame = 0;

    while(query.next()) {
      if(counter && ((counter % 10000) == 0)) printf("Processed %d samples...\n", counter);
      counter++;

      unsigned rowId = query.value("rowid").toUInt();

      int64_t time = query.value("time").toLongLong();

      if(currentFrame < frames.size()) {
        if(time > frames[currentFrame]) {
          currentFrame++;

          if(currentFrame == 1) {
            // first frame
            for(int i = 0; i < LYNSYN_SENSORS; i++) {
              frameEnergyMin[i] = 0;
              frameEnergyMax[i] = 0;
              frameEnergyAvg[i] = 0;
              currentFrameEnergy[i] = 0;
            }

          } else {
            // next frame
            for(int core = 0; core < 4; core++) for(auto location : locations[core]) location.second->addToAvg(frames.size()-1);
            for(int i = 0; i < LYNSYN_SENSORS; i++) {
              if(currentFrameEnergy[i] > frameEnergyMax[i]) frameEnergyMax[i] = currentFrameEnergy[i];
              if((frameEnergyMin[i] == 0) || (currentFrameEnergy[i] < frameEnergyMin[i])) frameEnergyMin[i] = currentFrameEnergy[i];
              frameEnergyAvg[i] += currentFrameEnergy[i];
              currentFrameEnergy[i] = 0;
            }
          }

          for(int core = 0; core < 4; core++) for(auto location : locations[core]) location.second->clearFrameData();
        }
      }

      QString bbText[4];
      QString modText[4];

      uint64_t pc[4];
      pc[0] = query.value("pc1").toULongLong();
      pc[1] = query.value("pc2").toULongLong();
      pc[2] = query.value("pc3").toULongLong();
      pc[3] = query.value("pc4").toULongLong();

      int64_t timeSinceLast = query.value("timeSinceLast").toLongLong();

      double power[7];
      power[0] = query.value("power1").toDouble();
      power[1] = query.value("power2").toDouble();
      power[2] = query.value("power3").toDouble();
      power[3] = query.value("power4").toDouble();
      power[4] = query.value("power5").toDouble();
      power[5] = query.value("power6").toDouble();
      power[6] = query.value("power7").toDouble();

      for(int i = 0; i < LYNSYN_SENSORS; i++) currentFrameEnergy[i] += power[i] * Pmu::cyclesToSeconds(timeSinceLast);

      for(int core = 0; core < 4; core++) {
        Location *location = getLocation(core, pc[core], &elfSupport, &locations[core]);

        bbText[core] = location->bbId;
        modText[core] = location->moduleId;

        location->updateRuntime(Pmu::cyclesToSeconds(timeSinceLast));
        for(int sensor = 0; sensor < 7; sensor++) {
          location->updateEnergy(sensor, power[sensor] * Pmu::cyclesToSeconds(timeSinceLast));
        }
      }

      updateQuery.bindValue(":rowid", rowId);

      updateQuery.bindValue(":basicblock1", bbText[0]);
      updateQuery.bindValue(":basicblock2", bbText[1]);
      updateQuery.bindValue(":basicblock3", bbText[2]);
      updateQuery.bindValue(":basicblock4", bbText[3]);

      updateQuery.bindValue(":module1", modText[0]);
      updateQuery.bindValue(":module2", modText[1]);
      updateQuery.bindValue(":module3", modText[2]);
      updateQuery.bindValue(":module4", modText[3]);

      bool success = updateQuery.exec();
      assert(success);
    }

    if(frameCount) {
      for(int i = 0; i < LYNSYN_SENSORS; i++) {
        frameEnergyAvg[i] /= frameCount;
      }
    }

    printf("Processed %d samples...\n", counter);

    QSqlDatabase::database().commit();

    QSqlDatabase::database().transaction();

    for(unsigned c = 0; c < LYNSYN_MAX_CORES; c++) {
      for(auto location : locations[c]) {
        QSqlQuery query;

        query.prepare("INSERT INTO location (core,basicblock,function,module,"
                      "runtime,energy1,energy2,energy3,energy4,energy5,energy6,energy7,"
                      "runtimeFrame,energyFrame1,energyFrame2,energyFrame3,energyFrame4,energyFrame5,energyFrame6,energyFrame7,"
                      "loopcount) "
                      "VALUES (:core,:basicblock,:function,:module,"
                      ":runtime,:energy1,:energy2,:energy3,:energy4,:energy5,:energy6,:energy7,"
                      ":runtimeFrame,:energyFrame1,:energyFrame2,:energyFrame3,:energyFrame4,:energyFrame5,:energyFrame6,:energyFrame7,"
                      ":loopcount)");

        query.bindValue(":core", c);
        query.bindValue(":basicblock", location.second->bbId);
        query.bindValue(":function", location.second->funcId);
        query.bindValue(":module", location.second->moduleId);
        query.bindValue(":runtime", location.second->runtime);
        query.bindValue(":energy1", location.second->energy[0]);
        query.bindValue(":energy2", location.second->energy[1]);
        query.bindValue(":energy3", location.second->energy[2]);
        query.bindValue(":energy4", location.second->energy[3]);
        query.bindValue(":energy5", location.second->energy[4]);
        query.bindValue(":energy6", location.second->energy[5]);
        query.bindValue(":energy7", location.second->energy[6]);
        query.bindValue(":runtimeFrame", location.second->runtimeFrameAvg);
        query.bindValue(":energyFrame1", location.second->energyFrameAvg[0]);
        query.bindValue(":energyFrame2", location.second->energyFrameAvg[1]);
        query.bindValue(":energyFrame3", location.second->energyFrameAvg[2]);
        query.bindValue(":energyFrame4", location.second->energyFrameAvg[3]);
        query.bindValue(":energyFrame5", location.second->energyFrameAvg[4]);
        query.bindValue(":energyFrame6", location.second->energyFrameAvg[5]);
        query.bindValue(":energyFrame7", location.second->energyFrameAvg[6]);
        query.bindValue(":loopcount", 0);

        bool success = query.exec();
        assert(success);

        delete location.second;
      }
    }

    QSqlDatabase::database().commit();

    QSqlDatabase::database().transaction();

    query.prepare("INSERT INTO meta ("
                  "samples,minTime,maxTime,minPower1,minPower2,minPower3,minPower4,minPower5,minPower6,minPower7,"
                  "maxPower1,maxPower2,maxPower3,maxPower4,maxPower5,maxPower6,maxPower7,"
                  "runtime,energy1,energy2,energy3,energy4,energy5,energy6,energy7,"
                  "frameRuntimeMin,frameRuntimeAvg,frameRuntimeMax,"
                  "frameEnergyMin1,frameEnergyAvg1,frameEnergyMax1,"
                  "frameEnergyMin2,frameEnergyAvg2,frameEnergyMax2,"
                  "frameEnergyMin3,frameEnergyAvg3,frameEnergyMax3,"
                  "frameEnergyMin4,frameEnergyAvg4,frameEnergyMax4,"
                  "frameEnergyMin5,frameEnergyAvg5,frameEnergyMax5,"
                  "frameEnergyMin6,frameEnergyAvg6,frameEnergyMax6,"
                  "frameEnergyMin7,frameEnergyAvg7,frameEnergyMax7"
                  ") VALUES ("
                  ":samples,:minTime,:maxTime,:minPower1,:minPower2,:minPower3,:minPower4,:minPower5,:minPower6,:minPower7,"
                  ":maxPower1,:maxPower2,:maxPower3,:maxPower4,:maxPower5,:maxPower6,:maxPower7,"
                  ":runtime,:energy1,:energy2,:energy3,:energy4,:energy5,:energy6,:energy7,"
                  ":frameRuntimeMin,:frameRuntimeAvg,:frameRuntimeMax,"
                  ":frameEnergyMin1,:frameEnergyAvg1,:frameEnergyMax1,"
                  ":frameEnergyMin2,:frameEnergyAvg2,:frameEnergyMax2,"
                  ":frameEnergyMin3,:frameEnergyAvg3,:frameEnergyMax3,"
                  ":frameEnergyMin4,:frameEnergyAvg4,:frameEnergyMax4,"
                  ":frameEnergyMin5,:frameEnergyAvg5,:frameEnergyMax5,"
                  ":frameEnergyMin6,:frameEnergyAvg6,:frameEnergyMax6,"
                  ":frameEnergyMin7,:frameEnergyAvg7,:frameEnergyMax7"
                  ")");

    query.bindValue(":samples", (quint64)samples);
    query.bindValue(":minTime", (qint64)minTime);
    query.bindValue(":maxTime", (qint64)maxTime);
    query.bindValue(":minPower1", minPower[0]);
    query.bindValue(":minPower2", minPower[1]);
    query.bindValue(":minPower3", minPower[2]);
    query.bindValue(":minPower4", minPower[3]);
    query.bindValue(":minPower5", minPower[4]);
    query.bindValue(":minPower6", minPower[5]);
    query.bindValue(":minPower7", minPower[6]);
    query.bindValue(":maxPower1", maxPower[0]);
    query.bindValue(":maxPower2", maxPower[1]);
    query.bindValue(":maxPower3", maxPower[2]);
    query.bindValue(":maxPower4", maxPower[3]);
    query.bindValue(":maxPower5", maxPower[4]);
    query.bindValue(":maxPower6", maxPower[5]);
    query.bindValue(":maxPower7", maxPower[6]);
    query.bindValue(":runtime", runtime);
    query.bindValue(":energy1", energy[0]);
    query.bindValue(":energy2", energy[1]);
    query.bindValue(":energy3", energy[2]);
    query.bindValue(":energy4", energy[3]);
    query.bindValue(":energy5", energy[4]);
    query.bindValue(":energy6", energy[5]);
    query.bindValue(":energy7", energy[6]);
    query.bindValue(":frameRuntimeMin", Pmu::cyclesToSeconds(frameRuntimeMin));
    query.bindValue(":frameRuntimeAvg", Pmu::cyclesToSeconds(frameRuntimeAvg));
    query.bindValue(":frameRuntimeMax", Pmu::cyclesToSeconds(frameRuntimeMax));
    query.bindValue(":frameEnergyMin1", frameEnergyMin[0]);
    query.bindValue(":frameEnergyAvg1", frameEnergyAvg[0]);
    query.bindValue(":frameEnergyMax1", frameEnergyMax[0]);
    query.bindValue(":frameEnergyMin2", frameEnergyMin[1]);
    query.bindValue(":frameEnergyAvg2", frameEnergyAvg[1]);
    query.bindValue(":frameEnergyMax2", frameEnergyMax[1]);
    query.bindValue(":frameEnergyMin3", frameEnergyMin[2]);
    query.bindValue(":frameEnergyAvg3", frameEnergyAvg[2]);
    query.bindValue(":frameEnergyMax3", frameEnergyMax[2]);
    query.bindValue(":frameEnergyMin4", frameEnergyMin[3]);
    query.bindValue(":frameEnergyAvg4", frameEnergyAvg[3]);
    query.bindValue(":frameEnergyMax4", frameEnergyMax[3]);
    query.bindValue(":frameEnergyMin5", frameEnergyMin[4]);
    query.bindValue(":frameEnergyAvg5", frameEnergyAvg[4]);
    query.bindValue(":frameEnergyMax5", frameEnergyMax[4]);
    query.bindValue(":frameEnergyMin6", frameEnergyMin[5]);
    query.bindValue(":frameEnergyAvg6", frameEnergyAvg[5]);
    query.bindValue(":frameEnergyMax6", frameEnergyMax[5]);
    query.bindValue(":frameEnergyMin7", frameEnergyMin[6]);
    query.bindValue(":frameEnergyAvg7", frameEnergyAvg[6]);
    query.bindValue(":frameEnergyMax7", frameEnergyMax[6]);

    bool success = query.exec();
    assert(success);

    QSqlDatabase::database().commit();

    query.exec("CREATE INDEX measurements_time_idx ON measurements(time)");
  }

  emit finished(0, "");

  return true;
}

bool Project::runApp() {

  emit advance(0, "Uploading binary");

  // upload binaries
  QFile tclFile("temp-pmu-prof.tcl");
  bool success = tclFile.open(QIODevice::WriteOnly);
  Q_UNUSED(success);
  assert(success);

  QString tcl = QString() + "set name " + name + "\n" + tcfUploadScript + "con\n";
      
  tclFile.write(tcl.toUtf8());

  tclFile.close();

  int ret = system("xsct temp-pmu-prof.tcl");
  if(ret) {
    emit finished(1, "Can't upload binaries");
    return false;
  }

  emit finished(0, "");

  return true;
}

///////////////////////////////////////////////////////////////////////////////

