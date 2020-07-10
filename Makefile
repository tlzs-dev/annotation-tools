TARGET=bin/annotation-tools

DEBUG ?= 1
OPTIMIZE ?= -O3

CC=gcc -std=gnu99 -D_GNU_SOURCE
LINKER=gcc -std=gnu99 -D_GNU_SOURCE

CFLAGS=-Iinclude -Isrc -Wall
ifeq ($(DEBUG),1)
	CFLAGS += -D_DEBUG -g
	OPTIMIZE=-O0
endif

LDFLAGS=$(CFLAGS) $(OPTIMIZE)




LIBS=-lm -lpthread -ljson-c -ljpeg -lpng -lcairo

CFLAGS += $(shell pkg-config --cflags gtk+-3.0)
LIBS += $(shell pkg-config --libs gtk+-3.0)

CFLAGS += $(shell pkg-config --cflags gio-2.0 glib-2.0)
LIBS += $(shell pkg-config --libs gio-2.0 glib-2.0)




SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:src/%.c=obj/%.o)


UTILS_SOURCES := $(wildcard utils/*.c)
UTILS_OBJECTS := $(UTILS_SOURCES:utils/%.c=obj/utils/%.o)

all: do_init $(TARGET)

$(TARGET): $(OBJECTS) $(UTILS_OBJECTS)
	$(LINKER) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJECTS): obj/%.o : src/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(UTILS_OBJECTS): obj/utils/%.o : utils/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: do_init clean
do_init:
	mkdir -p obj/utils bin lib

clean:
	rm -f obj/*.o obj/utils/*.o $(TARGET) 
