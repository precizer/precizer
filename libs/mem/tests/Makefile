# Name of the application
EXE=testitall

# Name of the library under test
LIBNAME=libmem

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory
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

# Defaults to the parent library .builds when running `make` from this
# directory standalone. The parent Makefile of this library overrides
# BUILDDIR with the same path, so both entrypoints write to one artifact tree
BUILDDIR ?= ../.builds
LIBDIR = $(patsubst lib%,%,$(LIBNAME))
TEST_BUILDDIR = $(BUILDDIR)/$(LIBDIR)/$(EXE)
COMPILE_COMMANDS = $(BUILDDIR)/compile_commands.json
LIBS = testitall xdiff rational testmocking sha512
LINK_LIBS = $(LIBDIR) $(LIBS)
TESTEDLIB_BUILDDIR = tests/$(BUILDDIR)
DEPLIB_BUILDDIR = ../$(LIBDIR)/tests/$(BUILDDIR)

# Extra libs for linking
LDLIBS += $(foreach d,$(LINK_LIBS),-l$d)
LDLIBS += -lpcre2-8
# Keep external libraries in this list. Internal libraries are linked through
# object files so --wrap can intercept calls made inside those libraries
EXT_LDLIBS = $(filter-out $(foreach d,$(LINK_LIBS),-l$d),$(LDLIBS))

# Link-time wrappers let mock-based tests force allocation, formatting,
# output, and time failures. Add newly wrapped functions here as needed
WRAPFUNCS = malloc realloc snprintf vsnprintf write gettimeofday localtime_r
ifeq ($(UNAME_S),Darwin)
# macOS lld does not honor -Wl,--wrap, so wrapped substitutions are disabled
WRAPFLAGS =
else
WRAPFLAGS = $(foreach f,$(WRAPFUNCS),-Wl,--wrap=$(f))
endif

# Additional include paths for external libraries
INCPATH += -I../src/
INCPATH += $(foreach d,$(LINK_LIBS),-I../../$(d)/src/)

# Detect whether GCC is in use
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

#
# Project files
#
SRCS = $(wildcard $(SRC)/*.c)
TMPLTS = $(wildcard $(SRC)/*.cc)
# Keep the suite's own headers separate so the format target changes only
# test files and does not rewrite headers owned by dependency libraries
LOCAL_HDRS = $(wildcard $(SRC)/*.h)
HDRS = $(LOCAL_HDRS) $(foreach d,$(LINK_LIBS),$(wildcard ../../$(d)/src/*.h))
OBJS = $(SRCS:.c=.o)

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
COV_LIB_OBJDIR = $(COV_LIBDIR)/obj
COV_LIB_OBJS = $(foreach lib,$(LINK_LIBS),$(COV_LIB_OBJDIR)/$(lib)/*.o)
COV_LDPATH = -L$(COV_LIBDIR) $(LDPATH)
ifeq ($(UNAME_S),Darwin)
COV_RPATH = -Wl,-rpath,@executable_path/../../../$(COV)/libs,-rpath,@executable_path
else
COV_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/../../../$(COV)/libs
endif
COV_CFLAGS = $(CFLAGS) -g -O0 -fprofile-arcs -ftest-coverage -DTESTITALL
COV_LDFLAGS = -lgcov --coverage

#
# Sanitize build settings
#
SNTZ = sanitize
SNTZ_DIR = $(TEST_BUILDDIR)/$(SNTZ)
SNTZ_EXE = $(SNTZ_DIR)/$(EXE)
SNTZ_OBJDIR = $(SNTZ_DIR)/obj
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(notdir $(OBJS)))
SNTZ_LIBDIR = $(BUILDDIR)/$(SNTZ)/libs
SNTZ_LIB_OBJDIR = $(SNTZ_LIBDIR)/obj
SNTZ_LIB_OBJS = $(foreach lib,$(LINK_LIBS),$(SNTZ_LIB_OBJDIR)/$(lib)/*.o)
SNTZ_LDPATH = -L$(SNTZ_LIBDIR) $(LDPATH)
ifeq ($(UNAME_S),Darwin)
SNTZ_RPATH = -Wl,-rpath,@executable_path/../../../$(SNTZ)/libs,-rpath,@executable_path
else
SNTZ_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/../../../$(SNTZ)/libs
endif
SNTZ_OPTIONS = -fsanitize=address,undefined -fno-omit-frame-pointer
SNTZ_CFLAGS = $(DBG_CFLAGS) $(SNTZ_OPTIONS)
ifeq ($(UNAME_S),Darwin)
SNTZ_LDFLAGS = -Wl,-undefined,dynamic_lookup $(SNTZ_OPTIONS)
else
SNTZ_LDFLAGS = -Wl,-z,defs $(SNTZ_OPTIONS)
endif

#
# Debug build settings
#
DBG = debug
DBG_DIR = $(TEST_BUILDDIR)/$(DBG)
DBG_EXE = $(DBG_DIR)/$(EXE)
DBG_OBJDIR = $(DBG_DIR)/obj
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(notdir $(OBJS)))
DBG_LIBDIR = $(BUILDDIR)/$(DBG)/libs
DBG_LIB_OBJDIR = $(DBG_LIBDIR)/obj
DBG_LIB_OBJS = $(foreach lib,$(LINK_LIBS),$(DBG_LIB_OBJDIR)/$(lib)/*.o)
DBG_LDPATH = -L$(DBG_LIBDIR) $(LDPATH)
ifeq ($(UNAME_S),Darwin)
DBG_RPATH = -Wl,-rpath,@executable_path/../../../$(DBG)/libs,-rpath,@executable_path
else
DBG_RPATH = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/../../../$(DBG)/libs
endif
DBG_CFLAGS = $(CFLAGS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG -DTESTITALL

.PHONY: all clean purge debug sanitize coverage run test tests tests-sanitize tests-debug tests-coverage coverage-report coverage-html format
.PHONY: testedlib-coverage testedlib-sanitize testedlib-debug
.PHONY: testlibs-coverage testlibs-sanitize testlibs-debug
.PHONY: debug-build compile-commands clean-compile-commands cppcheck

# Default build
all: sanitize

test: sanitize

tests: test

tests-sanitize: sanitize

tests-debug: debug

tests-coverage: coverage

run: sanitize

testlibs-coverage: testedlib-coverage
	@$(foreach lib,$(LIBS),$(MAKE) -s -C ../../$(lib) coverage BUILDDIR="$(DEPLIB_BUILDDIR)" || exit $$?;)

testlibs-sanitize: testedlib-sanitize
	@$(foreach lib,$(LIBS),$(MAKE) -s -C ../../$(lib) sanitize BUILDDIR="$(DEPLIB_BUILDDIR)" || exit $$?;)

testlibs-debug: testedlib-debug
	@$(foreach lib,$(LIBS),$(MAKE) -s -C ../../$(lib) debug BUILDDIR="$(DEPLIB_BUILDDIR)" || exit $$?;)

#
# Coverage rules
#
testedlib-coverage:
	@$(MAKE) -s -C ../ coverage BUILDDIR="$(TESTEDLIB_BUILDDIR)"

coverage: coverage-html

$(COV_EXE): $(COV_OBJS) testlibs-coverage
	@$(CC) $(COV_LDPATH) $(COV_LDFLAGS) $(COV_RPATH) $(WRAPFLAGS) -o $(COV_EXE) $(COV_OBJS) $(COV_LIB_OBJS) $(EXT_LDLIBS)
	@echo "$@ linked dynamically with coverage flags, not stripped"

$(COV_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) $(TMPLTS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$< compiled with coverage flags"

$(COV_OBJDIR):
	@mkdir -p "$(COV_OBJDIR)"

coverage-report: $(COV_EXE)
	@echo "Generating coverage report..."
	./$(COV_EXE)
	@echo "Coverage run finished. Inspect .gcda/.gcno outputs for details"

coverage-html: coverage-report
	@echo "Generating HTML coverage report..."
	@lcov --capture --branch-coverage --rc lcov_excl_br_line='(^\s*RETURN_STATUS|SLOWTEST\s*;)|(\b(?:provide|deliver|ask|run|call|ASSERT|IF|HEADER|TEST|SUTE)\s*\()' --directory $(COV_OBJDIR) --directory $(COV_LIB_OBJDIR) --output-file $(COV_OBJDIR)/coverage.info
	@genhtml --branch-coverage $(COV_OBJDIR)/coverage.info --output-directory $(COV_REPORTDIR)
	@echo "HTML coverage report generated in $(COV_REPORTDIR) directory"

#
# Sanitize rules
#
testedlib-sanitize:
	@$(MAKE) -s -C ../ sanitize BUILDDIR="$(TESTEDLIB_BUILDDIR)"

sanitize: $(SNTZ_EXE) | testlibs-sanitize
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(SNTZ_EXE)

$(SNTZ_EXE): $(SNTZ_OBJS) testlibs-sanitize
	@$(CC) $(SNTZ_LDPATH) $(SNTZ_LDFLAGS) $(SNTZ_RPATH) $(WRAPFLAGS) -o $(SNTZ_EXE) $(SNTZ_OBJS) $(SNTZ_LIB_OBJS) $(EXT_LDLIBS)
	@echo "$@ linked dynamically with sanitizer flags, not stripped"

$(SNTZ_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) $(TMPLTS) | $(SNTZ_OBJDIR)
	@$(CC) -c $(INCPATH) $(SNTZ_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$< compiled with sanitizer flags"

$(SNTZ_OBJDIR):
	@mkdir -p "$(SNTZ_OBJDIR)"

#
# Debug rules
#
testedlib-debug:
	@$(MAKE) -s -C ../ debug BUILDDIR="$(TESTEDLIB_BUILDDIR)"

debug-build: $(DBG_EXE)

debug: debug-build | testlibs-debug
	@./$(DBG_EXE)

$(DBG_EXE): $(DBG_OBJS) testlibs-debug
	@$(CC) $(DBG_LDPATH) $(DBG_RPATH) $(STATIC) $(WRAPFLAGS) -o $(DBG_EXE) $(DBG_OBJS) $(DBG_LIB_OBJS) $(EXT_LDLIBS)
ifneq ($(TESTS_DYNAMIC),)
	@echo "$@ linked dynamically with debug flags, not stripped"
else
	@echo "$@ linked statically with debug flags, not stripped"
endif

$(DBG_OBJDIR)/%.o: $(SRC)/%.c $(HDRS) $(TMPLTS) | $(DBG_OBJDIR)
	@$(CC) -c $(INCPATH) $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$< compiled with debug flags"

$(DBG_OBJDIR):
	@mkdir -p "$(DBG_OBJDIR)"

#
# Format rules
#
format:
	@echo "Formatting source files..."
	@for file in $(SRCS) $(LOCAL_HDRS) $(TMPLTS); do \
		echo "Formatting $$file"; \
		uncrustify -c ../../../Uncrustify.cfg --replace --no-backup $$file; \
	done
	@echo "All files formatted."

purge:
	@$(MAKE) -s -C ../ purge

clean-compile-commands:
	@rm -f $(COMPILE_COMMANDS)

clean: clean-compile-commands
	@rm -f $(SNTZ_EXE) $(DBG_EXE) $(COV_EXE)
	@rm -f $(DBG_OBJS)
	@rm -f $(SNTZ_OBJS)
	@rm -f $(COV_OBJS)

	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

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
	@mkdir -p "$(BUILDDIR)/$(LIBNAME)"
	bear --output $@ -- $(MAKE) -B -s debug-build

cppcheck: compile-commands
	cppcheck --project=$(COMPILE_COMMANDS) --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force --inconclusive

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
