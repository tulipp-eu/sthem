/******************************************************************************
 *
 *  This file is part of the Lynsyn PMU firmware.
 *
 *  Copyright 2018 Asbjørn Djupdal, NTNU, TULIPP EU Project
 *
 *  Lynsyn is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Lynsyn is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Lynsyn.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "lynsyn.h"
#include "adc.h"
#include "config.h"

#include <em_cmu.h>
#include <dmactrl.h>

#define REFERENCE adcRefExtSingle
#define ADC_DMA_CHANNEL 0

/***************************************************************************
 * Taken from AN0021
 *
 * @brief
 *   Calibrate offset and gain for the specified reference.
 *   Supports currently only single ended gain calibration.
 *   Could easily be expanded to support differential gain calibration.
 *
 * @details
 *   The offset calibration routine measures 0 V with the ADC, and adjust
 *   the calibration register until the converted value equals 0.
 *   The gain calibration routine needs an external reference voltage equal
 *   to the top value for the selected reference. For example if the 2.5 V
 *   reference is to be calibrated, the external supply must also equal 2.5V.
 *
 * @param[in] adc
 *   Pointer to ADC peripheral register block.
 *
 * @param[in] ref
 *   Reference used during calibration. Can be both external and internal
 *   references.
 *   The SCANGAIN and SINGLEGAIN calibration fields are not used when the 
 *   unbuffered differential 2xVDD reference is selected.
 * 
 * @return
 *   The final value of the calibration register, note that the calibration
 *   register gets updated with this value during the calibration.
 *   No need to load the calibration values after the function returns.
 ******************************************************************************/
static uint32_t ADC_Calibration(unsigned adcCalInput, int32_t adcGainCalValue) {
  int32_t  sample;
  uint32_t cal;

  /* Binary search variables */
  uint8_t high;
  uint8_t mid;
  uint8_t low;

  /* Reset ADC to be sure we have default settings and wait for ongoing */
  /* conversions to be complete. */
  ADC_Reset(ADC0);

  ADC_Init_TypeDef       init       = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;

  /* Init common settings for both single conversion and scan mode */
  init.timebase = ADC_TimebaseCalc(0);
  init.prescale = ADC_PrescaleCalc(ADC_CLOCK, 0);

  /* Set an oversampling rate for more accuracy */
  init.ovsRateSel = adcOvsRateSel4096;
  ADC_Init(ADC0, &init);

  /* Init for single conversion use, measure DIFF0 with selected reference. */
  singleInit.reference = REFERENCE;
  singleInit.input     = adcSingleInputDiff0;
  singleInit.acqTime   = adcAcqTime16;
  singleInit.diff      = true;
  /* Enable oversampling rate */
  singleInit.resolution = adcResOVS;

  ADC_InitSingle(ADC0, &singleInit);

  /* ADC is now set up for offset calibration */
  /* Offset calibration register is a 7 bit signed 2's complement value. */
  /* Use unsigned indexes for binary search, and convert when calibration */
  /* register is written to. */
  high = 128;
  low  = 0;

  /* Do binary search for offset calibration*/
  while(low < high) {
    /* Calculate midpoint */
    mid = low + (high - low) / 2;

    /* Midpoint is converted to 2's complement and written to both scan and */
    /* single calibration registers */
    cal      = ADC0->CAL & ~(_ADC_CAL_SINGLEOFFSET_MASK | _ADC_CAL_SCANOFFSET_MASK);
    cal     |= (uint8_t)(mid - 63) << _ADC_CAL_SINGLEOFFSET_SHIFT;
    cal     |= (uint8_t)(mid - 63) << _ADC_CAL_SCANOFFSET_SHIFT;
    ADC0->CAL = cal;
    
    /* Do a conversion */
    ADC_Start(ADC0, adcStartSingle);
    while(ADC0->STATUS & ADC_STATUS_SINGLEACT);

    /* Get ADC result */
    sample = ADC_DataSingleGet(ADC0);

    /* Check result and decide in which part of to repeat search */
    /* Calibration register has negative effect on result */
    if(sample < 0) {
      /* Repeat search in bottom half. */
      high = mid;
    } else if(sample > 0) {
      /* Repeat search in top half. */
      low = mid + 1;
    } else {
      /* Found it, exit while loop */
      break;
    }
  }

  /* Now do gain calibration, only INPUT and DIFF settings needs to be changed */
  ADC0->SINGLECTRL &= ~(_ADC_SINGLECTRL_INPUTSEL_MASK | _ADC_SINGLECTRL_DIFF_MASK);
  ADC0->SINGLECTRL |= (adcCalInput << _ADC_SINGLECTRL_INPUTSEL_SHIFT);
  ADC0->SINGLECTRL |= (false << _ADC_SINGLECTRL_DIFF_SHIFT);

  /* Gain calibration register is a 7 bit unsigned value. */
  high = 128;
  low  = 0;

  /* Do binary search for gain calibration */
  while(low < high) {
    /* Calculate midpoint and write to calibration register */
    mid = low + (high - low) / 2;
    cal      = ADC0->CAL & ~(_ADC_CAL_SINGLEGAIN_MASK | _ADC_CAL_SCANGAIN_MASK);
    cal     |= mid << _ADC_CAL_SINGLEGAIN_SHIFT;
    cal     |= mid << _ADC_CAL_SCANGAIN_SHIFT;
    ADC0->CAL = cal;

    /* Do a conversion */
    ADC_Start(ADC0, adcStartSingle);
    while(ADC0->STATUS & ADC_STATUS_SINGLEACT);

    /* Get ADC result */
    sample = ADC_DataSingleGet(ADC0);

    /* Check result and decide in which part to repeat search */
    /* Compare with a value atleast one LSB's less than top to avoid overshooting */
    /* Since oversampling is used, the result is 16 bits, but a couple of lsb's */
    /* applies to the 12 bit result value, if 0xffd is the top value in 12 bit, this */
    /* is in turn 0xffd0 in the 16 bit result. */
    /* Calibration register has positive effect on result */
    if(sample > adcGainCalValue) {
      /* Repeat search in bottom half. */
      high = mid;
    } else if(sample < adcGainCalValue) {
      /* Repeat search in top half. */
      low = mid + 1;
    } else {
      /* Found it, exit while loop */
      break;
    }
  }
  return ADC0->CAL;
}

void adcCalibrate(unsigned sensor, int32_t adcCalValue) {
  unsigned adcCalInput = adcSingleInputCh0;

  switch(sensor) {
    case 0: adcCalInput = adcSingleInputCh0; break;
    case 1: adcCalInput = adcSingleInputCh1; break;
    case 2: adcCalInput = adcSingleInputCh2; break;
    case 3: adcCalInput = adcSingleInputCh3; break;
    case 4: adcCalInput = adcSingleInputCh4; break;
    case 5: adcCalInput = adcSingleInputCh5; break;
    case 6: adcCalInput = adcSingleInputCh7; break;
    default: panic("Invalid sensor\n");
  }

  uint32_t cal = ADC_Calibration(adcCalInput, adcCalValue);
  setUint32("adcCal", cal);

  adcInit();
}

void adcInit(void) {
  CMU_ClockEnable(cmuClock_DMA, true);
  
  DMA_Init_TypeDef dmaInit;
  dmaInit.hprot = 0;
  dmaInit.controlBlock = dmaControlBlock;
  DMA_Init(&dmaInit);

  DMA_CfgChannel_TypeDef chnlCfg;
  chnlCfg.highPri = false;
  chnlCfg.enableInt = false;
  chnlCfg.select = DMAREQ_ADC0_SCAN;
  chnlCfg.cb = NULL;
  DMA_CfgChannel(ADC_DMA_CHANNEL, &chnlCfg);

  DMA_CfgDescr_TypeDef descrCfg;
  descrCfg.dstInc = dmaDataInc2;
  descrCfg.srcInc = dmaDataIncNone;
  descrCfg.size = dmaDataSize2;
  descrCfg.arbRate = dmaArbitrate1;
  descrCfg.hprot = 0;
  DMA_CfgDescr(ADC_DMA_CHANNEL, true, &descrCfg);
  
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  init.timebase = ADC_TimebaseCalc(0);
  init.prescale = ADC_PrescaleCalc(ADC_CLOCK, 0);
  init.lpfMode = ADC_CTRL_LPFMODE_DECAP;
  init.ovsRateSel = adcOvsRateSel8;
  ADC_Init(ADC0, &init);

  ADC_InitScan_TypeDef scanInit = ADC_INITSCAN_DEFAULT;
  scanInit.reference = REFERENCE;
  scanInit.input = ADC_SCANCTRL_INPUTMASK_CH0 |
                   ADC_SCANCTRL_INPUTMASK_CH1 |
                   ADC_SCANCTRL_INPUTMASK_CH2 |
                   ADC_SCANCTRL_INPUTMASK_CH3 |
                   ADC_SCANCTRL_INPUTMASK_CH4 |
                   ADC_SCANCTRL_INPUTMASK_CH5 |
                   ADC_SCANCTRL_INPUTMASK_CH7;
  scanInit.resolution = adcResOVS;
  ADC_InitScan(ADC0, &scanInit);

  if(configExists("adcCal")) {
    uint32_t cal = getUint32("adcCal");
    ADC0->CAL = cal;
  }
}

void adcScan(int16_t *sampleBuffer) {
  DMA_ActivateBasic(ADC_DMA_CHANNEL,
                    true,
                    false,
                    sampleBuffer,
                    (void *)((uint32_t)&(ADC0->SCANDATA)),
                    6);
  
  ADC_Start(ADC0, adcStartScan);
}

void adcScanWait(void) {
  while(ADC0->STATUS & ADC_STATUS_SCANACT);
}
