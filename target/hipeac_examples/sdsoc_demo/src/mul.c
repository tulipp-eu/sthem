#include <stdio.h>
#include <string.h>
#include <ff.h>

#include "sdsoc_demo.h"

double swmultiply(double a, double b) {
  double result = a * b;

  FIL fp;
  if(f_open(&fp, FATFS_MOUNT "logfile.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
    char buf[80];
    uint32_t bw;
    snprintf(buf, 79, "Multiplying %f with %f, result: %f\n", a, b, result);
    buf[79] = 0;
    f_write(&fp, buf, strlen(buf), &bw);
    f_close(&fp);
  }

  return result;
}

double hwmultiply(double a, double b) {
  return a * b;
}

