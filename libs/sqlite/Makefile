# Name of the library
LIBNAME=libsqlite

BUILDDIR = ../../.builds

#
# Compiler flags
#

# Stop the build on any errors -Werror
CFLAGS += -pipe -std=c11 -finline-functions
CFLAGS += -fbuiltin
CFLAGS += -w

# Disable threads
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

# Additional check flags. Must have!
WFLAGS += -Wall
WFLAGS += -Wpedantic
WFLAGS += -Wshadow
WFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs
# If not clang, then these options are for gcc

#
# Project files
#
SRCS = $(wildcard *.c)
HDRS = $(wildcard *.h)
# Exclude a file
OBJS = $(SRCS:.c=.o)

#
# Debug build settings
#
DBGDIR = $(BUILDDIR)/debug
DBGLIBDIR = $(DBGDIR)/libs
DBGSO = $(DBGLIBDIR)/$(SO)
DBGSTAT = $(DBGLIBDIR)/$(STATLIB)
DBGOBJS = $(addprefix $(DBGLIBDIR)/, $(OBJS))
DBGFLAGS = -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG
DBGCFLAGS += $(DBGFLAGS)

#
# Sanitize build settings
#
STZDIR = $(BUILDDIR)/sanitize
STZLIBDIR = $(STZDIR)/libs
STZSO = $(STZLIBDIR)/$(SO)
STZSTAT = $(STZLIBDIR)/$(STATLIB)
STZOBJS = $(addprefix $(STZLIBDIR)/, $(OBJS))
STZCFLAGS += $(DBGFLAGS)
STZCFLAGS += -fsanitize=address,undefined -static-libasan -fno-omit-frame-pointer

#
# Release build settings
#
RELDIR = $(BUILDDIR)/release
RELLIBDIR = $(RELDIR)/libs
RELSO = $(RELLIBDIR)/$(SO)
RELSTAT = $(RELLIBDIR)/$(STATLIB)
RELOBJS = $(addprefix $(RELLIBDIR)/, $(OBJS))
RELCFLAGS += -flto=auto -O3 -march=native -funroll-loops -DNDEBUG
RELLDFLAGS += -flto=auto -O3 -march=native -Wl,--hash-style=gnu -Wl,--as-needed

#
# Portable build settings
#
PRTDIR = $(BUILDDIR)/portable
PRTLIBDIR = $(PRTDIR)/libs
PRTSO = $(PRTLIBDIR)/$(SO)
PRTSTAT = $(PRTLIBDIR)/$(STATLIB)
PRTOBJS = $(addprefix $(PRTLIBDIR)/, $(OBJS))
PRTCFLAGS += -flto=auto -O2 -mtune=generic -funroll-loops -DNDEBUG
PRTLDFLAGS += -flto=auto -O2 -mtune=generic -Wl,--hash-style=gnu -Wl,--as-needed

.PHONY: all clean debug release prod production portable sanitize warning

# Default build
all: debug release portable
	@true

warning:
	@echo SQLite library compilation has started. This process may take a considerable amount of time.

prod: release
	@true

production: prod
	@true

test: debug
	@true

memtest: debug
	@true

#
# Debug rules
#
debug: warning $(DBGSO) $(DBGSTAT)

$(DBGSTAT): $(DBGOBJS)
	@ar rcs $@ $+
	@echo "$@ prepared to be static library."

$(DBGSO): $(DBGOBJS)
	@$(CC) -shared -o $(DBGSO) $^
	@echo "$@ prepared to be shared library."

$(DBGLIBDIR)/%.o: %.c $(HDRS) | $(DBGLIBDIR)
	@$(CC) -c -fPIC $(CFLAGS) $(DBGCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(DBGLIBDIR):
	@mkdir -p "$(DBGLIBDIR)"

#
# Sanitize rules
#
sanitize: warning $(STZSO) $(STZSTAT)

$(STZSTAT): $(STZOBJS)
	@ar rcs $@ $+
	@echo "$@ prepared to be static library."

$(STZSO): $(STZOBJS)
	@$(CC) -shared -o $(STZSO) $^
	@echo "$@ prepared to be shared library."

$(STZLIBDIR)/%.o: %.c $(HDRS) | $(STZLIBDIR)
	@$(CC) -c -fPIC $(CFLAGS) $(STZCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(STZLIBDIR):
	@mkdir -p "$(STZLIBDIR)"

#
# Release rules
#
release: warning $(RELSO) $(RELSTAT)

$(RELSTAT): $(RELOBJS)
	@ar rcs $@ $+
	@echo "$@ prepared to be the static library."

$(RELSO): $(RELOBJS)
	@$(CC) -shared $(STRIP) $(RELLDFLAGS) -o $(RELSO) $^
	@echo "$@ prepared to be the shared library."

$(RELLIBDIR)/%.o: %.c $(HDRS) | $(RELLIBDIR)
	@$(CC) -c -fPIC $(CFLAGS) $(WFLAGS) $(RELWFLAGS) $(RELCFLAGS) -o $@ $<
	@echo $<" compiled."

$(RELLIBDIR):
	@mkdir -p "$(RELLIBDIR)"

#
# Portable rules
#
portable: warning $(PRTSO) $(PRTSTAT)

$(PRTSTAT): $(PRTOBJS)
	@ar rcs $@ $+
	@echo "$@ prepared to be the static library."

$(PRTSO): $(PRTOBJS)
	@$(CC) -shared $(STRIP) $(PRTLDFLAGS) -o $(PRTSO) $^
	@echo "$@ prepared to be the shared library."

$(PRTLIBDIR)/%.o: %.c $(HDRS) | $(PRTLIBDIR)
	@$(CC) -c -fPIC $(CFLAGS) $(WFLAGS) $(PRTWFLAGS) $(PRTCFLAGS) -o $@ $<
	@echo $<" compiled."

$(PRTLIBDIR):
	@mkdir -p "$(PRTLIBDIR)"

clean:
	@rm -rf $(RELSO) $(RELSTAT) $(RELOBJS)
	@rm -rf $(PRTSO) $(PRTSTAT) $(PRTOBJS)
	@rm -rf $(DBGSO) $(DBGSTAT) $(DBGOBJS)
	@rm -rf $(STZSO) $(STZSTAT) $(STZOBJS)

	@test -d $(DBGLIBDIR) && rm -d $(DBGLIBDIR) 2>/dev/null || true
	@test -d $(DBGDIR) && rm -d $(DBGDIR) 2>/dev/null || true

	@test -d $(STZLIBDIR) && rm -d $(STZLIBDIR) 2>/dev/null || true
	@test -d $(STZDIR) && rm -d $(STZDIR) 2>/dev/null || true

	@test -d $(RELLIBDIR) && rm -d $(RELLIBDIR) 2>/dev/null || true
	@test -d $(RELDIR) && rm -d $(RELDIR) 2>/dev/null || true

	@test -d $(PRTLIBDIR) && rm -d $(PRTLIBDIR) 2>/dev/null || true
	@test -d $(PRTDIR) && rm -d $(PRTDIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true
	@echo $(LIBNAME) cleared
