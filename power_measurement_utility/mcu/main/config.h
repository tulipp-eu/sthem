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

#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_FLASH_ADDRESS  0x0FE00000
#define CONFIG_FLASH_SIZE     2048 // TODO: why does it not work with 4096?
#define CONFIG_FLASH_ID       "TULIPP"
#define CONFIG_FLASH_VERSION  "1"

#define ID_SIZE 8
#define VALUE_SIZE 56

void configInit(void);
void clearConfig(void);
bool configExists(char *id);
uint32_t getUint32(char *id);
double getDouble(char *id);
char *getString(char *id);
void setUint32(char *id, uint32_t val);
void setDouble(char *id, double val);
void setString(char *id, char *s);

#endif
