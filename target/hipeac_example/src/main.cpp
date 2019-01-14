/******************************************************************************
 *
 * HiPEAC TULIPP Tutorial Example
 *
 * January 2019
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdint.h>

#include <xstatus.h>
#include <xtime_l.h>
#include <xil_io.h>

#include <ff.h>

#include <tulipp.h>

#include "filters.h"
#include "ppm_library/ppm.h"

/*****************************************************************************/
/* Defines */

#define FRAMES 10
#define MAX_FILENAME_SIZE 256

#define ROWS     1080
#define COLS     1920

#define SIZE_ARR (ROWS*COLS)

#define FATFS_MOUNT "1:/"

/*****************************************************************************/
/* Variables */

static FATFS fatfs;

/*****************************************************************************/
/* Functions */

int main(void) {
  printf("Starting TULIPP example\n");

  /* Allocate buffers */
  uint32_t *inStreamData = (uint32_t*)malloc(SIZE_ARR * sizeof(uint32_t));
  uint32_t *outStreamData = (uint32_t*)malloc(SIZE_ARR * sizeof(uint32_t));

  if(!inStreamData || !outStreamData) {
    fprintf(stderr, "Cannot allocate enough memory\n");
    return 1;
  }

  /* Mount SD-card filesystem */
  if(f_mount(&fatfs, FATFS_MOUNT, 0)) {
    fprintf(stderr, "Could not mount '" FATFS_MOUNT "'\n");
    return 1;
  }

  /* Read input frame from SD card */
  printf("Reading frame from SD-Card...\n");
  if(readPpmToStream(inStreamData, FATFS_MOUNT "inframe.ppm", COLS, ROWS)) {
    fprintf(stderr, "Could not read file\n");
    return 1;
  }

  /*-------------------------------------------------------------------------*/

  printf("Starting processing frames\n");

  /* Start measurements here */
  tulippStart();

  /* Frame loop, frames are processed here */
  for(int frame = 0; frame < FRAMES; frame++) {
    /* Uncomment the filter you want to use */

    //hwPassthrough(inStreamData, outStreamData);
    //hwSepia(inStreamData, outStreamData);
    //hwColorConversionRgbxToGray(inStreamData, outStreamData);
    hwSobelX(inStreamData, outStreamData);
    //hwSobelY(inStreamData, outStreamData);
    //hwScharrX(inStreamData, outStreamData);
    //hwScharrY(inStreamData, outStreamData);

    /* Mark that a frame has been processed */
    tulippFrameDone();
  }

  /* Stop measurements here */
  tulippEnd();

  printf("Frames done\n");

  /*-------------------------------------------------------------------------*/

  /* Write output frame to SD card */
  printf("Writing frame to SD-Card ...\n");
  if(writeStreamToPpm(outStreamData, FATFS_MOUNT "out.ppm", COLS, ROWS)) {
    fprintf(stderr, "Could not write file\n");
    return 1;
  }

  /* Deallocate */
  free(inStreamData);
  free(outStreamData);

  printf("TULIPP example done\n");

  return 0;
}
