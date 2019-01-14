/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track
* changes/dates in source files. Any modifications to or software
* including (via compiler) GPL-licensed code must also be made available
* under the GPL along with build & install instructions.
*
* @file    img_filter_core.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These are the core functions used for the hardware accelerated image processing functions (Do not call functions from here)
*/

#ifndef SRC_IMG_FILTER_CORE_H_
#define SRC_IMG_FILTER_CORE_H_

#include "img_helper.h"

using namespace std;

#ifndef M_PI
#define M_PI          3.14159265359
#endif
#define INT_MIN               -2147483648

/*********************************************************************************************************************/
/* Linebuffer Functions */
/*********************************************************************************************************************/

/** @brief Reads data from line buffers
@param VecType     1 vector is 1 linebuffer element
@param KERN_SIZE   The size of the filter kernel
@param VEC_COLS    The amount of columns in the vectorized image
@param input       New image data to be stored into the linebuffer
@param linebuffer  Linebuffers to store (KERN_SIZE-1) image rows
@param output      Data at x coordinates in the linebuffers
@param x           The x coordinate in the vectorized image
*/
template <typename VecType, const vx_uint16 KERN_SIZE, const vx_uint16 VEC_COLS>
void readFromLineBuffer(VecType input, VecType linebuffer[KERN_SIZE][VEC_COLS], VecType output[KERN_SIZE], const vx_uint16 x) {
#pragma HLS INLINE

	if (x < VEC_COLS) {
		for (vx_uint16 i = 0; i < KERN_SIZE - 1; i++) {
#pragma HLS unroll
			output[i] = linebuffer[i][x];
		}
		output[KERN_SIZE - 1] = input;
	}
}

/** @brief Writes data to line buffers
@param VecType     1 vector is 1 linebuffer element
@param KERN_SIZE   The size of the filter kernel
@param VEC_COLS    The amount of columns in the vectorized image
@param input       Stores data at the x coordinates of the linebuffers
@param linebuffer  Linebuffers to store (KERN_SIZE-1) image rows
@param x           The x coordinate in the vectorized image
*/
template <typename VecType, const vx_uint16 KERN_SIZE, const vx_uint16 VEC_COLS>
void writeToLineBuffer(VecType input[KERN_SIZE], VecType linebuffer[KERN_SIZE][VEC_COLS], const vx_uint16 x) {
#pragma HLS INLINE

	if (x < VEC_COLS) {
		for (vx_uint16 i = 0; i < KERN_SIZE - 1; i++) {
#pragma HLS unroll
			linebuffer[i][x] = input[i + 1];
		}
	}
}

/*********************************************************************************************************************/
/* Sliding Window Functions for Different Border Types */
/*********************************************************************************************************************/

/** @brief Replicates the y borders if needed for the sliding window
@param VecType     1 vector is 1 linebuffer element
@param IMG_ROWS    Amount of rows in the image
@param KERN_RAD    Radius of the filter kernel
@param KERN_SIZE   Total amount of rows in the sliding window
@param input       Output of the linebuffers
@param output      Input with replicated borders if needed
@param y           y coordinate of the image
*/
template <typename VecType, const vx_uint16 IMG_ROWS, const vx_uint16 KERN_RAD, const vx_uint16 KERN_SIZE>
void slidingWindowReplicatedY(VecType input[KERN_SIZE], VecType output[KERN_SIZE], const vx_uint16 y) {
#pragma HLS INLINE

	// Get upper pixels and check y border
	if (KERN_RAD > 0) {
		output[KERN_RAD - 1] = (y > KERN_RAD) ? (input[KERN_RAD - 1]) : (input[KERN_RAD]);
		for (vx_int32 i = KERN_RAD - 2; i >= 0; i--) {
#pragma HLS unroll
			output[i] = (y > static_cast<vx_uint16>(KERN_SIZE - 2 - i)) ? (input[i]) : (output[i + 1]);
		}
	}

	// Pass through observed pixel in the image
	output[KERN_RAD] = input[KERN_RAD];

	// Get lower pixels and check y border
	if (KERN_RAD > 0) {
		output[KERN_RAD + 1] = (y < IMG_ROWS + KERN_RAD - 1) ? (input[KERN_RAD + 1]) : (input[KERN_RAD]);
		for (vx_uint16 i = KERN_RAD + 2; i < KERN_SIZE; i++) {
#pragma HLS unroll
			output[i] = (y < static_cast<vx_uint16>(IMG_ROWS + KERN_SIZE - 1 - i)) ? (input[i]) : (output[i - 1]);
		}
	}
}

/** @brief Moves sliding window and a replicated border in x direction
@param ScalarType  The data type of the image elements
@param KERN_SIZE   Total amount of rows in the sliding window
@param VEC_COLS    The amount of columns in the vectorized image
@param VEC_SIZE    The number of elements in a vector
@param WIN_BORD_A  Internal vertical border for the sliding window
@param WIN_BORD_B  Internal vertical border for the sliding window
@param WIN_COLS    The number of columns in a row
@param input       An array of input data for each row
@param window      The sliding window
@param x           x coordinate of the vectorized image
*/
template <typename ScalarType, const vx_uint16 KERN_SIZE, const vx_uint16 VEC_COLS, const vx_uint16 VEC_SIZE, const vx_uint16 WIN_BORD_A, const vx_uint16 WIN_BORD_B, const vx_uint16 WIN_COLS>
void slidingWindowReplicatedX(ScalarType input[KERN_SIZE][VEC_SIZE], ScalarType window[KERN_SIZE][WIN_COLS], const vx_uint16 x) {
#pragma HLS INLINE

	// Move sliding window and check x border
	for (vx_uint16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll

		// Move sliding window
		for (vx_uint16 j = 0; j < WIN_BORD_A; j++) {
#pragma HLS unroll
			window[i][j] = window[i][j + VEC_SIZE];
		}

		// Move pixel in sliding window and get/check left x border
		for (vx_uint16 j = WIN_BORD_A; j < WIN_BORD_B; j++) {
#pragma HLS unroll
			if (x == 0)
				window[i][j] = input[i][0];
			else
				window[i][j] = window[i][j + VEC_SIZE];
		}

		// Get new pixel array and check right x border
		for (vx_uint16 j = WIN_BORD_B; j < WIN_COLS; j++) {
#pragma HLS unroll
			if (x >= VEC_COLS)
				window[i][j] = input[i][VEC_SIZE - 1];
			else
				window[i][j] = input[i][j - WIN_BORD_B];
		}
	}
}

/** @brief Sets a constant border of 0 for the sliding window
@param VecType     1 vector is 1 linebuffer element
@param IMG_ROWS    Amount of rows in the image
@param KERN_RAD    Radius of the filter kernel
@param KERN_SIZE   Total amount of rows in the sliding window
@param input       Output of the linebuffers
@param output      Input with replicated borders if needed
@param y           y coordinate of the image
*/
template <typename VecType, const vx_uint16 IMG_ROWS, const vx_uint16 KERN_RAD, const vx_uint16 KERN_SIZE>
void slidingWindowConstantY(VecType input[KERN_SIZE], VecType bufferVector[KERN_SIZE], const vx_uint16 y) {
#pragma HLS INLINE

	// Get upper pixels and check y border
	for (vx_uint16 i = 0; i < KERN_RAD; i++) {
#pragma HLS unroll
		bufferVector[i] = (y > KERN_SIZE - 2 - i) ? (input[i]) : (0);
	}

	// Pass through observed pixel in the image
	bufferVector[KERN_RAD] = input[KERN_RAD];

	// Get lower pixels and check y border
	for (vx_uint16 i = KERN_RAD + 1; i < KERN_SIZE; i++) {
#pragma HLS unroll
		bufferVector[i] = (y < IMG_ROWS + KERN_SIZE - 1 - i) ? (input[i]) : (0);
	}
}

/** @brief Moves sliding window and a constant border in x direction
@param ScalarType  The data type of the image elements
@param KERN_SIZE   Total amount of rows in the sliding window
@param VEC_COLS    The amount of columns in the vectorized image
@param VEC_SIZE    The number of elements in a vector
@param WIN_BORD_A  Internal vertical border for the sliding window
@param WIN_BORD_B  Internal vertical border for the sliding window
@param WIN_COLS    The number of columns in a row
@param input       An array of input data for each row
@param window      The sliding window
@param x           x coordinate of the vectorized image
*/
template <typename ScalarType, const vx_uint16 KERN_SIZE, const vx_uint16 VEC_COLS, const vx_uint16 VEC_SIZE, const vx_uint16 WIN_BORD_A, const vx_uint16 WIN_BORD_B, const vx_uint16 WIN_COLS>
void slidingWindowConstantX(ScalarType input[KERN_SIZE][VEC_SIZE], ScalarType window[KERN_SIZE][WIN_COLS], const vx_uint16 x) {
#pragma HLS INLINE

	for (vx_uint16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll

		// Move sliding window
		for (vx_uint16 j = 0; j < WIN_BORD_A; j++) {
#pragma HLS unroll
			window[i][j] = window[i][j + VEC_SIZE];
		}

		// Move pixel in sliding window and get/check left x border
		for (vx_uint16 j = WIN_BORD_A; j < WIN_BORD_B; j++) {
#pragma HLS unroll
			if (x == 0)
				window[i][j] = 0;
			else
				window[i][j] = window[i][j + VEC_SIZE];
		}

		// Get new pixel vector and check right x border
		for (vx_uint16 j = WIN_BORD_B; j < WIN_COLS; j++) {
#pragma HLS unroll
			if (x >= VEC_COLS)
				window[i][j] = 0;
			else
				window[i][j] = input[i][j - WIN_BORD_B];
		}
	}
}

/** @brief Moves sliding window without considering borders
@param ScalarType  The data type of the image elements
@param KERN_SIZE   Total amount of rows in the sliding window
@param VEC_SIZE    The number of elements in a vector
@param WIN_BORD_B  Internal vertical border for the sliding window
@param WIN_COLS    The number of columns in a row
@param input       An array of input data for each row
@param window      The sliding window
*/
template <typename ScalarType, const vx_uint16 KERN_SIZE, const vx_uint16 VEC_SIZE, const vx_uint16 WIN_BORD_B, const vx_uint16 WIN_COLS>
void slidingWindowUnchanged(ScalarType input[KERN_SIZE][VEC_SIZE], ScalarType window[KERN_SIZE][WIN_COLS]) {
#pragma HLS INLINE

	for (vx_uint16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll

		// Move sliding window
		for (vx_uint16 j = 0; j < WIN_BORD_B; j++) {
#pragma HLS unroll
			window[i][j] = window[i][j + VEC_SIZE];
		}

		// Get new pixel vector
		for (vx_uint16 j = WIN_BORD_B; j < WIN_COLS; j++) {
#pragma HLS unroll
			window[i][j] = input[i][j - WIN_BORD_B];
		}
	}
}

/*********************************************************************************************************************/
/* Main sliding Window Function */
/*********************************************************************************************************************/

/** @brief Sliding window main function / considers different border types
@param VecType     1 vector is processed in each clock cycle
@param ScalarType  1 vector contains N scalar elements
@param IMG_ROWS    Amount of rows in the image
@param KERN_RAD    Radius of the filter kernel
@param VEC_COLS    The amount of columns in the vectorized image
@param VEC_SIZE    The number of elements in a vector
@param WIN_COLS    The number of columns in a row
@param KERN_SIZE   Total amount of rows in the sliding window
@param BORDER_TYPE The border type that is considered in the sliding window
*/
template <typename VecType, typename ScalarType, const vx_uint16 IMG_ROWS, const vx_uint16 KERN_RAD, const vx_uint16 VEC_COLS, const vx_uint16 VEC_SIZE, const vx_uint16 WIN_COLS, const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void slidingWindow(VecType input[KERN_SIZE], ScalarType window[KERN_SIZE][WIN_COLS], const vx_uint16 x, const vx_uint16 y) {
#pragma HLS INLINE

	// Constants
	const vx_uint16 WIN_BORD_A = WIN_COLS - VEC_SIZE - KERN_RAD;
	const vx_uint16 WIN_BORD_B = WIN_COLS - VEC_SIZE;

	// Input data rows in vector representation (after considering y border)
	VecType bufferVector[KERN_SIZE];
#pragma HLS array_partition variable=bufferVector complete  dim=0

	// Input data rows in scalar representation (after considering y border)
	ScalarType bufferScalar[KERN_SIZE][VEC_SIZE];
#pragma HLS array_partition variable=bufferScalar complete  dim=0

	// REPLICATED: replicates the border values when exceeding borders
	if (BORDER_TYPE == VX_BORDER_REPLICATE) {

		// Replicated y borders
		slidingWindowReplicatedY<VecType, IMG_ROWS, KERN_RAD, KERN_SIZE>(input, bufferVector, y);

		// Convert from vector to scalar type
		convertVectorToScalar<VecType, ScalarType, KERN_SIZE, VEC_SIZE>(bufferVector, bufferScalar);

		// Sets sliding window and replicated x borders
		slidingWindowReplicatedX<ScalarType, KERN_SIZE, VEC_COLS, VEC_SIZE, WIN_BORD_A, WIN_BORD_B, WIN_COLS>(bufferScalar, window, x);

		// CONSTANT: creates a constant border of zeros around the image
	}
	else if (BORDER_TYPE == VX_BORDER_CONSTANT) {

		// Constant y borders
		slidingWindowConstantY<VecType, IMG_ROWS, KERN_RAD, KERN_SIZE>(input, bufferVector, y);

		// Convert from vector to scalar type
		convertVectorToScalar<VecType, ScalarType, KERN_SIZE, VEC_SIZE>(bufferVector, bufferScalar);

		// Sets sliding window and constant x borders
		slidingWindowConstantX<ScalarType, KERN_SIZE, VEC_COLS, VEC_SIZE, WIN_BORD_A, WIN_BORD_B, WIN_COLS>(bufferScalar, window, x);

		// UNCHANGED: filters exceeding the borders are invalid
	}
	else if (BORDER_TYPE == VX_BORDER_UNDEFINED) {

		// Convert from vector to scalar type
		convertVectorToScalar<VecType, ScalarType, KERN_SIZE, VEC_SIZE>(input, bufferScalar);

		// Sets sliding window and does not create borders
		slidingWindowUnchanged<ScalarType, KERN_SIZE, VEC_SIZE, WIN_BORD_B, WIN_COLS>(bufferScalar, window);
	}
}

/*********************************************************************************************************************/
/* Filter Sub-Functions */
/*********************************************************************************************************************/

/** @brief Normalizes the result of a filter with the kernel sum
@param CompType  Data type for internal computation (uint64 or int64)
@param OutType   Data type of the output
@param input     The result after applying the filter
@param kernSum   The sum of all kernel elements
@return          The normalized result
*/
template <typename CompType, typename OutType>
CompType normalizeFilter(CompType input, double kernSum) {
#pragma HLS INLINE

	// Pre-compile normalization
	int e = 0;
	const double normalization = 1.0 / kernSum;
	frexp(normalization, &e);
	const vx_int32 shift = 8 * sizeof(OutType) - e;
	const vx_uint16 fractionA = static_cast<vx_uint16>(min(max(shift, (vx_int32)0), (vx_int32)31));
	vx_uint32 fractionB = 1;
	for (vx_uint16 i = 0; i < fractionA; i++) {
#pragma HLS unroll
		fractionB *= 2;
	}
	const CompType mult = static_cast<CompType>(normalization * static_cast<double>(fractionB));

	// Normalize sum
	return ((input * mult) >> static_cast<CompType>(fractionA));
}

/** @brief Compares and swaps 2 elements
@param InType  Data type of the image pixels
@param A       1. element to be compared
@param B       2. element to be compared
@param H       smaller element
@param L       bigger element
*/
template <typename InType>
void compareAndSwap(InType A, InType B, InType &L, InType &H) {
#pragma HLS INLINE

	// Compare and Swap
	if (A > B) {
		L = B;
		H = A;
	}
	else {
		L = A;
		H = B;
	}
}

/*********************************************************************************************************************/
/* Filter Functions */
/*********************************************************************************************************************/

/** @brief Computes a gaussian filter (optimized to the kernel symmetry)
@param InType        The input scalar data type
@param OutType       The output scalar data type
@param KERN_SIZE     The size of the kernel
@param inKernel      The gaussian kernel
@param window        The sliding window of this scalar computation
@param kernFraction  The kernel fix point size, to shift the result after multiplication
@return              The result of the gaussian convolution
*/
template <typename InType, typename OutType, const vx_uint16 KERN_SIZE>
OutType computeGaussian(OutType inKernel[KERN_SIZE][KERN_SIZE], InType window[KERN_SIZE][KERN_SIZE], const vx_uint16 kernFraction) {
#pragma HLS INLINE

	// Constants
	const vx_uint16 KERN_RAD = KERN_SIZE / 2;
	const vx_uint16 KERN_RNG = KERN_SIZE - 1;

	// Store result of the gaussian filter (scalar)
	vx_uint64 result = 0;

	// if: (y == KERN_RAD) && (x == KERN_RAD)
	{
		vx_uint64 sum = static_cast<vx_uint64>(window[KERN_RAD][KERN_RAD]);
		result = (static_cast<vx_uint64>(inKernel[KERN_RAD][KERN_RAD]) * sum);
	}

	// if: (y == KERN_RAD) && (x < KERN_RAD)
	for (vx_uint16 x = 0; x < KERN_RAD; x++) {
#pragma HLS unroll
		const vx_uint16 RADIUS = KERN_RAD - x;
		vx_uint64 sum = 0;
		sum += static_cast<vx_uint64>(window[KERN_RAD][x]);
		sum += static_cast<vx_uint64>(window[KERN_RAD][KERN_RNG - x]);
		sum += static_cast<vx_uint64>(window[KERN_RAD + RADIUS][x + RADIUS]);
		sum += static_cast<vx_uint64>(window[KERN_RAD - RADIUS][x + RADIUS]);
		result += (static_cast<vx_uint64>(inKernel[KERN_RAD][x]) * sum);
	}

	// if: (y == x) && (x < KERN_RAD)
	for (vx_uint16 y = 0; y < KERN_RAD; y++) {
#pragma HLS unroll
		vx_uint64 sum = 0;
		sum += static_cast<vx_uint64>(window[y][y]);
		sum += static_cast<vx_uint64>(window[y][KERN_RNG - y]);
		sum += static_cast<vx_uint64>(window[KERN_RNG - y][y]);
		sum += static_cast<vx_uint64>(window[KERN_RNG - y][KERN_RNG - y]);
		result += (static_cast<vx_uint64>(inKernel[y][y]) * sum);
	}

	// if: (y > 0) && (y < KERN_RAD) && (y > x)
	for (vx_uint16 x = 0; x <= KERN_RAD; x++) {
#pragma HLS unroll
		for (vx_uint16 y = x + 1; y < KERN_RAD; y++) {
#pragma HLS unroll
			vx_int64 sum = 0;
			sum += static_cast<vx_uint64>(window[y][x]);
			sum += static_cast<vx_uint64>(window[x][y]);
			sum += static_cast<vx_uint64>(window[KERN_RNG - y][x]);
			sum += static_cast<vx_uint64>(window[x][KERN_RNG - y]);
			sum += static_cast<vx_uint64>(window[KERN_RNG - x][y]);
			sum += static_cast<vx_uint64>(window[y][KERN_RNG - x]);
			sum += static_cast<vx_uint64>(window[KERN_RNG - x][KERN_RNG - y]);
			sum += static_cast<vx_uint64>(window[KERN_RNG - y][KERN_RNG - x]);
			result += (static_cast<vx_uint64>(inKernel[y][x]) * sum);
		}
	}

	// Shift back to fraction of OutType and return result
	result = result >> kernFraction;
	return static_cast<OutType>(result);
}


/** @brief Computes the x derivative (optimized for kernel symmetry)
@param InType        The input scalar data type
@param OutType       The output scalar data type
@param KERN_SIZE     The size of the kernel
@param inKernel      The sobel/scharr kernel
@param window        The sliding window of this scalar computation
@param kernFraction  The kernel fix point size, to shift the result after multiplication
@return              The result of the x derivative computation
*/
template <typename InType, typename OutType, const vx_uint16 KERN_SIZE>
OutType computeDerivativeX(OutType inKernel[KERN_SIZE][KERN_SIZE], InType window[KERN_SIZE][KERN_SIZE], const vx_uint16 kernFraction) {
#pragma HLS INLINE

	vx_int64 a = static_cast<vx_int64>(window[0][2]) + static_cast<vx_int64>(window[2][2]) -
		static_cast<vx_int64>(window[0][0]) - static_cast<vx_int64>(window[2][0]);
	vx_int64 b = static_cast<vx_int64>(window[1][2]) - static_cast<vx_int64>(window[1][0]);
	vx_int64 c = static_cast<vx_int64>(inKernel[0][0])*a + static_cast<vx_int64>(inKernel[1][0])*b;
	return static_cast<OutType>(c >> kernFraction);
}

/** @brief Computes the y derivative (optimized for kernel symmetry)
@param InType        The input scalar data type
@param OutType       The output scalar data type
@param KERN_SIZE     The size of the kernel
@param inKernel      The sobel/scharr kernel
@param window        The sliding window of this scalar computation
@param kernFraction  The kernel fix point size, to shift the result after multiplication
@return              The result of the y derivative computation
*/
template <typename InType, typename OutType, const vx_uint16 KERN_SIZE>
OutType computeDerivativeY(OutType inKernel[KERN_SIZE][KERN_SIZE], InType window[KERN_SIZE][KERN_SIZE], const vx_uint16 kernFraction) {
#pragma HLS INLINE

	vx_int64 a = static_cast<vx_int64>(window[2][0]) + static_cast<vx_int64>(window[2][2]) -
		static_cast<vx_int64>(window[0][0]) - static_cast<vx_int64>(window[0][2]);
	vx_int64 b = static_cast<vx_int64>(window[2][1]) - static_cast<vx_int64>(window[0][1]);
	vx_int64 c = static_cast<vx_int64>(inKernel[0][0])*a + static_cast<vx_int64>(inKernel[0][1])*b;
	return static_cast<OutType>(c >> kernFraction);
}

/** @brief Computes a costum convolution
@param InType        The input scalar data type
@param CompType      Data type for internal computation (uint64 or int64)
@param OutType       The output scalar data type
@param KERN_SIZE     The size of the kernel
@param inKernel      The costum kernel
@param window        The sliding window of this scalar computation
@return              The result of the costum convolution
*/
template <typename InType, typename CompType, typename OutType, const vx_uint16 KERN_SIZE>
OutType computeCostumConvolution(OutType inKernel[KERN_SIZE][KERN_SIZE], InType window[KERN_SIZE][KERN_SIZE]) {
#pragma HLS INLINE

	// Variables
	CompType sum = 0, kernSum = 0;

	// Compute the costum filter
	for (vx_uint16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll
		for (vx_uint16 j = 0; j < KERN_SIZE; j++) {
#pragma HLS unroll
			CompType kernelData = static_cast<CompType>(inKernel[i][j]);
			CompType windowData = static_cast<CompType>(window[i][j]);
			sum += (kernelData * windowData);
		}
	}

	// Sum the kernel values for normalization (pre-compile)
	for (vx_int16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll
		for (vx_uint16 j = 0; j < KERN_SIZE; j++) {
#pragma HLS unroll
			kernSum += static_cast<CompType>(inKernel[i][j]);
		}
	}

	// Normalize kernel sum
	const CompType result = normalizeFilter<CompType, OutType>(sum, static_cast<double>(kernSum));

	// Return result
	return static_cast<OutType>(result);
}

/** @brief Computes a box filter
@param InType        The input scalar data type
@param OutType       The output scalar data type
@param KERN_SIZE     The size of the kernel
@param window        The sliding window of this scalar computation
@return              The result of the box filter
*/
template <typename InType, typename OutType, const vx_uint16 KERN_SIZE>
OutType computeBoxFilter(InType window[KERN_SIZE][KERN_SIZE]) {
#pragma HLS INLINE

	// Variable
	vx_uint64 sum = 0;

	// Sum all input data
	for (vx_uint16 y = 0; y < KERN_SIZE; y++) {
#pragma HLS unroll
		for (vx_uint16 x = 0; x < KERN_SIZE; x++) {
#pragma HLS unroll
			sum += static_cast<vx_uint64>(window[y][x]);
		}
	}

	// Normalize kernel sum
	const vx_uint64 result = normalizeFilter<vx_uint64, OutType>(sum, static_cast<double>(KERN_SIZE*KERN_SIZE));

	// Return result
	return static_cast<OutType>(result);
}

/** @brief Computes a median filter
@param InType        The input scalar data type
@param OutType       The output scalar data type
@param KERN_SIZE     The size of the kernel
@param window        The sliding window of this scalar computation
@return              The result of the median filter
*/
template <typename InType, typename OutType, const vx_uint16 KERN_SIZE>
OutType computeMedianFilter3x3(InType window[KERN_SIZE][KERN_SIZE]) {
#pragma HLS INLINE

	// Variables
	InType B0, B1, B3, B4, B6, B7;
	InType C1, C2, C4, C5, C7, C8;
	InType D0, D1, D3, D4, D6, D7;
	InType E0, E1, E3, E4, E7, E8;
	InType F1, F2, F4, F5, F6, F7;
	InType G3, G4, H3, H4, I4, I5, J3, J4;

	// Input pixel
	InType A0 = window[0][0], A1 = window[0][1], A2 = window[0][2];
	InType A3 = window[1][0], A4 = window[1][1], A5 = window[1][2];
	InType A6 = window[2][0], A7 = window[2][1], A8 = window[2][2];

	// Get median with 19 compare and swap units
	compareAndSwap(A0, A1, B0, B1);
	compareAndSwap(A3, A4, B3, B4);
	compareAndSwap(A6, A7, B6, B7);
	compareAndSwap(B1, A2, C1, C2);
	compareAndSwap(B4, A5, C4, C5);
	compareAndSwap(B7, A8, C7, C8);
	compareAndSwap(B0, C1, D0, D1);
	compareAndSwap(B3, C4, D3, D4);
	compareAndSwap(B6, C7, D6, D7);
	compareAndSwap(D0, D3, E0, E1);
	compareAndSwap(D1, D4, E3, E4);
	compareAndSwap(C5, C8, E7, E8);
	compareAndSwap(E1, D6, F1, F2);
	compareAndSwap(E4, D7, F4, F5);
	compareAndSwap(C2, E7, F6, F7);
	compareAndSwap(E3, F4, G3, G4);
	compareAndSwap(F2, G4, H3, H4);
	compareAndSwap(H4, F6, I4, I5);
	compareAndSwap(H3, I4, J3, J4);

	// Return result
	return static_cast<OutType>(J4);
}

/*********************************************************************************************************************/
/* Filter Main Function */
/*********************************************************************************************************************/

/** @brief Selects and computes a filter
@param VecType      Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param InType       Data type of the input image
@param OutType      Data type of the output image
@param KERN_SIZE    Filter kernel size
@param KERN_NUM     Amount of filter kernel
@param WIN_COLS     Columns of the sliding window
@param VEC_SIZE     Vector size
@param IMG_COLS     Image width
@param IMG_ROWS     Image hight
@param BORDER_TYPE  Border type for pixel access beyond borders
@param window       The sliding window
@param inKernel     The input kernels
@param kernFraction The size of the fraction part of the input kernel
@param method       The filter method to be applied on the image
@param outputVector The results (vector) (per clock cycle)
@param x            X coordinate
@param y            Y coordinate
*/
template <typename VecType, typename InType, typename OutType, const vx_uint16 KERN_SIZE, const vx_uint16 KERN_NUM, const vx_uint16 WIN_COLS, const vx_uint16 VEC_SIZE, const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS, const vx_border_e BORDER_TYPE>
void computeFilter(InType window[KERN_SIZE][WIN_COLS], OutType inKernel[KERN_NUM][KERN_SIZE][KERN_SIZE], const vx_uint16 kernFraction[KERN_NUM], const SDSOC::filter_type method[KERN_NUM], VecType outputVector[KERN_NUM], const vx_uint16 x, const vx_uint16 y) {
#pragma HLS INLINE

	// Constants
	const vx_uint16 KERN_RAD = KERN_SIZE / 2;
	const vx_uint16 OHD_COLS = (WIN_COLS - KERN_RAD) / VEC_SIZE - 1;

	// Output array of the different filter (scalar)
	OutType outputArray[KERN_NUM][VEC_SIZE];
#pragma HLS array_partition variable=outputArray complete dim=0

	// Compute KERN_NUM different filter with VEC_SIZE elements per clock cycle
	for (vx_uint16 kernId = 0; kernId < KERN_NUM; kernId++) {
#pragma HLS unroll
		for (vx_uint16 vecId = 0; vecId < VEC_SIZE; vecId++) {
#pragma HLS unroll

			// Window for single vector element
			InType kernelWindow[KERN_SIZE][KERN_SIZE];
#pragma HLS array_partition variable=kernelWindow complete dim=0

			// Get window for single vector element
			for (vx_uint16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll
				for (vx_uint16 j = 0; j < KERN_SIZE; j++) {
#pragma HLS unroll
					kernelWindow[i][j] = window[i][j + vecId];
				}
			}

			// Check, if input or output is signed
			bool isSigned = (numeric_limits<InType>::is_signed) || (numeric_limits<OutType>::is_signed);

			// Compute filter for single vector element
			OutType result = 0;
			switch (method[kernId]) {
			case SDSOC::GAUSSIAN_FILTER:
				result = computeGaussian<InType, OutType, KERN_SIZE>(inKernel[kernId], kernelWindow, kernFraction[kernId]);
				break;
			case SDSOC::DERIVATIVE_X:
				result = computeDerivativeX<InType, OutType, KERN_SIZE>(inKernel[kernId], kernelWindow, kernFraction[kernId]);
				break;
			case SDSOC::DERIVATIVE_Y:
				result = computeDerivativeY<InType, OutType, KERN_SIZE>(inKernel[kernId], kernelWindow, kernFraction[kernId]);
				break;
			case SDSOC::CUSTOM_CONVOLUTION:
				if (isSigned)
					result = computeCostumConvolution<InType, vx_int64, OutType, KERN_SIZE>(inKernel[kernId], kernelWindow);
				else
					result = computeCostumConvolution<InType, vx_uint64, OutType, KERN_SIZE>(inKernel[kernId], kernelWindow);
				break;
			case SDSOC::BOX_FILTER:
				result = computeBoxFilter<InType, OutType, KERN_SIZE>(kernelWindow);
				break;
			case SDSOC::MEDIAN_FILTER:
				result = computeMedianFilter3x3<InType, OutType, KERN_SIZE>(kernelWindow);
				break;
			default:
				break;
			}

			// Pass through input data, if filter exceeds border and border type is unchanged
			if (BORDER_TYPE == VX_BORDER_UNDEFINED) {
				vx_uint16 xi = x*VEC_SIZE + vecId;
				if ((xi < OHD_COLS*VEC_SIZE + KERN_RAD) || (xi >= IMG_COLS + OHD_COLS*VEC_SIZE - KERN_RAD) || (y < 2 * KERN_RAD) || (y >= IMG_ROWS))
					outputArray[kernId][vecId] = kernelWindow[KERN_RAD][KERN_RAD];
				else
					outputArray[kernId][vecId] = result;
			}
			else {
				outputArray[kernId][vecId] = result;
			}
		}
	}

	// Convert result from scalar to vector type
	convertScalarToVector<VecType, OutType, KERN_NUM, VEC_SIZE>(outputArray, outputVector);
}

/*********************************************************************************************************************/
/* Helper Function to Create a Gaussian Kernel (COMPILE TIME) */
/*********************************************************************************************************************/

/** @brief Computes a gaussian kernel at compile time
@param ScalarType  Data type of the image pixels
@param KERN_SIZE   Gaussian kernel size
@param outKernel   The resulting output kernel
@param sigma       standard deviation of the gaussian kernel
@return            The fix point size of the kernel (for shifting after multiplication)
*/
template <typename ScalarType, const vx_uint16 KERN_SIZE>
const vx_uint16 computeGaussianKernel(ScalarType outKernel[KERN_SIZE][KERN_SIZE], const double sigma) {
#pragma HLS INLINE

	// Constants
	const vx_uint16 K_RAD = KERN_SIZE / 2;
	const double s = 2.0 * sigma * sigma;

	// Variables
	double sum = 0.0;
	double doubleKernel[KERN_SIZE][KERN_SIZE];
#pragma HLS array_partition variable=doubleKernel complete dim=0

	// Creates the gaussian kernel
	for (vx_uint16 x = 0; x < KERN_SIZE; x++) {
#pragma HLS unroll
		for (vx_uint16 y = 0; y < KERN_SIZE; y++) {
#pragma HLS unroll
			const vx_uint32 a = static_cast<vx_uint32>(x - K_RAD);
			const vx_uint32 b = static_cast<vx_uint32>(y - K_RAD);
			const double c = static_cast<double>(a*a + b*b);
			const double r = sqrt(c);
			doubleKernel[x][y] = (exp(-(r*r) / s)) / (M_PI * s);
			sum += doubleKernel[x][y];
		}
	}

	// Normalizes the gaussian kernel
	for (vx_uint16 x = 0; x < KERN_SIZE; ++x) {
#pragma HLS unroll
		for (vx_uint16 y = 0; y < KERN_SIZE; ++y) {
#pragma HLS unroll
			doubleKernel[x][y] /= sum;
		}
	}

	// Computes the fraction for the fixed point representation
	vx_int32 e_max = INT_MIN;
	for (vx_uint16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll
		for (vx_uint16 j = 0; j < KERN_SIZE; j++) {
#pragma HLS unroll
			int e = 0;
			frexp(doubleKernel[i][j], &e);
			e_max = max(e_max, (vx_int32)e);
		}
	}
	const vx_int32 fraction = 8 * sizeof(ScalarType) - e_max;
	const vx_uint16 kernFraction = static_cast<vx_uint16>(min(max(fraction, (vx_int32)0), (vx_int32)31));

	// Computes and stores the fixed point kernel
	vx_uint32 shift = 1;
	for (vx_uint16 i = 0; i < kernFraction; i++) {
#pragma HLS unroll
		shift *= 2;
	}
	for (vx_uint16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll
		for (vx_uint16 j = 0; j < KERN_SIZE; j++) {
#pragma HLS unroll
			outKernel[i][j] = static_cast<ScalarType>(doubleKernel[i][j] * static_cast<double>(shift));
		}
	}

	// Returns the fraction
	return kernFraction;
}

/*********************************************************************************************************************/
/* Main Function */
/*********************************************************************************************************************/

/** @brief This function can compute several 2d filters on 1 image
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param InType          Data type of the input image
@param OutType         Data type of the output image
@param IMG_COLS        Image width
@param IMG_ROWS        Image hight
@param KERN_SIZE       Filter kernel size
@param KERN_NUM        Amount of filter kernel
@param BORDER_TYPE     Border type for pixel access beyond borders
@param input           Input image
@param output          Output images - same amount as KERN_NUM
@param inKernel        Output kernels - same amount as KERN_NUM
@param kernFraction    Fix point fraction part in bits
@param method          The filter method to be applied on the image
*/
template <typename VecType, typename InType, typename OutType, const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS, const vx_uint16 KERN_SIZE, const vx_uint16 KERN_NUM, const vx_border_e BORDER_TYPE>
void filter3d(VecType *input, VecType *output[KERN_NUM], OutType inKernel[KERN_NUM][KERN_SIZE][KERN_SIZE], const vx_uint16 kernFraction[KERN_NUM], const SDSOC::filter_type method[KERN_NUM]) {
#pragma HLS INLINE

	// Constants
	const vx_uint16 VEC_SIZE = sizeof(VecType) / sizeof(InType);
	const vx_uint16 VEC_COLS = IMG_COLS / VEC_SIZE;
	const vx_uint16 KERN_RAD = KERN_SIZE / 2;
	const vx_uint16 WIN_COLS = 2 * KERN_RAD + VEC_SIZE + (VEC_SIZE - (KERN_RAD % VEC_SIZE)) % VEC_SIZE;
	const vx_uint16 OHD_COLS = (WIN_COLS - KERN_RAD) / VEC_SIZE - 1;

	// Check function parameters/types
	const vx_uint16 outSize = sizeof(OutType);
	const vx_uint16 vecSize = sizeof(VecType);
	STATIC_ASSERT((KERN_SIZE != 1), kernel_size_1_makes_no_sense);
	STATIC_ASSERT((KERN_SIZE % 2 == 1), kernel_size_must_be_odd_number);
	STATIC_ASSERT((IMG_COLS % vecSize == 0), image_colums_are_not_multiple_of_vector_size);
	STATIC_ASSERT((outSize == sizeof(InType)), size_of_in_and_out_type_must_be_equal);
	STATIC_ASSERT((outSize == 1) || (outSize == 2) || (outSize == 4), scalar_size_must_be_8_16_32_bit);
	STATIC_ASSERT((vecSize == 1) || (vecSize == 2) || (vecSize == 4) || (vecSize == 8), vector_size_must_be_8_16_32_64_bit);
	STATIC_ASSERT((vecSize == 1 * outSize) || (vecSize == 2 * outSize) || (vecSize == 4 * outSize) || (vecSize == 8 * outSize), vector_size_must_be_1_2_4_8_times_of_scalar);
	STATIC_ASSERT((numeric_limits<VecType>::is_signed == false), vector_type_must_be_unsigned);

	// Linebuffer TODO partitioning
	VecType linebuffer[KERN_SIZE - 1][VEC_COLS];

	// Input data for sliding window
	VecType buffer[KERN_SIZE];
#pragma HLS array_partition variable=buffer complete dim=0

	// Sliding window for complete vector
	InType window[KERN_SIZE][WIN_COLS];
#pragma HLS array_partition variable=window complete dim=0

	// Compute the filter (pipelined)
	vx_uint32 ptrIn = 0, ptrOut = 0;
	for (vx_uint16 y = 0; y < IMG_ROWS + KERN_RAD; y++) {
		for (vx_uint16 x = 0; x < VEC_COLS + OHD_COLS; x++) {
#pragma HLS PIPELINE II=1

			// Input & Output
			VecType inputData = 0;
			VecType outputData[KERN_NUM] = { 0 };
#pragma HLS array_partition variable=outputData complete dim=0

			// Read input data from global memory
			if ((y < IMG_ROWS) && (x < VEC_COLS)) {
				inputData = input[ptrIn];
				ptrIn++;
			}

			// Read data from line_buffer to buffer
			readFromLineBuffer<VecType, KERN_SIZE, VEC_COLS>(inputData, linebuffer, buffer, x);

			// Write data from buffer to line_buffer
			writeToLineBuffer<VecType, KERN_SIZE, VEC_COLS>(buffer, linebuffer, x);

			// Move sliding window with replicated/constant border
			slidingWindow<VecType, InType, IMG_ROWS, KERN_RAD, VEC_COLS, VEC_SIZE, WIN_COLS, KERN_SIZE, BORDER_TYPE>(buffer, window, x, y);

			// Compute filter
			computeFilter<VecType, InType, OutType, KERN_SIZE, KERN_NUM, WIN_COLS, VEC_SIZE, IMG_COLS, IMG_ROWS, BORDER_TYPE>(window, inKernel, kernFraction, method, outputData, x, y);

			// Write output data to global memory
			if ((y >= KERN_RAD) && (x >= OHD_COLS)) {
				for (vx_uint16 kernId = 0; kernId < KERN_NUM; kernId++) {
#pragma HLS unroll
					output[kernId][ptrOut] = outputData[kernId];
				}
				ptrOut++;
			}
		}
	}
}

#endif /* SRC_IMG_FILTER_CORE_H_ */
