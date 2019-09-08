CC = gcc

all: pmapelf

pmapelf: pmapelf.c
	$(CROSS_COMPILE)$(CC) $(FLAGS) $(DEFINES) -O3 $< -o $@ $(LINKING)

clean:
	rm -Rf pmapelf
