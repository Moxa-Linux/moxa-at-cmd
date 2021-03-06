CC ?= gcc
CFLAGS ?= -Wall
LDFLAGS ?=
DESTDIR ?= /

all: main.o at_lib.o at-parser.o
	$(CC) $(CFLAGS) main.o at_lib.o at-parser.o -o mxat

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install:
	mkdir -p $(DESTDIR)/usr/sbin/
	cp mxat $(DESTDIR)/usr/sbin/

.PHONY: clean
clean:
	rm -f *.o mxat
