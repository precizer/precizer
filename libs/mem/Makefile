# Name of the library
LIBNAME=libmem

BUILDDIR = ../../.builds

UNAME_S := $(shell uname -s)

#
# Compiler flags
#
# USE_LLD and AR are exported by the root Makefile when COMPILER=clang
# to use LLVM's own linker and archiver, avoiding gold plugin version mismatch.
CFLAGS += -pipe -std=c2x -finline-functions
CFLAGS += -fbuiltin

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
else
WFLAGS += -Wno-gnu-zero-variadic-macro-arguments
endif

#
# Project files
#
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h)

# Exclude a file
OBJS = $(SRCS:$(SRC_DIR)/%.c=%.o)

LIBS = rational

EXAMPLE_BIN = example/example
EXAMPLE_SRC = example/example.c

# Additional include paths for external libraries
INCPATH += -I$(SRC_DIR)
INCPATH += $(foreach d,$(LIBS),-I../$d/src/)
LDLIBS = $(foreach l,$(LIBS),-l$(l))

#
# Debug build settings
#
DBG = debug
DBG_DIR = $(BUILDDIR)/$(DBG)
DBG_LIBDIR = $(DBG_DIR)/libs
DBG_OBJDIR = $(DBG_LIBDIR)/obj
DBG_LDPATH = -L$(DBG_LIBDIR)
DBG_SO = $(DBG_LIBDIR)/$(SO)
DBG_STAT = $(DBG_LIBDIR)/$(STATLIB)
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(OBJS))
DBG_CFLAGS = $(CFLAGS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -fno-omit-frame-pointer -DDEBUG
ifeq ($(UNAME_S),Darwin)
DBG_LDFLAGS = $(USE_LLD) -Wl,-undefined,dynamic_lookup
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
DBG_LDFLAGS = $(USE_LLD) -Wl,--as-needed
else
DBG_LDFLAGS = $(USE_LLD) -Wl,-z,defs -Wl,--as-needed
endif

#
# Coverage build settings
#
COV = coverage
COV_DIR = $(BUILDDIR)/$(COV)
COV_LIBDIR = $(COV_DIR)/libs
COV_OBJDIR = $(COV_LIBDIR)/obj
COV_LDPATH = -L$(COV_LIBDIR)
COV_SO = $(COV_LIBDIR)/$(SO)
COV_STAT = $(COV_LIBDIR)/$(STATLIB)
COV_OBJS = $(addprefix $(COV_OBJDIR)/, $(OBJS))
COV_CFLAGS = $(CFLAGS) -fprofile-arcs -ftest-coverage -g -O0 -fno-omit-frame-pointer -DDEBUG
COV_LDFLAGS += $(USE_LLD) -lgcov --coverage

#
# Sanitize build settings
#
SNTZ = sanitize
SNTZ_DIR = $(BUILDDIR)/$(SNTZ)
SNTZ_LIBDIR = $(SNTZ_DIR)/libs
SNTZ_OBJDIR = $(SNTZ_LIBDIR)/obj
SNTZ_LDPATH = -L$(SNTZ_LIBDIR)
SNTZ_SO = $(SNTZ_LIBDIR)/$(SO)
SNTZ_STAT = $(SNTZ_LIBDIR)/$(STATLIB)
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(OBJS))
SNTZ_OPTIONS = -fsanitize=address,undefined -fno-omit-frame-pointer
ifeq ($(UNAME_S),Darwin)
SNTZ_LDFLAGS = -Wl,-undefined,dynamic_lookup -install_name @rpath/$(SO)
endif
SNTZ_CFLAGS = $(DBG_CFLAGS) $(SNTZ_OPTIONS)
SNTZ_LDFLAGS += $(USE_LLD) $(SNTZ_OPTIONS)

#
# Production build settings
#
PROD = production
PROD_DIR = $(BUILDDIR)/$(PROD)
PROD_LIBDIR = $(PROD_DIR)/libs
PROD_OBJDIR = $(PROD_LIBDIR)/obj
PROD_LDPATH = -L$(PROD_LIBDIR)
PROD_SO = $(PROD_LIBDIR)/$(SO)
PROD_STAT = $(PROD_LIBDIR)/$(STATLIB)
PROD_OBJS = $(addprefix $(PROD_OBJDIR)/, $(OBJS))
PROD_CFLAGS ?= $(CFLAGS) $(LTO) -O3 -march=native -funroll-loops -pipe -ffunction-sections -fdata-sections -fomit-frame-pointer
ifeq ($(UNAME_S),Darwin)
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(LTO) -Wl,-O3 -Wl,-dead_strip -Wl,-undefined,dynamic_lookup
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(LTO) -Wl,-O3 -Wl,--gc-sections
else
PROD_LDFLAGS ?= $(LDFLAGS) $(USE_LLD) $(LTO) -Wl,-O3 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,--gc-sections -Wl,-z,defs
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
PRTB_OBJDIR = $(PRTB_LIBDIR)/obj
PRTB_LDPATH = -L$(PRTB_LIBDIR)
PRTB_SO = $(PRTB_LIBDIR)/$(SO)
PRTB_STAT = $(PRTB_LIBDIR)/$(STATLIB)
PRTB_OBJS = $(addprefix $(PRTB_OBJDIR)/, $(OBJS))
PRTB_CFLAGS = $(CFLAGS) $(LTO) -O2 -mtune=generic -funroll-loops -pipe -ffunction-sections -fdata-sections -fomit-frame-pointer
ifeq ($(UNAME_S),Darwin)
PRTB_LDFLAGS = $(USE_LLD) $(LTO) -Wl,-O2 -Wl,-dead_strip -Wl,-undefined,dynamic_lookup
else ifneq ($(findstring CYGWIN,$(UNAME_S)),)
PRTB_LDFLAGS = $(USE_LLD) $(LTO) -Wl,-O2 -Wl,--gc-sections
else
PRTB_LDFLAGS = $(USE_LLD) $(LTO) -Wl,-O2 -Wl,--hash-style=both -Wl,--as-needed -Wl,--gc-sections -Wl,-z,defs
endif
# Extra warnings enabled only for GCC
ifneq ($(GCC),)
PRTB_WFLAGS += -Wsuggest-attribute=const
PRTB_WFLAGS += -Wsuggest-attribute=pure
PRTB_WFLAGS += -Wsuggest-attribute=noreturn
PRTB_WFLAGS += -Wsuggest-attribute=format
PRTB_WFLAGS += -Wmissing-format-attribute
endif

.PHONY: all clean debug production prod portable sanitize format gcc-analyzer example
.PHONY: coverage memtest tests test cppcheck clang-analyzer print-%

# Default build
all: debug production portable

#
# Debug rules
#
memtest: debug
tests: test
test: sanitize
debug: $(DBG_SO) $(DBG_STAT)

$(DBG_STAT): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(AR) rcs $@ $+
	@echo $@ prepared to be static library

$(DBG_SO): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(CC) -shared $(DBG_LDFLAGS) $(DBG_LDPATH) -o $@ $^ $(LDLIBS)
	@echo $@ prepared to be shared library

$(DBG_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(DBG_OBJDIR)
	@$(CC) -c -fPIC $(INCPATH) $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(DBG_OBJDIR):
	@mkdir -p "$@"

$(DBG_LIBDIR):
	@mkdir -p "$@"
	@$(foreach lib,$(LIBS),$(MAKE) -s -C ../$(lib) debug;)

#
# Coverage rules
#
coverage: $(COV_SO) $(COV_STAT)

$(COV_STAT): $(COV_OBJS) | $(COV_LIBDIR)
	@$(AR) rcs $@ $+
	@echo $@ prepared to be static library

$(COV_SO): $(COV_OBJS) | $(COV_LIBDIR)
	@$(CC) -shared $(COV_LDFLAGS) $(COV_LDPATH) -o $@ $^ $(LDLIBS)
	@echo $@ prepared to be shared library

$(COV_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(COV_OBJDIR)
	@$(CC) -c -fPIC $(INCPATH) $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(COV_OBJDIR):
	@mkdir -p "$@"

$(COV_LIBDIR):
	@mkdir -p "$@"
	@$(foreach lib,$(LIBS),$(MAKE) -s -C ../$(lib) coverage;)

#
# Sanitize rules
#
sanitize: $(SNTZ_SO) $(SNTZ_STAT)

$(SNTZ_STAT): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(AR) rcs $@ $+
	@echo $@ prepared to be the static library

$(SNTZ_SO): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(CC) -shared $(SNTZ_LDFLAGS) $(SNTZ_LDPATH) -o $@ $^ $(LDLIBS)
	@echo $@ prepared to be the shared library

$(SNTZ_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(SNTZ_OBJDIR)
	@$(CC) -c -fPIC $(INCPATH) $(WFLAGS) $(SNTZ_CFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(SNTZ_OBJDIR):
	@mkdir -p "$@"

$(SNTZ_LIBDIR):
	@mkdir -p "$@"
	@$(foreach lib,$(LIBS),$(MAKE) -s -C ../$(lib) sanitize;)

#
# Production rules
#
prod: production
production: $(PROD_SO) $(PROD_STAT)

$(PROD_STAT): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(AR) rcs $@ $+
	@echo $@ prepared to be the static library

$(PROD_SO): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(CC) -shared $(STRIP) $(PROD_LDFLAGS) $(PROD_LDPATH) -o $@ $^ $(LDLIBS)
	@echo $@ prepared to be the shared library

$(PROD_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PROD_OBJDIR)
	@$(CC) -c -fPIC $(INCPATH) $(WFLAGS) $(PROD_WFLAGS) $(PROD_CFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(PROD_OBJDIR):
	@mkdir -p "$@"

$(PROD_LIBDIR):
	@mkdir -p "$@"
	@$(foreach lib,$(LIBS),$(MAKE) -s -C ../$(lib) production;)

#
# Portable rules
#
portable: $(PRTB_SO) $(PRTB_STAT)

$(PRTB_STAT): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(AR) rcs $@ $+
	@echo $@ prepared to be the static library

$(PRTB_SO): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(CC) -shared $(STRIP) $(PRTB_LDFLAGS) $(PRTB_LDPATH) -o $@ $^ $(LDLIBS)
	@echo $@ prepared to be the shared library

$(PRTB_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PRTB_OBJDIR)
	@$(CC) -c -fPIC $(INCPATH) $(WFLAGS) $(PRTB_WFLAGS) $(PRTB_CFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(PRTB_OBJDIR):
	@mkdir -p "$@"

$(PRTB_LIBDIR):
	@mkdir -p "$@"
	@$(foreach lib,$(LIBS),$(MAKE) -s -C ../$(lib) portable;)

clean:
	@rm -f $(EXAMPLE_BIN)
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

cppcheck:
	cppcheck --enable=all --platform=unix64 --std=c2x -q --force --inconclusive $(SRC_DIR)

clang-analyzer: CC = clang
clang-analyzer:
	# Run clang static analyzer and view analysis results in a web browser when the build command completes
	scan-build -V make debug

# Optional preprocessor files
$(SRC_DIR)/%.i: $(SRC_DIR)/%.c
	@$(CC) -E -C -o $@ $(INCPATH) $(PROD_CFLAGS) $<
	@bcpp -na -s -i 4 $@ > $@.h
	@sed -i 's/[ \t]*\# [[:digit:]]\+ \".*//g' $@.h

example: $(EXAMPLE_BIN)

$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(PROD_STAT)
	$(CC) $(INCPATH) $(STATIC) $(PROD_CFLAGS) $(PROD_LDPATH) $(WFLAGS) -o $@ $(EXAMPLE_SRC) $(PROD_STAT) $(LDLIBS)
	@echo "$@ built"

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
