.PHONY : all
all : libtulipp.a libhtulipp.a

###############################################################################

CC = aarch64-none-elf-gcc
AR = aarch64-none-elf-ar

CFLAGS = -DSDDRIVE="\"1:/\"" -I/opt/Xilinx/SDx/2017.4/target/aarch64-none/include/

libtulipp.a : interruptWrapper.o gmon.o tulipp.o
	${AR} cr $@ $^

interruptWrapper.o : interruptWrapper.c
	${CC} ${CFLAGS} $< -c -o $@

gmon.o : gmon.c gmon.h
	${CC} ${CFLAGS} $< -c -o $@

tulipp.o : tulipp.c tulipp.h
	${CC} ${CFLAGS} $< -c -o $@

###############################################################################

HIPPEROS_CC = aarch64-hipperos-gcc
HIPPEROS_AR = aarch64-hipperos-ar

HIPPEROS_DIR=/home/djupdal/research/tulipp/hipperos_18.10_tulipp_te0820_te0820_elf_noLogging/

HIPPEROS_CFLAGS = -DSDDRIVE="\"\"" -DHIPPEROS -isystem ${HIPPEROS_DIR}/hipperos-18.10-te0820_elf_noLogging/libs/hapi/include -isystem ${HIPPEROS_DIR}/hipperos-18.10-te0820_elf_noLogging/kernel/include -isystem ${HIPPEROS_DIR}/hipperos-18.10-te0820_elf_noLogging/libs/fatfs/include/ -isystem ${HIPPEROS_DIR}/hipperos-18.10-te0820_elf_noLogging/libs/drivers/include/sdmmc/

###############################################################################

libhtulipp.a : gmon_hipperos.o tulipp_hipperos.o 
	${HIPPEROS_AR} cr $@ $^

gmon_hipperos.o : gmon.c gmon.h
	${HIPPEROS_CC} ${HIPPEROS_CFLAGS} $< -c -o $@

tulipp_hipperos.o : tulipp.c tulipp.h
	${HIPPEROS_CC} ${HIPPEROS_CFLAGS} $< -c -o $@

###############################################################################

.PHONY : clean
clean :
	rm -rf *.o *.a




