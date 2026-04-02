# Name of the application
EXE=testitall

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory

#
# Compiler flags
#

CFLAGS += -pipe -std=c2x -finline-functions
CFLAGS += -fbuiltin

UNAME_S := $(shell uname -s)

# macOS-specific code
ifeq ($(UNAME_S),Darwin)
CFLAGS += -DEVIL_EMPIRE_OS
endif

SRC = src
# tests-dynamic clears STATIC so the test runner uses shared libraries during debug runs
ifneq ($(TESTS_DYNAMIC),)
STATIC =
else
STATIC = -static -static-libgcc -Wl,--gc-sections
endif

APP_SRC_DIR = ../src
TESTED_APP_NAME := $(shell awk '$$2 == "APP_NAME" && $$3 ~ /^"/ { gsub(/"/,"",$$3); print $$3; exit }' $(APP_SRC_DIR)/version.h)
ROOT_BUILDDIR = ../.builds
BUILDDIR = $(ROOT_BUILDDIR)/$(EXE)
COMPILE_COMMANDS = compile_commands.json
LIBS = testitall mem sha512 rational sqlite3 xdiff

ifeq ($(TESTED_APP_NAME),)
$(error Failed to extract APP_NAME from $(APP_SRC_DIR)/version.h)
endif

# Extra libs for linking
LDLIBS += $(foreach d,$(LIBS),-l$d) -lpcre2-8
# cmocka is used in sanitizer/coverage builds.
CMOCKA_LDLIBS = -lcmocka

SYS := $(shell gcc -dumpmachine)
ifneq (, $(findstring alpine, $(SYS)))
# Alpine Linux uses external libraries
LDLIBS += -largp -lfts
endif

# For old gcc versions
GCC_VERSION := $(shell gcc -dumpversion)
# Checking if the GCC version is less than 10
ifeq ($(shell expr $(GCC_VERSION) \< 10), 1)
LDLIBS += -pthread
endif

# Additional include paths for external libraries
INCPATH += $(foreach d,$(LIBS),-I../libs/$d/src/)
## Source files of precizer project
INCPATH += -I$(APP_SRC_DIR)

ifeq ($(UNAME_S),Darwin)
INCPATH += $(shell pkg-config --cflags libpcre2-8)
# argp lib
INCPATH += -I/opt/homebrew/include
LDPATH += -L/opt/homebrew/lib
LDLIBS += -largp
endif

# Detect whether we're using GCC
GCC := $(findstring gcc,$(notdir $(firstword $(CC))))

# Warning flags for additional checks
WFLAGS += -Wall # Enable common warning set
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

# Link-time wrappers for cmocka tests (add more functions as needed).
WRAPFUNCS = fopen fread ferror remove
ifeq ($(UNAME_S),Darwin)
# macOS with  lld supports -wrap
WRAPFLAGS =
else
WRAPFLAGS = $(foreach f,$(WRAPFUNCS),-Wl,--wrap=$(f))
endif

#
# Project files
#
SRCS = $(wildcard $(SRC)/*.c)
TMPLTS = $(wildcard $(SRC)/*.cc)
HDRS = $(wildcard $(SRC)/*.h)
APP_HDRS = $(wildcard $(APP_SRC_DIR)/*.h)
ALLHDRS = $(HDRS) $(APP_HDRS)
OBJS = $(SRCS:.c=.o)
APP_SRCS = $(wildcard $(APP_SRC_DIR)/*.c)
APP_OBJS = $(APP_SRCS:.c=.o)

#
# Coverage build settings
#
COV = coverage
COV_DIR = $(BUILDDIR)/$(COV)
COV_EXE = $(COV_DIR)/$(EXE)
COV_OBJDIR = $(COV_DIR)/obj
COV_REPORTDIR = $(COV_OBJDIR)/../report/
COV_OBJS = $(addprefix $(COV_OBJDIR)/, $(notdir $(OBJS)))
COV_LIBDIR = $(ROOT_BUILDDIR)/$(COV)/libs
COV_APP_OBJDIR = $(ROOT_BUILDDIR)/$(COV)/obj
COV_LDPATH = -L$(COV_LIBDIR) $(LDPATH)
ifeq ($(UNAME_S),Darwin)
COV_RPATH = -Wl,-rpath,@executable_path/$(COV_LIBDIR),-rpath,@executable_path/../../$(COV_LIBDIR)
else
COV_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(COV_LIBDIR),-rpath,\$$ORIGIN/../../$(COV_LIBDIR)
endif

COV_APP_OBJS = $(addprefix $(COV_OBJDIR)/, $(notdir $(APP_OBJS)))
# Keep TESTITALL in the test runner build flags only.
# Adding it to the global CFLAGS leaks it into recursive builds such as
# `make -C ../ sanitize`, which disables main() in the standalone precizer
# binary and enables test-only hooks there
COV_CFLAGS = $(CFLAGS) -g -O0 -fprofile-arcs -ftest-coverage -DTESTITALL -DTESTITALL_TEST_HOOKS
COV_LDFLAGS = -lgcov --coverage

#
# Sanitize build settings
#
SNTZ = sanitize
SNTZ_DIR = $(BUILDDIR)/$(SNTZ)
SNTZ_EXE = $(SNTZ_DIR)/$(EXE)
SNTZ_OBJDIR = $(SNTZ_DIR)/obj
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(OBJS)))
SNTZ_APP_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(APP_OBJS)))
SNTZ_LIBDIR = $(ROOT_BUILDDIR)/$(SNTZ)/libs
SNTZ_LDPATH = -L$(SNTZ_LIBDIR) $(LDPATH)
ifeq ($(UNAME_S),Darwin)
SNTZ_RPATH = -Wl,-rpath,@executable_path/$(SNTZ_LIBDIR),-rpath,@executable_path/../../$(SNTZ_LIBDIR)
else
SNTZ_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(SNTZ_LIBDIR),-rpath,\$$ORIGIN/../../$(SNTZ_LIBDIR)
endif
SNTZ_OPTIONS = -fsanitize=address,undefined -fno-omit-frame-pointer
SNTZ_CFLAGS = $(DBG_CFLAGS) $(SNTZ_OPTIONS)
ifeq ($(UNAME_S),Darwin)
SNTZ_LDFLAGS = -Wl,-undefined,dynamic_lookup $(SNTZ_OPTIONS)
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
SNTZ_LDFLAGS = $(SNTZ_OPTIONS)
else
SNTZ_LDFLAGS = -Wl,-z,defs $(SNTZ_OPTIONS)
endif

#
# Debug build settings
#
DBG = debug
DBG_DIR = $(BUILDDIR)/$(DBG)
DBG_EXE = $(DBG_DIR)/$(EXE)
DBG_OBJDIR = $(DBG_DIR)/obj
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(OBJS)))
DBG_APP_OBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(APP_OBJS)))
DBG_LIBDIR = $(ROOT_BUILDDIR)/$(DBG)/libs
DBG_LDPATH = -L$(DBG_LIBDIR) $(LDPATH)
ifeq ($(UNAME_S),Darwin)
DBG_RPATH = -Wl,-rpath,@executable_path/$(DBG_LIBDIR),-rpath,@executable_path/../../$(DBG_LIBDIR)
else
DBG_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBG_LIBDIR),-rpath,\$$ORIGIN/../../$(DBG_LIBDIR)
endif
DBG_CFLAGS = $(CFLAGS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG -DTESTITALL -DTESTITALL_TEST_HOOKS

.PHONY: all clean debug sanitize coverage run test tests coverage-report coverage-html format
.PHONY: precizer-coverage precizer-sanitize precizer-debug hugetestfile purge static-analyzers static-analyzers-cli gcc-analyzer cppcheck clang-analyzer clang-analyzer-cli print-%
.PHONY: testlibs-coverage testlibs-sanitize testlibs-debug debug-build clean-compile-commands

# Default build
all: sanitize

test: sanitize

tests: test

run: sanitize

testlibs-coverage: precizer-coverage
	@$(MAKE) -s -C ../libs/xdiff coverage
	@$(MAKE) -s -C ../libs/testitall coverage TESTITALL_APP_NAME=$(TESTED_APP_NAME)

testlibs-sanitize: precizer-sanitize
	@$(MAKE) -s -C ../libs/xdiff sanitize
	@$(MAKE) -s -C ../libs/testitall sanitize TESTITALL_APP_NAME=$(TESTED_APP_NAME)

testlibs-debug: precizer-debug
	@$(MAKE) -s -C ../libs/xdiff debug
	@$(MAKE) -s -C ../libs/testitall debug TESTITALL_APP_NAME=$(TESTED_APP_NAME)

#
# Coverage rules
#
precizer-coverage:
	@$(MAKE) -s -C ../ precizer-coverage

coverage: hugetestfile coverage-html

$(COV_EXE): $(COV_OBJS) $(COV_APP_OBJS) | precizer-coverage testlibs-coverage
	@$(CC) $(COV_LDPATH) $(COV_LDFLAGS) $(COV_RPATH) $(WRAPFLAGS) -o $(COV_EXE) $^ $(LDLIBS) $(CMOCKA_LDLIBS)
	@echo "$@ linked."

$(COV_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(COV_OBJDIR)/%.o: $(APP_SRC_DIR)/%.c $(ALLHDRS) $(TMPLTS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(COV_OBJDIR):
	@mkdir -p "$(COV_OBJDIR)"

coverage-report: hugetestfile precizer-coverage $(COV_EXE)
	@echo "Generating coverage report..."
	./$(COV_EXE)
	@echo "Coverage run finished. Inspect .gcda/.gcno outputs for details"

coverage-html: coverage-report
	@echo "Generating HTML coverage report..."
	@lcov --capture --branch-coverage --rc lcov_excl_br_line='(^\s*RETURN_STATUS|SLOWTEST\s*;)|(\b(?:provide|ASSERT|TEST|SUTE)\s*\()' --directory $(COV_OBJDIR) --directory $(COV_APP_OBJDIR) --output-file $(COV_OBJDIR)/coverage.info
	@genhtml --branch-coverage $(COV_OBJDIR)/coverage.info --output-directory $(COV_REPORTDIR)
	@echo "HTML coverage report generated in $(COV_REPORTDIR) directory"

#
# Sanitize rules
#
precizer-sanitize:
	@$(MAKE) -s -C ../ sanitize

sanitize: precizer-sanitize hugetestfile $(SNTZ_EXE)
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(SNTZ_EXE)

$(SNTZ_EXE): $(SNTZ_OBJS) $(SNTZ_APP_OBJS) | precizer-sanitize testlibs-sanitize
	@$(CC) $(SNTZ_LDPATH) $(SNTZ_LDFLAGS) $(SNTZ_RPATH) $(WRAPFLAGS) -o $(SNTZ_EXE) $^ $(LDLIBS) $(CMOCKA_LDLIBS)
	@echo "$@ linked."

$(SNTZ_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(INCPATH) $(SNTZ_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(SNTZ_OBJDIR)/%.o: $(APP_SRC_DIR)/%.c $(ALLHDRS) $(TMPLTS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(INCPATH) $(SNTZ_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(SNTZ_OBJDIR):
	@mkdir -p "$(SNTZ_OBJDIR)"

#
# Debug rules
#
precizer-debug:
	@$(MAKE) -s -C ../ debug

debug-build: $(DBG_EXE)

debug: hugetestfile debug-build
	@./$(DBG_EXE)

$(DBG_EXE): $(DBG_OBJS) $(DBG_APP_OBJS) | precizer-debug testlibs-debug
	@$(CC) $(DBG_LDPATH) $(DBG_RPATH) $(STATIC) $(WRAPFLAGS) -o $(DBG_EXE) $^ $(LDLIBS)
	@echo "$@ linked."

$(DBG_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(DBG_OBJDIR)
	@$(CC) -c $(INCPATH) $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(DBG_OBJDIR)/%.o: $(APP_SRC_DIR)/%.c $(ALLHDRS) $(TMPLTS) | $(DBG_OBJDIR)
	@$(CC) -c $(INCPATH) $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(DBG_OBJDIR):
	@mkdir -p "$(DBG_OBJDIR)"

HUGETESTFILE = fixtures/huge/hugetestfile
HUGETESTDIR := $(dir $(HUGETESTFILE))

hugetestfile: $(HUGETESTFILE)

$(HUGETESTFILE):
	@echo Creating a huge file for testing
	@mkdir -p $(HUGETESTDIR)
	@dd if=/dev/urandom of=$(HUGETESTFILE) bs=1M count=10
	@echo The huge file for testing has been created

#
# Format rules
#
format:
	@echo "Formatting source files..."
	@for file in $(SRCS) $(HDRS) $(TMPLTS); do \
	    echo "Formatting $$file"; \
	    uncrustify -c ../Uncrustify.cfg --replace --no-backup $$file; \
	done
	@echo "All files formatted."

clean-hugetestfile:
	@rm -f $(HUGETESTFILE)

clean-compile-commands:
	@rm -f $(COMPILE_COMMANDS)

clean: clean-hugetestfile clean-compile-commands
	@rm -f $(SNTZ_EXE) $(DBG_EXE) $(COV_EXE)
	@rm -f $(DBG_OBJS) $(DBG_APP_OBJS)
	@rm -f $(SNTZ_OBJS) $(SNTZ_APP_OBJS)
	@rm -f $(COV_OBJS) $(COV_APP_OBJS)

	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true
	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true

	@rm -f $(COV_OBJDIR)/*.gcno $(COV_OBJDIR)/*.gcda $(COV_OBJDIR)/*.gcov $(COV_OBJDIR)/coverage.info
	@rm -rf $(COV_REPORTDIR)
	@test -d $(COV_OBJDIR) && rm -d $(COV_OBJDIR) 2>/dev/null || true
	@test -d $(COV_DIR) && rm -d $(COV_DIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true
	@test -d $(ROOT_BUILDDIR) && rm -d $(ROOT_BUILDDIR) 2>/dev/null || true
	@echo $(EXE) cleared

purge: clean-hugetestfile clean-compile-commands
	@$(MAKE) -s -C ../ purge

# Optional preprocessor files
%.i:%.c
	@$(CC) -E -C -o $@ $(INCPATH) $(CFLAGS) $<
# C-C++ Beautifier
#	@bcpp -na $@ > $@.h
	@bcpp -na -s -i 4 $@ > $@.h
	@sed -i 's/[ \t]*\# [[:digit:]]\+ \".*//g' $@.h
#	@sed -i '/^ *$//d' $@.h

#
# Print of variables
#
# If you want to find out the value of a makefile variable, just:
# make print-VARIABLE
# and it will return:
# VARIABLE = the_value_of_the_variable
#
print-%:
	@echo '$* = $($*)'

#
# GCC Static Analysis
#
static-analyzers: gcc-analyzer cppcheck clang-analyzer
static-analyzers-cli: gcc-analyzer cppcheck clang-analyzer-cli

gcc-analyzer: WFLAGS += -fanalyzer -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3
gcc-analyzer: CC = gcc
gcc-analyzer: debug-build

cppcheck:
	bear --output $(COMPILE_COMMANDS) -- make -B -s debug-build
	cppcheck --project=$(COMPILE_COMMANDS) --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force -i ../libs/sqlite3/src --inconclusive

# Find the highest-versioned clang binary available (clang-21, clang-20, …),
# fall back to plain "clang" when no versioned binary exists
CLANG := $(notdir $(or $(lastword $(sort $(wildcard $(addsuffix /clang-[0-9]*,$(subst :, ,$(PATH)))))),clang))
CLANG_SUFFIX := $(patsubst clang%,%,$(CLANG))
SCAN_BUILD := scan-build$(CLANG_SUFFIX)

clang-analyzer: CC = $(CLANG)
clang-analyzer:
	@echo "Using compiler: $(CLANG), analyzer: $(SCAN_BUILD)"
	# Run clang static analyzer and view analysis results in a web browser when the build command completes
	$(SCAN_BUILD) --exclude ../libs/sqlite3 -V $(MAKE) -s debug-build

clang-analyzer-cli: CC = $(CLANG)
clang-analyzer-cli:
	@echo "Using compiler: $(CLANG), analyzer: $(SCAN_BUILD)"
	$(SCAN_BUILD) --exclude ../libs/sqlite3 $(MAKE) -s debug-build
