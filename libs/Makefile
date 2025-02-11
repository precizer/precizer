# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered

SUBDIRS  = sqlite rational sha512 testitall mem xdiff

TOPTARGETS := all clean release debug prod production sanitize test cosmo gcc-analyzer

$(TOPTARGETS): warning $(SUBDIRS)
$(SUBDIRS):
	@$(MAKE) -s -C $@ $(MAKECMDGOALS) 2>/dev/null

.PHONY: $(TOPTARGETS) $(SUBDIRS) warning

warning:
	@echo Library compilation has started. This process may take a considerable amount of time.

gcc-analyzer:
	@true

all: $(SUBDIRS)
	@echo Building all internal libraries
