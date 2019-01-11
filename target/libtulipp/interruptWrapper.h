#ifndef _INTERRUPT_WRAPPER_H
#define _INTERRUPT_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <xscugic.h>

#define INTERRUPT_CPU7 XSCUGIC_SPI_CPU7_MASK
#define INTERRUPT_CPU6 XSCUGIC_SPI_CPU6_MASK
#define INTERRUPT_CPU5 XSCUGIC_SPI_CPU5_MASK
#define INTERRUPT_CPU4 XSCUGIC_SPI_CPU4_MASK
#define INTERRUPT_CPU3 XSCUGIC_SPI_CPU3_MASK
#define INTERRUPT_CPU2 XSCUGIC_SPI_CPU2_MASK
#define INTERRUPT_CPU1 XSCUGIC_SPI_CPU1_MASK
#define INTERRUPT_CPU0 XSCUGIC_SPI_CPU0_MASK

#define SGI_INTERRUPT_ID0			0x0F
#define SGI_INTERRUPT_ID1			0x0E
#define SGI_INTERRUPT_ID2			0x0D
#define SGI_INTERRUPT_ID3			0x0C
#define SGI_INTERRUPT_ID4			0x0B
#define SGI_INTERRUPT_ID5			0x0A
#define SGI_INTERRUPT_ID6			0x09
#define SGI_INTERRUPT_ID7			0x08
#define SGI_INTERRUPT_ID8			0x07
#define SGI_INTERRUPT_ID9			0x06
#define SGI_INTERRUPT_ID10			0x05
#define SGI_INTERRUPT_ID11			0x04
#define SGI_INTERRUPT_ID12			0x03
#define SGI_INTERRUPT_ID13			0x02
#define SGI_INTERRUPT_ID14			0x01
#define SGI_INTERRUPT_ID15			0x00

void disableInterrupts(void);
bool interruptCpu(u32 const cpu_mask, u32 const interrupt_id);
bool setupInterrupts(void);
bool setupTimerInterrupts(double const period, Xil_ExceptionHandler callback);
bool setupSoftwareInterrupts(u32 interrupt_id, Xil_ExceptionHandler callback);

#endif
