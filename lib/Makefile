#INTEL CONFIDENTIAL
#Copyright (C) 2022 Intel Corporation
#This software and the related documents are Intel copyrighted materials, and your use of them is governed by the express license under which they were provided to you ("License"). Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose or transmit this software or the related documents without Intel's prior written permission.
#This software and the related documents are provided as is, with no express or implied warranties, other than those that are expressly stated in the License.

export PWD=$(shell pwd)
BINNAME=libvsyncalter.so
MAKE += --no-print-directory
export CC=g++
export DBG_FLAGS?=-Ofast
export LIBS=-lrt -ldrm -lpciaccess
export INCLUDES=-I../cmn -I/usr/include/drm
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

.cpp.o: $(HEADERS)
	@echo $<
	@$(CC) $(DBG_FLAGS) $(CFLAGS) $(LFLAGS) $(INCLUDES) $< 

debug:
	@export DBG_FLAGS='-g'; \
	$(MAKE)

clean:
	@echo "Cleaning..."
	@rm -f $(BINNAME) *.o

