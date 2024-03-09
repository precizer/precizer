# Name of library
LIBNAME=librational

#
# Compiler flags
#
CC ?= cc

# Stop the build on any errors -Werror
CFLAGS += -pipe -std=c11 -static -finline-functions
CFLAGS += -fbuiltin

SO = $(LIBNAME).so
STATLIB = $(LIBNAME).a

STRIP = -s

# Additional check flags. Must have!
WFLAGS += -Wall -Wpedantic
WFLAGS += -Wextra -Wshadow
WFLAGS += -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code -Wformat-y2k
WFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs
WFLAGS += -Wswitch-default -Wtrigraphs -Wstrict-overflow=5
WFLAGS += -Wfloat-equal -Wundef -Wshadow
WFLAGS += -Wbad-function-cast -Wcast-qual -Wcast-align
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

#
# Debug build settings
#
DBGDIR = debug
DBGSO = $(DBGDIR)/$(SO)
DBGSTAT = $(DBGDIR)/$(STATLIB)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS += -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG

#
# Release build settings
#
RELDIR = release
RELSO = $(RELDIR)/$(SO)
RELSTAT = $(RELDIR)/$(STATLIB)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -O3 -funroll-loops -DNDEBUG
RELCFLAGS += -march=native
# If not clang, then these options are for gcc
ifneq ($(CC), clang)
RELWFLAGS += -Wsuggest-attribute=const -Wsuggest-attribute=pure -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wmissing-format-attribute
endif

.PHONY: all clean debug release prod production sanitize

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

sanitize: debug
	@true

#
# Debug rules
#
debug: $(DBGSO) $(DBGSTAT)
	@true

$(DBGSTAT): $(DBGOBJS)
	@ar crs $@ $+
	@ranlib $@
	@echo "$@ prepared to be static library."

$(DBGSO): $(DBGOBJS)
	@$(CC) -shared -o $(DBGSO) $^
	@echo "$@ prepared to be shared library."

$(DBGDIR)/%.o: %.c $(HDRS)
	@mkdir -p $(DBGDIR)
	@$(CC) -c -fPIC $(CFLAGS) $(DBGCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

#
# Release rules
#
release: $(RELSO) $(RELSTAT)
	@true

$(RELSTAT): $(RELOBJS)
	@ar crs $@ $+
	@ranlib $@
	@echo "$@ prepared to be static library."

$(RELSO): $(RELOBJS)
	@$(CC) -shared $(STRIP) -o $(RELSO) $^
	@echo "$@ prepared to be shared library."

$(RELDIR)/%.o: %.c $(HDRS)
	@mkdir -p $(RELDIR)
	@$(CC) -c -fPIC $(CFLAGS) $(WFLAGS) $(RELWFLAGS) $(RELCFLAGS) -o $@ $<
	@echo $<" compiled."

clean:
	@rm -rf *.out.* *.so doc $(RELSO) $(RELSTAT) $(RELOBJS) $(DBGSO) $(DBGSTAT) $(DBGOBJS) $(OMPOBJS)
	@test -d $(DBGDIR) && rm -d $(DBGDIR) || true
	@test -d $(RELDIR) && rm -d $(RELDIR) || true
	@echo $(LIBNAME) cleared.
