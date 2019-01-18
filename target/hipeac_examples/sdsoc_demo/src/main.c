/******************************************************************************
 *
 * HiPEAC TULIPP Tutorial Example
 *
 * SDSoC demo
 *
 * Asbjørn Djupdal January 2019
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ff.h>

#include <tulipp.h>

#include "sdsoc_demo.h"

static FATFS fatfs;

int main(int argc, char *argv[]) {
  /* Mount SD-card filesystem */
  if(f_mount(&fatfs, FATFS_MOUNT, 0)) {
    fprintf(stderr, "Could not mount '" FATFS_MOUNT "'\n");
    return 1;
  }

  printf("SW multiply: %f\n", swmultiply(3, 5));
  printf("HW multiply: %f\n", hwmultiply(3, 5));
}
