#include "interruptWrapper.h"

#include <xparameters.h>
#include <xstatus.h>
#include <xil_exception.h>
#include <xttcps.h>
#include <xscugic.h>
#include <xiicps.h>
#include <xgpiops.h>
#include <xil_cache.h>

static unsigned long numberOfTtcInt;
static unsigned long numberOfSGInt;

static XScuGic interruptController;

typedef struct timerCallback {
	XTtcPs timer;
	Xil_ExceptionHandler callback;
} timerCallback;

timerCallback timerReference;

#define TTC_TICK_DEVICE_ID 	XPAR_XTTCPS_1_DEVICE_ID
#define TTC_TICK_INTR_ID   	XPAR_XTTCPS_1_INTR
#define INTC_DEVICE_ID     	XPAR_SCUGIC_SINGLE_DEVICE_ID

#define SIG_INTR_ID			0x0E

static bool interruptsActive = false;


void _timerInterruptHandler(void *CallBackRef) {
	timerCallback *timerRef = (timerCallback *) CallBackRef;
	uint32_t StatusEvent = XTtcPs_GetInterruptStatus(&(timerRef->timer));
	XTtcPs_ClearInterruptStatus(&(timerRef->timer), StatusEvent);

	if((XTTCPS_IXR_INTERVAL_MASK & StatusEvent) != 0) {
		numberOfTtcInt++;
		timerRef->callback((void *) &(timerRef->timer));
	}
}

bool setupTimerInterrupts(double const pcSamplerPeriod, Xil_ExceptionHandler callback) {
	int status;
	XTtcPs_Config *config;
	XTtcPs *timer = &(timerReference.timer);

	XInterval interval = 0;
	u8 prescaler = 0;

	if (!interruptsActive && !setupInterrupts()) {
		return false;
	}

	config = XTtcPs_LookupConfig(TTC_TICK_DEVICE_ID);
	if(config == NULL) {
		return false;
	}

	status = XTtcPs_CfgInitialize(timer, config, config->BaseAddress);
	if(status != XST_SUCCESS) {
		return false;
	}

	XTtcPs_SetOptions(timer, XTTCPS_OPTION_INTERVAL_MODE | XTTCPS_OPTION_WAVE_DISABLE);

	XTtcPs_CalcIntervalFromFreq(timer, 1 / pcSamplerPeriod, &interval, &prescaler);

	XTtcPs_SetInterval(timer, interval);
	XTtcPs_SetPrescaler(timer, prescaler);

	status = XScuGic_Connect(&interruptController,
			TTC_TICK_INTR_ID,
			(Xil_ExceptionHandler) _timerInterruptHandler,
			(void *)&timerReference);

	if(status != XST_SUCCESS) {
		return false;
	}

	timerReference.callback = callback;
	numberOfTtcInt = 0;

	XScuGic_Enable(&interruptController, TTC_TICK_INTR_ID);

	XTtcPs_EnableInterrupts(timer, XTTCPS_IXR_INTERVAL_MASK);

	XTtcPs_Start(timer);

	return true;
}

bool setupSoftwareInterrupts(u32 const interrupt_id, Xil_ExceptionHandler callback) {
	int status;

	if (!interruptsActive && !setupInterrupts()) {
		return false;
	}

	status = XScuGic_Connect(&interruptController,
			interrupt_id,
			callback,
			(void *)&interruptController);

	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	numberOfSGInt = 0;

	XScuGic_Enable(&interruptController, interrupt_id);

	return true;
}

bool setupInterrupts(void) {
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
  interruptsActive = true;
  return true;
}

bool interruptCpu(u32 const cpu_mask, u32 const interrupt_id) {

	int status;

	if (!interruptsActive && !setupInterrupts()) {
		return false;
	}

	status = XScuGic_SoftwareIntr(&interruptController,
					interrupt_id,
					cpu_mask);

	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return true;
}


void disableInterrupts() {
	Xil_ExceptionDisable();
}

void statistics() {
	printf("Number of software generated interrupts: %lu\n", numberOfSGInt);
	printf("Number of timer generated interrupts: %lu\n", numberOfTtcInt);
}
