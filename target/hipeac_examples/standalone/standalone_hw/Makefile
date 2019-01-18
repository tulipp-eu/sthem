###################################################
# Makefile for the HiPEAC TULIPP bare metal example
# 2019
###################################################

CC = tulippcc
CPP = tulipp++
LD = sds++ ${SDSFLAGS}
AS = aarch64-none-elf-as

export AS

XILINX_DIR = /opt/Xilinx
XILINX_VERSION = 2018.2

PLATFORM_REPO = /opt/Xilinx/SDx/default/custom-platforms

#PLATFORM = ${PLATFORM_REPO}/TE0820_3EG_1E_2018_2
PLATFORM = ${PLATFORM_REPO}/TE0820_4EV_1E_2018_2

STHEM = /opt/sthem

SDWHW = -sds-hw hwMedian src/filters.cpp -clkid 0 -sds-end

SDSFLAGS = -sds-pf ${PLATFORM} -target-os standalone -dmclkid 0 -sds-sys-config Standalone -sds-proc psu_cortexa53_0 $(SDSHW)

FLAGS = -DFATFS_MOUNT='\"1:/\"' \
	-I${STHEM}/target/SDSoC-Image-Processing-Library/ \
	-I${STHEM}/target/libtulipp

CPPFLAGS = ${FLAGS} \
  -I ${XILINX_DIR}/SDx/${XILINX_VERSION}/target/aarch64-none/include \
  -I ${XILINX_DIR}/Vivado/${XILINX_VERSION}/include \
  -I ${XILINX_DIR}/SDK/${XILINX_VERSION}/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/include/c++/7.2.1 \
  -I ${XILINX_DIR}/SDK/${XILINX_VERSION}/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/include/c++/7.2.1/aarch64-none-elf \
  -I ${XILINX_DIR}/SDK/${XILINX_VERSION}/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/include/c++/7.2.1/backward \
  -I ${XILINX_DIR}/SDK/${XILINX_VERSION}/gnu/aarch64/lin/aarch64-none/lib/gcc/aarch64-none-elf/7.2.1/include \
  -I ${XILINX_DIR}/SDK/${XILINX_VERSION}/gnu/aarch64/lin/aarch64-none/lib/gcc/aarch64-none-elf/7.2.1/include-fixed \
  -I ${XILINX_DIR}/SDK/${XILINX_VERSION}/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/include \
  -I ${XILINX_DIR}/SDK/${XILINX_VERSION}/gnu/aarch64/lin/aarch64-none/aarch64-none-elf/libc/usr/include

LDFLAGS = -L${STHEM}/target/libtulipp -ltulipp -lm

standalone.elf : main.o filters.o ppm.o __tulipp__.o
	${LD} ${LDFLAGS} $^ -o $@

__tulipp__.c :
	echo "/* TULIPP dummy file */" > __tulipp__.c

__tulipp__.o : __tulipp__.c
	sdscc ${SDSFLAGS} $< -c -o $@

ppm.o : src/ppm_library/ppm.c
	${CPP} ${CPPFLAGS} $< -c -o $@

filters.o : src/filters.cpp
	${CPP} ${CPPFLAGS} $< -c -o $@

main.o : src/main.cpp
	${CPP} ${CPPFLAGS} $< -c -o $@

.PHONY : clean
clean :
	rm -rf *.ll *.xml *.s *.o *.elf *.bit .Xil sd_card _sds __tulipp__*
