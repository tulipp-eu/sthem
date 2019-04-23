CC = gcc

all: pmapelf

pmapelf: pmapelf.c
	$(CROSS_COMPILE)$(CC) -O3 $< -o $@

clean:
	rm -Rf pmapelf
