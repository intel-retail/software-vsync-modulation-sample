#INTEL CONFIDENTIAL
#Copyright (C) 2022 Intel Corporation
#This software and the related documents are Intel copyrighted materials, and your use of them is governed by the express license under which they were provided to you ("License"). Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose or transmit this software or the related documents without Intel's prior written permission.
#This software and the related documents are provided as is, with no express or implied warranties, other than those that are expressly stated in the License.

#DIRS = $(shell find . -maxdepth 1 -type d -not -path "./.git" \
#	   -not -path "." -not -path "./release" -not -path "./cmn" | sort)
DIRS = lib synctest
.PHONY: $(DIRS)

MAKE += --no-print-directory
CFLAGS =-g
all:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir; \
	done

debug:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir debug; \
	done

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done

distclean:
	@rm -rf release

release: distclean
	@$(MAKE) clean
	@$(MAKE)
	@mkdir release
	@cp lib/*.so synctest/synctest release
