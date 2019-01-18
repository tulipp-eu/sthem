#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "ppm.h"

#include "ff.h"

void ppmToStream(u32 *stream, ppmImage *image) {
	unsigned int const width = image->width;
	unsigned int const height = image->height;
	for(u32 y=0; y<height; y++) {
		for(u32 x=0; x<width; x++) {
			stream[y*width+x] = ((u32) image->buffer[y*width+x].r << 24) | ((u32) image->buffer[y*width+x].g << 16) | ((u32) image->buffer[y*width+x].b << 8);
		}
	}
}

void streamToPpm(u32 *stream, ppmImage *image) {
	unsigned int const width = image->width;
	unsigned int const height = image->height;
	for(u32 y=0; y<height; y++) {
		for(u32 x=0; x<width; x++) {
			image->buffer[y*width+x].r = (u8) (0xFF & (stream[y*width+x] >> 24));
			image->buffer[y*width+x].g = (u8) (0xFF & (stream[y*width+x] >> 16));
			image->buffer[y*width+x].b = (u8) (0xFF & (stream[y*width+x] >> 8));
		}
	}
}

int readPpmToStream(u32 *stream, char const *filename, unsigned int const width, unsigned int const height) {
	ppmImage *image = readPpm(filename);
	if ((image == NULL) || (image->width != width) || (image->height != height)) {
		return 1;
	}
	ppmToStream(stream, image);
	freePpm(image);
	return 0;
}

int writeStreamToPpm(u32 *stream, char const *filename, unsigned int const width, unsigned int const height) {
	int ret = 0;
	ppmImage *image = (ppmImage*)malloc(sizeof(ppmImage));
	image->width = width;
	image->height = height;
	image->buffer = (pixel *) malloc(width * height * sizeof(pixel));
  if(!image->buffer) return 1;
	streamToPpm(stream, image);
	ret = writePpm(filename, image);
	freePpm(image);
	return (ret >= 0) ? 0 : 1;
}


int ppmReadToWhitespace(FIL *fp, char *buf, unsigned int const max) {
	unsigned int b = 0;
	if (f_read(fp, buf, max, &b) != FR_OK || b == 0) {
		memset(buf, '\0', max);
		return -1;
	}
	for (unsigned int i = 0; i < b; i++) {
		if (isspace(buf[i])) {
			f_lseek(fp, f_tell(fp) - b + i + 1);
			memset(&buf[i], '\0', max - i);
			return i;
		}
	}
	if (b < max) {
		memset(&buf[b], '\0', max - b);
	}
	return b;
}

void ppmDiscardLine(FIL *fp) {
    char c = 0;
    unsigned int b = 0;
    do {
    	b = 0;
    	if (f_read(fp, &c, 1, &b) != FR_OK) {
    		break;
    	}
    } while (b == 1 && c != '\n');
}

ppmImage *readPpm(char const *filename) {
    ppmImage *image = (ppmImage *) malloc(sizeof(ppmImage));
    char buf[READ_BUFLEN+1]; memset(buf, '\0', READ_BUFLEN+1);
    int bytesRead = 0;
    unsigned int rawBytesRead = 0;
    FIL fp;
    unsigned int bytesToRead = 0;
    long scale = 0;
    unsigned int headerState = 0;

    if (image == NULL) {
    	fprintf(stderr, "Cannot allocate image!\n");
        goto ERR_MEM;
    }

    if (f_open(&fp, filename, FA_READ) != FR_OK) {
    	fprintf(stderr, "Cannot open '%s'!\n", filename);
        goto ERR_FILE;
    }

    headerState = 0;
    bytesRead = 0;
    while (headerState < 4 && bytesRead >= 0) {
    	bytesRead = ppmReadToWhitespace(&fp, buf, READ_BUFLEN);
    	if (bytesRead > 0 || headerState == 0) {
			switch (headerState) {
				case 0:
					if (bytesRead != 2 || buf[0] != 'P' || buf[1] != '6') {
						goto ERR_PPM;
					}
					headerState++;
					break;
				case 1:
				case 2:
					if (buf[0] == '#') {
						ppmDiscardLine(&fp);
					} else {
						if (headerState == 1) {
							image->width = strtol(buf, NULL, 10);
						} else {
							image->height = strtol(buf, NULL, 10);
						}
						if (errno == ERANGE) {
							goto ERR_PPM;
						}
						headerState++;
					}
					break;
				case 3:
					scale = strtol(buf, NULL, 10);
					if (errno == ERANGE || scale != 255) {
						goto ERR_PPM;
					}
					headerState++;
					break;
			}
    	}
    }
    if (bytesRead < 0) {
    	goto ERR_PPM;
    }

    image->buffer = (pixel*) malloc(sizeof(pixel) * image->width * image->height);
    if (image->buffer == NULL) {
    	fprintf(stderr, "Cannot allocate image buffer!\n");
        goto ERR_BUF;
    }

    bytesToRead = image->width * image->height * sizeof(pixel);
    rawBytesRead = 0;
    f_read(&fp, (char *) image->buffer, bytesToRead, &rawBytesRead);
    if (rawBytesRead != bytesToRead) {
    	free(image->buffer);
    	goto ERR_BUF;
    }

    f_close(&fp);

    return image;
ERR_PPM:
	fprintf(stderr, "Invalid ppm format!\n");
ERR_BUF:
	f_close(&fp);
ERR_FILE:
	free(image);
ERR_MEM:
    return NULL;
}

void freePpm(ppmImage *image) {
    free(image->buffer);
    free(image);
}

int writePpm (char const *filename, ppmImage * image) {
    char buf[HEADER_MAX_LEN+1]; memset(buf, '\0', HEADER_MAX_LEN+1);
    unsigned int bytesToWrite = 0;
    unsigned int bytesWritten = 0;
    int lenHeader = 0;
    FIL fp;

    lenHeader = sprintf(buf, "P6\n%u %u\n255\n", image->width, image->height);

    if (lenHeader < 0) {
        goto ERR;
    }

    if (f_open(&fp, filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        goto ERR;
    }

    if (f_write(&fp, buf, lenHeader, &bytesWritten) != FR_OK || (unsigned int) lenHeader != bytesWritten) {
        goto ERR;
    };

    bytesToWrite = image->width * image->height * sizeof(pixel);
    bytesWritten = 0;
    if (f_write(&fp, image->buffer, bytesToWrite, &bytesWritten) != FR_OK || bytesWritten != bytesToWrite) {
        goto ERR;
    }

    f_close(&fp);

    return lenHeader + bytesToWrite;
ERR:
    fprintf(stderr, "Could not save ppm to '%s'!\n", filename);
    return -1;
}
