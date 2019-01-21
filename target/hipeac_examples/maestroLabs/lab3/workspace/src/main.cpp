/******************************************************************************
 *
 * HiPEAC TULIPP Tutorial Example
 *
 * January 2019
 *
 *****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ff.h>

//#include <tulipp.h> // TODO Asbjorn/Bjorn
#include "filters.h"
#include "ppm.h"

#include "os.h"

#include <sds_lib.h>

#include <app_config.h>

/*****************************************************************************/
/* Defines */

#define ROWS 1080
#define COLS 1920

#define SIZE_ARR (ROWS * COLS)

/*****************************************************************************/
/* Variables */

static FATFS fatfs;

/*****************************************************************************/
/* Functions */

static void writeFile(const char* file, uint32_t* data)
{
    /* Write output frame to SD card */
    PRINTF("Writing frame to SD-Card ...");
    if (writeStreamToPpm(data, file, COLS, ROWS)) {
        PRINTF("Could not write file %s", file);
        abort();
    }
}

static void initDevices()
{
#if defined(__hipperos__)
    board_init();
    sdmmc_hostInit();
#endif
}

static int tulippTutorialLab()
{
    initDevices();

    /* Allocate DMA buffers */
    const size_t streamSize = (SIZE_ARR * sizeof(uint32_t));
    auto inStreamData = (uint32_t*) DMA_MALLOC(streamSize);
    auto outStreamData = (uint32_t*) DMA_MALLOC(streamSize);

    if (!inStreamData || !outStreamData) {
        EXITFAIL("Cannot allocate enough memory");
    }

    /* Mount SD-card filesystem */
    if (f_mount(&fatfs, FATFS_MOUNT, 0)) {
        EXITFAIL(("Could not mount '" FATFS_MOUNT));
    }

    /* Read input frame from SD card */
    PRINTF("Reading frame from SD-Card...");
    if (readPpmToStream(inStreamData, FATFS_MOUNT "inframe.ppm", COLS, ROWS)) {
        EXITFAIL("Could not read file");
    }

    /*-------------------------------------------------------------------------*/

    PRINTF("Starting processing frames");

    /* Start measurements here */
//    tulippStart();

    /* Stop measurements here */
//    tulippStop(); // TODO look with Asbjorn/Bjorn

    /* Deallocate DMA buffers */
    DMA_FREE(inStreamData);
    DMA_FREE(outStreamData);

    PRINTF("TULIPP example done");

    return 0;
}

extern "C" {
    DEFMAIN(tulippTutorial)
    {
        return tulippTutorialLab();
    }
}
