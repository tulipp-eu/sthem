/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track
* changes/dates in source files. Any modifications to or software
* including (via compiler) GPL-licensed code must also be made available
* under the GPL along with build & install instructions.
*
* @file    img_filter_test.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These functions have been implemented to test the functionality and accuracy of the sdsoc functions
*/

#ifndef SRC_IMG_FILTER_TEST_H_
#define SRC_IMG_FILTER_TEST_H_

#include "img_filter_base.h"

using namespace std;

/*********************************************************************************************************************/
/* Test Functions Headers */
/*********************************************************************************************************************/

template<const vx_int32 KERN_SIZE, const vx_int32 SIGMA, const vx_border_e BORDER_TYPE>
void swTestGaussian(const float * input, float * output, const vx_int32 rows, const vx_int32 cols);
void swTestScharr3x3(const float * input, float * outputLx, float * outputLy, const vx_int32 rows, const vx_int32 cols);
void swTestSobel3x3(const float * input, float * outputLx, float * outputLy, const vx_int32 rows, const vx_int32 cols);
template<const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void swTestCustomConvolution(const float *input, float *output, float imageKernel[KERN_SIZE][KERN_SIZE], const vx_int32 rows, const vx_int32 cols);
template<const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void swTestBoxFilter(const float * input, float * output, const vx_int32 rows, const vx_int32 cols);
void swTestMedianFilter3x3(const float *input, float *output, const vx_int32 rows, const vx_int32 cols);

/*********************************************************************************************************************/
/* SW Test Main Function */
/*********************************************************************************************************************/

/*! \brief Test all filter functions */
void swTestFilterMain(void) {

	/******** Constants to test the functions ********************************************************************/

	const vx_uint32 COLS_FHD = 1920;
	const vx_uint32  ROWS_FHD = 1080;
	const vx_uint32  PIXELS_FHD = COLS_FHD*ROWS_FHD;

	// The typed of the scalar and vector representation of the image
	typedef vx_uint16 imgVectorT;
	typedef vx_uint16 imgUintT;
	typedef vx_int16  imgIntT;

	// Kernel size of the filters 
	const vx_uint32  GAUSSIAN_SIZE = 5;
	const vx_uint32  BOX_FILTER_SIZE = 3;
	const vx_uint32  CUSTOM_SIZE = 3;

	// Sigma of 1.0 is calculated 256*1.0. Templates do not accept floating point values
	const vx_uint32  GAUSSIAN_SIGMA = static_cast<vx_uint16>(256);

	// How are filters handled, when they exceed the border 
	const vx_border_e  GAUSSIAN_BORDER = VX_BORDER_REPLICATE;
	const vx_border_e  SCHARR_BORDER = VX_BORDER_REPLICATE; // test function only supports replicated
	const vx_border_e  SOBEL_BORDER = VX_BORDER_REPLICATE; // test function only supports replicated
	const vx_border_e  CUSTOM_BORDER = VX_BORDER_REPLICATE;
	const vx_border_e  BOX_BORDER = VX_BORDER_REPLICATE;
	const vx_border_e  MEDIAN_BORDER = VX_BORDER_UNDEFINED;  // test function only supports unchanged

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

	/******** Allocate memory ********************************************************************/

	// Create host buffers 
	float *imgHostA = new float[PIXELS_FHD];
	float *imgHostB = new float[PIXELS_FHD];
	float *imgHostC = new float[PIXELS_FHD];
	float *imgHostD = new float[PIXELS_FHD];
	float *outputHstA = new float[PIXELS_FHD];
	float *outputHstB = new float[PIXELS_FHD];
	float *outputDevA = new float[PIXELS_FHD];
	float *outputDevB = new float[PIXELS_FHD];

	// Create device buffers
	imgUintT *inputA = new imgUintT[PIXELS_FHD];
	imgUintT *inputB = new imgUintT[PIXELS_FHD];
	imgUintT *output = new imgUintT[PIXELS_FHD];
	imgIntT *outputX = new imgIntT[PIXELS_FHD];
	imgIntT *outputY = new imgIntT[PIXELS_FHD];

	/******** Read or create image ***************************************************************/

	// Create an image :) and transform it to floating point format
	bool turn = true;
	for (vx_int32 i = 0, count = 0; i < PIXELS_FHD; i++) {
		count = count + (turn ? 1 : -1);
		if (count >= 222 || count <= 0) turn = !turn;
		float m = ((float)(count + (rand() % 34)));
		float n = ((float)(count + (rand() % 34)));
		imgHostA[i] = m / 256.0f;
		imgHostB[i] = n / 256.0f;
	}

	// Initially smooth image with a 7x7 gaussian with sigma = 1.5
	swTestGaussian<7, 384, VX_BORDER_REPLICATE>(imgHostA, imgHostC, ROWS_FHD, COLS_FHD);
	swTestGaussian<7, 384, VX_BORDER_REPLICATE>(imgHostB, imgHostD, ROWS_FHD, COLS_FHD);

	/******** Test accelerated functions *********************************************************/

	// Convert to fixed point
	convertFloatingToInteger<imgUintT, PIXELS_FHD>(imgHostC, inputA);
	convertFloatingToInteger<imgUintT, PIXELS_FHD>(imgHostD, inputB);

	// Compute Gaussian	
	imgGaussian<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, GAUSSIAN_SIZE, GAUSSIAN_SIGMA, GAUSSIAN_BORDER>((imgVectorT *)inputA, (imgVectorT *)output);
	convertIntegerToFloating<imgUintT, PIXELS_FHD>(output, outputDevA);
	cout << "Gaussian:\n";
	swTestGaussian<GAUSSIAN_SIZE, GAUSSIAN_SIGMA, GAUSSIAN_BORDER>(imgHostC, outputHstA, ROWS_FHD, COLS_FHD);
	checkErrorFloat<PIXELS_FHD>(outputHstA, outputDevA);

	// Compute Scharr	
	imgScharr3x3<imgUintT, imgIntT, imgVectorT, COLS_FHD, ROWS_FHD, SCHARR_BORDER>((imgVectorT *)inputA, (imgVectorT *)outputX, (imgVectorT *)outputY);
	convertIntegerToFloating<imgIntT, PIXELS_FHD>(outputX, outputDevA);
	convertIntegerToFloating<imgIntT, PIXELS_FHD>(outputY, outputDevB);
	cout << "Scharr:\n";
	swTestScharr3x3(imgHostC, outputHstA, outputHstB, ROWS_FHD, COLS_FHD);
	checkErrorFloat<PIXELS_FHD>(outputHstA, outputDevA);
	checkErrorFloat<PIXELS_FHD>(outputHstB, outputDevB);

	// Compute Sobel	
	imgSobel3x3<imgUintT, imgIntT, imgVectorT, COLS_FHD, ROWS_FHD, SOBEL_BORDER>((imgVectorT *)inputA, (imgVectorT *)outputX, (imgVectorT *)outputY);
	convertIntegerToFloating<imgIntT, PIXELS_FHD>(outputX, outputDevA);
	convertIntegerToFloating<imgIntT, PIXELS_FHD>(outputY, outputDevB);
	cout << "Sobel:\n";
	swTestSobel3x3(imgHostC, outputHstA, outputHstB, ROWS_FHD, COLS_FHD);
	checkErrorFloat<PIXELS_FHD>(outputHstA, outputDevA);
	checkErrorFloat<PIXELS_FHD>(outputHstB, outputDevB);

	// Custom Convolution Filter	
	imgCustomConvolution<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, CUSTOM_SIZE, CUSTOM_BORDER>((imgVectorT *)inputA, (imgVectorT *)output, customKernelDev);
	convertIntegerToFloating<imgUintT, PIXELS_FHD>(output, outputDevA);
	cout << "Custom Convolution:\n";
	swTestCustomConvolution<CUSTOM_SIZE, CUSTOM_BORDER>(imgHostC, outputHstA, customKernelHst, ROWS_FHD, COLS_FHD);
	checkErrorFloat<PIXELS_FHD>(outputHstA, outputDevA);

	// Compute Box Filter	
	imgBoxFilter<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, BOX_FILTER_SIZE, BOX_BORDER>((imgVectorT *)inputA, (imgVectorT *)output);
	convertIntegerToFloating<imgUintT, PIXELS_FHD>(output, outputDevA);
	cout << "Box Filter:\n";
	swTestBoxFilter<BOX_FILTER_SIZE, BOX_BORDER>(imgHostC, outputHstA, ROWS_FHD, COLS_FHD);
	checkErrorFloat<PIXELS_FHD>(outputHstA, outputDevA);

	// Test Median Filter	
	imgMedianFilter3x3<imgUintT, imgVectorT, COLS_FHD, ROWS_FHD, MEDIAN_BORDER>((imgVectorT *)inputA, (imgVectorT *)output);
	convertIntegerToFloating<imgUintT, PIXELS_FHD>(output, outputDevA);
	cout << "Median Filter:\n";
	swTestMedianFilter3x3(imgHostC, outputHstA, ROWS_FHD, COLS_FHD);
	checkErrorFloat<PIXELS_FHD>(outputHstA, outputDevA);

	// Free Memory
	delete[] imgHostA;
	delete[] imgHostB;
	delete[] imgHostC;
	delete[] imgHostD;
	delete[] outputHstA;
	delete[] outputHstB;
	delete[] outputDevA;
	delete[] outputDevB;
	delete[] inputA;
	delete[] inputB;
	delete[] output;
	delete[] outputX;
	delete[] outputY;
}

/*********************************************************************************************************************/
/* SW Test Functions */
/*********************************************************************************************************************/

/**
@brief  Computes a GAUSSIAN filter of variable size on an image
@param KERN_SIZE    The kernel size of the filter
@param SIGMA        The standard deviation sigma
@param BORDER_TYPE  The border type if filter exceeds border
@param input        The input image
@param output       The output image
@param rows         The image height
@param cols         The image width
*/
template<const vx_int32 KERN_SIZE, const vx_int32 SIGMA, const vx_border_e BORDER_TYPE>
void swTestGaussian(const float * input, float * output, const vx_int32 rows, const vx_int32 cols) {

	// Constants
	const vx_uint16 KERN_RAD = KERN_SIZE / 2;
	const float SIGM = static_cast<float>(SIGMA) / 256.0f;
	const float s = 2.0f * SIGM * SIGM;

	// Variables
	float sum = 0.0f;
	float gaussKernel[KERN_SIZE][KERN_SIZE];

	// Generate gaussian kernel
	for (vx_uint16 x = 0; x < KERN_SIZE; x++) {
#pragma HLS unroll
		for (vx_uint16 y = 0; y < KERN_SIZE; y++) {
#pragma HLS unroll
			const vx_uint32 a = static_cast<vx_uint32>(x - KERN_RAD);
			const vx_uint32 b = static_cast<vx_uint32>(y - KERN_RAD);
			const float c = static_cast<float>(a*a + b*b);
			const float r = sqrtf(c);
			const float result = (expf(-(r*r) / s)) / (static_cast<float>(M_PI) * s);
			gaussKernel[x][y] = result;
			sum += result;
		}
	}

	// Normalize gaussian kernel
	for (vx_uint16 x = 0; x < KERN_SIZE; ++x) {
#pragma HLS unroll
		for (vx_uint16 y = 0; y < KERN_SIZE; ++y) {
#pragma HLS unroll
			gaussKernel[x][y] /= sum;
		}
	}

	// Compute gaussian filter
	for (vx_int32 y = 0; y < rows; y++) {
		for (vx_int32 x = 0; x < cols; x++) {

			// Data is passed through if filter exceeds border
			if (BORDER_TYPE == VX_BORDER_UNDEFINED) {
				if (x < KERN_RAD || x >= cols - KERN_RAD || y < KERN_RAD || y >= rows - KERN_RAD) {
					output[y*cols + x] = input[y*cols + x];
					continue;
				}
			}

			// Compute single gaussian filter
			sum = 0.0f;
			for (vx_int32 i = -KERN_RAD; i <= KERN_RAD; i++) {
				for (vx_int32 j = -KERN_RAD; j <= KERN_RAD; j++) {

					// Replicated border
					if (BORDER_TYPE == VX_BORDER_REPLICATE) {
						vx_int32 yi = min(max(y + i, static_cast<vx_int32>(0)), rows - 1);
						vx_int32 xj = min(max(x + j, static_cast<vx_int32>(0)), cols - 1);
						sum += input[yi*cols + xj] * gaussKernel[i + KERN_RAD][j + KERN_RAD];

						// Constant border
					}
					else if (BORDER_TYPE == VX_BORDER_CONSTANT) {
						vx_int32 yi = y + i;
						vx_int32 xj = x + j;
						if ((xj >= 0) && (xj < cols) && (yi >= 0) && (yi < rows))
							sum += input[yi*cols + xj] * gaussKernel[i + KERN_RAD][j + KERN_RAD];

						// Data is passed through if filter exceeds border
					}
					else if (BORDER_TYPE == VX_BORDER_UNDEFINED) {
						vx_int32 yi = y + i;
						vx_int32 xj = x + j;
						sum += input[yi*cols + xj] * gaussKernel[i + KERN_RAD][j + KERN_RAD];
					}
				}
			}

			// Store result
			output[y*cols + x] = sum;
		}
	}
}

/**
@brief Computes a 3x3 SCHARR filter on an image
@param input     The input image
@param outputLx  The output image using the x derivative
@param outputLy  The output image using the y derivative
@param rows      The image height
@param cols      The image width
*/
void swTestScharr3x3(const float * input, float * outputLx, float * outputLy, const vx_int32 rows, const vx_int32 cols) {

	for (vx_int32 ym = 0; ym < rows; ym++) {

		// Y pointer
		vx_int32 yl = max(ym - 1, static_cast<vx_int32>(0));
		vx_int32 yh = min(ym + 1, rows - 1);
		const float* src_l = &input[yl * cols];
		const float* src_m = &input[ym * cols];
		const float* src_h = &input[yh * cols];
		float* Lx_m = &outputLx[ym * cols];
		float* Ly_m = &outputLy[ym * cols];

		for (vx_int32 xm = 0; xm < cols; xm++) {

			// X pointer
			vx_int32 xl = max(xm - 1, static_cast<vx_int32>(0));
			vx_int32 xh = min(xm + 1, cols - 1);

			// Compute Scharr(x, y)
			Lx_m[xm] = 0.09375f * (src_l[xh] + src_h[xh] - src_l[xl] - src_h[xl]) + 0.3125f * (src_m[xh] - src_m[xl]);
			Ly_m[xm] = 0.09375f * (src_h[xl] + src_h[xh] - src_l[xl] - src_l[xh]) + 0.3125f * (src_h[xm] - src_l[xm]);
		}
	}
}

/**
@brief Computes a 3x3 SOBEL filter on an image
@param input     The input image
@param outputLx  The output image using the x derivative
@param outputLy  The output image using the y derivative
@param rows      The image height
@param cols      The image width
*/
void swTestSobel3x3(const float * input, float * outputLx, float * outputLy, const vx_int32 rows, const vx_int32 cols) {

	for (vx_int32 ym = 0; ym < rows; ym++) {

		// Y pointer
		vx_int32 yl = max(ym - 1, static_cast<vx_int32>(0));
		vx_int32 yh = min(ym + 1, rows - 1);
		const float* src_l = &input[yl * cols];
		const float* src_m = &input[ym * cols];
		const float* src_h = &input[yh * cols];
		float* Lx_m = &outputLx[ym * cols];
		float* Ly_m = &outputLy[ym * cols];

		for (vx_int32 xm = 0; xm < cols; xm++) {

			// X pointer
			vx_int32 xl = max(xm - 1, static_cast<vx_int32>(0));
			vx_int32 xh = min(xm + 1, cols - 1);

			// Compute Sobel(x, y)
			Lx_m[xm] = 0.125f * (src_l[xh] + src_h[xh] - src_l[xl] - src_h[xl]) + 0.25f * (src_m[xh] - src_m[xl]);
			Ly_m[xm] = 0.125f * (src_h[xl] + src_h[xh] - src_l[xl] - src_l[xh]) + 0.25f * (src_h[xm] - src_l[xm]);
		}
	}
}

/**
@brief  Computes a BOX filter of variable size on an image
@param KERN_SIZE    The kernel size of the filter
@param BORDER_TYPE  The border type if filter exceeds border
@param input        The input image
@param output       The output image
@param rows         The image height
@param cols         The image width
*/
template<const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void swTestBoxFilter(const float * input, float * output, const vx_int32 rows, const vx_int32 cols) {

	// Constants
	const vx_int32 KERN_RAD = KERN_SIZE / 2;
	const float NORM = 1.0f / powf(KERN_SIZE, 2);

	// Compute box filter
	for (vx_int32 y = 0; y < rows; y++) {
		for (vx_int32 x = 0; x < cols; x++) {

			// Data is passed through if filter exceeds border
			if (BORDER_TYPE == VX_BORDER_UNDEFINED) {
				if (x < KERN_RAD || x >= cols - KERN_RAD || y < KERN_RAD || y >= rows - KERN_RAD) {
					output[y*cols + x] = input[y*cols + x];
					continue;
				}
			}

			// Compute single box filter
			float sum = 0.0f;
			for (vx_int32 i = -KERN_RAD; i <= KERN_RAD; i++) {
				for (vx_int32 j = -KERN_RAD; j <= KERN_RAD; j++) {

					// Replicated border
					if (BORDER_TYPE == VX_BORDER_REPLICATE) {
						vx_int32 yi = min(max(y + i, static_cast<vx_int32>(0)), rows - 1);
						vx_int32 xj = min(max(x + j, static_cast<vx_int32>(0)), cols - 1);
						sum += input[yi*cols + xj];

						// Constant border
					}
					else if (BORDER_TYPE == VX_BORDER_CONSTANT) {
						vx_int32 yi = y + i;
						vx_int32 xj = x + j;
						if ((xj >= 0) && (xj < cols) && (yi >= 0) && (yi < rows))
							sum += input[yi*cols + xj];

						// Data is passed through if filter exceeds border
					}
					else if (BORDER_TYPE == VX_BORDER_UNDEFINED) {
						vx_int32 yi = y + i;
						vx_int32 xj = x + j;
						sum += input[yi*cols + xj];
					}
				}
			}

			// Store result
			output[y*cols + x] = sum * NORM;
		}
	}
}

/**
@brief Computes a 3x3 CUSTOM filter on an image
@param KERN_SIZE    The kernel size of the filter
@param BORDER_TYPE  The border type if filter exceeds border
@param input        The input image
@param output       The output image
@param imageKernel  The input kernel filter
@param rows         The image height
@param cols         The image width
*/
template<const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void swTestCustomConvolution(const float *input, float *output, float imageKernel[KERN_SIZE][KERN_SIZE], const vx_int32 rows, const vx_int32 cols) {

	// CConstants
	const vx_int32 KERN_RAD = KERN_SIZE / 2;

	float NORM = 0.0f;
	for (vx_int32 i = 0; i < KERN_SIZE; i++) {
		for (vx_int32 j = 0; j < KERN_SIZE; j++) {
			NORM += imageKernel[i][j];
		}
	}
	NORM = 1.0f / NORM;

	// Compute custom filter
	for (vx_int32 y = 0; y < rows; y++) {
		for (vx_int32 x = 0; x < cols; x++) {

			// Data is passed through if filter exceeds border
			if (BORDER_TYPE == VX_BORDER_UNDEFINED) {
				if (x < KERN_RAD || x >= cols - KERN_RAD || y < KERN_RAD || y >= rows - KERN_RAD) {
					output[y*cols + x] = input[y*cols + x];
					continue;
				}
			}

			// Compute single custom filter
			float sum = 0.0f;
			for (vx_int32 i = -KERN_RAD; i <= KERN_RAD; i++) {
				for (vx_int32 j = -KERN_RAD; j <= KERN_RAD; j++) {

					// Replicated border
					if (BORDER_TYPE == VX_BORDER_REPLICATE) {
						vx_int32 yi = min(max(y + i, static_cast<vx_int32>(0)), rows - 1);
						vx_int32 xj = min(max(x + j, static_cast<vx_int32>(0)), cols - 1);
						sum += (input[yi*cols + xj] * imageKernel[i + KERN_RAD][j + KERN_RAD]);

						// Constant border
					}
					else if (BORDER_TYPE == VX_BORDER_CONSTANT) {
						vx_int32 yi = y + i;
						vx_int32 xj = x + j;
						if ((xj >= 0) && (xj < cols) && (yi >= 0) && (yi < rows))
							sum += (input[yi*cols + xj] * imageKernel[i + KERN_RAD][j + KERN_RAD]);

						// Data is passed through if filter exceeds border
					}
					else if (BORDER_TYPE == VX_BORDER_UNDEFINED) {
						vx_int32 yi = y + i;
						vx_int32 xj = x + j;
						sum += (input[yi*cols + xj] * imageKernel[i + KERN_RAD][j + KERN_RAD]);
					}
				}
			}

			// Store result
			output[y*cols + x] = sum * NORM;
		}
	}
}

/**
@brief  Computes a 3x3 MEDIAN filter on an image
@param input        The input image
@param output       The output image
@param rows         The image height
@param cols         The image width
*/
void swTestMedianFilter3x3(const float *input, float *output, const vx_int32 rows, const vx_int32 cols) {

	// Compute median filter
	for (vx_int32 y = 0; y < rows; y++) {
		for (vx_int32 x = 0; x < cols; x++) {

			// Keep borders unchanged
			if (x == 0 || x == cols - 1 || y == 0 || y == rows - 1) {
				output[y*cols + x] = input[y*cols + x];
				continue;
			}

			// Load input data and store into array for sorting
			float data[9] = {
				input[(y - 1)*cols + (x - 1)],
				input[(y - 1)*cols + (x)],
				input[(y - 1)*cols + (x + 1)],
				input[(y)*cols + (x - 1)],
				input[(y)*cols + (x)],
				input[(y)*cols + (x + 1)],
				input[(y + 1)*cols + (x - 1)],
				input[(y + 1)*cols + (x)],
				input[(y + 1)*cols + (x + 1)]
			};

			// Sort array
			sort(data, data + 9);

			// Store median
			output[y*cols + x] = data[4];
		}
	}
}

#endif /* SRC_IMG_FILTER_TEST_H_ */
