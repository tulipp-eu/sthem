/**
* @file    main.cpp
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
*
* @section LICENSE
* GNU GPLv3: 
* You may copy, distribute and modify the software as long as you track 
* changes/dates in source files. Any modifications to or software 
* including (via compiler) GPL-licensed code must also be made available 
* under the GPL along with build & install instructions.
*
* @section DESCRIPTION
* This is the main function for testing
*/

#include "img_filter_base.h"
#include "img_filter_test.h"
#include "img_other_base.h"
#include "img_other_test.h"
#include "img_pixelop_base.h"
#include "img_pixelop_test.h"

using namespace std;

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

// Typedefs for Bit Depth Conversion
typedef vx_int16 convDepthInScalT;
typedef vx_uint8 convDepthOutScalT;
typedef vx_int32 convDepthInVecT;
typedef vx_uint16 convDepthOutVecT;

// The typed of the scalar and vector representation of the image
typedef vx_uint16 imgVectorT;
typedef vx_uint16 imgUintT;
typedef vx_int16  imgIntT;

// Vector size must be sizeof(imgVectorT) / be sizeof(imgUintT)
// unfortunately sdsoc compiler has problems with sizeof() in defines
#define VECTOR_SIZE 1
#define VECTOR_PIXELS PIXELS_FHD / VECTOR_SIZE

// Numbher of bins and offset for LUT and Histogram functions
#define LUT_BINS 256
#define LUT_OFFSET 0

// Kernel size of the filters 
#define GAUSSIAN_SIZE 5                            
#define BOX_FILTER_SIZE 3
#define CUSTOM_SIZE 3

// Sigma of 1.0 is calculated 256*1.0. Templates do not accept floating point values
#define GAUSSIAN_SIGMA static_cast<vx_uint16>(256) 

// How are filters handled, when they exceed the border 
#define GAUSSIAN_BORDER VX_BORDER_REPLICATE
#define SCHARR_BORDER   VX_BORDER_REPLICATE // test function only supports replicated
#define SOBEL_BORDER    VX_BORDER_REPLICATE // test function only supports replicated
#define CUSTOM_BORDER   VX_BORDER_REPLICATE
#define BOX_BORDER      VX_BORDER_REPLICATE
#define MEDIAN_BORDER   VX_BORDER_UNDEFINED  // test function only supports unchanged

// Policies and scale for pixel operations
#define ADD_CONVERT_POLICY VX_CONVERT_POLICY_SATURATE
#define SUB_CONVERT_POLICY VX_CONVERT_POLICY_SATURATE
#define MUL_CONVERT_POLICY VX_CONVERT_POLICY_SATURATE
#define MUL_ROUND_POLICY   VX_ROUND_POLICY_TO_NEAREST_EVEN
#define MUL_SCALE          static_cast<vx_uint16>(32768)

// Convolution kernel for the CUSTOM filter 
imgUintT customKernelDev[CUSTOM_SIZE][CUSTOM_SIZE] = {
	{ 1845, 2547, 3328 },
	{ 4837, 5293, 6035 },
	{ 7384, 8642, 9420 }
};
float customKernelHst[CUSTOM_SIZE][CUSTOM_SIZE] = {
	{ 1845, 2547, 3328 },
	{ 4837, 5293, 6035 },
	{ 7384, 8642, 9420 }
};

/*********************************************************************************************************************/
/* Accelerated Filter functions (Wrapper needed, since templates cant be accelerated) */
/*********************************************************************************************************************/

// Computes a Gaussian filter over a window of the input image 
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:VECTOR_PIXELS],output[0:VECTOR_PIXELS])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestGaussian(imgVectorT *input, imgVectorT *output) {
	imgGaussian<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, GAUSSIAN_SIZE, GAUSSIAN_SIGMA, GAUSSIAN_BORDER>(input, output);
}

// Computes a 3x3 Scharr filter over a window of the input image
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, outputX:PHYSICAL_CONTIGUOUS, outputY:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:VECTOR_PIXELS], outputX[0:VECTOR_PIXELS], outputY[0:VECTOR_PIXELS])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, outputX:AXIDMA_SIMPLE, outputY:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, outputX:SEQUENTIAL, outputY:SEQUENTIAL)
void hwTestScharr3x3(imgVectorT *input, imgVectorT *outputX, imgVectorT *outputY) {
	imgScharr3x3<imgUintT, imgIntT, imgVectorT, COLS_FHD, ROWS_FHD, SCHARR_BORDER>(input, outputX, outputY);
}

// Computes a 3x3 Sobel filter over a window of the input image
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, outputX:PHYSICAL_CONTIGUOUS, outputY:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:VECTOR_PIXELS], outputX[0:VECTOR_PIXELS], outputY[0:VECTOR_PIXELS])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, outputX:AXIDMA_SIMPLE, outputY:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, outputX:SEQUENTIAL, outputY:SEQUENTIAL)
void hwTestSobel3x3(imgVectorT *input, imgVectorT *outputX, imgVectorT *outputY) {
	imgSobel3x3<imgUintT, imgIntT, imgVectorT, COLS_FHD, ROWS_FHD, SOBEL_BORDER>(input, outputX, outputY);
}

// Computes a Costum filter over a window of the input image
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:VECTOR_PIXELS],output[0:VECTOR_PIXELS])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestCustomConvolution(imgVectorT *input, imgVectorT *output) {
	imgCustomConvolution<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, CUSTOM_SIZE, CUSTOM_BORDER>(input, output, customKernelDev);
}

// Computes a Box filter over a window of the input image
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:VECTOR_PIXELS],output[0:VECTOR_PIXELS])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestBoxFilter(imgVectorT *input, imgVectorT *output) {
	imgBoxFilter<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, BOX_FILTER_SIZE, BOX_BORDER>(input, output);
}

// Computes a 3x3 Median filter over a window of the input image
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:VECTOR_PIXELS],output[0:VECTOR_PIXELS])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestMedianFilter3x3(imgVectorT *input, imgVectorT *output) {
	imgMedianFilter3x3<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, MEDIAN_BORDER>(input, output);
}

/*********************************************************************************************************************/
/* Accelerated Arithmetic functions */
/*********************************************************************************************************************/

// Computes the absolute difference between two images
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, in2:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], in2[0:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, in2:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, in2:SEQUENTIAL, out:SEQUENTIAL)
void hwTestAbsDiff(imgVectorT *in1, imgVectorT *in2, imgVectorT *out) {
	imgAbsDiff<imgUintT, imgVectorT, PIXELS_FHD>(in1, in2, out);
}

// Performs addition between two images
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, in2:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], in2[0:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, in2:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, in2:SEQUENTIAL, out:SEQUENTIAL)
void hwTestAdd(imgVectorT *in1, imgVectorT *in2, imgVectorT *out) {
	imgAdd<imgUintT, imgVectorT, PIXELS_FHD, ADD_CONVERT_POLICY>(in1, in2, out);
}

// Performs subtraction between two images
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, in2:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], in2[0:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, in2:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, in2:SEQUENTIAL, out:SEQUENTIAL)
void hwTestSub(imgVectorT *in1, imgVectorT *in2, imgVectorT *out) {
	imgSubtract<imgUintT, imgVectorT, PIXELS_FHD, SUB_CONVERT_POLICY>(in1, in2, out);
}

// Implements the Gradient Magnitude Computation Kernel
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, in2:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], in2[0:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, in2:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, in2:SEQUENTIAL, out:SEQUENTIAL)
void hwTestMagnitude(imgVectorT *in1, imgVectorT *in2, imgVectorT *out) {
	imgMagnitude<imgUintT, imgVectorT, PIXELS_FHD>(in1, in2, out);
}

// Performs element-wise multiplication between two images and a scalar value
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, in2:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], in2[0:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, in2:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, in2:SEQUENTIAL, out:SEQUENTIAL)
void hwTestMultiply(imgVectorT *in1, imgVectorT *in2, imgVectorT *out) {
	imgMultiply<imgUintT, imgVectorT, PIXELS_FHD, MUL_CONVERT_POLICY, MUL_ROUND_POLICY, MUL_SCALE>(in1, in2, out);
}

/*********************************************************************************************************************/
/* Accelerated Bitwise functions */
/*********************************************************************************************************************/

// Performs a bitwise AND operation between two unsigned images.
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, in2:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], in2[0:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, in2:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, in2:SEQUENTIAL, out:SEQUENTIAL)
void hwTestAnd(imgVectorT *in1, imgVectorT *in2, imgVectorT *out) {
	imgAnd<imgUintT, imgVectorT, PIXELS_FHD>(in1, in2, out);
}

// Performs a bitwise EXCLUSIVE OR (XOR) operation between two unsigned images.
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, in2:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], in2[0:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, in2:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, in2:SEQUENTIAL, out:SEQUENTIAL)
void hwTestXor(imgVectorT *in1, imgVectorT *in2, imgVectorT *out) {
	imgXor<imgUintT, imgVectorT, PIXELS_FHD>(in1, in2, out);
}

// Performs a bitwise INCLUSIVE OR operation between two unsigned images.
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, in2:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], in2[0:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, in2:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, in2:SEQUENTIAL, out:SEQUENTIAL)
void hwTestOr(imgVectorT *in1, imgVectorT *in2, imgVectorT *out) {
	imgOr<imgUintT, imgVectorT, PIXELS_FHD>(in1, in2, out);
}

// Performs a bitwise NOT operation on a input images.
#pragma SDS data mem_attribute(in1:PHYSICAL_CONTIGUOUS, out:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(in1[O:VECTOR_PIXELS], out[0:VECTOR_PIXELS])
#pragma SDS data data_mover(in1:AXIDMA_SIMPLE, out:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(in1:SEQUENTIAL, out:SEQUENTIAL)
void hwTestNot(imgVectorT *in1, imgVectorT *out) {
	imgNot<imgUintT, imgVectorT, PIXELS_FHD>(in1, out);
}

/*********************************************************************************************************************/
/* Accelerated Other Functions */
/*********************************************************************************************************************/

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestConvertBitDepthDownSaturate(convDepthInVecT *input, convDepthOutVecT *output) {
	imgConvertDepth<convDepthInScalT, convDepthOutScalT, convDepthInVecT, convDepthOutVecT, PIXELS_FHD, VX_CONVERT_POLICY_SATURATE, 8>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestConvertBitDepthDownWrap(convDepthInVecT *input, convDepthOutVecT *output) {
	imgConvertDepth<convDepthInScalT, convDepthOutScalT, convDepthInVecT, convDepthOutVecT, PIXELS_FHD, VX_CONVERT_POLICY_WRAP, 8>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestColorConversionRgbToGray(vx_uint32 *input, vx_uint8* output) {
#pragma HLS inline
	imgConvertColor<vx_uint32, vx_uint8, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_RGB, VX_DF_IMAGE_U8>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestColorConversionRgbxToGray(vx_uint32 *input, vx_uint8* output) {
#pragma HLS inline
	imgConvertColor<vx_uint32, vx_uint8, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_RGBX, VX_DF_IMAGE_U8>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestColorConversionGrayToRgb(vx_uint8 *input, vx_uint32* output) {
#pragma HLS inline
	imgConvertColor<vx_uint8, vx_uint32, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_U8, VX_DF_IMAGE_RGB>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestColorConversionGrayToRgbx(vx_uint8 *input, vx_uint32* output) {
#pragma HLS inline
	imgConvertColor<vx_uint8, vx_uint32, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_U8, VX_DF_IMAGE_RGBX>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_HD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestScaleNearestNeighbor(vx_uint8 *input, vx_uint8* output) {
#pragma HLS inline
	imgScaleDown<COLS_FHD, ROWS_FHD, COLS_HD, ROWS_HD, VX_INTERPOLATION_NEAREST_NEIGHBOR>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_HD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestScaleBilinear(vx_uint8 *input, vx_uint8* output) {
#pragma HLS inline
	imgScaleDown<COLS_FHD, ROWS_FHD, COLS_HD, ROWS_HD, VX_INTERPOLATION_BILINEAR>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestIntegral(vx_uint8 *input, vx_uint32* output) {
#pragma HLS inline
	imgIntegral<COLS_FHD, ROWS_FHD>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestHistogram8BitInput(vx_uint8 *input, vx_uint32* output) {
#pragma HLS inline
	imgHistogram<vx_uint8, PIXELS_FHD, LUT_BINS, (1 << 8), LUT_OFFSET>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestHistogram16BitInput(vx_uint16 *input, vx_uint32* output) {
#pragma HLS inline
	imgHistogram<vx_uint16, PIXELS_FHD, LUT_BINS, (1 << 16), LUT_OFFSET>(input, output);
}

#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, lut:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], lut[O:LUT_COUNT], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, lut:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, lut:SEQUENTIAL, output:SEQUENTIAL)
void hwTestTableLookup8BitInput(vx_uint8 *input, vx_uint8 *lut, vx_uint8 *output) {
#pragma HLS inline
	imgTableLookup<vx_uint8, PIXELS_FHD, LUT_BINS, LUT_OFFSET>(input, lut, output);
}

/*********************************************************************************************************************/
/* Example Application. Gradient Magnitude of a Gaussian smoothed image (including loop level parallelism) */
/*********************************************************************************************************************/

#pragma SDS data copy(input[O:VECTOR_PIXELS], output[0:VECTOR_PIXELS])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwTestExampleApplication(imgVectorT *input, imgVectorT *output) {

	// FIFOs to stream data between functions
	static imgVectorT lsmooth[VECTOR_PIXELS];
#pragma HLS STREAM variable = lsmooth depth = 512
	static imgVectorT lx[VECTOR_PIXELS];
#pragma HLS STREAM variable = lx depth = 512
	static imgVectorT ly[VECTOR_PIXELS];
#pragma HLS STREAM variable = ly depth = 512

	// Pragma to stream between loops/functions
#pragma HLS DATAFLOW

	// Smooth the image with a gaussian convolution
	imgGaussian<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, GAUSSIAN_SIZE, GAUSSIAN_SIGMA, GAUSSIAN_BORDER>(input, lsmooth);

	// Compute the derivatives in X and Y direction
	imgScharr3x3<imgUintT, imgIntT, imgVectorT, COLS_FHD, ROWS_FHD, SCHARR_BORDER>(lsmooth, lx, ly);

	// Compute the Gradient Magnitude 
	imgMagnitude<imgIntT, imgVectorT, PIXELS_FHD>(lx, ly, output);
}

/*********************************************************************************************************************/
/* Main function */
/*********************************************************************************************************************/
int main(void) {

	/******** Call Test Functions ********************************************************************/

	//swTestOtherMain();
	//swTestPixelopMain();
	//swTestFilterMain();
	//getchar();

	/******** Allocate memory ********************************************************************/

#ifdef SDSOC
	// Create device buffers for Filter/arithmetic/Bitwise Functions
	imgUintT *inputA = (imgUintT*)sds_alloc(sizeof(imgUintT)*PIXELS_FHD);
	imgUintT *inputB = (imgUintT*)sds_alloc(sizeof(imgUintT)*PIXELS_FHD);
	imgUintT *output = (imgUintT*)sds_alloc(sizeof(imgUintT)*PIXELS_FHD);
	imgIntT *outputX = (imgIntT*)sds_alloc(sizeof(imgIntT)*PIXELS_FHD);
	imgIntT *outputY = (imgIntT*)sds_alloc(sizeof(imgIntT)*PIXELS_FHD);

	// Create device buffers for other Functions
	convDepthInScalT *input_fhd_vec = (convDepthInScalT*)sds_alloc(sizeof(convDepthInScalT)*PIXELS_FHD);
	convDepthOutScalT *output_fhd_vec = (convDepthOutScalT*)sds_alloc(sizeof(convDepthOutScalT)*PIXELS_FHD);
	vx_uint8 *input_fhd_8 = (vx_uint8*)sds_alloc(sizeof(vx_uint8)*PIXELS_FHD);
	vx_uint16 *input_fhd_16 = (vx_uint16*)sds_alloc(sizeof(vx_uint16)*PIXELS_FHD);
	vx_uint32 *input_fhd_32 = (vx_uint32*)sds_alloc(sizeof(vx_uint32)*PIXELS_FHD);
	vx_uint8 *output_fhd_8 = (vx_uint8*)sds_alloc(sizeof(vx_uint8)*PIXELS_FHD);
	vx_uint32 *output_fhd_32 = (vx_uint32*)sds_alloc(sizeof(vx_uint32)*PIXELS_FHD);
	vx_uint8 *output_hd_8 = (vx_uint8*)sds_alloc(sizeof(vx_uint8)*PIXELS_HD);
	vx_uint32 *histogram_bins = (vx_uint32*)sds_alloc(sizeof(vx_uint32)*LUT_BINS);
	vx_uint8 *lookuptable_bins = (vx_uint8*)sds_alloc(sizeof(vx_uint8)*LUT_BINS);
#else
	// Create device buffers for Filter/arithmetic/Bitwise Functions
	imgUintT *inputA = new imgUintT[PIXELS_FHD];
	imgUintT *inputB = new imgUintT[PIXELS_FHD];
	imgUintT *output = new imgUintT[PIXELS_FHD];
	imgIntT *outputX = new imgIntT[PIXELS_FHD];
	imgIntT *outputY = new imgIntT[PIXELS_FHD];

	// Create device buffers for Other Functions
	convDepthInScalT *input_fhd_vec = new convDepthInScalT[PIXELS_FHD];
	convDepthOutScalT *output_fhd_vec = new convDepthOutScalT[PIXELS_FHD];
	vx_uint8 *input_fhd_8 = new vx_uint8[PIXELS_FHD];
	vx_uint16 *input_fhd_16 = new vx_uint16[PIXELS_FHD];
	vx_uint32 *input_fhd_32 = new vx_uint32[PIXELS_FHD];
	vx_uint8 *output_fhd_8 = new vx_uint8[PIXELS_FHD];
	vx_uint32 *output_fhd_32 = new vx_uint32[PIXELS_FHD];
	vx_uint8 *output_hd_8 = new vx_uint8[PIXELS_FHD];
	vx_uint32 *histogram_bins = new vx_uint32[LUT_BINS];
	vx_uint8 *lookuptable_bins = new vx_uint8[LUT_BINS];

#endif

	/******** Call HW Functions ********************************************************************/

	// Filter Functions
	hwTestGaussian((imgVectorT *)inputA, (imgVectorT *)output);
	hwTestScharr3x3((imgVectorT *)inputA, (imgVectorT *)outputX, (imgVectorT *)outputY);
	hwTestSobel3x3((imgVectorT *)inputA, (imgVectorT *)outputX, (imgVectorT *)outputY);
	hwTestCustomConvolution((imgVectorT *)inputA, (imgVectorT *)output);
	hwTestBoxFilter((imgVectorT *)inputA, (imgVectorT *)output);
	hwTestMedianFilter3x3((imgVectorT *)inputA, (imgVectorT *)output);

	// Arithmetic Operations
	hwTestAbsDiff((imgVectorT *)inputA, (imgVectorT *)inputB, (imgVectorT *)output);
	hwTestAdd((imgVectorT *)inputA, (imgVectorT *)inputB, (imgVectorT *)output);
	hwTestSub((imgVectorT *)inputA, (imgVectorT *)inputB, (imgVectorT *)output);
	hwTestMultiply((imgVectorT *)inputA, (imgVectorT *)inputB, (imgVectorT *)output);
	hwTestMagnitude((imgVectorT *)inputA, (imgVectorT *)inputB, (imgVectorT *)output);

	// Bitwise operations
	hwTestAnd((imgVectorT *)inputA, (imgVectorT *)inputB, (imgVectorT *)output);
	hwTestXor((imgVectorT *)inputA, (imgVectorT *)inputB, (imgVectorT *)output);
	hwTestOr((imgVectorT *)inputA, (imgVectorT *)inputB, (imgVectorT *)output);
	hwTestNot((imgVectorT *)inputA, (imgVectorT *)output);

	// Other Functions
	hwTestConvertBitDepthDownSaturate((convDepthInVecT *)input_fhd_vec, (convDepthOutVecT *)output_fhd_vec);
	hwTestConvertBitDepthDownWrap((convDepthInVecT *)input_fhd_vec, (convDepthOutVecT *)output_fhd_vec);
	hwTestColorConversionRgbToGray(input_fhd_32, output_fhd_8);
	hwTestColorConversionRgbxToGray(input_fhd_32, output_fhd_8);
	hwTestColorConversionGrayToRgb(input_fhd_8, output_fhd_32);
	hwTestColorConversionGrayToRgbx(input_fhd_8, output_fhd_32);
	hwTestScaleNearestNeighbor(input_fhd_8, output_hd_8);
	hwTestScaleBilinear(input_fhd_8, output_hd_8);
	hwTestIntegral(input_fhd_8, output_fhd_32);
	hwTestHistogram8BitInput(input_fhd_8, histogram_bins);
	hwTestHistogram16BitInput(input_fhd_16, histogram_bins);
	hwTestTableLookup8BitInput(input_fhd_8, lookuptable_bins, output_fhd_8);

	/******** Call Example Applications ********************************************************************/

	// Compute the example application
	hwTestExampleApplication((imgVectorT *)inputA, (imgVectorT *)outputX);

	/******** Free memory ************************************************************************/

#ifdef SDSOC
	sds_free(inputA);
	sds_free(inputB);
	sds_free(output);
	sds_free(outputX);
	sds_free(outputY);
	sds_free(input_fhd_vec);
	sds_free(output_fhd_vec);
	sds_free(input_fhd_8);
	sds_free(input_fhd_16);
	sds_free(input_fhd_32);
	sds_free(output_fhd_8);
	sds_free(output_fhd_32);
	sds_free(output_hd_8);
#else
	delete[] inputA;
	delete[] inputB;
	delete[] output;
	delete[] outputX;
	delete[] outputY;
	delete[] input_fhd_vec;
	delete[] output_fhd_vec;
	delete[] input_fhd_8;
	delete[] input_fhd_16;
	delete[] input_fhd_32;
	delete[] output_fhd_8;
	delete[] output_fhd_32;
	delete[] output_hd_8;
	delete[] histogram_bins;
	delete[] lookuptable_bins;
#endif

	cout << "Finished" << endl;
	getchar();
	return 0;
}


//#ifdef OPENCV
//#include <opencv2/calib3d.hpp>
//#include <opencv2/imgproc.hpp>
//#include <opencv2/highgui.hpp>
//static const char* filenameA = "C:/Users/Lester/Documents/Visual Studio 2013/Projects/OpenCV_Test/datasets/boat/img1.pgm";
//static const char* filenameB = "C:/Users/Lester/Documents/Visual Studio 2013/Projects/OpenCV_Test/datasets/bikes/img1.ppm";
//#endif
//#ifdef OPENCV
//// Read Image from file
//cv::Mat imgA = cv::imread(filenameA, 0);
//cv::Mat imgB = cv::imread(filenameB, 0);
//
//// Resize image to design resolution
//resize(imgA, imgA, cv::Size(COLS_FHD, ROWS_FHD), 0, 0, cv::INTER_NEAREST);
//resize(imgB, imgB, cv::Size(COLS_FHD, ROWS_FHD), 0, 0, cv::INTER_NEAREST);
//
//// Transform image to floating point format
//for (int i = 0; i < PIXELS; i++) {
//	imgHostA[i] = (float)(((uint8_t *)imgA.data)[i]) / 256.0f;
//	imgHostB[i] = (float)(((uint8_t *)imgB.data)[i]) / 256.0f;
//}
//
//// Initially smooth image with a 7x7 gaussian with sigma = 1.5
//swTestGaussian<7, 384, VX_BORDER_REPLICATE>(imgHostA, imgHostC, ROWS_FHD, COLS_FHD);
//swTestGaussian<7, 384, VX_BORDER_REPLICATE>(imgHostB, imgHostD, ROWS_FHD, COLS_FHD);
//
//#else
//#endif