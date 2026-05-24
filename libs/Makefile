# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory
MAKEFLAGS += --no-print-directory
CONFIG += ordered

SUBDIRS  = sqlite3 rational sha512 mem xdiff testitall
PURGE_SUBDIRS = $(SUBDIRS) testmocking

TOPTARGETS := all clean debug prod production portable sanitize test gcc-analyzer format coverage

# Dependencies between libraries
mem: rational
xdiff: rational
testitall: mem xdiff rational

# When invoked from the umbrella project, every library writes into the
# project-wide .builds at the repo root. Each library Makefile defaults to its
# own local .builds when launched directly from the library directory
$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@$(MAKE) -s -C $@ $(MAKECMDGOALS) BUILDDIR=../../.builds

.PHONY: $(TOPTARGETS) $(SUBDIRS) purge

# Purge is intentionally separate from TOPTARGETS because it must clean each
# library's own local artifact tree. Passing BUILDDIR=../../.builds here would
# make child Makefiles clean only the umbrella project's root artifact tree
purge:
	@$(foreach dir,$(PURGE_SUBDIRS),$(MAKE) -s -C $(dir) purge || exit $$?;)

gcc-analyzer:
	@true

all: $(SUBDIRS)
	@echo Building all internal libraries
