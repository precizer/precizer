# Name of the library
LIBNAME=libsqlite3

# Defaults to a local .builds when running `make` from this directory.
# When the umbrella project drives the build, BUILDDIR is overridden via the
# library dispatcher Makefile to point at the project-wide .builds
BUILDDIR ?= .builds
# Returns a concise path for build messages.
# Plain file names stay unchanged; nested paths keep only final-directory/file
short_path = $(if $(filter ./,$(dir $(1))),$(notdir $(1)),$(notdir $(patsubst %/,%,$(dir $(1))))/$(notdir $(1)))

UNAME_S := $(shell uname -s)

#
# Compiler flags
#
# USE_LLD and AR are exported by the root Makefile when COMPILER=clang
# to use LLVM's own linker and archiver, avoiding gold plugin version mismatch
CFLAGS += -std=c2x -finline-functions
CFLAGS += -w

# Disable threads and other options
CFLAGS += -DSQLITE_LIKE_DOESNT_MATCH_BLOBS \
 -DSQLITE_MAX_EXPR_DEPTH=0 \
 -DSQLITE_OMIT_BTREECOUNT \
 -DSQLITE_OMIT_COMPILEOPTION_DIAGS \
 -DSQLITE_OMIT_DECLTYPE \
 -DSQLITE_OMIT_DEPRECATED \
 -DSQLITE_OMIT_EXPLAIN \
 -DSQLITE_OMIT_FLOATING_POINT \
 -DSQLITE_OMIT_JSON \
 -DSQLITE_OMIT_LOAD_EXTENSION \
 -DSQLITE_OMIT_PROGRESS_CALLBACK \
 -DSQLITE_OMIT_SHARED_CACHE \
 -DSQLITE_OMIT_TCL \
 -DSQLITE_OMIT_TRACE \
 -DSQLITE_OMIT_UTF16 \
 -DSQLITE_THREADSAFE=0 \
 -DTHREADSAFE=0
# -DSQLITE_OMIT_SUBQUERY \
# -DSQLITE_OMIT_TRIGGER \
# -DSQLITE_OMIT_VIEW \

SO = $(LIBNAME).so
STATLIB = $(LIBNAME).a

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
WFLAGS += -Wpedantic
WFLAGS += -Wshadow
WFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs

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
# This coverage target intentionally does not add gcov instrumentation.
# This keeps coverage statistics focused on files whose coverage has strategic value
COV_OPT_CFLAGS ?= -pipe -fbuiltin -O0 -fno-omit-frame-pointer
COV_CFLAGS = $(CFLAGS) $(COV_OPT_CFLAGS) -g -DDEBUG

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

.PHONY: all purge clean debug production prod portable sanitize gcc-analyzer compile-commands
.PHONY: coverage memtest tests test tests-sanitize tests-debug tests-coverage format print-%

define SQLITE_WARN_ONCE
$(if $(SQLITE_WARNING_PRINTED),,\
echo "SQLite library compilation has started. This process may take a considerable amount of time" \
$(eval SQLITE_WARNING_PRINTED:=1))
endef

# Default build
all: debug production portable

#
# GCC Static Analysis
#
gcc-analyzer: CC = gcc
gcc-analyzer: debug

compile-commands:
	@true

#
# Debug rules
#
memtest: debug
tests: test
test: tests-sanitize

tests-sanitize:
	@true

tests-debug:
	@true

tests-coverage:
	@true

debug: $(DBG_SO) $(DBG_STAT)

$(DBG_STAT): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$(call short_path,$@) prepared as debug static library archive"

$(DBG_SO): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(CC) -shared $(DBG_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as debug shared library, not stripped"

$(DBG_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(DBG_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with debug flags"

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
	@echo "$(call short_path,$@) prepared as coverage static library archive"

$(COV_SO): $(COV_OBJS) | $(COV_LIBDIR)
	@$(CC) -shared -o $@ $^
	@echo "$(call short_path,$@) prepared as coverage shared library, not stripped"

$(COV_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(COV_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(COV_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with coverage flags"

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
	@echo "$(call short_path,$@) prepared as sanitizer static library archive"

$(SNTZ_SO): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(CC) -shared $(SNTZ_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as sanitizer shared library, not stripped"

$(SNTZ_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(SNTZ_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(WFLAGS) $(SNTZ_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with sanitizer flags"

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
	@echo "$(call short_path,$@) prepared as release static library archive"

$(PROD_SO): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(CC) -shared $(STRIP) $(PROD_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as release shared library, stripped"

$(PROD_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PROD_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(WFLAGS) $(PROD_WFLAGS) $(PROD_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with release flags"

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
	@echo "$(call short_path,$@) prepared as portable static library archive"

$(PRTB_SO): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(CC) -shared $(STRIP) $(PRTB_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as portable shared library, stripped"

$(PRTB_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PRTB_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(WFLAGS) $(PRTB_WFLAGS) $(PRTB_CFLAGS) -o $@ $<
	@echo "$(call short_path,$<) compiled with portable flags"

$(PRTB_OBJDIR): | $(PRTB_LIBDIR)
	@mkdir -p "$@"

$(PRTB_LIBDIR):
	@mkdir -p "$@"

purge:
	@test -d $(BUILDDIR) && rm -rf $(BUILDDIR) 2>/dev/null || true
	@test -f $(EXE) && rm $(EXE) 2>/dev/null || true
	@echo $(LIBNAME) artifacts cleared

clean:
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
# Format rules (do nothing)
#
format:
	@true

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
