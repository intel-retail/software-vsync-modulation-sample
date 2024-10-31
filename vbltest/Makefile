# Copyright (C) 2024 Intel Corporation
# SPDX-License-Identifier: MIT

# Set the compiler
CXX := g++

# Set the compiler flags
CXXFLAGS := -Wall -I. -I../cmn

# Directory for libraries
LIBDIR := ../lib

# Derive the binary name from the parent directory
BINNAME := $(notdir $(CURDIR))

# Set the source directory and find all C++ files
SRCDIR := .
SOURCES := $(wildcard $(SRCDIR)/*.cpp)

# Set the object directory and define object files
OBJDIR := obj
OBJECTS := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Define dependencies
LIB_DEPENDENCIES := $(LIBDIR)/libvsyncalter.a  $(LIBDIR)/libvsyncalter.so

# Default target (static linking)
all: static

# Target for building with dynamic linking
dynamic: LIBS := -L$(LIBDIR) -lvsyncalter
dynamic: $(BINNAME)

# Target for building with static linking
static: LIBS := -L$(LIBDIR) -l:libvsyncalter.a -lrt -ldrm -lpciaccess
static: $(BINNAME)

# Rule to link the binary
$(BINNAME): $(OBJECTS) $(LIB_DEPENDENCIES)
	@echo "Linking $@..."
	@$(CXX) $(DBG_FLAGS) $(CXXFLAGS) -o $@ $(OBJECTS) $(LIBS)

# Rule to compile the source files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(OBJDIR)
	@$(CXX) $(DBG_FLAGS) $(CXXFLAGS) -c $< -o $@

debug:
	@export DBG_FLAGS='-g -D DEBUGON'; \
	$(MAKE)

# Include dependency files
-include $(OBJECTS:.o=.d)

# Phony targets for cleanliness and utility
.PHONY: clean dynamic static

# Clean the build artifacts
clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJDIR) $(BINNAME)
