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

#include <fstream>

#include "dse.h"
#include "cfg/loop.h"
#include "cfg/instruction.h"
#include "dseresultdialog.h"
#include "dsealgorithm.h"
#include "exhaustive.h"

unsigned Dse::loopDepth(Loop *loop) {
  unsigned depth = 1;

  QVector<Loop*> loops;
  loop->getAllLoops(loops, QVector<BasicBlock*>());

  for(auto l : loops) {
    unsigned childDepth = loopDepth(l) + 1;
    if(childDepth > depth) depth = childDepth;
  }

  return depth;
}

unsigned numberOfSolutions(QVector<unsigned> loopDepths) {
  if(!loopDepths.size()) return 0;

  unsigned x = loopDepths[0] + 1;
  for(int i = 1; i < loopDepths.size(); i++) {
    x *= loopDepths[i] + 1;
  }
  return x;
}

void Dse::dialog(Container *cont) {
  dseAlgorithm = NULL;

  // detect loops
  loops.clear();
  cont->getAllLoops(loops, QVector<BasicBlock*>());
  QVector<unsigned> loopDepths;
  for(auto loop : loops) {
    loopDepths.push_back(loopDepth(loop));
  }

  DseDialog dialog(numberOfSolutions(loopDepths));
  dialog.exec();

  if(dialog.shouldRun && (dialog.loopsCheckBox->checkState() == Qt::Checked)) {
    if((containerId != cont->id) || (moduleId != cont->getModule()->id)) {
      clear();
    }

    fitnessChoice = dialog.fitnessCombo->currentIndex();
    containerId = cont->id;
    moduleId = cont->getModule()->id;
    algorithm = dialog.algCombo->currentIndex();

    outputFile = new std::ofstream("results.dse"); 

    (*outputFile) << *this << '\n';;
    outputFile->flush();

    // search for best solution
    switch(algorithm) {
      case 0: {
        QMessageBox msgBox;
        msgBox.setText("GA not implemented");
        msgBox.exec();
        break;
      }
      case 1:
        dseAlgorithm = new Exhaustive(mainProject, fitnessChoice, &dseRuns, outputFile, loops, loopDepths,
                                      dialog.rerunFailedCheckBox->checkState() == Qt::Checked);
        break;
    }

    if(dseAlgorithm) {
      QApplication::setOverrideCursor(Qt::WaitCursor);
      progDialog = new QProgressDialog(QString("Running DSE..."), QString(), 0, numberOfSolutions(loopDepths), NULL);
      progDialog->setWindowModality(Qt::WindowModal);
      progDialog->setMinimumDuration(0);
      progDialog->setValue(0);

      dseAlgorithm->moveToThread(&thread);
      connect(&thread, SIGNAL (started()), dseAlgorithm, SLOT (run()));
      connect(dseAlgorithm, SIGNAL(advance(int)), progDialog, SLOT(setValue(int)), Qt::BlockingQueuedConnection);
      connect(dseAlgorithm, SIGNAL(finished()), this, SLOT(finished()));
      thread.start();
    }
  }
}

void Dse::finished() {
  delete progDialog;
  progDialog = NULL;

  outputFile->close();
  delete outputFile;
  outputFile = NULL;

  disconnect(&thread, SIGNAL (started()), 0, 0);
  disconnect(dseAlgorithm, SIGNAL(advance(int)), 0, 0);
  disconnect(dseAlgorithm, SIGNAL(finished()), 0, 0);

  thread.quit();
  thread.wait();

  QApplication::restoreOverrideCursor();

  showResults();
}

void Dse::showResults() {
  if(dseRuns.size()) {
    Module *mod = cfg->getModuleById(moduleId);
    assert(mod);
    Container *cont = static_cast<Container*>(mod->getVertexById(containerId));
    assert(cont);
    loops.clear();
    cont->getAllLoops(loops, QVector<BasicBlock*>());

    DseResultDialog resultDialog(&dseRuns, fitnessChoice, loops);
    resultDialog.exec();
  } else {
    QMessageBox msgBox;
    msgBox.setText("No data to display");
    msgBox.exec();
  }
}
