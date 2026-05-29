# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory
MAKEFLAGS += --no-print-directory
CONFIG += ordered

SUBDIRS  = sqlite3 rational sha512 mem xdiff testitall
PURGE_SUBDIRS = $(SUBDIRS) testmocking

TOPTARGETS := all clean debug prod production portable sanitize test tests-debug tests-sanitize tests-coverage gcc-analyzer format coverage compile-commands purge

.PHONY: $(TOPTARGETS) $(SUBDIRS)

# Dependencies between libraries
mem: rational
xdiff: rational
testitall: mem xdiff rational

# Each child library Makefile defaults to its own local .builds directory.
# Callers that need a shared artifact tree pass BUILDDIR explicitly
$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@$(MAKE) -s -C $@ $(MAKECMDGOALS)

gcc-analyzer:
	@true

all: $(SUBDIRS)
	@echo Building all internal libraries
