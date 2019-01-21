/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track
* changes/dates in source files. Any modifications to or software
* including (via compiler) GPL-licensed code must also be made available
* under the GPL along with build & install instructions.
*
* @file    img_filter_base.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These are all accelerated image filter functions (Call from here)
*/

#ifndef SRC_IMG_FILTER_BASE_H_
#define SRC_IMG_FILTER_BASE_H_

#include "img_filter_core.h"

using namespace std;

/*********************************************************************************************************************/
/* Filter Function Declaration */
/*********************************************************************************************************************/

/** @brief  Computes a Gaussian filter over a window of the input image
@param ScalarType The type of the input image.    unsigned
@param VectorType The type for the vectorization. unsigned
@param WIDTH  Image width
@param HEIGHT Image height
@param BORDER_TYPE  type of border: constant or replicated
@param KERN_SIZE  kernel size
@param SIGMA  standard deviation sigma | value is fix point with 8 bit fraction (e.g. (int16)(1.5*256.0) for 1.5)
@param input  The input image
@param output The output image
*/
template<typename ScalarType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_uint16 KERN_SIZE, const vx_uint16 SIGMA, const vx_border_e BORDER_TYPE>
void imgGaussian(VectorType *input, VectorType *output);

/** @brief  Computes a Scharr filter over a window of the input image
@param InputType  The type of the input image.    unsigned
@param OutputType The type of the input image.    signed
@param VectorType The type for the vectorization. unsigned
@param WIDTH  Image width
@param HEIGHT Image height
@param BORDER_TYPE  type of border: constant or replicated
@param KERN_SIZE  kernel size
@param SIGMA  standard deviation sigma | value fix point with 8 bit fraction
@param input   The input image
@param outputX The output image (x derivative)
@param outputY The output image (y derivative)
*/
template <typename InputType, typename OutputType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_border_e BORDER_TYPE>
void imgScharr3x3(VectorType *input, VectorType *outputX, VectorType *outputY);

/** @brief  Computes a Sobel filter over a window of the input image
@param InputType  The type of the input image.    unsigned
@param OutputType The type of the input image.    signed
@param VectorType The type for the vectorization. unsigned
@param WIDTH  Image width
@param HEIGHT Image height
@param BORDER_TYPE  type of border: constant or replicated
@param KERN_SIZE  kernel size
@param SIGMA  standard deviation sigma | value fix point with 8 bit fraction
@param input  The input image
@param outputX The output image (x derivative)
@param outputY The output image (y derivative)
*/
template <typename InputType, typename OutputType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_border_e BORDER_TYPE>
void imgSobel3x3(VectorType *input, VectorType *outputX, VectorType *outputY);

/** @brief  Computes a Costum filter over a window of the input image
@param ScalarType  The type of the input image.   unsigned/signed
@param VectorType The type for the vectorization. unsigned
@param WIDTH  Image width
@param HEIGHT Image height
@param BORDER_TYPE  type of border: constant or replicated
@param KERN_SIZE  kernel size
@param input  The input image
@param output The output image
@param inputKernel The custom convolution kernel
*/
template<typename ScalarType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void imgCustomConvolution(VectorType *input, VectorType *output, ScalarType inputKernel[KERN_SIZE][KERN_SIZE]);

/** @brief  Computes a Box filter over a window of the input image
@param ScalarType  The type of the input image.   unsigned
@param VectorType The type for the vectorization. unsigned
@param WIDTH  Image width
@param HEIGHT Image height
@param BORDER_TYPE  type of border: constant or replicated
@param KERN_SIZE  kernel size
@param input  The input image
@param output The output image
*/
template<typename ScalarType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void imgBoxFilter(VectorType *input, VectorType *output);

/** @brief  Computes a Median filter over a window of the input image
@param ScalarType  The type of the input image.   unsigned
@param VectorType The type for the vectorization. unsigned
@param WIDTH  Image width
@param HEIGHT Image height
@param BORDER_TYPE  type of border: constant or replicated
@param KERN_SIZE  kernel size
@param input  The input image
@param output The output image
*/
template<typename ScalarType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_border_e BORDER_TYPE>
void imgMedianFilter3x3(VectorType *input, VectorType *output);

/*********************************************************************************************************************/
/* Filter Function Definition */
/*********************************************************************************************************************/

// Computes a Gaussian filter over a window of the input image
template<typename ScalarType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_uint16 KERN_SIZE, const vx_uint16 SIGMA, const vx_border_e BORDER_TYPE>
void imgGaussian(VectorType *input, VectorType *output) {
#pragma HLS INLINE

	// Check function parameters/types
	STATIC_ASSERT(numeric_limits<ScalarType>::is_signed == false, scalar_type_must_be_unsigned);

	// Kernel Parameters
	const vx_uint16 KERN_NUM = 1;
	const double SIGMA_DOUBLE = static_cast<double>(SIGMA) / 256.0;

	// Function Input
	ScalarType gaussianKernel[KERN_NUM][KERN_SIZE][KERN_SIZE];
#pragma HLS array_partition variable=gaussianKernel complete dim=0
	const vx_uint16 kernelFraction[KERN_NUM] = { computeGaussianKernel<ScalarType, KERN_SIZE>(gaussianKernel[0], SIGMA_DOUBLE) };
	const SDSOC::filter_type kernelType[KERN_NUM] = { SDSOC::GAUSSIAN_FILTER };

	// Function Output
	VectorType *outputVector[KERN_NUM] = { output };

	// Compute Gaussian
	filter3d<VectorType, ScalarType, ScalarType, WIDTH, HEIGHT, KERN_SIZE, KERN_NUM, BORDER_TYPE>(input, outputVector, gaussianKernel, kernelFraction, kernelType);
}

// Computes a Scharr filter over a window of the input image
template <typename InputType, typename OutputType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_border_e BORDER_TYPE>
void imgScharr3x3(VectorType *input, VectorType *outputX, VectorType *outputY) {
#pragma HLS INLINE

	// Check function parameters/types
	STATIC_ASSERT(numeric_limits<InputType>::is_signed == false, derivative_input_must_be_unsigned);
	STATIC_ASSERT(numeric_limits<OutputType>::is_signed == true, derivative_output_must_be_signed);

	// Kernel Parameters
	const vx_uint16 KERN_NUM = 2;
	const vx_uint16 KERN_SIZE = 3;

	// Function Input
	OutputType kernelVector[KERN_NUM][KERN_SIZE][KERN_SIZE] = {
		{
			{ 3, 0, -3 },
			{ 10, 0, -10 },
			{ 3, 0, -3 },
		},
		{
			{ 3, 10, 3 },
			{ 0, 0, 0 },
			{ -3, -10, -3 },
		} };
#pragma HLS array_partition variable=kernelVector complete dim=0
	const vx_uint16 kernelFraction[KERN_NUM] = { 5, 5 };
	const SDSOC::filter_type kernelType[KERN_NUM] = { SDSOC::DERIVATIVE_X, SDSOC::DERIVATIVE_Y };

	// Function Output
	VectorType *outputVector[KERN_NUM] = { outputX, outputY };

	// Compute scharr
	filter3d<VectorType, InputType, OutputType, WIDTH, HEIGHT, KERN_SIZE, KERN_NUM, BORDER_TYPE>(input, outputVector, kernelVector, kernelFraction, kernelType);
}

// Computes a Sobel filter over a window of the input image
template <typename InputType, typename OutputType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_border_e BORDER_TYPE>
void imgSobel3x3(VectorType *input, VectorType *outputX, VectorType *outputY) {
#pragma HLS INLINE

	// Check function parameters/types
	STATIC_ASSERT(numeric_limits<InputType>::is_signed == false, input_must_be_unsigned);
	STATIC_ASSERT(numeric_limits<OutputType>::is_signed == true, output_must_be_signed);

	// Kernel Parameters
	const vx_uint16 KERN_NUM = 2;
	const vx_uint16 KERN_SIZE = 3;

	// Function Input
	OutputType kernelVector[KERN_NUM][KERN_SIZE][KERN_SIZE] = {
		{
			{ 1, 0, -1 },
			{ 2, 0, -2 },
			{ 1, 0, -1 },
		},
		{
			{ 1, 2, 1 },
			{ 0, 0, 0 },
			{ -1, -2, -1 },
		} };
#pragma HLS array_partition variable=kernelVector complete dim=0
	const vx_uint16 kernelFraction[KERN_NUM] = { 3, 3 };
	const SDSOC::filter_type kernelType[KERN_NUM] = { SDSOC::DERIVATIVE_X, SDSOC::DERIVATIVE_Y };

	// Function Output
	VectorType *outputVector[KERN_NUM] = { outputX, outputY };

	// Compute scharr
	filter3d<VectorType, InputType, OutputType, WIDTH, HEIGHT, KERN_SIZE, KERN_NUM, BORDER_TYPE>(input, outputVector, kernelVector, kernelFraction, kernelType);
}

// Computes a Box filter over a window of the input image
template<typename ScalarType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void imgCustomConvolution(VectorType *input, VectorType *output, ScalarType inputKernel[KERN_SIZE][KERN_SIZE]) {
#pragma HLS INLINE

	// Kernel Parameters
	const vx_uint16 KERN_NUM = 1;

	// Function Input
	ScalarType inKernel[KERN_NUM][KERN_SIZE][KERN_SIZE];
	for (vx_uint16 i = 0; i < KERN_SIZE; i++) {
#pragma HLS unroll
		for (vx_uint16 j = 0; j < KERN_SIZE; j++) {
#pragma HLS unroll
			inKernel[0][i][j] = inputKernel[i][j];
		}
	}
	const vx_uint16 kernelFraction[KERN_NUM] = { 0 };
	const SDSOC::filter_type kernelType[KERN_NUM] = { SDSOC::CUSTOM_CONVOLUTION };

	// Function Output
	VectorType *outputVector[KERN_NUM] = { output };

	// Compute Box Filter
	filter3d<VectorType, ScalarType, ScalarType, WIDTH, HEIGHT, KERN_SIZE, KERN_NUM, BORDER_TYPE>(input, outputVector, inKernel, kernelFraction, kernelType);
}

// Computes a Box filter over a window of the input image
template<typename ScalarType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_uint16 KERN_SIZE, const vx_border_e BORDER_TYPE>
void imgBoxFilter(VectorType *input, VectorType *output) {
#pragma HLS INLINE

	// Check function parameters/types
	STATIC_ASSERT(numeric_limits<ScalarType>::is_signed == false, scalar_type_must_be_unsigned);

	// Kernel Parameters
	const vx_uint16 KERN_NUM = 1;

	// Function Input
	ScalarType inKernel[KERN_NUM][KERN_SIZE][KERN_SIZE];
	const vx_uint16 kernelFraction[KERN_NUM] = { 0 };
	const SDSOC::filter_type kernelType[KERN_NUM] = { SDSOC::BOX_FILTER };

	// Function Output
	VectorType *outputVector[KERN_NUM] = { output };

	// Compute Box Filter
	filter3d<VectorType, ScalarType, ScalarType, WIDTH, HEIGHT, KERN_SIZE, KERN_NUM, BORDER_TYPE>(input, outputVector, inKernel, kernelFraction, kernelType);
}

// Computes a Median filter over a window of the input image
template<typename ScalarType, typename VectorType, const vx_uint16 WIDTH, const vx_uint16 HEIGHT, const vx_border_e BORDER_TYPE>
void imgMedianFilter3x3(VectorType *input, VectorType *output) {
#pragma HLS INLINE

	// Check function parameters/types
	STATIC_ASSERT(numeric_limits<ScalarType>::is_signed == false, scalar_type_must_be_unsigned);

	// Kernel Parameters
	const vx_uint16 KERN_NUM = 1;
	const vx_uint16 KERN_SIZE = 3;

	// Function Input
	ScalarType inKernel[KERN_NUM][KERN_SIZE][KERN_SIZE];
	const vx_uint16 kernelFraction[KERN_NUM] = { 0 };
	const SDSOC::filter_type kernelType[KERN_NUM] = { SDSOC::MEDIAN_FILTER };

	// Function Output
	VectorType *outputVector[KERN_NUM] = { output };

	// Compute Box Filter
	filter3d<VectorType, ScalarType, ScalarType, WIDTH, HEIGHT, KERN_SIZE, KERN_NUM, BORDER_TYPE>(input, outputVector, inKernel, kernelFraction, kernelType);
}

#endif /* SRC_IMG_FILTER_BASE_H_ */
