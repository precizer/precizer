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
STATIC = -static -static-libgcc -Wl,--gc-sections

PRZRSRC = ../src
TESTED_APP_NAME := $(shell awk '$$2 == "APP_NAME" && $$3 ~ /^"/ { gsub(/"/,"",$$3); print $$3; exit }' $(PRZRSRC)/version.h)
BUILDS = ../.builds
BUILDEXE = $(BUILDS)/$(EXE)
BUILDEXEDIR = $(dir $(BUILDEXE))
COMPILE_COMMANDS = compile_commands.json
LIBS = testitall mem sha512 rational sqlite3 xdiff

ifeq ($(TESTED_APP_NAME),)
$(error Failed to extract APP_NAME from $(PRZRSRC)/version.h)
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
INCPATH += -I$(PRZRSRC)

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
PRZRHDRS = $(wildcard $(PRZRSRC)/*.h)
ALLHDRS = $(HDRS) $(PRZRHDRS)
OBJS = $(SRCS:.c=.o)
PRZRSRCS = $(wildcard $(PRZRSRC)/*.c)
PRZROBJS = $(PRZRSRCS:.c=.o)

#
# Coverage build settings
#
COV = coverage
COV_DIR = $(BUILDEXE)/$(COV)
COV_EXE = $(COV_DIR)/$(EXE)
COV_OBJDIR = $(COV_DIR)/obj
COV_REPORTDIR = $(COV_OBJDIR)/../report/
COV_OBJS = $(addprefix $(COV_OBJDIR)/, $(notdir $(OBJS)))
COV_LIBDIR = $(BUILDEXEDIR)$(COV)/libs
COV_LIBSOBJ = $(BUILDEXEDIR)$(COV)/obj
COV_LIBS = -L$(COV_LIBDIR) $(LDPATH)
COV_DYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(COV_LIBDIR),-rpath,\$$ORIGIN/../../$(COV_LIBDIR)
ifeq ($(UNAME_S),Darwin)
COV_DYNLIB = -Wl,-rpath,@executable_path/$(COV_LIBDIR),-rpath,@executable_path/../../$(COV_LIBDIR)
endif

COV_PRZROBJS = $(addprefix $(COV_OBJDIR)/, $(notdir $(PRZROBJS)))
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
SNTZ_DIR = $(BUILDEXE)/$(SNTZ)
SNTZ_EXE = $(SNTZ_DIR)/$(EXE)
SNTZ_OBJDIR = $(SNTZ_DIR)/obj
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(OBJS)))
SNTZ_PRZROBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(PRZROBJS)))
SNTZ_LIBDIR = $(BUILDEXEDIR)$(SNTZ)/libs
SNTZ_LIBS = -L$(SNTZ_LIBDIR) $(LDPATH)
SNTZ_DYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(SNTZ_LIBDIR),-rpath,\$$ORIGIN/../../$(SNTZ_LIBDIR)
ifeq ($(UNAME_S),Darwin)
SNTZ_DYNLIB = -Wl,-rpath,@executable_path/$(SNTZ_LIBDIR),-rpath,@executable_path/../../$(SNTZ_LIBDIR)
endif
SNTZ_OPTIONS = -fsanitize=address,undefined -fno-omit-frame-pointer
SNTZ_CFLAGS = $(DBG_CFLAGS) $(SNTZ_OPTIONS)
SNTZ_LDFLAGS = -Wl,-z,defs $(SNTZ_OPTIONS)
ifeq ($(UNAME_S),Darwin)
SNTZ_LDFLAGS = -Wl,-undefined,dynamic_lookup $(SNTZ_OPTIONS)
endif

#
# Debug build settings
#
DBG = debug
DBG_DIR = $(BUILDEXE)/$(DBG)
DBG_EXE = $(DBG_DIR)/$(EXE)
DBG_OBJDIR = $(DBG_DIR)/obj
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(OBJS)))
DBG_PRZROBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(PRZROBJS)))
DBG_LIBDIR = $(BUILDEXEDIR)$(DBG)/libs
DBG_LIBS = -L$(DBG_LIBDIR) $(LDPATH)
DBG_DYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBG_LIBDIR),-rpath,\$$ORIGIN/../../$(DBG_LIBDIR)
ifeq ($(UNAME_S),Darwin)
DBG_DYNLIB = -Wl,-rpath,@executable_path/$(DBG_LIBDIR),-rpath,@executable_path/../../$(DBG_LIBDIR)
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

$(COV_EXE): $(COV_OBJS) $(COV_PRZROBJS) | precizer-coverage testlibs-coverage
	@$(CC) $(COV_LIBS) $(COV_LDFLAGS) $(COV_DYNLIB) $(WRAPFLAGS) -o $(COV_EXE) $^ $(LDLIBS) $(CMOCKA_LDLIBS)
	@echo "$@ linked."

$(COV_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(COV_OBJDIR)/%.o: $(PRZRSRC)/%.c $(ALLHDRS) $(TMPLTS) | $(COV_OBJDIR)
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
	@lcov --capture --branch-coverage --rc lcov_excl_br_line='(^\s*RETURN_STATUS|SLOWTEST\s*;)|(\b(?:provide|ASSERT|TEST|SUTE)\s*\()' --directory $(COV_OBJDIR) --directory $(COV_LIBSOBJ) --output-file $(COV_OBJDIR)/coverage.info
	@genhtml --branch-coverage $(COV_OBJDIR)/coverage.info --output-directory $(COV_REPORTDIR)
	@echo "HTML coverage report generated in $(COV_REPORTDIR) directory"

#
# Sanitize rules
#
precizer-sanitize:
	@$(MAKE) -s -C ../ sanitize

sanitize: precizer-sanitize hugetestfile $(SNTZ_EXE)
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(SNTZ_EXE)

$(SNTZ_EXE): $(SNTZ_OBJS) $(SNTZ_PRZROBJS) | precizer-sanitize testlibs-sanitize
	@$(CC) $(SNTZ_LIBS) $(SNTZ_LDFLAGS) $(SNTZ_DYNLIB) $(WRAPFLAGS) -o $(SNTZ_EXE) $^ $(LDLIBS) $(CMOCKA_LDLIBS)
	@echo "$@ linked."

$(SNTZ_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(INCPATH) $(SNTZ_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(SNTZ_OBJDIR)/%.o: $(PRZRSRC)/%.c $(ALLHDRS) $(TMPLTS) | $(SNTZ_OBJDIR)
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

$(DBG_EXE): $(DBG_OBJS) $(DBG_PRZROBJS) | precizer-debug testlibs-debug
	@$(CC) $(DBG_LIBS) $(DBG_DYNLIB) $(STATIC) $(WRAPFLAGS) -o $(DBG_EXE) $^ $(LDLIBS)
	@echo "$@ linked."

$(DBG_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(DBG_OBJDIR)
	@$(CC) -c $(INCPATH) $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(DBG_OBJDIR)/%.o: $(PRZRSRC)/%.c $(ALLHDRS) $(TMPLTS) | $(DBG_OBJDIR)
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
	@rm -f $(DBG_OBJS) $(DBG_PRZROBJS)
	@rm -f $(SNTZ_OBJS) $(SNTZ_PRZROBJS)
	@rm -f $(COV_OBJS) $(COV_PRZROBJS)

	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true
	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true

	@rm -f $(COV_OBJDIR)/*.gcno $(COV_OBJDIR)/*.gcda $(COV_OBJDIR)/*.gcov $(COV_OBJDIR)/coverage.info
	@rm -rf $(COV_REPORTDIR)
	@test -d $(COV_OBJDIR) && rm -d $(COV_OBJDIR) 2>/dev/null || true
	@test -d $(COV_DIR) && rm -d $(COV_DIR) 2>/dev/null || true

	@test -d $(BUILDEXE) && rm -d $(BUILDEXE) 2>/dev/null || true
	@test -d $(BUILDS) && rm -d $(BUILDS) 2>/dev/null || true
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

clang-analyzer: CC = clang-20
clang-analyzer: SCAN-BUILD = scan-build-20
clang-analyzer:
	# Run clang static analyzer and view analysis results in a web browser when the build command completes
	$(SCAN-BUILD) --exclude ../libs/sqlite3 -V $(MAKE) -s debug-build

clang-analyzer-cli: CC = clang-20
clang-analyzer-cli: SCAN-BUILD = scan-build-20
clang-analyzer-cli:
	$(SCAN-BUILD) --exclude ../libs/sqlite3 $(MAKE) -s debug-build
