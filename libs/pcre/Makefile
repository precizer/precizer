# Name of the library
LIBNAME=libpcre2-8

ZIP=pcre2
VERSION=pcre2-10.43

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered
CMAKE_FLAGS += -DPCRE2_SUPPORT_LIBBZ2=OFF -DBUILD_STATIC_LIBS=ON -DPCRE2_BUILD_TESTS=OFF -DPCRE2_BUILD_PCRE2_8=ON

TOPTARGETS := all clean release debug prod production sanitize test

SO = $(LIBNAME).so
STATLIB = $(LIBNAME).a

.PHONY: $(TOPTARGETS)

# Default build
all: debug release
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

$(ZIP):
	@true

switch_version: $(ZIP)
	cd $(ZIP); git checkout tags/$(VERSION)
	mv $(ZIP).zip $(ZIP).old.zip
	zip -rqq $(ZIP).zip $(ZIP)

#
# Debug build settings
#
DBGDIR = debug
DBGSO = $(DBGDIR)/$(SO)
DBGSTAT = $(DBGDIR)/$(STATLIB)

#
# Release build settings
#
RELDIR = release
RELSO = $(RELDIR)/$(SO)
RELSTAT = $(RELDIR)/$(STATLIB)

#
# Debug rules
#
debug: $(ZIP) $(DBGSO) $(DBGSTAT)
	@true

$(DBGSO): $(DBGSTAT)
	@true

$(DBGSTAT):
	@mkdir -p debug; cd debug; cmake ../$(ZIP) -DPCRE2_DEBUG=ON -DBUILD_SHARED_LIBS=ON $(CMAKE_FLAGS); $(MAKE)
	@ln -f -s $(STATLIB) debug/libpcre.a
	@echo "$@ prepared to be the library for debug."

#
# Release rules
#
release: $(ZIP) $(RELSO) $(RELSTAT)
	@true

$(RELSO): $(RELSTAT)
	@true

$(RELSTAT):
	@mkdir -p release; cd release; cmake ../$(ZIP) -DBUILD_SHARED_LIBS=OFF $(CMAKE_FLAGS); $(MAKE)
	@ln -f -s $(STATLIB) release/libpcre.a
	@echo "$@ prepared to be the library for release."

update:
	git submodule update --remote

clean:
	@test -d $(ZIP) && rm -rf $(ZIP) || true
	@test -d $(DBGDIR) && rm -rf $(DBGDIR) || true
	@test -d $(RELDIR) && rm -rf $(RELDIR) || true
	@echo $(LIBNAME) cleared.
