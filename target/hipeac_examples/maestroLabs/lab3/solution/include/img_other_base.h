/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track
* changes/dates in source files. Any modifications to or software
* including (via compiler) GPL-licensed code must also be made available
* under the GPL along with build & install instructions.
*
* @file    img_other_base.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These are all remaining image processing functions functions (Call from here)
*/

#ifndef SRC_IMG_OTHER_BASE_H_
#define SRC_IMG_OTHER_BASE_H_

#include "img_other_core.h"

/*********************************************************************************************************************/
/* Other Operations */
/*********************************************************************************************************************/

/** @brief Converts image bit depth. The output image dimensions should be the same as the dimensions of the input image. \n
*   @details (d = down | u = up | s = sign) \n
* type|u8 |s8 |u16|s16|u32|s32
* ----|---|---|---|---|---|---
* u8	| -	| s	| u	| u	| u	| u
* s8	| s	| -	| u	| u	| u	| u
* u16	| d	| d	| -	| s	| u	| u
* s16	| d	| d	| s	| -	| u	| u
* u32	| d	| d	| d	| d	| -	| s
* s32	| d	| d	| d	| d	| s	| -
*
* @param InScalar     The datatype of the input image (8, 16, 32 bit unsigned/signed)
* @param OutScalar    The datatype of the output image (8, 16, 32 bit unsigned/signed)
* @param InVector     The type for the vectorization of the input image (8,16,32,64 bit unsigned/signed)
* @param OutVector    The type for the vectorization of the output image (8,16,32,64 bit unsigned/signed)
* @param IMG_PIXEL    The amount of image pixels
* @param CONV_POLICY  The conversion policy for overflow (wrap or saturated)
* @param SHIFT        Bits are shifted by this amount (e.g. up for u8->s16, down for s16->u8)
*                     maximum shift is abs(inBitDepth - outBitDepth)
* @param input        The input image
* @param output       The output image
*/
template<typename InScalar, typename OutScalar, typename InVector, typename OutVector, const vx_uint32 IMG_PIXEL, const vx_convert_policy_e CONV_POLICY, const vx_uint16 SHIFT>
void imgConvertDepth(InVector *in, OutVector *out) {
#pragma HLS INLINE
	convertDepthArray<InScalar, OutScalar, InVector, OutVector, IMG_PIXEL, CONV_POLICY, SHIFT>(in, out);
}

/** @brief  Converts the Color of an image between RGB/RGBX/Grayscale
*   @details Possible Conversions are\n
* RGB -> Grayscale\n
* RGBX -> Grayscale\n
* Grayscale -> RGB\n
* Grayscale -> RGBX
* @param InType      The datatype of the input image  (uint8, uint32)
* @param OutType     The datatype of the output image (uint8, uint32)
* @param IMG_COLS    The columns of the image
* @param IMG_ROWS    The rows of the image
* @param INPUT_TYPE  The color type of the input image (RGB, RGBX, Grayscale)
* @param OUTPUT_TYPE The color type of the output image (RGB, RGBX, Grayscale)
* @param input       The input image
* @param output      The output image
*/
template <typename InType, typename OutType, const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS, vx_df_image_e INPUT_TYPE, vx_df_image_e OUTPUT_TYPE>
void imgConvertColor(InType* input, OutType* output) {
#pragma HLS INLINE
	colorConversion<InType, OutType, IMG_COLS, IMG_ROWS, INPUT_TYPE, OUTPUT_TYPE>(input, output);
}

/** @brief  Scale an image down using bilinear or nearest neighbor interpolation
*
* @param COLS_IN     The columns of the input image
* @param ROWS_IN     The rows of the input image
* @param COLS_OUT    The columns of the output image
* @param ROWS_OUT    The rows of the output image
* @param SCALE_TYPE  The type of interpolation (bilinear or nearest neigbor possible)
* @param input       The input image
* @param output      The output image
*/
template<vx_uint16 COLS_IN, vx_uint16 ROWS_IN, vx_uint16 COLS_OUT, vx_uint16 ROWS_OUT, vx_interpolation_type_e SCALE_TYPE>
void imgScaleDown(vx_uint8* input, vx_uint8* output) {
#pragma HLS INLINE
	scaleDown<COLS_IN, ROWS_IN, COLS_OUT, ROWS_OUT, SCALE_TYPE>(input, output);
}

/** @brief  Computes the integral image of the input. The output image dimensions should be the same as the dimensions of the input image.
*   @details Each output pixel is the sum of the corresponding input pixel and all other pixels above and to the left of it.
*
* @param COLS_IN     The columns of the image
* @param ROWS_IN     The rows of the image
* @param input       The input image
* @param output      The output image
*/
template<const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS>
void imgIntegral(vx_uint8* input, vx_uint32* output) {
#pragma HLS INLINE
	integral<IMG_COLS, IMG_ROWS>(input, output);
}

/** @brief  Generates a distribution from an image.
*   @details This kernel counts the number of occurrences of each pixel value within the window size of a pre-calculated number of bins.
*
* @param IN_TYPE              The Input Type can be vx_uint8 and vx_uint16
* @param IMG_PIXELS           The amount of pixels for input and output image
* @param DISTRIBUTION_BINS    Indicates the number of bins.
* @param DISTRIBUTION_RANG    Indicates the total number of the consecutive values of the distribution interval.
* @param DISTRIBUTION_OFFSET  Indicates the start of the values to use (inclusive).
* @param input                The input image
* @param output               The output image
*/
template<typename IN_TYPE, const vx_uint32 IMG_PIXELS, const vx_uint32 DISTRIBUTION_BINS, const vx_uint32 DISTRIBUTION_RANG, const IN_TYPE DISTRIBUTION_OFFSET>
void imgHistogram(IN_TYPE *input, vx_uint32 *output) {
#pragma HLS INLINE
	histogram<IN_TYPE, IMG_PIXELS, DISTRIBUTION_BINS, DISTRIBUTION_RANG, DISTRIBUTION_OFFSET>(input, output);
}

/** @brief  Implements the Table Lookup Image Kernel. The output image dimensions should be the same as the dimensions of the input image.
*   @details This kernel uses each pixel in an image to index into a LUT and put the indexed LUT value into the output image. The formats supported are vx_uint8 and vx_int16.
*
* @param DATA_TYPE   The data type can be vx_uint8 and vx_int16
* @param IMG_PIXELS  The amount of pixels for input and output image
* @param LUT_COUNT   Indicates the number of elements in the LUT.
* @param LUT_OFFSET  Indicates the index of the input value = 0.
* @param input       The input image
* @param output      The output image
*/
template<typename DATA_TYPE, const vx_uint32 IMG_PIXELS, const vx_uint32 LUT_COUNT, const vx_uint32 LUT_OFFSET>
void imgTableLookup(DATA_TYPE *input, DATA_TYPE *lut, DATA_TYPE *output) {
#pragma HLS INLINE	
	tableLookup<DATA_TYPE, IMG_PIXELS, LUT_COUNT, LUT_OFFSET>(input, lut, output);
}

#endif /* SRC_IMG_OTHER_BASE_H_ */
