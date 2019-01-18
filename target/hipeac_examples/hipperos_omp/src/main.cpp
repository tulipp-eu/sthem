/******************************************************************************
 *
 * HiPEAC TULIPP Tutorial Example
 *
 * January 2019
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ff.h>

#include <tulipp.h>

#define FATFS_MOUNT "1:/"


#include "filters.h"
#include "ppm.h"

#include "os.h"

#include "omp.h"



/*****************************************************************************/
/* Defines */

#define FRAMES 10
#define MAX_FILENAME_SIZE 256

#define ROWS     1080
#define COLS     1920

#define SIZE_ARR (ROWS*COLS)

/*****************************************************************************/
/* Variables */

static FATFS fatfs;

/*****************************************************************************/
/* Functions */

void noFilter(u32* inStreamData, u32* outStreamData, size_t streamSize)
{
  memcpy(outStreamData, inStreamData, streamSize);
}

int hipeac19demo(void)
{
  #if defined(__hipperos__)
  board_init();
  sdmmc_hostInit();
  #endif

  /* Allocate buffers */
  const size_t streamSize = (SIZE_ARR * sizeof(uint32_t));
  uint32_t *inStreamData = (uint32_t*)malloc(streamSize);
  uint32_t *outStreamData = (uint32_t*)malloc(streamSize);

  if(!inStreamData || !outStreamData) {
    EXITFAIL("Cannot allocate enough memory");
  }

  /* Mount SD-card filesystem */
  if(f_mount(&fatfs, FATFS_MOUNT, 0)) {
    EXITFAIL(("Could not mount '" FATFS_MOUNT));
  }

  /* Read input frame from SD card */
  PRINTF("Reading frame from SD-Card...");
  if(readPpmToStream(inStreamData, FATFS_MOUNT "inframe.ppm", COLS, ROWS)) {
    EXITFAIL("Could not read file");
  }

  /*-------------------------------------------------------------------------*/

  PRINTF("Starting processing frames\n");

  /* Start measurements here */
  tulippStart();


  #pragma omp parallel
  {
    const int localNbOfThreads = omp_get_num_threads();
    const int threadId = omp_get_thread_num();

    /* Frame loop, frames are processed here */
    for (int frame = 0; frame < FRAMES; frame++) {
      /* Uncomment the filter you want to use */
      //    noFilter(inStreamData, outStreamData, streamSize);

      //    hwPassthrough(inStreamData, outStreamData);
      //    hwSepia(inStreamData, outStreamData);
      hwColorConversionRgbxToGray(inStreamData, outStreamData);
      //    hwSobelX(inStreamData, outStreamData);
      //    hwSobelY(inStreamData, outStreamData);
      //    hwScharrX(inStreamData, outStreamData);
      //    hwScharrY(inStreamData, outStreamData);

      /* Mark that a frame has been processed */
      tulippFrameDone();
    }
  }

  /* Stop measurements here */
  tulippEnd();

  PRINTF("Frames done\n");

  /*-------------------------------------------------------------------------*/

  /* Write output frame to SD card */
  PRINTF("Writing frame to SD-Card ...");
  if(writeStreamToPpm(outStreamData, FATFS_MOUNT "out.ppm", COLS, ROWS)) {
    EXITFAIL("Could not write file");
  }

  /* Deallocate */
  free(inStreamData);
  free(outStreamData);

  PRINTF("TULIPP example done");

  return 0;
}

extern "C" {
  DEFMAIN(hipeac) {
    return hipeac19demo();
  }
}
