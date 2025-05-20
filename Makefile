PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

TARGET=equalff
# Update LIB_NAME for dynamic library (e.g., .so, .dylib)
# gcc on macOS will create .dylib even if .so is specified, which is fine.
LIB_NAME=libequalff.so
LIB_TARGET=lib/$(LIB_NAME)

CC=gcc
# Base CFLAGS
CFLAGS_BASE=-g -Wall -std=gnu99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
# CFLAGS for general compilation (CLI and includes)
CFLAGS=$(CFLAGS_BASE) -Ilib
# CFLAGS for library object files (add -fPIC)
LIB_CFLAGS=$(CFLAGS_BASE) -fPIC -Ilib

LDFLAGS=-Llib
LIBS=-lequalff

.PHONY: default all clean install uninstall libclean cliclean library

default: $(TARGET)
all: default
library: $(LIB_TARGET)

# Library specific files
LIB_SOURCES = $(wildcard lib/*.c)
LIB_OBJECTS = $(patsubst %.c,%.o,$(LIB_SOURCES))
LIB_HEADERS = $(wildcard lib/*.h)

# CLI specific files
CLI_SOURCES = $(wildcard cli/*.c)
CLI_OBJECTS = $(patsubst %.c,%.o,$(CLI_SOURCES))

# Rule to build library objects (e.g., lib/salloc.o from lib/salloc.c)
# Objects are placed in the lib/ directory. Use LIB_CFLAGS with -fPIC.
lib/%.o: lib/%.c $(LIB_HEADERS)
	$(CC) $(LIB_CFLAGS) -c $< -o $@

# Rule to build the dynamic library
$(LIB_TARGET): $(LIB_OBJECTS)
	$(CC) -shared -o $@ $(LIB_OBJECTS)

# Rule to build CLI objects (e.g., cli/equalff.o from cli/equalff.c)
# Objects are placed in the cli/ directory.
# CLI sources might depend on headers from the lib/ directory (via -Ilib CFLAG)
cli/%.o: cli/%.c $(LIB_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(CLI_OBJECTS) $(LIB_TARGET)

# Rule to build the final executable
# Depends on the library and CLI objects
$(TARGET): $(CLI_OBJECTS) $(LIB_TARGET)
	$(CC) $(CLI_OBJECTS) $(LDFLAGS) $(LIBS) -Wl,-rpath,./lib -o $@

clean: cliclean libclean
	-rm -f $(TARGET)

cliclean:
	-rm -f cli/*.o

libclean:
	-rm -f lib/*.o
	-rm -f $(LIB_TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)
	cp $(TARGET) $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)
# If you want to install the dynamic library, you'd add steps here, e.g.:
# cp $(LIB_TARGET) $(DESTDIR)$(PREFIX)/lib
# chmod 644 $(DESTDIR)$(PREFIX)/lib/$(LIB_NAME)
	cp equalff.man $(DESTDIR)$(MANDIR)/$(TARGET).1
	chmod 644 $(DESTDIR)$(MANDIR)/$(TARGET).1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(MANDIR)/$(TARGET).1
# If library installed:
# rm -f $(DESTDIR)$(PREFIX)/lib/$(LIB_NAME)
