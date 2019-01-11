#ifndef _AMP_H
#define _AMP_H

#include <sleep.h>
#include <xil_mmu.h>

#ifndef MYSELF
#error "MYSELF is not defined! Define MYSELF to the AMP rank (CPU0,CPU1,...)"
#endif

//Some preprocessor magic...
#define _GET_MACRO2(_1,_2,NAME,...) NAME
#define OWN(...) 			_GET_MACRO2(__VA_ARGS__, _OWN2, _OWN1)(__VA_ARGS__)
#define WAIT(...) 			_GET_MACRO2(__VA_ARGS__, _WAIT2, _WAIT1)(__VA_ARGS__)

#define COMM_ADDR_ROOT		0x3FE00000
#define COMM_ADDR(x)		COMM_ADDR_ROOT + (x * sizeof(unsigned long))

#define _WAIT1(x) 			_WAIT2(x,MYSELF)
#define _OWN1(x) 			_OWN2(x,MYSELF)
#define _WAIT2(x,y) 		while(x != y) {asm("nop");}
#define _OWN2(x,y) 			x = y
#define RELEASE(x,y) 		x = y
#define INIT()				Xil_SetTlbAttributes(COMM_ADDR_ROOT, DEVICE_MEMORY)

//No need to set this to device memory, as it will be done through INIT() in 2Mb blocks
#define COMM_DATA_ADDR(x)	(COMM_ADDR_ROOT + 0x10000 + (x * sizeof(unsigned long)))

#define CPU0 	0
#define CPU1 	1
#define CPU2 	2
#define CPU3 	3

#endif
