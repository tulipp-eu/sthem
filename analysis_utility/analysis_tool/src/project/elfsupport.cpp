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

#include "elfsupport.h"

void ElfSupport::setPc(uint64_t pc) {
  if(prevPc != pc) {
    prevPc = pc;

    auto it = addr2lineCache.find(pc);
    if(it != addr2lineCache.end()) {
      addr2line = (*it).second;
      return;
    }

    QString fileName = "";
    QString function = "Unknown";
    uint64_t lineNumber = 0;

    if(!elfFile.trimmed().isEmpty()) {
      char buf[1024];
      FILE *fp;
      std::stringstream pcStream;
      std::string cmd;

      // create command
      pcStream << std::hex << pc;
      cmd = "addr2line -C -f -a " + pcStream.str() + " -e " + elfFile.toUtf8().constData();

      // run addr2line program
      if((fp = popen(cmd.c_str(), "r")) == NULL) goto error;

      // discard first output line
      if(fgets(buf, 1024, fp) == NULL) goto error;

      // get function name
      if(fgets(buf, 1024, fp) == NULL) goto error;
      function = QString::fromUtf8(buf).simplified();
      if(function == "??") function = "Unknown";

      // get filename and linenumber
      if(fgets(buf, 1024, fp) == NULL) goto error;

      {
        QString qbuf = QString::fromUtf8(buf);
        fileName = qbuf.left(qbuf.indexOf(':'));
        lineNumber = qbuf.mid(qbuf.indexOf(':') + 1).toULongLong();
      }

      // close stream
      if(pclose(fp)) goto error;
    }

    addr2line = Addr2Line(fileName, function, lineNumber);
    addr2lineCache[pc] = addr2line;

    return;

  error:
    addr2line = Addr2Line("", "", 0);
    addr2lineCache[pc] = addr2line;
  }
}

QString ElfSupport::getFilename(uint64_t pc) {
  setPc(pc);
  return addr2line.filename;
}

QString ElfSupport::getFunction(uint64_t pc) {
  setPc(pc);
  return addr2line.function;
}

uint64_t ElfSupport::getLineNumber(uint64_t pc) {
  setPc(pc);
  return addr2line.lineNumber;
}

bool ElfSupport::isBb(uint64_t pc) {
  setPc(pc);
  return addr2line.filename[0] == '@';
}

QString ElfSupport::getModuleId(uint64_t pc) {
  setPc(pc);
  return addr2line.filename.right(addr2line.filename.size()-1);
}

uint64_t ElfSupport::lookupSymbol(QString symbol) {
  FILE *fp;
  char buf[1024];

  // create command line
  QString cmd = QString("nm ") + elfFile;

  // run program
  if((fp = popen(cmd.toUtf8().constData(), "r")) == NULL) goto error;

  while(fgets(buf, 1024, fp)) {
    QStringList line = QString::fromUtf8(buf).split(' ');
    if(line[2].trimmed() == symbol) {
      return line[0].toULongLong(0, 16);
    }
  }

 error:
  return 0;
}

