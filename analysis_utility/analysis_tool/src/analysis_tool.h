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

#ifndef ANALYSIS_TOOL_H
#define ANALYSIS_TOOL_H

#include <QVariant>
#include <QVector>
#include <QComboBox>

#define APP_NAME   "Analysis Tool"
#define ORG_NAME   "TULIPP"
#define ORG_DOMAIN "ntnu.no"
#define VERSION    1.5

///////////////////////////////////////////////////////////////////////////////
// XML defines

#define TAG_FUNCTION     "function"
#define TAG_BASICBLOCK   "bb"
#define TAG_REGION       "region"
#define TAG_LOOP         "loop"
#define TAG_INSTRUCTION  "instr"
#define TAG_EDGE         "edge"

#define ATTR_ID                "id"
#define ATTR_TARGET            "target"
#define ATTR_SUPERBB           "superbb"
#define ATTR_FILE              "file"
#define ATTR_LINE              "line"
#define ATTR_COLUMN            "col"
#define ATTR_ENTRY             "entry"
#define ATTR_FRAME_DONE        "frameDone"
#define ATTR_ISSTATIC          "isStatic"
#define ATTR_ISMEMBER          "isMember"
#define ATTR_PTRTOPTRARG       "ptrToPtrArg"
#define ATTR_VARIABLE          "variable"
#define ATTR_ARRAY             "array"
#define ATTR_ARRAYWITHPTRTOPTR "arrayWithPtrToPtr"
#define ATTR_COMPLEXPTRCAST    "complexPtrCast"

#define INSTR_ID_CALL "call"
#define INSTR_ID_RET  "ret"

///////////////////////////////////////////////////////////////////////////////
// layout defines

#define TEXT_CLEARANCE         10
#define LINE_CLEARANCE         10
#define INPUTOUTPUT_SIZE       10

#define NTNU_BLUE         QColor(0,   80,  158)
#define NTNU_YELLOW       QColor(241, 210, 130)
#define NTNU_CYAN         QColor(92,  190, 201)
#define NTNU_PEAR         QColor(213, 209, 14)
#define NTNU_CHICKPEA     QColor(223, 216, 197)
#define NTNU_LIGHT_BLUE   QColor(121, 162, 206)
#define NTNU_GREEN        QColor(201, 212, 178)
#define NTNU_BEIGE        QColor(204, 189, 143)
#define NTNU_PINK         QColor(173, 32,  142)
#define NTNU_LIGHT_GRAY   QColor(221, 231, 238)
#define NTNU_BROWN        QColor(144, 73,  45)
#define NTNU_PURPLE       QColor(85,  41,  136)
#define NTNU_ORANGE       QColor(245, 128, 37)

#define BACKGROUND_COLOR  Qt::white
#define FOREGROUND_COLOR  Qt::black

#define MODULE_COLOR      NTNU_GREEN
#define GROUP_COLOR       NTNU_GREEN

#define HW_FUNCTION_COLOR NTNU_ORANGE

#define FUNCTION_COLOR    NTNU_BROWN
#define REGION_COLOR      NTNU_YELLOW
#define LOOP_COLOR        NTNU_YELLOW
#define BASICBLOCK_COLOR  NTNU_CHICKPEA

#define VERTEX_COLOR      NTNU_LIGHT_GRAY
#define ENTRY_COLOR       NTNU_LIGHT_GRAY
#define EXIT_COLOR        NTNU_LIGHT_GRAY
#define INSTRUCTION_COLOR NTNU_LIGHT_GRAY

#define RUNTIME_COLOR     NTNU_YELLOW
#define POWER_COLOR       NTNU_YELLOW
#define ENERGY_COLOR      NTNU_YELLOW

#define EDGE_COLORS { \
  Qt::red, \
  Qt::green, \
  Qt::blue \
}

#define COLOR_NAMES { \
  "Red", \
  "Green", \
  "Blue" \
}

#define MAX_UNCONNECTED_NODES_IN_A_ROW 2

///////////////////////////////////////////////////////////////////////////////
// gui defines

#define SCALE_IN_FACTOR 1.25
#define SCALE_OUT_FACTOR 0.8

///////////////////////////////////////////////////////////////////////////////
// build defines

#define A9_CLANG_TARGET  "-target armv7a--none-gnueabi -mcpu=cortex-a9 -mfloat-abi=hard"
#define A9_LLC_TARGET    "-mcpu=cortex-a9 -float-abi=hard"
#define A9_AS_TARGET     "-mcpu=cortex-a9 -mfloat-abi=hard"

#define A53_CLANG_TARGET "-target aarch64--none-gnueabi"
#define A53_LLC_TARGET   "-mcpu=cortex-a53"
#define A53_AS_TARGET    "-mcpu=cortex-a53"

///////////////////////////////////////////////////////////////////////////////
// other

class BasicBlock;
class Loop;

QVariant makeQVariant(QVector<BasicBlock*> callStack);
QVector<BasicBlock*> fromQVariant(QVariant variant);
void indent(unsigned in);
QString xmlPurify(QString text);
QString getLine(QString filename, unsigned lineNumber, unsigned column);
QVector<Loop*> getLoop(Loop *loop, unsigned depth);
void addFitnessCombo(QComboBox *combo);
bool isSystemFile(QString filename);

#endif
