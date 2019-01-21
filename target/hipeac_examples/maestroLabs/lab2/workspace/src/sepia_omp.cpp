
#include "sepia_omp.h"

#include <omp.h>
#include <hipperos/hthreads.h>

#include "filters.h"

#define NB_THREADS 2u

void sepia_noOmp(uint32_t *input, uint32_t* output)
{
    for(int i=0; i<PIXELS_FHD; i++)
    {
        uint8_t in_channelR, in_channelG, in_channelB, in_channelX;
        in_channelR = (0xFF & (input[i] >> 24));
        in_channelG = (0xFF & (input[i] >> 16));
        in_channelB = (0xFF & (input[i] >> 8));
        in_channelX = (0xFF & (input[i] >> 0));

        uint32_t tmp_sepiaR;
        uint8_t out_sepiaR;
        tmp_sepiaR = (((uint32_t)in_channelR*402) + ((uint32_t)in_channelG*787) + ((uint32_t)in_channelB*194)) >> 10;
        if(tmp_sepiaR > 255)
            out_sepiaR = 255;
        else
            out_sepiaR = tmp_sepiaR;

        uint32_t tmp_sepiaG;
        uint8_t out_sepiaG;
        tmp_sepiaG = (((uint32_t)in_channelR*357) + ((uint32_t)in_channelG*702) + ((uint32_t)in_channelB*172)) >> 10;
        if(tmp_sepiaG > 255)
            out_sepiaG = 255;
        else
            out_sepiaG = tmp_sepiaG;

        uint32_t tmp_sepiaB;
        uint8_t out_sepiaB;
        tmp_sepiaB = (((uint32_t)in_channelR*279) + ((uint32_t)in_channelG*547) + ((uint32_t)in_channelB*134)) >> 10;
        if(tmp_sepiaB > 255)
            out_sepiaB = 255;
        else
            out_sepiaB = tmp_sepiaB;

        output[i] = (((uint32_t)out_sepiaR) << 24) | (((uint32_t)out_sepiaG) << 16) | (((uint32_t)out_sepiaB) << 8) | (((uint32_t)in_channelX) << 0);
    }
}

void sepia_omp(uint32_t *input, uint32_t* output)
{
    const size_t nbCores = NB_THREADS;
    const CoreId_t masterThreadCoreId = 0u;

    for (Thid_t threadId = 1u; threadId < NB_THREADS; ++threadId) {
        const CoreId_t currentThreadCoreId =
                (masterThreadCoreId + threadId) % nbCores;
        const unsigned targetAffinity = (1u << currentThreadCoreId);
        h_thread_setAffinity(threadId, targetAffinity);
    }

    omp_set_num_threads(NB_THREADS);

    #pragma omp parallel
    {
        const int localNbOfThreads = omp_get_num_threads();
        const int threadId = omp_get_thread_num();
        const int i_start = (threadId * PIXELS_FHD) / localNbOfThreads;
        const int i_end = ((threadId + 1) * PIXELS_FHD) / localNbOfThreads;

        for (int i = i_start; i < i_end; i++) {
            uint8_t in_channelR, in_channelG, in_channelB, in_channelX;
            in_channelR = (0xFF & (input[i] >> 24));
            in_channelG = (0xFF & (input[i] >> 16));
            in_channelB = (0xFF & (input[i] >> 8));
            in_channelX = (0xFF & (input[i] >> 0));

            uint32_t tmp_sepiaR;
            uint8_t out_sepiaR;
            tmp_sepiaR =
                    (((uint32_t) in_channelR * 402) + ((uint32_t) in_channelG * 787) + ((uint32_t) in_channelB * 194))
                            >> 10;
            if (tmp_sepiaR > 255)
                out_sepiaR = 255;
            else
                out_sepiaR = tmp_sepiaR;

            uint32_t tmp_sepiaG;
            uint8_t out_sepiaG;
            tmp_sepiaG =
                    (((uint32_t) in_channelR * 357) + ((uint32_t) in_channelG * 702) + ((uint32_t) in_channelB * 172))
                            >> 10;
            if (tmp_sepiaG > 255)
                out_sepiaG = 255;
            else
                out_sepiaG = tmp_sepiaG;

            uint32_t tmp_sepiaB;
            uint8_t out_sepiaB;
            tmp_sepiaB =
                    (((uint32_t) in_channelR * 279) + ((uint32_t) in_channelG * 547) + ((uint32_t) in_channelB * 134))
                            >> 10;
            if (tmp_sepiaB > 255)
                out_sepiaB = 255;
            else
                out_sepiaB = tmp_sepiaB;

            output[i] =
                    (((uint32_t) out_sepiaR) << 24) | (((uint32_t) out_sepiaG) << 16) | (((uint32_t) out_sepiaB) << 8) |
                    (((uint32_t) in_channelX) << 0);
        }
    }
}
