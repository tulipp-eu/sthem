/*-
 * Copyright (c) 1983, 1992, 1993
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This file is taken from Cygwin distribution. Please keep it in sync.
 * The differences should be within __MINGW32__ guard.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "gmon.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <ff.h>

#ifdef HIPPEROS
#include <SdMmc.h>
#include <hipperos/hstdio.h>
#endif

#ifdef HIPPEROS
#define printf h_printf
#endif

#define MINUS_ONE_P (-1)
#define bzero(ptr,size) memset (ptr, 0, size);
#define ERR(s) write(2, s, sizeof(s))

struct gmonparam _gmonparam = { GMON_PROF_OFF, NULL, 0, NULL, 0, NULL, 0, 0L, 0, 0, 0};
static char already_setup = 0; /* flag to indicate if we need to init */
static int    s_scale;
/* see profil(2) where this is described (incorrectly) */
#define        SCALE_1_TO_1    0x10000L

static void moncontrol(int mode);

static FATFS fatfs;

void monstartup (size_t lowpc, size_t highpc) {
    register size_t o;
    char *cp;
    struct gmonparam *p = &_gmonparam;

    /*
     * round lowpc and highpc to multiples of the density we're using
     * so the rest of the scaling (here and in gprof) stays in ints.
     */
    p->lowpc = ROUNDDOWN(lowpc, HISTFRACTION * sizeof(HISTCOUNTER));
    p->highpc = ROUNDUP(highpc, HISTFRACTION * sizeof(HISTCOUNTER));
    p->textsize = p->highpc - p->lowpc;
    p->kcountsize = p->textsize / HISTFRACTION;
    p->fromssize = p->textsize / HASHFRACTION;
    p->tolimit = p->textsize * ARCDENSITY / 100;
    if (p->tolimit < MINARCS) {
        p->tolimit = MINARCS;
    } else if (p->tolimit > MAXARCS) {
        p->tolimit = MAXARCS;
    }
    p->tossize = p->tolimit * sizeof(struct tostruct);

    cp = malloc(p->kcountsize + p->fromssize + p->tossize);
    if (cp == (char *)MINUS_ONE_P) {
        ERR("monstartup: out of memory\n");
        return;
    }

    /* zero out cp as value will be added there */
    bzero(cp, p->kcountsize + p->fromssize + p->tossize);

    p->tos = (struct tostruct *)cp;
    cp += p->tossize;
    p->kcount = (u_short *)cp;
    cp += p->kcountsize;
    p->froms = (u_short *)cp;

    p->tos[0].link = 0;

    o = p->highpc - p->lowpc;
    if (p->kcountsize < o) {
#ifndef notdef
        s_scale = ((float)p->kcountsize / o ) * SCALE_1_TO_1;
#else /* avoid floating point */
        int quot = o / p->kcountsize;

        if (quot >= 0x10000)
            s_scale = 1;
        else if (quot >= 0x100)
            s_scale = 0x10000 / quot;
        else if (o >= 0x800000)
            s_scale = 0x1000000 / (o / (p->kcountsize >> 8));
        else
            s_scale = 0x1000000 / ((o << 8) / p->kcountsize);
#endif
    } else {
        s_scale = SCALE_1_TO_1;
    }

    p->loopcounts = malloc((p->textsize/4) * sizeof(uint64_t));
    if(!p->loopcounts) {
      ERR("monstartup: out of memory\n");
      return;
    }
    memset(p->loopcounts, 0, (p->textsize/4) * sizeof(uint64_t));

    moncontrol(1); /* start */
}

bool _mcleanup(unsigned core) {
    static const char gmon_out[] = SDDRIVE "tulipp.gmn";
    int hz;
    int fromindex;
    int endfrom;
    size_t frompc;
    int toindex;
    struct rawarc rawarc;
    struct gmonparam *p = &_gmonparam;
    struct gmonhdr gmonhdr, *hdr;
    const char *proffile;
    if (p->state != GMON_PROF_ON) {
    	return false;
    }

    if (p->state == GMON_PROF_ERROR) {
        ERR("_mcleanup: tos overflow\n");
    }

    hz = 0;
    moncontrol(0); /* stop */
    proffile = gmon_out;

#ifdef HIPPEROS
    sdmmc_hostInit();
#endif

		FRESULT res = f_mount(&fatfs, SDDRIVE, 1);
		if(res != FR_OK) {
			printf("CALLTRACER: Could not mount (%d)\n", res);
      return false;
		}

    FIL fp;
    int mode = FA_OPEN_ALWAYS | FA_CREATE_ALWAYS | FA_WRITE;
    res = f_open(&fp, proffile, mode);
    if(res != FR_OK) {
			printf("CALLTRACER: Could not open (%d)\n", res);
      return false;
    }

    hdr = (struct gmonhdr *)&gmonhdr;
    hdr->lpc = p->lowpc;
    hdr->hpc = p->highpc;
    hdr->ncnt = sizeof(gmonhdr);
    hdr->version = GMONVERSION;
    hdr->core = core;
    hdr->profrate = hz;

    hdr->loops = 0;
    for(int i = 0; i < (p->textsize/4); i++) {
      if(p->loopcounts[i]) {
        hdr->loops++;
      }
    }

    UINT bw;

    f_write(&fp, (char *)hdr, sizeof *hdr, &bw);

    for(int i = 0; i < (p->textsize/4); i++) {
      if(p->loopcounts[i]) {
        uint64_t pc = (i*4) + p->lowpc;
        f_write(&fp, &pc, sizeof(uint64_t), &bw);
        f_write(&fp, &(p->loopcounts[i]), sizeof(uint64_t), &bw);
      }
    }

    endfrom = p->fromssize / sizeof(*p->froms);
    for (fromindex = 0; fromindex < endfrom; fromindex++) {
        if (p->froms[fromindex] == 0) {
            continue;
        }
        frompc = p->lowpc;
        frompc += fromindex * HASHFRACTION * sizeof(*p->froms);
        for (toindex = p->froms[fromindex]; toindex != 0; toindex = p->tos[toindex].link) {
            rawarc.raw_frompc = frompc;
            rawarc.raw_selfpc = p->tos[toindex].selfpc;
            rawarc.raw_count = p->tos[toindex].count;
            f_write(&fp, &rawarc, sizeof rawarc, &bw);
        }
    }

    f_close(&fp);

    return true;
}

/*
 * Control profiling
 *    profiling is what mcount checks to see if
 *    all the data structures are ready.
 */
static void moncontrol(int mode) {
    struct gmonparam *p = &_gmonparam;

    if (mode) {
        /* start */
        //profil((char *)p->kcount, p->kcountsize, p->lowpc, s_scale);
        p->state = GMON_PROF_ON;
    } else {
        /* stop */
        //profil((char *)0, 0, 0, 0);
        p->state = GMON_PROF_OFF;
    }
}

void _mcount_internal(u_short *frompcindex, u_short *selfpc) {
  register struct tostruct    *top;
  register struct tostruct    *prevtop;
  register long            toindex;
  struct gmonparam *p = &_gmonparam;

  if (!already_setup) {
    already_setup = 1;
#ifdef HIPPEROS
    extern char __init_start;
    extern char _rodata_start;
    monstartup((size_t)&__init_start, (size_t)&_rodata_start);
#else
    extern char __rodata_start;
    monstartup(0x0, (size_t)&__rodata_start);
#endif
  }
  /*
   *    check that we are profiling
   *    and that we aren't recursively invoked.
   */
  if (p->state!=GMON_PROF_ON) {
    goto out;
  }
  p->state++;
  /*
   *    check that frompcindex is a reasonable pc value.
   *    for example:    signal catchers get called from the stack,
   *            not from text space.  too bad.
   */
  frompcindex = (u_short*)((long)frompcindex - (long)p->lowpc);
  if ((unsigned long)frompcindex > p->textsize) {
      goto done;
  }
  frompcindex = (u_short*)&p->froms[((long)frompcindex) / (HASHFRACTION * sizeof(*p->froms))];
  toindex = *frompcindex;
  if (toindex == 0) {
    /*
    *    first time traversing this arc
    */
    toindex = ++p->tos[0].link; /* the link of tos[0] points to the last used record in the array */
    if (toindex >= p->tolimit) { /* more tos[] entries than we can handle! */
        goto overflow;
      }
    *frompcindex = toindex;
    top = &p->tos[toindex];
    top->selfpc = (size_t)selfpc;
    top->count = 1;
    top->link = 0;
    goto done;
  }
  top = &p->tos[toindex];
  if (top->selfpc == (size_t)selfpc) {
    /*
     *    arc at front of chain; usual case.
     */
    top->count++;
    goto done;
  }
  /*
   *    have to go looking down chain for it.
   *    top points to what we are looking at,
   *    prevtop points to previous top.
   *    we know it is not at the head of the chain.
   */
  for (; /* goto done */; ) {
    if (top->link == 0) {
      /*
       *    top is end of the chain and none of the chain
       *    had top->selfpc == selfpc.
       *    so we allocate a new tostruct
       *    and link it to the head of the chain.
       */
      toindex = ++p->tos[0].link;
      if (toindex >= p->tolimit) {
        goto overflow;
      }
      top = &p->tos[toindex];
      top->selfpc = (size_t)selfpc;
      top->count = 1;
      top->link = *frompcindex;
      *frompcindex = toindex;
      goto done;
    }
    /*
     *    otherwise, check the next arc on the chain.
     */
    prevtop = top;
    top = &p->tos[top->link];
    if (top->selfpc == (size_t)selfpc) {
      /*
       *    there it is.
       *    increment its count
       *    move it to the head of the chain.
       */
      top->count++;
      toindex = prevtop->link;
      prevtop->link = top->link;
      top->link = *frompcindex;
      *frompcindex = toindex;
      goto done;
    }
  }
  done:
    p->state--;
    /* and fall through */
  out:
    return;        /* normal return restores saved registers */
  overflow:
    p->state++; /* halt further profiling */
    #define    TOLIMIT    "mcount: tos overflow\n"
    write (2, TOLIMIT, sizeof(TOLIMIT));
  goto out;
}

void _monInit(void) {
  _gmonparam.state = GMON_PROF_OFF;
  already_setup = 0;
}

///////////////////////////////////////////////////////////////////////////////

void __tulipp_func_enter (void *this_fn, void *call_site) {
  _mcount_internal(call_site, this_fn);
}
 
void __tulipp_loop_header (void *loopAddr) {
}
 
void __tulipp_loop_body (void *loopAddr) {
  struct gmonparam *p = &_gmonparam;

  if (p->state == GMON_PROF_ON) {
    struct gmonparam *p = &_gmonparam;

    if(((size_t)loopAddr >= p->lowpc) && ((size_t)loopAddr <= p->highpc)) {
      unsigned index = ((size_t)loopAddr - p->lowpc)/4;
      p->loopcounts[index]++;
    }
  }
}
 
void __tulipp_exit(void) {
#ifdef HIPPEROS
  unsigned core = ~0; // FIXME
#else
#ifdef AARCH64
  uint64_t mpidr;
  asm ("mrs %0, MPIDR_EL1\n":"=r"(mpidr)::);
  unsigned core = mpidr & 0xff;
#else
  unsigned core = ~0; // FIXME
#endif
#endif

  if(_mcleanup(core)) {
    printf("CALLTRACER: Call trace file written\n");
  }
}

void __tulipp_init(void) {
  printf("CALLTRACER: init\n"); 
  _monInit();
  atexit(__tulipp_exit);
}
