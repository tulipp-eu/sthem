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

#include <QMenu>

#include "cfgview.h"
#include "cfgscene.h"
#include "vertex.h"
#include "function.h"

void CfgView::mouseReleaseEvent(QMouseEvent *event) {
  QGraphicsView::mouseReleaseEvent(event);
  viewport()->setCursor(Qt::ArrowCursor);
}

void CfgView::wheelEvent(QWheelEvent *event) {
  if(event->modifiers() & Qt::ControlModifier) {
    if(event->delta() > 0) zoomInEvent();
    else zoomOutEvent();
    event->accept();
  } else {
    QGraphicsView::wheelEvent(event);
  }
}

void CfgView::keyPressEvent(QKeyEvent *event) {
  if(event->modifiers() & Qt::ControlModifier) {
    if(event->key() == Qt::Key_Plus) {
      zoomInEvent();
    }
    if(event->key() == Qt::Key_Minus) {
      zoomOutEvent();
    }
  }
  event->accept();
}

void CfgView::contextMenuEvent(QContextMenuEvent *event) {
  QGraphicsItem *item = scene()->itemAt(mapToScene(event->pos()), QTransform());
  if(item) {
    contextVertex = (Vertex*)item->data(0).value<void*>();
    contextCallStack = fromQVariant(item->data(1));
  } else {
    contextVertex = NULL;
  }

  if(dynamic_cast<Container*>(contextVertex)) {
    QMenu menu(this);

    menu.addAction(clearColorsAct);
    menu.addAction(setTopAct);
    menu.addAction(hlsAnalysisAct);
    if(contextVertex->isHw(contextCallStack)) {
      menu.addAction(dseAct);
    }

    menu.exec(event->globalPos());
  }
}

void CfgView::clearColorsEvent() {
  if(contextVertex) {
    contextVertex->clearColors();

    std::vector<LineSegment> lines = contextVertex->getLineSegments();
    for(auto line : lines) {
      line.edge->color = -1;
      line.edge->zvalue = 0;
    }

    static_cast<CfgScene*>(scene())->redraw();
  }
}

void CfgView::setTopEvent() {
  Container *cont = dynamic_cast<Container*>(contextVertex);
  if(cont) {
    static_cast<CfgScene*>(scene())->drawElement(cont, contextCallStack);
  }
}

void CfgView::dseEvent() {
  Container *cont = dynamic_cast<Container*>(contextVertex);
  if(dse) dse->dialog(cont);
}

QVector<AnalysisInfo> removeValid(QVector<AnalysisInfo> externalCalls) {
  QVector<AnalysisInfo> calls;

  for(auto s : externalCalls) {
    if(true
       && !s.text.startsWith("llvm.dbg.declare")
       && !s.text.startsWith("llvm.memset")
       && !s.text.startsWith("llvm.memcpy")
       && !s.text.startsWith("llvm.trap")
       && !s.text.startsWith("std::terminate")
       && !s.text.startsWith("__cxa_")
       && !s.text.startsWith("__assert_func")

       && !s.text.startsWith("abs")
       && !s.text.startsWith("atan")
       && !s.text.startsWith("atanf")
       && !s.text.startsWith("atan2")
       && !s.text.startsWith("atan2")
       && !s.text.startsWith("ceil")
       && !s.text.startsWith("ceilf")
       && !s.text.startsWith("copysign")
       && !s.text.startsWith("copysignf")
       && !s.text.startsWith("cos")
       && !s.text.startsWith("cosf")
       && !s.text.startsWith("coshf")
       && !s.text.startsWith("expf")
       && !s.text.startsWith("fabs")
       && !s.text.startsWith("fabsf")
       && !s.text.startsWith("floorf")
       && !s.text.startsWith("fmax")
       && !s.text.startsWith("fmin")
       && !s.text.startsWith("logf")
       && !s.text.startsWith("fpclassify")
       && !s.text.startsWith("isfinite")
       && !s.text.startsWith("isinf")
       && !s.text.startsWith("isnan")
       && !s.text.startsWith("isnormal")
       && !s.text.startsWith("log")
       && !s.text.startsWith("log10")
       && !s.text.startsWith("modf")
       && !s.text.startsWith("modff")
       && !s.text.startsWith("recip")
       && !s.text.startsWith("recipf")
       && !s.text.startsWith("round")
       && !s.text.startsWith("rsqrt")
       && !s.text.startsWith("rsqrtf")
       && !s.text.startsWith("signbit")
       && !s.text.startsWith("sin")
       && !s.text.startsWith("sincos")
       && !s.text.startsWith("sincosf")
       && !s.text.startsWith("sinf")
       && !s.text.startsWith("sinhf")
       && !s.text.startsWith("sqrt")
       && !s.text.startsWith("tan")
       && !s.text.startsWith("tanf")
       && !s.text.startsWith("trunc")

       && !s.text.startsWith("abort")
       && !s.text.startsWith("atexit")
       && !s.text.startsWith("exit")
       && !s.text.startsWith("fprintf")
       && !s.text.startsWith("printf")
       && !s.text.startsWith("perror")
       && !s.text.startsWith("putchar")
       && !s.text.startsWith("puts")
       )
      calls.push_back(s);
  }

  return calls;
}

void CfgView::hlsAnalysisEvent() {
  AnalysisModel *analysisModel = new AnalysisModel(contextVertex->getCfgName());

  QVector<AnalysisInfo> recursiveFunctions = contextVertex->getRecursiveFunctions(QVector<BasicBlock*>());
  QVector<AnalysisInfo> externalCalls = removeValid(contextVertex->getExternalCalls(QVector<BasicBlock*>()));
  QVector<AnalysisInfo> arraysWithPtrToPtr = contextVertex->getArraysWithPtrToPtr(QVector<BasicBlock*>());

  bool memFuncConst = contextVertex->memFuncConst();
  bool hasComplexPtrCast = false; //contextVertex->hasComplexPtrCast(QVector<BasicBlock*>());

  bool isFunction = contextVertex->isFunction();
  bool isStatic = false;
  bool hasTemplate = false;
  bool isMember = false;
  bool hasPointerToPointerArg = false;

  if(isFunction) {
    Function *func = static_cast<Function*>(contextVertex);

    isStatic = func->isStatic();
    hasTemplate = func->hasTemplate();
    isMember = func->isMember();
    hasPointerToPointerArg = func->hasPointerToPointerArg();
  }

  if(recursiveFunctions.size()) {
    unsigned typeId = analysisModel->addType("Has recursive functions");
    for(auto func : recursiveFunctions) {
      analysisModel->addLine(typeId, func.text, func.sourceFilename, func.sourceLineNumbers);
    }
  }
  if(externalCalls.size()) {
    unsigned typeId = analysisModel->addType("Has external function calls, virtual functions or function pointers");
    for(auto func : externalCalls) {
      analysisModel->addLine(typeId, func.text, func.sourceFilename, func.sourceLineNumbers);
    }
  }
  if(!memFuncConst) analysisModel->addType("Has non-const args to memcpy or memset");
  if(arraysWithPtrToPtr.size()) {
    unsigned typeId = analysisModel->addType("Has array with pointers to pointers");
    for(auto array : arraysWithPtrToPtr) {
      analysisModel->addLine(typeId, array.text, array.sourceFilename, array.sourceLineNumbers);
    }
  }
  if(hasComplexPtrCast) analysisModel->addType("Has complex pointer casts");
  if(!isFunction) analysisModel->addType("Is not a function");
  if(isStatic) analysisModel->addType("Is a static function");
  if(hasTemplate) analysisModel->addType("Is a template function");
  if(isMember) analysisModel->addType("Is a class member");
  if(hasPointerToPointerArg) analysisModel->addType("Has argument with pointer to pointer");

  analysisView->setModel(NULL);
  analysisView->setModel(analysisModel);
  QSettings settings;
  analysisView->header()->restoreState(settings.value("analysisViewState").toByteArray());
}

CfgView::CfgView(QTreeView *analysisView, QGraphicsScene *scene) : QGraphicsView(scene) {
  setBackgroundBrush(QBrush(BACKGROUND_COLOR, Qt::SolidPattern));

  dse = NULL;

  this->analysisView = analysisView;

  setDragMode(QGraphicsView::ScrollHandDrag);
  viewport()->setCursor(Qt::ArrowCursor);

  clearColorsAct = new QAction("&Clear edge colors", this);
  connect(clearColorsAct, SIGNAL(triggered()), this, SLOT(clearColorsEvent()));
  
  setTopAct = new QAction("&Set as top", this);
  connect(setTopAct, SIGNAL(triggered()), this, SLOT(setTopEvent()));

  hlsAnalysisAct = new QAction("&View HLS Compatability", this);
  connect(hlsAnalysisAct, SIGNAL(triggered()), this, SLOT(hlsAnalysisEvent()));

  dseAct = new QAction("&Automatic DSE", this);
  connect(dseAct, SIGNAL(triggered()), this, SLOT(dseEvent()));

  dseAct->setEnabled(false);
}
