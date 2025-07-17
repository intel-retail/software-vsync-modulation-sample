# Copyright (C) 2024 Intel Corporation
# SPDX-License-Identifier: MIT

#DIRS = $(shell find . -maxdepth 1 -type d -not -path "./.git" \
#	   -not -path "." -not -path "./release" -not -path "./cmn" | sort)
DIRS = lib test synctest vbltest
.PHONY: $(DIRS)

MAKE += --no-print-directory

all:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir; \
	done

debug:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir debug; \
	done

doxygen:
	@mkdir -p output/doxygen
	@( cat resources/swgen_doxy ; echo "PROJECT_NUMBER=$(VERSION)" ) | doxygen -
clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done

distclean:
	@rm -rf output

release:
	@rm -rf output/release
	@$(MAKE) clean
	@$(MAKE)
	@mkdir -p output/release
	@cp lib/*.so resources/gPTP.cfg test/vsync_test synctest/synctest vbltest/vbltest output/release
