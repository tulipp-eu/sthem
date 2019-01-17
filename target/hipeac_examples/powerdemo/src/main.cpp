/******************************************************************************
 *
 * HiPEAC TULIPP Tutorial Example
 *
 * January 2019
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdint.h>

#include <tulipp.h>

/*****************************************************************************/
/* Defines */

#define TESTS 10

#define TIMES 4
#define LOW_POWER 3000000
#define HIGH_POWER 2000000

/*****************************************************************************/
/* Variables */

/*****************************************************************************/
/* Functions */

uint64_t *testBuf;

void __attribute__ ((noinline)) highPower(void) {
  for(unsigned j = 0; j < TIMES; j++) {
    uint64_t *dst = testBuf;

    for(unsigned i = 0; i < HIGH_POWER; i++) {
      *dst++ = i;
    }
  }
}

void __attribute__ ((noinline)) lowPower(void) {
  for(unsigned i = 0; i < (TIMES * LOW_POWER); i++) {
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
  }
}

int main(void) {
  printf("Starting TULIPP example\n");

  tulippStart();

  for(int i = 0; i < TESTS; i++) {
    highPower();
    lowPower();
    tulippFrameDone();
  }

  tulippEnd();

  printf("TULIPP example done\n");

  return 0;
}

