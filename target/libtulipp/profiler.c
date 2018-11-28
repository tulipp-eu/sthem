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
#include <xgpiops.h>
#include <xil_cache.h>

#include <sds_lib.h>

#include <compatibility_layer.h>
#include <gmon.h>

#include "interruptWrapper.h"
#define MYSELF 	CPU1
#include "amp.h"

#define SHARED_PC			(*(volatile unsigned long *)(COMM_DATA_ADDR(0)))
#define LOCK_INT			(*(volatile unsigned long *)(COMM_ADDR(1)))

#include "profiler.h"

#define ARMV8_PMCR_E            (1 << 0) /* Enable all counters */
#define ARMV8_PMCR_P            (1 << 1) /* Reset all counters */
#define ARMV8_PMCR_C            (1 << 2) /* Cycle counter reset */

#define ARMV8_PMUSERENR_EN      (1 << 0) /* EL0 access enable */
#define ARMV8_PMUSERENR_CR      (1 << 2) /* Cycle counter read enable */
#define ARMV8_PMUSERENR_ER      (1 << 3) /* Event counter read enable */

#define ARMV8_PMCNTENSET_EL0_EN (1 << 31) /* Performance Monitors Count Enable Set register */

static XGpioPs Gpio;

#define TTC_TICK_DEVICE_ID XPAR_XTTCPS_1_DEVICE_ID
#define TTC_TICK_INTR_ID   XPAR_XTTCPS_1_INTR
#define INTC_DEVICE_ID     XPAR_SCUGIC_SINGLE_DEVICE_ID

#define I2C_NO_CMD               0
#define I2C_MAGIC                1
#define I2C_READ_CURRENT_AVG     2
#define I2C_READ_CURRENT_INSTANT 3
#define I2C_GET_CAL              4

#define IIC_DEVICE_ID   XPAR_XIICPS_1_DEVICE_ID
#define IIC_SLAVE_ADDR     0x55
#define IIC_SCLK_RATE    100000

#define GPIO_DEVICE_ID XPAR_XGPIOPS_0_DEVICE_ID

#define OUTPUT_PIN 78

static XIicPs Iic;
static XScuGic interruptController;
static XTtcPs timer;

static uint64_t *currentBuf;
static uint64_t *ticksBuf;

static uint64_t unknownCurrent;
static uint64_t unknownTicks;

static uint64_t pcStart;
static uint32_t bufSize;

static volatile bool profilerActive;

static double pcSamplerPeriod;

static bool iicReadData(uint8_t *recBuf, int size);

static void timerInterruptHandler(void *CallBackRef) {
  uint32_t StatusEvent = XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
  XTtcPs_ClearInterruptStatus((XTtcPs *)CallBackRef, StatusEvent);

  if(profilerActive) {
    int16_t current;
    iicReadData((uint8_t*)&current, 2);

    RELEASE(LOCK_INT, CPU0);
    interruptCpu(INTERRUPT_CPU0, SGI_INTERRUPT_ID0);
    WAIT(LOCK_INT);

    uint64_t pc = SHARED_PC;

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

static void timerInterruptHandlerSingleCore(void *CallBackRef) {
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

bool setupTimerInterruptsSingleCore(double const period, Xil_ExceptionHandler callback) {
  { // interrupts
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

  }

  { // timer
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

    XTtcPs_CalcIntervalFromFreq(&timer, 1 / period, &interval, &prescaler);

    XTtcPs_SetInterval(&timer, interval);
    XTtcPs_SetPrescaler(&timer, prescaler);

    status = XScuGic_Connect(&interruptController, TTC_TICK_INTR_ID, callback, (void *)&timer);
    if(status != XST_SUCCESS) return false;

    XScuGic_Enable(&interruptController, TTC_TICK_INTR_ID);

    XTtcPs_EnableInterrupts(&timer, XTTCPS_IXR_INTERVAL_MASK);

    XTtcPs_Start(&timer);
  }

  return true;
}

uint64_t getCycleCounter(void) {
  uint64_t result = 0;
  asm volatile("mrs %0, pmccntr_el0" : "=r" (result));
  return result;
}

uint64_t getCounter(unsigned i) {
  uint64_t result = 0;
  switch(i) {
    case 0: asm volatile("mrs %0, pmevcntr0_el0" : "=r" (result)); break;
    case 1: asm volatile("mrs %0, pmevcntr1_el0" : "=r" (result)); break;
    case 2: asm volatile("mrs %0, pmevcntr2_el0" : "=r" (result)); break;
    case 3: asm volatile("mrs %0, pmevcntr3_el0" : "=r" (result)); break;
    case 4: asm volatile("mrs %0, pmevcntr4_el0" : "=r" (result)); break;
    case 5: asm volatile("mrs %0, pmevcntr5_el0" : "=r" (result)); break;
    default: result = -1; break;
  }
  return result;
}

void initPerfCounters(uint32_t pmuEvent0, uint32_t pmuEvent1, uint32_t pmuEvent2, uint32_t pmuEvent3, uint32_t pmuEvent4, uint32_t pmuEvent5) {
  // setup performance counters
  uint32_t value = 0;

  /* Enable Performance Counter */
  asm volatile("mrs %0, pmcr_el0" : "=r" (value));
  value |= ARMV8_PMCR_E; /* Enable */
  value |= ARMV8_PMCR_C; /* Cycle counter reset */
  value |= ARMV8_PMCR_P; /* Reset all counters */
  asm volatile("MSR PMCR_EL0, %0" : : "r" (value));

  /* Enable cycle counter register */
  asm volatile("mrs %0, pmcntenset_el0" : "=r" (value));
  value |= ARMV8_PMCNTENSET_EL0_EN;
  asm volatile("msr pmcntenset_el0, %0" : : "r" (value));

  /* Enable event counter register */
  asm volatile("isb");

  asm volatile("msr pmevtyper0_el0, %0" : : "r" (pmuEvent0));
  asm volatile("msr pmevtyper1_el0, %0" : : "r" (pmuEvent1));
  asm volatile("msr pmevtyper2_el0, %0" : : "r" (pmuEvent2));
  asm volatile("msr pmevtyper3_el0, %0" : : "r" (pmuEvent3));
  asm volatile("msr pmevtyper4_el0, %0" : : "r" (pmuEvent4));
  asm volatile("msr pmevtyper5_el0, %0" : : "r" (pmuEvent5));

  /* enable counters */
  uint32_t r = 0;
  asm volatile("mrs %0, pmcntenset_el0" : "=r" (r));
  asm volatile("msr pmcntenset_el0, %0" : : "r" (r|0x3f));
}

bool setupGpio(void) {
  XGpioPs_Config *ConfigPtr = XGpioPs_LookupConfig(GPIO_DEVICE_ID);

  int status = XGpioPs_CfgInitialize(&Gpio, ConfigPtr, ConfigPtr->BaseAddr);
  if(status != XST_SUCCESS) {
    printf("Can't initialize GPIO\n");
    return false;
  }

  XGpioPs_SetDirection(&Gpio, XGPIOPS_BANK3, 1);
  XGpioPs_Write(&Gpio, XGPIOPS_BANK3, 0x0);
  XGpioPs_SetOutputEnable(&Gpio, XGPIOPS_BANK3, 1);

  return true;
}

bool startProfiler(uint64_t textStart, uint64_t textSize, double period, bool dualCore) {
  // setup timer interrupt
  if(period > 0) {
    pcSamplerPeriod = period;

    if(setupIic()) {
      uint8_t magic;
      iicSetCommand(I2C_MAGIC, 0);
      iicReadData(&magic, 1);

      if(magic == 0xad) {
        bufSize = textSize/4;
        currentBuf = malloc(bufSize * sizeof(uint64_t));

        if(currentBuf) {
          memset(currentBuf, 0, bufSize * sizeof(uint64_t));
          ticksBuf = malloc(bufSize * sizeof(uint64_t));

          if(ticksBuf) {
            memset(ticksBuf, 0, bufSize * sizeof(uint64_t));

            pcStart = textStart;
            unknownCurrent = 0;
            unknownTicks = 0;

            if(dualCore) {
              iicSetCommand(I2C_READ_CURRENT_INSTANT, 0x2);
            } else {
              iicSetCommand(I2C_READ_CURRENT_AVG, 0x2);
              {
                int16_t current;
                iicReadData((uint8_t*)&current, 2);
              }
            }

            if(dualCore) {
              if(setupTimerInterrupts(period, timerInterruptHandler)) {
                printf("PROFILER: Started, profiling between %p and %p at %f Hz\n", (void*)pcStart, (void*)pcStart+textSize, 1/period);
                profilerActive = true;
                return true;
              } else printf("PROFILER: Can't init timer\n");
            } else {
              if(setupTimerInterruptsSingleCore(period, timerInterruptHandlerSingleCore)) {
                printf("PROFILER: Started, profiling between %p and %p at %f Hz\n", (void*)pcStart, (void*)pcStart+textSize, 1/period);
                profilerActive = true;
                return true;
              } else printf("PROFILER: Can't init timer\n");
            }
          } else printf("PROFILER: Can't allocate ticksBuf\n");
        } else printf("PROFILER: Can't allocate currentBuf\n");
      } else printf("PROFILER: Can't read I2C\n");
    } else printf("PROFILER: Can't setup I2C\n");

    profilerActive = false;
    return false;
  }

  return true;
}

void disablePerfCounters(void) {
  uint32_t value = 0;
  uint32_t mask = 0;

  /* Disable Performance Counter */
  asm volatile("mrs %0, pmcr_el0" : "=r" (value));
  mask = 0;
  mask |= ARMV8_PMCR_E; /* Enable */
  mask |= ARMV8_PMCR_C; /* Cycle counter reset */
  mask |= ARMV8_PMCR_P; /* Reset all counters */
  asm volatile("msr pmcr_el0, %0" : : "r" (value & ~mask));

  /* Disable cycle counter register */
  asm volatile("mrs %0, pmcntenset_el0" : "=r" (value));
  mask = 0;
  mask |= ARMV8_PMCNTENSET_EL0_EN;
  asm volatile("msr pmcntenset_el0, %0" : : "r" (value & ~mask));
}

void stopProfiler(char *filename) {
  if(profilerActive) {
    Xil_ExceptionDisable();
  }

  FILE *fp = fopen(filename, "w");
  if(fp == NULL) {
    printf("PROFILER: Can't open file\n");
    return;
  }

  printf("PROFILER: Writing to file %s\n", filename);

  iicSetCommand(I2C_GET_CAL, 0);
  double calData[14];
  iicReadData((uint8_t*)&calData, 14 * sizeof(double));

  uint8_t core = 0;
  uint8_t sensor = 1;

  double unknownCurrentAvg = unknownCurrent / (double)unknownTicks;
  double unknownRuntime = unknownTicks * pcSamplerPeriod;

  fwrite(&core, sizeof(uint8_t), 1, fp);
  fwrite(&sensor, sizeof(uint8_t), 1, fp);
  for(int i = 0; i < 14; i++) {
    fwrite(&(calData[i]), sizeof(double), 1, fp);
  }
  fwrite(&pcSamplerPeriod, sizeof(double), 1, fp);

  fwrite(&unknownCurrentAvg, sizeof(double), 1, fp);
  fwrite(&unknownRuntime, sizeof(double), 1, fp);

  uint32_t count = 0;
  for(int i = 0; i < bufSize; i++) {
    if((currentBuf[i] != 0) || (ticksBuf[i] != 0)) {
      count++;
    }
  }

  fwrite(&count, sizeof(uint32_t), 1, fp);

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

void profilerOn(void) {
  XGpioPs_WritePin(&Gpio, OUTPUT_PIN, 0x1);
}

void profilerOff(void) {
  XGpioPs_WritePin(&Gpio, OUTPUT_PIN, 0x0);
}
