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

#include <QBrush>

#include "basicblock.h"
#include "instruction.h"
#include "function.h"
#include "profile/profline.h"

int BasicBlock::appendInstructions(QGraphicsItem *parent, int xx, int yy, unsigned *width,
                                   Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    Vertex *visibleItem = NULL;
    float childScaling = scaling;
    if(instr) {
      QVector<BasicBlock*> callStackFunc = callStack;

      if(instr->name == INSTR_ID_CALL) {
        if(callStack.contains(this)) {
          instr->recursive = true;
          visibleItem = instr;
        } else {
          instr->recursive = false;
          Function *func = getModule()->getFunctionById(instr->target);
          if(!func) {
            func = getTop()->getFunctionById(instr->target);
          }
          if(func) {
            visibleItem = func;
            assert(func->callers >= 1);
            double ratio = getTop()->getProfile()->getArcRatio(Config::core, this, func);
            childScaling *= ratio;
            callStackFunc.push_back(this);
          } else {
            visibleItem = instr;
          }
        }
      } else if(Config::includeAllInstructions) {
        visibleItem = instr;
      }

      if(visibleItem) {
        visibleItem->appendItems(parent, visualTop, callStackFunc, scaling);
        visibleItem->setPos(xx, yy);

        yy += visibleItem->height + TEXT_CLEARANCE;

        if(visibleItem->width + (TEXT_CLEARANCE*2) > *width) *width = visibleItem->width + (TEXT_CLEARANCE*2);
        height += visibleItem->height;

        visibleItem->closeItems();
      }
    }
  }

  return yy;
}

void BasicBlock::appendLocalItems(int startX, int yy, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
  unsigned xx = startX + LINE_CLEARANCE;

  if(expanded) {
    yy += LINE_CLEARANCE;
  }

  // entry
  entryNode->appendItems(getBaseItem(), visualTop, callStack, 1);
  entryNode->setPos(xx, yy);
  xx += entryNode->width + LINE_CLEARANCE;
  if(xx > width) width = xx;
  yy += entryNode->height + LINE_CLEARANCE;
  xx = startX + LINE_CLEARANCE;
  entryNode->closeItems();

  if(expanded) {
    yy = appendInstructions(getBaseItem(), xx, yy, &width, visualTop, callStack, scaling);
  }

  // exits
  xx = 3*LINE_CLEARANCE/2;
  if(exitNodes.size()) {
    for(auto child : exitNodes) {
      child->appendItems(getBaseItem(), visualTop, callStack, 1);
      child->setPos(xx, yy);
      xx += child->width + LINE_CLEARANCE;
      child->closeItems();
    }
    if(xx > width) width = xx;
    yy += exitNodes[0]->height + LINE_CLEARANCE;
    xx = startX + 3*LINE_CLEARANCE/2;
  }

  height = yy;
}

void BasicBlock::getProfData(unsigned core, QVector<BasicBlock*> callStack,
                             double *runtime, double *energy, double *runtimeFrame, double *energyFrame, uint64_t *count) {
  if(cachedRuntime == INT_MAX) {
    Profile *profile = getTop()->getProfile();

    if(profile) {
      profile->getProfData(core, this, &cachedRuntime, cachedEnergy, &cachedRuntimeFrame, cachedEnergyFrame, &cachedCount);

      for(auto child : children) {
        Instruction *instr = dynamic_cast<Instruction*>(child);
        if(instr) {
          if(instr->name == INSTR_ID_CALL) {
            Function *func = getModule()->getFunctionById(instr->target);
            if(!func) {
              func = getTop()->getFunctionById(instr->target);
            }
            if(func) {
              if(!callStack.contains(this)) {
                if(func->callers == 1) {
                  double runtimeChild;
                  double runtimeChildFrame;
                  double energyChild[Pmu::MAX_SENSORS];
                  double energyChildFrame[Pmu::MAX_SENSORS];
                  uint64_t countChild;
                  callStack.push_back(this);
                  func->getProfData(core, callStack, &runtimeChild, energyChild, &runtimeChildFrame, energyChildFrame, &countChild);

                  cachedRuntime += runtimeChild;
                  cachedRuntimeFrame += runtimeChildFrame;
                  for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
                    cachedEnergy[i] += energyChild[i];
                    cachedEnergyFrame[i] += energyChildFrame[i];
                  }
                } else {
                  double runtimeChild;
                  double runtimeChildFrame;
                  double energyChild[Pmu::MAX_SENSORS];
                  double energyChildFrame[Pmu::MAX_SENSORS];
                  uint64_t countChild;
                  callStack.push_back(this);
                  func->getProfData(core, callStack, &runtimeChild, energyChild, &runtimeChildFrame, energyChildFrame, &countChild);
                  double ratio = profile->getArcRatio(core, this, func);

                  cachedRuntime += runtimeChild * ratio;
                  cachedRuntimeFrame += runtimeChildFrame * ratio;
                  for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
                    cachedEnergy[i] += energyChild[i] * ratio;
                    cachedEnergyFrame[i] += energyChildFrame[i] * ratio;
                  }
                }
              }
            }
          }
        }
      }
    } else {
      cachedRuntime = 0;
      cachedRuntimeFrame = 0;
      for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
        cachedEnergy[i] = 0;
        cachedEnergyFrame[i] = 0;
      }
      cachedCount = 0;
    }
  }

  *runtime = cachedRuntime;
  *runtimeFrame = cachedRuntimeFrame;
  for(unsigned i = 0; i < Pmu::MAX_SENSORS; i++) {
    energy[i] = cachedEnergy[i];
    energyFrame[i] = cachedEnergyFrame[i];
  }
  *count = cachedCount;
}

void BasicBlock::getMeasurements(unsigned core, QVector<BasicBlock*> callStack, QVector<Measurement> *measurements) {
  Profile *profile = getTop()->getProfile();

  if(profile) {
    profile->getMeasurements(core, this, measurements);

    for(auto child : children) {
      Instruction *instr = dynamic_cast<Instruction*>(child);
      if(instr) {
        if(instr->name == INSTR_ID_CALL) {
          Function *func = getModule()->getFunctionById(instr->target);
          if(!func) {
            func = getTop()->getFunctionById(instr->target);
          }
          if(func) {
            if(!callStack.contains(this)) {
              if(func->callers == 1) {
                callStack.push_back(this);
                func->getMeasurements(core, callStack, measurements);
              }
            }
          }
        }
      }
    }
  }
}

void BasicBlock::calculateCallers() {
  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    if(instr) {
      if(instr->name == INSTR_ID_CALL) {
        Function *func = getModule()->getFunctionById(instr->target);
        if(!func) {
          func = getTop()->getFunctionById(instr->target);
        }
        if(func) {
          int callers = func->callers;
          func->addCaller(this);
          if(!callers) {
            func->calculateCallers();
          }
        }
      }
    }
  }
}

QVector<AnalysisInfo> BasicBlock::getRecursiveFunctions(QVector<BasicBlock*> callStack) {
  QVector<AnalysisInfo> recursiveFunctions;

  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    if(instr) {
      if(instr->name == INSTR_ID_CALL) {
        Function *func = getModule()->getFunctionById(instr->target);

        if(callStack.contains(this)) {
          recursiveFunctions.push_back(AnalysisInfo(func->getCfgName(), func->getSourceFilename(), func->getSourceLineNumber()));
          return recursiveFunctions;
        } else {
          if(!func) {
            func = getTop()->getFunctionById(instr->target);
          }
          if(func) {
            if(!isSystemFile(func->getSourceFilename())) {
              callStack.push_back(this);
              recursiveFunctions += func->getRecursiveFunctions(callStack);
            }
          }
        }
      }
    }
  }

  return recursiveFunctions;
}

QVector<AnalysisInfo> BasicBlock::getExternalCalls(QVector<BasicBlock*> callStack) {
  QVector<AnalysisInfo> externalCalls;

  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    if(instr) {
      if(instr->name == INSTR_ID_CALL) {
        if(callStack.contains(this)) {
          return externalCalls;
        } else {
          Function *func = getModule()->getFunctionById(instr->target);
          if(!func) {
            func = getTop()->getFunctionById(instr->target);
          }
          if(func) {
            if(!isSystemFile(func->getSourceFilename())) {
              callStack.push_back(this);
              externalCalls += func->getExternalCalls(callStack);
            }
          } else {
            externalCalls.push_back(AnalysisInfo(instr->target, instr->getSourceFilename(), instr->getSourceLineNumber()));
          }
        }
      }
    }
  }

  return externalCalls;
}

QVector<AnalysisInfo> BasicBlock::getArraysWithPtrToPtr(QVector<BasicBlock*> callStack) {
  QVector<AnalysisInfo> arraysWithPtrToPtr;

  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    if(instr) {
      if(instr->hasArrayWithPtrToPtr()) {
        arraysWithPtrToPtr.push_back(AnalysisInfo("Array", instr->getFunction()->getSourceFilename(), instr->getFunction()->getSourceLineNumber()));
      }
      if(instr->name == INSTR_ID_CALL) {
        if(callStack.contains(this)) {
          return arraysWithPtrToPtr;
        } else {
          Function *func = getModule()->getFunctionById(instr->target);
          if(!func) {
            func = getTop()->getFunctionById(instr->target);
          }
          if(func) {
            if(!isSystemFile(func->getSourceFilename())) {
              callStack.push_back(this);
              arraysWithPtrToPtr += func->getArraysWithPtrToPtr(callStack);
            }
          }
        }
      }
    }
  }

  return arraysWithPtrToPtr;
}

bool BasicBlock::hasComplexPtrCast(QVector<BasicBlock*> callStack) {
  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    if(instr) {
      if(instr->hasComplexPtrCast(callStack)) return true;
      if(instr->name == INSTR_ID_CALL) {
        if(callStack.contains(this)) {
          return false;
        } else {
          Function *func = getModule()->getFunctionById(instr->target);
          if(!func) {
            func = getTop()->getFunctionById(instr->target);
          }
          if(func) {
            if(!isSystemFile(func->getSourceFilename())) {
              callStack.push_back(this);
              if(func->hasComplexPtrCast(callStack)) {
                return true;
              }
            }
          }
        }
      }
    }
  }

  return false;
}

void BasicBlock::getAllLoops(QVector<Loop*> &loops, QVector<BasicBlock*> callStack, bool recursive) {
  if(recursive) {
    for(auto child : children) {
      Instruction *instr = dynamic_cast<Instruction*>(child);
      if(instr) {
        if(instr->name == INSTR_ID_CALL) {
          if(callStack.contains(this)) {
            return;
          } else {
            Function *func = getModule()->getFunctionById(instr->target);
            if(!func) {
              func = getTop()->getFunctionById(instr->target);
            }
            if(func) {
              if(!isSystemFile(func->getSourceFilename())) {
                callStack.push_back(this);
                func->getAllLoops(loops, callStack, true);
              }
            }
          }
        }
      }
    }
  }
}

QString BasicBlock::getCfgName() {
  QString text = "BasicBlock";

  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    if(instr) {
      if(instr->name == INSTR_ID_CALL) {
        text += "+";
      }
    }
  }

  return text;
}

bool BasicBlock::containsFunctionCall(Function *func) {
  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    if(instr) {
      if(instr->name == INSTR_ID_CALL) {
        if(instr->target == "<indirect>") return true;
        Function *called = getModule()->getFunctionById(instr->target);
        if(!called) {
          called = getTop()->getFunctionById(instr->target);
        }
        if(called) {
          if(func == called) return true;
        }
      }
    }
  }
  return false;
}

BasicBlock *BasicBlock::getFrameDoneBb(QString frameDoneFunction) {
  for(auto child : children) {
    Instruction *instr = dynamic_cast<Instruction*>(child);
    if(instr) {
      if(instr->name == INSTR_ID_CALL) {
        if(instr->target == frameDoneFunction) return this;
      }
    }
  }
  return NULL;
}

