TARGET=equalff
LIBS=
CC=gcc
CFLAGS=-g -Wall -std=gnu99 -D_XOPEN_SOURCE=700

.PHONY: default all clean

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
