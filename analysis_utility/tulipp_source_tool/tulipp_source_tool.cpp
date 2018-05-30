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

#include <sstream>
#include <string>
#include <iostream>
#include <stdio.h>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory TulippCategory("TULIPP");

static llvm::cl::opt<std::string> OutputFilename("output", llvm::cl::desc("Output filename"), llvm::cl::value_desc("outputfilename"), llvm::cl::Required);

static llvm::cl::list<std::string> PipelineLoop("pipeloop", llvm::cl::desc("Pipeline loop"), llvm::cl::value_desc("pipelineloops"), llvm::cl::ZeroOrMore, llvm::cl::cat(TulippCategory));

static llvm::cl::list<std::string> Array("array", llvm::cl::desc("Partition array"), llvm::cl::value_desc("array"), llvm::cl::ZeroOrMore, llvm::cl::cat(TulippCategory));

static llvm::cl::list<unsigned> Dim("dim", llvm::cl::desc("Array dimension to partition"), llvm::cl::value_desc("dim"), llvm::cl::ZeroOrMore, llvm::cl::cat(TulippCategory));

std::string sourceFile;

std::string getBaseName(std::string path) {
  std::string file;
  unsigned long pos = path.rfind("/");
  if(pos == std::string::npos) file = path;
  else file = path.substr(pos+1);
  return file;
}

///////////////////////////////////////////////////////////////////////////////

class InstrumentationVisitor : public RecursiveASTVisitor<InstrumentationVisitor> {

private:
  Rewriter &rewriter;

public:
  InstrumentationVisitor(Rewriter &R) : rewriter(R) {}

  bool VisitStmt(Stmt *s) {

    SourceManager &SM = rewriter.getSourceMgr();
    int line = SM.getSpellingLineNumber(s->getLocStart());
    int column = SM.getSpellingColumnNumber(s->getLocStart());

    if(isa<ForStmt>(s) || isa<WhileStmt>(s) || isa<DoStmt>(s)) {
      Stmt *body;

      bool instrument = true;

      std::stringstream loopId;
      loopId << line << "," << column;

      if(isa<ForStmt>(s)) {
        ForStmt *statement = cast<ForStmt>(s);
        body = statement->getBody();
      } else if(isa<WhileStmt>(s)) {
        WhileStmt *statement = cast<WhileStmt>(s);
        body = statement->getBody();
      } else if(isa<DoStmt>(s)) {
        DoStmt *statement = cast<DoStmt>(s);
        body = statement->getBody();
      }

      // pipeline loop
      for(unsigned i = 0; i < PipelineLoop.size(); i++) {
        if(loopId.str() == PipelineLoop[i]) {
          instrument = false;

          if(isa<CompoundStmt>(body)) {
            CompoundStmt *body_s = cast<CompoundStmt>(body);

            rewriter.InsertTextAfterToken(body_s->getLBracLoc(), "\n#pragma HLS PIPELINE\n");

          } else {
            rewriter.InsertTextAfter(body->getSourceRange().getBegin(), "\n{ \n#pragma HLS PIPELINE\n");
            rewriter.InsertTextAfterToken(body->getSourceRange().getEnd(), "}");
          }
        }
      }
    }

    return true;
  }

  bool VisitDecl(Decl *D) {
    SourceManager &SM = rewriter.getSourceMgr();
    int line = SM.getSpellingLineNumber(D->getLocStart());

    std::string filename = SM.getFilename(D->getLocStart()).str();

    if(isa<VarDecl>(D)) {
      VarDecl *var = cast<VarDecl>(D);
      if(isa<ArrayType>(var->getType())) {
        for(unsigned i = 0; i < Array.size(); i++) {
          if(var->getNameAsString() == Array[i]) {
            SourceLocation loc = SM.translateFileLineCol(SM.getFileEntryForID(SM.getMainFileID()), line, 0);
            std::stringstream str;
            str << "\n#pragma HLS ARRAY_PARTITION variable=" << var->getNameAsString() << " complete dim=" << Dim[i] << "\n";
            rewriter.InsertText(loc, str.str());
          }
        }
      }
    }

    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *f) {
    return true;
  }
};

///////////////////////////////////////////////////////////////////////////////

class InstrumentationConsumer : public ASTConsumer {

private:
  InstrumentationVisitor Visitor;

public:
  InstrumentationConsumer(Rewriter &R) : Visitor(R) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for(DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      Visitor.TraverseDecl(*b);
    }
    return true;
  }
};

///////////////////////////////////////////////////////////////////////////////

class InstrumentationAction : public ASTFrontendAction {

private:
  Rewriter rewriter;

public:
  InstrumentationAction() {}

  void EndSourceFileAction() override {
    SourceManager &SM = rewriter.getSourceMgr();

    // output rewritten source
    if(PipelineLoop.size() || Array.size()) {
      std::error_code err;
      llvm::raw_fd_ostream ostream(OutputFilename, err, llvm::sys::fs::OpenFlags::F_None);
      rewriter.getEditBuffer(SM.getMainFileID()).write(ostream);
    }
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<InstrumentationConsumer>(rewriter);
  }
};

///////////////////////////////////////////////////////////////////////////////

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, TulippCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  sourceFile = getBaseName(op.getSourcePathList()[0]);

  return Tool.run(newFrontendActionFactory<InstrumentationAction>().get());
}
