/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track
* changes/dates in source files. Any modifications to or software
* including (via compiler) GPL-licensed code must also be made available
* under the GPL along with build & install instructions.
*
* @file    img_pixelop_base.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These are all pixel operation functions (Call from here)
*/

#ifndef SRC_IMG_PIXELOP_BASE_H_
#define SRC_IMG_PIXELOP_BASE_H_

#include "img_pixelop_core.h"

using namespace std;

/*********************************************************************************************************************/
/* Arithmetic Operations */
/*********************************************************************************************************************/

/** @brief Computes the absolute difference between two images. The output image dimensions should be the same as the dimensions of the input images. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL>
void imgAbsDiff(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE
	operationArithmetic<ScalarType, VecType, IMG_PIXEL, VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, 0, SDSOC::ABSOLUTE_DIFFERENCE>(in1, in2, out);
}

/** @brief Performs addition between two images. The output image dimensions should be the same as the dimensions of the input images. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param CONV_POLICY     The round conversion <tt>\ref vx_convert_policy_e</tt>
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL, const vx_convert_policy_e CONV_POLICY>
void imgAdd(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE
	operationArithmetic<ScalarType, VecType, IMG_PIXEL, CONV_POLICY, VX_ROUND_POLICY_TO_ZERO, 0, SDSOC::ARITHMETIC_ADDITION>(in1, in2, out);
}

/** @brief Performs subtraction between two images.The output image dimensions should be the same as the dimensions of the input images. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param CONV_POLICY     The round conversion <tt>\ref vx_convert_policy_e</tt>
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL, const vx_convert_policy_e CONV_POLICY>
void imgSubtract(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE
	operationArithmetic<ScalarType, VecType, IMG_PIXEL, CONV_POLICY, VX_ROUND_POLICY_TO_ZERO, 0, SDSOC::ARITHMETIC_SUBTRACTION>(in1, in2, out);
}

/** @brief Implements the Gradient Magnitude Computation Kernel. The output image dimensions should be the same as the dimensions of the input images. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL>
void imgMagnitude(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE
	operationArithmetic<ScalarType, VecType, IMG_PIXEL, VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, 0, SDSOC::MAGNITUDE>(in1, in2, out);
}

/** @brief Performs element-wise multiplication between two images and a scalar value. The output image dimensions should be the same as the dimensions of the input images. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param CONV_POLICY     The round conversion <tt>\ref vx_convert_policy_e</tt>
@param ROUND_POLICY    The round policy <tt>\ref vx_round_policy_e</tt>
@param SCALE           A number multiplied to each product before overflow handling.
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL, const vx_convert_policy_e CONV_POLICY, const vx_round_policy_e ROUND_POLICY, const vx_uint16 SCALE>
void imgMultiply(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE
	operationArithmetic<ScalarType, VecType, IMG_PIXEL, CONV_POLICY, ROUND_POLICY, SCALE, SDSOC::MULTIPLY>(in1, in2, out);
}

/*********************************************************************************************************************/
/* Bitwise Operations */
/*********************************************************************************************************************/

/** @brief Performs a bitwise AND operation between two images.The output image dimensions should be the same as the dimensions of the input images. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL>
void imgAnd(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE
	operationBitwise<ScalarType, VecType, IMG_PIXEL, SDSOC::BITWISE_AND>(in1, in2, out);
}

/** @brief Performs a bitwise EXCLUSIVE OR (XOR) operation between two images. The output image dimensions should be the same as the dimensions of the input images. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL>
void imgXor(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE
	operationBitwise<ScalarType, VecType, IMG_PIXEL, SDSOC::BITWISE_XOR>(in1, in2, out);
}

/** @brief Performs a bitwise INCLUSIVE OR operation between two images. The output image dimensions should be the same as the dimensions of the input images. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL>
void imgOr(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE
	operationBitwise<ScalarType, VecType, IMG_PIXEL, SDSOC::BITWISE_OR>(in1, in2, out);
}

/** @brief Performs a bitwise NOT operation on a input image. The output image dimensions should be the same as the dimensions of the input image. 
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param in1             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL>
void imgNot(VecType *in1, VecType *out) {
#pragma HLS INLINE
	operationBitwise<ScalarType, VecType, IMG_PIXEL, SDSOC::BITWISE_NOT>(in1, NULL, out);
}

#endif /* SRC_IMG_PIXELOP_BASE_H_ */
