CC = gcc

TARGET = intrvelf

LINKING = -lrt
LINKING += -lusb-1.0
FLAGS=-O3

INCLUDES = -I/usr/include/libusb-1.0/

all: $(TARGET)

$(TARGET): intrvelf.o lynsyn.o
	$(CROSS_COMPILE)$(CC) $(FLAGS) $(INCLUDES) $(DEFINES) $^ -o $@ $(LINKING)

intrvelf.o: intrvelf.c
	$(CROSS_COMPILE)$(CC) $(FLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

lynsyn.o: lynsyn.c
	$(CROSS_COMPILE)$(CC) $(FLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

clean:
	rm -Rf $(TARGET)
	rm -Rf intrvelf.o
	rm -Rf lynsyn.o
