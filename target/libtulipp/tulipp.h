#ifndef TULIPP_H
#define TULIPP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AARCH64

// PMU events that can be counted by the Cortex A53 core

#define PMU_EVENT_EXT_MEM_REQ 0xc0
#define PMU_EVENT_EXT_MEM_REQ_NC 0xc1
#define PMU_EVENT_PREFETCH_LINEFILL 0xc2
#define PMU_EVENT_PREFETCH_LINEFILL_DROP 0xc3
#define PMU_EVENT_READ_ALLOC_ENTER 0xc4
#define PMU_EVENT_READ_ALLOC 0xc5
#define PMU_EVENT_PRE_DECODE_ERR 0xc6
#define PMU_EVENT_STALL_SB_FULL 0xc7
#define PMU_EVENT_EXT_SNOOP 0xc8
#define PMU_EVENT_BR_COND 0xc9
#define PMU_EVENT_BR_INDIRECT_MISPRED 0xca
#define PMU_EVENT_BR_INDIRECT_MISPRED_ADDR 0xcb
#define PMU_EVENT_BR_COND_MISPRED 0xcc
#define PMU_EVENT_L1I_CACHE_ERR 0xd0
#define PMU_EVENT_L1D_CACHE_ERR 0xd1
#define PMU_EVENT_TLB_ERR 0xd2
#define PMU_EVENT_OTHER_IQ_DEP_STALL 0xe0
#define PMU_EVENT_IC_DEP_STALL 0xe1
#define PMU_EVENT_IUTLB_DEP_STALL 0xe2
#define PMU_EVENT_DECODE_DEP_STALL 0xe3
#define PMU_EVENT_OTHER_INTERLOCK_STALL 0xe4
#define PMU_EVENT_AGU_DEP_STALL 0xe5
#define PMU_EVENT_SIMD_DEP_STALL 0xe6
#define PMU_EVENT_LD_DEP_STALL 0xe7
#define PMU_EVENT_ST_DEP_STALL 0xe8
#define PMU_EVENT_EXC_FIQ 0x87 
#define PMU_EVENT_EXC_IRQ 0x86 
#define PMU_EVENT_BR_INDIRECT_SPEC 0x7A 
#define PMU_EVENT_BUS_ACCESS_ST 0x61 
#define PMU_EVENT_BUS_ACCESS_LD 0x60 
//#define PMU_EVENT_L1D_CACHE_ALLOCATE 0x1F 
#define PMU_EVENT_CHAIN 0x1E 
#define PMU_EVENT_BUS_CYCLES 0x1D 
//#define PMU_EVENT_TTBR_WRITE_RETIRED 0x1C 
//#define PMU_EVENT_INST_SPEC 0x1B 
#define PMU_EVENT_MEMORY_ERROR 0x1A 
#define PMU_EVENT_BUS_ACCESS 0x19 
#define PMU_EVENT_L2D_CACHE_WB 0x18 
#define PMU_EVENT_L2D_CACHE_REFILL 0x17 
#define PMU_EVENT_L2D_CACHE 0x16 
#define PMU_EVENT_L1D_CACHE_WB 0x15 
#define PMU_EVENT_L1I_CACHE 0x14 
#define PMU_EVENT_MEM_ACCESS 0x13 
#define PMU_EVENT_BR_PRED 0x12 
#define PMU_EVENT_CPU_CYCLES 0x11 
#define PMU_EVENT_BR_MIS_PRED 0x10 
#define PMU_EVENT_UNALIGNED_LDST_RETIRED 0x0F 
#define PMU_EVENT_BR_RETURN_RETIRED 0x0E 
#define PMU_EVENT_BR_IMMED_RETIRED 0x0D 
#define PMU_EVENT_PC_WRITE_RETIRED 0x0C 
#define PMU_EVENT_CID_WRITE_RETIRED 0x0B 
#define PMU_EVENT_EXC_RETURN 0x0A 
#define PMU_EVENT_EXC_TAKEN 0x09 
#define PMU_EVENT_INST_RETIRED 0x08 
#define PMU_EVENT_ST_RETIRED 0x07 
#define PMU_EVENT_LD_RETIRED 0x06 
#define PMU_EVENT_L1D_TLB_REFILL 0x05 
#define PMU_EVENT_L1D_CACHE 0x04 
#define PMU_EVENT_L1D_CACHE_REFILL 0x03 
#define PMU_EVENT_L1I_TLB_REFILL 0x02 
#define PMU_EVENT_L1I_CACHE_REFILL 0x01 
#define PMU_EVENT_SW_INCR 0x00 

/** Start timer interrupt based profiler */
bool startProfiler(uint64_t textStart, uint64_t textSize, double period, bool dualCore);
/** Stop timer interrupt based profiler and save profile to disk */
void stopProfiler(char *filename);

/** Setup GPIO support */
bool setupGpio(void);
/** Begin GPIO controlled Region-Of-Interest */
void profilerOn(void);
/** End GPIO controlled Region-Of-Interest */
void profilerOff(void);

/** Init performance counters */
void initPerfCounters(uint32_t pmuEvent0, uint32_t pmuEvent1, uint32_t pmuEvent2, uint32_t pmuEvent3, uint32_t pmuEvent4, uint32_t pmuEvent5);
/** Disable performance counters */
void disablePerfCounters(void);
/** Return current cycle counter */
uint64_t getCycleCounter(void);
/** Return value of given performance counter */
uint64_t getCounter(unsigned i);

/** Setup call tracer */
bool setupCallTracer(void);
/** End call tracer, write to file */
bool endCallTracer(unsigned core);

#endif

void __attribute__ ((noinline)) tulippStart(void);
void __attribute__ ((noinline)) tulippEnd(void);
void __attribute__ ((noinline)) tulippFrameDone(void);

#ifdef __cplusplus
}
#endif

#endif
