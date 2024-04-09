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
	@cp lib/*.so resources/gPTP.cfg test/vsync_test synctest/synctest vbltest/vbltest release
