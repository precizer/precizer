# How to install dependencies and build the app:
#
# GCC
# sudo apt -y install gcc make libpcre2-dev llvm
#
# LLVM for sanitizer
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
COMPILE_COMMANDS = compile_commands.json

#
# Compiler flags
#

CFLAGS += -pipe -std=c2x -finline-functions
CFLAGS += -fbuiltin

# To pass a #define into the build:
# make DEFINES=-DWRITE_CSV=false memtest
CFLAGS += $(DEFINES)

LIBS = sha512 mem rational sqlite3
STATLIBS = sha512 mem rational
LDLIBS = $(foreach d,$(LIBS),-l$d)

SYS := $(shell gcc -dumpmachine)
ifneq (, $(findstring alpine, $(SYS)))
# Alpine Linux uses external libraries
LDLIBS += -largp -lfts
endif

UNAME_S := $(shell uname -s)

# macOS-specific code
ifeq ($(UNAME_S),Darwin)
CFLAGS += -DEVIL_EMPIRE_OS
endif

# Detect whether we're using GCC (covers names like arm-linux-gnu-gcc)
GCC := $(findstring gcc,$(notdir $(firstword $(CC))))

EXE = precizer
ifeq ($(UNAME_S),Darwin)
STATIC =
STRIP ?= -Wl,-x
else
# tests-dynamic disables static linking so the debug test build can use shared libraries
ifneq ($(TESTS_DYNAMIC),)
STATIC =
else
STATIC = -static -static-libgcc -Wl,--gc-sections
endif
STRIP ?= -s
endif

# UPX compression (disabled on macOS)
ifeq ($(UNAME_S),Darwin)
UPX = true
else
UPX ?= upx --best --lzma -qqq
endif

# Warning flags for additional checks
WFLAGS += -Werror # Stop the build on any errors
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

# Extra libs for linking
LDLIBS += -lpcre2-8

# Additional include headers of external libraries
DYNAMIC_INCPATH += $(foreach d,$(STATLIBS),-Ilibs/$d/src/)
INCPATH += $(foreach d,$(LIBS),-Ilibs/$d/src/)

ifeq ($(UNAME_S),Darwin)
#DYNAMIC_INCPATH += $(shell pkg-config --cflags libpcre2-8)
#INCPATH += $(shell pkg-config --cflags libpcre2-8)
# argp lib
DYNAMIC_INCPATH += -I/opt/homebrew/include
INCPATH += -I/opt/homebrew/include
LDPATH += -L/opt/homebrew/lib
LDLIBS += -largp
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
endif

#
# Project files
#
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h)

# Exclude a file
OBJS = $(SRCS:.c=.o)
PREPROC = $(SRCS:.c=.i) # Preprocessed files http://www.viva64.com/en/t/0076/
PREPROC += $(SRCS:.c=.i.h)
# Assembly
ASM = $(SRCS:.c=.asm)

#
# Debug build settings
#
DBG_DIR = $(BUILDDIR)/debug
DBG_LIBDIR = $(DBG_DIR)/libs
DBG_OBJDIR = $(DBG_DIR)/obj
DBG_LDPATH = -L$(DBG_LIBDIR) $(LDPATH)
DBG_EXE = $(DBG_DIR)/$(EXE)
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(OBJS)))
DBG_CFLAGS = $(CFLAGS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -fno-omit-frame-pointer -DDEBUG -DTESTITALL_TEST_HOOKS
LIBS_GOAL ?= debug
ifeq ($(UNAME_S),Darwin)
DBG_RPATH = -Wl,-rpath,@executable_path/$(DBG_LIBDIR),-rpath,@executable_path/libs
DBG_LDFLAGS = -Wl,-undefined,dynamic_lookup
else
DBG_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBG_LIBDIR),-rpath,\$$ORIGIN/libs
DBG_LDFLAGS = -Wl,-z,defs -Wl,--as-needed
endif
# tests-dynamic injects a debug-only rpath so copied test binaries can find shared libraries
ifneq ($(TESTS_DYNAMIC),)
DBG_LINK_RPATH = $(DBG_RPATH)
else
DBG_LINK_RPATH =
endif
# Activate the Gprof profiler.
# Works incorrectly with Valgrind.
# It is better to use Callgrind - the call graph format
# is supported by visualization tools like kcachegrind.
#DBG_CFLAGS += -pg

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
COV_CFLAGS = $(CFLAGS) -fprofile-arcs -ftest-coverage -g -O0 -fno-omit-frame-pointer -DDEBUG -DTESTITALL_TEST_HOOKS
COV_LDFLAGS = -lgcov --coverage

#
# Sanitize build settings
#
SNTZ_DIR = $(BUILDDIR)/sanitize
SNTZ_LIBDIR = $(SNTZ_DIR)/libs
SNTZ_OBJDIR = $(SNTZ_DIR)/obj
SNTZ_LDPATH = -L$(SNTZ_LIBDIR) $(LDPATH)
SNTZ_EXE = $(SNTZ_DIR)/$(EXE)
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(OBJS)))
SNTZ_OPTIONS = -fsanitize=address,undefined -fno-omit-frame-pointer
SNTZ_CFLAGS = $(DBG_CFLAGS) $(SNTZ_OPTIONS)
ifeq ($(UNAME_S),Darwin)
SNTZ_RPATH = -Wl,-rpath,@executable_path/$(SNTZ_LIBDIR),-rpath,@executable_path/libs,-rpath,@executable_path/../debug/libs
SNTZ_LDFLAGS = -Wl,-undefined,dynamic_lookup $(SNTZ_OPTIONS)
else
SNTZ_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(SNTZ_LIBDIR),-rpath,\$$ORIGIN/libs,-rpath,\$$ORIGIN/../debug/libs
SNTZ_LDFLAGS = -Wl,-z,defs $(SNTZ_OPTIONS)
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
PROD_CFLAGS ?= $(CFLAGS) -flto=auto -O3 -march=native -funroll-loops -pipe -ffunction-sections -fdata-sections -fomit-frame-pointer
ifeq ($(UNAME_S),Darwin)
PROD_LDFLAGS ?= $(LDFLAGS) -flto=auto -Wl,-O3 -Wl,-dead_strip
else
PROD_LDFLAGS ?= $(LDFLAGS) -flto=auto -Wl,-O3 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,--gc-sections -Wl,-z,defs
endif

#
# Dynamic production build settings
#
DYNP_DIR = $(BUILDDIR)/dynamic-production
DYNP_OBJDIR = $(DYNP_DIR)/obj
DYNP_LDPATH = $(LDPATH)
DYNP_EXE = $(DYNP_DIR)/$(EXE)
DYNP_OBJS = $(addprefix $(DYNP_OBJDIR)/, $(notdir $(OBJS)))
DYNP_CFLAGS = $(PROD_CFLAGS)
DYNP_LDFLAGS = $(PROD_LDFLAGS)
DYNP_STATIC_LIBS = $(addprefix $(PROD_LIBDIR)/lib,$(addsuffix .a,$(STATLIBS)))
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
PRTB_CFLAGS = $(CFLAGS) -flto=auto -O2 -mtune=generic -funroll-loops -pipe -ffunction-sections -fdata-sections -fomit-frame-pointer
ifeq ($(UNAME_S),Darwin)
PRTB_LDFLAGS = -flto=auto -Wl,-O2 -Wl,-dead_strip
else
PRTB_LDFLAGS = -flto=auto -Wl,-O2 -Wl,--hash-style=both -Wl,--as-needed -Wl,--gc-sections -Wl,-z,defs
endif

# https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory
TOPTARGETS := all

define BUILD_USAGE_BANNER
printf "Now some tests could be running:\n"
printf "\033[1mStage 1. Adding:\033[0m\n./$(EXE) --progress --database=database1.db tests/fixtures/diffs/diff1\n"
printf "\033[1mStage 2. Adding:\033[0m\n./$(EXE) --progress --database=database2.db tests/fixtures/diffs/diff2\n"
printf "\033[1mFinal stage. Comparing:\033[0m\n./$(EXE) --compare database1.db database2.db\n"
endef

.PHONY: all clean debug remake tests sanitize banner run format portable production prod dynamic-production dynamic-production-build debuglibs coveragelibs sanitizelibs prodlibs dynprodlibs portablelibs debugfinal prodfinal sanitizefinal dynprodfinal portfinal coverage coveragefinal precizer-coverage print-%
.PHONY: production-done portable-done
.PHONY: banner-production banner-dynamic-production banner-portable
.PHONY: purge clean-all clean-tools clean-tests clean-preproc clean-asm clean-docker clean-docker-image test test-coverage tests-sanitize tests-debug tests-dynamic docker docker-portable docker-dynamic-production docker-start-build build-docker copy-from-docker run-docker tests-in-docker analyze static-analyzers static-analyzers-cli gcc-analyzer cppcheck memtest cachegrind callgrind helgrind massif clang-analyzer clang-analyzer-cli doc spellcheck gource perf stat cloc
.PHONY: docker-check-every-os docker-check-os-% clean-docker-os-% print-docker-oses

#
# Debug rules
#
debug: $(DBG_EXE) debugfinal

debugfinal: $(DBG_EXE)
	@echo "The application has been built and is located: $(DBG_EXE)"

debuglibs:
	@$(MAKE) -s -C libs $(LIBS_GOAL) SUBDIRS="$(LIBS)"

$(DBG_EXE): $(DBG_OBJS) debuglibs
	@$(CC) $(STATIC) $(DBG_LDPATH) $(DBG_LINK_RPATH) $(DBG_LDFLAGS) -o $@ $(DBG_OBJS) $(LDLIBS)
ifeq ($(UNAME_S),Darwin)
	@echo "$@ linked dynamically, not stripped"
else
	@echo "$@ linked statically, not stripped"
endif

$(DBG_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(DBG_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(DBG_CFLAGS) -o $@ $<
	@echo "$< compiled with debug flags"

$(DBG_OBJDIR):
	@mkdir -p $(DBG_OBJDIR)

#
# Coverage rules
#
coverage: test-coverage
precizer-coverage: $(COV_EXE) coveragefinal

coveragefinal: $(COV_EXE)
	@echo "The application has been built and is located: $(COV_EXE)"

coveragelibs:
	@$(MAKE) -s -C libs coverage SUBDIRS="$(LIBS)"

$(COV_EXE): $(COV_OBJS) coveragelibs
	@$(CC) $(STATIC) $(COV_LDPATH) $(COV_LDFLAGS) -o $@ $(COV_OBJS) $(LDLIBS)
ifeq ($(UNAME_S),Darwin)
	@echo "$@ linked dynamically, not stripped"
else
	@echo "$@ linked statically, not stripped"
endif

$(COV_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(COV_CFLAGS) -o $@ $<
	@echo "$< compiled with coverage flags"

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
	@echo "The application has been built and is located: $(SNTZ_EXE)"

sanitizelibs:
	@$(MAKE) -s -C libs sanitize SUBDIRS="$(LIBS)"

$(SNTZ_EXE): $(SNTZ_OBJS) sanitizelibs
	@$(CC) $(SNTZ_LDPATH) $(SNTZ_RPATH) $(SNTZ_LDFLAGS) -o $@ $(SNTZ_OBJS) $(LDLIBS)
	@echo "$@ linked dynamically, not stripped"

$(SNTZ_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(SNTZ_CFLAGS) -o $@ $<
	@echo "$< compiled with sanitizer flags"

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
	@cp $(PROD_EXE) $(EXE)
	@$(UPX) $(EXE)
	@echo "The $(PROD_EXE) has been copied to the current directory"

prodlibs:
	@$(MAKE) -s -C libs production SUBDIRS="$(LIBS)"

$(PROD_EXE): $(PROD_OBJS) prodlibs
	@$(CC) $(STATIC) $(STRIP) $(PROD_LDPATH) $(PROD_LDFLAGS) -o $@ $(PROD_OBJS) $(LDLIBS)
ifeq ($(UNAME_S),Darwin)
	@echo "$@ linked dynamically, stripped"
else
	@echo "$@ linked statically, stripped"
endif

$(PROD_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PROD_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(PROD_CFLAGS) -o $@ $<
	@echo "$< compiled with release flags"

$(PROD_OBJDIR):
	@mkdir -p $(PROD_OBJDIR)

#
# Dynamic production rules
#
dynamic-production: banner-dynamic-production

dynamic-production-build: $(DYNP_EXE)

banner-dynamic-production: dynprodfinal
	@$(BUILD_USAGE_BANNER)

dynprodfinal: dynamic-production-build
	@cp $(DYNP_EXE) $(EXE)
	@$(UPX) $(EXE)
	@echo "The $(DYNP_EXE) has been copied to the current directory"

dynprodlibs:
	@$(MAKE) -s -C libs production SUBDIRS="$(STATLIBS)"

$(DYNP_EXE): $(DYNP_OBJS) dynprodlibs
	@$(CC) $(STRIP) $(DYNP_LDPATH) $(DYNP_LDFLAGS) -o $@ $(DYNP_OBJS) $(DYNP_STATIC_LIBS) $(DYNP_SHARED_LIBS)
	@echo "$@ linked dynamically, stripped"

$(DYNP_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(DYNP_OBJDIR)
	@$(CC) -c $(DYNAMIC_INCPATH) $(WFLAGS) $(DYNP_CFLAGS) -o $@ $<
	@echo "$< compiled with release flags"

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
	@cp $(PRTB_EXE) $(EXE)
	@$(UPX) $(EXE)
	@echo "The $(PRTB_EXE) has been copied to the current directory"

portablelibs:
	@$(MAKE) -s -C libs portable SUBDIRS="$(LIBS)"

$(PRTB_EXE): $(PRTB_OBJS) portablelibs
	@$(CC) $(STRIP) $(STATIC) $(PRTB_LDPATH) $(PRTB_LDFLAGS) -o $@ $(PRTB_OBJS) $(LDLIBS)
ifeq ($(UNAME_S),Darwin)
	@echo "$@ linked dynamically, stripped"
else
	@echo "$@ linked statically, stripped"
endif

$(PRTB_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PRTB_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(PRTB_CFLAGS) -o $@ $<
	@echo "$< compiled with portable flags"

$(PRTB_OBJDIR):
	@mkdir -p $(PRTB_OBJDIR)

clean: | clean-preproc clean-asm clean-tests
	@rm -f *.out.* doc
	@rm -f $(COMPILE_COMMANDS)
	@rm -f $(DBG_EXE) $(COV_EXE) $(SNTZ_EXE) $(PRTB_EXE) $(PROD_EXE) $(DYNP_EXE)
	@rm -f $(SNTZ_OBJS) $(DBG_OBJS) $(COV_OBJS) $(PRTB_OBJS) $(PROD_OBJS) $(DYNP_OBJS)

	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true
	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

	@test -d $(COV_LIBDIR) && rm -d $(COV_LIBDIR) 2>/dev/null || true
	@test -d $(COV_OBJDIR) && rm -d $(COV_OBJDIR) 2>/dev/null || true
	@test -d $(COV_DIR) && rm -d $(COV_DIR) 2>/dev/null || true

	@test -d $(SNTZ_LIBDIR) && rm -d $(SNTZ_LIBDIR) 2>/dev/null || true
	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true

	@test -d $(PROD_LIBDIR) && rm -d $(PROD_LIBDIR) 2>/dev/null || true
	@test -d $(PROD_OBJDIR) && rm -d $(PROD_OBJDIR) 2>/dev/null || true
	@test -d $(PROD_DIR) && rm -d $(PROD_DIR) 2>/dev/null || true

	@test -d $(DYNP_OBJDIR) && rm -d $(DYNP_OBJDIR) 2>/dev/null || true
	@test -d $(DYNP_DIR) && rm -d $(DYNP_DIR) 2>/dev/null || true

	@test -d $(PRTB_LIBDIR) && rm -d $(PRTB_LIBDIR) 2>/dev/null || true
	@test -d $(PRTB_OBJDIR) && rm -d $(PRTB_OBJDIR) 2>/dev/null || true
	@test -d $(PRTB_DIR) && rm -d $(PRTB_DIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true

	@test -f $(EXE) && rm $(EXE) || true
	@echo $(EXE) cleared

purge:
	@test -d $(BUILDDIR) && rm -rf $(BUILDDIR) 2>/dev/null || true
	@test -f $(COMPILE_COMMANDS) && rm $(COMPILE_COMMANDS) 2>/dev/null || true
	@test -f $(EXE) && rm $(EXE) 2>/dev/null || true
	@$(MAKE) -C $(TEST_DIR) clean-hugetestfile
	@echo Quick cleanup of all artifacts

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
	@$(MAKE) -s -C $(TEST_DIR) debug TESTS_DYNAMIC=1

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
DOCKER_COMPILER ?=
DOCKER_COMPILER_TAG = $(if $(DOCKER_COMPILER),-$(DOCKER_COMPILER),)

# Optional test-type override (e.g. DOCKER_TEST_TYPE=tests-debug).
# Empty means the Dockerfile default (ENV TEST_TYPE) is used.
DOCKER_TEST_TYPE ?=
DOCKER_TEST_TYPE_TAG = $(if $(DOCKER_TEST_TYPE),-$(DOCKER_TEST_TYPE),)

DOCKER_IMAGE     = $(EXE):$(DOCKER_OS)-$(DOCKER_BUILD)$(DOCKER_COMPILER_TAG)
# Make the container name unique per OS/build/compiler/test-type to avoid clobbering
DOCKER_CONTAINER = $(EXE)-$(DOCKER_OS)-$(DOCKER_BUILD)$(DOCKER_COMPILER_TAG)$(DOCKER_TEST_TYPE_TAG)

DOCKERFILE            = .docker/Dockerfile.$(DOCKER_OS)
DOCKER_CREATE_FLAGS  ?= -it
DOCKER_RUN_FLAGS     ?= -it
DOCKER_ARTIFACT_PATH ?= /$(EXE)/$(EXE)

# When non-empty, pipeline targets (docker-run, docker-export, docker-all) skip
# image removal so that the base layer cache is preserved across repeated builds
# for the same OS.  The caller is responsible for cleaning up afterwards.
# Used by docker-check-os-% to avoid re-downloading large base images between
# build variants of the same OS.
DOCKER_KEEP_IMAGE ?=

.PHONY: build-docker create-docker start-docker copy-from-docker
.PHONY: docker-export docker-run docker-all docker docker-% docker-export-% docker-run-%
.PHONY: clean-docker clean-docker-image clean-all-docker tests-in-docker

# Build the image
build-docker:
	@test -f "$(DOCKERFILE)" || { echo "No Dockerfile: $(DOCKERFILE)"; exit 2; }
	@docker build -f "$(DOCKERFILE)" --build-arg BUILD="$(DOCKER_BUILD)" --build-arg COMPILER="$(DOCKER_COMPILER)" -t "$(DOCKER_IMAGE)" .

# Create a named container from the image (same container will be used for run+copy)
create-docker:
	@docker rm -f "$(DOCKER_CONTAINER)" > /dev/null 2>&1 || true
	@docker create $(DOCKER_CREATE_FLAGS) $(if $(DOCKER_TEST_TYPE),-e TEST_TYPE="$(DOCKER_TEST_TYPE)",) --name "$(DOCKER_CONTAINER)" "$(DOCKER_IMAGE)" > /dev/null

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

clean-all-docker:
	@docker image prune -f > /dev/null 2>&1 || true
	@docker image prune -af > /dev/null 2>&1 || true
	@docker rm -f $(shell docker ps -aq) > /dev/null 2>&1 || true
	@docker rmi -f $(shell docker images -q) > /dev/null 2>&1 || true
	@echo All docker images cleared

#
# Ordered pipelines (guaranteed sequence)
#

# Build -> Create -> Copy -> Clean container [-> Clean image]
docker-export:
	@$(MAKE) build-docker
	@$(MAKE) create-docker
	@$(MAKE) copy-from-docker
	@$(MAKE) clean-docker
	$(if $(DOCKER_KEEP_IMAGE),,@$(MAKE) clean-docker-image)

# Build -> Create -> Run (attach) -> Clean container [-> Clean image]
docker-run:
	@$(MAKE) build-docker
	@$(MAKE) create-docker
	@$(MAKE) start-docker
	@$(MAKE) clean-docker
	$(if $(DOCKER_KEEP_IMAGE),,@$(MAKE) clean-docker-image)

# Build -> Create -> Run -> Copy -> Clean container [-> Clean image]
docker-all:
	@$(MAKE) build-docker
	@$(MAKE) create-docker
	@$(MAKE) start-docker
	@$(MAKE) copy-from-docker
	@$(MAKE) clean-docker
	$(if $(DOCKER_KEEP_IMAGE),,@$(MAKE) clean-docker-image)

# Run the image in a fresh throwaway container 1000 times (build once, then run many)
tests-in-docker: build-docker
	@i=1; while [ $$i -le 1000 ]; do \
		docker run $(DOCKER_RUN_FLAGS) --rm "$(DOCKER_IMAGE)" || break; \
		i=$$((i + 1)); \
	done

#
# Generic docker targets
#
# Supported:
#   make docker                           -> docker-export (defaults)
#   make docker-arch                      -> docker-all for arch + default build
#   make docker-ubuntu-dynamic-production -> docker-all
#   make docker-export-alpine-portable    -> docker-export
#   make docker-run-gentoo-production     -> docker-run
#
docker: docker-export-$(DOCKER_DEFAULT_OS)-$(DOCKER_DEFAULT_BUILD)

# $1 = "ubuntu" or "ubuntu-dynamic-production"
docker_os_from_target    = $(firstword $(subst -, ,$1))
docker_build_from_target = $(if $(findstring -,$1),$(patsubst $(call docker_os_from_target,$1)-%,%,$1),$(DOCKER_DEFAULT_BUILD))

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

#
# Docker matrix build & test: run all build variants
# for each OS found in .docker/
#
# Purpose
# -------
# These targets automatically discover all Dockerfiles in the .docker directory
# and run build+tests inside Docker containers for every supported OS and build
# flavor (portable/production/…).
#
# This helps ensure the project can be built and tested across multiple Linux
# distributions and build configurations (static, dynamic, debug).
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
# By default, each OS runs the following targets (in this order):
#   portable
#   production
#   dynamic-production
#   debug
#
# The list is controlled by DOCKER_MATRIX_BUILDS and can be overridden:
#   make DOCKER_MATRIX_BUILDS="production sanitize" docker-check-every-os
#
# Main commands
# -------------
# 1) Run the full matrix for all OSes found in .docker:
#      make docker-check-every-os
#
# 2) Run the matrix for a single OS (example: ubuntu):
#      make docker-check-os-ubuntu
#
# 3) Run only specific build flavors:
#      make DOCKER_MATRIX_BUILDS="portable production" docker-check-every-os
#
# 4) Cleanup Docker artifacts for a single OS (containers/images for all flavors):
#      make clean-docker-os-ubuntu
#
# What runs inside the loop
# -------------------------
# For each OS "<os>", the following commands are executed:
#   make docker-<os>-portable
#   make docker-<os>-production
#   make docker-<os>-dynamic-production
#   make docker-<os>-debug
#
# These targets already exist in this Makefile and use the docker-all pipeline:
#   build image -> create container -> run (tests) -> copy artifact -> cleanup
#
# Cleanup behavior
# ----------------
# During the matrix run, DOCKER_KEEP_IMAGE=1 is passed to each docker-run
# invocation so that built images (and the cached base layers they depend on)
# are preserved across build variants of the same OS.  This avoids
# re-downloading large base images (e.g. gentoo/stage3) between variants.
#
# After all build flavors complete for a given OS, cleanup is performed:
# - Containers removed:  $(EXE)-<os>-<build>
# - Images removed:      $(EXE):<os>-<build>
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
DOCKER_OSES        := $(sort $(patsubst Dockerfile.%,%,$(notdir $(DOCKER_DOCKERFILES))))

# Build variants to run per OS (order matters)
DOCKER_MATRIX_BUILDS ?= portable production dynamic-production debug sanitize

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

# Run all build variants for a single OS (with every compiler and test type),
# then cleanup.
# DOCKER_KEEP_IMAGE=1 prevents each docker-run invocation from removing the image
# (and its cached base layers) so the same base image is reused across build variants.
# All images are cleaned up at the end by clean-docker-os-%.
# Per-OS test list: DOCKER_MATRIX_TESTS minus DOCKER_MATRIX_TESTS_EXCLUDE_<os>.
docker-check-os-%:
	@set -e; \
	os="$*"; \
	for compiler in $(DOCKER_MATRIX_COMPILERS); do \
		dc=""; \
		if [ "$$compiler" != "default" ]; then dc="$$compiler"; fi; \
		for b in $(DOCKER_MATRIX_BUILDS); do \
			for t in $(filter-out $(DOCKER_MATRIX_TESTS_EXCLUDE_$*),$(DOCKER_MATRIX_TESTS)); do \
				echo "---- $$os / $$b / $${compiler} / $$t ----"; \
				$(MAKE) docker-run-$$os-$$b DOCKER_KEEP_IMAGE=1 DOCKER_COMPILER=$$dc DOCKER_TEST_TYPE=$$t; \
			done; \
		done; \
	done; \
	$(MAKE) clean-docker-os-$$os

# Cleanup all images/containers for a single OS (all build variants, compilers, test types)
clean-docker-os-%:
	@set -e; \
	os="$*"; \
	for compiler in $(DOCKER_MATRIX_COMPILERS); do \
		ctag=""; \
		if [ "$$compiler" != "default" ]; then ctag="-$$compiler"; fi; \
		for b in $(DOCKER_MATRIX_BUILDS); do \
			docker image rm -f "$(EXE):$$os-$$b$$ctag" >/dev/null 2>&1 || true; \
			for t in $(filter-out $(DOCKER_MATRIX_TESTS_EXCLUDE_$*),$(DOCKER_MATRIX_TESTS)); do \
				docker rm -f "$(EXE)-$$os-$$b$$ctag-$$t" >/dev/null 2>&1 || true; \
			done; \
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

cppcheck:
	bear --output $(COMPILE_COMMANDS) -- make -B -s .builds/production/precizer
	cppcheck --project=$(COMPILE_COMMANDS) --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force -i libs/sqlite3/src --inconclusive

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

clang-analyzer: CC = $(CLANG)
clang-analyzer:
	@echo "Using compiler: $(CLANG), analyzer: $(SCAN_BUILD)"
	# Run clang static analyzer and view analysis results in a web browser when the build command completes
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
