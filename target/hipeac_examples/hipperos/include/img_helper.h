/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track 
* changes/dates in source files. Any modifications to or software 
* including (via compiler) GPL-licensed code must also be made available 
* under the GPL along with build & install instructions. 
*
* @file    img_helper.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These are helper functions needed by different functions (e.g. static assertion). 
* All own Enumerations, Typedefs, Macros and Namespaces are contained in this file. 
*/

#ifndef SRC_IMG_HELPER_H_
#define SRC_IMG_HELPER_H_

//using namespace std;

/*! \brief set this to SDSoC, when using SDSoC */
#define SDSOC

/*********************************************************************************************************************/
/* All includes libraries are here */
/*********************************************************************************************************************/

#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
//#include <iostream>
#include <cmath>
#include <algorithm>
#include "vx_types.h"

#include "os.h"
#define max(_a, _b) (_a >= _b ? _a : _b)

#ifdef SDSOC
//#include "sds_lib.h"
#endif

/*********************************************************************************************************************/
/* Own Macros */
/*********************************************************************************************************************/

// Maximum and Minumum values of datatypes
#define VX_INT8_MIN     static_cast<int8_t>(0-128)  /*!< \brief Minimum of signed 8 bit type */
#define VX_INT16_MIN    static_cast<int16_t>(0-32768)  /*!< \brief Minimum of signed 16 bit type */
#define VX_INT32_MIN    static_cast<int32_t>(0-2147483648)  /*!< \brief Minimum of signed 32 bit type */
#define VX_INT8_MAX     static_cast<int8_t>(127)  /*!< \brief Maximum of signed 8 bit type */
#define VX_INT16_MAX    static_cast<int16_t>(32767)  /*!< \brief Maximum of signed 16 bit type */
#define VX_INT32_MAX    static_cast<int32_t>(2147483647)  /*!< \brief Maximum of signed 32 bit type */
#define VX_UINT8_MAX    static_cast<uint8_t>(0xff)  /*!< \brief Maximum of unsigned 8 bit type */
#define VX_UINT16_MAX   static_cast<uint16_t>(0xffff)  /*!< \brief Maximum of unsigned 16 bit type */
#define VX_UINT32_MAX   static_cast<uint32_t>(0xffffffff)  /*!< \brief Maximum of unsigned 32 bit type */

/*! \brief Gets the datatype of an integer:
* \details The output is of type <tt>\ref vx_type_e</tt> and is 8, 16, 32, 64 bit signed or unsigned
*/
#define GET_TYPE(TYPE) (const vx_type_e)( \
	(numeric_limits<TYPE>::is_integer == true) ? ( \
		(numeric_limits<TYPE>::is_signed == false) ? ( \
			(sizeof(TYPE) == 1) ? ( \
				VX_TYPE_UINT8 \
			) : ( \
				(sizeof(TYPE) == 2) ? ( \
					VX_TYPE_UINT16 \
				) : ( \
					(sizeof(TYPE) == 4) ? ( \
						VX_TYPE_UINT32 \
					) : ( \
						(sizeof(TYPE) == 8) ? ( \
							VX_TYPE_UINT64 \
						) : ( \
							VX_TYPE_INVALID \
						) \
					) \
				) \
			) \
		) :  ( \
			(sizeof(TYPE) == 1) ? ( \
				VX_TYPE_INT8 \
			) : ( \
				(sizeof(TYPE) == 2) ? ( \
					VX_TYPE_INT16 \
				) : ( \
					(sizeof(TYPE) == 4) ? ( \
						VX_TYPE_INT32 \
					) : ( \
						(sizeof(TYPE) == 8) ? ( \
							VX_TYPE_INT64 \
						) : ( \
							VX_TYPE_INVALID \
						) \
					) \
				) \
			) \
		) \
	) : ( \
		VX_TYPE_INVALID \
	) \
)

/*********************************************************************************************************************/
/* Own enums  */
/*********************************************************************************************************************/

namespace SDSOC {

	/*! \brief Filter method with window (TODO add seperative filters)
	\details Recommended/Tested border types for different convolution methods \n
	Filter   | UNCHANGED | CONSTANT | REPLICATED
	---------|-----------|----------|------------
	Gaussian | yes       | yes      | yes
	Scharr   | no        | no       | yes
	Sobel    | no        | no       | yes
	Boxed    | yes       | yes      | yes
	Custom   | yes       | yes      | yes
	Median   | yes       | no       | no
	*/
	enum filter_type {
		GAUSSIAN_FILTER,     /*!< \brief Gaussian filter (optimized for kernel structure) */
		DERIVATIVE_X,        /*!< \brief Scharr/Sobel derivative filter (optimized for kernel structure) */
		DERIVATIVE_Y,        /*!< \brief Scharr/Sobel derivative filter (optimized for kernel structure) */
		CUSTOM_CONVOLUTION,  /*!< \brief Costum convolution filter */
		BOX_FILTER,          /*!< \brief Box filter (optimized for kernel structure) */
		MEDIAN_FILTER        /*!< \brief Median filter (optimized for kernel structure) */
	};

	/*! \brief Performs an bitwise operation on an image
	*/
	enum bitwiseOperation {
		BITWISE_AND,  /*!< \brief Performs a bitwise AND operation between two unsigned images. */
		BITWISE_XOR,  /*!< \brief Performs a bitwise EXCLUSIVE OR (XOR) operation between two unsigned images. */
		BITWISE_OR,   /*!< \brief Performs a bitwise INCLUSIVE OR operation between two unsigned images. */
		BITWISE_NOT   /*!< \brief Performs a bitwise NOT operation on a input images. */
	};

	/*! \brief Performs an arithmetic operation on an image
	*/
	enum arithmeticOperation {
		ABSOLUTE_DIFFERENCE,      /*!< \brief Computes the absolute difference between two images */
		ARITHMETIC_ADDITION,      /*!< \brief Performs addition between two images */
		ARITHMETIC_SUBTRACTION,   /*!< \brief Performs subtraction between two images */
		MAGNITUDE,                /*!< \brief Implements the Gradient Magnitude Computation Kernel */
		MULTIPLY                  /*!< \brief Performs element-wise multiplication between two images and a scalar value. */
	};
}


/*********************************************************************************************************************/
/* Static Assertion  */
/*********************************************************************************************************************/

// note that we wrap the non existing type inside a struct to avoid warning
// messages about unused variables when static assertions are used at function
// scope
// the use of sizeof makes sure the assertion error is not ignored by SFINAE
template <bool>
struct StaticAssertion;
template <>
struct StaticAssertion<true>
{
}; // StaticAssertion<true>
template<int i>
struct StaticAssertionTest
{
}; // StaticAssertionTest<int>

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

   /*! \brief Pre-C++11 Static Assertions Without Boost
   * \details: from  http://stackoverflow.com/questions/1980012/boost-static-assert-without-boost/1980156
   *
   * <code>STATIC_ASSERT(expression, message)</code>
   *
   * When the static assertion test fails, a compiler error message that somehow
   * contains the "STATIC_ASSERTION_FAILED_AT_LINE_xxx_message" is generated.
   *
   * /!\ message has to be a valid C++ identifier, that is to say it must not
   * contain space characters, cannot start with a digit, etc.
   *
   * STATIC_ASSERT(true, this_message_will_never_be_displayed);
   */
#define STATIC_ASSERT(expression, message)\
	struct CONCATENATE(__static_assertion_at_line_, __LINE__)\
	{\
		StaticAssertion<static_cast<bool>((expression))> CONCATENATE(CONCATENATE(CONCATENATE(STATIC_ASSERTION_FAILED_AT_LINE_, __LINE__), _), message);\
	};//\
	//typedef StaticAssertionTest<sizeof(CONCATENATE(__static_assertion_at_line_, __LINE__))> CONCATENATE(__static_assertion_test_at_line_, __LINE__)

/*********************************************************************************************************************/
/* HW: Vector/Scalar Conversion Functions */
/*********************************************************************************************************************/

/** @brief HW: Converts N vector to N scalar arrays
@param VecType     1 element holds 1 vector
@param ScalarType  The data type of the image elements
@param VEC_NUM     The amount of vector to be converted
@param VEC_SIZE    The number of elements in a vector
@param input       An array of vectors
@param output      An array of arrays
*/
template <typename VecType, typename ScalarType, const vx_uint16 VEC_NUM, const vx_uint16 VEC_SIZE>
void convertVectorToScalar(VecType input[VEC_NUM], ScalarType output[VEC_NUM][VEC_SIZE]) {
#pragma HLS INLINE

	for (vx_uint16 i = 0; i < VEC_NUM; i++) {
#pragma HLS unroll
		for (vx_uint16 j = 0; j < VEC_SIZE; j++) {
#pragma HLS unroll
			output[i][j] = static_cast<ScalarType>(input[i] >> (j * sizeof(ScalarType) * 8));
		}
	}
}

/** @brief HW: Converts N scalar arrays to N vector
@param VecType     1 element holds 1 vector
@param ScalarType  The data type of the image elements
@param VEC_NUM     The amount of vector to be converted
@param VEC_SIZE    The number of elements in a vector
@param input       An array of arrays
@param output      An array of vectors
*/
template <typename VecType, typename ScalarType, const vx_uint16 VEC_NUM, const vx_uint16 VEC_SIZE>
void convertScalarToVector(ScalarType input[VEC_NUM][VEC_SIZE], VecType output[VEC_NUM]) {
#pragma HLS INLINE

	for (vx_uint16 i = 0; i < VEC_NUM; i++) {
#pragma HLS unroll
		for (vx_uint16 j = 0; j < VEC_SIZE; j++) {
#pragma HLS unroll
			output[i] += (static_cast<VecType>(input[i][j])) << (j * sizeof(ScalarType) * 8);
		}
	}
}

/*********************************************************************************************************************/
/* HW: Function Parameter Verification Functions */
/*********************************************************************************************************************/

//
/** @brief HW: Check function parameters/types for pixel operation
@param ScalarType  The data type
@param VecType     The vector type
@param IMG_PIXEL   Number of pixel in the image
*/
template<typename ScalarType, typename VecType, const vx_uint32 IMG_PIXEL>
void checkDataTypeOperation(void) {
#pragma HLS INLINE

	const vx_uint32 scalarSize = sizeof(ScalarType);
	const vx_uint32 vectorSize = sizeof(VecType);
	STATIC_ASSERT(scalarSize <= 4, scalar_size_must_be_8_16_32_bit);
	STATIC_ASSERT(vectorSize <= 8, vector_size_must_be_8_16_32_64_bit);
	STATIC_ASSERT((IMG_PIXEL % vectorSize == 0), image_pixels_are_not_multiple_of_vector_size);
	STATIC_ASSERT(vectorSize >= scalarSize, vector_size_must_be_1_2_4_8_times_of_scalar);
}

/*********************************************************************************************************************/
/* SW: Test Helper Functions */
/*********************************************************************************************************************/

/** @brief SW: Computes the maximum absolute error between two images
*/
template <const vx_uint32 IMG_PIXELS>
void checkErrorFloat(float *host, float *device) {

	float absDifMax = 0.0;
	for (vx_uint32 i = 0; i < IMG_PIXELS; i++) {
		float absDif = abs(device[i] - host[i]);
		absDifMax = max(absDifMax, absDif);
	}

	const int absDifMaxInt = (int) absDifMax;
    const int absDifMaxDec = (int) ((absDifMax - ((float) absDifMaxInt)) * 100);
	PRINTF("Max. absolut error: %d.%d", absDifMaxInt, absDifMaxDec);
}

/** @brief SW: Converts a fix point image of format InType to a floating point image
*/
template <typename InType, const vx_uint32 IMG_PIXELS>
void convertIntegerToFloating(InType *input, float *output) {

	for (vx_uint32 i = 0; i < IMG_PIXELS; i++)
		output[i] = static_cast<float>(input[i]) * powf(2.0f, -8.0 * sizeof(InType));
}

/** @brief SW: Converts a floating point image to a fix point image of format OutType
*/
template <typename OutType, const vx_uint32 IMG_PIXELS>
void convertFloatingToInteger(float *input, OutType *output) {

	for (vx_uint32 i = 0; i < IMG_PIXELS; i++)
		output[i] = static_cast<OutType>(input[i] * powf(2.0, 8.0 * sizeof(OutType)));
}

#endif /* SRC_IMG_HELPER_H_ */
