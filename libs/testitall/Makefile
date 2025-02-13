#
# “The Winner Test It All”
#                  © ABBA
#

# Name of the library
LIBNAME=libtestitall

BUILDDIR = ../../.builds

#
# Compiler flags
#

# Stop the build on any errors -Werror
CFLAGS += -pipe -std=c11 -static -finline-functions
CFLAGS += -fbuiltin

SO = $(LIBNAME).so
STATLIB = $(LIBNAME).a

STRIP = -s

# Additional check flags. Must have!
WFLAGS += -Wall
WFLAGS += -Werror
WFLAGS += -Wpedantic
WFLAGS += -Wshadow
WFLAGS += -Wextra
WFLAGS += -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code -Wformat-y2k
WFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs
WFLAGS += -Wswitch-default -Wtrigraphs -Wstrict-overflow=5
WFLAGS += -Wfloat-equal -Wundef
WFLAGS += -Wbad-function-cast -Wcast-qual -Wcast-align
WFLAGS += -Wsuggest-attribute=const -Wsuggest-attribute=pure -Wsuggest-attribute=noreturn
WFLAGS += -Wsuggest-attribute=format -Wmissing-format-attribute
WFLAGS += -Wwrite-strings
WFLAGS += -Winline
# If not clang, then these options are for gcc
ifneq ($(CC), clang)
WFLAGS += -Wlogical-op
endif

#
# Project files
#
SRCS = $(wildcard *.c)
HDRS = $(wildcard *.h)
# Exclude a file
OBJS = $(SRCS:.c=.o)

LIBS = rational mem xdiff

# Additional include headers of external libraries
INCPATH += $(foreach d,$(LIBS),-I../$d)

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
RELCFLAGS = -O3 -funroll-loops -DNDEBUG
RELCFLAGS += -march=native
# If not clang, then these options are for gcc
ifneq ($(CC), clang)
RELWFLAGS += -Wsuggest-attribute=const -Wsuggest-attribute=pure -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wmissing-format-attribute
endif

.PHONY: all clean debug release prod production sanitize format gcc-analyzer

# Default build
all: debug release
	@true

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
debug: $(DBGSO) $(DBGSTAT)

$(DBGSTAT): $(DBGOBJS)
	@ar rcs $@ $+
	@echo "$@ prepared to be static library."

$(DBGSO): $(DBGOBJS)
	@$(CC) -shared $(INCPATH) -o $(DBGSO) $^
	@echo "$@ prepared to be shared library."

$(DBGLIBDIR)/%.o: %.c $(HDRS) | $(DBGLIBDIR)
	@$(CC) -c -fPIC $(INCPATH) $(CFLAGS) $(DBGCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(DBGLIBDIR):
	@mkdir -p "$(DBGLIBDIR)"

#
# Sanitize rules
#
sanitize: $(STZSO) $(STZSTAT)

$(STZSTAT): $(STZOBJS)
	@ar rcs $@ $+
	@echo "$@ prepared to be static library."

$(STZSO): $(STZOBJS)
	@$(CC) -shared $(INCPATH) -o $(STZSO) $^
	@echo "$@ prepared to be shared library."

$(STZLIBDIR)/%.o: %.c $(HDRS) | $(STZLIBDIR)
	@$(CC) -c -fPIC $(INCPATH) $(CFLAGS) $(STZCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(STZLIBDIR):
	@mkdir -p "$(STZLIBDIR)"

#
# Release rules
#
release: $(RELSO) $(RELSTAT)

$(RELSTAT): $(RELOBJS)
	@ar rcs $@ $+
	@echo "$@ prepared to be the static library."

$(RELSO): $(RELOBJS)
	@$(CC) -shared $(STRIP) $(INCPATH) -o $(RELSO) $^
	@echo "$@ prepared to be the shared library."

$(RELLIBDIR)/%.o: %.c $(HDRS) | $(RELLIBDIR)
	@$(CC) -c -fPIC $(INCPATH) $(CFLAGS) $(WFLAGS) $(RELWFLAGS) $(RELCFLAGS) -o $@ $<
	@echo $<" compiled."

$(RELLIBDIR):
	@mkdir -p "$(RELLIBDIR)"

clean:
	@rm -rf $(RELSO) $(RELSTAT) $(RELOBJS)
	@rm -rf $(DBGSO) $(DBGSTAT) $(DBGOBJS)
	@rm -rf $(STZSO) $(STZSTAT) $(STZOBJS)

	@test -d $(DBGLIBDIR) && rm -d $(DBGLIBDIR) 2>/dev/null || true
	@test -d $(DBGDIR) && rm -d $(DBGDIR) 2>/dev/null || true

	@test -d $(STZLIBDIR) && rm -d $(STZLIBDIR) 2>/dev/null || true
	@test -d $(STZDIR) && rm -d $(STZDIR) 2>/dev/null || true

	@test -d $(RELLIBDIR) && rm -d $(RELLIBDIR) 2>/dev/null || true
	@test -d $(RELDIR) && rm -d $(RELDIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true
	@echo $(LIBNAME) cleared.

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
gcc-analyzer: WFLAGS += -fanalyzer -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3
# -Wanalyzer-too-complex
gcc-analyzer: CC = gcc
gcc-analyzer: debug

cppcheck:
	cppcheck --enable=all --platform=unix64 --std=c11 -q --force -i ../mem -i tests --inconclusive .

clang-analyzer:
	# Run clang static analyzer and view analysis results in a web browser when the build command completes
	scan-build -V make debug
