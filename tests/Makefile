# Name of the application
EXE=testitall

#
# Compiler flags
#

CFLAGS += -pipe -std=c2x -finline-functions
CFLAGS += -fbuiltin
# The definition used to disable the main() function
# of the tested application
CFLAGS += -DTESTITALL

UNAME_S := $(shell uname -s)

# macOS-specific code
ifeq ($(UNAME_S),Darwin)
CFLAGS += -DEVIL_EMPIRE_OS
endif

SRC = src
STATIC = -static -static-libgcc -Wl,--gc-sections

PRZRSRC = ../src
BUILDS = ../.builds
BUILDEXE = $(BUILDS)/$(EXE)
BUILDEXEDIR = $(dir $(BUILDEXE))
LIBS = testitall mem sha512 rational sqlite3 xdiff

# Extra libs for linking
LDLIBS += $(foreach d,$(LIBS),-l$d) -lpcre2-8

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
COV_CFLAGS += $(CFLAGS) -g -O0 -fprofile-arcs -ftest-coverage
COV_LDFLAGS += -lgcov --coverage

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
SNTZ_OPTIONS = -fsanitize=address,undefined -static-libasan -fno-omit-frame-pointer
SNTZ_CFLAGS = $(DBG_CFLAGS) $(SNTZ_OPTIONS)
SNTZ_LDFLAGS = -Wl,-z,defs $(SNTZ_OPTIONS)
ifeq ($(UNAME_S),Darwin)
SNTZ_OPTIONS = -fsanitize=address,undefined -fno-omit-frame-pointer
SNTZ_LDFLAGS = $(SNTZ_OPTIONS)
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

DBG_CFLAGS += $(CFLAGS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG

.PHONY: all clean debug sanitize coverage run test tests coverage-report coverage-html format
.PHONY: precizer-coverage precizer-sanitize precizer-debug hugetestfile purge gcc-analyzer cppcheck print-%

# Default build
all: sanitize

test: sanitize

tests: test

run: sanitize

#
# Coverage rules
#
precizer-coverage:
	@$(MAKE) -s -C ../ precizer-coverage

coverage: coverage-html

$(COV_EXE): $(COV_OBJS) $(COV_PRZROBJS) | precizer-coverage
	@$(CC) $(COV_LIBS) $(COV_LDFLAGS) $(COV_DYNLIB) -o $(COV_EXE) $^ $(LDLIBS)
	@echo "$@ linked."

$(COV_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(COV_OBJDIR)/%.o: $(PRZRSRC)/%.c $(ALLHDRS) $(TMPLTS) | $(COV_OBJDIR)
	@$(CC) -c $(INCPATH) $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(COV_OBJDIR):
	@mkdir -p "$(COV_OBJDIR)"

coverage-report: $(COV_EXE)
	@echo "Generating coverage report..."
	./$(COV_EXE)
	@echo "Coverage run finished. Inspect .gcda/.gcno outputs for details"

coverage-html: coverage-report
	@echo "Generating HTML coverage report..."
	@lcov --capture --directory $(COV_OBJDIR) --directory $(COV_LIBSOBJ) --output-file $(COV_OBJDIR)/coverage.info
	@genhtml $(COV_OBJDIR)/coverage.info --output-directory $(COV_REPORTDIR)
	@echo "HTML coverage report generated in $(COV_REPORTDIR) directory"

#
# Sanitize rules
#
precizer-sanitize:
	@$(MAKE) -s -C ../ sanitize

sanitize: hugetestfile $(SNTZ_EXE)
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(SNTZ_EXE)

$(SNTZ_EXE): $(SNTZ_OBJS) $(SNTZ_PRZROBJS) | precizer-sanitize
	@$(CC) $(SNTZ_LIBS) $(SNTZ_LDFLAGS) $(SNTZ_DYNLIB) -o $(SNTZ_EXE) $^ $(LDLIBS)
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

debug: hugetestfile $(DBG_EXE)
	@./$(DBG_EXE)

$(DBG_EXE): $(DBG_OBJS) $(DBG_PRZROBJS) | precizer-debug
	@$(CC) $(DBG_LIBS) $(DBG_DYNLIB) $(STATIC) -o $(DBG_EXE) $^ $(LDLIBS)
	@echo "$@ linked."

$(DBG_OBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(DBG_OBJDIR)
	@$(CC) -c $(INCPATH) $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(DBG_OBJDIR)/%.o: $(PRZRSRC)/%.c $(ALLHDRS) $(TMPLTS) | $(DBG_OBJDIR)
	@$(CC) -c $(INCPATH) $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(DBG_OBJDIR):
	@mkdir -p "$(DBG_OBJDIR)"

HUGETESTFILE = examples/huge/hugetestfile
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

clean:
	@rm -f $(HUGETESTFILE)
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

purge:
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
# Printing variables
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
gcc-analyzer: WFLAGS += -fanalyzer -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3
gcc-analyzer: CC = gcc
gcc-analyzer: debug

cppcheck:
	cppcheck --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force $(INCPATH) --inconclusive ./src
