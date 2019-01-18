
#ifndef DEFHIPEACOS_H
#define DEFHIPEACOS_H

#include <stdarg.h>

#if defined(__hipperos__)

#include <hipperos/HipperosConfig.h>
#include <hipperos/hstdio.h>
#include <Board.h>
#include <SdMmc.h>

#define EXITFAIL(msg) \
    do { \
        h_printf((msg)); \
        return EXIT_FAILURE; \
    } while (0)

#define PRINTF(...) \
    do { \
        h_printf(__VA_ARGS__); \
    } while (0)

#define EPRINTF(...) \
    do { \
        h_printf(__VA_ARGS__); \
    } while (0)

#define FATFS_MOUNT ""

#else

#include <xstatus.h>
#include <xtime_l.h>
#include <xil_io.h>

#define EXITFAIL(msg) \
    do { \
        fprintf(stderr, (msg) "\n"); \
        return 1; \
    } while (0)

#define PRINTF(...) \
    do { \
        printf(...); \
    } while (0)

#define EPRINTF(...) \
    do { \
        fprintf(stderr, __VA_ARGS__);
    } while (0)

#define FATFS_MOUNT "1:/"

#endif

#endif /* DEFHIPEACOS_H */
