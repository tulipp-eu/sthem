#include "filters.h"

void ReplicateChannel(vx_uint8 *input, vx_uint32* output) {
#pragma HLS inline
	for(int i=0; i<PIXELS_FHD; i++)
	{
#pragma HLS PIPELINE II=1
		output[i] = (vx_uint32) input[i] * 0x00010101;;
	}
}


void convert_1channel32BitsTo4Channels8Bits(vx_uint32* input, vx_uint8* output1, vx_uint8* output2, vx_uint8* output3, vx_uint8* output4)
{
	for(uint32_t i=0; i<PIXELS_FHD; i++)
	{
#pragma HLS PIPELINE II=1

		uint32_t data = (uint32_t)input[i];

		output1[i] = static_cast<uint8_t>(0xFF & (data >> 0));
		output2[i] = static_cast<uint8_t>(0xFF & (data >> 8));
		output3[i] = static_cast<uint8_t>(0xFF & (data >> 16));
		output4[i] = static_cast<uint8_t>(0xFF & (data >> 24));
	}
}


void convert_4Channels8BitsTochannel32Bits(vx_uint8* input1, vx_uint8* input2, vx_uint8* input3, vx_uint8* input4, vx_uint32* output)
{
	for(uint32_t i=0; i<PIXELS_FHD; i++)
	{
#pragma HLS PIPELINE II=1

		output[i] = (((uint32_t)input1[i]) << 0) | (((uint32_t)input2[i]) << 8) | (((uint32_t)input3[i]) << 16) | (((uint32_t)input4[i]) << 24);
	}
}


template<typename InType, typename OutType, const vx_uint32 IMG_PIXEL>
void imageSignedToUnsigned(InType *input, OutType *output) {

    // Constants
    const vx_uint32 inSize = sizeof(InType);
    const vx_uint32 outSize = sizeof(OutType);
    const bool IN_SIGNED = numeric_limits<InType>::is_signed;
    const bool OUT_SIGNED = numeric_limits<OutType>::is_signed;

    // Check function parameters/types
    STATIC_ASSERT(inSize == outSize, input_and_output_data_type_size_must_be_the_same);
    STATIC_ASSERT(inSize <= 4, data_type_size_must_be_8_16_32_bit);
    STATIC_ASSERT(IN_SIGNED == true, input_must_be_signed);
    STATIC_ASSERT(OUT_SIGNED == false, output_must_be_unsigned);

    // Computes conversion (pipelined)
    for (vx_uint32 i = 0; i < IMG_PIXEL; i++) {
#pragma HLS PIPELINE II=1

        // Input
        vx_int64 data = static_cast<vx_int64>(input[i]);

        // Addition
        //vx_int64 result = data + static_cast<vx_int64>(offset);
        vx_int64 result = MIN(255, abs(data)<<1);

        // Output
        output[i] = static_cast<OutType>(result);
    }
}

// *********** Filters for DPR *********** //

// Passthrough.
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwPassthrough(vx_uint32 *input, vx_uint32* output) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS DATAFLOW
	for(int i=0; i<PIXELS_FHD; i++)
	{
#pragma HLS PIPELINE II=1
		output[i] = input[i];
	}
}

// Sepia.
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwSepia(vx_uint32 *input, vx_uint32* output) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS DATAFLOW
	for(int i=0; i<PIXELS_FHD; i++)
	{
#pragma HLS PIPELINE II=1
		uint8_t in_channelR, in_channelG, in_channelB, in_channelX;
		in_channelR = (0xFF & (input[i] >> 24));
		in_channelG = (0xFF & (input[i] >> 16));
		in_channelB = (0xFF & (input[i] >> 8));
		in_channelX = (0xFF & (input[i] >> 0));

		uint32_t tmp_sepiaR;
		uint8_t out_sepiaR;
		tmp_sepiaR = (((uint32_t)in_channelR*402) + ((uint32_t)in_channelG*787) + ((uint32_t)in_channelB*194)) >> 10;
		if(tmp_sepiaR > 255)
			out_sepiaR = 255;
		else
			out_sepiaR = tmp_sepiaR;

		uint32_t tmp_sepiaG;
		uint8_t out_sepiaG;
		tmp_sepiaG = (((uint32_t)in_channelR*357) + ((uint32_t)in_channelG*702) + ((uint32_t)in_channelB*172)) >> 10;
		if(tmp_sepiaG > 255)
			out_sepiaG = 255;
		else
			out_sepiaG = tmp_sepiaG;

		uint32_t tmp_sepiaB;
		uint8_t out_sepiaB;
		tmp_sepiaB = (((uint32_t)in_channelR*279) + ((uint32_t)in_channelG*547) + ((uint32_t)in_channelB*134)) >> 10;
		if(tmp_sepiaB > 255)
			out_sepiaB = 255;
		else
			out_sepiaB = tmp_sepiaB;

		output[i] = (((uint32_t)out_sepiaR) << 24) | (((uint32_t)out_sepiaG) << 16) | (((uint32_t)out_sepiaB) << 8) | (((uint32_t)in_channelX) << 0);

	}
}

// Converts RGBx (32-bits) to grayscale (32-bits) for displaying porpuses only.
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwColorConversionRgbxToGray(vx_uint32 *input, vx_uint32* output) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS inline

	// FIFOs to stream data between functions
	vx_uint8 output_fhd_8[PIXELS_FHD];
#pragma HLS STREAM variable = output_fhd_8 depth = 512

// Pragma to stream between loops/functions
#pragma HLS DATAFLOW

	imgConvertColor<vx_uint32, vx_uint8, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_RGBX, VX_DF_IMAGE_U8>(input, output_fhd_8);

	ReplicateChannel(output_fhd_8, output);
}

// Sobel Filter RGBx (32-bits) in X direction.
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwSobelX(vx_uint32 *input, vx_uint32* output) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS inline

	// FIFOs to stream data between functions
	vx_uint8 output_fhd_8[PIXELS_FHD];
#pragma HLS STREAM variable = output_fhd_8 depth = 512

	static vx_int8 SignedSobelChannel1[PIXELS_FHD];
#pragma HLS STREAM variable = SignedSobelChannel1 depth = 512

	static vx_uint8 UnsignedSobelChannel1[PIXELS_FHD];
#pragma HLS STREAM variable = UnsignedSobelChannel1 depth = 512

	vx_int8 tmp[PIXELS_FHD];

// Pragma to stream between loops/functions
#pragma HLS DATAFLOW

	imgConvertColor<vx_uint32, vx_uint8, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_RGBX, VX_DF_IMAGE_U8>(input, output_fhd_8);
	imgSobel3x3<uint8_t, int8_t, uint8_t, COLS_FHD, ROWS_FHD, SOBEL_BORDER>((uint8_t*) output_fhd_8, (uint8_t*) SignedSobelChannel1, (uint8_t*) tmp);
	imageSignedToUnsigned<int8_t, uint8_t, PIXELS_FHD>(SignedSobelChannel1, UnsignedSobelChannel1);
	ReplicateChannel(UnsignedSobelChannel1, output);
}

// Sobel Filter RGBx (32-bits) in Y direction.
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwSobelY(vx_uint32 *input, vx_uint32* output) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS inline

	// FIFOs to stream data between functions
	vx_uint8 output_fhd_8[PIXELS_FHD];
#pragma HLS STREAM variable = output_fhd_8 depth = 512

	static vx_int8 SignedSobelChannel1[PIXELS_FHD];
#pragma HLS STREAM variable = SignedSobelChannel1 depth = 512

	static vx_uint8 UnsignedSobelChannel1[PIXELS_FHD];
#pragma HLS STREAM variable = UnsignedSobelChannel1 depth = 512

	vx_int8 tmp[PIXELS_FHD];

// Pragma to stream between loops/functions
#pragma HLS DATAFLOW

	imgConvertColor<vx_uint32, vx_uint8, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_RGBX, VX_DF_IMAGE_U8>(input, output_fhd_8);
	imgSobel3x3<uint8_t, int8_t, uint8_t, COLS_FHD, ROWS_FHD, SOBEL_BORDER>((uint8_t*) output_fhd_8, (uint8_t*) tmp, (uint8_t*) SignedSobelChannel1);
	imageSignedToUnsigned<int8_t, uint8_t, PIXELS_FHD>(SignedSobelChannel1, UnsignedSobelChannel1);
	ReplicateChannel(UnsignedSobelChannel1, output);
}


// Scharr Filter RGBx (32-bits) in X direction.
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwScharrX(vx_uint32 *input, vx_uint32* output) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS inline

	// FIFOs to stream data between functions
	vx_uint8 output_fhd_8[PIXELS_FHD];
#pragma HLS STREAM variable = output_fhd_8 depth = 512

	static vx_int8 SignedSobelChannel1[PIXELS_FHD];
#pragma HLS STREAM variable = SignedSobelChannel1 depth = 512

	static vx_uint8 UnsignedSobelChannel1[PIXELS_FHD];
#pragma HLS STREAM variable = UnsignedSobelChannel1 depth = 512

	vx_int8 tmp[PIXELS_FHD];

// Pragma to stream between loops/functions
#pragma HLS DATAFLOW

	imgConvertColor<vx_uint32, vx_uint8, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_RGBX, VX_DF_IMAGE_U8>(input, output_fhd_8);
	imgScharr3x3<uint8_t, int8_t, uint8_t, COLS_FHD, ROWS_FHD, SCHARR_BORDER>((uint8_t*) output_fhd_8, (uint8_t*) SignedSobelChannel1, (uint8_t*) tmp);
	imageSignedToUnsigned<int8_t, uint8_t, PIXELS_FHD>(SignedSobelChannel1, UnsignedSobelChannel1);
	ReplicateChannel(UnsignedSobelChannel1, output);
}


// Scharr Filter RGBx (32-bits) in Y direction.
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data copy(input[O:PIXELS_FHD], output[0:PIXELS_FHD])
#pragma SDS data data_mover(input:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
void hwScharrY(vx_uint32 *input, vx_uint32* output) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS inline

	// FIFOs to stream data between functions
	vx_uint8 output_fhd_8[PIXELS_FHD];
#pragma HLS STREAM variable = output_fhd_8 depth = 512

	static vx_int8 SignedSobelChannel1[PIXELS_FHD];
#pragma HLS STREAM variable = SignedSobelChannel1 depth = 512

	static vx_uint8 UnsignedSobelChannel1[PIXELS_FHD];
#pragma HLS STREAM variable = UnsignedSobelChannel1 depth = 512

	vx_int8 tmp[PIXELS_FHD];

// Pragma to stream between loops/functions
#pragma HLS DATAFLOW

	imgConvertColor<vx_uint32, vx_uint8, COLS_FHD, ROWS_FHD, VX_DF_IMAGE_RGBX, VX_DF_IMAGE_U8>(input, output_fhd_8);
	imgScharr3x3<uint8_t, int8_t, uint8_t, COLS_FHD, ROWS_FHD, SCHARR_BORDER>((uint8_t*) output_fhd_8, (uint8_t*) tmp, (uint8_t*) SignedSobelChannel1);
	imageSignedToUnsigned<int8_t, uint8_t, PIXELS_FHD>(SignedSobelChannel1, UnsignedSobelChannel1);
	ReplicateChannel(UnsignedSobelChannel1, output);
}
