# Copyright (C) 2024 Intel Corporation
# SPDX-License-Identifier: MIT

export PWD=$(shell pwd)
BINNAME=libvsyncalter.so
MAKE += --no-print-directory
export CC=g++
export DBG_FLAGS?=-Ofast
export LIBS=-lrt -ldrm -lpciaccess
export INCLUDES=-I../cmn -I/usr/include/drm -Iplatforms
export CFLAGS=-c -fPIC
export LFLAGS=-Wall
export HEADERS=$(wildcard *.h)
export SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)

all::
	@echo "Compiling..."

all::$(SOURCES) $(BINNAME)

all::
	@echo "Done"

$(BINNAME): $(OBJECTS)
	@$(CC) $(DBG_FLAGS) $(LFLAGS) -shared -o $@ $(OBJECTS) $(LIBS) 
	@if [ "$(DBG_FLAGS)" = "" ]; then \
		echo "Stripping $@"; \
		strip -x $@; \
	fi

.cpp.o:
	@echo $<
	@$(CC) $(DBG_FLAGS) $(CFLAGS) $(LFLAGS) $(INCLUDES) $< 

debug:
	@export DBG_FLAGS='-g'; \
	$(MAKE)

clean:
	@echo "Cleaning..."
	@rm -f $(BINNAME) *.o

