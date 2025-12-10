# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered

SUBDIRS  = sqlite3 rational sha512 mem xdiff testitall

TOPTARGETS := all clean debug prod production portable sanitize test cosmo gcc-analyzer format

# Dependencies between libraries
mem: rational
xdiff: rational
testitall: mem xdiff rational

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@$(MAKE) -d -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS) warning

gcc-analyzer:
	@true

all: $(SUBDIRS)
	@echo Building all internal libraries
