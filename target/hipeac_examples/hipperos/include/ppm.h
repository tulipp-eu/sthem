#ifndef PPM_H
#define PPM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "ff.h"


#if defined(__hipperos__)
#include <hipperos/types.h>
#endif

#define READ_BUFLEN 255
#define HEADER_MAX_LEN 255


typedef struct pixel {
    u8 r;
    u8 g;
    u8 b;
} pixel;

typedef struct ppmImage {
    pixel* buffer;
    unsigned int width;
    unsigned int height;
} ppmImage;

void ppmToStream(u32 *stream, ppmImage *image);
void streamToPpm(u32 *stream, ppmImage *image);
int readPpmToStream(u32 *stream, char const *filename, unsigned int const width, unsigned int const height);
int writeStreamToPpm(u32 *stream, char const *filename, unsigned int const width, unsigned int const height);
int ppmReadToWhitespace(FIL *fp, char *buf, unsigned int const max);
void ppmDiscardLine(FIL *fp);
ppmImage *readPpm(char const *filename);
void freePpm(ppmImage *image);
int writePpm (char const *filename, ppmImage * image);

#ifdef __cplusplus
}
#endif

#endif
