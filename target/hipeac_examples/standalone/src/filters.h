#include "img_filter_base.h"

//#include "img_filter_test.h"
#include "img_other_base.h"
//#include "img_other_test.h"
#include "img_pixelop_base.h"
//#include "img_pixelop_test.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))


#ifndef MAX(x, y)
#define MAX(x, y)           (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN(x, y)
#define MIN(x, y)           (((x) < (y)) ? (x) : (y))
#endif

/*********************************************************************************************************************/
/* System configurations */
/*********************************************************************************************************************/

// Image Size
#define COLS_FHD 1920
#define ROWS_FHD 1080
#define PIXELS_FHD COLS_FHD*ROWS_FHD
#define COLS_HD 1280
#define ROWS_HD 720
#define PIXELS_HD COLS_HD*ROWS_HD

// Vector size must be sizeof(imgVectorT) / be sizeof(imgUintT)
// unfortunately sdsoc compiler has problems with sizeof() in defines
#define VECTOR_SIZE 1
#define VECTOR_PIXELS PIXELS_FHD / VECTOR_SIZE

// Number of bins and offset for LUT and Histogram functions
#define LUT_BINS 256
#define LUT_OFFSET 0

// Kernel size of the filters
#define BOX_FILTER_SIZE 3
#define CONVOLVE_SIZE 3
#define DILATE_SIZE 3
#define ERODE_SIZE 3
#define GAUSSIAN_SIZE 3
#define MEDIAN_SIZE 3
#define SOBEL_SIZE 3

// How are filters handled, when they exceed the border
#define BOX_BORDER      VX_BORDER_CONSTANT
#define CONVOLVE_BORDER VX_BORDER_CONSTANT
#define DILATE_BORDER   VX_BORDER_REPLICATE
#define ERODE_BORDER    VX_BORDER_REPLICATE
#define GAUSSIAN_BORDER VX_BORDER_CONSTANT
#define MEDIAN_BORDER   VX_BORDER_REPLICATE
#define SCHARR_BORDER   VX_BORDER_CONSTANT
#define SOBEL_BORDER    VX_BORDER_CONSTANT

// Use separable filters if available
#define BOX_SEPARABLE      true
#define GAUSSIAN_SEPARABLE true

// Policies and scale for pixel operations
#define ADD_CONVERT_POLICY VX_CONVERT_POLICY_WRAP
#define SUB_CONVERT_POLICY VX_CONVERT_POLICY_WRAP
#define MUL_CONVERT_POLICY VX_CONVERT_POLICY_WRAP
#define MUL_ROUND_POLICY   VX_ROUND_POLICY_TO_ZERO
#define MUL_SCALE          static_cast<vx_uint16>(16384)

// Typedefs for Bit Depth Conversion
typedef vx_int16 convDepthInScalT;
typedef vx_uint8 convDepthOutScalT;
typedef vx_int32 convDepthInVecT;
typedef vx_uint16 convDepthOutVecT;

// The typed of the scalar and vector representation of the image
typedef vx_uint8 imgVectorT;
typedef vx_uint8 imgUintT;
typedef vx_int8  imgIntT;

// Convolution kernel for the HwConvolve filter
const imgUintT convolve_kernel[CONVOLVE_SIZE][CONVOLVE_SIZE] = {
	{ 18, 25, 33 },
	{ 48, 52, 60 },
	{ 73, 86, 94 }
};


/*********************************************************************************************************************/
/* Function prototypes */
/*********************************************************************************************************************/
void hwPassthrough(vx_uint32 *input, vx_uint32* output);
void hwSepia(vx_uint32 *input, vx_uint32* output);
void hwColorConversionRgbxToGray(vx_uint32 *input, vx_uint32* output);
void hwSobelX(vx_uint32 *input, vx_uint32* output);
void hwSobelY(vx_uint32 *input, vx_uint32* output);
void hwScharrX(vx_uint32 *input, vx_uint32* output);
void hwScharrY(vx_uint32 *input, vx_uint32* output);
void hwMedian(vx_uint32 *input, vx_uint32* output);
