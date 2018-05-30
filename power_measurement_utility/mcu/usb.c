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
#include "usb.h"
#include "adc.h"
#include "config.h"

#include "descriptors.h"

#include <stdio.h>
#include <em_usb.h>

#include "jtag.h"
#include "fpga.h"
#include "usbprotocol.h"

#define START_BP 0
#define STOP_BP  1

///////////////////////////////////////////////////////////////////////////////

unsigned startCore;
unsigned stopCore;

static uint8_t inBuffer[MAX_PACKET_SIZE + 3];

///////////////////////////////////////////////////////////////////////////////

void UsbStateChange(USBD_State_TypeDef oldState, USBD_State_TypeDef newState);

static const USBD_Callbacks_TypeDef callbacks = {
  .usbReset        = NULL,
  .usbStateChange  = UsbStateChange,
  .setupCmd        = NULL,
  .isSelfPowered   = NULL,
  .sofInt          = NULL
};

const USBD_Init_TypeDef initstruct = {
  .deviceDescriptor    = &USBDESC_deviceDesc,
  .configDescriptor    = USBDESC_configDesc,
  .stringDescriptors   = USBDESC_strings,
  .numberOfStrings     = sizeof(USBDESC_strings)/sizeof(void*),
  .callbacks           = &callbacks,
  .bufferingMultiplier = USBDESC_bufferingMultiplier,
  .reserved            = 0
};

///////////////////////////////////////////////////////////////////////////////

int UsbDataSent(USB_Status_TypeDef status, uint32_t xf, uint32_t remaining) {
  (void)remaining;
  return USB_STATUS_OK;
}

int UsbDataReceived(USB_Status_TypeDef status, uint32_t xf, uint32_t remaining) {
  if((status == USB_STATUS_OK) && (xf > 0)) {
    struct RequestPacket *req = (struct RequestPacket *)inBuffer;

    switch(req->cmd) {

      case USB_CMD_INIT: {
        struct InitReplyPacket initReply;
        initReply.hwVersion = getUint32("hwver");
        initReply.swVersion = USB_PROTOCOL_VERSION;
        initReply.calibration[0] = getDouble("cal0");
        initReply.calibration[1] = getDouble("cal1");
        initReply.calibration[2] = getDouble("cal2");
        initReply.calibration[3] = getDouble("cal3");
        initReply.calibration[4] = getDouble("cal4");
        initReply.calibration[5] = getDouble("cal5");
        initReply.calibration[6] = getDouble("cal6");

        while(USBD_EpIsBusy(CDC_EP_DATA_IN));
        int ret = USBD_Write(CDC_EP_DATA_IN, &initReply, sizeof(struct InitReplyPacket), UsbDataSent);
        if(ret != USB_STATUS_OK) printf("USB write error: %d\n", ret);

        jtagExt();

        break;
      }

      case USB_CMD_HW_INIT: {
        struct HwInitRequestPacket *hwInitReq = (struct HwInitRequestPacket *)req;

        printf("Init non-volatile memory, version %d\n", hwInitReq->hwVersion);

        clearConfig();
        setUint32("hwver", hwInitReq->hwVersion);

        setDouble("cal0", 1);
        setDouble("cal1", 1);
        setDouble("cal2", 1);
        setDouble("cal3", 1);
        setDouble("cal4", 1);
        setDouble("cal5", 1);
        setDouble("cal6", 1);

        break;
      }

      case USB_CMD_JTAG_INIT:
        printf("Init JTAG chain\n");
        jtagInt();
        jtagInitCores();
        break;

      case USB_CMD_BREAKPOINT: {
        struct BreakpointRequestPacket *bpReq = (struct BreakpointRequestPacket *)req;

        printf("Set %s BP %llx on core %d\n", (bpReq->bpType == BP_TYPE_START) ? "start" : "stop", bpReq->addr, bpReq->core);
        coreSetBp(bpReq->core, (bpReq->bpType == BP_TYPE_START) ? START_BP : STOP_BP, bpReq->addr);
        break;
      }

      case USB_CMD_START_SAMPLING:
        printf("Starting sample mode\n");

        sampleMode = true;

        // run until first bp
        coresResume();
        // wait until we reach bp
        while(!coreHalted());
        // run until next bp
        coreClearBp(startCore, START_BP);
        coresResume();
        
        break;

      case USB_CMD_CAL: {
        struct CalibrateRequestPacket *cal = (struct CalibrateRequestPacket *)req;

        printf("Calibrating ADC channel %d (val %x) hw: %d\n", cal->channel, (unsigned)cal->calVal, cal->hw);

        if(cal->hw) {
          adcCalibrate(cal->channel, cal->calVal);
        }

        int16_t current[7];

        uint32_t currentAvg = 0;
        for(int i = 0; i < CAL_AVERAGE_SAMPLES; i++) {
          adcScan(current);
          adcScanWait();
          currentAvg += current[cal->channel];
        }
        currentAvg /= CAL_AVERAGE_SAMPLES;

        switch(cal->channel) {
          case 0: setDouble("cal0", (cal->calVal >> 1) / (double)currentAvg); break;
          case 1: setDouble("cal1", (cal->calVal >> 1) / (double)currentAvg); break;
          case 2: setDouble("cal2", (cal->calVal >> 1) / (double)currentAvg); break;
          case 3: setDouble("cal3", (cal->calVal >> 1) / (double)currentAvg); break;
          case 4: setDouble("cal4", (cal->calVal >> 1) / (double)currentAvg); break;
          case 5: setDouble("cal5", (cal->calVal >> 1) / (double)currentAvg); break;
          case 6: setDouble("cal6", (cal->calVal >> 1) / (double)currentAvg); break;
        }

        break;
      }

      case USB_CMD_TEST: {
        struct TestRequestPacket *testReq = (struct TestRequestPacket *)req;
        struct TestReplyPacket testReply __attribute__((__aligned__(4)));

        testReply.testStatus = 0;

        bool sendStatus = true;

        switch(testReq->testNum) {
          case TEST_USB: { // USB communication
            struct UsbTestReplyPacket usbTestReply __attribute__((__aligned__(4)));

            sendStatus = false;

            for(int i = 0; i < 256; i++) {
              usbTestReply.buf[i] = i;
            }

            while(USBD_EpIsBusy(CDC_EP_DATA_IN));
            int ret = USBD_Write(CDC_EP_DATA_IN, &usbTestReply, sizeof(struct UsbTestReplyPacket), UsbDataSent);
            if(ret != USB_STATUS_OK) printf("USB write error: %d\n", ret);

            break;
          }
          case TEST_SPI: { // SPI channel
#ifdef USE_FPGA_JTAG_CONTROLLER
            testReply.testStatus = fpgaInitOk();
#else
            testReply.testStatus = FPGA_INIT_OK;
#endif
            break;
          }
          case TEST_JTAG: { // JTAG ports
#ifdef USE_FPGA_JTAG_CONTROLLER
            testReply.testStatus = jtagTest();
#endif
            break;
          }
          case TEST_OSC: { // FPGA oscillator
#ifdef USE_FPGA_JTAG_CONTROLLER
            testReply.testStatus = oscTest();
#else
            testReply.testStatus = true;
#endif
            break;
          }
          case TEST_LEDS_ON: { // LEDs on
            setLed(0);
            setLed(1);
            sendStatus = false;
            break;
          }
          case TEST_LEDS_OFF: { // LEDs off
            clearLed(0);
            clearLed(1);
            sendStatus = false;
            break;
          }
          case TEST_ADC: { // ADC channel test
            struct AdcTestReplyPacket testReply __attribute__((__aligned__(4)));

            sendStatus = false;

            adcScan(testReply.current);
            adcScanWait();

            while(USBD_EpIsBusy(CDC_EP_DATA_IN));
            int ret = USBD_Write(CDC_EP_DATA_IN, &testReply, sizeof(struct AdcTestReplyPacket), UsbDataSent);
            if(ret != USB_STATUS_OK) printf("USB write error: %d\n", ret);

            break;
          }
        }

        if(sendStatus) {
          while(USBD_EpIsBusy(CDC_EP_DATA_IN));
          int ret = USBD_Write(CDC_EP_DATA_IN, &testReply, sizeof(struct TestReplyPacket), UsbDataSent);
          if(ret != USB_STATUS_OK) printf("USB write error: %d\n", ret);
        }

        break;
      }
    }
  }
  USBD_Read(CDC_EP_DATA_OUT, inBuffer, MAX_PACKET_SIZE, UsbDataReceived);

  return USB_STATUS_OK;
}

void UsbStateChange(USBD_State_TypeDef oldState, USBD_State_TypeDef newState) {
  (void) oldState;
  if(newState == USBD_STATE_CONFIGURED) {
    USBD_Read(CDC_EP_DATA_OUT, inBuffer, MAX_PACKET_SIZE, UsbDataReceived);
  }
}

void usbInit(void) {
  USBD_Init(&initstruct);
}

void sendSample(struct SampleReplyPacket *sample) {
  while(USBD_EpIsBusy(CDC_EP_DATA_IN));
  int ret = USBD_Write(CDC_EP_DATA_IN, sample, sizeof(struct SampleReplyPacket), UsbDataSent);
  if(ret != USB_STATUS_OK) printf("USB write error: %d\n", ret);
}
