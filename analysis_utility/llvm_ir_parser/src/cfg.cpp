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

int bbCounter = 1;
int regCounter = 1;
int loopCounter = 1;

///////////////////////////////////////////////////////////////////////////////

std::vector<BbNode*> Node::getAllBasicBlocks() {
  std::vector<BbNode*> bbs;
  for(auto child : children) {
    std::vector<BbNode*> childBbs = child->getAllBasicBlocks();
    bbs.insert(bbs.end(), childBbs.begin(), childBbs.end());
  }
  return bbs;
}

RegNode *Node::getMyRegion() {
  Node *p = parent;
  while(!p->isRegion() && p) p = p->parent;
  return static_cast<RegNode*>(p);
}

void Node::merge() {
  // merge children
  for(auto child : children) {
    child->merge();
  }

  // merge grandchild loops into child regions
  std::vector<Node*> childNodes = children;
  for(auto child : childNodes) {
    if(child->isRegion()) {
      if(child->children.size() == 1) {
        Node *grandChild = child->children[0];
        if(grandChild->isLoop()) {
          // remove child from this node
          auto it = std::find(children.begin(), children.end(), child);
          assert(it != children.end());
          children.erase(it);
          // add grandchild to this node
          grandChild->parent = this;
          children.push_back(grandChild);
        }
      }
    }
  }

  // merge myself
  if(children.size() == 1) {
    Node *child = children[0];
    if(child->isRegion()) {
      children.clear();
      for(auto grandChild : child->children) {
        grandChild->parent = this;
        children.push_back(grandChild);
      }
      child->children.clear();
      delete child;
    }
  }
}

bool Node::addLoop(Loop *loop) {
  if(!containsLoop(loop)) return false;

  // try to add to child
  for(auto child : children) {
    if(child->addLoop(loop)) return true;
  }
  
  // if not, add here
  LoopNode *loopNode = new LoopNode(loop, this);

  std::vector<Node*> childNodes = children;
  
  for(auto child : childNodes) {
    if(loopNode->containsNode(child)) {
      // remove child from this node
      auto it = std::find(children.begin(), children.end(), child);
      assert(it != children.end());
      children.erase(it);
      // add child to loopNode
      child->parent = loopNode;
      loopNode->children.push_back(child);
    }
  }

  children.push_back(loopNode);

  return true;
}

std::vector<BasicBlock*> Node::getAllBbs() {
  std::vector<BasicBlock*> basicBlocks;
  for(auto child : children) {
    std::vector<BasicBlock*> childBlocks = child->getAllBbs();
    basicBlocks.insert(basicBlocks.end(), childBlocks.begin(), childBlocks.end());
  }
  return basicBlocks;
}

///////////////////////////////////////////////////////////////////////////////

ModuleNode::ModuleNode(Module *mod) : Node(NULL) {
  this->mod = mod;

  for(auto &func : mod->functions()) {
    if(!func.isIntrinsic() && func.getBasicBlockList().size() > 0) {
      children.push_back(new FunctionNode(&func, this));
    }
  }
}

void ModuleNode::printXML(FILE *fp) {
  std::string fileName = mod->getSourceFileName();
  std::string moduleName = removeExtension(mod->getName().str());

  char *dirName = get_current_dir_name();

  if(fileName[0] == '/') {
    fprintf(fp, "<module id=\"%s\" file=\"%s\">\n",
            moduleName.c_str(), fileName.c_str());
  } else {
    fprintf(fp, "<module id=\"%s\" file=\"%s/%s\">\n",
            moduleName.c_str(), dirName, fileName.c_str());
  }

  free(dirName);

  for(auto child : children) {
    child->printXML(fp);
  }

  fprintf(fp, "</module>\n");
}

///////////////////////////////////////////////////////////////////////////////

void getAllLoops(Loop *loop, std::vector<Loop*> &loops) {
  // add loop
  loops.push_back(loop);

  // add all subloops
  const std::vector<Loop*> subLoops = loop->getSubLoopsVector();

  for(auto subLoop : subLoops) {
    getAllLoops(subLoop, loops);
  }
}

FunctionNode::FunctionNode(Function *func, ModuleNode *parent) : Node(parent) {
  this->func = func;

  // analyze
  DominatorTree dt;
  dt.recalculate(*func);

  PostDominatorTree pdt;
  pdt.recalculate(*func);

  DominanceFrontier df;
  df.analyze(dt);

  // find regions
  RegionInfo *regInfo = new RegionInfo();
  regInfo->recalculate(*func, &dt, &pdt, &df);
      
  // create region tree
  RegNode *top = new RegNode(regInfo->getTopLevelRegion(), this,
                             &func->getEntryBlock());

  // add to children vector
  children.push_back(top);

  // find loops
  LoopInfoBase<BasicBlock, Loop>* loopInfo = new LoopInfoBase<BasicBlock, Loop>();
  loopInfo->releaseMemory();
  loopInfo->analyze(dt);    

  std::vector<Loop*> loops;

  for(auto loop : *loopInfo) {
    getAllLoops(loop, loops);
  }

  for(auto loop : loops) {
    addLoop(loop);
  }

  // split basicblocks with call instructions
  splitBbs();

  // merge
  merge();
}

void FunctionNode::printXML(FILE *fp) {
  std::string funcName = xmlify(demangle(func->getName().str()));

  bool isMember = false;
  if(func->arg_size()) {
    Argument &arg = *(func->arg_begin());
    isMember = arg.getName().str() == "this";
  }

  bool ptrToPtrArg = false;
  for(auto &arg : func->args()) {
    if(arg.getType()->isPointerTy()) {
      PointerType *type = static_cast<PointerType*>(arg.getType());
      ptrToPtrArg = type->getElementType()->isPointerTy();
    }
  }

  fprintf(fp, "<function id=\"%s\" isStatic=\"%s\" isMember=\"%s\" ptrToPtrArg=\"%s\"",
          funcName.c_str(),
          func->hasExternalLinkage() ? "false" : "true",
          isMember ? "true" : "false",
          ptrToPtrArg ? "true" : "false");

  // find function source reference
  SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
  func->getAllMetadata(MDs);
  for(auto &MD : MDs) {
    if(MDNode *N = MD.second) {
      if(auto *subProgram = dyn_cast<DISubprogram>(N)) {
        std::string fileName = subProgram->getFilename().str();

        if(fileName[0] == '/') {
          fprintf(fp, " file=\"%s\" line=\"%d\"",
                  fileName.c_str(), subProgram->getLine());
        } else {
          char *dirName = get_current_dir_name();
          fprintf(fp, " file=\"%s/%s\" line=\"%d\"",
                  dirName, fileName.c_str(), subProgram->getLine());
          free(dirName);
        }
      }
    }
  }
  fprintf(fp, ">\n");

  for(auto child : children) {
    child->printXML(fp);
  }

  fprintf(fp, "</function>\n");
}

void FunctionNode::instrument() {
  Node::instrument();

  StringRef Func = "__tulipp_func_enter";
  Instruction *InsertionPt = &*func->begin()->getFirstInsertionPt();

  Module &M = *InsertionPt->getParent()->getParent()->getParent();
  LLVMContext &C = InsertionPt->getParent()->getContext();

  DebugLoc DL;
  if(auto SP = func->getSubprogram()) DL = DebugLoc::get(SP->getScopeLine(), 0, SP);

  Type *ArgTypes[] = {Type::getInt8PtrTy(C), Type::getInt8PtrTy(C)};

 
  Constant *Fn = M.getOrInsertFunction(Func, FunctionType::get(Type::getVoidTy(C), ArgTypes, false));
 
  Instruction *RetAddr = CallInst::Create(Intrinsic::getDeclaration(&M, Intrinsic::returnaddress),
                                          ArrayRef<Value *>(ConstantInt::get(Type::getInt32Ty(C), 0)), "",
                                          InsertionPt);
 
  Value *Args[] = {ConstantExpr::getBitCast(func, Type::getInt8PtrTy(C)), RetAddr};
 
  CallInst *Call = CallInst::Create(Fn, ArrayRef<Value *>(Args), "", InsertionPt);
  Call->setDebugLoc(DL);
}

///////////////////////////////////////////////////////////////////////////////

BbNode::BbNode(BasicBlock *bb, Node *parent, bool isEntry) : Node(parent) {
  id = bbCounter++;
  this->bb = bb;
  this->isEntry = isEntry;
}

void BbNode::splitBbs() {
  bool hasCall = false;

  std::vector<BbNode*> bbNodes;
  bbNodes.push_back(this);

  instructions.clear();

  nextId = bbCounter;

  std::vector<Instruction*> *instrs = &instructions;

  for(auto &instr : *bb) {
    if(instr.getOpcode() == Instruction::Call) {

      CallInst *call = static_cast<CallInst*>(&instr);
      if(call->getCalledFunction()) {
        if(call->getCalledFunction()->getName().str().compare(0, 9, "llvm.dbg.") == 0) {
          continue;
        }
        if(call->getCalledFunction()->getName().str().compare(0, 14, "llvm.lifetime.") == 0) {
          continue;
        }
      }

      if(hasCall) {
        // create new Bb
        BbNode *newNode = new BbNode(NULL, parent);
        newNode->nextId = bbCounter;

        bbNodes.push_back(newNode);

        instrs = &(newNode->instructions);
        instrs->push_back(&instr);

      } else {
        instrs->push_back(&instr);
        hasCall = true;
      }

    } else if(instr.getOpcode() == Instruction::Invoke) {

      InvokeInst *call = static_cast<InvokeInst*>(&instr);
      if(call->getCalledFunction()) {
        if(call->getCalledFunction()->getName().str().compare(0, 9, "llvm.dbg.") == 0) {
          continue;
        }
        if(call->getCalledFunction()->getName().str().compare(0, 14, "llvm.lifetime.") == 0) {
          continue;
        }
      }

      if(hasCall) {
        // create new Bb
        BbNode *newNode = new BbNode(NULL, parent);
        newNode->nextId = bbCounter;

        bbNodes.push_back(newNode);

        instrs = &(newNode->instructions);
        instrs->push_back(&instr);

      } else {
        instrs->push_back(&instr);
        hasCall = true;
      }

    } else {
      instrs->push_back(&instr);
    }
  }

  if(bbNodes.size() > 1) {
    RegNode *reg = new RegNode(parent);

    reg->isSuperBb = true;

    auto it = std::find(parent->children.begin(), parent->children.end(), this);
    assert(it != parent->children.end());

    parent->children.erase(it);
    parent->children.push_back(reg);
    for(auto bbNode : bbNodes) {
      bbNode->parent = reg;
      reg->children.push_back(bbNode);
    }
  }
}

void BbNode::printXML(FILE *fp) {
  fprintf(fp, "<bb id=\"%d\"", id);
  if(isEntry) {
    fprintf(fp, " entry=\"true\"");
  }
  fprintf(fp, ">\n");

  Instruction *lastInstr = NULL;

  // find all instructions
  for(auto instr : instructions) {
    if(instr->getOpcode() == Instruction::Call) {
      CallInst *call = static_cast<CallInst*>(instr);
      if(call->getCalledFunction()) {
        /*if(call->getCalledFunction()->isIntrinsic()) {
          continue;
          } else */{
          fprintf(fp, "<instr id=\"call\" target=\"%s\"", xmlify(demangle(call->getCalledFunction()->getName().str())).c_str());
        }
      } else {
        fprintf(fp, "<instr id=\"call\" target=\"&lt;indirect&gt;\"");
      }
    } else if(instr->getOpcode() == Instruction::Invoke) {
      InvokeInst *invoke = static_cast<InvokeInst*>(instr);
      if(invoke->getCalledFunction()) {
        fprintf(fp, "<instr id=\"call\" target=\"%s\"", xmlify(demangle(invoke->getCalledFunction()->getName().str())).c_str());
      } else {
        fprintf(fp, "<instr id=\"call\" target=\"&lt;indirect&gt;\"");
      }
    } else if(instr->getOpcode() == Instruction::Alloca) {
      AllocaInst *alloca = static_cast<AllocaInst*>(instr);

      bool arrayWithPtrToPtr = false;
      bool isArray = alloca->getAllocatedType()->isArrayTy();
      if(isArray) {
        PointerType *type = static_cast<PointerType*>(alloca->getAllocatedType());
        if(type->getElementType()->isPointerTy()) {
          PointerType *type2 = static_cast<PointerType*>(type->getElementType());
          arrayWithPtrToPtr = type2->getElementType()->isPointerTy();
        }
      }

      fprintf(fp, "<instr id=\"alloca\" variable=\"%s\" array=\"%s\" arrayWithPtrToPtr=\"%s\"",
              alloca->getName().str().c_str(), isArray ? "true" : "false", arrayWithPtrToPtr ? "true" : "false");
    } else if(instr->getOpcode() == Instruction::BitCast) {
      BitCastInst *bitcast = static_cast<BitCastInst*>(instr);

      bool complexPtrCast = false;

      Type *source = bitcast->getSrcTy();
      Type *dest = bitcast->getDestTy();
      if(source->isPointerTy() && dest->isPointerTy()) {
        PointerType *sourcePtr = static_cast<PointerType*>(source);
        PointerType *destPtr = static_cast<PointerType*>(dest);
        if(sourcePtr->getElementType()->isStructTy()) {
          if(sourcePtr->getElementType()->getStructName().str().find("class.ap_") == std::string::npos) {
            complexPtrCast = true;
          }
        }
        if(destPtr->getElementType()->isStructTy()) {
          if(destPtr->getElementType()->getStructName().str().find("class.ap_") == std::string::npos) {
            complexPtrCast = true;
          }
        }
      }

      fprintf(fp, "<instr id=\"bitcast\" complexPtrCast=\"%s\"", complexPtrCast ? "true" : "false");
    } else {
      fprintf(fp, "<instr id=\"%s\"", instr->getOpcodeName());
    }
    if(instr->getDebugLoc()) {
      DebugLoc debugLoc = instr->getDebugLoc();
      if(debugLoc.getLine()) {
        std::string fileName = debugLoc->getFilename().str();

        if(fileName[0] == '/') {
          fprintf(fp, " file=\"%s\" line=\"%d\" col=\"%d\"",
                  fileName.c_str(),
                  debugLoc.getLine(),
                  debugLoc.getCol());
        } else {
          char *dirName = get_current_dir_name();
          fprintf(fp, " file=\"%s/%s\" line=\"%d\" col=\"%d\"",
                  dirName, 
                  fileName.c_str(),
                  debugLoc.getLine(),
                  debugLoc.getCol());
          free(dirName);
        }
      }
    }

    fprintf(fp, "/>\n");

    lastInstr = instr;
  }

  // find all edges
  if(lastInstr) {
    if(lastInstr->isTerminator()) {
      // this is a branch
      for(unsigned i = 0; i < lastInstr->getNumOperands(); i++) {
        if(getTop()->getId(static_cast<BasicBlock*>(lastInstr->getOperand(i))) > 0) {
          fprintf(fp, "<edge target=\"%d\"/>\n", getTop()->getId(static_cast<BasicBlock*>(lastInstr->getOperand(i))));
        }
      }

    } else {
      // this is a call
      // insert edge to next fakebb
      fprintf(fp, "<edge target=\"%d\"/>\n", nextId);
    }
  }

  fprintf(fp, "</bb>\n");
}

///////////////////////////////////////////////////////////////////////////////

RegNode::RegNode(Node *parent) : Node(parent) {
  this->region = NULL;
  id = regCounter++;
  isSuperBb = false;
}

RegNode::RegNode(Region *region, Node *parent, BasicBlock *entryBlock) : Node(parent) {
  this->region = region;
  id = regCounter++;

  for(const std::unique_ptr<Region> &subReg : *region) {
    RegNode *child = new RegNode(subReg.get(), this, entryBlock);
    children.push_back(child);
  }
  
  for(auto bb : region->blocks()) {
    bool selected = true;
    for(const std::unique_ptr<Region> &subReg : *region) {
      if(subReg.get()->contains(bb)) selected = false;
    }
    if(selected) {
      BbNode *bbNode = new BbNode(bb, this, bb == entryBlock);
      children.push_back(bbNode);
    }
  }
}

void RegNode::printXML(FILE *fp) {
  if(isSuperBb) {
    fprintf(fp, "<region id=\"reg%d\" superbb=\"true\">\n", id);
  } else {
    fprintf(fp, "<region id=\"reg%d\">\n", id);
  }

  for(auto child : children) {
    child->printXML(fp);
  }

  fprintf(fp, "</region>\n");
}

///////////////////////////////////////////////////////////////////////////////

LoopNode::LoopNode(Loop *loop, Node *parent) : Node(parent) {
  this->loop = loop;
  id = loopCounter++;
}

void LoopNode::printXML(FILE *fp) {
  DebugLoc loc = loop->getStartLoc();

  fprintf(fp, "<loop id=\"loop%d\" file=\"%s\" line=\"%d\" col=\"%d\">\n", id, loc->getFilename().str().c_str(), loc.getLine(), loc.getCol());

  for(auto child : children) {
    child->printXML(fp);
  }

  fprintf(fp, "</loop>\n");
}

void LoopNode::instrument() {
  Node::instrument();

  {
    StringRef Func = "__tulipp_loop_header";
    BasicBlock *bbHeader = loop->getHeader();
    Instruction *InsertionPt = &(*bbHeader->getFirstInsertionPt());

    Module &M = *InsertionPt->getParent()->getParent()->getParent();
    LLVMContext &C = InsertionPt->getParent()->getContext();

    DebugLoc DL = bbHeader->getFirstNonPHIOrDbgOrLifetime()->getDebugLoc();

    Type *ArgTypes[] = {Type::getInt8PtrTy(C)};
 
    Constant *Fn = M.getOrInsertFunction(Func, FunctionType::get(Type::getVoidTy(C), ArgTypes, false));
 
    Value *Args[] = {ConstantExpr::getBitCast(BlockAddress::get(getFunc()->func, bbHeader), Type::getInt8PtrTy(C))};
 
    CallInst *Call = CallInst::Create(Fn, ArrayRef<Value *>(Args), "", InsertionPt);
    Call->setDebugLoc(DL);
  }

  {
    StringRef Func = "__tulipp_loop_body";
    BasicBlock *bbLatch = loop->getLoopLatch();
    Instruction *InsertionPt = &(*bbLatch->getFirstInsertionPt());

    Module &M = *InsertionPt->getParent()->getParent()->getParent();
    LLVMContext &C = InsertionPt->getParent()->getContext();

    DebugLoc DL = bbLatch->getFirstNonPHIOrDbgOrLifetime()->getDebugLoc();

    Type *ArgTypes[] = {Type::getInt8PtrTy(C)};
 
    Constant *Fn = M.getOrInsertFunction(Func, FunctionType::get(Type::getVoidTy(C), ArgTypes, false));
 
    Value *Args[] = {ConstantExpr::getBitCast(BlockAddress::get(getFunc()->func, bbLatch), Type::getInt8PtrTy(C))};
 
    CallInst *Call = CallInst::Create(Fn, ArrayRef<Value *>(Args), "", InsertionPt);
    Call->setDebugLoc(DL);
  }
}
