# Name of the library
LIBNAME=libargp

# Defaults to a local .builds when running `make` from this directory.
# When the umbrella project drives the build, BUILDDIR is overridden via the
# library dispatcher Makefile to point at the project-wide .builds
BUILDDIR ?= .builds
# Returns a concise path for build messages.
# Plain file names stay unchanged; nested paths keep only final-directory/file
short_path = $(if $(filter ./,$(dir $(1))),$(notdir $(1)),$(notdir $(patsubst %/,%,$(dir $(1))))/$(notdir $(1)))
COMPILE_COMMANDS = $(BUILDDIR)/compile_commands.json

UNAME_S := $(shell uname -s)

#
# Compiler flags
#
# USE_LLD and AR are exported by the root Makefile when COMPILER=clang
# to use LLVM's own linker and archiver, avoiding gold plugin version mismatch
C_STANDARD := -std=gnu2x
CPPFLAGS ?=
CFLAGS ?= -finline-functions
LDFLAGS ?=

# Detect the MSYS runtime target, independently of the active shell
IS_MSYS := $(shell $(CC) $(CPPFLAGS) $(CFLAGS) -dM -E -x c /dev/null 2>/dev/null | awk '$$2 == "__MSYS__" { print 1; exit }')

SO = $(LIBNAME).so
STATLIB = $(LIBNAME).a

# Detect GCC from its version output, including cc and cross-compiler names
GCC := $(findstring gcc version,$(shell $(CC) -v 2>&1))

ifeq ($(UNAME_S),Darwin)
STRIP ?= -Wl,-x
else
STRIP ?= -s
endif

LTO = -flto=auto

# Warning flags for additional checks
WFLAGS += -Wall
#WFLAGS += -Werror # Treat every warning as an error (-Werror stops the build)
#WFLAGS += -Wpedantic
WFLAGS += -Wshadow
#WFLAGS += -Wextra
WFLAGS += -Winit-self -Wunreachable-code -Wformat-y2k
#WFLAGS += -Wformat-nonliteral
WFLAGS += -Wformat-security -Wmissing-include-dirs
WFLAGS += -Wswitch-default -Wtrigraphs
WFLAGS += -Wfloat-equal
#WFLAGS += -Wundef
# Upstream sources intentionally cast away const qualifiers
WFLAGS += -Wbad-function-cast
#WFLAGS += -Wcast-align
WFLAGS += -Wwrite-strings
WFLAGS += -Winline
# Extra warnings enabled only for GCC
ifneq ($(GCC),)
WFLAGS += -Wlogical-op
endif
ifeq ($(COMPILER),clang)
WFLAGS += -Wno-incompatible-pointer-types-discards-qualifiers
endif

#
# Project files
#
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h)

INCPATH += -I$(SRC_DIR)
CPPFLAGS += -DHAVE_CONFIG_H -D_GNU_SOURCE
# Upstream extern inline definitions require GNU inline semantics
CFLAGS += -fgnu89-inline

# Object names corresponding to the source files
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
# Callers may replace these defaults through DBG_OPT_CFLAGS and
# DBG_OPT_LDFLAGS. Debug optimization flags follow CFLAGS and LDFLAGS and
# win conflicts
DBG_OPT_CFLAGS ?= -pipe -fbuiltin -O0 -fno-omit-frame-pointer
DBG_CFLAGS ?= $(DBG_OPT_CFLAGS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -DDEBUG
DBG_OPT_LDFLAGS ?=
ifeq ($(UNAME_S),Darwin)
DBG_LDFLAGS ?= $(USE_LLD) $(DBG_OPT_LDFLAGS) -Wl,-undefined,dynamic_lookup
else ifneq ($(IS_MSYS),)
DBG_LDFLAGS ?= $(USE_LLD) $(DBG_OPT_LDFLAGS)
else
DBG_LDFLAGS ?= $(USE_LLD) $(DBG_OPT_LDFLAGS) -Wl,-z,defs
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
# Vendored sources are excluded from gcov instrumentation
# Callers may replace these defaults through COV_OPT_CFLAGS and
# COV_OPT_LDFLAGS. Coverage optimization flags follow CFLAGS and LDFLAGS and
# win conflicts
COV_OPT_CFLAGS ?= -pipe -fbuiltin -O0 -fno-omit-frame-pointer
COV_CFLAGS ?= $(COV_OPT_CFLAGS) -g -DDEBUG
COV_OPT_LDFLAGS ?=
COV_LDFLAGS ?= $(USE_LLD) $(COV_OPT_LDFLAGS)

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
# Callers may replace these defaults through SNTZ_OPT_CFLAGS and
# SNTZ_OPT_LDFLAGS. Sanitizer optimization flags follow CFLAGS and LDFLAGS,
# while later required sanitizer flags take final priority
SNTZ_OPT_LDFLAGS ?=
ifeq ($(UNAME_S),Darwin)
SNTZ_LDFLAGS ?= -Wl,-undefined,dynamic_lookup -install_name @rpath/$(SO) $(SNTZ_OPT_LDFLAGS) $(USE_LLD) $(SNTZ_OPTIONS)
else
SNTZ_LDFLAGS ?= $(SNTZ_OPT_LDFLAGS) $(USE_LLD) $(SNTZ_OPTIONS)
endif
SNTZ_OPT_CFLAGS ?= $(DBG_OPT_CFLAGS)
SNTZ_CFLAGS ?= $(SNTZ_OPT_CFLAGS) $(SNTZ_OPTIONS) -g -ggdb -ggdb1 -ggdb2 -ggdb3 -DDEBUG

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
# Callers may replace these defaults through PROD_OPT_CFLAGS and
# PROD_OPT_LDFLAGS. Production optimization flags follow CFLAGS and LDFLAGS
# and win conflicts
PROD_OPT_CFLAGS ?= -pipe -fbuiltin $(LTO) -O3 -march=native -funroll-loops -ffunction-sections -fdata-sections -fomit-frame-pointer
PROD_CFLAGS ?= $(PROD_OPT_CFLAGS)
ifeq ($(UNAME_S),Darwin)
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,-dead_strip
PROD_LDFLAGS ?= $(USE_LLD) $(PROD_OPT_LDFLAGS) -Wl,-undefined,dynamic_lookup
else ifneq ($(IS_MSYS),)
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,--gc-sections
PROD_LDFLAGS ?= $(USE_LLD) $(PROD_OPT_LDFLAGS)
else
PROD_OPT_LDFLAGS ?= $(LTO) -Wl,-O3 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,--gc-sections
PROD_LDFLAGS ?= $(USE_LLD) $(PROD_OPT_LDFLAGS) -Wl,-z,defs
endif
#
# Distribution package build settings
#
DIST = distribution
DIST_DIR = $(BUILDDIR)/$(DIST)
DIST_LIBDIR = $(DIST_DIR)/libs
DIST_OBJDIR = $(DIST_LIBDIR)/obj/$(LIBDIR)
DIST_SO = $(DIST_LIBDIR)/$(SO)
DIST_STAT = $(DIST_LIBDIR)/$(STATLIB)
DIST_OBJS = $(addprefix $(DIST_OBJDIR)/, $(OBJS))
# Callers may replace these fallbacks through DIST_OPT_CFLAGS and
# DIST_OPT_LDFLAGS. CFLAGS and LDFLAGS follow distribution optimization flags
# and win conflicts
DIST_OPT_CFLAGS ?= -O2
DIST_CFLAGS ?= $(DIST_OPT_CFLAGS)
DIST_OPT_LDFLAGS ?=
DIST_LDFLAGS ?= $(DIST_OPT_LDFLAGS)

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
# Callers may replace these defaults through PRTB_OPT_CFLAGS and
# PRTB_OPT_LDFLAGS. Portable optimization flags follow CFLAGS and LDFLAGS and
# win conflicts
PRTB_OPT_CFLAGS ?= -pipe -fbuiltin $(LTO) -O2 -mtune=generic -funroll-loops -ffunction-sections -fdata-sections -fomit-frame-pointer
PRTB_CFLAGS ?= $(PRTB_OPT_CFLAGS)
ifeq ($(UNAME_S),Darwin)
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,-dead_strip
PRTB_LDFLAGS ?= $(USE_LLD) $(PRTB_OPT_LDFLAGS) -Wl,-undefined,dynamic_lookup
else ifneq ($(IS_MSYS),)
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,--gc-sections
PRTB_LDFLAGS ?= $(USE_LLD) $(PRTB_OPT_LDFLAGS)
else
PRTB_OPT_LDFLAGS ?= $(LTO) -Wl,-O2 -Wl,--hash-style=both -Wl,--as-needed -Wl,--gc-sections
PRTB_LDFLAGS ?= $(USE_LLD) $(PRTB_OPT_LDFLAGS) -Wl,-z,defs
endif
.PHONY: all purge clean debug production prod distribution portable sanitize format gcc-analyzer
.PHONY: coverage memtest tests test tests-sanitize tests-debug tests-coverage
.PHONY: cppcheck clang-analyzer compile-commands clean-compile-commands update print-%

# Default build
all: debug production portable

# Complete the clone before replacing the bundled sources and licenses
update:
	@set -eu; \
	upstream_dir=$$(mktemp -d); \
	trap 'rm -rf "$$upstream_dir"' 0; \
	trap 'exit 1' HUP INT TERM; \
	git clone --depth 1 --branch main https://github.com/tom42/argp-standalone.git \
		"$$upstream_dir"; \
	mkdir -p "$(SRC_DIR)" LICENSES; \
	cp "$$upstream_dir"/src/*.c "$$upstream_dir"/src/*.h "$(SRC_DIR)/"; \
	cp "$$upstream_dir/include/argp-standalone/argp.h" "$(SRC_DIR)/"; \
	cp "$$upstream_dir/config.h.in" "$$upstream_dir/LICENSE" .; \
	printf 'Updated argp-standalone from main\n'

#
# Configuration rules
#
# Keep feature detection separate for each build mode and rerun it when flags change
CONFIG_DIRS = $(DBG_OBJDIR) $(COV_OBJDIR) $(SNTZ_OBJDIR) $(PROD_OBJDIR) $(DIST_OBJDIR) $(PRTB_OBJDIR)
CONFIG_HEADERS = $(addsuffix /config.h,$(CONFIG_DIRS))
CONFIG_GENERATORS = $(addsuffix /configure,$(CONFIG_DIRS))
# Probe executables must reject unresolved symbols; shared-library linker flags
# such as macOS dynamic_lookup would incorrectly report missing functions as available
export CC CPPFLAGS LDFLAGS USE_LLD C_STANDARD MACOSX_DEPLOYMENT_TARGET

$(CONFIG_GENERATORS): configure.c Makefile
	@mkdir -p "$(@D)"
	@$(CC) -O2 $(C_STANDARD) -Wall -Wextra -Werror -o "$@" "$<"

# The generator checks settings on every invocation. Changed prerequisites
# force fresh probes, including an explicit rebuild requested with make -B
$(CONFIG_HEADERS): %/config.h: config.h.in %/configure check-config
	@"$(@D)/configure" "$<" "$@" $(if $(filter-out check-config,$?),--force)

.PHONY: check-config
check-config:

#
# Debug rules
#
memtest: debug
tests: test
test: tests-sanitize

tests-sanitize tests-debug tests-coverage:
	@true

debug: $(DBG_SO) $(DBG_STAT)

$(DBG_OBJDIR)/config.h: export CONFIG_FLAGS = $(CFLAGS) $(DBG_CFLAGS)

$(DBG_STAT): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$(call short_path,$@) prepared as debug static library archive"

$(DBG_SO): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(CC) $(LDFLAGS) -shared $(DBG_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as debug shared library, not stripped"

$(DBG_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) $(DBG_OBJDIR)/config.h Makefile | $(DBG_OBJDIR)
	@$(CC) -c $(CPPFLAGS) -fPIC $(INCPATH) -I$(@D) $(CFLAGS) $(DBG_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with debug flags"

$(DBG_OBJDIR): | $(DBG_LIBDIR)
	@mkdir -p "$@"

$(DBG_LIBDIR):
	@mkdir -p "$@"

#
# Coverage rules
#
coverage: $(COV_SO) $(COV_STAT)

$(COV_OBJDIR)/config.h: export CONFIG_FLAGS = $(CFLAGS) $(COV_CFLAGS)

$(COV_STAT): $(COV_OBJS) | $(COV_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$(call short_path,$@) prepared as coverage static library archive"

$(COV_SO): $(COV_OBJS) | $(COV_LIBDIR)
	@$(CC) $(LDFLAGS) -shared $(COV_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as coverage shared library, not stripped"

$(COV_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) $(COV_OBJDIR)/config.h Makefile | $(COV_OBJDIR)
	@$(CC) -c $(CPPFLAGS) -fPIC $(INCPATH) -I$(@D) $(CFLAGS) $(COV_CFLAGS) $(WFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with coverage flags"

$(COV_OBJDIR): | $(COV_LIBDIR)
	@mkdir -p "$@"

$(COV_LIBDIR):
	@mkdir -p "$@"

#
# Sanitize rules
#
sanitize: $(SNTZ_SO) $(SNTZ_STAT)

$(SNTZ_OBJDIR)/config.h: export CONFIG_FLAGS = $(CFLAGS) $(SNTZ_CFLAGS)

$(SNTZ_STAT): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$(call short_path,$@) prepared as sanitizer static library archive"

$(SNTZ_SO): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(CC) $(LDFLAGS) -shared $(SNTZ_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as sanitizer shared library, not stripped"

$(SNTZ_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) $(SNTZ_OBJDIR)/config.h Makefile | $(SNTZ_OBJDIR)
	@$(CC) -c $(CPPFLAGS) -fPIC $(INCPATH) -I$(@D) $(WFLAGS) $(CFLAGS) $(SNTZ_CFLAGS) $(C_STANDARD) -o $@ $<
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

$(PROD_OBJDIR)/config.h: export CONFIG_FLAGS = $(CFLAGS) $(PROD_CFLAGS)

$(PROD_STAT): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$(call short_path,$@) prepared as release static library archive"

$(PROD_SO): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(CC) $(LDFLAGS) -shared $(STRIP) $(PROD_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as release shared library, stripped"

$(PROD_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) $(PROD_OBJDIR)/config.h Makefile | $(PROD_OBJDIR)
	@$(CC) -c $(CPPFLAGS) -fPIC $(INCPATH) -I$(@D) $(WFLAGS) $(PROD_WFLAGS) $(CFLAGS) $(PROD_CFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with release flags"

$(PROD_OBJDIR): | $(PROD_LIBDIR)
	@mkdir -p "$@"

$(PROD_LIBDIR):
	@mkdir -p "$@"

#
# Distribution package rules
#
distribution: $(DIST_SO) $(DIST_STAT)

$(DIST_OBJDIR)/config.h: export CONFIG_FLAGS = $(DIST_CFLAGS) $(CFLAGS)

$(DIST_STAT): $(DIST_OBJS) | $(DIST_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$(call short_path,$@) prepared as distribution static library archive"

$(DIST_SO): $(DIST_OBJS) | $(DIST_LIBDIR)
	@$(CC) $(DIST_LDFLAGS) $(LDFLAGS) -shared -o $@ $^
	@echo "$(call short_path,$@) prepared as distribution shared library, not stripped"

$(DIST_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) $(DIST_OBJDIR)/config.h Makefile | $(DIST_OBJDIR)
	@$(CC) -c $(CPPFLAGS) -fPIC $(INCPATH) -I$(@D) $(WFLAGS) $(DIST_CFLAGS) $(CFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with distribution flags"

$(DIST_OBJDIR): | $(DIST_LIBDIR)
	@mkdir -p "$@"

$(DIST_LIBDIR):
	@mkdir -p "$@"

#
# Portable rules
#
portable: $(PRTB_SO) $(PRTB_STAT)

$(PRTB_OBJDIR)/config.h: export CONFIG_FLAGS = $(CFLAGS) $(PRTB_CFLAGS)

$(PRTB_STAT): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(AR) rcs $@ $+
	@echo "$(call short_path,$@) prepared as portable static library archive"

$(PRTB_SO): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(CC) $(LDFLAGS) -shared $(STRIP) $(PRTB_LDFLAGS) -o $@ $^
	@echo "$(call short_path,$@) prepared as portable shared library, stripped"

$(PRTB_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) $(PRTB_OBJDIR)/config.h Makefile | $(PRTB_OBJDIR)
	@$(CC) -c $(CPPFLAGS) -fPIC $(INCPATH) -I$(@D) $(WFLAGS) $(PRTB_WFLAGS) $(CFLAGS) $(PRTB_CFLAGS) $(C_STANDARD) -o $@ $<
	@echo "$(call short_path,$<) compiled with portable flags"

$(PRTB_OBJDIR): | $(PRTB_LIBDIR)
	@mkdir -p "$@"

$(PRTB_LIBDIR):
	@mkdir -p "$@"

purge: clean-compile-commands
	@test -d $(BUILDDIR) && rm -rf $(BUILDDIR) 2>/dev/null || true
	@echo $(LIBNAME) artifacts cleared

clean-compile-commands:
	@rm -f $(COMPILE_COMMANDS)

clean: clean-compile-commands
	@rm -f $(CONFIG_HEADERS) $(CONFIG_GENERATORS) \
		$(foreach d,$(CONFIG_DIRS),$(d)/config.h.tmp $(d)/configure.log $(d)/conftest.c $(d)/conftest.exe)
	@rm -f $(PROD_SO) $(PROD_STAT) $(PROD_OBJS)
	@rm -f $(DIST_SO) $(DIST_STAT) $(DIST_OBJS)
	@rm -f $(PRTB_SO) $(PRTB_STAT) $(PRTB_OBJS)
	@rm -f $(DBG_SO) $(DBG_STAT) $(DBG_OBJS)
	@rm -f $(COV_SO) $(COV_STAT)
	@rm -rf $(COV_OBJDIR)
	@rm -f $(SNTZ_SO) $(SNTZ_STAT) $(SNTZ_OBJS)

	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_LIBDIR)/obj && rm -d $(DBG_LIBDIR)/obj 2>/dev/null || true
	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

	@test -d $(COV_LIBDIR)/obj && rm -d $(COV_LIBDIR)/obj 2>/dev/null || true
	@test -d $(COV_LIBDIR) && rm -d $(COV_LIBDIR) 2>/dev/null || true
	@test -d $(COV_DIR) && rm -d $(COV_DIR) 2>/dev/null || true

	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_LIBDIR)/obj && rm -d $(SNTZ_LIBDIR)/obj 2>/dev/null || true
	@test -d $(SNTZ_LIBDIR) && rm -d $(SNTZ_LIBDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true

	@test -d $(PROD_OBJDIR) && rm -d $(PROD_OBJDIR) 2>/dev/null || true
	@test -d $(PROD_LIBDIR)/obj && rm -d $(PROD_LIBDIR)/obj 2>/dev/null || true
	@test -d $(PROD_LIBDIR) && rm -d $(PROD_LIBDIR) 2>/dev/null || true
	@test -d $(PROD_DIR) && rm -d $(PROD_DIR) 2>/dev/null || true

	@test -d $(DIST_OBJDIR) && rm -d $(DIST_OBJDIR) 2>/dev/null || true
	@test -d $(DIST_LIBDIR)/obj && rm -d $(DIST_LIBDIR)/obj 2>/dev/null || true
	@test -d $(DIST_LIBDIR) && rm -d $(DIST_LIBDIR) 2>/dev/null || true
	@test -d $(DIST_DIR) && rm -d $(DIST_DIR) 2>/dev/null || true

	@test -d $(PRTB_OBJDIR) && rm -d $(PRTB_OBJDIR) 2>/dev/null || true
	@test -d $(PRTB_LIBDIR)/obj && rm -d $(PRTB_LIBDIR)/obj 2>/dev/null || true
	@test -d $(PRTB_LIBDIR) && rm -d $(PRTB_LIBDIR) 2>/dev/null || true
	@test -d $(PRTB_DIR) && rm -d $(PRTB_DIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true
	@echo $(LIBNAME) cleared

#
# Format rules
#
# Preserve upstream formatting; only the local generator is formatted here
format:
	@echo "Formatting source files..."
	@for file in configure.c; do \
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
	bear --output $@ -- $(MAKE) -B -s debug

cppcheck: compile-commands
	cppcheck --check-level=exhaustive --project=$(COMPILE_COMMANDS) --suppress=missingIncludeSystem --enable=all --platform=unix64 --std=c2x -q --force --inconclusive

# Run the Clang static analyzer and open its results in a web browser
clang-analyzer: CC = clang
clang-analyzer:
	scan-build -V make debug

# Optional preprocessor files
$(SRC_DIR)/%.i: $(SRC_DIR)/%.c $(DBG_OBJDIR)/config.h
	@$(CC) -E -C $(CPPFLAGS) $(INCPATH) -I$(DBG_OBJDIR) $(CFLAGS) $(C_STANDARD) -o $@ $<
	@bcpp -na -s -i 4 $@ > $@.h
	@sed -i 's/[ \t]*\# [[:digit:]]\+ \".*//g' $@.h

#
# Print of variables
#
# Display a Makefile variable with make print-VARIABLE
# Output format: VARIABLE = the_value_of_the_variable
#
print-%:
	@echo '$* = $($*)'
