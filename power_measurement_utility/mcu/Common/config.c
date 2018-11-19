/******************************************************************************
 *
 *  This file is part of the Lynsyn PMU firmware.
 *
 *  Copyright 2018 Asbjørn Djupdal, NTNU, TULIPP EU Project
 *
 *  Lynsyn is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Lynsyn is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Lynsyn.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "lynsyn.h"
#include "config.h"

#include <em_msc.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define CONFIG_LINES (CONFIG_FLASH_SIZE/sizeof(struct configLine))

struct configLine {
  char id[ID_SIZE];
  union {
    char value[VALUE_SIZE];
    uint32_t intValue;
    double doubleValue;
  };
};

struct configLine configMem[CONFIG_LINES+1];

static void flushConfig(void) {
  MSC_Init();
  MSC_ErasePage((uint32_t*)CONFIG_FLASH_ADDRESS);
  if(MSC_WriteWord((uint32_t*)CONFIG_FLASH_ADDRESS, configMem, CONFIG_FLASH_SIZE) != mscReturnOk) {
    printf("Can't write flash\n");
  }
  MSC_Deinit();
}

void configInit(void) {
  memcpy(configMem, (uint32_t*)CONFIG_FLASH_ADDRESS, CONFIG_FLASH_SIZE);

  if(strncmp(configMem[0].id, CONFIG_FLASH_ID, strlen(CONFIG_FLASH_ID))) {
    printf("Initializing flash\n");
    strcpy(configMem[0].id, CONFIG_FLASH_ID);
    strcpy(configMem[0].value, CONFIG_FLASH_VERSION);

    for(int i = 1; i < CONFIG_LINES; i++) {
      configMem[i].id[0] = 0;
      configMem[i].value[0] = 0;
    }
    flushConfig();
  }
}

static struct configLine *getLine(char *id) {
  for(int i = 1; i < CONFIG_LINES; i++) {
    if(!strncmp(configMem[i].id, id, 8)) {
      return &configMem[i];
    }
  }
  return NULL;
}

static struct configLine *getFreeLine(void) {
  for(int i = 1; i < CONFIG_LINES; i++) {
    if(!configMem[i].id[0]) {
      return &configMem[i];
    }
  }
  return NULL;
}

bool configExists(char *id) {
  return getLine(id);
}

uint32_t getUint32(char *id) {
  struct configLine *line = getLine(id);
  if(line) {
    return line->intValue;
  }
  return 0;
}

double getDouble(char *id) {
  struct configLine *line = getLine(id);
  if(line) {
    return line->doubleValue;
  }
  return 0;
}

char *getString(char *id) {
  struct configLine *line = getLine(id);
  if(line) return line->value;
  return 0;
}

void setUint32(char *id, uint32_t val) {
  if(strlen(id) >= ID_SIZE) panic("Config id length");

  struct configLine *line = getLine(id);

  if(!line) line = getFreeLine();

  if(line) {
    strcpy(line->id, id);
    line->intValue = val;
    flushConfig();
  }
}

void setDouble(char *id, double val) {
  if(strlen(id) >= ID_SIZE) panic("Config id length");
  
  struct configLine *line = getLine(id);

  if(!line) line = getFreeLine();

  if(line) {
    strcpy(line->id, id);
    line->doubleValue = val;
    flushConfig();
  }
}

void setString(char *id, char *s) {
  if(strlen(id) >= ID_SIZE) panic("Config id length");
  if(strlen(s) >= VALUE_SIZE) panic("Config value length");

  struct configLine *line = getLine(id);

  if(!line) line = getFreeLine();

  if(line) {
    strcpy(line->id, id);
    strcpy(line->value, s);
    flushConfig();
  }
}

void clearConfig(void) {
  configMem[0].id[0] = 0;
  flushConfig();
  configInit();
}
