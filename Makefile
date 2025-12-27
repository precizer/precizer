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

#
# Compiler flags
#

CFLAGS += -pipe -std=c2x -finline-functions
CFLAGS += -fbuiltin

# To pass a #define into the build:
# make DEFINES=-DWRITE_CSV=false memtest
CFLAGS += $(DEFINES)

LIBS = sha512 mem rational
EXTRA_LIBS = $(LIBS) sqlite3
LDLIBS = $(foreach d,$(EXTRA_LIBS),-l$d)

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

SRC = src
STRIP = -Wl,-s
ifeq ($(UNAME_S),Darwin)
STRIP =
endif

STATIC = -static -static-libgcc -Wl,--gc-sections

# UPX compression (disabled on macOS)
UPX ?= upx --best --lzma -qqq
ifeq ($(UNAME_S),Darwin)
UPX = true
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
ARGS = tests/examples/diffs

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered

# Test directory
TESTDIR = tests

# Tools directory
TOOLSDIR = tools

# Extra libs for linking
LDLIBS += -lpcre2-8

# For old gcc versions
GCC_VERSION := $(shell gcc -dumpversion)
# Checking if the GCC version is less than 10
ifeq ($(shell expr $(GCC_VERSION) \< 10), 1)
LDLIBS += -pthread
endif

# Additional include headers of external libraries
DYNAMIC_INCPATH += $(foreach d,$(LIBS),-Ilibs/$d/src/)
INCPATH += $(foreach d,$(EXTRA_LIBS),-Ilibs/$d/src/)

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
clang: CC = clang
clang: all

#
# Project files
#
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h)

# Exclude a file
OBJS = $(SRCS:.c=.o)
PREPROC = $(SRCS:.c=.i) # Preproc files http://www.viva64.com/en/t/0076/
PREPROC += $(SRCS:.c=.i.h)
# Asm
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
DBG_CFLAGS = $(CFLAGS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -fno-omit-frame-pointer -DDEBUG
DBG_LDFLAGS = -Wl,-z,defs -Wl,--as-needed
ifeq ($(UNAME_S),Darwin)
DBG_LDFLAGS = -Wl,-undefined,dynamic_lookup
endif
# Activation of the Gprof profiler.
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
COV_CFLAGS = $(CFLAGS) -fprofile-arcs -ftest-coverage -g -O0 -fno-omit-frame-pointer -DDEBUG
COV_LDFLAGS =  -lgcov --coverage

#
# Sanitize build settings
#
SNTZ_DIR = $(BUILDDIR)/sanitize
SNTZ_LIBDIR = $(SNTZ_DIR)/libs
SNTZ_OBJDIR = $(SNTZ_DIR)/obj
SNTZ_LDPATH = -L$(SNTZ_LIBDIR) $(LDPATH)
SNTZ_EXE = $(SNTZ_DIR)/$(EXE)
SNTZ_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(SNTZ_LIBDIR),-rpath,\$$ORIGIN/libs,-rpath,\$$ORIGIN/../debug/libs
ifeq ($(UNAME_S),Darwin)
SNTZ_RPATH = -Wl,-rpath,@executable_path/$(SNTZ_LIBDIR),-rpath,@executable_path/libs,-rpath,@executable_path/../debug/libs
endif
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(OBJS)))
SNTZ_OPTIONS = -fsanitize=address,undefined -static-libasan -fno-omit-frame-pointer
SNTZ_CFLAGS = $(DBG_CFLAGS) $(SNTZ_OPTIONS)
SNTZ_LDFLAGS = -Wl,-z,defs $(SNTZ_OPTIONS)
ifeq ($(UNAME_S),Darwin)
SNTZ_OPTIONS = -fsanitize=address,undefined -fno-omit-frame-pointer
SNTZ_LDFLAGS = $(SNTZ_OPTIONS)
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
PROD_CFLAGS = $(CFLAGS) -flto=auto -O3 -march=native -funroll-loops -pipe -ffunction-sections -fdata-sections -fomit-frame-pointer -DNDEBUG
PROD_LDFLAGS = -flto=auto -Wl,-O3 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,--gc-sections -Wl,-z,defs
ifeq ($(UNAME_S),Darwin)
PROD_LDFLAGS = -flto=auto -Wl,-O3 -Wl,-dead_strip -Wl,-x
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
DYNP_STATIC_LIBS = $(addprefix $(PROD_LIBDIR)/lib,$(addsuffix .a,$(LIBS)))
DYNP_SHARED_LIBS = $(filter-out $(addprefix -l,$(LIBS)),$(LDLIBS))

#
# Portable build settings
#
PRTB_DIR = $(BUILDDIR)/portable
PRTB_LIBDIR = $(PRTB_DIR)/libs
PRTB_OBJDIR = $(PRTB_DIR)/obj
PRTB_LDPATH = -L$(PRTB_LIBDIR) $(LDPATH)
PRTB_EXE = $(PRTB_DIR)/$(EXE)
PRTB_OBJS = $(addprefix $(PRTB_OBJDIR)/, $(notdir $(OBJS)))
PRTB_CFLAGS = $(CFLAGS) -flto=auto -O2 -mtune=generic -funroll-loops -pipe -ffunction-sections -fdata-sections -fomit-frame-pointer -DNDEBUG
PRTB_LDFLAGS = -flto=auto -Wl,-O2 -Wl,--hash-style=both -Wl,--as-needed -Wl,--gc-sections -Wl,-z,defs
ifeq ($(UNAME_S),Darwin)
PRTB_LDFLAGS = -flto=auto -Wl,-O2 -Wl,-dead_strip -Wl,-x
endif

# https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory
TOPTARGETS := all

.PHONY: all clean debug remake clang tests sanitize banner run format portable production prod dynamic-production debugfinal prodfinal sanitizefinal dynprodfinal portfinal coverage
.PHONY: purge clean-all clean-tools clean-tests clean-preproc clean-asm clean-docker clean-docker-image clean-all-dockers test test-coverage tests-sanitize tests-debug docker docker-portable build-docker build-docker-portable copy-from-docker run-docker tests-in-docker analyze gcc-analyzer cppcheck memtest cachegrind callgrind helgrind massif sparse-analyzer clang-analyzer splint doc spellcheck gource perf stat cloc coveragefinal precizer-coverage print-%

#
# Debug rules
#
debug: $(DBG_LIBDIR) $(DBG_EXE) debugfinal

debugfinal: $(DBG_EXE)
	@echo "The application has been built and is located: $(DBG_EXE)"

$(DBG_EXE): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(CC) $(STATIC) $(DBG_LDPATH) $(DBG_LDFLAGS) -o $@ $^ $(LDLIBS)
	@echo "$@ linked"

$(DBG_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(DBG_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(DBG_CFLAGS) -o $@ $<
	@echo "$< compiled"

$(DBG_OBJDIR):
	@mkdir -p $(DBG_OBJDIR)

$(DBG_LIBDIR):
	@$(MAKE) -s -C libs debug

#
# Coverage rules
#
coverage: $(COV_LIBDIR) $(COV_EXE) coveragefinal | test-coverage
precizer-coverage: $(COV_LIBDIR) $(COV_EXE) coveragefinal

coveragefinal: $(COV_EXE)
	@echo "The application has been built and is located: $(COV_EXE)"

$(COV_EXE): $(COV_OBJS) | $(COV_LIBDIR)
	@$(CC) $(STATIC) $(COV_LDPATH) $(COV_LDFLAGS) -o $@ $^ $(LDLIBS)
	@echo "$@ linked"

$(COV_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(COV_CFLAGS) -o $@ $<
	@echo "$< compiled"

$(COV_OBJDIR):
	@mkdir -p $(COV_OBJDIR)

$(COV_LIBDIR):
	@$(MAKE) -s -C libs coverage

test-coverage:
	@$(MAKE) -s -C $(TESTDIR) coverage

#
# Sanitize rules
#
run: sanitize
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(SNTZ_EXE) $(ARGS)

sanitize: $(SNTZ_LIBDIR) $(SNTZ_EXE) sanitizefinal

sanitizefinal: $(SNTZ_EXE)
	@echo "The application has been built and is located: $(SNTZ_EXE)"

$(SNTZ_EXE): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(CC) $(SNTZ_LDPATH) $(SNTZ_RPATH) $(SNTZ_LDFLAGS) -o $@ $^ $(LDLIBS)
	@echo "$@ linked"

$(SNTZ_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(SNTZ_CFLAGS) -o $@ $<
	@echo "$< compiled"

$(SNTZ_OBJDIR):
	@mkdir -p $(SNTZ_OBJDIR)

$(SNTZ_LIBDIR):
	@$(MAKE) -s -C libs sanitize

#
# Production rules
#
prod: production
production: $(PROD_LIBDIR) $(PROD_EXE) prodfinal banner

prodfinal: $(PROD_EXE)
	@cp $(PROD_EXE) $(EXE)
	@$(UPX) $(EXE)
	@echo "The $(PROD_EXE) has been copied to the current directory"

$(PROD_EXE): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(CC) $(STATIC) $(STRIP) $(PROD_LDPATH) $(PROD_LDFLAGS) -o $@ $^ $(LDLIBS)
	@strip -x $(PROD_EXE)
	@echo "$@ linked"

$(PROD_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(PROD_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(PROD_CFLAGS) -o $@ $<
	@echo "$< compiled"

$(PROD_OBJDIR):
	@mkdir -p $(PROD_OBJDIR)

$(PROD_LIBDIR):
	@$(MAKE) -s -C libs production

#
# Dynamic production rules
#
dynamic-production: $(PROD_LIBDIR) $(DYNP_EXE) dynprodfinal banner

dynprodfinal: $(DYNP_EXE)
	@cp $(DYNP_EXE) $(EXE)
	@$(UPX) $(EXE)
	@echo "The $(DYNP_EXE) has been copied to the current directory"

$(DYNP_EXE): $(DYNP_OBJS)
	@$(CC) $(STRIP) $(DYNP_LDPATH) $(DYNP_LDFLAGS) -o $@ $^ $(DYNP_STATIC_LIBS) $(DYNP_SHARED_LIBS)
	@strip -x $(DYNP_EXE)
	@echo "$@ linked"

$(DYNP_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(DYNP_OBJDIR)
	@$(CC) -c $(DYNAMIC_INCPATH) $(WFLAGS) $(DYNP_CFLAGS) -o $@ $<
	@echo "$< compiled"

$(DYNP_OBJDIR):
	@mkdir -p $(DYNP_OBJDIR)

#
# Portable rules
#
portable: $(PRTB_LIBDIR) $(PRTB_EXE) portfinal banner

portfinal: $(PRTB_EXE)
	@cp $(PRTB_EXE) $(EXE)
	@$(UPX) $(EXE)
	@echo "The $(PRTB_EXE) has been copied to the current directory"

$(PRTB_EXE): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(CC) $(STRIP) $(STATIC) $(PRTB_LDPATH) $(PRTB_LDFLAGS) -o $@ $^ $(LDLIBS)
	@strip -x $(PRTB_EXE)
	@echo "$@ linked"

$(PRTB_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(PRTB_OBJDIR)
	@$(CC) -c $(INCPATH) $(WFLAGS) $(PRTB_CFLAGS) -o $@ $<
	@echo "$< compiled"

$(PRTB_OBJDIR):
	@mkdir -p $(PRTB_OBJDIR)

$(PRTB_LIBDIR):
	@$(MAKE) -s -C libs portable

clean: | clean-preproc clean-asm clean-tests
	@rm -f *.out.* doc
	@rm -f $(DBG_EXE) $(COV_EXE) $(SNTZ_EXE) $(PRTB_EXE) $(PROD_EXE) $(DYNP_EXE)
	@rm -f $(SNTZ_OBJS) $(DBG_OBJS) $(COV_OBJS) $(PRTB_OBJS) $(PROD_OBJS) $(DYNP_OBJS)

	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true
	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true

	@test -d $(COV_OBJDIR) && rm -d $(COV_OBJDIR) 2>/dev/null || true
	@test -d $(COV_DIR) && rm -d $(COV_DIR) 2>/dev/null || true
	@test -d $(COV_LIBDIR) && rm -d $(COV_LIBDIR) 2>/dev/null || true

	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true
	@test -d $(SNTZ_LIBDIR) && rm -d $(SNTZ_LIBDIR) 2>/dev/null || true

	@test -d $(PROD_OBJDIR) && rm -d $(PROD_OBJDIR) 2>/dev/null || true
	@test -d $(PROD_DIR) && rm -d $(PROD_DIR) 2>/dev/null || true
	@test -d $(PROD_LIBDIR) && rm -d $(PROD_LIBDIR) 2>/dev/null || true

	@test -d $(DYNP_OBJDIR) && rm -d $(DYNP_OBJDIR) 2>/dev/null || true
	@test -d $(DYNP_DIR) && rm -d $(DYNP_DIR) 2>/dev/null || true

	@test -d $(PRTB_OBJDIR) && rm -d $(PRTB_OBJDIR) 2>/dev/null || true
	@test -d $(PRTB_DIR) && rm -d $(PRTB_DIR) 2>/dev/null || true
	@test -d $(PRTB_LIBDIR) && rm -d $(PRTB_LIBDIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true

	@test -f $(EXE) && rm $(EXE) || true
	@echo $(EXE) cleared

purge:
	@test -d $(BUILDDIR) && rm -rf $(BUILDDIR) 2>/dev/null || true

clean-all: clean-tests clean clean-tools clean-docker
	@$(MAKE) -C libs clean

clean-tools:
	@$(MAKE) -C $(TOOLSDIR) clean

clean-tests:
	@$(MAKE) -C $(TESTDIR) clean

clean-preproc:
	@rm -rf $(PREPROC)

clean-asm:
	@rm -rf $(ASM)

test: tests
tests: tests-sanitize
tests-sanitize: sanitize
	@$(MAKE) -s -C $(TESTDIR) sanitize

tests-debug: debug
	@$(MAKE) -s -C $(TESTDIR) debug

#
# Build and test within Docker container
#

# gentoo | ubuntu | arch | ...
DOCKER_OS ?= gentoo

# production | portable
DOCKER_BUILD ?= production

DOCKER_IMAGE = $(EXE):$(DOCKER_OS)-$(DOCKER_BUILD)
DOCKER_CONTAINER = $(EXE)

# Build image and create application container
build-docker:
	@docker build -f .docker/Dockerfile.$(DOCKER_OS) --build-arg BUILD=$(DOCKER_BUILD) -t $(DOCKER_IMAGE) .
	@docker rm -f $(DOCKER_CONTAINER) > /dev/null 2>&1 || true
	@docker create --name $(DOCKER_CONTAINER) $(DOCKER_IMAGE)

# Run the application within the built container
run-docker:
	@docker run -it --rm $(DOCKER_IMAGE)

# Copying a statically compiled application from a container
# to the current directory on the system
copy-from-docker:
	@docker cp $(EXE):/$(EXE)/$(EXE) $(EXE)

# Run it 1000 times
tests-in-docker: build-docker
	i=1; while [ $$i -le 1000 ]; do docker -f .docker/Dockerfile.$(DOCKER_OS) run -it --rm $(DOCKER_IMAGE) || break; i=$$((i + 1)); done

# Clean the built container
clean-docker:
	@docker rm -f $(DOCKER_CONTAINER) > /dev/null 2>&1 || true

clean-docker-image:
	@docker image rm -f $(DOCKER_IMAGE) > /dev/null 2>&1 || true
	@docker image prune -f > /dev/null 2>&1 || true
	@echo Docker image $(EXE) cleared

clean-all-dockers:
	@docker image prune -f > /dev/null 2>&1 || true
	@docker image prune -af > /dev/null 2>&1 || true
	@docker rm -f $(shell docker ps -aq) > /dev/null 2>&1 || true
	@docker rmi -f $(shell docker images -q) > /dev/null 2>&1 || true
	@echo All docker images cleared

# Generic Docker targets: use any .docker/Dockerfile.<os>
# Requires .docker/Dockerfile.<os> to exist.
# Examples:
#   make docker-arch
#   make docker-alpine-portable
#   make docker-gentoo-dynamic-production
docker: docker-ubuntu

docker-portable: docker-ubuntu-portable

docker-dynamic-production: docker-ubuntu-dynamic-production

docker-start-build: build-docker run-docker copy-from-docker clean-docker clean-docker-image

docker-ubuntu: docker-ubuntu-production

docker-ubuntu-portable: DOCKER_OS=ubuntu
docker-ubuntu-portable: DOCKER_BUILD=portable
docker-ubuntu-portable: docker-start-build

docker-ubuntu-production: DOCKER_OS=ubuntu
docker-ubuntu-production: DOCKER_BUILD=production
docker-ubuntu-production: docker-start-build

docker-ubuntu-dynamic-production: DOCKER_OS=ubuntu
docker-ubuntu-dynamic-production: DOCKER_BUILD=dynamic-production
docker-ubuntu-dynamic-production: docker-start-build

docker-debian: docker-debian-production

docker-debian-portable: DOCKER_OS=debian
docker-debian-portable: DOCKER_BUILD=portable
docker-debian-portable: docker-start-build

docker-debian-production: DOCKER_OS=debian
docker-debian-production: DOCKER_BUILD=production
docker-debian-production: docker-start-build

docker-debian-dynamic-production: DOCKER_OS=debian
docker-debian-dynamic-production: DOCKER_BUILD=dynamic-production
docker-debian-dynamic-production: docker-start-build

docker-gentoo: docker-gentoo-production

docker-gentoo-portable: DOCKER_OS=gentoo
docker-gentoo-portable: DOCKER_BUILD=portable
docker-gentoo-portable: docker-start-build

docker-gentoo-production: DOCKER_OS=gentoo
docker-gentoo-production: DOCKER_BUILD=production
docker-gentoo-production: docker-start-build

docker-gentoo-dynamic-production: DOCKER_OS=gentoo
docker-gentoo-dynamic-production: DOCKER_BUILD=dynamic-production
docker-gentoo-dynamic-production: docker-start-build

docker-arch: docker-arch-production

docker-arch-portable: DOCKER_OS=arch
docker-arch-portable: DOCKER_BUILD=portable
docker-arch-portable: docker-start-build

docker-arch-production: DOCKER_OS=arch
docker-arch-production: DOCKER_BUILD=production
docker-arch-production: docker-start-build

docker-arch-dynamic-production: DOCKER_OS=arch
docker-arch-dynamic-production: DOCKER_BUILD=dynamic-production
docker-arch-dynamic-production: docker-start-build

docker-alpine: docker-alpine-production

docker-alpine-portable: DOCKER_OS=alpine
docker-alpine-portable: DOCKER_BUILD=portable
docker-alpine-portable: docker-start-build

docker-alpine-production: DOCKER_OS=alpine
docker-alpine-production: DOCKER_BUILD=production
docker-alpine-production: docker-start-build

docker-alpine-dynamic-production: DOCKER_OS=alpine
docker-alpine-dynamic-production: DOCKER_BUILD=dynamic-production
docker-alpine-dynamic-production: docker-start-build

docker-rocky: docker-rocky-production

docker-rocky-portable: DOCKER_OS=rocky
docker-rocky-portable: DOCKER_BUILD=portable
docker-rocky-portable: docker-start-build

docker-rocky-production: DOCKER_OS=rocky
docker-rocky-production: DOCKER_BUILD=production
docker-rocky-production: docker-start-build

docker-rocky-dynamic-production: DOCKER_OS=rocky
docker-rocky-dynamic-production: DOCKER_BUILD=dynamic-production
docker-rocky-dynamic-production: docker-start-build

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

# Optional Assembler files
%.asm:%.c
	@rm -f $@
	@$(CC) -S -C $(INCPATH) $(WFLAGS) $(PROD_CFLAGS) $(PROD_LDFLAGS) -o $@ $(LDLIBS) $<

#
# Other rules
#

remake: clean all

# Static analysers and sanitizers
analyze: sanitize clang-analyzer cachegrind callgrind massif cppcheck memtest gcc-analyzer perf

#
# GCC Static Analysis
#
gcc-analyzer: WFLAGS += -fanalyzer -fno-analyzer-state-purge -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3
# -Wanalyzer-too-complex
gcc-analyzer: CC = gcc
gcc-analyzer: debug

cppcheck:
	cppcheck --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force -i libs -i tests $(DYNAMIC_INCPATH) --inconclusive .

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

SPARSE=sparse
SPARSE_FLAGS=-Wsparse-all -nostdinc
sparse-analyzer:
	$(foreach src,$(SRCS),$(SPARSE) $(SPARSE_FLAGS) $(INCPATH) $(DBG_CFLAGS) $(DBG_LDPATH) $(WFLAGS) $(src);)

clang-analyzer: CC = clang-20
clang-analyzer: SCAN-BUILD = scan-build-20
clang-analyzer:
	# Run clang static analyzer and view analysis results in a web browser when the build command completes
	$(SCAN-BUILD) --exclude libs/sqlite3 -V $(MAKE) debug

splint:
	splint -I /usr/include/x86_64-linux-gnu +posixlib $(SRCS) $(INCPATH)

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
	@cloc ./src

banner:
	@printf "Now some tests could be running:\n"
	@printf "\033[1mStage 1. Adding:\033[0m\n./$(EXE) --progress --database=database1.db tests/examples/diffs/diff1\n"
	@printf "\033[1mStage 2. Adding:\033[0m\n./$(EXE) --progress --database=database2.db tests/examples/diffs/diff2\n"
	@printf "\033[1mFinal stage. Comparing:\033[0m\n./$(EXE) --compare database1.db database2.db\n"

#
# Print of variables
#
# If you want to find out the value of a makefile variable, just:
#make print-VARIABLE
# and it will return:
#VARIABLE = the_value_of_the_variable
#
print-%:
	@echo '$* = $($*)'
