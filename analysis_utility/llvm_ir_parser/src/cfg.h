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

#include <cxxabi.h>
#include <regex>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Constants.h"

#include "llvm/IRReader/IRReader.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/GlobalsModRef.h"

using namespace llvm;

std::string demangle(std::string name);
std::string xmlify(std::string text);
std::string removeExtension(std::string filename);

class RegNode;
class BbNode;
class FunctionNode;

///////////////////////////////////////////////////////////////////////////////

class Node {
public:
  std::vector<Node*> children;
  Node *parent;

  Node(Node *parent) {
    this->parent = parent;
  }
  virtual ~Node() {}

  virtual void printXML(FILE *fp) = 0;
  virtual void print() = 0;
  virtual RegNode *getMyRegion();
  virtual std::vector<BbNode*> getAllBasicBlocks();

  virtual Node *getTop() {
    if(parent) return parent->getTop();
    return this;
  }

  virtual void merge();

  virtual int getId(Instruction *instr) {
    for(auto child : children) {
      int i = child->getId(instr);
      if(i > 0) return i;
    }
    return 0;
  }

  virtual int getId(BasicBlock *bb) {
    for(auto child : children) {
      int i = child->getId(bb);
      if(i > 0) return i;
    }
    return 0;
  }

  virtual bool isRegion() {
    return false;
  }
  virtual bool isLoop() {
    return false;
  }
  virtual bool isBb() {
    return false;
  }
  virtual void splitBbs() {
    std::vector<Node*> c = children;

    for(auto child : c) {
      child->splitBbs();
    }
  }
  virtual bool containsLoop(Loop *loop) {
    return false;
  }
  virtual std::vector<BasicBlock*> getAllBbs();
  virtual bool addLoop(Loop *loop);
  virtual void instrument() {
    for(auto child : children) {
      child->instrument();
    }
  }

  virtual FunctionNode *getFunc() {
    return parent->getFunc();
  }
};

///////////////////////////////////////////////////////////////////////////////

class ModuleNode : public Node {
  Module *mod;

public:
  ModuleNode(Module *mod);
  void printXML(FILE *fp);
  void print() {
    printf("Module %s\n", mod->getName().str().c_str());
  }
};

///////////////////////////////////////////////////////////////////////////////

class FunctionNode : public Node {
public:
  Function *func;

  FunctionNode(Function *func, ModuleNode *parent);
  void printXML(FILE *fp);
  bool containsLoop(Loop *loop) {
    return true;
  }
  void print() {
    printf("Function %s\n", func->getName().str().c_str());
  }
  void instrument();
  FunctionNode *getFunc() { return this; }
};

///////////////////////////////////////////////////////////////////////////////

class BbNode : public Node {
public:
  BasicBlock *bb;
  bool isEntry;
  std::vector<Instruction*> instructions;
  int id;
  int nextId;

  BbNode(BasicBlock *bb, Node *parent, bool isEntry = false);

  virtual std::vector<BasicBlock*> getAllBbs() {
    std::vector<BasicBlock*> basicBlocks;
    if(bb) basicBlocks.push_back(bb);
    return basicBlocks;
  }

  virtual int getId(Instruction *instr) {
    for(auto i : instructions) {
      if(i == instr) return id;
    }
    return 0;
  }

  virtual int getId(BasicBlock *bb) {
    if(this->bb == bb) return id;
    return 0;
  }

  std::vector<BbNode*> getAllBasicBlocks() {
    std::vector<BbNode*> bbs;
    bbs.push_back(this);
    return bbs;
  }

  virtual void splitBbs();

  bool isBb() {
    return true;
  }

  void printXML(FILE *fp);
  void print() {
    printf("BasicBlock %d\n", id);
  }

  static bool comparePtrToBbNode(BbNode* a, BbNode* b) {
    return (a->getTop()->getId(a->bb) < a->getTop()->getId(b->bb));
  }

};

///////////////////////////////////////////////////////////////////////////////

class RegNode : public Node {
public:
  Region *region;
  int id;
  bool isSuperBb;

  RegNode(Node *parent);
  RegNode(Region *region, Node *parent, BasicBlock *entryBlock);

  bool containsLoop(Loop *loop) {
    return region->contains(loop);
  }

  bool isRegion() {
    return true;
  }

  void printXML(FILE *fp);
  void print() {
    printf("Region %d\n", id);
  }
};

///////////////////////////////////////////////////////////////////////////////

class LoopNode : public Node {
public:
  Loop *loop;
  int id;

  LoopNode(Loop *loop, Node *parent);

  bool containsLoop(Loop *loop) {
    return this->loop->contains(loop);
  }
  bool containsNode(Node *node) {
    std::vector<BasicBlock*> basicBlocks = node->getAllBbs();
    for(auto bb : basicBlocks) {
      if(!loop->contains(bb)) return false;
    }
    return true;
  }

  bool isLoop() {
    return true;
  }

  void printXML(FILE *fp);
  void print() {
    printf("Loop %d\n", id);
  }
  void instrument();
};

#endif
