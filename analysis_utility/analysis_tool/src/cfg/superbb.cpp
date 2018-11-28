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

#include "basicblock.h"
#include "superbb.h"

void SuperBB::appendLocalItems(int startX, int yy, Vertex *visualTop, QVector<BasicBlock*> callStack, float scaling) {
  unsigned xx = startX + LINE_CLEARANCE;

  if(expanded) {
    yy += LINE_CLEARANCE;
  }

  // entries
  if(entries.size()) {
    for(auto child : entries) {
      child->appendItems(getBaseItem(), visualTop, callStack, scaling);
      child->setPos(xx, yy);
      xx += child->width + LINE_CLEARANCE;
      child->closeItems();
    }
    if(xx > width) width = xx;
    yy += entries[0]->height + LINE_CLEARANCE;
    xx = startX + LINE_CLEARANCE;
  }
    
  if(expanded) {
    for(auto child : children) {
      BasicBlock *bb = static_cast<BasicBlock*>(child);
      yy = bb->appendInstructions(getBaseItem(), xx, yy, &width, visualTop, callStack, scaling);
    }
  }

  // exits
  xx = 3*LINE_CLEARANCE/2;
  if(exits.size()) {
    for(auto child : exits) {
      child->appendItems(getBaseItem(), visualTop, callStack, scaling);
      child->setPos(xx, yy);
      xx += child->width + LINE_CLEARANCE;
      child->closeItems();
    }
    if(xx > width) width = xx;
    yy += exits[0]->height + LINE_CLEARANCE;
    xx = startX + 3*LINE_CLEARANCE/2;
  }

  height = yy + LINE_CLEARANCE;
}
