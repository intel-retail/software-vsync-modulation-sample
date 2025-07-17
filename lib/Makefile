# Copyright (C) 2024 Intel Corporation
# SPDX-License-Identifier: MIT

# Compiler and Flags
CXX := g++
CXXFLAGS := -Wall -fPIC -I../cmn -Iplatforms -Iinclude -I. -I/usr/include/drm
LIBS :=-lrt -ldrm -lpciaccess
# Directories
SRCDIR := .
OBJDIR := obj
INCLUDEDIR := include

COVERAGE_FLAGS := -fprofile-arcs -ftest-coverage -O0 -g

# Target libraries
TARGET_SHARED := libvsyncalter.so
TARGET_STATIC := libvsyncalter.a

# Find source files, create a list of object files
SOURCES := $(wildcard $(SRCDIR)/*.cpp)
HEADERS := $(wildcard $(SRCDIR)/*.h ../cmn/*.h $(SRCDIR)/platforms/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPS := $(OBJECTS:.o=.d)  # Dependency files corresponding to object files

# Check if VERBOSE is set to 1, if not, silence the commands
ifeq ($(VERBOSE),1)
CMD_PREFIX=
else
CMD_PREFIX=@
endif

# Default target
all: $(TARGET_SHARED) $(TARGET_STATIC)

# Shared library
$(TARGET_SHARED): $(OBJECTS)
	$(CMD_PREFIX)echo "Creating shared library..."
	$(CMD_PREFIX)$(CXX) -Wall -shared $(CXXFLAGS)  $(DBG_FLAGS) -o $@ $^ $(LIBS)
# Static library
$(TARGET_STATIC): $(OBJECTS)
	$(CMD_PREFIX)echo "Creating static library..."
	$(CMD_PREFIX)ar rcs $@ $^

# Compile source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS)
	$(CMD_PREFIX)echo "Compiling $<"
	$(CMD_PREFIX)mkdir -p $(OBJDIR)
	$(CMD_PREFIX)$(CXX) $(CXXFLAGS) $(DBG_FLAGS) -c $< -o $@

# Include the .d files
-include $(DEPS)

coverage:
	@echo "Building static and shared libraries with coverage flags..."
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) $(COVERAGE_FLAGS)" all

debug:
	@export DBG_FLAGS='-g -O0 -D DEBUGON'; \
	$(MAKE)

# Clean up build artifacts
clean:
	$(CMD_PREFIX)echo "Cleaning..."
	$(CMD_PREFIX)rm -f $(TARGET_SHARED) $(TARGET_STATIC) $(OBJECTS) $(DEPS)
	$(CMD_PREFIX)rm -rf $(OBJDIR)


# Phony targets
.PHONY: all clean
