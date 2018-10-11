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

#include <unistd.h>

#include "cfg.h"

using namespace llvm;

static LLVMContext Context;
static DIBuilder *diBuilder;
static DIFile *diFile;
static DICompileUnit *diUnit;

ModuleNode *top = NULL;

///////////////////////////////////////////////////////////////////////////////

std::string demangle(std::string name) {
  int status = -4;

  std::unique_ptr<char, void(*)(void*)> res {
    abi::__cxa_demangle(name.c_str(), NULL, NULL, &status),
      std::free
      };

  std::string text = name;

  if(status==0) {
    text = res.get();
  }

  return text;
}

std::string removeExtension(std::string filename) {
  size_t lastDot = filename.find_last_of(".");
  if (lastDot == std::string::npos) return filename;
  return filename.substr(0, lastDot); 
}

std::string xmlify(std::string text) {
  text = std::regex_replace(text, std::regex("<"), "&lt;");
  text = std::regex_replace(text, std::regex(">"), "&gt;");
  text = std::regex_replace(text, std::regex("&"), "&amp;");
  text = std::regex_replace(text, std::regex("\""), "&quot;");
  text = std::regex_replace(text, std::regex("\'"), "&apos;");
  return text;
}

void printIR(Module *mod, std::string filename) {
  std::error_code err;
  raw_fd_ostream ostream(filename, err, llvm::sys::fs::OpenFlags::F_None);
  PrintModulePass printer(ostream);
  AnalysisManager<Module> dummy;
  printer.run(*mod, dummy);
}

void printXML(std::string filename) {
  FILE *fp = fopen(filename.c_str(), "wb");
  fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  top->printXML(fp);
  fclose(fp);
}

void recreateDbInfo(Module *mod) {
  StripDebugInfo(*mod);

  std::string moduleName = removeExtension(mod->getName().str());

  diBuilder = new DIBuilder(*mod);
  diFile = diBuilder->createFile("@" + moduleName, "");
  diUnit = diBuilder->createCompileUnit(dwarf::DW_LANG_C, diFile, "llvm_ir_parser", false, "", 0);

  for(auto &func : mod->functions()) {
    if(!func.isIntrinsic() && func.getBasicBlockList().size() > 0) {
      unsigned entryBlockId = top->getId(&func.getEntryBlock());
      std::string funcName = demangle(func.getName().str().c_str());
      DISubroutineType *subRoutineType = diBuilder->createSubroutineType(DITypeRefArray());
      DISubprogram *diSub = diBuilder->createFunction(diFile, funcName, func.getName(), diFile, entryBlockId, subRoutineType, false, true, entryBlockId);

      for(auto &bb : func.getBasicBlockList()) {
        for(auto &instr : bb) {
          instr.setDebugLoc(DebugLoc::get(top->getId(&instr), 1, diSub));
        }        
      }

      func.setSubprogram(diSub);
      diBuilder->finalize();
    }
  }
}

void instrumentFunctions(Module *mod) {
  std::string moduleName = removeExtension(mod->getName().str());

  for(auto &func : mod->functions()) {
    if(!func.isIntrinsic() && func.getBasicBlockList().size() > 0) {
      func.addFnAttr("instrument-function-entry-inlined", "__cyg_profile_func_enter");
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <input ir file> <-xml|-ll> <output file> [--instrument]\n", argv[0]);
    exit(1);
  }

  SMDiagnostic Err;
  std::unique_ptr<Module> mod = parseIRFile(argv[1], Err, Context);

  if(!mod) {
    Err.print(argv[0], errs());
    return 1;
  }

  top = new ModuleNode(mod.get(), argc >= 5);

  if(!strncmp("-xml", argv[2], 4)) {
    printXML(argv[3]);

  } else if(!strncmp("-ll", argv[2], 3)) {
    recreateDbInfo(mod.get());
    printIR(mod.get(), argv[3]);

  } else {
    printf("Unknown output\n");
    exit(1);
  }

  return 0;
}

