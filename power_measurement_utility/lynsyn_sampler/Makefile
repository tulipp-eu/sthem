CFLAGS = -g -O2 -Wall -std=gnu99 -I/usr/include/libusb-1.0/
LDFLAGS = -lusb-1.0

CC = gcc
LD = gcc

lynsyn_sampler : main.o
	${LD} $^ ${LDFLAGS} -o $@

%.o : %.c
	${CC} ${CFLAGS} -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o lynsyn_sampler
