# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered

SUBDIRS  = sqlite rational sha512 pcre2 testitall mem

TOPTARGETS := all clean release debug prod production sanitize test cosmo

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	@$(MAKE) -s -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

gccanalyzer:
	@true

all: $(SUBDIRS)
