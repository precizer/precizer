# Name of the library
LIBNAME=libsqlite3

BUILDDIR = ../../.builds

#
# Compiler flags
#
CFLAGS += -pipe -std=c2x -finline-functions
CFLAGS += -fbuiltin
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

STRIP = -s

# Warning flags for additional checks
WFLAGS = -Wall
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

#
# Debug build settings
#
DBG = debug
DBG_DIR = $(BUILDDIR)/$(DBG)
DBG_LIBDIR = $(DBG_DIR)/libs
DBG_OBJDIR = $(DBG_DIR)/obj
DBG_SO = $(DBG_LIBDIR)/$(SO)
DBG_STAT = $(DBG_LIBDIR)/$(STATLIB)
DBG_OBJS = $(addprefix $(DBG_OBJDIR)/, $(OBJS))
DBG_CFLAGS = -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -fno-omit-frame-pointer -DDEBUG
DBG_LDFLAGS = -Wl,-z,defs

#
# Sanitize build settings
#
SNTZ = sanitize
SNTZ_DIR = $(BUILDDIR)/$(SNTZ)
SNTZ_LIBDIR = $(SNTZ_DIR)/libs
SNTZ_OBJDIR = $(SNTZ_DIR)/obj
SNTZ_SO = $(SNTZ_LIBDIR)/$(SO)
SNTZ_STAT = $(SNTZ_LIBDIR)/$(STATLIB)
SNTZ_OBJS = $(addprefix $(SNTZ_OBJDIR)/, $(OBJS))
SNTZ_OPTIONS = -fsanitize=address,undefined -static-libasan
SNTZ_CFLAGS = $(DBG_CFLAGS) $(SNTZ_OPTIONS)
SNTZ_LDFLAGS = $(SNTZ_OPTIONS)

#
# Production build settings
#
PROD = production
PROD_DIR = $(BUILDDIR)/$(PROD)
PROD_LIBDIR = $(PROD_DIR)/libs
PROD_OBJDIR = $(PROD_DIR)/obj
PROD_SO = $(PROD_LIBDIR)/$(SO)
PROD_STAT = $(PROD_LIBDIR)/$(STATLIB)
PROD_OBJS = $(addprefix $(PROD_OBJDIR)/, $(OBJS))
PROD_CFLAGS = -flto=auto -O3 -march=native -funroll-loops -pipe -ffunction-sections -fdata-sections -fomit-frame-pointer -DNDEBUG
PROD_LDFLAGS = -flto=auto -Wl,-O3 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,--gc-sections -Wl,-z,defs

#
# Portable build settings
#
PRTB = portable
PRTB_DIR = $(BUILDDIR)/$(PRTB)
PRTB_LIBDIR = $(PRTB_DIR)/libs
PRTB_OBJDIR = $(PRTB_DIR)/obj
PRTB_SO = $(PRTB_LIBDIR)/$(SO)
PRTB_STAT = $(PRTB_LIBDIR)/$(STATLIB)
PRTB_OBJS = $(addprefix $(PRTB_OBJDIR)/, $(OBJS))
PRTB_CFLAGS = -flto=auto -O2 -mtune=generic -funroll-loops -pipe -ffunction-sections -fdata-sections -fomit-frame-pointer -DNDEBUG
PRTB_LDFLAGS = -flto=auto -Wl,-O2 -Wl,--hash-style=both -Wl,--as-needed -Wl,--gc-sections -Wl,-z,defs

.PHONY: all clean debug production prod portable sanitize

define SQLITE_WARN_ONCE
$(if $(SQLITE_WARNING_PRINTED),,\
$(info SQLite library compilation has started. This process may take a considerable amount of time) \
$(eval SQLITE_WARNING_PRINTED:=1))
endef

# Default build
all: debug production portable

#
# Debug rules
#
memtest: debug
test: debug
debug: $(DBG_SO) $(DBG_STAT)

$(DBG_STAT): $(DBG_OBJS) | $(DBG_LIBDIR)
	@ar rcs $@ $+
	@echo $@ prepared to be static library

$(DBG_SO): $(DBG_OBJS) | $(DBG_LIBDIR)
	@$(CC) -shared $(DBG_LDFLAGS) -o $@ $^
	@echo $@ prepared to be shared library

$(DBG_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(DBG_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(CFLAGS) $(DBG_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(DBG_OBJDIR):
	@mkdir -p "$@"

$(DBG_LIBDIR):
	@mkdir -p "$@"

#
# Sanitize rules
#
sanitize: $(SNTZ_SO) $(SNTZ_STAT)

$(SNTZ_STAT): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@ar rcs $@ $+
	@echo $@ prepared to be static library

$(SNTZ_SO): $(SNTZ_OBJS) | $(SNTZ_LIBDIR)
	@$(CC) -shared $(SNTZ_LDFLAGS) -o $@ $^
	@echo $@ prepared to be shared library

$(SNTZ_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(SNTZ_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(CFLAGS) $(SNTZ_CFLAGS) $(WFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(SNTZ_OBJDIR):
	@mkdir -p "$@"

$(SNTZ_LIBDIR):
	@mkdir -p "$@"

#
# Production rules
#
prod: production
production: $(PROD_SO) $(PROD_STAT)

$(PROD_STAT): $(PROD_OBJS) | $(PROD_LIBDIR)
	@ar rcs $@ $+
	@echo $@ prepared to be the static library

$(PROD_SO): $(PROD_OBJS) | $(PROD_LIBDIR)
	@$(CC) -shared $(STRIP) $(PROD_LDFLAGS) -o $@ $^
	@echo $@ prepared to be the shared library

$(PROD_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PROD_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(CFLAGS) $(WFLAGS) $(PROD_WFLAGS) $(PROD_CFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(PROD_OBJDIR):
	@mkdir -p "$@"

$(PROD_LIBDIR):
	@mkdir -p "$@"

#
# Portable rules
#
portable: $(PRTB_SO) $(PRTB_STAT)

$(PRTB_STAT): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@ar rcs $@ $+
	@echo $@ prepared to be the static library

$(PRTB_SO): $(PRTB_OBJS) | $(PRTB_LIBDIR)
	@$(CC) -shared $(STRIP) $(PRTB_LDFLAGS) -o $@ $^
	@echo $@ prepared to be the shared library

$(PRTB_OBJDIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(PRTB_OBJDIR)
	@$(call SQLITE_WARN_ONCE)
	@$(CC) -c -fPIC $(CFLAGS) $(WFLAGS) $(PRTB_WFLAGS) $(PRTB_CFLAGS) -o $@ $<
	@echo "$(notdir $<) compiled"

$(PRTB_OBJDIR):
	@mkdir -p "$@"

$(PRTB_LIBDIR):
	@mkdir -p "$@"

clean:
	@rm -rf $(PROD_SO) $(PROD_STAT) $(PROD_OBJS)
	@rm -rf $(PRTB_SO) $(PRTB_STAT) $(PRTB_OBJS)
	@rm -rf $(DBG_SO) $(DBG_STAT) $(DBG_OBJS)
	@rm -rf $(SNTZ_SO) $(SNTZ_STAT) $(SNTZ_OBJS)

	@test -d $(DBG_LIBDIR) && rm -d $(DBG_LIBDIR) 2>/dev/null || true
	@test -d $(DBG_OBJDIR) && rm -d $(DBG_OBJDIR) 2>/dev/null || true
	@test -d $(DBG_DIR) && rm -d $(DBG_DIR) 2>/dev/null || true

	@test -d $(SNTZ_LIBDIR) && rm -d $(SNTZ_LIBDIR) 2>/dev/null || true
	@test -d $(SNTZ_OBJDIR) && rm -d $(SNTZ_OBJDIR) 2>/dev/null || true
	@test -d $(SNTZ_DIR) && rm -d $(SNTZ_DIR) 2>/dev/null || true

	@test -d $(PROD_LIBDIR) && rm -d $(PROD_LIBDIR) 2>/dev/null || true
	@test -d $(PROD_OBJDIR) && rm -d $(PROD_OBJDIR) 2>/dev/null || true
	@test -d $(PROD_DIR) && rm -d $(PROD_DIR) 2>/dev/null || true

	@test -d $(PRTB_LIBDIR) && rm -d $(PRTB_LIBDIR) 2>/dev/null || true
	@test -d $(PRTB_OBJDIR) && rm -d $(PRTB_OBJDIR) 2>/dev/null || true
	@test -d $(PRTB_DIR) && rm -d $(PRTB_DIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true
	@echo $(LIBNAME) cleared

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
