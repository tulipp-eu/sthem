/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track
* changes/dates in source files. Any modifications to or software
* including (via compiler) GPL-licensed code must also be made available
* under the GPL along with build & install instructions.
*
* @file    img_pixelop_core.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These are all core functions needed for the pixel operation functions (Do not call functions from here)
*/

#ifndef SRC_IMG_PIXELOP_CORE_H_
#define SRC_IMG_PIXELOP_CORE_H_

#include "img_helper.h"

using namespace std;

/*********************************************************************************************************************/
/* Compute Square Root */
/*********************************************************************************************************************/

/** @brief Computes square root: Rounding to floor or to nearest integer
@details
16 bit input | 8 bit output | 8 stages | rounding to floor \n
Resource Type          | LUT | FF
-----------------------|-----|-----
pipeline 16 iterations | 400 | 138
no pipeline	           | 171 | 84

32 bit input | 16 bit output | 16 stages | rounding to floor \n
Resource Type          | LUT | FF
-----------------------|-----|-----
pipeline 16 iterations | 1268| 755
no pipeline	           | 381 | 204


64 bit input | 32 bit output | 32 stages | rounding to floor \n
Resource Type          | LUT | FF
-----------------------|-----|-----
pipeline 16 iterations | 4996| 3049
no pipeline	           | 807 | 398

@param InType			The data type of the output
@param CompType			The data type of the input
@param ROUND_NEAREST	round to floor == false | round to nearest integer == true
@param MAXIMUM			The maximum falue of the output
@param value			The input value
@return					The square root of the input value
*/
template<typename InType, typename CompType, const InType MAXIMUM, const bool CHECK_MAX, const vx_round_policy_e ROUND_POLICY>
InType sqrtLester(CompType value) {
#pragma HLS INLINE

	// Number of stages (latency)
	const vx_uint8 N = sizeof(InType) * 8;

	// Variables
	InType   A1 = 0;  // A^1 Intermediate result
	CompType A2 = 0;  // A^2 Square of the intermediate result

	// Each stage computes 1 bit of the resulting vector
	for (vx_uint8 n = N - 1; n < N; n--) {
#pragma HLS unroll

		// Add new bit of position n and compute (A1 + B1)^2
		InType   B1 = static_cast<  InType>(1) << (n);
		CompType B2 = static_cast<CompType>(B1) << (n);
		CompType AB = static_cast<CompType>(A1) << (n);
		CompType A2_next = A2 + B2 + (AB << 1); // A*A + B*B + 2*A*B

		// Alternative -> more LUT, less FF
		// CompType A2_next = A2 + (static_cast<CompType>(B1 | (A1 << 1)) << n);

		// Store if tmp does not exceed value
		if (A2_next <= value) {
			A1 |= B1;
			A2 = A2_next;
		}
	}

	// Round to the nearest integer and check for overflow
	if (ROUND_POLICY == VX_ROUND_POLICY_TO_NEAREST_EVEN) {
		if (CHECK_MAX) {
			if (((value - A2) > static_cast<CompType>(A1)) && (A1 != MAXIMUM))
				A1++;
		} else {
			if ((value - A2) > static_cast<CompType>(A1))
				A1++;
		}
	}

	// Return result
	return A1;
}

/*********************************************************************************************************************/
/* Compute Absolute Difference of one pixel */
/*********************************************************************************************************************/

/*! \brief Compute Absolute Difference */
template<typename ScalarType, typename CompType, const vx_convert_policy_e CONV_POLICY, const bool IS_SIGNED, const CompType MAX_VAL>
ScalarType absDiff(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	CompType result = 0;

	// Compute absolute difference
	CompType A = static_cast<CompType>(inScalarA);
	CompType B = static_cast<CompType>(inScalarB);
	CompType C = static_cast<CompType>(abs(A - B));

	// Apply conversion policy for signed input
	if (IS_SIGNED) {
		if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE)
			result = min(C, MAX_VAL);
		else if (CONV_POLICY == VX_CONVERT_POLICY_WRAP)
			result = C;

	// Apply conversion policy for unsigned input
	} else {
		if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE)
			result = C;
		else if (CONV_POLICY == VX_CONVERT_POLICY_WRAP)
			result = C;
	}

	// Return result
	return static_cast<ScalarType>(result);
}

/*! \brief Computes Absolute Difference of one pixel (Top) */
template<typename ScalarType, const bool IS_SIGNED, const vx_uint16 TYPE_SIZE, const vx_convert_policy_e CONV_POLICY>
ScalarType computeAbsDiff(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	ScalarType result = 0;

	// Compute absolute difference for signed input/output
	if (IS_SIGNED) {
		if (TYPE_SIZE == 1)
			result = absDiff<ScalarType, vx_int16, CONV_POLICY, IS_SIGNED, static_cast<vx_int16>(VX_INT8_MAX)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = absDiff<ScalarType, vx_int32, CONV_POLICY, IS_SIGNED, static_cast<vx_int32>(VX_INT16_MAX)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = absDiff<ScalarType, vx_int64, CONV_POLICY, IS_SIGNED, static_cast<vx_int64>(VX_INT32_MAX)>(inScalarA, inScalarB);

	// Compute difference for unsigned input/output
	} else {
		if (TYPE_SIZE == 1)
			result = absDiff<ScalarType, vx_int16, CONV_POLICY, IS_SIGNED, static_cast<vx_int16>(VX_UINT8_MAX)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = absDiff<ScalarType, vx_int32, CONV_POLICY, IS_SIGNED, static_cast<vx_int32>(VX_UINT16_MAX)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = absDiff<ScalarType, vx_int64, CONV_POLICY, IS_SIGNED, static_cast<vx_int64>(VX_UINT32_MAX)>(inScalarA, inScalarB);
	}

	// Return result
	return result;
}

/*********************************************************************************************************************/
/* Compute Addition */
/*********************************************************************************************************************/

/*! \brief Compute Addition of one pixel */
template<typename ScalarType, typename CompType, const vx_convert_policy_e CONV_POLICY, const bool IS_SIGNED, const CompType MAX_VAL, const CompType MIN_VAL>
ScalarType addition(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	CompType result = 0;

	// Compute addition
	CompType A = static_cast<CompType>(inScalarA);
	CompType B = static_cast<CompType>(inScalarB);
	CompType C = A + B;

	// Apply conversion policy for signed input
	if (IS_SIGNED) {

		if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE)
			result = max(min(C, MAX_VAL), MIN_VAL);

		else if (CONV_POLICY == VX_CONVERT_POLICY_WRAP) 
			result = C;

	// Apply conversion policy for unsigned input
	} else {
		if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE) {

			// If overflow bit is set, saturate
			const CompType overflow = (static_cast<CompType>(1)) << (sizeof(ScalarType) * 8);			
			result = ((C & overflow) != 0) ? (MAX_VAL) : (C);

		} else if (CONV_POLICY == VX_CONVERT_POLICY_WRAP) {
			result = C;
		}
	}

	// Return result
	return static_cast<ScalarType>(result);
}

/*! \brief Computes Addition of one pixel (Top) */
template<typename ScalarType, const bool IS_SIGNED, const vx_uint16 TYPE_SIZE, const vx_convert_policy_e CONV_POLICY>
ScalarType computeAddition(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	ScalarType result = 0;

	// Compute addition for signed input/output
	if (IS_SIGNED) {
		if (TYPE_SIZE == 1)
			result = addition<ScalarType, vx_int16, CONV_POLICY, IS_SIGNED, static_cast<vx_int16>(VX_INT8_MAX), static_cast<vx_int16>(VX_INT8_MIN)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = addition<ScalarType, vx_int32, CONV_POLICY, IS_SIGNED, static_cast<vx_int32>(VX_INT16_MAX), static_cast<vx_int32>(VX_INT16_MIN)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = addition<ScalarType, vx_int64, CONV_POLICY, IS_SIGNED, static_cast<vx_int64>(VX_INT32_MAX), static_cast<vx_int64>(VX_INT32_MIN)>(inScalarA, inScalarB);

	// Compute addition for unsigned input/output
	} else {
		if (TYPE_SIZE == 1)
			result = addition<ScalarType, vx_uint16, CONV_POLICY, IS_SIGNED, static_cast<vx_uint16>(VX_UINT8_MAX), static_cast<vx_uint16>(0)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = addition<ScalarType, vx_uint32, CONV_POLICY, IS_SIGNED, static_cast<vx_uint32>(VX_UINT16_MAX), static_cast<vx_uint32>(0)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = addition<ScalarType, vx_uint64, CONV_POLICY, IS_SIGNED, static_cast<vx_uint64>(VX_UINT32_MAX), static_cast<vx_uint64>(0)>(inScalarA, inScalarB);
	}

	// Return result
	return result;
}

/*********************************************************************************************************************/
/* Compute Subtraction */
/*********************************************************************************************************************/

/*! \brief Compute Subtraction of one pixel */
template<typename ScalarType, typename CompType, const vx_convert_policy_e CONV_POLICY, const bool IS_SIGNED, const CompType MAX_VAL, const CompType MIN_VAL>
ScalarType subtraction(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	CompType result = 0;

	// Subtract input data
	CompType A = static_cast<CompType>(inScalarA);
	CompType B = static_cast<CompType>(inScalarB);	
	CompType C = A - B;

	// Apply conversion policy for signed input
	if (IS_SIGNED) {

		if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE) 
			C = max(min(C, MAX_VAL), MIN_VAL);

		else if (CONV_POLICY == VX_CONVERT_POLICY_WRAP)
			result = C;

	// Apply conversion policy for unsigned input
	} else {

		if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE)
			result = (B > A) ? (0) : (C);

		else if (CONV_POLICY == VX_CONVERT_POLICY_WRAP)
			result = C;
	}
	 
	// Return result
	return static_cast<ScalarType>(result);
}

/*! \brief Computes Subtraction of one pixel (Top) */
template<typename ScalarType, const bool IS_SIGNED, const vx_uint16 TYPE_SIZE, const vx_convert_policy_e CONV_POLICY>
ScalarType computeSubtraction(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	ScalarType result = 0;

	// Compute subtraction for signed input/output
	if (IS_SIGNED) {
		if (TYPE_SIZE == 1)
			result = subtraction<ScalarType, vx_int16, CONV_POLICY, IS_SIGNED, static_cast<vx_int16>(VX_INT8_MAX), static_cast<vx_int16>(VX_INT8_MIN)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = subtraction<ScalarType, vx_int32, CONV_POLICY, IS_SIGNED, static_cast<vx_int32>(VX_INT16_MAX), static_cast<vx_int32>(VX_INT16_MIN)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = subtraction<ScalarType, vx_int64, CONV_POLICY, IS_SIGNED, static_cast<vx_int64>(VX_INT32_MAX), static_cast<vx_int64>(VX_INT32_MIN)>(inScalarA, inScalarB);

	// Compute subtraction for unsigned input/output
	} else {
		if (TYPE_SIZE == 1)
			result = subtraction<ScalarType, vx_uint16, CONV_POLICY, IS_SIGNED, static_cast<vx_uint16>(VX_UINT8_MAX), static_cast<vx_uint16>(0)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = subtraction<ScalarType, vx_uint32, CONV_POLICY, IS_SIGNED, static_cast<vx_uint32>(VX_UINT16_MAX), static_cast<vx_uint32>(0)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = subtraction<ScalarType, vx_uint64, CONV_POLICY, IS_SIGNED, static_cast<vx_uint64>(VX_UINT32_MAX), static_cast<vx_uint64>(0)>(inScalarA, inScalarB);
	}

	// Return result
	return result;
}

/*********************************************************************************************************************/
/* Compute Magnitude */
/*********************************************************************************************************************/

/*! \brief Compute Magnitude of one pixel (signed) 
    \details Tested for 8, 16 and 32 bit | exact OpenVX results
*/
template<typename ScalarInt, typename ScalarUint, typename CompInt, typename CompUint, const ScalarUint MAX_VALUE, const vx_round_policy_e ROUND_POLICY>
ScalarInt magnitudeSigned(ScalarInt inScalarA, ScalarInt inScalarB) {
#pragma HLS INLINE

	// Compute magnitude for signed input/output
	CompInt A = static_cast<CompInt>(inScalarA);
	CompInt B = static_cast<CompInt>(inScalarB);
	CompUint C = static_cast<CompUint>(A*A) + static_cast<CompUint>(B*B);
	ScalarUint D = sqrtLester<ScalarUint, CompUint, MAX_VALUE, false, ROUND_POLICY>(C); 
	return static_cast<ScalarInt>(min(D, MAX_VALUE));
}

/*! \brief Compute Magnitude of one pixel (unsigned) 
     \details Tested for 8, 16 and 32 bit | exact OpenVX results if MAX_VALUE and CHECK_MAX are true \n
	          TODO: use overflow comparison if CompUint is not 64 bit
*/
template<typename ScalarUint, typename CompUint, const ScalarUint MAX_VALUE, const vx_round_policy_e ROUND_POLICY>
ScalarUint magnitudeUnsigned(ScalarUint inScalarA, ScalarUint inScalarB) {
#pragma HLS INLINE

	// Compute magnitude for unsigned input/output
	CompUint A = static_cast<CompUint>(inScalarA);
	CompUint B = static_cast<CompUint>(inScalarB);
	CompUint AA = A*A;
	CompUint BB = B*B;
	CompUint C = AA + BB;
	ScalarUint D = sqrtLester<ScalarUint, CompUint, MAX_VALUE, true, ROUND_POLICY>(C);
	return (C >= AA && C >= BB) ? (D) : (MAX_VALUE);
}

/*! \brief Computes Magnitude of one pixel (Top) */
template<typename ScalarType, const bool IS_SIGNED, const vx_uint16 TYPE_SIZE, const vx_round_policy_e ROUND_POLICY>
ScalarType computeMagnitude(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	ScalarType result = 0;

	// Compute magnitude for signed input/output
	if (IS_SIGNED) {
		if (TYPE_SIZE == 1)
			result = magnitudeSigned<ScalarType, vx_uint8, vx_int16, vx_uint16, static_cast<vx_uint8>(VX_INT8_MAX), ROUND_POLICY>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = magnitudeSigned<ScalarType, vx_uint16, vx_int32, vx_uint32, static_cast<vx_uint16>(VX_INT16_MAX), ROUND_POLICY>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = magnitudeSigned<ScalarType, vx_uint32, vx_int64, vx_uint64, static_cast<vx_uint32>(VX_INT32_MAX), ROUND_POLICY>(inScalarA, inScalarB);

	// Compute magnitude for unsigned input/output
	} else {
		if (TYPE_SIZE == 1)
			result = magnitudeUnsigned<ScalarType, vx_uint16, static_cast<ScalarType>(VX_UINT8_MAX), ROUND_POLICY>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = magnitudeUnsigned<ScalarType, vx_uint32, static_cast<ScalarType>(VX_UINT16_MAX), ROUND_POLICY>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = magnitudeUnsigned<ScalarType, vx_uint64, static_cast<ScalarType>(VX_UINT32_MAX), ROUND_POLICY>(inScalarA, inScalarB);
	}

	// Return result
	return result;
}

/*********************************************************************************************************************/
/* Compute Pixel-wise Multiplication */
/*********************************************************************************************************************/

/** @brief Performs element-wise multiplication between two images and a scalar value. (24 bit precision)
@details
16 bit unsigned data type | SATURATE | ROUND EVEN \n
Resource Type  |LUT | FF |DSP
---------------|----|----|----
scale = 10000  |~130|~169| 3
scale = 2^15   |~72 |~76 | 1

16 bit unsigned data type | SATURATE | NO ROUND \n
Resource Type  |LUT | FF |DSP
---------------|----|----|----
scale = 10000  |~74 |~114| 3
scale = 2^15   |~72 |~93 | 1

16 bit unsigned data type | WRAP | ROUND EVEN \n
Resource Type  |LUT | FF |DSP
---------------|----|----|----
scale = 10000  |~109|~165| 3
scale = 2^15   |~76 |~56 | 1

16 bit unsigned data type | WRAP | NO ROUND \n
Resource Type  |LUT | FF |DSP
---------------|----|----|----
scale = 10000  |~56 |~111| 3
scale = 2^15   |~56 |~91 | 1
*/
template<typename ScalarType, typename CompType, typename NormType, const bool IS_SIGNED, const vx_uint16 TYPE_SIZE, const vx_convert_policy_e CONV_POLICY, const vx_round_policy_e ROUND_POLICY, const vx_uint16 SCALE, const CompType MAX_VAL, const CompType MIN_VAL>
ScalarType multiply(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	CompType D = 0, result = 0;

	// Check if scaling is by a power of two
	const bool IS_POWER_TWO = (SCALE & (SCALE - 1)) == 0;

	// Compute the amount of bits data needs to be shiftet, if it is power of two
	vx_uint16 SHIFT = 0;
	for (vx_uint16 i = 0; i < sizeof(vx_uint16) * 8; i++) {
#pragma HLS unroll
		if ((SCALE & static_cast<vx_uint16>(1 << i)) != 0)
			SHIFT = i;
	}

	// Compute the scale, if it is not a power of two
	int e = 0;
	const double normalization = 1.0 / static_cast<double>(max(SCALE, static_cast<vx_uint16>(1)));
	frexp(normalization, &e);	
	vx_uint32 fractionA = 1, fractionB = 1;
	for (vx_int32 i = 0; i < 24; i++) {
#pragma HLS unroll
		fractionA *= 2;
	}
	for (vx_int32 i = 0; i < (-e); i++) {
#pragma HLS unroll
		fractionB *= 2;
	}
	const vx_int16 MULT_SHIFT = static_cast<vx_int16>(24 - e);
	const NormType MULT_INT = static_cast<NormType>(normalization * static_cast<double>(fractionA)* static_cast<double>(fractionB) + 0.5);
	const double MULT_DBL = 1.0 / static_cast<double>(max(SCALE, static_cast<vx_uint16>(1)));


	// Scale should not be negative, scale of zero gives no result
	if (SCALE > 0) {		

		// Compute Multiplicaion
		CompType A = static_cast<CompType>(inScalarA);
		CompType B = static_cast<CompType>(inScalarB);
		CompType C = A*B;
	
		// Multiply and truncate to nearest integer
		if (ROUND_POLICY == VX_ROUND_POLICY_TO_NEAREST_EVEN) {

			// Shift bits, if scale it is a power of two
			if (IS_POWER_TWO) {
				D = (C + (static_cast<CompType>(1) << (SHIFT - 1))) >> SHIFT;

			// Multiply by scale, if it is not a power of two
			} else {

				// Use floating point multiplication if data size is 32 bit
				if (TYPE_SIZE == 4) {
					D = static_cast<CompType>(static_cast<double>(C)* MULT_DBL + 0.5);

				// Use 24 bit precise integer multiplication if data size is 16 or 8 bit
				} else {
					D = static_cast<CompType>((static_cast<NormType>(C)* MULT_INT + (static_cast<NormType>(1) << (MULT_SHIFT - 1))) >> MULT_SHIFT);
				}
			}		

		// Multiply and truncate the least significant values
		} else if (ROUND_POLICY == VX_ROUND_POLICY_TO_ZERO) {

			// Shift bits, if scale it is a power of two
			if (IS_POWER_TWO) {
				D = (C) >> SHIFT;

			// Multiply by scale, if it is not a power of two
			} else {

				// Use floating point multiplication if data size is 32 bit
				if (TYPE_SIZE == 4) {
					D = static_cast<CompType>(static_cast<double>(C)* MULT_DBL);

				// Use 24 bit precise integer multiplication if data size is 16 or 8 bit
				} else {
					D = static_cast<CompType>((static_cast<NormType>(C)* MULT_INT) >> MULT_SHIFT);
				}
			}
		}
	
		// Saturate the result 
		if (CONV_POLICY == VX_CONVERT_POLICY_SATURATE) {		
			if (IS_SIGNED) {
				result = max(min(D, MAX_VAL), MIN_VAL);
			} else {
				result = min(D, MAX_VAL);
			}		

		// Take LSBs
		} else if (CONV_POLICY == VX_CONVERT_POLICY_WRAP) {
			result = D;
		}
	}

	// Return result
	return static_cast<ScalarType>(result);
}

/*! \brief Computes Multiplication of 1 pixel (Top) */
template<typename ScalarType, const bool IS_SIGNED, const vx_uint16 TYPE_SIZE, const vx_convert_policy_e CONV_POLICY, const vx_round_policy_e ROUND_POLICY, const vx_uint16 SCALE>
ScalarType computeMultiply(ScalarType inScalarA, ScalarType inScalarB) {
#pragma HLS INLINE

	// Store result
	ScalarType result = 0;

	// Compute subtraction for signed input/output
	if (IS_SIGNED) {
		if (TYPE_SIZE == 1)
			result = multiply<ScalarType, vx_int16, vx_int64, IS_SIGNED, TYPE_SIZE, CONV_POLICY, ROUND_POLICY, SCALE, static_cast<vx_int16>(VX_INT8_MAX), static_cast<vx_int16>(VX_INT8_MIN)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = multiply<ScalarType, vx_int32, vx_int64, IS_SIGNED, TYPE_SIZE, CONV_POLICY, ROUND_POLICY, SCALE, static_cast<vx_int32>(VX_INT16_MAX), static_cast<vx_int32>(VX_INT16_MIN)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = multiply<ScalarType, vx_int64, vx_int64, IS_SIGNED, TYPE_SIZE, CONV_POLICY, ROUND_POLICY, SCALE, static_cast<vx_int64>(VX_INT32_MAX), static_cast<vx_int64>(VX_INT32_MIN)>(inScalarA, inScalarB);

	// Compute subtraction for unsigned input/output
	} else {
		if (TYPE_SIZE == 1)
			result = multiply<ScalarType, vx_uint16, vx_uint64, IS_SIGNED, TYPE_SIZE, CONV_POLICY, ROUND_POLICY, SCALE, static_cast<vx_uint16>(VX_UINT8_MAX), static_cast<vx_uint16>(0)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 2)
			result = multiply<ScalarType, vx_uint32, vx_uint64, IS_SIGNED, TYPE_SIZE, CONV_POLICY, ROUND_POLICY, SCALE, static_cast<vx_uint32>(VX_UINT16_MAX), static_cast<vx_uint32>(0)>(inScalarA, inScalarB);
		else if (TYPE_SIZE == 4)
			result = multiply<ScalarType, vx_uint64, vx_uint64, IS_SIGNED, TYPE_SIZE, CONV_POLICY, ROUND_POLICY, SCALE, static_cast<vx_uint64>(VX_UINT32_MAX), static_cast<vx_uint64>(0)>(inScalarA, inScalarB);
	}

	// Return result
	return result;
}

/*********************************************************************************************************************/
/* Arithmetic Operations */
/*********************************************************************************************************************/

/** @brief Performs an arithmetic operation between two images (convert policy: SATURATE)
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param CONV_POLICY     The round conversion <tt>\ref vx_convert_policy_e</tt>
@param ROUND_POLICY    The round policy <tt>\ref vx_round_policy_e</tt>
@param SCALE           The scale for multiplication
@param OPERATION_TYPE  Can be ABSOLUTE_DIFFERENCE | ARITHMETIC_ADDITION | ARITHMETIC_SUBTRACTION | MAGNITUDE | MULTIPLY
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL, const vx_convert_policy_e CONV_POLICY, const vx_round_policy_e ROUND_POLICY, const vx_uint16 SCALE, SDSOC::arithmeticOperation OPERATION_TYPE>
void operationArithmetic(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE

	// Constants
	const vx_uint16 TYPE_SIZE = sizeof(ScalarType);
	const vx_uint16 VEC_SIZE = static_cast<vx_uint16>(sizeof(VecType) / TYPE_SIZE);
	const vx_uint32 VEC_PIXEL = IMG_PIXEL / static_cast<vx_uint32>(VEC_SIZE);
	const bool IS_SIGNED = numeric_limits<ScalarType>::is_signed;

	// Check function parameters/types
	checkDataTypeOperation<ScalarType, VecType, IMG_PIXEL>();
	
	// Computes arithmetic operations (pipelined)
	for (vx_uint32 i = 0; i < VEC_PIXEL; i++) {
#pragma HLS PIPELINE II=1

		// Scalar arrays
		ScalarType inScalarA[VEC_SIZE];
#pragma HLS array_partition variable=inScalarA complete  dim=0
		ScalarType inScalarB[VEC_SIZE];
#pragma HLS array_partition variable=inScalarB complete  dim=0
		ScalarType outScalar[VEC_SIZE];
#pragma HLS array_partition variable=outScalar complete  dim=0

		// Vector variables
		VecType inVectorA = in1[i];
		VecType inVectorB = in2[i];
		VecType outVector = 0;

		// Converts data from vector to scalar
		convertVectorToScalar<VecType, ScalarType, 1, VEC_SIZE>(&inVectorA, &inScalarA);
		convertVectorToScalar<VecType, ScalarType, 1, VEC_SIZE>(&inVectorB, &inScalarB);

		// Compute a vector of arithmetic operations in 1 clock cycle
		for (vx_uint16 j = 0; j < VEC_SIZE; j++){
#pragma HLS unroll

			// Result of a single arithmetic operation
			ScalarType result = 0;

			// Computes the absolute difference between two images
			if (OPERATION_TYPE == SDSOC::ABSOLUTE_DIFFERENCE) {	
				result = computeAbsDiff<ScalarType, IS_SIGNED, TYPE_SIZE, VX_CONVERT_POLICY_SATURATE>(inScalarA[j], inScalarB[j]);

			// Performs addition between two images
			} else if (OPERATION_TYPE == SDSOC::ARITHMETIC_ADDITION) {
				result = computeAddition<ScalarType, IS_SIGNED, TYPE_SIZE, CONV_POLICY>(inScalarA[j], inScalarB[j]);

			// Performs subtraction between two images
			} else if (OPERATION_TYPE == SDSOC::ARITHMETIC_SUBTRACTION) {				
				result = computeSubtraction<ScalarType, IS_SIGNED, TYPE_SIZE, CONV_POLICY>(inScalarA[j], inScalarB[j]);

			// Implements the Gradient Magnitude Computation Kernel. 
			} else if (OPERATION_TYPE == SDSOC::MAGNITUDE) {
				result = computeMagnitude<ScalarType, IS_SIGNED, TYPE_SIZE, VX_ROUND_POLICY_TO_NEAREST_EVEN>(inScalarA[j], inScalarB[j]);
			
			// Performs element-wise multiplication between two images and a scalar value.
			} else if (OPERATION_TYPE == SDSOC::MULTIPLY) {
				result = computeMultiply<ScalarType, IS_SIGNED, TYPE_SIZE, CONV_POLICY, ROUND_POLICY, SCALE>(inScalarA[j], inScalarB[j]);
			}
			
			// Store result in array
			outScalar[j] = result;
		}

		// Convert data back to vector type
		convertScalarToVector<VecType, ScalarType, 1, VEC_SIZE>(&outScalar, &outVector);

		// Write result into global memory
		out[i] = outVector;
	}
}

/*********************************************************************************************************************/
/* Bitwise Operations */ 
/*********************************************************************************************************************/

/** @brief Performs a bitwise operation on an image or between two images
@param ScalarType      Data type of the image pixels
@param VecType         Data type of the vector (unsigned | vector_size = sizeof(VecType) / sizeof(ScalarType))
@param IMG_PIXEL       Amount of pixels in the image
@param OPERATION_TYPE  Can be BITWISE_AND | BITWISE_XOR | BITWISE_OR | BITWISE_NOT
@param in1             Input image
@param in2             Input image
@param out             Output image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL, SDSOC::bitwiseOperation OPERATION_TYPE>
void operationBitwise(VecType *in1, VecType *in2, VecType *out) {
#pragma HLS INLINE

	// Constants
	const vx_uint16 VEC_SIZE = static_cast<vx_uint16>(sizeof(VecType) / sizeof(ScalarType));
	const vx_uint32 VEC_PIXEL = IMG_PIXEL / static_cast<vx_uint32>(VEC_SIZE);

	// Check function parameters/types
	checkDataTypeOperation<ScalarType, VecType, IMG_PIXEL>();
	STATIC_ASSERT((numeric_limits<ScalarType>::is_signed == false), data_type_must_be_unsigned);

	// Computes bitwise operations (pipelined)
	for (vx_uint32 i = 0; i < VEC_PIXEL; i++) {
#pragma HLS PIPELINE II=1

		// Scalar arrays
		ScalarType inScalarA[VEC_SIZE];
#pragma HLS array_partition variable=inScalarA complete  dim=0
		ScalarType inScalarB[VEC_SIZE];
#pragma HLS array_partition variable=inScalarB complete  dim=0
		ScalarType outScalar[VEC_SIZE];
#pragma HLS array_partition variable=outScalar complete  dim=0

		// Vector variables
		VecType inVectorA = in1[i];
		VecType inVectorB = 0;
		if (OPERATION_TYPE != SDSOC::BITWISE_NOT)
			inVectorB = in2[i];
		VecType outVector = 0;

		// Converts data from vector to scalar
		convertVectorToScalar<VecType, ScalarType, 1, VEC_SIZE>(&inVectorA, &inScalarA);
		if (OPERATION_TYPE != SDSOC::BITWISE_NOT)
			convertVectorToScalar<VecType, ScalarType, 1, VEC_SIZE>(&inVectorB, &inScalarB);

		// Compute a vector of bitwise operations in 1 clock cycle
		for (vx_uint16 j = 0; j < VEC_SIZE; j++){
#pragma HLS unroll

			// Performs a bitwise AND operation between two unsigned images.
			if (OPERATION_TYPE == SDSOC::BITWISE_AND) {
				outScalar[j] = inScalarA[j] & inScalarB[j];

			// Performs a bitwise EXCLUSIVE OR (XOR) operation between two unsigned images.
			} else if (OPERATION_TYPE == SDSOC::BITWISE_XOR) {
				outScalar[j] = inScalarA[j] ^ inScalarB[j];

			// Performs a bitwise INCLUSIVE OR operation between two unsigned images.
			} else if (OPERATION_TYPE == SDSOC::BITWISE_OR) {
				outScalar[j] = inScalarA[j] | inScalarB[j];

			// Performs a bitwise NOT operation on a input images.
			} else if (OPERATION_TYPE == SDSOC::BITWISE_NOT) {
				outScalar[j] = ~(inScalarA[j]);
			}
		}

		// Convert data back to vector type
		convertScalarToVector<VecType, ScalarType, 1, VEC_SIZE>(&outScalar, &outVector);

		// Write result into global memory
		out[i] = outVector;
	}
}

#endif /* SRC_IMG_PIXELOP_CORE_H_ */
