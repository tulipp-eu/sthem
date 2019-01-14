/**
* Licence: GNU GPLv3 \n
* You may copy, distribute and modify the software as long as you track
* changes/dates in source files. Any modifications to or software
* including (via compiler) GPL-licensed code must also be made available
* under the GPL along with build & install instructions.
*
* @file    img_pixelop_help.h
* @author  Lester Kalms <lester.kalms@tu-dresden.de>
* @version 1.0
* @brief Description:\n
*  These functions test the pixel operation functions
*/

#ifndef SRC_IMG_PIXELOP_TEST_H_
#define SRC_IMG_PIXELOP_TEST_H_

#include "img_pixelop_base.h"

using namespace std;

/*********************************************************************************************************************/
/* Test Functions Headers */
/*********************************************************************************************************************/

void swTestMagnitude(void);
void swTestPixelopMain(void);

/*********************************************************************************************************************/
/* SW Test Functions */
/*********************************************************************************************************************/

/*! \brief Calls all Software Test Functions */
void swTestPixelopMain(void) {
	swTestMagnitude();
}

/*! \brief Test the Magnitude Function */
void swTestMagnitude(void) {

	// Change Function Setup Here
	const vx_uint32 COLS = 1280;
	const vx_uint32 ROWS = 720;	
	typedef vx_uint32 vectorT;
	typedef vx_uint16 scalarT;

	// Constants
	const vx_uint32 PIXELS = COLS * ROWS;

	// Create device buffers 
	scalarT *inputA = new scalarT[PIXELS];
	scalarT *inputB = new scalarT[PIXELS];
	scalarT *output = new scalarT[PIXELS];

	// Create Host Buffers
	float *outputDevA = new float[PIXELS];
	float *imgHostA = new float[PIXELS];
	float *imgHostB = new float[PIXELS];
	float *outputHstA = new float[PIXELS];

	// Create an image :) and transform it to floating point format
	bool turn = true;
	for (int i = 0, count = 0; i < PIXELS; i++) {
		count = count + (turn ? 1 : -1);
		if (count >= 222 || count <= 0) turn = !turn;
		float m = ((float)(count + (rand() % 34)));
		float n = ((float)(count + (rand() % 34)));
		imgHostA[i] = m / 256.0f;
		imgHostB[i] = n / 256.0f;
	}

	// Convert to fixed point
	convertFloatingToInteger<scalarT, PIXELS>(imgHostA, inputA);
	convertFloatingToInteger<scalarT, PIXELS>(imgHostB, inputB);

	// Check the Magnitude Function
	cout << "Magnitude:\n";
	imgMagnitude<scalarT, vectorT, PIXELS>((vectorT *)inputA, (vectorT *)inputB, (vectorT *)output);
	convertIntegerToFloating<scalarT, PIXELS>(output, outputDevA);
	for (vx_uint32 i = 0; i<PIXELS; i++) {
		float A = imgHostA[i];
		float B = imgHostB[i];
		outputHstA[i] = min(1.0f, sqrtf(A*A + B*B));
	}
	checkErrorFloat<PIXELS>(outputHstA, outputDevA);

	// Free memory
	delete[] inputA;
	delete[] inputB;
	delete[] output;
	delete[] outputDevA;
	delete[] imgHostA;
	delete[] imgHostB;
	delete[] outputHstA;
}

#endif /* SRC_PIXELOP_TEST_H_ */
