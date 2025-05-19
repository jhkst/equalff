PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

TARGET=equalff
LIBS=
CC=gcc
CFLAGS=-g -Wall -std=gnu99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE

.PHONY: default all clean install uninstall

default: $(TARGET)
all: default

HEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TAGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $@

clean:
	-rm -f *.o 
	-rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)
	cp $(TARGET) $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)
	cp equalff.man $(DESTDIR)$(MANDIR)/$(TARGET).1
	chmod 644 $(DESTDIR)$(MANDIR)/$(TARGET).1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(MANDIR)/$(TARGET).1
