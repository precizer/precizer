# Name of the library
PCRENAME=pcre2
LIBNAME=lib$(PCRENAME)
LIBVERSION=$(LIBNAME)-8

BUILDDIR = ../../.builds
ZIP = $(PCRENAME)
TMPDIR = $(BUILDDIR)/tmp
UNZIP = $(TMPDIR)/$(ZIP)
VERSION = $(PCRENAME)-10.44

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered
CMAKE_FLAGS += -DPCRE2_SUPPORT_LIBBZ2=OFF -DBUILD_STATIC_LIBS=ON -DPCRE2_BUILD_TESTS=OFF -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_SHOW_REPORT=OFF -DPCRE2_BUILD_PCRE2GREP=OFF -DPCRE2_SUPPORT_LIBREADLINE=OFF

TOPTARGETS := all clean release debug prod production sanitize test

SO = $(LIBVERSION).so
STATLIB = $(LIBVERSION).a
BUILD = $(UNZIP)/$(STATLIB)

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

$(UNZIP):
	@mkdir -p $(TMPDIR)
	@unzip -nqq $(ZIP).zip -d $(TMPDIR)

$(BUILD):
	@cd $(UNZIP); cmake ../$(ZIP) -DPCRE2_DEBUG=ON -DBUILD_SHARED_LIBS=ON $(CMAKE_FLAGS); $(MAKE)
	@echo "$@ prepared to be the library for debug"

checkout:
	cd $(UNZIP); git pull; git checkout tags/$(VERSION); cd ..; zip -rqq $(ZIP).zip $(ZIP)

move_archive:
	mv $(ZIP).zip $(ZIP).old.zip
	mv $(TMPDIR)/$(ZIP).zip .

switch_version: $(UNZIP) checkout move_archive clear_tmp_unzipped
	@true

#
# Debug build settings
#
DBGDIR = $(BUILDDIR)/debug
DBGLIBDIR = $(DBGDIR)/libs
DBGLIBINC = $(DBGDIR)/inc
DBGSO = $(DBGLIBDIR)/$(SO)
DBGSTAT = $(DBGLIBDIR)/$(STATLIB)
DBGLIBH = $(DBGLIBINC)/$(PCRENAME).h

#
# Sanitize build settings
#
STZDIR = $(BUILDDIR)/sanitize
STZLIBDIR = $(STZDIR)/libs
STZLIBINC = $(STZDIR)/inc
STZSO = $(STZLIBDIR)/$(SO)
STZSTAT = $(STZLIBDIR)/$(STATLIB)
STZLIBH = $(STZLIBINC)/$(PCRENAME).h

#
# Release build settings
#
RELDIR = $(BUILDDIR)/release
RELLIBDIR = $(RELDIR)/libs
RELLIBINC = $(RELDIR)/inc
RELSO = $(RELLIBDIR)/$(SO)
RELSTAT = $(RELLIBDIR)/$(STATLIB)
RELLIBH = $(RELLIBINC)/$(PCRENAME).h

#
# Debug rules
#
debug: $(UNZIP) $(BUILD) $(DBGSO) $(DBGSTAT) $(DBGLIBH)
	@true

$(DBGSO): $(BUILD)
	@mkdir -p $(DBGLIBDIR)
	@cp $(UNZIP)/$(SO)* $(DBGLIBDIR)/
	@cd $(DBGLIBDIR); ln -f -s $(SO) $(LIBNAME).so

$(DBGSTAT): $(BUILD)
	@mkdir -p $(DBGLIBDIR)
	@cp $(UNZIP)/$(STATLIB) $(DBGLIBDIR)/
	@cd $(DBGLIBDIR); ln -f -s $(STATLIB) $(LIBNAME).a

$(DBGLIBH):
	@mkdir -p $(DBGLIBINC)
	@cp $(UNZIP)/$(PCRENAME).h $(DBGLIBINC)/

#
# Debug rules
#

sanitize: CMAKE_FLAGS += -DCMAKE_C_FLAGS="-fsanitize=address,undefined" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined -static-libasan"
sanitize: $(UNZIP) $(BUILD) $(STZSO) $(STZSTAT) $(STZLIBH)
	@true

$(STZSO): $(BUILD)
	@mkdir -p $(STZLIBDIR)
	@cp $(UNZIP)/$(SO)* $(STZLIBDIR)/
	@cd $(STZLIBDIR); ln -f -s $(SO) $(LIBNAME).so

$(STZSTAT): $(BUILD)
	@mkdir -p $(STZLIBDIR)
	@cp $(UNZIP)/$(STATLIB) $(STZLIBDIR)/
	@cd $(STZLIBDIR); ln -f -s $(STATLIB) $(LIBNAME).a

$(STZLIBH):
	@mkdir -p $(STZLIBINC)
	@cp $(UNZIP)/$(PCRENAME).h $(STZLIBINC)/


#
# Release rules
#
release: $(UNZIP) $(BUILD) $(RELSO) $(RELSTAT) $(RELLIBH)
	@true

$(RELSO): $(BUILD)
	@mkdir -p $(RELLIBDIR)
	@cp $(UNZIP)/$(SO)* $(RELLIBDIR)/
	@cd $(RELLIBDIR); ln -f -s $(SO) $(LIBNAME).so

$(RELSTAT): $(BUILD)
	@mkdir -p $(RELLIBDIR)
	@cp $(UNZIP)/$(STATLIB) $(RELLIBDIR)/
	@cd $(RELLIBDIR); ln -f -s $(STATLIB) $(LIBNAME).a

$(RELLIBH):
	@mkdir -p $(RELLIBINC)
	@cp $(UNZIP)/$(PCRENAME).h $(RELLIBINC)/

clean:
	@test -f $(RELLIBH) && rm $(RELLIBH) || true
	@test -d $(RELLIBINC) && rm -d $(RELLIBINC) 2>/dev/null || true

	@test -f $(DBGLIBH) && rm $(DBGLIBH) || true
	@test -d $(DBGLIBINC) && rm -d $(DBGLIBINC) 2>/dev/null || true
	@test -h $(DBGLIBDIR)/$(LIBNAME).so && rm -f $(DBGLIBDIR)/$(LIBVERSION).so* $(DBGLIBDIR)/$(LIBNAME).so || true
	@test -h $(DBGLIBDIR)/$(LIBNAME).a && rm -f $(DBGLIBDIR)/$(LIBVERSION).a $(DBGLIBDIR)/$(LIBNAME).a || true
	@test -d $(DBGLIBDIR) && rm -d $(DBGLIBDIR) 2>/dev/null || true
	@test -d $(DBGDIR) && rm -d $(DBGDIR) 2>/dev/null || true

	@test -h $(RELLIBDIR)/$(LIBNAME).so && rm -f $(RELLIBDIR)/$(LIBVERSION).so* $(RELLIBDIR)/$(LIBNAME).so || true
	@test -h $(RELLIBDIR)/$(LIBNAME).a && rm -f $(RELLIBDIR)/$(LIBVERSION).a $(RELLIBDIR)/$(LIBNAME).a || true
	@test -d $(RELLIBDIR) && rm -d $(RELLIBDIR) 2>/dev/null || true
	@test -d $(RELDIR) && rm -d $(RELDIR) 2>/dev/null || true

	@test -d $(UNZIP) && rm -rf $(UNZIP) || true
	@test -d $(TMPDIR) && rm -rf $(TMPDIR) || true
	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true
	@echo $(LIBVERSION) cleared.
