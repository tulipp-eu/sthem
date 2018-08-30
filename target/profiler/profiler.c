#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <xparameters.h>
#include <xstatus.h>
#include <xil_exception.h>
#include <xttcps.h>
#include <xscugic.h>
#include <xiicps.h>

#include <compatibility_layer.h>

#include "profiler.h"

extern char __rodata_start;

#define TTC_TICK_DEVICE_ID XPAR_XTTCPS_1_DEVICE_ID
#define TTC_TICK_INTR_ID   XPAR_XTTCPS_1_INTR
#define INTC_DEVICE_ID     XPAR_SCUGIC_SINGLE_DEVICE_ID

#define I2C_NO_CMD       0
#define I2C_MAGIC        1
#define I2C_READ_CURRENT 2

#define IIC_DEVICE_ID   XPAR_XIICPS_1_DEVICE_ID
#define IIC_SLAVE_ADDR     0x55
#define IIC_SCLK_RATE    200000

static XIicPs Iic;
static XScuGic interruptController;
static XTtcPs timer;

static uint64_t *currentBuf;
static uint64_t *ticksBuf;

static uint64_t unknownCurrent;
static uint64_t unknownTicks;

static uint64_t pcStart;
static int bufSize;

static volatile bool profilerActive;

static double pcSamplerPeriod;

static bool iicReadData(uint8_t *recBuf, int size);

static void timerInterruptHandler(void *CallBackRef) {
  uint32_t StatusEvent = XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
  XTtcPs_ClearInterruptStatus((XTtcPs *)CallBackRef, StatusEvent);

  if((XTTCPS_IXR_INTERVAL_MASK & StatusEvent) != 0) {
    if(profilerActive) {
      uint64_t pc = 0;
      asm ("mrs %0, ELR_EL3\n":"=r"(pc)::);

      int16_t current;
      iicReadData((uint8_t*)&current, 2);

      int index = pc/4 - pcStart;
      if((index >= 0) && (index < bufSize)) {
        currentBuf[index] += current;
        ticksBuf[index]++;
      } else {
        unknownCurrent += current;
        unknownTicks++;
      }
    }
  }
}

static bool setupIic(void) {
  int Status;
  XIicPs_Config *Config;

  /*
   * Initialize the IIC driver so that it's ready to use
   * Look up the configuration in the config table,
   * then initialize it.
   */
  Config = XIicPs_LookupConfig(IIC_DEVICE_ID);
  if (NULL == Config) {
    return false;
  }

  Status = XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);
  if (Status != XST_SUCCESS) {
    return false;
  }

  /*
   * Perform a self-test to ensure that the hardware was built correctly.
   */
  Status = XIicPs_SelfTest(&Iic);
  if (Status != XST_SUCCESS) {
    return false;
  }

  /*
   * Set the IIC serial clock rate.
   */
  XIicPs_SetSClk(&Iic, IIC_SCLK_RATE);

  return true;
}

static bool iicSetCommand(uint8_t cmd, uint8_t arg) {
  uint8_t buf[2];
  buf[0] = cmd;
  buf[1] = arg;

  while(XIicPs_BusIsBusy(&Iic));

  int Status = XIicPs_MasterSendPolled(&Iic, buf, 2, IIC_SLAVE_ADDR);

  return Status == XST_SUCCESS;
}

static bool iicReadData(uint8_t *recBuf, int size) {
  while(XIicPs_BusIsBusy(&Iic));

  int Status = XIicPs_MasterRecvPolled(&Iic, recBuf, size, IIC_SLAVE_ADDR);

  return Status == XST_SUCCESS;
}

static bool setupTimer(void) {
  int status;
  XTtcPs_Config *config;

  XInterval interval = 0;
  u8 prescaler = 0;

  config = XTtcPs_LookupConfig(TTC_TICK_DEVICE_ID);
  if(config == NULL) {
    return false;
  }

  status = XTtcPs_CfgInitialize(&timer, config, config->BaseAddress);
  if(status != XST_SUCCESS) {
    return false;
  }

  XTtcPs_SetOptions(&timer, XTTCPS_OPTION_INTERVAL_MODE | XTTCPS_OPTION_WAVE_DISABLE);

  XTtcPs_CalcIntervalFromFreq(&timer, 1 / pcSamplerPeriod, &interval, &prescaler);

  XTtcPs_SetInterval(&timer, interval);
  XTtcPs_SetPrescaler(&timer, prescaler);

  status = XScuGic_Connect(&interruptController, TTC_TICK_INTR_ID, (Xil_ExceptionHandler)timerInterruptHandler, (void *)&timer);
  if(status != XST_SUCCESS) return false;

  XScuGic_Enable(&interruptController, TTC_TICK_INTR_ID);

  XTtcPs_EnableInterrupts(&timer, XTTCPS_IXR_INTERVAL_MASK);

  XTtcPs_Start(&timer);

  return true;
}

static bool setupInterrupts(void) {
  int status;
  XScuGic_Config *IntcConfig;

  Xil_ExceptionInit();

  IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
  if(NULL == IntcConfig) {
    return false;
  }

  status = XScuGic_CfgInitialize(&interruptController, IntcConfig,
                                 IntcConfig->CpuBaseAddress);
  if(status != XST_SUCCESS) {
    return false;
  }

  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                               (Xil_ExceptionHandler) XScuGic_InterruptHandler,
                               &interruptController);

  Xil_ExceptionEnable();

  return true;
}

bool startProfiler(double period) {
  pcSamplerPeriod = period;

  uint64_t textStart = 0;
  uint64_t textSize = (uint64_t)&__rodata_start;

  if(setupInterrupts()) {
    if(setupIic()) {

      uint8_t magic;
      iicSetCommand(I2C_MAGIC, 0);
      iicReadData(&magic, 1);
      if(magic == 0xad) {
        bufSize = textSize/4;
        currentBuf = calloc(bufSize, sizeof(uint64_t));

        if(currentBuf) {
          ticksBuf = calloc(bufSize, sizeof(uint64_t));

          if(ticksBuf) {
            pcStart = textStart;
            unknownCurrent = 0;
            unknownTicks = 0;

            /* printf("Magic: %x\n", magic); */
            /* for(int i = 0; i < 65536; i++) { */
            /*   iicReadData(&magic, 1); */
            /*   if(magic != 0xad) { */
            /*     printf("I2C ERROR\n"); */
            /*     return 1; */
            /*   } */
            /* } */
            /* printf("IIC OK\n"); */

            iicSetCommand(I2C_READ_CURRENT, 0x2);
            {
              int16_t current;
              iicReadData((uint8_t*)&current, 2);
            }

            if(setupTimer()) {
              printf("PROFILER: Started, profiling between %p and %p\n", (void*)pcStart, (void*)textSize);
              profilerActive = true;
              return true;
            } else printf("PROFILER: Can't init timer\n");
          } else printf("PROFILER: Can't allocate ticksBuffer\n");
        } else printf("PROFILER: Can't allocate currentBuffer\n");
      } else printf("PROFILER: Can't read I2C\n");
    } else printf("PROFILER: Can't setup I2C\n");
  } else printf("PROFILER: Can't setup interrupts\n");

  profilerActive = false;
  return false;
}

void stopProfiler(void) {
  if(profilerActive) {
    Xil_ExceptionDisable();

    FILE *fp = fopen("profile.pro", "w");
    if(fp == NULL) {
      printf("PROFILER: Can't open file\n");
      return;
    }

    uint8_t core = 0;
    uint8_t sensor = 1;

    double unknownCurrentAvg = unknownCurrent / (double)unknownTicks;
    double unknownRuntime = unknownTicks * pcSamplerPeriod;

    fwrite(&core, sizeof(uint8_t), 1, fp);
    fwrite(&sensor, sizeof(uint8_t), 1, fp);
    fwrite(&pcSamplerPeriod, sizeof(double), 1, fp);
    fwrite(&unknownCurrentAvg, sizeof(double), 1, fp);
    fwrite(&unknownRuntime, sizeof(double), 1, fp);

    for(int i = 0; i < bufSize; i++) {
      if((currentBuf[i] != 0) || (ticksBuf[i] != 0)) {
        uint64_t pc = (uint64_t)i * (uint64_t)4 + pcStart;
        double current = currentBuf[i] / (double)ticksBuf[i];
        double runtime = ticksBuf[i] * pcSamplerPeriod;

        fwrite(&pc, sizeof(uint64_t), 1, fp);
        fwrite(&current, sizeof(double), 1, fp);
        fwrite(&runtime, sizeof(double), 1, fp);
      }
    }

    fclose(fp);

    printf("PROFILER: Done\n");
  }
}
