CC = aarch64-hipperos-gcc
AR = aarch64-hipperos-ar

HIPPEROS_DIR=/home/djupdal/research/tulipp/hipperos_18.10_tulipp_te0820_te0820_elf_noLogging/

CFLAGS = -isystem ${HIPPEROS_DIR}/hipperos-18.10-te0820_elf_noLogging/libs/hapi/include -isystem ${HIPPEROS_DIR}/hipperos-18.10-te0820_elf_noLogging/kernel/include -isystem ${HIPPEROS_DIR}/hipperos-18.10-te0820_elf_noLogging/libs/fatfs/include/ -isystem ${HIPPEROS_DIR}/hipperos-18.10-te0820_elf_noLogging/libs/drivers/include/sdmmc/

libhtulipp.a : gmon.o profiler.o 
	${AR} cr $@ *.o

gmon.o : gmon.c gmon.h
	${CC} ${CFLAGS} $< -c -o $@

profiler.o : profiler.c profiler.h
	${CC} ${CFLAGS} $< -c -o $@

.PHONY : clean
clean :
	rm -rf *.o *.a
