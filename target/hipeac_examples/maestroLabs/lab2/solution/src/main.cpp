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

#include <tulipp.h>
#include "filters.h"
#include "sepia_omp.h"
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
    tulippStart();

    PRINTF("Start processing frames: hwPassthrough");
    hwPassthrough(inStreamData, outStreamData);
    PRINTF("Done processing frames: hwPassthrough");
    writeFile("hwPassthrough.ppm", outStreamData);

    PRINTF("Start processing frames: hwSepia");
    hwSepia(inStreamData, outStreamData);
    PRINTF("Done processing frames: hwSepia");
    writeFile("hwSepia.ppm", outStreamData);

    PRINTF("Start processing frames: hwColorConversionRgbxToGray");
    hwColorConversionRgbxToGray(inStreamData, outStreamData);
    PRINTF("Done processing frames: hwColorConversionRgbxToGray");
    writeFile("hwColorConversionRgbxToGray.ppm", outStreamData);

    PRINTF("Start processing frames: hwSobelX");
    hwSobelX(inStreamData, outStreamData);
    PRINTF("Done processing frames: hwSobelX");
    writeFile("hwSobelX.ppm", outStreamData);

    PRINTF("Start processing frames: hwSobelY");
    hwSobelY(inStreamData, outStreamData);
    PRINTF("Done processing frames: hwSobelY");
    writeFile("hwSobelY.ppm", outStreamData);

    PRINTF("Start processing frames: hwScharrX");
    hwScharrX(inStreamData, outStreamData);
    PRINTF("Done processing frames: hwScharrX");
    writeFile("hwScharrX.ppm", outStreamData);

    PRINTF("Start processing frames: hwScharrY");
    hwScharrY(inStreamData, outStreamData);
    PRINTF("Done processing frames: hwScharrY");
    writeFile("hwScharrY.ppm", outStreamData);

    #if APP_OPENMP
    PRINTF("Start processing frames: sepia_noOmp");
    sepia_noOmp(inStreamData, outStreamData);
    PRINTF("Done processing frames: sepia_noOmp");
    writeFile("sepia_noOmp.ppm", outStreamData);

    PRINTF("Start processing frames: sepia_omp");
    sepia_omp(inStreamData, outStreamData);
    PRINTF("Done processing frames: sepia_omp");
    writeFile("sepia_omp.ppm", outStreamData);
    #endif /* APP_OPENMP */

    /* Stop measurements here */
    tulippStop();

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
