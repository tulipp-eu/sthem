/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track
* changes/dates in source files. Any modifications to or software
* including (via compiler) GPL-licensed code must also be made available
* under the GPL along with build & install instructions.
*
* @file    img_other_core.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These are all core functions needed for the remaining image processing functions (Do not call functions from here)
*/

#ifndef SRC_IMG_OTHER_CORE_H_
#define SRC_IMG_OTHER_CORE_H_

#include "img_helper.h"

using namespace std;

/*********************************************************************************************************************/
/* Bit Depth Conversion Operations */
/*********************************************************************************************************************/

/*! \brief Converts image bit depth. The output image dimensions should be the same as the dimensions of the input image. */
template<typename InScalar, typename OutScalar, const vx_convert_policy_e CONV_POLICY, const vx_uint16 SHIFT>
OutScalar convertDepthSingle(InScalar in) {
#pragma HLS INLINE

	// Result
	OutScalar result = 0;

	// Constants
	const bool IN_IS_SIGNED = numeric_limits<InScalar>::is_signed;
	const bool OUT_IS_SIGNED = numeric_limits<OutScalar>::is_signed;
	const vx_int16 IN_SIZE = sizeof(InScalar);
	const vx_int16 OUT_SIZE = sizeof(OutScalar);

	// Check for max shift value
	const vx_uint16 MAX_SHIFT = (IN_SIZE > OUT_SIZE) ? static_cast<vx_uint16>(IN_SIZE - OUT_SIZE) : static_cast<vx_uint16>(OUT_SIZE - IN_SIZE);
	STATIC_ASSERT(MAX_SHIFT*8 >= SHIFT, shift_size_out_of_range);

	// Check if down or up conversion
	bool DOWN_CONVERSION = false;
	bool UP_CONVERSION = false;

	// Border values for VX_CONVERT_POLICY_SATURATE
	vx_int64 MAX_VAL = 0;
	vx_int64 MIN_VAL = 0;

	// Set if down or up conversion
	if ((IN_SIZE == 1 && OUT_SIZE == 2) || 
		(IN_SIZE == 1 && OUT_SIZE == 4) || 
		(IN_SIZE == 2 && OUT_SIZE == 4)) {
		UP_CONVERSION = true;
	} else if (
		(IN_SIZE == 2 && OUT_SIZE == 1) || 
		(IN_SIZE == 4 && OUT_SIZE == 1) || 
		(IN_SIZE == 4 && OUT_SIZE == 2)) {
		DOWN_CONVERSION = true;
	} 

	// Set border values for VX_CONVERT_POLICY_SATURATE
	if (OUT_IS_SIGNED == true) {
		if (OUT_SIZE == 1) {
			MAX_VAL = VX_INT8_MAX;
			MIN_VAL = VX_INT8_MIN;
		} else if (OUT_SIZE == 2) {
			MAX_VAL = VX_INT16_MAX;
			MIN_VAL = VX_INT16_MIN;
		} else if (OUT_SIZE == 4) {
			MAX_VAL = VX_INT32_MAX;
			MIN_VAL = VX_INT32_MIN;
		}
	} else if (OUT_IS_SIGNED == false) {
		if (OUT_SIZE == 1)
			MAX_VAL = VX_UINT8_MAX;
		else if (OUT_SIZE == 2)
			MAX_VAL = VX_UINT16_MAX;
		else if (OUT_SIZE == 4)
			MAX_VAL = VX_UINT32_MAX;
	}

	// Do up or down conversion
	if (DOWN_CONVERSION == true) {
		if (CONV_POLICY == VX_CONVERT_POLICY_WRAP) {
			result = (static_cast<OutScalar>(in >> SHIFT));
		} else if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE) {
			vx_int64 value = static_cast<vx_int64>(in >> SHIFT);
			value = max(min(value, MAX_VAL), MIN_VAL);
			result = static_cast<OutScalar>(value);
		}
	}
	if (UP_CONVERSION == true) {
		if (CONV_POLICY == VX_CONVERT_POLICY_WRAP) {
			result = (static_cast<OutScalar>(in)) << SHIFT;
		} else if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE) {
			if (OUT_IS_SIGNED == true) {
				vx_int64 value = (static_cast<vx_int64>(in)) << SHIFT;
				value = static_cast<OutScalar>(max(min(value, MAX_VAL), MIN_VAL));
				result = static_cast<OutScalar>(value);
			} else {
				result = (static_cast<OutScalar>(in)) << SHIFT;
			}			
		}
	}

	// Convert from signed to unsigned or v.v.
	if ((IN_SIZE == OUT_SIZE) && (IN_IS_SIGNED != OUT_IS_SIGNED)) {
		if (CONV_POLICY == VX_CONVERT_POLICY_WRAP) {
			result = static_cast<OutScalar>(in);
		} else if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE) {
			vx_int64 value = static_cast<vx_int64>(in);
			value = max(min(value, MAX_VAL), MIN_VAL);
			result = static_cast<OutScalar>(value);
		}
	}	

	// Return result
	return result;
}

/*! \brief Converts image bit depth. The output image dimensions should be the same as the dimensions of the input image. (Top Function) */
template<typename InScalar, typename OutScalar, typename InVector, typename OutVector, const vx_uint32 IMG_PIXEL, const vx_convert_policy_e CONV_POLICY, const vx_uint16 SHIFT>
void convertDepthArray(InVector *in, OutVector *out) {
#pragma HLS INLINE

	// Constants
	const vx_uint16 VEC_SIZE = static_cast<vx_uint16>(sizeof(InVector) / sizeof(InScalar));
	const vx_uint16 VEC_SIZE_B = static_cast<vx_uint16>(sizeof(OutVector) / sizeof(OutScalar));
	const vx_uint32 VEC_PIXEL = IMG_PIXEL / static_cast<vx_uint32>(VEC_SIZE);

	// Check function parameters/types
	checkDataTypeOperation<InScalar, InVector, IMG_PIXEL>();
	checkDataTypeOperation<OutScalar, OutVector, IMG_PIXEL>();
	STATIC_ASSERT(VEC_SIZE == VEC_SIZE_B, vector_size_of_output_and_input_must_be_the_same);

	// Computes operations (pipelined)
	for (vx_uint32 i = 0; i < VEC_PIXEL; i++) {
#pragma HLS PIPELINE II=1

		// Scalar arrays
		InScalar inScalar[VEC_SIZE];
#pragma HLS array_partition variable=inScalar complete  dim=0
		OutScalar outScalar[VEC_SIZE];
#pragma HLS array_partition variable=outScalar complete  dim=0

		// Vector variables
		InVector inVector = in[i];
		OutVector outVector = 0;

		// Converts data from vector to scalar
		convertVectorToScalar<InVector, InScalar, 1, VEC_SIZE>(&inVector, &inScalar);

		// Compute a vector of arithmetic operations in 1 clock cycle
		for (vx_uint16 j = 0; j < VEC_SIZE; j++) {
#pragma HLS unroll
			outScalar[j] = convertDepthSingle<InScalar, OutScalar, CONV_POLICY, SHIFT>(inScalar[j]);
		}

		// Convert data back to vector type
		convertScalarToVector<OutVector, OutScalar, 1, VEC_SIZE>(&outScalar, &outVector);

		// Write result into global memory
		out[i] = outVector;
	}
}

/*********************************************************************************************************************/
/* Color Conversion Operations */
/*********************************************************************************************************************/

/*! \brief Convert from Grayscale to RGB */
template<typename InType, typename OutType, const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS>
void grayscaleToRgb(InType* input, OutType* output) {

	// Global variables
	vx_uint8 data = 0;
	vx_uint8 cases = 0;
	vx_uint32 data_buf[2];

	// Compute gray-scale on image
	for (vx_uint32 i = 0, j = 0; i < IMG_COLS*IMG_ROWS; i++) {
#pragma HLS PIPELINE II=1

		// Internal variables
		vx_uint32 data_cur[4];

		// Get next data beat		
		data = (vx_uint8)(input[i]);

		// Buffer gray values
		switch (cases) {
		case 0:
			data_cur[0] = (vx_uint32)data;
			data_cur[1] = (vx_uint32)data;
			data_cur[2] = (vx_uint32)data;
			cases = 1;
			break;
		case 1:
			data_cur[3] = (vx_uint32)data;
			data_buf[0] = (vx_uint32)data;
			data_buf[1] = (vx_uint32)data;
			cases = 2;
			break;
		case 2:
			data_cur[0] = data_buf[0];
			data_cur[1] = data_buf[1];
			data_cur[2] = (vx_uint32)data;
			data_cur[3] = (vx_uint32)data;
			data_buf[0] = (vx_uint32)data;
			cases = 3;
			break;
		default:
			data_cur[0] = data_buf[0];
			data_cur[1] = (vx_uint32)data;
			data_cur[2] = (vx_uint32)data;
			data_cur[3] = (vx_uint32)data;
			cases = 0;
			break;
		}

		// Convert RGB to gray-scale
		vx_uint32 rgb = data_cur[0] + (data_cur[1] << 8) + (data_cur[2] << 16) + (data_cur[3] << 24);

		// Store output
		if (cases != 1) {
			output[j] = rgb;
			j++;
		}
	}
}

/*! \brief Convert from Grayscale to RGBX */
template<typename InType, typename OutType, const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS>
void grayscaleToRgbx(InType* input, OutType* output) {

	// Convert grayscale to RGBX
	for (vx_uint32 i = 0; i < IMG_COLS*IMG_ROWS; i++) {
#pragma HLS PIPELINE II=1

		// Get next input		
		vx_uint32 data = (vx_uint32)(input[i]);

		// Convert RGB to gray-scale
		vx_uint32 rgbx = data + (data << 8) + (data << 16);

		// Store output
		output[i] = rgbx;
	}
}

/*! \brief Convert from RGBX to Grayscale */
template<typename InType, typename OutType, const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS>
void rgbxToGrayscale(InType* input, OutType* output) {
#pragma HLS INLINE

	// Compute gray-scale on image
	for (vx_uint32 i = 0; i < IMG_COLS*IMG_ROWS; i++) {
#pragma HLS PIPELINE II=1

		// Internal variables
		vx_uint8 red, green, blue;

		// Get next input	
		vx_uint32 data = input[i];

		// Extract values
		red   = (vx_uint8)((data >>  0) & 0xFF); // (data << 24) >> 24;			
		green = (vx_uint8)((data >>  8) & 0xFF); // data(data << 16) >> 24;
		blue  = (vx_uint8)((data >> 16) & 0xFF); // (data << 8) >> 24;

		// Convert RGB to gray-scale
		vx_uint32 gray = 299 * (vx_uint32)red + 587 * (vx_uint32)green + 114 * (vx_uint32)blue;

		// Devide by 1000 and normalized
		vx_uint8 gray_norm = (16777 * (gray + 500)) >> 24;

		// Store output
		output[i] = gray_norm;
	}
}

/*! \brief Convert from RGB to Grayscale */
template<typename InType, typename OutType, const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS>
void rgbToGrayscale(InType* input, OutType* output) {
#pragma HLS INLINE

	// Global variables
	vx_uint32 data = 0;
	vx_uint8 cases = 0;
	vx_uint8 data_buf[3];	

	// Compute gray-scale on image
	for (vx_uint32 i = 0, j = 0; i < IMG_COLS*IMG_ROWS; i++) {
#pragma HLS PIPELINE II=1

		// Internal variables
		vx_uint8 red, green, blue;
		vx_uint8 data_cur[4];
		
		// Get next data beat		
		if (cases < 3) {
			data = input[j];
			j++;
		}

		// Extract values
		data_cur[0] = (vx_uint8)((data >>  0) & 0xFF); // (data << 24) >> 24;			
		data_cur[1] = (vx_uint8)((data >>  8) & 0xFF); // data(data << 16) >> 24;
		data_cur[2] = (vx_uint8)((data >> 16) & 0xFF); // (data << 8) >> 24;
		data_cur[3] = (vx_uint8)((data >> 24) & 0xFF); // (data >> 24);

		// Get rgb values and buffer values
		switch (cases) {
			case 0:
				red   = data_cur[0];
				green = data_cur[1];
				blue  = data_cur[2];
				data_buf[0] = data_cur[3];
				cases = 1;
				break;
			case 1:
				red   = data_buf[0];
				green = data_cur[0];
				blue  = data_cur[1];
				data_buf[0] = data_cur[2];
				data_buf[1] = data_cur[3];
				cases = 2;
				break;
			case 2:
				red   = data_buf[0];
				green = data_buf[1];
				blue  = data_cur[0];
				data_buf[0] = data_cur[1];
				data_buf[1] = data_cur[2];
				data_buf[2] = data_cur[3];
				cases = 3;
				break;
			default:
				red   = data_buf[0];
				green = data_buf[1];
				blue  = data_buf[2];
				cases = 0;
				break;
		}

		// Convert RGB to gray-scale
		vx_uint32 gray = 299 * (vx_uint32)red + 587 * (vx_uint32)green + 114 * (vx_uint32)blue;

		// Devide by 1000 and normalized
		vx_uint8 gray_norm = (16777 * (gray + 500)) >> 24;

		// Store output
		output[i] = gray_norm;
	}
}

/*! \brief Converts the Color of an image between RGB/RGBX/Grayscale */
template <typename InType, typename OutType, const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS, vx_df_image_e INPUT_TYPE, vx_df_image_e OUTPUT_TYPE>
void colorConversion(InType* input, OutType* output) {
#pragma HLS INLINE

	// Get size of datatypes
	const vx_uint32 IN_SIZE = sizeof(InType);
	const vx_uint32 OUT_SIZE = sizeof(OutType);

	// Check if datatype size is allowed
	const bool allowed_datatype_size =
		(IN_SIZE == 4 && OUT_SIZE == 1) ||
		(IN_SIZE == 1 && OUT_SIZE == 4);
	STATIC_ASSERT(allowed_datatype_size, color_conversion_type_not_supported);

	// Check if color conversion is allowed
	const bool allowed_conversion_types =
		(INPUT_TYPE == VX_DF_IMAGE_RGB && OUTPUT_TYPE == VX_DF_IMAGE_U8) ||
		(INPUT_TYPE == VX_DF_IMAGE_RGBX && OUTPUT_TYPE == VX_DF_IMAGE_U8) ||
		(INPUT_TYPE == VX_DF_IMAGE_U8 && OUTPUT_TYPE == VX_DF_IMAGE_RGB) ||
		(INPUT_TYPE == VX_DF_IMAGE_U8 && OUTPUT_TYPE == VX_DF_IMAGE_RGBX);
	STATIC_ASSERT(allowed_conversion_types, color_conversion_type_not_supported);

	// RGBx/RGB to Grayscale
	if (IN_SIZE == 4 && OUT_SIZE == 1) {

		// RGB to Grayscale
		if (INPUT_TYPE == VX_DF_IMAGE_RGB && OUTPUT_TYPE == VX_DF_IMAGE_U8)
			rgbToGrayscale<InType, OutType, IMG_COLS, IMG_ROWS>(input, output);

		// RGBx to Grayscale
		else if (INPUT_TYPE == VX_DF_IMAGE_RGBX && OUTPUT_TYPE == VX_DF_IMAGE_U8)
			rgbxToGrayscale<InType, OutType, IMG_COLS, IMG_ROWS>(input, output);

	// Grayscale to RGBx/RGB
	} else if (IN_SIZE == 1 && OUT_SIZE == 4) {

		// Grayscale to RGB
		if (INPUT_TYPE == VX_DF_IMAGE_U8 && OUTPUT_TYPE == VX_DF_IMAGE_RGB)
			grayscaleToRgb<InType, OutType, IMG_COLS, IMG_ROWS>(input, output);

		// Grayscale to RGBx
		else if (INPUT_TYPE == VX_DF_IMAGE_U8 && OUTPUT_TYPE == VX_DF_IMAGE_RGBX)
			grayscaleToRgbx<InType, OutType, IMG_COLS, IMG_ROWS>(input, output);
	} 
}

/*********************************************************************************************************************/
/* Scale Functions */
/*********************************************************************************************************************/

/*! \brief Resize the image down to nearest neighbor */
template<vx_uint16 COLS_IN, vx_uint16 ROWS_IN, vx_uint16 COLS_OUT, vx_uint16 ROWS_OUT>
void scaleDownNearest(vx_uint8* input, vx_uint8* output) {
#pragma HLS INLINE

	// Compute the scale factor of the resized image in x and y direction
	const vx_uint32 ACCURACY = 14;
	const vx_uint32 COLS_SCALE = (vx_uint32)(((vx_float64)COLS_IN / (vx_float64)COLS_OUT) * pow(2, ACCURACY) + 0.5);
	const vx_uint32 ROWS_SCALE = (vx_uint32)(((vx_float64)ROWS_IN / (vx_float64)ROWS_OUT) * pow(2, ACCURACY) + 0.5);

	// Compute the resized image
	for (vx_uint32 y = 0, y_dst = 0, y_dst_next = 0; y < ROWS_IN; y++) {
		for (vx_uint32 x = 0, x_dst = 0; x < COLS_IN; x++) {
#pragma HLS PIPELINE II=1

			// Get next input pixel in a stream
			vx_uint8 data = input[y*COLS_IN + x];

			// Compute the coordinates of the next needed input pixel
			vx_uint32 x_src = (((x_dst << 1) + 1) * COLS_SCALE) >> (ACCURACY + 1);
			vx_uint32 y_src = (((y_dst << 1) + 1) * ROWS_SCALE) >> (ACCURACY + 1);

			// Check if current input pixel is the needed one
			if ((y_src == y) && (x_src == x)) {

				// Write the ouput pixel to stream
				output[y_dst*COLS_OUT + x_dst] = data;

				// Update pointer
				x_dst++;
				y_dst_next = 1;
			}

			// Update pointer
			if ((y_dst_next == 1) && (x == COLS_IN - 1)) {
				y_dst++;
				y_dst_next = 0;
			}
		}
	}
}

/*! \brief Resize the image down using bilinear interpolation */
template<vx_uint16 COLS_IN, vx_uint16 ROWS_IN, vx_uint16 COLS_OUT, vx_uint16 ROWS_OUT>
void scaleDownBilinear(vx_uint8* input, vx_uint8* output) {
#pragma HLS INLINE

	// Compute constants
	const vx_uint32 ACCURACY = 12; // Minimum 12, Maximum 15
	const vx_uint32 ONE = (1 << ACCURACY);
	const vx_uint32 SHIFT = 2*ACCURACY - 10-8;
	const vx_uint32 MASK_FRAKTION = (1 << (ACCURACY + 2)) - 1;	
	const vx_uint32 COLS_SCALE = (vx_uint32)(((vx_float64)COLS_IN / (vx_float64)COLS_OUT) * pow(2, ACCURACY) + 0.5);
	const vx_uint32 ROWS_SCALE = (vx_uint32)(((vx_float64)ROWS_IN / (vx_float64)ROWS_OUT) * pow(2, ACCURACY) + 0.5);

	// Strat and step size of srrc ptr with "ACCURACY" fraction size 
	// x_src = (x + 0.5) * scale_cols - 0.5 | fraction = 1+1+ACCURACY | not negative: SCALE > 1.0f
	const vx_uint32 X_SRC_STRT = (COLS_SCALE << 1) - 1;
	const vx_uint32 Y_SRC_STRT = (ROWS_SCALE << 1) - 1;
	const vx_uint32 X_SRC_STEP = COLS_SCALE * 4;
	const vx_uint32 Y_SRC_STEP = ROWS_SCALE * 4;

	// Buffer the input pixel
	vx_uint8 data_new = 0;
	vx_uint8 data_left = 0, data_right = 0;
	vx_uint8 buffer_one[COLS_IN];
	vx_uint8 buffer_two[COLS_IN];	

	// Compute the resized image
	for (vx_uint32 y = 0, y_dst = 0, y_src = Y_SRC_STRT, y_dst_next = 0; y <= ROWS_IN; y++) {
		for (vx_uint32 x = 0, x_dst = 0, x_src = X_SRC_STRT; x <= COLS_IN; x++) {
#pragma HLS PIPELINE II=1

			vx_uint8 tl, tr, bl, br;

			// Buffer the previous input pixel [x-1]
			vx_uint8 data_old = data_new;

			// Read Input Pixel from Stream
			if(x < COLS_IN && y < ROWS_IN)
				data_new = input[y*COLS_IN + x];			

			// Compute the coordinates of all needed input pixels			
			vx_uint32 x_fract = (x_src & MASK_FRAKTION) >> 2;
			vx_uint32 y_fract = (y_src & MASK_FRAKTION) >> 2;			
			vx_uint16 x_l = (vx_uint16)(x_src >> (ACCURACY + 2));
			vx_uint16 y_t = (vx_uint16)(y_src >> (ACCURACY + 2));
			vx_uint16 x_r = x_l + 1;
			vx_uint16 y_b = y_t + 1;

			// Write/Read data to/from buffer
			if ((y % 2) == 0) {
				if (x_l < COLS_IN)
					data_left = buffer_two[x_l];
				if (x_r < COLS_IN)
					data_right = buffer_two[x_r];
				if (x > 0)
					buffer_one[x - 1] = data_old;
			} else {
				if (x_l < COLS_IN)
					data_left = buffer_one[x_l];
				if (x_r < COLS_IN)
					data_right = buffer_one[x_r];
				if (x > 0)
					buffer_two[x - 1] = data_old;
			}

			// Get the 4 needed input pixel (using replicated borders)
			if (y_t == COLS_IN - 1) {
				if (x_l == ROWS_IN - 1) {
					tl = tr = bl = br = data_left;
				} else {
					tr = br = data_right;
					tl = bl = data_left;
				}
			} else {
				if (x_l == ROWS_IN - 1) {
					bl = br = data_old;
					tl = tr = data_left;
				} else {
					tl = data_right;
					tr = data_left;
					bl = data_old;
					br = data_new;
				}
			}

			// Compute the Bilinear Interpolation (Saturated)
			vx_uint16 tl_part = (vx_uint16)(((((ONE - x_fract) * (ONE - y_fract)) >> 10) * (vx_uint32)tl) >> 8);
			vx_uint16 tr_part = (vx_uint16)(((((      x_fract) * (ONE - y_fract)) >> 10) * (vx_uint32)tr) >> 8);
			vx_uint16 bl_part = (vx_uint16)(((((ONE - x_fract) * (      y_fract)) >> 10) * (vx_uint32)bl) >> 8);
			vx_uint16 br_part = (vx_uint16)(((((      x_fract) * (      y_fract)) >> 10) * (vx_uint32)br) >> 8);
			vx_uint16 sum = (tl_part + tr_part + bl_part + br_part) >> (2 * ACCURACY - 10 - 8);
			vx_uint8 result = (vx_uint8)(min(sum, (vx_uint16)(255)));

			// Check if the input pixel are the needed ones
			if ((x_r == x) && (y_b == y)) {

				// Write ouput pixel to stream
				output[y_dst*COLS_OUT + x_dst] = result;

				// Update pointer
				x_dst++;
				x_src += X_SRC_STEP;
				y_dst_next = 1;
			}

			// Update pointer
			if ((y_dst_next == 1) && (x_dst == COLS_OUT)) {
				y_dst++;
				y_src += Y_SRC_STEP;
				y_dst_next = 0;				
			}
		}
	}
}

/*! \brief Scale an image down using bilinear or nearest neighbor interpolation */
template<vx_uint16 COLS_IN, vx_uint16 ROWS_IN, vx_uint16 COLS_OUT, vx_uint16 ROWS_OUT, vx_interpolation_type_e SCALE_TYPE>
void scaleDown(vx_uint8* input, vx_uint8* output) {
#pragma HLS INLINE

	// Check if output image resolution is smaller than input image resolution
	STATIC_ASSERT(COLS_IN >= COLS_OUT, only_down_scale_of_image_supported);
	STATIC_ASSERT(ROWS_IN >= ROWS_OUT, only_down_scale_of_image_supported);
	STATIC_ASSERT(SCALE_TYPE == VX_INTERPOLATION_NEAREST_NEIGHBOR || SCALE_TYPE == VX_INTERPOLATION_BILINEAR, scale_type_not_supported);

	// Nearest neighbor interpolation
	if (SCALE_TYPE == VX_INTERPOLATION_NEAREST_NEIGHBOR) {
		scaleDownNearest<COLS_IN, ROWS_IN, COLS_OUT, ROWS_OUT>(input, output);

	// Bilinear Interpolation
	} else if (SCALE_TYPE == VX_INTERPOLATION_BILINEAR) {
		scaleDownBilinear<COLS_IN, ROWS_IN, COLS_OUT, ROWS_OUT>(input, output);

	// Interpolation not supported
	} 
}

/*********************************************************************************************************************/
/* Integral Functions */
/*********************************************************************************************************************/

/*! \brief Computes the integral image of the input. The output image dimensions should be the same as the dimensions of the input image.  */
template<const vx_uint16 IMG_COLS, const vx_uint16 IMG_ROWS>
void integral(vx_uint8* input, vx_uint32* output) {
#pragma HLS INLINE

	// Buffers 1 row of the integral image
	vx_uint32 buffer[IMG_COLS];

	// Compute integral on image
	for (vx_uint32 y = 0; y < IMG_ROWS; y++) {
		for (vx_uint32 x = 0, sum_row = 0; x < IMG_COLS; x++) {
#pragma HLS PIPELINE II=1

			// Read input
			vx_uint8 pixel = input[y*IMG_COLS + x];

			// Compute the sum of the current row
			sum_row += +pixel;

			// Compute the integral pixel
			vx_uint32 sum_area = sum_row;
			if (y > 0)
				sum_area += buffer[x];

			// Buffer the integral pixels of row for next row
			buffer[x] = sum_area;

			// Write output
			output[y*IMG_COLS + x] = sum_area;
		}
	}
}

/*********************************************************************************************************************/
/* Historgam and LUT Operations */
/*********************************************************************************************************************/

/*! \brief Generates a distribution from an image.  */
template<typename IN_TYPE, const vx_uint32 IMG_PIXELS, const vx_uint32 DISTRIBUTION_BINS, const vx_uint32 DISTRIBUTION_RANG, const IN_TYPE DISTRIBUTION_OFFSET>
void histogram(IN_TYPE input[IMG_PIXELS], vx_uint32 output[IMG_PIXELS]) {
#pragma HLS INLINE

	// Check Datatype
	const vx_type_e TYPE = GET_TYPE(IN_TYPE);
	const bool allowed_type = (TYPE == VX_TYPE_UINT8) || (TYPE == VX_TYPE_UINT16);
	STATIC_ASSERT(allowed_type, data_type_is_not_allowed_for_histogram);

	// Compute Scale (BINS / RANGE)
	const vx_uint32 ACCURACY = 12;
	const vx_uint32 SCALE = static_cast<vx_uint32>((static_cast<double>(DISTRIBUTION_BINS) / static_cast<double>(DISTRIBUTION_RANG)) * pow(2, ACCURACY));

	// Check for overflow
	const vx_uint32 MAX_DATA_RANGE = ACCURACY + (DISTRIBUTION_BINS / DISTRIBUTION_RANG) + 1 + sizeof(IN_TYPE);
	STATIC_ASSERT(MAX_DATA_RANGE <= 32, overflow__reduce_bins_or_increase_range);

	//Global variables
	vx_uint32 pre_bin = 0, cur_bin = 0;
	vx_uint32 pre_val = 0, cur_val = 0;

	// Buffer Histogram in BRAM
	vx_uint32 LUTA[DISTRIBUTION_BINS];
	vx_uint32 LUTB[DISTRIBUTION_BINS];

	//filling with zeros
	for (vx_uint32 i = 0; i < DISTRIBUTION_BINS; i++) {
#pragma HLS PIPELINE II=1
		LUTA[i] = 0;
		LUTB[i] = 0;
	}

	// Compute Histogram
	for (vx_uint32 i = 0; i < IMG_PIXELS + 1; i++) {
#pragma HLS PIPELINE II=1

		// Gett the input data
		IN_TYPE pixel = 0;
		if (i < IMG_PIXELS)
			pixel = input[i];

		// Compute the BIN
		vx_uint16 index = (vx_uint16)pixel - (vx_uint16)DISTRIBUTION_OFFSET;
		if (DISTRIBUTION_BINS != DISTRIBUTION_RANG)
			cur_bin = ((vx_uint32)index * SCALE) >> ACCURACY;
		else
			cur_bin = (vx_uint32)index;

		// Read current bin & write previous bin
		if (i % 2 == 0) {
			cur_val = LUTA[cur_bin];
			LUTB[pre_bin] = pre_val;
		} else {
			cur_val = LUTB[cur_bin];
			LUTA[pre_bin] = pre_val;
		}

		// Update to store in next iteration
		cur_val++;
		pre_bin = cur_bin;
		pre_val = cur_val;
	}

	// Write output data
	for (vx_uint32 i = 0; i < DISTRIBUTION_BINS; i++) {
#pragma HLS PIPELINE II = 1
		output[i] = LUTA[i] + LUTB[i];
	}
}

/*! \brief Implements the Table Lookup Image Kernel. The output image dimensions should be the same as the dimensions of the input image. */
template<typename DATA_TYPE, const vx_uint32 IMG_PIXELS, const vx_uint32 LUT_COUNT, const vx_uint32 LUT_OFFSET>
void tableLookup(DATA_TYPE input[IMG_PIXELS], DATA_TYPE lut[LUT_COUNT], DATA_TYPE output[IMG_PIXELS]) {
#pragma HLS INLINE

	// Check Datatype
	const vx_type_e TYPE = GET_TYPE(DATA_TYPE);
	const bool allowed_type = (TYPE == VX_TYPE_UINT8) || (TYPE == VX_TYPE_INT16);
	STATIC_ASSERT(allowed_type, data_type_is_not_allowed_for_histogram);

	// Buffer for lookup table
	DATA_TYPE table[LUT_COUNT];

	// Read lookup table into buffer
	for (vx_uint32 i = 0; i < LUT_COUNT; i++) {
#pragma HLS PIPELINE II=1
		table[i] = lut[i];
	}

	// Perform table lookup
	for (vx_uint32 i = 0; i < IMG_PIXELS; i++) {
#pragma HLS PIPELINE II=1	

		// Read from Input
		DATA_TYPE src = input[i];

		// Add Offset
		vx_int32 index = (vx_int32)src + (vx_int32)LUT_OFFSET;

		// Perform Table Lookup and write to output
		if ((index >= 0) && (index < (vx_int32)LUT_COUNT))
			output[i] = table[index];
	}
}

#endif /* SRC_IMG_OTHER_CORE_H_ */
