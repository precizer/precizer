# How to install dependencies and build the app:
#
# GCC
# sudo apt -y install gcc make libpcre2-dev
#
# Clang (LLVM linker and tools are needed for LTO)
# sudo apt -y install clang lld llvm
#
# Sanitizer support (llvm-symbolizer for readable stack traces)
# sudo apt -y install llvm libubsan1
#
# Support XXH3_128bits algorithm
# sudo apt -y install libxxhash-dev
#
# Libraries
# sudo apt -y install libgoogle-perftools-dev
#
# Install statistics and test tools
# sudo apt-get install cloc valgrind clang-tools cppcheck
#
# make production # or
# make prod # or (same as production)
# make portable # or
# make debug # or
# make # prod by default
#
# Perf tool:
# sudo apt-get install linux-tools-common linux-tools-generic linux-tools-`uname -r`
# make perf # or
# make analyze
#
# Automated build with GitHub Actions:
#
# * Create an annotated tag:
#   ```sh
#   git tag -a v0.2.0 -m "Release version 0.2.0"
#   ```
# * Push the tag to the remote server for release creation:
#   ```sh
#   git push origin v0.2.0
#   ```
# * Delete a failed local tag:
#   ```sh
#   git tag -d v0.2.0
#   ```

# Define our suffix list for quick compilation
.SUFFIXES:          # Delete the default suffixes
.SUFFIXES: .c .o .h # Define our suffix list

BUILDDIR = .builds
# Returns a concise path for build messages.
# Plain file names stay unchanged; nested paths keep only final-directory/file
short_path = $(if $(filter ./,$(dir $(1))),$(notdir $(1)),$(notdir $(patsubst %/,%,$(dir $(1))))/$(notdir $(1)))
COMPILE_COMMANDS = $(BUILDDIR)/compile_commands.json

#
# Compiler flags
#

CFLAGS += -std=c2x -finline-functions

# To pass a #define into the build:
# make DEFINES=-DWRITE_CSV=false memtest
CFLAGS += $(DEFINES)

LIBS = sha512 mem rational sqlite3
STATLIBS = sha512 mem rational
LDLIBS = $(foreach d,$(LIBS),-l$d)

UNAME_S := $(shell uname -s)

# macOS-specific code
ifeq ($(UNAME_S),Darwin)
CFLAGS += -DEVIL_EMPIRE_OS
endif

# Detect whether we're using GCC (covers names like arm-linux-gnu-gcc)
GCC := $(findstring gcc,$(notdir $(firstword $(CC))))

EXE = precizer
ROOT_EXE = ./$(EXE)
ifeq ($(UNAME_S),Darwin)
STATIC =
STRIP ?= -Wl,-x
else
STATIC = -static -static-libgcc -Wl,--gc-sections
STRIP ?= -s
endif

ifneq ($(findstring CYGWIN,$(UNAME_S)),)
LTO =
else
LTO = -flto=auto
endif

# UPX compression (disabled on macOS)
ifeq ($(UNAME_S),Darwin)
UPX = true
else
UPX ?= upx --best --lzma -qqq
endif

# Warning flags for additional checks
WFLAGS += -Werror # Treat every warning as an error (-Werror stops the build)
WFLAGS += -Wall
WFLAGS += -Wpedantic
WFLAGS += -Wshadow
WFLAGS += -Wextra
WFLAGS += -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code -Wformat-y2k
WFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs
WFLAGS += -Wswitch-default -Wtrigraphs -Wstrict-overflow=5
WFLAGS += -Wfloat-equal -Wundef
WFLAGS += -Wbad-function-cast -Wcast-qual -Wcast-align
WFLAGS += -Wwrite-strings
WFLAGS += -Winline
# Extra warnings enabled only for GCC
ifneq ($(GCC),)
WFLAGS += -Wlogical-op
WFLAGS += -Wsuggest-attribute=const
WFLAGS += -Wsuggest-attribute=pure
WFLAGS += -Wsuggest-attribute=noreturn
WFLAGS += -Wsuggest-attribute=format
WFLAGS += -Wmissing-format-attribute
endif

# Arguments for the test
ARGS = tests/fixtures/diffs

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered

# Test directory
TEST_DIR = tests

# Tools directory
TOOLS_DIR = tools

# Additional include headers of external libraries
DYNAMIC_INCPATH += $(foreach d,$(STATLIBS),-Ilibs/$d/src/)
INCPATH += $(foreach d,$(LIBS),-Ilibs/$d/src/)

ifeq ($(UNAME_S),Darwin)
# argp lib
DYNAMIC_INCPATH += $(shell pkg-config --cflags libpcre2-8)
INCPATH += $(shell pkg-config --cflags libpcre2-8)
LDLIBS += $(shell pkg-config --libs libpcre2-8)
LDLIBS += -largp
else
LDLIBS += -lpcre2-8
ifneq ($(findstring CYGWIN,$(UNAME_S)),)
# argp is not part of Cygwin's C library
LDLIBS += -largp
endif
endif

# Default build
all: production

# Clang
# Find the highest-versioned clang binary available (clang-21, clang-20, …),
# fall back to plain "clang" when no versioned binary exists
CLANG := $(notdir $(or $(lastword $(sort $(wildcard $(addsuffix /clang-[0-9]*,$(subst :, ,$(PATH)))))),clang))
# Extract the version suffix (e.g. "-20") so scan-build and other
# LLVM tools with the same versioning scheme can be resolved too
CLANG_SUFFIX := $(patsubst clang%,%,$(CLANG))
SCAN_BUILD := scan-build$(CLANG_SUFFIX)

# Set COMPILER=clang to build any target with the latest available clang:
#   make COMPILER=clang production
#   make COMPILER=clang dynamic-production
#   COMPILER=clang make debug
ifeq ($(COMPILER),clang)
CC := $(CLANG)
export CC
$(info Using compiler: $(CLANG))
# Use LLVM's own linker to avoid gold plugin version mismatch with LTO
ifneq ($(shell which ld.lld$(CLANG_SUFFIX) 2>/dev/null),)
USE_LLD := -fuse-ld=lld$(CLANG_SUFFIX)
else ifneq ($(shell which ld.lld 2>/dev/null),)
USE_LLD := -fuse-ld=lld
endif
export USE_LLD
# Use LLVM's own archiver so ar can index LTO bitcode without the BFD plugin
ifneq ($(shell which llvm-ar$(CLANG_SUFFIX) 2>/dev/null),)
AR := llvm-ar$(CLANG_SUFFIX)
else ifneq ($(shell which llvm-ar 2>/dev/null),)
AR := llvm-ar
endif
export AR
endif

SYS := $(shell $(CC) -dumpmachine 2>/dev/null)
ifneq (, $(findstring alpine, $(SYS)))
# Alpine Linux uses external libraries
LDLIBS += -largp -lfts
endif

#
# Project files
#
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h) $(foreach d,$(LIBS),$(wildcard libs/$d/src/*.h))

# Exclude a file
OBJS = $(SRCS:.c=.o)
PREPROC = $(SRCS:.c=.i) # Preprocessed files http://www.viva64.com/en/t/0076/
PREPROC += $(SRCS:.c=.i.h)
# Assembly
ASM = $(SRCS:.c=.asm)

#
# Debug build settings
#
DBG = debug
DBG_DIR = $(BUILDDIR)/$(DBG)
DBG_LIBDIR = $(DBG_DIR)/libs
DBG_OBJDIR = $(DBG_DIR)/obj
DBG_LDPATH = -L$(DBG_LIBDIR) $(LDPATH)
DBG_EXE = $(DBG_DIR)/$(EXE)
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(OBJS)))
DBG_LIBS = $(LIBS)
DBG_LIB_OBJS = $(foreach lib,$(DBG_LIBS),$(DBG_LIBDIR)/obj/$(lib)/*.o)
DBG_EXT_LDLIBS = $(filter-out $(addprefix -l,$(DBG_LIBS)),$(LDLIBS))
DBG_INCPATH = $(INCPATH)
DBG_OPT_CFLAGS ?= -pipe -fbuiltin -Og -fno-omit-frame-pointer
DBG_CFLAGS = $(CFLAGS) $(DBG_OPT_CFLAGS) -g -ggdb3 -DDEBUG -DTESTITALL_TEST_HOOKS
LIBS_GOAL ?= debug
ifeq ($(UNAME_S),Darwin)
DBG_RPATH = -Wl,-rpath,@executable_path/$(DBG_LIBDIR),-rpath,@executable_path/libs
DBG_LDFLAGS = $(USE_LLD) -Wl,-undefined,dynamic_lookup
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
DBG_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBG_LIBDIR),-rpath,\$$ORIGIN/libs
DBG_OPT_LDFLAGS ?= -Wl,--as-needed
DBG_LDFLAGS = $(USE_LLD) $(DBG_OPT_LDFLAGS)
else
DBG_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBG_LIBDIR),-rpath,\$$ORIGIN/libs
DBG_OPT_LDFLAGS ?= -Wl,--as-needed
DBG_LDFLAGS = $(USE_LLD) -Wl,-z,defs $(DBG_OPT_LDFLAGS)
endif
# Activate the Gprof profiler.
# Works incorrectly with Valgrind.
# It is better to use Callgrind - the call graph format
# is supported by visualization tools like kcachegrind.
#DBG_CFLAGS += -pg

#
# Dynamic debug build settings
#
DBG_DYN = debug-dynamic
DBG_DYN_DIR = $(BUILDDIR)/$(DBG_DYN)
DBG_DYN_LIBDIR = $(DBG_LIBDIR)
DBG_DYN_OBJDIR = $(DBG_DYN_DIR)/obj
DBG_DYN_LDPATH = $(LDPATH)
DBG_DYN_EXE = $(DBG_DYN_DIR)/$(EXE)
DBG_DYN_OBJS = $(addprefix $(DBG_DYN_OBJDIR)/, $(notdir $(OBJS)))
DBG_DYN_LIBS = $(STATLIBS)
DBG_DYN_LIB_OBJS = $(foreach lib,$(DBG_DYN_LIBS),$(DBG_DYN_LIBDIR)/obj/$(lib)/*.o)
DBG_DYN_EXT_LDLIBS = $(filter-out $(addprefix -l,$(DBG_DYN_LIBS)),$(LDLIBS))
DBG_DYN_INCPATH = $(DYNAMIC_INCPATH)
DBG_DYN_HDRS = $(wildcard $(SRC_DIR)/*.h) $(foreach d,$(DBG_DYN_LIBS),$(wildcard libs/$d/src/*.h))
DBG_DYN_CFLAGS = $(DBG_CFLAGS)
DBG_DYN_LDFLAGS = $(DBG_LDFLAGS)

#
# Coverage build settings
#
COV = coverage
COV_DIR = $(BUILDDIR)/$(COV)
COV_LIBDIR = $(COV_DIR)/libs
COV_OBJDIR = $(COV_DIR)/obj
COV_LDPATH = -L$(COV_LIBDIR) $(LDPATH)
COV_EXE = $(COV_DIR)/$(EXE)
COV_OBJS = $(addprefix $(COV_OBJDIR)/, $(notdir $(OBJS)))
COV_LIB_OBJS = $(foreach lib,$(LIBS),$(COV_LIBDIR)/obj/$(lib)/*.o)
COV_EXT_LDLIBS = $(filter-out $(addprefix -l,$(LIBS)),$(LDLIBS))
COV_OPT_CFLAGS ?= -pipe -fbuiltin -O0 -fno-omit-frame-pointer
COV_CFLAGS = $(CFLAGS) -fprofile-arcs -ftest-coverage $(COV_OPT_CFLAGS) -g -DDEBUG -DTESTITALL_TEST_HOOKS
COV_LDFLAGS = $(USE_LLD) -lgcov --coverage

#
# Sanitize build settings
#
SNTZ_DIR = $(BUILDDIR)/sanitize
SNTZ_LIBDIR = $(SNTZ_DIR)/libs
SNTZ_OBJDIR = $(SNTZ_DIR)/obj
SNTZ_LDPATH = -L$(SNTZ_LIBDIR) $(LDPATH)
SNTZ_EXE = $(SNTZ_DIR)/$(EXE)
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(OBJS)))
SNTZ_LIB_OBJS = $(foreach lib,$(LIBS),$(SNTZ_LIBDIR)/obj/$(lib)/*.o)
SNTZ_EXT_LDLIBS = $(filter-out $(addprefix -l,$(LIBS)),$(LDLIBS))
SNTZ_OPTIONS = -fsanitize=address,undefined
SNTZ_OPT_CFLAGS ?= $(DBG_OPT_CFLAGS)
SNTZ_CFLAGS = $(CFLAGS) $(SNTZ_OPT_CFLAGS) $(SNTZ_OPTIONS) -g -ggdb3 -DDEBUG -DTESTITALL_TEST_HOOKS
ifeq ($(UNAME_S),Darwin)
SNTZ_RPATH = -Wl,-rpath,@executable_path/$(SNTZ_LIBDIR),-rpath,@executable_path/libs,-rpath,@executable_path/../debug/libs
SNTZ_LDFLAGS = $(USE_LLD) -Wl,-undefined,dynamic_lookup $(SNTZ_OPTIONS)
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
SNTZ_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(SNTZ_LIBDIR),-rpath,\$$ORIGIN/libs,-rpath,\$$ORIGIN/../debug/libs
SNTZ_LDFLAGS = $(USE_LLD) $(SNTZ_OPTIONS)
else
SNTZ_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(SNTZ_LIBDIR),-rpath,\$$ORIGIN/libs,-rpath,\$$ORIGIN/../debug/libs
SNTZ_LDFLAGS = $(USE_LLD) -Wl,-z,defs $(SNTZ_OPTIONS)
endif

#
# Production build settings
#
PROD_DIR = $(BUILDDIR)/production
PROD_LIBDIR = $(PROD_DIR)/libs
PROD_OBJDIR = $(PROD_DIR)/obj
PROD_LDPATH = -L$(PROD_LIBDIR) $(LDPATH)
PROD_EXE = $(PROD_DIR)/$(EXE)
PROD_OBJS = $(addprefix $(PROD_OBJDIR)/, $(notdir $(OBJS)))
PROD_LIB_OBJS = $(foreach lib,$(LIBS),$(PROD_LIBDIR)/obj/$(lib)/*.o)
PROD_EXT_LDLIBS = $(filter-out $(addprefix -l,$(LIBS)),$(LDLIBS))
PROD_OPT_CFLAGS ?= -pipe -fbuiltin $(LTO) -O3 -march=native -funroll-loops -ffunction-sections -fdata-sections -fomit-frame-pointer
PROD_CFLAGS ?= $(CFLAGS) $(PROD_OPT_CFLAGS)
# PROD_LDFLAGS and PROD_CFLAGS use ?= so that distribution package managers
# (Gentoo Portage, Debian dpkg-buildflags, RPM macros, etc.) can override them
# with system-wide hardening and optimization flags via the command line:
#   emake PROD_LDFLAGS='$(LDFLAGS)' PROD_CFLAGS='$(CFLAGS)'
# When overridden, $(LDFLAGS) is replaced by the value from the package manager;
# when not overridden, it expands to empty (LDFLAGS is not set in this project).
# See .packaging/gentoo/ for a real-world example.
ifeq ($(UNAME_S),Darwin)
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,-dead_strip
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(PROD_OPT_LDFLAGS)
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,--gc-sections
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(PROD_OPT_LDFLAGS)
else
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,--gc-sections
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(PROD_OPT_LDFLAGS) -Wl,-z,defs
endif

#
# Dynamic production build settings
#
DYNP_DIR = $(BUILDDIR)/dynamic-production
DYNP_OBJDIR = $(DYNP_DIR)/obj
DYNP_LDPATH = $(LDPATH)
DYNP_EXE = $(DYNP_DIR)/$(EXE)
DYNP_OBJS = $(addprefix $(DYNP_OBJDIR)/, $(notdir $(OBJS)))
DYNP_LIB_SRCS = $(foreach lib,$(STATLIBS),$(wildcard libs/$(lib)/src/*.c))
DYNP_LIB_OBJS = $(foreach lib,$(STATLIBS),$(PROD_LIBDIR)/obj/$(lib)/*.o)
DYNP_CFLAGS = $(PROD_CFLAGS)
DYNP_LDFLAGS = $(PROD_LDFLAGS)
DYNP_SHARED_LIBS = $(filter-out $(addprefix -l,$(STATLIBS)),$(LDLIBS))

#
# Portable build settings
#
PRTB_DIR = $(BUILDDIR)/portable
PRTB_LIBDIR = $(PRTB_DIR)/libs
PRTB_OBJDIR = $(PRTB_DIR)/obj
PRTB_LDPATH = -L$(PRTB_LIBDIR) $(LDPATH)
PRTB_EXE = $(PRTB_DIR)/$(EXE)
PRTB_OBJS = $(addprefix $(PRTB_OBJDIR)/, $(notdir $(OBJS)))
PRTB_LIB_OBJS = $(foreach lib,$(LIBS),$(PRTB_LIBDIR)/obj/$(lib)/*.o)
PRTB_EXT_LDLIBS = $(filter-out $(addprefix -l,$(LIBS)),$(LDLIBS))
PRTB_OPT_CFLAGS ?= -pipe -fbuiltin $(LTO) -O2 -mtune=generic -funroll-loops -ffunction-sections -fdata-sections -fomit-frame-pointer
PRTB_CFLAGS = $(CFLAGS) $(PRTB_OPT_CFLAGS)
ifeq ($(UNAME_S),Darwin)
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,-dead_strip
PRTB_LDFLAGS = $(USE_LLD) $(PRTB_OPT_LDFLAGS)
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,--gc-sections
PRTB_LDFLAGS = $(USE_LLD) $(PRTB_OPT_LDFLAGS)
else
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,--hash-style=both -Wl,--as-needed -Wl,--gc-sections
PRTB_LDFLAGS = $(USE_LLD) $(PRTB_OPT_LDFLAGS) -Wl,-z,defs
endif

# https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory
TOPTARGETS := all
LIBS_MATRIX_BUILDS = libs-debug libs-coverage libs-production libs-portable libs-sanitize
LIBS_MATRIX_TESTS = libs-tests-debug libs-tests-coverage libs-tests-sanitize

define BUILD_USAGE_BANNER
printf "Now some tests could be running:\n"
printf "\033[1mStage 1. Adding:\033[0m\n./$(EXE) --progress --database=database1.db tests/fixtures/diffs/diff1\n"
printf "\033[1mStage 2. Adding:\033[0m\n./$(EXE) --progress --database=database2.db tests/fixtures/diffs/diff2\n"
printf "\033[1mFinal stage. Comparing:\033[0m\n./$(EXE) --compare database1.db database2.db\n"
endef

.PHONY: all clean debug debug-dynamic remake tests sanitize banner run format portable production prod dynamic-production dynamic-production-build debug-build debug-dynamic-build debuglibs debugdynlibs coveragelibs sanitizelibs prodlibs dynprodlibs portablelibs debugfinal debugdynfinal prodfinal sanitizefinal dynprodfinal portfinal coverage coveragefinal precizer-coverage print-%
.PHONY: production-done portable-done
.PHONY: banner-production banner-dynamic-production banner-portable
.PHONY: purge clean-all clean-tools clean-tests clean-preproc clean-asm test test-coverage tests-sanitize tests-debug tests-dynamic analyze static-analyzers static-analyzers-cli gcc-analyzer cppcheck memtest cachegrind callgrind helgrind massif clang-analyzer clang-analyzer-cli doc spellcheck gource perf stat cloc
.PHONY: compile-commands
.PHONY: docker-check-every-os docker-check-os-% clean-docker-os-% print-docker-oses
.PHONY: $(LIBS_MATRIX_BUILDS) $(LIBS_MATRIX_TESTS)

$(LIBS_MATRIX_BUILDS) $(LIBS_MATRIX_TESTS):
	@$(MAKE) -s -C libs $(patsubst libs-%,%,$@)

#
# Debug rules
#
debug: $(DBG_EXE) debugfinal

debug-build: $(DBG_EXE)
	@$(DBG_EXE) --version

debugfinal: $(DBG_EXE)
	@$(DBG_EXE) --version
	@echo "The application has been built and is located: $(DBG_EXE)"

debuglibs:
	@$(MAKE) -s -C libs $(LIBS_GOAL) SUBDIRS="$(DBG_LIBS)" BUILDDIR=../../$(BUILDDIR)

$(DBG_EXE): $(DBG_OBJS) debuglibs
	@$(CC) $(STATIC) $(DBG_LDPATH) $(DBG_LDFLAGS) $(DBG_OBJS) $(DBG_LIB_OBJS) $(DBG_EXT_LDLIBS) -o $@
ifeq ($(UNAME_S),Darwin)
	@echo "$(call short_path,$@) linked dynamically, not stripped"
else
	@echo "$(call short_path,$@) linked statically, not stripped"
endif

$(DBG_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(DBG_OBJDIR)
	@$(CC) -c $(DBG_INCPATH) $(WFLAGS) $(DBG_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with debug flags"

$(DBG_OBJDIR):
	@mkdir -p $(DBG_OBJDIR)

debug-dynamic: $(DBG_DYN_EXE) debugdynfinal

debug-dynamic-build: $(DBG_DYN_EXE)
	@$(DBG_DYN_EXE) --version

debugdynfinal: $(DBG_DYN_EXE)
	@$(DBG_DYN_EXE) --version
	@echo "The application has been built and is located: $(DBG_DYN_EXE)"

debugdynlibs:
	@$(MAKE) -s -C libs $(LIBS_GOAL) SUBDIRS="$(DBG_DYN_LIBS)" BUILDDIR=../../$(BUILDDIR)

$(DBG_DYN_EXE): $(DBG_DYN_OBJS) debugdynlibs
	@$(CC) $(DBG_DYN_LDPATH) $(DBG_DYN_LDFLAGS) $(DBG_DYN_OBJS) $(DBG_DYN_LIB_OBJS) $(DBG_DYN_EXT_LDLIBS) -o $@
	@echo "$(call short_path,$@) linked dynamically, not stripped"

$(DBG_DYN_OBJDIR)/%.o: $(SRC_DIR)/%.c $(DBG_DYN_HDRS) | $(DBG_DYN_OBJDIR)
	@$(CC) -c $(DBG_DYN_INCPATH) $(WFLAGS) $(DBG_DYN_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with dynamic debug flags"

$(DBG_DYN_OBJDIR):
	@mkdir -p $(DBG_DYN_OBJDIR)

#
# Coverage rules
#
coverage: test-coverage
precizer-coverage: $(COV_EXE) coveragefinal

coveragefinal: $(COV_EXE)
	@$(COV_EXE) --version
	@echo "The application has been built and is located: $(COV_EXE)"

coveragelibs:
	@$(MAKE) -s -C libs coverage SUBDIRS="$(LIBS)" BUILDDIR=../../$(BUILDDIR)

$(COV_EXE): $(COV_OBJS) coveragelibs
	@$(CC) $(STATIC) $(COV_LDPATH) $(COV_LDFLAGS) -o $@ $(COV_OBJS) $(COV_LIB_OBJS) $(COV_EXT_LDLIBS)
ifeq ($(UNAME_S),Darwin)
	@echo "$(call short_path,$@) linked dynamically, not stripped"
else
	@echo "$(call short_path,$@) linked statically, not stripped"
endif

$(COV_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(COV_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with coverage flags"

$(COV_OBJDIR):
	@mkdir -p $(COV_OBJDIR)

test-coverage:
	@$(MAKE) -s -C $(TEST_DIR) coverage

#
# Sanitize rules
#
run: sanitize
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(SNTZ_EXE) $(ARGS)

sanitize: $(SNTZ_EXE) sanitizefinal

sanitizefinal: $(SNTZ_EXE)
	@$(SNTZ_EXE) --version
	@echo "The application has been built and is located: $(SNTZ_EXE)"

sanitizelibs:
	@$(MAKE) -s -C libs sanitize SUBDIRS="$(LIBS)" BUILDDIR=../../$(BUILDDIR)

$(SNTZ_EXE): $(SNTZ_OBJS) sanitizelibs
	@$(CC) $(SNTZ_LDPATH) $(SNTZ_RPATH) $(SNTZ_LDFLAGS) $(SNTZ_OBJS) $(SNTZ_LIB_OBJS) $(SNTZ_EXT_LDLIBS) -o $@
	@echo "$(call short_path,$@) linked dynamically, not stripped"

$(SNTZ_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(SNTZ_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with sanitizer flags"

$(SNTZ_OBJDIR):
	@mkdir -p $(SNTZ_OBJDIR)

#
# Production rules
#
prod: production
production: banner-production

production-done: $(PROD_EXE) prodfinal

banner-production: production-done
	@$(BUILD_USAGE_BANNER)

prodfinal: $(PROD_EXE)
	@cp $(PROD_EXE) $(ROOT_EXE)
	@$(UPX) $(ROOT_EXE)
	@$(ROOT_EXE) --version
	@echo "The $(PROD_EXE) has been copied to the current directory"

prodlibs:
	@$(MAKE) -s -C libs production SUBDIRS="$(LIBS)" BUILDDIR=../../$(BUILDDIR)

$(PROD_EXE): $(PROD_OBJS) prodlibs
	@$(CC) $(STATIC) $(STRIP) $(PROD_LDPATH) $(PROD_LDFLAGS) -o $@ $(PROD_OBJS) $(PROD_LIB_OBJS) $(PROD_EXT_LDLIBS)
ifeq ($(UNAME_S),Darwin)
	@echo "$(call short_path,$@) linked dynamically, stripped"
else
	@echo "$(call short_path,$@) linked statically, stripped"
endif

$(PROD_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PROD_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(PROD_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with release flags"

$(PROD_OBJDIR):
	@mkdir -p $(PROD_OBJDIR)

#
# Dynamic production rules
#
dynamic-production: banner-dynamic-production

dynamic-production-build: $(DYNP_EXE)
	@$(DYNP_EXE) --version

banner-dynamic-production: dynprodfinal
	@$(BUILD_USAGE_BANNER)

dynprodfinal: dynamic-production-build
	@cp $(DYNP_EXE) $(ROOT_EXE)
	@$(UPX) $(ROOT_EXE)
	@$(ROOT_EXE) --version
	@echo "The $(DYNP_EXE) has been copied to the current directory"

dynprodlibs:
	@$(MAKE) -s -C libs production SUBDIRS="$(STATLIBS)" BUILDDIR=../../$(BUILDDIR)

$(DYNP_EXE): $(DYNP_OBJS) dynprodlibs
	@$(CC) $(STRIP) $(DYNP_LDPATH) $(DYNP_LDFLAGS) -o $@ $(DYNP_OBJS) $(DYNP_LIB_OBJS) $(DYNP_SHARED_LIBS)
	@echo "$(call short_path,$@) linked dynamically, stripped"

$(DYNP_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(DYNP_OBJDIR)
	@$(CC) -c $(DYNAMIC_INCPATH) $(WFLAGS) $(DYNP_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with release flags"

$(DYNP_OBJDIR):
	@mkdir -p $(DYNP_OBJDIR)

#
# Portable rules
#
portable: banner-portable

portable-done: $(PRTB_EXE) portfinal

banner-portable: portable-done
	@$(BUILD_USAGE_BANNER)

portfinal: $(PRTB_EXE)
	@cp $(PRTB_EXE) $(ROOT_EXE)
	@$(UPX) $(ROOT_EXE)
	@$(ROOT_EXE) --version
	@echo "The $(PRTB_EXE) has been copied to the current directory"

portablelibs:
	@$(MAKE) -s -C libs portable SUBDIRS="$(LIBS)" BUILDDIR=../../$(BUILDDIR)

$(PRTB_EXE): $(PRTB_OBJS) portablelibs
	@$(CC) $(STRIP) $(STATIC) $(PRTB_LDPATH) $(PRTB_LDFLAGS) -o $@ $(PRTB_OBJS) $(PRTB_LIB_OBJS) $(PRTB_EXT_LDLIBS)
ifeq ($(UNAME_S),Darwin)
	@echo "$(call short_path,$@) linked dynamically, stripped"
else
	@echo "$(call short_path,$@) linked statically, stripped"
endif

$(PRTB_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PRTB_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(PRTB_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with portable flags"

$(PRTB_OBJDIR):
	@mkdir -p $(PRTB_OBJDIR)

clean-compile-commands:
	@rm -f $(COMPILE_COMMANDS)

clean: | clean-preproc clean-asm clean-tests clean-compile-commands
	@rm -f *.out.* doc
	@rm -f $(DBG_EXE) $(DBG_DYN_EXE) $(COV_EXE) $(SNTZ_EXE) $(PRTB_EXE) $(PROD_EXE) $(DYNP_EXE)
	@rm -f $(SNTZ_OBJS) $(DBG_OBJS) $(DBG_DYN_OBJS) $(COV_OBJS) $(PRTB_OBJS) $(PROD_OBJS) $(DYNP_OBJS)

	@test -d $(DBG_LIBDIR)/obj && rm -d $(DBG_LIBDIR)/obj 2>/dev/null || true
	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true
	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

	@test -d $(DBG_DYN_OBJDIR) && rm -d $(DBG_DYN_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DYN_DIR) && rm -d $(DBG_DYN_DIR) 2>/dev/null || true

	@test -d $(COV_LIBDIR)/obj && rm -d $(COV_LIBDIR)/obj 2>/dev/null || true
	@test -d $(COV_LIBDIR) && rm -d $(COV_LIBDIR) 2>/dev/null || true
	@test -d $(COV_OBJDIR) && rm -d $(COV_OBJDIR) 2>/dev/null || true
	@test -d $(COV_DIR) && rm -d $(COV_DIR) 2>/dev/null || true

	@test -d $(SNTZ_LIBDIR)/obj && rm -d $(SNTZ_LIBDIR)/obj 2>/dev/null || true
	@test -d $(SNTZ_LIBDIR) && rm -d $(SNTZ_LIBDIR) 2>/dev/null || true
	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true

	@test -d $(PROD_LIBDIR)/obj && rm -d $(PROD_LIBDIR)/obj 2>/dev/null || true
	@test -d $(PROD_LIBDIR) && rm -d $(PROD_LIBDIR) 2>/dev/null || true
	@test -d $(PROD_OBJDIR) && rm -d $(PROD_OBJDIR) 2>/dev/null || true
	@test -d $(PROD_DIR) && rm -d $(PROD_DIR) 2>/dev/null || true

	@test -d $(DYNP_OBJDIR) && rm -d $(DYNP_OBJDIR) 2>/dev/null || true
	@test -d $(DYNP_DIR) && rm -d $(DYNP_DIR) 2>/dev/null || true

	@test -d $(PRTB_LIBDIR)/obj && rm -d $(PRTB_LIBDIR)/obj 2>/dev/null || true
	@test -d $(PRTB_LIBDIR) && rm -d $(PRTB_LIBDIR) 2>/dev/null || true
	@test -d $(PRTB_OBJDIR) && rm -d $(PRTB_OBJDIR) 2>/dev/null || true
	@test -d $(PRTB_DIR) && rm -d $(PRTB_DIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true

	@test -f $(EXE) && rm $(EXE) || true
	@echo $(EXE) cleared

purge: clean-compile-commands
	@test -d $(BUILDDIR) && rm -rf $(BUILDDIR) 2>/dev/null || true
	@test -f $(EXE) && rm $(EXE) 2>/dev/null || true
	@$(MAKE) -s -C libs purge
	@$(MAKE) -C $(TEST_DIR) clean-hugetestfile
	@echo $(TEST_DIR) huge test file artifacts cleared
	@echo $(EXE) artifacts cleared

clean-all: clean-tests clean clean-tools clean-docker
	@$(MAKE) -C libs clean

clean-tools:
	@$(MAKE) -C $(TOOLS_DIR) clean

clean-tests:
	@$(MAKE) -C $(TEST_DIR) clean

clean-preproc:
	@rm -rf $(PREPROC)

clean-asm:
	@rm -rf $(ASM)

test: tests
tests: tests-sanitize
tests-sanitize:
	@$(MAKE) -s -C $(TEST_DIR) sanitize

tests-debug:
	@$(MAKE) -s -C $(TEST_DIR) debug

# Run the debug test suite without static linking to avoid requiring static external libraries
tests-dynamic:
	@$(MAKE) -s -C $(TEST_DIR) debug-dynamic

#
# Build and test within a Docker container
#

# Defaults for high-level docker targets
DOCKER_DEFAULT_OS    ?= ubuntu
DOCKER_DEFAULT_BUILD ?= production

# You can override these:
#   make docker-export-alpine-portable
#   make docker-run-ubuntu-dynamic-production
#   make docker-arch                  (build defaults to production)
DOCKER_OS    ?= $(DOCKER_DEFAULT_OS)
DOCKER_BUILD ?= $(DOCKER_DEFAULT_BUILD)

# Optional compiler override passed into the container (e.g. DOCKER_COMPILER=clang).
# Empty means the Dockerfile/OS default compiler is used.
# Do not pass the literal value "default". Leave DOCKER_COMPILER empty to use
# the default compiler selected by the Dockerfile or OS
DOCKER_COMPILER ?=
DOCKER_COMPILER_TAG = $(if $(DOCKER_COMPILER),-$(DOCKER_COMPILER),)

# Optional test-type override (e.g. DOCKER_TEST_TYPE=tests-debug).
# Empty means the Dockerfile default (ENV TEST_TYPE) is used.
DOCKER_TEST_TYPE ?=
DOCKER_TEST_TYPE_TAG = $(if $(DOCKER_TEST_TYPE),-$(DOCKER_TEST_TYPE),)

# Normalize Docker host architecture for per-platform build customization
DOCKER_HOST_MACHINE ?= $(firstword $(subst -, ,$(MAKE_HOST)))
ifeq ($(DOCKER_HOST_MACHINE),x86_64)
DOCKER_PLATFORM_ARCH ?= amd64
else ifeq ($(DOCKER_HOST_MACHINE),aarch64)
DOCKER_PLATFORM_ARCH ?= arm64
else ifeq ($(DOCKER_HOST_MACHINE),arm64)
DOCKER_PLATFORM_ARCH ?= arm64
else
DOCKER_PLATFORM_ARCH ?= $(DOCKER_HOST_MACHINE)
endif

# Extra arguments passed to the Dockerfile build-time make invocation
# Per-OS and architecture overrides follow DOCKER_BUILD_MAKE_ARGS_<os>_<arch>
# UPX=true disables UPX for platform combinations where compressed binaries
# currently crash at runtime
# TODO: Remove these workarounds after UPX-compressed binaries run reliably
DOCKER_BUILD_MAKE_ARGS_alpine_amd64 ?= UPX=true
DOCKER_BUILD_MAKE_ARGS_ubuntu_arm64 ?= UPX=true
DOCKER_BUILD_MAKE_ARGS ?= $(DOCKER_BUILD_MAKE_ARGS_$(DOCKER_OS)_$(DOCKER_PLATFORM_ARCH))

DOCKER_IMAGE     = $(EXE):$(DOCKER_OS)-$(DOCKER_BUILD)$(DOCKER_COMPILER_TAG)
# Make the container name unique per OS/build/compiler/test-type to avoid clobbering
DOCKER_CONTAINER = $(EXE)-$(DOCKER_OS)-$(DOCKER_BUILD)$(DOCKER_COMPILER_TAG)$(DOCKER_TEST_TYPE_TAG)

DOCKERFILE            = .docker/Dockerfile.$(DOCKER_OS)
# Docker flags are empty by default so automation targets do not require a TTY.
# DOCKER_CREATE_FLAGS applies to docker-{export,run,test,all}-* because they use docker create/start.
# DOCKER_RUN_FLAGS applies only to tests-in-docker because it uses docker run directly.
# To enable interactive mode for docker-run-ubuntu-production, for example:
#   make DOCKER_CREATE_FLAGS=-it docker-run-ubuntu-production
# To pass flags to the direct docker run loop, for example:
#   make DOCKER_RUN_FLAGS=-it tests-in-docker
DOCKER_CREATE_FLAGS  ?=
DOCKER_RUN_FLAGS     ?=
DOCKER_ARTIFACT_PATH ?= /$(EXE)/$(EXE)
# Labels mark Docker artifacts created by this project.
# A single key/value pair keeps filtering simple during cleanup
DOCKER_LABEL_KEY    ?= io.github.precizer
DOCKER_LABEL_VALUE  ?= 1
DOCKER_LABEL_ARGS    = --label "$(DOCKER_LABEL_KEY)=$(DOCKER_LABEL_VALUE)"

.PHONY: build-docker create-docker start-docker copy-from-docker
.PHONY: docker-export docker-run docker-test docker-all docker docker-% docker-export-% docker-run-% docker-build-% docker-test-%
.PHONY: clean-docker clean-docker-image clean-docker-project clean-docker-project-deep docker-nuke-host tests-in-docker

# Build the image
build-docker:
	@test -f "$(DOCKERFILE)" || { echo "No Dockerfile: $(DOCKERFILE)"; exit 2; }
	@docker build $(DOCKER_LABEL_ARGS) -f "$(DOCKERFILE)" --build-arg BUILD="$(DOCKER_BUILD)" --build-arg COMPILER="$(DOCKER_COMPILER)" $(if $(DOCKER_BUILD_MAKE_ARGS),--build-arg MAKE_ARGS="$(DOCKER_BUILD_MAKE_ARGS)",) -t "$(DOCKER_IMAGE)" .

# Create a named container from the image (same container will be used for run+copy)
create-docker:
	@docker rm -f "$(DOCKER_CONTAINER)" > /dev/null 2>&1 || true
	@docker create $(DOCKER_CREATE_FLAGS) $(DOCKER_LABEL_ARGS) $(if $(DOCKER_TEST_TYPE),-e TEST_TYPE="$(DOCKER_TEST_TYPE)",) --name "$(DOCKER_CONTAINER)" "$(DOCKER_IMAGE)" > /dev/null

# Start the created container and attach to it
# Note: this runs the image's default CMD/ENTRYPOINT
start-docker:
	@docker start -ai "$(DOCKER_CONTAINER)"

# Copy the built artifact out of the container
copy-from-docker:
	@docker cp "$(DOCKER_CONTAINER):$(DOCKER_ARTIFACT_PATH)" "$(EXE)"

# Remove the container
clean-docker:
	@docker rm -f "$(DOCKER_CONTAINER)" > /dev/null 2>&1 || true

# Remove the image (and prune dangling layers)
clean-docker-image:
	@docker image rm -f "$(DOCKER_IMAGE)" > /dev/null 2>&1 || true
	@docker image prune -f > /dev/null 2>&1 || true
	@echo Docker image $(DOCKER_IMAGE) cleared

# Remove Docker containers and images created by this project.
# Dangling layers left behind by image removal are pruned afterwards
clean-docker-project:
	@docker rm -f $$(docker ps -aq --filter "label=$(DOCKER_LABEL_KEY)=$(DOCKER_LABEL_VALUE)") > /dev/null 2>&1 || true; \
	docker image rm -f $$(docker images -q --filter "label=$(DOCKER_LABEL_KEY)=$(DOCKER_LABEL_VALUE)") > /dev/null 2>&1 || true; \
	docker image prune -f > /dev/null 2>&1 || true; \
	echo Docker artifacts for label $(DOCKER_LABEL_KEY)=$(DOCKER_LABEL_VALUE) cleared

# Extend project cleanup by also removing base images referenced by .docker/Dockerfile.*
clean-docker-project-deep: clean-docker-project
	@for image in $(DOCKER_BASE_IMAGES); do \
		docker image rm -f "$$image" > /dev/null 2>&1 || true; \
	done; \
	docker image prune -f > /dev/null 2>&1 || true; \
	echo Docker base images from .docker cleared

# Remove all Docker artifacts from the host.
# This target is intentionally destructive and is not scoped to this project
docker-nuke-host:
	@docker rm -f $$(docker ps -aq) > /dev/null 2>&1 || true
	@docker rmi -f $$(docker images -q) > /dev/null 2>&1 || true
	@docker image prune -af > /dev/null 2>&1 || true
	@echo All Docker artifacts on the host cleared

#
# Ordered pipelines (guaranteed sequence)
#

# Build -> Create -> Copy -> Clean container -> Clean image
docker-export:
	@rc=0; \
	$(MAKE) build-docker && \
	$(MAKE) create-docker && \
	$(MAKE) copy-from-docker || rc=$$?; \
	$(MAKE) clean-docker; \
	$(MAKE) clean-docker-image; \
	exit $$rc

# Build -> Create -> Run (attach) -> Clean container -> Clean image
docker-run:
	@rc=0; \
	$(MAKE) build-docker && \
	$(MAKE) create-docker && \
	$(MAKE) start-docker || rc=$$?; \
	$(MAKE) clean-docker; \
	$(MAKE) clean-docker-image; \
	exit $$rc

# Create -> Run (attach) -> Clean container (image must already exist)
docker-test:
	@rc=0; \
	$(MAKE) create-docker && \
	$(MAKE) start-docker || rc=$$?; \
	$(MAKE) clean-docker; \
	exit $$rc

# Build -> Create -> Run -> Copy -> Clean container -> Clean image
docker-all:
	@rc=0; \
	$(MAKE) build-docker && \
	$(MAKE) create-docker && \
	$(MAKE) start-docker && \
	$(MAKE) copy-from-docker || rc=$$?; \
	$(MAKE) clean-docker; \
	$(MAKE) clean-docker-image; \
	exit $$rc

# Run the image in a fresh throwaway container 1000 times (build once, then run many)
tests-in-docker: build-docker
	@rc=0; \
	i=1; while [ $$i -le 1000 ]; do \
		docker run $(DOCKER_RUN_FLAGS) --rm "$(DOCKER_IMAGE)" || { rc=$$?; break; }; \
		i=$$((i + 1)); \
	done; \
	$(MAKE) clean-docker-image; \
	exit $$rc

#
# Generic docker targets
#
# Supported:
#   make docker                           -> docker-export (defaults)
#   make docker-arch                      -> docker-all for arch + default build
#   make docker-ubuntu-dynamic-production -> docker-all
#   make docker-export-alpine-portable    -> docker-export
#   make docker-run-gentoo-production     -> docker-run
# Example variants for alpine / portable / default / tests-debug:
#   make docker-run-alpine-portable DOCKER_TEST_TYPE=tests-debug
#       build image -> create container -> run tests -> cleanup
#   make docker-build-alpine-portable
#       build image only (no container run)
#   make docker-test-alpine-debug DOCKER_TEST_TYPE=tests-debug
#       create container -> run tests -> cleanup (image must already exist)
#   make docker-alpine-portable DOCKER_TEST_TYPE=tests-debug
#       build image -> create container -> run tests -> copy ./precizer -> cleanup
#
docker: docker-export-$(DOCKER_DEFAULT_OS)-$(DOCKER_DEFAULT_BUILD)

# $1 = "ubuntu" or "ubuntu-dynamic-production"
# Match $1 against known OS names discovered from .docker/Dockerfile.*
docker_os_from_target    = $(firstword $(foreach os,$(DOCKER_OSES),$(if $(filter $(os) $(os)-%,$1),$(os))))
docker_build_from_target = $(if $(filter $(call docker_os_from_target,$1),$1),$(DOCKER_DEFAULT_BUILD),$(patsubst $(call docker_os_from_target,$1)-%,%,$1))

# Default "docker-<os>[-<build>]" does the full pipeline (run + copy + cleanup)
docker-%:
	@$(MAKE) docker-all \
	DOCKER_OS=$(call docker_os_from_target,$*) \
	DOCKER_BUILD=$(call docker_build_from_target,$*)

# Explicit pipelines
docker-export-%:
	@$(MAKE) docker-export \
	DOCKER_OS=$(call docker_os_from_target,$*) \
	DOCKER_BUILD=$(call docker_build_from_target,$*)

docker-run-%:
	@$(MAKE) docker-run \
	DOCKER_OS=$(call docker_os_from_target,$*) \
	DOCKER_BUILD=$(call docker_build_from_target,$*)

docker-build-%:
	@$(MAKE) build-docker \
	DOCKER_OS=$(call docker_os_from_target,$*) \
	DOCKER_BUILD=$(call docker_build_from_target,$*)

docker-test-%:
	@$(MAKE) docker-test \
	DOCKER_OS=$(call docker_os_from_target,$*) \
	DOCKER_BUILD=$(call docker_build_from_target,$*)

#
# Docker matrix: build all variants and run all test types
# for each OS found in .docker/
#
# Purpose
# -------
# These targets automatically discover all Dockerfiles in the .docker directory
# and run build+tests inside Docker containers for every supported OS and build
# flavor (portable/production/…).
#
# This helps ensure the project can be built and tested across multiple Linux
# distributions, build configurations (static, dynamic, debug, sanitize),
# compilers (default, clang), and test types (tests, tests-debug, tests-dynamic).
#
# How the OS list is discovered
# -----------------------------
# Put Dockerfiles under:
#   .docker/Dockerfile.<os>
#
# Example:
#   .docker/Dockerfile.ubuntu
#   .docker/Dockerfile.debian
#   .docker/Dockerfile.alpine
#
# Make extracts "<os>" from those filenames and forms DOCKER_OSES automatically.
#
# To print the detected OS list:
#   make print-docker-oses
#
# Which build flavors run per OS
# ------------------------------
# By default, each OS runs the following build types:
#   portable, production, dynamic-production, debug, sanitize
#
# Per-OS exclusions can remove specific builds (e.g. Alpine has no sanitizer):
#   DOCKER_MATRIX_BUILDS_EXCLUDE_alpine ?= sanitize
#
# The list is controlled by DOCKER_MATRIX_BUILDS and can be overridden:
#   make DOCKER_MATRIX_BUILDS="production debug" docker-check-every-os
#
# Main commands
# -------------
# 1) Run the full matrix for all OSes found in .docker:
#      make docker-check-every-os
#
# 2) Run the matrix for a single OS (example: ubuntu):
#      make docker-check-os-ubuntu
#
# 3) Run only specific build flavors and use one of them for tests:
#      make DOCKER_MATRIX_BUILDS="portable production" DOCKER_MATRIX_TEST_BUILD=production docker-check-every-os
#
# 4) Cleanup Docker artifacts for a single OS (containers/images for all flavors):
#      make clean-docker-os-ubuntu
#
# What runs inside the loop
# -------------------------
# For each OS and compiler, two separate loops run:
#   1) Builds: verify each build variant compiles (image only, no tests)
#   2) Tests:  run each test type on DOCKER_MATRIX_TEST_BUILD image
#
# For example (ubuntu, default compiler):
#   make docker-build-ubuntu-portable             # build only
#   make docker-build-ubuntu-production            # build only
#   make docker-build-ubuntu-dynamic-production    # build only
#   make docker-build-ubuntu-debug                 # build only
#   make docker-build-ubuntu-sanitize              # build only
#   make docker-test-ubuntu-debug  DOCKER_TEST_TYPE=tests          # test only
#   make docker-test-ubuntu-debug  DOCKER_TEST_TYPE=tests-debug    # test only
#   make docker-test-ubuntu-debug  DOCKER_TEST_TYPE=tests-dynamic  # test only
#   ... then the same for clang ...
#
# Cleanup behavior
# ----------------
# During the matrix run, images are preserved naturally: docker-build only
# creates images, docker-test only runs containers — neither removes images.
# This avoids re-downloading large base images (e.g. gentoo/stage3) between
# variants.
#
# After all variants complete for a given OS, cleanup is performed:
# - Images removed:      $(EXE):<os>-<build>[-<compiler>]  (all build variants)
# - Containers removed:  $(EXE)-<os>-<test-build>[-<compiler>]-<test-type>
# - docker image prune -f is executed (dangling layers)
#
# This is intentional to keep the workspace clean and runs reproducible.
#
# Notes
# -----
# - Requires Docker installed and usable by the current user (docker build/run).
# - If any build/test step fails, the matrix stops immediately and exits non-zero.

# Find OS list from .docker/Dockerfile.<os>
DOCKER_DOCKERFILES := $(wildcard .docker/Dockerfile.*)
# Collect unique base images from project Dockerfiles for deep cleanup
DOCKER_BASE_IMAGES = $(shell awk '/^FROM[[:space:]]/ { print $$2 }' $(DOCKER_DOCKERFILES) | sort -u)
DOCKER_OSES        := $(sort $(patsubst Dockerfile.%,%,$(notdir $(DOCKER_DOCKERFILES))))

# Build-time targets to run per OS (order matters).
# The list includes application builds and library-only build/test targets.
# Per-OS exclusions: define DOCKER_MATRIX_BUILDS_EXCLUDE_<os> to remove
# specific build types (e.g. Alpine has no sanitizer).
DOCKER_MATRIX_BUILDS ?= portable production dynamic-production debug sanitize precizer-coverage $(LIBS_MATRIX_BUILDS) $(LIBS_MATRIX_TESTS)
DOCKER_MATRIX_BUILDS_EXCLUDE_alpine ?= sanitize libs-sanitize libs-tests-sanitize

# Compilers to test per OS.  "default" means the OS-provided compiler
# (no COMPILER= override).  Additional entries (e.g. clang) are passed
# as DOCKER_COMPILER to the container so the Makefile picks them up.
DOCKER_MATRIX_COMPILERS ?= default clang

# Test variants to run per OS.  Each entry overrides ENV TEST_TYPE at
# container runtime (the image is not rebuilt).
# Per-OS exclusions: define DOCKER_MATRIX_TESTS_EXCLUDE_<os> to remove
# specific test types (e.g. Alpine has no sanitizer, so "tests" is excluded).
DOCKER_MATRIX_TESTS ?= tests tests-debug tests-dynamic
DOCKER_MATRIX_TESTS_EXCLUDE_alpine ?= tests

# Which build variant to use when running the test-only loop
DOCKER_MATRIX_TEST_BUILD ?= debug

print-docker-oses:
	@echo "$(DOCKER_OSES)"

# Run matrix for all OSes found
docker-check-every-os:
	@set -e; \
	if [ -z "$(DOCKER_OSES)" ]; then \
		echo "No .docker/Dockerfile.* found"; \
		exit 2; \
	fi; \
	for os in $(DOCKER_OSES); do \
		echo "=============================="; \
		echo " Docker matrix for: $$os"; \
		echo "=============================="; \
		$(MAKE) docker-check-os-$$os; \
	done

# Run all build variants and test types for a single OS, then cleanup.
# Two separate loops per compiler:
#   1) Builds: verify each build variant compiles (image only, no container run)
#   2) Tests:  run each test type on DOCKER_MATRIX_TEST_BUILD image (no rebuild)
# Images are preserved until clean-docker-os-% runs at the end.
# Per-OS exclusions: DOCKER_MATRIX_{BUILDS,TESTS}_EXCLUDE_<os>.
docker-check-os-%:
	@rc=0; \
	os="$*"; \
	for compiler in $(DOCKER_MATRIX_COMPILERS); do \
		dc=""; \
		if [ "$$compiler" != "default" ]; then dc="$$compiler"; fi; \
		for b in $(filter-out $(DOCKER_MATRIX_BUILDS_EXCLUDE_$*),$(DOCKER_MATRIX_BUILDS)); do \
			echo "---- $$os / $$b / $${compiler} / build ----"; \
			$(MAKE) docker-build-$$os-$$b DOCKER_COMPILER=$$dc || { rc=$$?; break 2; }; \
		done; \
		for t in $(filter-out $(DOCKER_MATRIX_TESTS_EXCLUDE_$*),$(DOCKER_MATRIX_TESTS)); do \
			echo "---- $$os / $(DOCKER_MATRIX_TEST_BUILD) / $${compiler} / $$t ----"; \
			$(MAKE) docker-test-$$os-$(DOCKER_MATRIX_TEST_BUILD) DOCKER_COMPILER=$$dc DOCKER_TEST_TYPE=$$t || { rc=$$?; break 2; }; \
		done; \
	done; \
	$(MAKE) clean-docker-os-$$os; \
	exit $$rc

# Cleanup all images and containers for a single OS
clean-docker-os-%:
	@os="$*"; \
	for compiler in $(DOCKER_MATRIX_COMPILERS); do \
		ctag=""; \
		if [ "$$compiler" != "default" ]; then ctag="-$$compiler"; fi; \
		for b in $(filter-out $(DOCKER_MATRIX_BUILDS_EXCLUDE_$*),$(DOCKER_MATRIX_BUILDS)); do \
			docker image rm -f "$(EXE):$$os-$$b$$ctag" >/dev/null 2>&1 || true; \
		done; \
		for t in $(filter-out $(DOCKER_MATRIX_TESTS_EXCLUDE_$*),$(DOCKER_MATRIX_TESTS)); do \
			docker rm -f "$(EXE)-$$os-$(DOCKER_MATRIX_TEST_BUILD)$$ctag-$$t" >/dev/null 2>&1 || true; \
		done; \
	done; \
	docker image prune -f >/dev/null 2>&1 || true; \
	echo "Docker artifacts for $$os cleared"

#
# Format rules
#
format:
	@echo "Formatting source files..."
	@for file in $(SRCS) $(HDRS); do \
		echo "Formatting $$file"; \
		uncrustify -c Uncrustify.cfg --replace --no-backup $$file; \
	done
	@echo "All files formatted."

# Optional preprocessor files
%.i:%.c
	@rm -f $@ $@.h
	@$(CC) -E -C -o $@ $(INCPATH) $(CFLAGS) $<
# C-C++ Beautifier
#	@bcpp -na $@ > $@.h
	@bcpp -na -s -i 4 $@ > $@.h
	@sed -i 's/[ \t]*\# [[:digit:]]\+ \".*//g' $@.h
#	@sed -i '/^ *$//d' $@.h

# Optional assembler files
%.asm:%.c
	@rm -f $@
	@$(CC) -S -C $(INCPATH) $(WFLAGS) $(PROD_CFLAGS) $(PROD_LDFLAGS) -o $@ $(LDLIBS) $<

#
# Other rules
#

remake: clean all

# Static analyzers and sanitizers
analyze: sanitize clang-analyzer cachegrind callgrind massif cppcheck memtest gcc-analyzer perf
static-analyzers: gcc-analyzer cppcheck clang-analyzer
static-analyzers-cli: gcc-analyzer cppcheck clang-analyzer-cli

#
# GCC Static Analysis
#
gcc-analyzer: DBG_CFLAGS += -fanalyzer -fno-analyzer-state-purge -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3
# -Wanalyzer-too-complex
gcc-analyzer: LIBS_GOAL = gcc-analyzer
gcc-analyzer: CC = gcc
gcc-analyzer: debug

compile-commands: $(COMPILE_COMMANDS)

$(COMPILE_COMMANDS):
	@mkdir -p "$(BUILDDIR)"
	bear --output $@ -- $(MAKE) -B -s -C $(TEST_DIR) debug-build

cppcheck: compile-commands
	cppcheck --project=$(COMPILE_COMMANDS) --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force -i libs/sqlite3/src --inconclusive --check-level=exhaustive

memtest: debug
	valgrind -v --tool=memcheck --leak-check=full --leak-resolution=high --undef-value-errors=no --show-reachable=yes --num-callers=20 $(DBG_DIR)/$(EXE) $(ARGS)

cachegrind: debug
	valgrind --tool=cachegrind --branch-sim=yes $(DBG_DIR)/$(EXE) $(ARGS)

callgrind: debug
	valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes $(DBG_DIR)/$(EXE) $(ARGS)

helgrind: debug
	valgrind --tool=helgrind --read-var-info=yes --track-origins=yes --num-callers=20 $(DBG_DIR)/$(EXE) $(ARGS)

massif: debug
	valgrind --tool=massif --stacks=yes --num-callers=20 $(DBG_DIR)/$(EXE) $(ARGS)
	ms_print ./massif.out.*

# Run clang static analyzer and view analysis results in a web browser when the build command completes
clang-analyzer: CC = $(CLANG)
clang-analyzer:
	@echo "Using compiler: $(CLANG), analyzer: $(SCAN_BUILD)"
	$(SCAN_BUILD) --exclude libs/sqlite3 -V $(MAKE) debug

clang-analyzer-cli: CC = $(CLANG)
clang-analyzer-cli:
	@echo "Using compiler: $(CLANG), analyzer: $(SCAN_BUILD)"
	$(SCAN_BUILD) --exclude libs/sqlite3 $(MAKE) debug

doc:
	@doxygen Doxyfile

spellcheck:
	@~/.cargo/bin/typos libs/sha512/ libs/rational/ libs/mem/ src/ README.md README.ru.md TODO

gource:
	gource --seconds-per-day 0.1 --auto-skip-seconds 1

#https://eax.me/c-cpp-profiling/
#https://perf.wiki.kernel.org/index.php/Main_Page
perf:
	sudo perf stat $(DBG_DIR)/$(EXE) $(ARGS)

# Code statistics and line counts
stat: cloc
cloc:
#	@cloc --exclude-dir=$(SNTZ_DIR),$(DBG_DIR),$(PROD_DIR) $(PRTB_DIR) ./src
	@cloc $(SRC_DIR) libs/sha512/src/ libs/mem/src/ libs/rational/src/ libs/testitall/src/

banner:
	@$(BUILD_USAGE_BANNER)

#
# Print variables
#
# If you want to find out the value of a makefile variable:
#   make print-VARIABLE
# Output:
#   VARIABLE = the_value_of_the_variable
#
print-%:
	@echo '$* = $($*)'
