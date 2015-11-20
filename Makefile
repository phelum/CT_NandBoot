#CC = gcc
CC = g++
CFLAGS = -g -O0 -Wall -Wextra
#CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200112L
CFLAGS += -Iinclude/

TOOLS = bootfix

.PHONY: all clean

all: $(TOOLS)

clean:
	@rm -vf $(TOOLS)

gzip:
	tar -acf /mnt/CT_NandBoot.tar.gz *

$(TOOLS): Makefile

LIBUSB = libusb-1.0
LIBUSB_CFLAGS = `pkg-config --cflags $(LIBUSB)`
LIBUSB_LIBS = `pkg-config --libs $(LIBUSB)`

bootfix: bootfix.cpp bootfix.h usbfel.inc usblib.inc nand_part.h nand_part.inc
	$(CC) $(CFLAGS) $(LIBUSB_CFLAGS) $(LDFLAGS) -o $@ $(filter %.cpp,$^) $(LIBS) $(LIBUSB_LIBS)


.gitignore: Makefile
	@for x in $(TOOLS) '*.o' '*.swp'; do \
		echo "$$x"; \
	done > $@
