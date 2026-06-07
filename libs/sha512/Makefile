# Name of the library
LIBNAME=libsha512

# Defaults to a local .builds when running `make` from this directory.
# When the umbrella project drives the build, BUILDDIR is overridden via the
# library dispatcher Makefile to point at the project-wide .builds
BUILDDIR ?= .builds
COMPILE_COMMANDS = $(BUILDDIR)/compile_commands.json

UNAME_S := $(shell uname -s)

#
# Compiler flags
#
# USE_LLD and AR are exported by the root Makefile when COMPILER=clang
# to use LLVM's own linker and archiver, avoiding gold plugin version mismatch
CFLAGS += -std=c2x -finline-functions

SO = $(LIBNAME).so
STATLIB = $(LIBNAME).a

# Detect whether we're using GCC
GCC := $(findstring gcc,$(notdir $(firstword $(CC))))

ifeq ($(UNAME_S),Darwin)
STRIP ?= -Wl,-x
else
STRIP ?= -s
endif

ifneq ($(findstring CYGWIN,$(UNAME_S)),)
LTO =
else
LTO = -flto=auto
endif

# Warning flags for additional checks
WFLAGS += -Wall
WFLAGS += -Werror # Treat every warning as an error (-Werror stops the build)
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
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h)

# Exclude a file
OBJS = $(SRCS:$(SRC_DIR)/%.c=%.o)
LIBDIR = $(patsubst lib%,%,$(LIBNAME))

#
# Debug build settings
#
DBG = debug
DBG_DIR = $(BUILDDIR)/$(DBG)
DBG_LIBDIR = $(DBG_DIR)/libs
DBG_OBJDIR = $(DBG_LIBDIR)/obj/$(LIBDIR)
DBG_SO = $(DBG_LIBDIR)/$(SO)
DBG_STAT = $(DBG_LIBDIR)/$(STATLIB)
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(OBJS))
DBG_OPT_CFLAGS ?= -pipe -fbuiltin -O0 -fno-omit-frame-pointer
DBG_CFLAGS = $(CFLAGS) $(DBG_OPT_CFLAGS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -DDEBUG
ifeq ($(UNAME_S),Darwin)
DBG_LDFLAGS = $(USE_LLD) -Wl,-undefined,dynamic_lookup
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
DBG_LDFLAGS = $(USE_LLD)
else
DBG_LDFLAGS = $(USE_LLD) -Wl,-z,defs
endif

#
# Coverage build settings
#
COV = coverage
COV_DIR = $(BUILDDIR)/$(COV)
COV_LIBDIR = $(COV_DIR)/libs
COV_OBJDIR = $(COV_LIBDIR)/obj/$(LIBDIR)
COV_SO = $(COV_LIBDIR)/$(SO)
COV_STAT = $(COV_LIBDIR)/$(STATLIB)
COV_OBJS = $(addprefix $(COV_OBJDIR)/, $(OBJS))
COV_OPT_CFLAGS ?= -pipe -fbuiltin -O0 -fno-omit-frame-pointer
COV_CFLAGS = $(CFLAGS) -fprofile-arcs -ftest-coverage $(COV_OPT_CFLAGS) -g -DDEBUG
COV_LDFLAGS += $(USE_LLD) -lgcov --coverage

#
# Sanitize build settings
#
SNTZ = sanitize
SNTZ_DIR = $(BUILDDIR)/$(SNTZ)
SNTZ_LIBDIR = $(SNTZ_DIR)/libs
SNTZ_OBJDIR = $(SNTZ_LIBDIR)/obj/$(LIBDIR)
SNTZ_SO = $(SNTZ_LIBDIR)/$(SO)
SNTZ_STAT = $(SNTZ_LIBDIR)/$(STATLIB)
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(OBJS))
SNTZ_OPTIONS = -fsanitize=address,undefined
ifeq ($(UNAME_S),Darwin)
SNTZ_LDFLAGS = -Wl,-undefined,dynamic_lookup -install_name @rpath/$(SO)
endif
SNTZ_OPT_CFLAGS ?= $(DBG_OPT_CFLAGS)
SNTZ_CFLAGS = $(CFLAGS) $(SNTZ_OPT_CFLAGS) $(SNTZ_OPTIONS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -DDEBUG
SNTZ_LDFLAGS += $(USE_LLD) $(SNTZ_OPTIONS)

#
# Production build settings
#
PROD = production
PROD_DIR = $(BUILDDIR)/$(PROD)
PROD_LIBDIR = $(PROD_DIR)/libs
PROD_OBJDIR = $(PROD_LIBDIR)/obj/$(LIBDIR)
PROD_SO = $(PROD_LIBDIR)/$(SO)
PROD_STAT = $(PROD_LIBDIR)/$(STATLIB)
PROD_OBJS = $(addprefix $(PROD_OBJDIR)/, $(OBJS))
PROD_OPT_CFLAGS ?= -pipe -fbuiltin $(LTO) -O3 -march=native -funroll-loops -ffunction-sections -fdata-sections -fomit-frame-pointer
PROD_CFLAGS ?= $(CFLAGS) $(PROD_OPT_CFLAGS)
ifeq ($(UNAME_S),Darwin)
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,-dead_strip
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(PROD_OPT_LDFLAGS) -Wl,-undefined,dynamic_lookup
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,--gc-sections
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(PROD_OPT_LDFLAGS)
else
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,--gc-sections
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(PROD_OPT_LDFLAGS) -Wl,-z,defs
endif
# Extra warnings enabled only for GCC
ifneq ($(GCC),)
PROD_WFLAGS += -Wsuggest-attribute=const
PROD_WFLAGS += -Wsuggest-attribute=pure
PROD_WFLAGS += -Wsuggest-attribute=noreturn
PROD_WFLAGS += -Wsuggest-attribute=format
PROD_WFLAGS += -Wmissing-format-attribute
endif

#
# Portable build settings
#
PRTB = portable
PRTB_DIR = $(BUILDDIR)/$(PRTB)
PRTB_LIBDIR = $(PRTB_DIR)/libs
PRTB_OBJDIR = $(PRTB_LIBDIR)/obj/$(LIBDIR)
PRTB_SO = $(PRTB_LIBDIR)/$(SO)
PRTB_STAT = $(PRTB_LIBDIR)/$(STATLIB)
PRTB_OBJS = $(addprefix $(PRTB_OBJDIR)/, $(OBJS))
PRTB_OPT_CFLAGS ?= -pipe -fbuiltin $(LTO) -O2 -mtune=generic -funroll-loops -ffunction-sections -fdata-sections -fomit-frame-pointer
PRTB_CFLAGS = $(CFLAGS) $(PRTB_OPT_CFLAGS)
ifeq ($(UNAME_S),Darwin)
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,-dead_strip
PRTB_LDFLAGS = $(USE_LLD) $(PRTB_OPT_LDFLAGS) -Wl,-undefined,dynamic_lookup
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,--gc-sections
PRTB_LDFLAGS = $(USE_LLD) $(PRTB_OPT_LDFLAGS)
else
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,--hash-style=both -Wl,--as-needed -Wl,--gc-sections
PRTB_LDFLAGS = $(USE_LLD) $(PRTB_OPT_LDFLAGS) -Wl,-z,defs
endif
# Extra warnings enabled only for GCC
ifneq ($(GCC),)
PRTB_WFLAGS += -Wsuggest-attribute=const
PRTB_WFLAGS += -Wsuggest-attribute=pure
PRTB_WFLAGS += -Wsuggest-attribute=noreturn
PRTB_WFLAGS += -Wsuggest-attribute=format
PRTB_WFLAGS += -Wmissing-format-attribute
endif

.PHONY: all purge clean debug production prod portable sanitize format gcc-analyzer
.PHONY: coverage memtest tests test tests-sanitize tests-debug tests-coverage
.PHONY: cppcheck clang-analyzer compile-commands clean-compile-commands print-%

# Default build
all: debug production portable

#
# Debug rules
#
memtest: debug
tests: test
test: tests-sanitize

tests-sanitize:
	@$(MAKE) -s -C tests/ sanitize BUILDDIR=../$(BUILDDIR)

tests-debug:
	@$(MAKE) -s -C tests/ debug BUILDDIR=../$(BUILDDIR)

tests-coverage:
	@$(MAKE) -s -C tests/ coverage-report BUILDDIR=../$(BUILDDIR)
debug: $(DBG_SO) $(DBG_STAT)

$(DBG_STAT): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$@ prepared as debug static library archive"

$(DBG_SO): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(CC) -shared $(DBG_LDFLAGS) -o $@ $^
	@echo "$@ prepared as debug shared library, not stripped"

$(DBG_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(DBG_OBJDIR)
	@$(CC) -c -fPIC $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled with debug flags"

$(DBG_OBJDIR): | $(DBG_LIBDIR)
	@mkdir -p "$@"

$(DBG_LIBDIR):
	@mkdir -p "$@"

#
# Coverage rules
#
coverage: $(COV_SO) $(COV_STAT)

$(COV_STAT): $(COV_OBJS) | $(COV_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$@ prepared as coverage static library archive"

$(COV_SO): $(COV_OBJS) | $(COV_LIBDIR)
	@$(CC) -shared $(COV_LDFLAGS) -o $@ $^
	@echo "$@ prepared as coverage shared library, not stripped"

$(COV_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(COV_OBJDIR)
	@$(CC) -c -fPIC $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled with coverage flags"

$(COV_OBJDIR): | $(COV_LIBDIR)
	@mkdir -p "$@"

$(COV_LIBDIR):
	@mkdir -p "$@"

#
# Sanitize rules
#
sanitize: $(SNTZ_SO) $(SNTZ_STAT)

$(SNTZ_STAT): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$@ prepared as sanitizer static library archive"

$(SNTZ_SO): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(CC) -shared $(SNTZ_LDFLAGS) -o $@ $^
	@echo "$@ prepared as sanitizer shared library, not stripped"

$(SNTZ_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(SNTZ_OBJDIR)
	@$(CC) -c -fPIC $(WFLAGS) $(SNTZ_CFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled with sanitizer flags"

$(SNTZ_OBJDIR): | $(SNTZ_LIBDIR)
	@mkdir -p "$@"

$(SNTZ_LIBDIR):
	@mkdir -p "$@"

#
# Production rules
#
prod: production
production: $(PROD_SO) $(PROD_STAT)

$(PROD_STAT): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$@ prepared as release static library archive"

$(PROD_SO): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(CC) -shared $(STRIP) $(PROD_LDFLAGS) -o $@ $^
	@echo "$@ prepared as release shared library, stripped"

$(PROD_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PROD_OBJDIR)
	@$(CC) -c -fPIC $(WFLAGS) $(PROD_WFLAGS) $(PROD_CFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled with release flags"

$(PROD_OBJDIR): | $(PROD_LIBDIR)
	@mkdir -p "$@"

$(PROD_LIBDIR):
	@mkdir -p "$@"

#
# Portable rules
#
portable: $(PRTB_SO) $(PRTB_STAT)

$(PRTB_STAT): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$@ prepared as portable static library archive"

$(PRTB_SO): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(CC) -shared $(STRIP) $(PRTB_LDFLAGS) -o $@ $^
	@echo "$@ prepared as portable shared library, stripped"

$(PRTB_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PRTB_OBJDIR)
	@$(CC) -c -fPIC $(WFLAGS) $(PRTB_WFLAGS) $(PRTB_CFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled with portable flags"

$(PRTB_OBJDIR): | $(PRTB_LIBDIR)
	@mkdir -p "$@"

$(PRTB_LIBDIR):
	@mkdir -p "$@"

purge: clean-compile-commands
	@test -d $(BUILDDIR) && rm -rf $(BUILDDIR) 2>/dev/null || true
	@test -f $(EXE) && rm $(EXE) 2>/dev/null || true
	@echo $(LIBNAME) artifacts cleared

clean-compile-commands:
	@rm -f $(COMPILE_COMMANDS)

clean: clean-compile-commands
	@rm -rf $(PROD_SO) $(PROD_STAT) $(PROD_OBJS)
	@rm -rf $(PRTB_SO) $(PRTB_STAT) $(PRTB_OBJS)
	@rm -rf $(DBG_SO) $(DBG_STAT) $(DBG_OBJS)
	@rm -rf $(COV_SO) $(COV_STAT) $(COV_OBJS)
	@rm -rf $(SNTZ_SO) $(SNTZ_STAT) $(SNTZ_OBJS)

	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

	@test -d $(COV_OBJDIR) && rm -d $(COV_OBJDIR) 2>/dev/null || true
	@test -d $(COV_LIBDIR) && rm -d $(COV_LIBDIR) 2>/dev/null || true
	@test -d $(COV_DIR) && rm -d $(COV_DIR) 2>/dev/null || true

	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_LIBDIR) && rm -d $(SNTZ_LIBDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true

	@test -d $(PROD_OBJDIR) && rm -d $(PROD_OBJDIR) 2>/dev/null || true
	@test -d $(PROD_LIBDIR) && rm -d $(PROD_LIBDIR) 2>/dev/null || true
	@test -d $(PROD_DIR) && rm -d $(PROD_DIR) 2>/dev/null || true

	@test -d $(PRTB_OBJDIR) && rm -d $(PRTB_OBJDIR) 2>/dev/null || true
	@test -d $(PRTB_LIBDIR) && rm -d $(PRTB_LIBDIR) 2>/dev/null || true
	@test -d $(PRTB_DIR) && rm -d $(PRTB_DIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true
	@echo $(LIBNAME) cleared

#
# Format rules
#
format:
	@echo "Formatting source files..."
	@for file in $(SRCS) $(HDRS); do \
	    echo "Formatting $$file"; \
	    uncrustify -c ../../Uncrustify.cfg --replace --no-backup $$file; \
	done
	@echo "All files formatted."

#
# GCC Static Analysis
#
gcc-analyzer: DBG_CFLAGS += -fanalyzer -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3
# -Wanalyzer-too-complex
gcc-analyzer: CC = gcc
gcc-analyzer: debug

compile-commands: $(COMPILE_COMMANDS)

$(COMPILE_COMMANDS):
	@mkdir -p "$(BUILDDIR)"
	bear --output $@ -- $(MAKE) -B -s -C tests debug-build

cppcheck: compile-commands
	cppcheck --check-level=exhaustive --project=$(COMPILE_COMMANDS) --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force --inconclusive

# Run clang static analyzer and view analysis results in a web browser when the build command completes
clang-analyzer: CC = clang
clang-analyzer:
	scan-build -V make debug

# Optional preprocessor files
$(SRC_DIR)/%.i: $(SRC_DIR)/%.c
	@$(CC) -E -C -o $@ $(INCPATH) $(CFLAGS) $<
	@bcpp -na -s -i 4 $@ > $@.h
	@sed -i 's/[ \t]*\# [[:digit:]]\+ \".*//g' $@.h

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
