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

#include <stdio.h>

#include <em_usb.h>
#include <em_msc.h>

#include "lynsyn_main.h"
#include "usb.h"
#include "adc.h"
#include "config.h"
#include "jtag.h"
#include "fpga.h"
#include "../common/usbprotocol.h"

#include "descriptors.h"

///////////////////////////////////////////////////////////////////////////////

uint8_t startCore;
uint8_t stopCore;
uint64_t frameBp;

static uint32_t crc_mcu;

static uint32_t lowWanted[7];
static uint32_t lowActual[7];

static uint8_t inBuffer[MAX_PACKET_SIZE + 3];

struct UsbTestReplyPacket usbTestReply __attribute__((__aligned__(4)));
struct CalInfoPacket calInfo __attribute__((__aligned__(4)));
struct InitReplyPacket initReply __attribute__((__aligned__(4)));
struct TestReplyPacket testReply __attribute__((__aligned__(4)));
struct AdcTestReplyPacket adcTestReply __attribute__((__aligned__(4)));

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

int InitSent(USB_Status_TypeDef status, uint32_t xf, uint32_t remaining) {
  (void)remaining;

  calInfo.offset[0] = getDouble("offset0");
  calInfo.offset[1] = getDouble("offset1");
  calInfo.offset[2] = getDouble("offset2");
  calInfo.offset[3] = getDouble("offset3");
  calInfo.offset[4] = getDouble("offset4");
  calInfo.offset[5] = getDouble("offset5");
  calInfo.offset[6] = getDouble("offset6");

  calInfo.gain[0] = getDouble("gain0");
  calInfo.gain[1] = getDouble("gain1");
  calInfo.gain[2] = getDouble("gain2");
  calInfo.gain[3] = getDouble("gain3");
  calInfo.gain[4] = getDouble("gain4");
  calInfo.gain[5] = getDouble("gain5");
  calInfo.gain[6] = getDouble("gain6");

  while(USBD_EpIsBusy(CDC_EP_DATA_IN));
  int ret = USBD_Write(CDC_EP_DATA_IN, &calInfo, sizeof(struct CalInfoPacket), UsbDataSent);
  if(ret != USB_STATUS_OK) printf("USB write error: %d\n", ret);

  jtagExt();

  return USB_STATUS_OK;
}

void Fill_boot_reply_package( struct BootInitReplyPackage  * boot_reply_package)
{
	boot_reply_package->hwVersion=getUint32("hwver");        // hardware_version
	boot_reply_package->swVersion=SW_VERSION;// software_version
	//BootInit_reply_package->NANDflash_version=0x7520; // nan versian
	boot_reply_package->BlVersion=2102;
	/*
 int res1 = check_flash_for_Framework(Flash_offset1 ,  inBuffer );// check nand flash Flash_offset1 section
 int res2 = check_flash_for_Framework(Flash_offset2 ,  inBuffer );// check nand flash Flash_offset2 section
 if((res1 == -1)||(res2 == -1) )
 {
	 // if none of flash section valid , return not ready
  boot_reply_package->ready = 0 ;
  return ;
 }
 if(res1<0)
 {
	 res1=0;
 }
 if(res2<0)
 {
	 res2=0;
 }
 */
	boot_reply_package->ready = 1 ;// raedy
 //boot_reply_package->boot_version1 = res1 ;
// boot_reply_package->boot_version2 = res2 ;
 return ;
}

void Send_Data2PC(uint8_t *inBuffer, int length) {
	// send data to host 
  while(USBD_EpIsBusy(CDC_EP_DATA_IN));
  int ret = USBD_Write(CDC_EP_DATA_IN, inBuffer, length , UsbDataSent);
  if(ret != USB_STATUS_OK) printf("Data error send: %d\n", ret);
}

void Flash_erase()
{
	int i;
	uint8_t * pointer_internal= (uint8_t *)FLASH_NEW_APPLICATION_START;
	for(i=(uint32_t)pointer_internal;i<FLASH_SIZE;i+=4096)
	 {
		 // erace internal flash
		 MSC_ErasePage((uint32_t*)i);
	 }
	// erace nand flash from flash_address_start to flash_address_end
}

uint32_t CRC32(uint32_t CRC , uint32_t *data , int length )
{
 int i ;
 for( i = 0 ; i < length ; i++ )
 {
  CRC = CRC + data[i] ;
 }
 return CRC ;
}

int UsbDataReceived(USB_Status_TypeDef status, uint32_t xf, uint32_t remaining) {
  if((status == USB_STATUS_OK) && (xf > 0)) {
    struct RequestPacket *req = (struct RequestPacket *)inBuffer;

    switch(req->cmd) {

      case USB_CMD_INIT: {
        initReply.hwVersion = getUint32("hwver");
        initReply.swVersion = SW_VERSION;

        initReply.calibration[0] = 0;
        initReply.calibration[1] = 0;
        initReply.calibration[2] = 0;
        initReply.calibration[3] = 0;
        initReply.calibration[4] = 0;
        initReply.calibration[5] = 0;
        initReply.calibration[6] = 0;
        initReply.adcCal = 0;

        while(USBD_EpIsBusy(CDC_EP_DATA_IN));
        int ret = USBD_Write(CDC_EP_DATA_IN, &initReply, sizeof(struct InitReplyPacket), InitSent);
        if(ret != USB_STATUS_OK) printf("USB write error: %d\n", ret);

        break;
      }

      case USB_CMD_HW_INIT: {
        struct HwInitRequestPacket *hwInitReq = (struct HwInitRequestPacket *)req;

        printf("Init non-volatile memory, version %d\n", hwInitReq->hwVersion);

        clearConfig();
        setUint32("hwver", hwInitReq->hwVersion);

        setDouble("offset0", 0);
        setDouble("offset1", 0);
        setDouble("offset2", 0);
        setDouble("offset3", 0);
        setDouble("offset4", 0);
        setDouble("offset5", 0);
        setDouble("offset6", 0);

        setDouble("gain0", 1);
        setDouble("gain1", 1);
        setDouble("gain2", 1);
        setDouble("gain3", 1);
        setDouble("gain4", 1);
        setDouble("gain5", 1);
        setDouble("gain6", 1);

        break;
      }

      case USB_CMD_BootMode: {
        // ack to host command for Boot info and Version
        MSC_Init();
        struct BootInitReplyPackage boot_reply_package;
        Fill_boot_reply_package( &boot_reply_package ) ;
        boot_reply_package.reply.cmd= USB_CMD_BootMode;
        Send_Data2PC((uint8_t *)&boot_reply_package,sizeof (struct BootInitReplyPackage));
        Flash_erase();
        crc_mcu = 0;
        printf("Receiving new firmware...\n");
        MSC_Deinit();
        break;
      }

      case USB_CMD_FLASH_Save: {
        MSC_Init();

        struct FlashBootPackage *flash_boot_package = (struct FlashBootPackage *)req;

        uint32_t *base_add = (uint32_t*)flash_boot_package->flash_address;
        uint32_t *pointer_inbuffer = (uint32_t*)flash_boot_package->Data;

        MSC_WriteWord(base_add, pointer_inbuffer, FLASH_BUFFER_SIZE);

        crc_mcu=CRC32(crc_mcu,(uint32_t * )flash_boot_package->Data,FLASH_BUFFER_SIZE/4);

        MSC_Deinit();

        break;
      }

      case USB_CMD_RESET: {
        //finalize operation
        struct ResetPackage *resetPackage = (struct ResetPackage *)req;

        printf("New firmware received\n");

        if(crc_mcu != resetPackage->crc) {
          printf("Incorrect CRC, not upgrading\n");

        } else {
          MSC_Init();

          uint32_t buf = NEW_APP_MAGIC;

          MSC_WriteWord((uint32_t*)FLASH_NEW_APPLICATION_END, &buf, 4);

          MSC_Deinit();

          DWT->CYCCNT = 0;
          while(DWT->CYCCNT < 24000000);

          NVIC_SystemReset();
        }

        break;
      }

      case USB_CMD_JTAG_INIT:
        printf("Init JTAG chain\n");
        jtagInt();
        if(!jtagInitCores()) {
          panic("Can't init JTAG\n");
        }
        break;

      case USB_CMD_BREAKPOINT: {
        struct BreakpointRequestPacket *bpReq = (struct BreakpointRequestPacket *)req;

        switch(bpReq->bpType) {
          case BP_TYPE_START:
            printf("Set start BP %llx on core %d\n", bpReq->addr, bpReq->core);
            coreSetBp(bpReq->core, START_BP, bpReq->addr);
            break;
          case BP_TYPE_STOP:
            printf("Set stop BP %llx on core %d\n", bpReq->addr, bpReq->core);
            coreSetBp(bpReq->core, STOP_BP, bpReq->addr);
            stopCore = bpReq->core;
            break;
          case BP_TYPE_FRAME:
            printf("Set frame BP %llx on core %d\n", bpReq->addr, bpReq->core);
            frameBp = bpReq->addr;
            //coreSetBp(bpReq->core, FRAME_BP, bpReq->addr);
            break;
        }
            
        break;
      }

      case USB_CMD_START_SAMPLING: {
        struct StartSamplingRequestPacket *startSamplingReq = (struct StartSamplingRequestPacket *)req;

        samplePc = startSamplingReq->flags & SAMPLING_FLAG_SAMPLE_PC;
        gpioMode = startSamplingReq->flags & SAMPLING_FLAG_GPIO;
        useStartBp = startSamplingReq->flags & SAMPLING_FLAG_BP;
        useStopBp = !(startSamplingReq->flags & SAMPLING_FLAG_PERIOD);

        printf("Starting sample mode (%llx)\n", startSamplingReq->flags);

        setLed(0);

        if(useStartBp) {
          // run until start bp
          coresResume();

          // wait until we reach start bp
          while(!coreHalted(startCore));

          // sampling starts here
          sampleMode = true;

          coreClearBp(startCore, START_BP);
          coreSetBp(startCore, FRAME_BP, frameBp);
          coresResume();

        } else {
          // sampling starts here
          sampleMode = true;
        }

        sampleStop = startSamplingReq->samplePeriod + calculateTime();

        break;
      }

      case USB_CMD_CAL: {
        struct CalibrateRequestPacket *cal = (struct CalibrateRequestPacket *)req;

        int16_t current[7];

        uint32_t currentAvg = 0;
        for(int i = 0; i < CAL_AVERAGE_SAMPLES; i++) {
          adcScan(current);
          adcScanWait();
          currentAvg += current[cal->channel];
        }
        currentAvg /= CAL_AVERAGE_SAMPLES;
        
        printf("cal %lx\n", currentAvg);

        if(cal->flags & CALREQ_FLAG_HIGH) {
          uint32_t highWanted = cal->calVal >> 1;
          uint32_t highActual = currentAvg;

          double slope = ((double)highActual - (double)lowActual[cal->channel]) / ((double)highWanted - (double)lowWanted[cal->channel]);
          double offset = (double)highActual - slope * (double)highWanted;

          double gain = 1 / slope;

          printf("Calibrating ADC channel %d (offset %f gain %f)\n", cal->channel, offset, gain);

          switch(cal->channel) {
            case 0:
              setDouble("offset0", offset);
              setDouble("gain0", gain);
              break;
            case 1:
              setDouble("offset1", offset);
              setDouble("gain1", gain);
              break;
            case 2:
              setDouble("offset2", offset);
              setDouble("gain2", gain);
              break;
            case 3:
              setDouble("offset3", offset);
              setDouble("gain3", gain);
              break;
            case 4:
              setDouble("offset4", offset);
              setDouble("gain4", gain);
              break;
            case 5:
              setDouble("offset5", offset);
              setDouble("gain5", gain);
              break;
            case 6:
              setDouble("offset6", offset);
              setDouble("gain6", gain);
              break;
          }

        } else {
          lowWanted[cal->channel] = cal->calVal >> 1;
          lowActual[cal->channel] = currentAvg;
        }

        break;
      }

      case USB_CMD_CAL_SET: {
        struct CalSetRequestPacket *cal = (struct CalSetRequestPacket *)req;

        printf("Setting ADC channel %d (val %f %f)\n", (int)cal->channel, cal->offset, cal->gain);

        switch(cal->channel) {
          case 0:
            setDouble("offset0", cal->offset);
            setDouble("gain0", cal->gain);
            break;
          case 1:
            setDouble("offset1", cal->offset);
            setDouble("gain1", cal->gain);
            break;
          case 2:
            setDouble("offset2", cal->offset);
            setDouble("gain2", cal->gain);
            break;
          case 3:
            setDouble("offset3", cal->offset);
            setDouble("gain3", cal->gain);
            break;
          case 4:
            setDouble("offset4", cal->offset);
            setDouble("gain4", cal->gain);
            break;
          case 5:
            setDouble("offset5", cal->offset);
            setDouble("gain5", cal->gain);
            break;
          case 6:
            setDouble("offset6", cal->offset);
            setDouble("gain6", cal->gain);
            break;
        }

        break;
      }

      case USB_CMD_TEST: {
        struct TestRequestPacket *testReq = (struct TestRequestPacket *)req;

        testReply.testStatus = 0;

        bool sendStatus = true;

        switch(testReq->testNum) {
          case TEST_USB: { // USB communication
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
            sendStatus = false;

            int16_t current[7];
            uint32_t currentAvg[7];

            for(int sensor = 0; sensor < 7; sensor++) {
              currentAvg[sensor] = 0;
            }
            for(int i = 0; i < CAL_AVERAGE_SAMPLES; i++) {
              adcScan(current);
              adcScanWait();
              for(int sensor = 0; sensor < 7; sensor++) {
                currentAvg[sensor] += current[sensor];
              }
            }
            for(int sensor = 0; sensor < 7; sensor++) {
              adcTestReply.current[sensor] = currentAvg[sensor] / CAL_AVERAGE_SAMPLES;
            }

            while(USBD_EpIsBusy(CDC_EP_DATA_IN));
            int ret = USBD_Write(CDC_EP_DATA_IN, &adcTestReply, sizeof(struct AdcTestReplyPacket), UsbDataSent);
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

void sendSamples(struct SampleReplyPacket *sample, unsigned n) {
  while(USBD_EpIsBusy(CDC_EP_DATA_IN));
  int ret = USBD_Write(CDC_EP_DATA_IN, sample, n * sizeof(struct SampleReplyPacket), UsbDataSent);
  if(ret != USB_STATUS_OK) printf("USB write error: %d\n", ret);
}
