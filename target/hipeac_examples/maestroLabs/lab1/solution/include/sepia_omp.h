
#ifndef DEF_SEPIA_OMP_H
#define DEF_SEPIA_OMP_H

#include <inttypes.h>

void sepia_noOmp(uint32_t *input, uint32_t* output);

void sepia_omp(uint32_t *input, uint32_t* output);

#endif /* DEF_SEPIA_OMP_H */
