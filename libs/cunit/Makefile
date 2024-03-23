# Name of the library
LIBNAME=libcunit

ZIP=cunit

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered

TOPTARGETS := all clean release debug prod production sanitize test

STATLIB = $(LIBNAME).a
LIBSTAT = debug/CUnit/$(LIBNAME).a

.PHONY: $(TOPTARGETS)

# Default build
all: debug release $(LIBSTAT)
	@true

prod: release
	@true

production: prod
	@true

test: debug
	@true

memtest: debug
	@true

sanitize: debug
	@true

debug:
	@true

release:
	@true

$(ZIP):
	@unzip -nqq $(ZIP).zip

$(LIBSTAT): $(ZIP)
	@mkdir -p debug; cd debug; cmake ../$(ZIP) -DCUNIT_DISABLE_TESTS=ON -DCUNIT_DISABLE_EXAMPLES=ON; cd CUnit; make
	@echo "$(LIBNAME) library prepared."

clean:
	@test -d $(ZIP) && rm -rf $(ZIP) || true
	@test -d debug && rm -rf debug || true
	@echo $(LIBNAME) cleared.
