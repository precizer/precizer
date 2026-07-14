# Name of the application
EXE=testitall

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory

#
# Compiler flags
#

C_STANDARD := -std=c2x
CPPFLAGS ?=
CFLAGS ?= -finline-functions
LDFLAGS ?=

UNAME_S := $(shell uname -s)

# macOS-specific code
ifeq ($(UNAME_S),Darwin)
CFLAGS += -DEVIL_EMPIRE_OS
endif

SRC = src
STATIC = -static -static-libgcc -Wl,--gc-sections

APP_SRC_DIR = ../src
BUILDDIR = ../.builds
# Returns a concise path for build messages.
# Plain file names stay unchanged; nested paths keep only final-directory/file
short_path = $(if $(filter ./,$(dir $(1))),$(notdir $(1)),$(notdir $(patsubst %/,%,$(dir $(1))))/$(notdir $(1)))
TEST_BUILDDIR = $(BUILDDIR)/$(EXE)
COMPILE_COMMANDS = $(BUILDDIR)/compile_commands.json
LIBS = testitall testmocking mem sha512 rational sqlite3 xdiff monocypher

TESTED_APP_NAME := $(shell awk '$$2 == "APP_NAME" && $$3 ~ /^"/ { gsub(/"/,"",$$3); print $$3; exit }' $(APP_SRC_DIR)/version.h)
ifeq ($(TESTED_APP_NAME),)
$(error Failed to extract APP_NAME from $(APP_SRC_DIR)/version.h)
endif

# Extra libs for linking
LDLIBS += $(foreach d,$(LIBS),-l$d)
ifeq ($(UNAME_S),Darwin)
LDLIBS += $(shell pkg-config --libs libpcre2-8)
ARGP_PREFIX ?= $(shell brew --prefix argp-standalone 2>/dev/null)
LDPATH += -L$(ARGP_PREFIX)/lib
else
LDLIBS += -lpcre2-8
endif
EXT_LDLIBS = $(filter-out $(addprefix -l,$(LIBS)),$(LDLIBS))

SYS := $(shell $(CC) -dumpmachine 2>/dev/null)
ifneq (, $(findstring alpine, $(SYS)))
# Alpine Linux uses external libraries
LDLIBS += -largp -lfts
endif

# Additional include paths for external libraries
INCPATH += $(foreach d,$(LIBS),-I../libs/$d/src/)
## Source files of precizer project
INCPATH += -I$(APP_SRC_DIR)

ifeq ($(UNAME_S),Darwin)
INCPATH += $(shell pkg-config --cflags libpcre2-8)
INCPATH += -I$(ARGP_PREFIX)/include
LDLIBS += -largp
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
# argp is not part of Cygwin's C library
LDLIBS += -largp
endif

# Detect whether we're using GCC
GCC := $(findstring gcc,$(notdir $(firstword $(CC))))

# Warning flags for additional checks
WFLAGS += -Werror # Treat every warning as an error (-Werror stops the build)
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

# Link-time wrappers for mock-based tests (add more functions as needed).
WRAPFUNCS = fopen openat __openat_2 fdopen fread ferror remove access malloc realloc snprintf vsnprintf write gettimeofday localtime_r
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
TMPLTS = $(wildcard $(SRC)/*.tpl)
HDRS = $(wildcard $(SRC)/*.h) $(foreach d,$(LIBS),$(wildcard ../libs/$d/src/*.h))
APP_HDRS = $(wildcard $(APP_SRC_DIR)/*.h)
ALLHDRS = $(HDRS) $(APP_HDRS) $(LIB_TESTS_HDRS)
OBJS = $(SRCS:.c=.o)
APP_SRCS = $(wildcard $(APP_SRC_DIR)/*.c)
APP_OBJS = $(APP_SRCS:.c=.o)

# Source files contributed by bundled library test suites
LIB_TESTS_SRC_DIRS = $(sort $(wildcard ../libs/*/tests/src))
LIB_TESTS_SRCS = $(wildcard ../libs/*/tests/src/test_*.c) \
                 $(wildcard ../libs/*/tests/src/mocks_*.c)
LIB_TESTS_TMPLTS = $(wildcard ../libs/*/tests/src/*.tpl)
LIB_TESTS_HDRS = $(wildcard ../libs/*/tests/src/*.h)
INCPATH += $(addprefix -I,$(LIB_TESTS_SRC_DIRS))

#
# Coverage build settings
#
COV = coverage
COV_DIR = $(TEST_BUILDDIR)/$(COV)
COV_EXE = $(COV_DIR)/$(EXE)
COV_OBJDIR = $(COV_DIR)/obj
COV_REPORTDIR = $(COV_OBJDIR)/../report/
COV_OBJS = $(addprefix $(COV_OBJDIR)/, $(notdir $(OBJS)))
COV_LIBDIR = $(BUILDDIR)/$(COV)/libs
COV_APP_OBJDIR = $(BUILDDIR)/$(COV)/obj
COV_LDPATH = -L$(COV_LIBDIR) $(LDPATH)
ifeq ($(UNAME_S),Darwin)
COV_RPATH = -Wl,-rpath,@executable_path/$(COV_LIBDIR),-rpath,@executable_path/../../$(COV_LIBDIR)
else
COV_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(COV_LIBDIR),-rpath,\$$ORIGIN/../../$(COV_LIBDIR)
endif

COV_APP_OBJS = $(addprefix $(COV_OBJDIR)/, $(notdir $(APP_OBJS)))
COV_LIB_OBJDIR = $(BUILDDIR)/$(COV)/libs/obj
COV_LIB_OBJS = $(foreach lib,$(LIBS),$(COV_LIB_OBJDIR)/$(lib)/*.o)
LIB_TESTS_COV_OBJS = $(patsubst ../%.c,$(COV_OBJDIR)/%.o,$(LIB_TESTS_SRCS))

# Keep TESTITALL in the test runner build flags only.
# Adding it to the global CFLAGS leaks it into recursive builds such as
# `make -C ../ sanitize`, which disables main() in the standalone precizer
# binary and enables test-only hooks there
# Callers may replace these defaults through COV_OPT_CFLAGS and
# COV_OPT_LDFLAGS. Coverage optimization flags follow CFLAGS and LDFLAGS and
# win conflicts
COV_OPT_CFLAGS ?= -pipe -fbuiltin -O0 -fno-omit-frame-pointer
COV_CFLAGS ?= -fprofile-arcs -ftest-coverage $(COV_OPT_CFLAGS) -g -ggdb3 -DTESTITALL -DTESTITALL_TEST_HOOKS
COV_OPT_LDFLAGS ?=
COV_LDFLAGS ?= $(COV_OPT_LDFLAGS) -lgcov --coverage

#
# Sanitize build settings
#
SNTZ = sanitize
SNTZ_DIR = $(TEST_BUILDDIR)/$(SNTZ)
SNTZ_EXE = $(SNTZ_DIR)/$(EXE)
SNTZ_OBJDIR = $(SNTZ_DIR)/obj
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(OBJS)))
SNTZ_APP_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(APP_OBJS)))
LIB_TESTS_SNTZ_OBJS = $(patsubst ../%.c,$(SNTZ_OBJDIR)/%.o,$(LIB_TESTS_SRCS))
SNTZ_LIB_OBJDIR = $(BUILDDIR)/$(SNTZ)/libs/obj
SNTZ_LIB_OBJS = $(foreach lib,$(LIBS),$(SNTZ_LIB_OBJDIR)/$(lib)/*.o)
SNTZ_LIBDIR = $(BUILDDIR)/$(SNTZ)/libs
SNTZ_LDPATH = -L$(SNTZ_LIBDIR) $(LDPATH)
ifeq ($(UNAME_S),Darwin)
SNTZ_RPATH = -Wl,-rpath,@executable_path/$(SNTZ_LIBDIR),-rpath,@executable_path/../../$(SNTZ_LIBDIR)
else
SNTZ_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(SNTZ_LIBDIR),-rpath,\$$ORIGIN/../../$(SNTZ_LIBDIR)
endif
SNTZ_OPTIONS = -fsanitize=address,undefined
# Callers may replace these defaults through SNTZ_OPT_CFLAGS and
# SNTZ_OPT_LDFLAGS. Sanitizer optimization flags follow CFLAGS and LDFLAGS,
# while later required sanitizer flags take final priority
SNTZ_OPT_CFLAGS ?= $(DBG_OPT_CFLAGS)
SNTZ_CFLAGS ?= $(SNTZ_OPT_CFLAGS) $(SNTZ_OPTIONS) -g -ggdb3 -DDEBUG -DTESTITALL -DTESTITALL_TEST_HOOKS
SNTZ_OPT_LDFLAGS ?=
ifeq ($(UNAME_S),Darwin)
SNTZ_LDFLAGS ?= $(SNTZ_OPT_LDFLAGS) -Wl,-undefined,dynamic_lookup $(SNTZ_OPTIONS)
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
SNTZ_LDFLAGS ?= $(SNTZ_OPT_LDFLAGS) $(SNTZ_OPTIONS)
else
SNTZ_LDFLAGS ?= $(SNTZ_OPT_LDFLAGS) -Wl,-z,defs $(SNTZ_OPTIONS)
endif

#
# Debug build settings
#
DBG = debug
DBG_DIR = $(TEST_BUILDDIR)/$(DBG)
DBG_EXE = $(DBG_DIR)/$(EXE)
DBG_OBJDIR = $(DBG_DIR)/obj
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(OBJS)))
DBG_APP_OBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(APP_OBJS)))
LIB_TESTS_DBG_OBJS = $(patsubst ../%.c,$(DBG_OBJDIR)/%.o,$(LIB_TESTS_SRCS))
DBG_LIB_OBJDIR = $(BUILDDIR)/$(DBG)/libs/obj
DBG_LIB_OBJS = $(foreach lib,$(LIBS),$(DBG_LIB_OBJDIR)/$(lib)/*.o)
DBG_LIBDIR = $(BUILDDIR)/$(DBG)/libs
DBG_LDPATH = -L$(DBG_LIBDIR) $(LDPATH)
DBG_INCPATH = $(INCPATH)
ifeq ($(UNAME_S),Darwin)
DBG_RPATH = -Wl,-rpath,@executable_path/$(DBG_LIBDIR),-rpath,@executable_path/../../$(DBG_LIBDIR)
else
DBG_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBG_LIBDIR),-rpath,\$$ORIGIN/../../$(DBG_LIBDIR)
endif
# Callers may replace these defaults through DBG_OPT_CFLAGS and
# DBG_OPT_LDFLAGS. Debug optimization flags follow CFLAGS and LDFLAGS and
# win conflicts
DBG_OPT_CFLAGS ?= -pipe -fbuiltin -Og -fno-omit-frame-pointer
DBG_CFLAGS ?= $(DBG_OPT_CFLAGS) -g -ggdb3 -DDEBUG -DTESTITALL -DTESTITALL_TEST_HOOKS
DBG_OPT_LDFLAGS ?=
DBG_LDFLAGS ?= $(DBG_OPT_LDFLAGS)

#
# Dynamic debug build settings
#
DBG_DYN = debug-dynamic
DBG_DYN_DIR = $(TEST_BUILDDIR)/$(DBG_DYN)
DBG_DYN_EXE = $(DBG_DYN_DIR)/$(EXE)
DBG_DYN_OBJDIR = $(DBG_DYN_DIR)/obj
DBG_DYN_OBJS = $(addprefix $(DBG_DYN_OBJDIR)/, $(notdir $(OBJS)))
DBG_DYN_APP_OBJS = $(addprefix $(DBG_DYN_OBJDIR)/, $(notdir $(APP_OBJS)))
LIB_TESTS_DBG_DYN_OBJS = $(patsubst ../%.c,$(DBG_DYN_OBJDIR)/%.o,$(LIB_TESTS_SRCS))
DBG_DYN_LIBS = $(filter-out sqlite3,$(LIBS))
DBG_DYN_LIB_OBJDIR = $(DBG_LIB_OBJDIR)
DBG_DYN_LIBDIR = $(DBG_LIBDIR)
DBG_DYN_LIB_OBJS = $(foreach lib,$(DBG_DYN_LIBS),$(DBG_DYN_LIB_OBJDIR)/$(lib)/*.o)
DBG_DYN_LDPATH = $(LDPATH)
DBG_DYN_EXT_LDLIBS = $(filter-out $(addprefix -l,$(DBG_DYN_LIBS)),$(LDLIBS))
DBG_DYN_INCPATH = $(filter-out -I../libs/sqlite3/src/,$(INCPATH))
DBG_DYN_HDRS = $(wildcard $(SRC)/*.h) $(foreach d,$(DBG_DYN_LIBS),$(wildcard ../libs/$d/src/*.h))
DBG_DYN_ALLHDRS = $(DBG_DYN_HDRS) $(APP_HDRS) $(LIB_TESTS_HDRS)
# Callers may replace these defaults through DBG_DYN_OPT_CFLAGS and
# DBG_DYN_OPT_LDFLAGS. Dynamic debug optimization flags follow CFLAGS and
# LDFLAGS and win conflicts
DBG_DYN_OPT_CFLAGS ?= $(DBG_OPT_CFLAGS)
DBG_DYN_CFLAGS ?= $(DBG_DYN_OPT_CFLAGS) -g -ggdb3 -DDEBUG -DTESTITALL -DTESTITALL_TEST_HOOKS
DBG_DYN_OPT_LDFLAGS ?= $(DBG_OPT_LDFLAGS)
DBG_DYN_LDFLAGS ?= $(DBG_DYN_OPT_LDFLAGS)

.PHONY: all clean debug debug-dynamic sanitize coverage run test tests tests-sanitize tests-debug tests-dynamic coverage-report coverage-html format
.PHONY: precizer-coverage precizer-sanitize precizer-debug precizer-debug-dynamic hugetestfile purge static-analyzers static-analyzers-cli gcc-analyzer cppcheck clang-analyzer clang-analyzer-cli print-%
.PHONY: testlibs-coverage testlibs-sanitize testlibs-debug testlibs-debug-dynamic debug-build debug-dynamic-build compile-commands clean-compile-commands

# Default build
all: sanitize

test: sanitize

tests: test

tests-sanitize: sanitize

tests-debug: debug

tests-dynamic: debug-dynamic

run: sanitize

testlibs-coverage: precizer-coverage
	@$(MAKE) -s -C ../libs coverage SUBDIRS="xdiff testitall testmocking monocypher" TESTITALL_APP_NAME=$(TESTED_APP_NAME) BUILDDIR=../$(BUILDDIR)

testlibs-sanitize: precizer-sanitize
	@$(MAKE) -s -C ../libs sanitize SUBDIRS="xdiff testitall testmocking monocypher" TESTITALL_APP_NAME=$(TESTED_APP_NAME) BUILDDIR=../$(BUILDDIR)

testlibs-debug: precizer-debug
	@$(MAKE) -s -C ../libs debug SUBDIRS="xdiff testitall testmocking monocypher" TESTITALL_APP_NAME=$(TESTED_APP_NAME) BUILDDIR=../$(BUILDDIR)

testlibs-debug-dynamic: precizer-debug-dynamic
	@$(MAKE) -s -C ../libs debug SUBDIRS="xdiff testitall testmocking monocypher" TESTITALL_APP_NAME=$(TESTED_APP_NAME) BUILDDIR=../$(BUILDDIR)

#
# Coverage rules
#
precizer-coverage:
	@$(MAKE) -s -C ../ precizer-coverage

coverage: hugetestfile coverage-html

$(COV_EXE): $(COV_OBJS) $(COV_APP_OBJS) $(LIB_TESTS_COV_OBJS) precizer-coverage testlibs-coverage
	@$(CC) $(LDFLAGS) $(COV_LDPATH) $(COV_LDFLAGS) $(COV_RPATH) $(WRAPFLAGS) $(COV_OBJS) $(COV_APP_OBJS) $(LIB_TESTS_COV_OBJS) $(COV_LIB_OBJS) $(EXT_LDLIBS) -o $(COV_EXE)
	@echo "$(call short_path,$@) linked dynamically with coverage flags, not stripped"

$(COV_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(COV_OBJDIR)
	@$(CC) -c $(CPPFLAGS) $(INCPATH) $(CFLAGS) $(COV_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with coverage flags"

$(COV_OBJDIR)/%.o: $(APP_SRC_DIR)/%.c $(ALLHDRS) $(TMPLTS) | $(COV_OBJDIR)
	@$(CC) -c $(CPPFLAGS) $(INCPATH) $(CFLAGS) $(COV_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with coverage flags"

$(COV_OBJDIR)/%.o: ../%.c $(ALLHDRS) $(LIB_TESTS_HDRS) $(LIB_TESTS_TMPLTS) | $(COV_OBJDIR)
	@mkdir -p "$(dir $@)"
	@$(CC) -c $(CPPFLAGS) $(INCPATH) $(CFLAGS) $(COV_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with coverage flags"

$(COV_OBJDIR):
	@mkdir -p "$(COV_OBJDIR)"

coverage-report: hugetestfile precizer-coverage $(COV_EXE)
	@echo "Generating coverage report..."
	./$(COV_EXE)
	@echo "Coverage run finished. Inspect .gcda/.gcno outputs for details"

coverage-html: coverage-report
	@echo "Generating HTML coverage report..."
	@lcov --capture --branch-coverage --exclude '*/tests/src/mocks.c' --rc lcov_excl_br_line='(^\s*RETURN_STATUS|SLOWTEST\s*;)|(\b(?:provide|deliver|ask|run|call|ASSERT|IF|HEADER|TEST|SUTE)\s*\()' --directory $(COV_OBJDIR) --directory $(COV_APP_OBJDIR) --directory $(COV_LIB_OBJDIR) --output-file $(COV_OBJDIR)/coverage.info
	@genhtml --branch-coverage $(COV_OBJDIR)/coverage.info --output-directory $(COV_REPORTDIR)
	@echo "HTML coverage report generated in $(COV_REPORTDIR) directory"

#
# Sanitize rules
#
precizer-sanitize:
	@$(MAKE) -s -C ../ sanitize

sanitize: precizer-sanitize hugetestfile $(SNTZ_EXE)
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(SNTZ_EXE)

$(SNTZ_EXE): $(SNTZ_OBJS) $(SNTZ_APP_OBJS) $(LIB_TESTS_SNTZ_OBJS) precizer-sanitize testlibs-sanitize
	@$(CC) $(LDFLAGS) $(SNTZ_LDPATH) $(SNTZ_LDFLAGS) $(SNTZ_RPATH) $(WRAPFLAGS) $(SNTZ_OBJS) $(SNTZ_APP_OBJS) $(LIB_TESTS_SNTZ_OBJS) $(SNTZ_LIB_OBJS) $(EXT_LDLIBS) -o $(SNTZ_EXE)
	@echo "$(call short_path,$@) linked dynamically with sanitizer flags, not stripped"

$(SNTZ_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(CPPFLAGS) $(INCPATH) $(CFLAGS) $(SNTZ_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with sanitizer flags"

$(SNTZ_OBJDIR)/%.o: $(APP_SRC_DIR)/%.c $(ALLHDRS) $(TMPLTS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(CPPFLAGS) $(INCPATH) $(CFLAGS) $(SNTZ_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with sanitizer flags"

$(SNTZ_OBJDIR)/%.o: ../%.c $(ALLHDRS) $(LIB_TESTS_HDRS) $(LIB_TESTS_TMPLTS) | $(SNTZ_OBJDIR)
	@mkdir -p "$(dir $@)"
	@$(CC) -c $(CPPFLAGS) $(INCPATH) $(CFLAGS) $(SNTZ_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with sanitizer flags"

$(SNTZ_OBJDIR):
	@mkdir -p "$(SNTZ_OBJDIR)"

#
# Debug rules
#
precizer-debug:
	@$(MAKE) -s -C ../ debug

precizer-debug-dynamic:
	@$(MAKE) -s -C ../ debug-dynamic

debug-build: $(DBG_EXE)

debug: hugetestfile debug-build
	@./$(DBG_EXE)

$(DBG_EXE): $(DBG_OBJS) $(DBG_APP_OBJS) $(LIB_TESTS_DBG_OBJS) precizer-debug testlibs-debug
	@$(CC) $(LDFLAGS) $(DBG_LDPATH) $(DBG_LDFLAGS) $(DBG_RPATH) $(STATIC) $(WRAPFLAGS) $(DBG_OBJS) $(DBG_APP_OBJS) $(LIB_TESTS_DBG_OBJS) $(DBG_LIB_OBJS) $(EXT_LDLIBS) -o $(DBG_EXE)
	@echo "$(call short_path,$@) linked statically with debug flags, not stripped"

$(DBG_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(DBG_OBJDIR)
	@$(CC) -c $(CPPFLAGS) $(DBG_INCPATH) $(CFLAGS) $(DBG_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with debug flags"

$(DBG_OBJDIR)/%.o: $(APP_SRC_DIR)/%.c $(ALLHDRS) $(TMPLTS) | $(DBG_OBJDIR)
	@$(CC) -c $(CPPFLAGS) $(DBG_INCPATH) $(CFLAGS) $(DBG_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with debug flags"

$(DBG_OBJDIR)/%.o: ../%.c $(ALLHDRS) $(LIB_TESTS_HDRS) $(LIB_TESTS_TMPLTS) | $(DBG_OBJDIR)
	@mkdir -p "$(dir $@)"
	@$(CC) -c $(CPPFLAGS) $(DBG_INCPATH) $(CFLAGS) $(DBG_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with debug flags"

$(DBG_OBJDIR):
	@mkdir -p "$(DBG_OBJDIR)"

debug-dynamic-build: $(DBG_DYN_EXE)

debug-dynamic: hugetestfile debug-dynamic-build
	@./$(DBG_DYN_EXE)

$(DBG_DYN_EXE): $(DBG_DYN_OBJS) $(DBG_DYN_APP_OBJS) $(LIB_TESTS_DBG_DYN_OBJS) precizer-debug-dynamic testlibs-debug-dynamic
	@$(CC) $(LDFLAGS) $(DBG_DYN_LDPATH) $(DBG_DYN_LDFLAGS) $(WRAPFLAGS) $(DBG_DYN_OBJS) $(DBG_DYN_APP_OBJS) $(LIB_TESTS_DBG_DYN_OBJS) $(DBG_DYN_LIB_OBJS) $(DBG_DYN_EXT_LDLIBS) -o $(DBG_DYN_EXE)
	@echo "$(call short_path,$@) linked dynamically with debug flags, not stripped"

$(DBG_DYN_OBJDIR)/%.o: $(SRC)/%.c $(DBG_DYN_ALLHDRS) $(TMPLTS) | $(DBG_DYN_OBJDIR)
	@$(CC) -c $(CPPFLAGS) $(DBG_DYN_INCPATH) $(CFLAGS) $(DBG_DYN_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with dynamic debug flags"

$(DBG_DYN_OBJDIR)/%.o: $(APP_SRC_DIR)/%.c $(DBG_DYN_ALLHDRS) $(TMPLTS) | $(DBG_DYN_OBJDIR)
	@$(CC) -c $(CPPFLAGS) $(DBG_DYN_INCPATH) $(CFLAGS) $(DBG_DYN_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with dynamic debug flags"

$(DBG_DYN_OBJDIR)/%.o: ../%.c $(DBG_DYN_ALLHDRS) $(LIB_TESTS_HDRS) $(LIB_TESTS_TMPLTS) | $(DBG_DYN_OBJDIR)
	@mkdir -p "$(dir $@)"
	@$(CC) -c $(CPPFLAGS) $(DBG_DYN_INCPATH) $(CFLAGS) $(DBG_DYN_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with dynamic debug flags"

$(DBG_DYN_OBJDIR):
	@mkdir -p "$(DBG_DYN_OBJDIR)"

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
	@rm -f $(SNTZ_EXE) $(DBG_EXE) $(DBG_DYN_EXE) $(COV_EXE)
	@rm -f $(DBG_OBJS) $(DBG_APP_OBJS) $(LIB_TESTS_DBG_OBJS)
	@rm -f $(DBG_DYN_OBJS) $(DBG_DYN_APP_OBJS) $(LIB_TESTS_DBG_DYN_OBJS)
	@rm -f $(SNTZ_OBJS) $(SNTZ_APP_OBJS) $(LIB_TESTS_SNTZ_OBJS)
	@rm -f $(COV_OBJS) $(COV_APP_OBJS) $(LIB_TESTS_COV_OBJS)
	@rm -rf $(DBG_OBJDIR)/libs
	@rm -rf $(DBG_DYN_OBJDIR)/libs
	@rm -rf $(SNTZ_OBJDIR)/libs
	@rm -rf $(COV_OBJDIR)/libs

	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true
	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

	@test -d $(DBG_DYN_OBJDIR) && rm -d $(DBG_DYN_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DYN_DIR) && rm -d $(DBG_DYN_DIR) 2>/dev/null || true

	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true

	@rm -f $(COV_OBJDIR)/*.gcno $(COV_OBJDIR)/*.gcda $(COV_OBJDIR)/*.gcov $(COV_OBJDIR)/coverage.info
	@rm -rf $(COV_REPORTDIR)
	@test -d $(COV_OBJDIR) && rm -d $(COV_OBJDIR) 2>/dev/null || true
	@test -d $(COV_DIR) && rm -d $(COV_DIR) 2>/dev/null || true

	@test -d $(TEST_BUILDDIR) && rm -d $(TEST_BUILDDIR) 2>/dev/null || true
	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true
	@echo $(EXE) cleared

compile-commands: $(COMPILE_COMMANDS)

$(COMPILE_COMMANDS):
	@mkdir -p "$(BUILDDIR)"
	bear --output $@ -- $(MAKE) -B -s debug-build

cppcheck: compile-commands
	cppcheck --project=$(COMPILE_COMMANDS) --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force -i ../libs/sqlite3/src --inconclusive

#
# Generating a huge test file consisting of random bytes
#
HUGETESTFILE = fixtures/huge/hugetestfile
HUGETESTDIR := $(dir $(HUGETESTFILE))

hugetestfile: $(HUGETESTFILE)

$(HUGETESTFILE):
	@echo Creating a huge file for testing
	@mkdir -p $(HUGETESTDIR)
	@dd if=/dev/urandom of=$(HUGETESTFILE) bs=1M count=10
	@echo The huge file for testing has been created

#
# GCC Static Analysis
#
static-analyzers: gcc-analyzer cppcheck clang-analyzer
static-analyzers-cli: gcc-analyzer cppcheck clang-analyzer-cli

gcc-analyzer: WFLAGS += -fanalyzer -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3
gcc-analyzer: CC = gcc
gcc-analyzer: debug-build

# Find the highest-versioned clang binary available (clang-21, clang-20, …),
# fall back to plain "clang" when no versioned binary exists
CLANG := $(notdir $(or $(lastword $(sort $(wildcard $(addsuffix /clang-[0-9]*,$(subst :, ,$(PATH)))))),clang))
CLANG_SUFFIX := $(patsubst clang%,%,$(CLANG))
SCAN_BUILD := scan-build$(CLANG_SUFFIX)

# Run clang static analyzer and view analysis results in a web browser when the build command completes
clang-analyzer: CC = $(CLANG)
clang-analyzer:
	@echo "Using compiler: $(CLANG), analyzer: $(SCAN_BUILD)"
	$(SCAN_BUILD) --exclude ../libs/sqlite3 -V $(MAKE) -s debug-build

clang-analyzer-cli: CC = $(CLANG)
clang-analyzer-cli:
	@echo "Using compiler: $(CLANG), analyzer: $(SCAN_BUILD)"
	$(SCAN_BUILD) --exclude ../libs/sqlite3 $(MAKE) -s debug-build
purge: clean-hugetestfile clean-compile-commands
	@$(MAKE) -s -C ../ purge

# Optional preprocessor files
%.i:%.c
	@$(CC) -E -C $(CPPFLAGS) $(INCPATH) $(CFLAGS) $(C_STANDARD) -o $@ $<
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
