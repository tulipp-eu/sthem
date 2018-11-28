CC = aarch64-none-elf-gcc
AR = aarch64-none-elf-ar

CFLAGS = -I../../../sthem-benchmarks/libs -I/opt/Xilinx/SDx/2017.4/target/aarch64-none/include/

libtulipp.a : gmon.o profiler.o
	${AR} cr $@ *.o

gmon.o : gmon.c gmon.h
	${CC} ${CFLAGS} $< -c -o $@

profiler.o : profiler.c profiler.h
	${CC} ${CFLAGS} $< -c -o $@

.PHONY : clean
clean :
	rm -rf *.o *.a
