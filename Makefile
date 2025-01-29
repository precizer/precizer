# How to install dependencies and build the app:
#
# GCC
# sudo apt -y install build-essential clang libpcre2-dev
#
# LLVM for sanitizer
# sudo apt -y install llvm libubsan1
#
# Support XXH3_128bits algorythm
# sudo apt -y install libxxhash-dev
#
# Libraries
# sudo apt -y install libgoogle-perftools-dev
#
# Inatall stat and test tools
# sudo apt-get install cloc valgrind clang-tools cppcheck
#
# make production # or
# make prod # or (same as production)
# make portable # or
# make debug # or
# make # prod by default
#
# Perf tool:
# sudo apt-get install linux-tools-common linux-tools-generic linux-tools-`uname -r`
# make perf # or
# make test
#

# Define our suffix list for quick compilation
.SUFFIXES:          # Delete the default suffixes
.SUFFIXES: .c .o .h # Define our suffix list

#
# Compiler flags
#

CFLAGS += -pipe -std=c11
CFLAGS += -fbuiltin

# To pass #define inside a code:
# make DEFINES=-DWRITE_CSV=false memtest
CFLAGS += $(DEFINES)

SYS := $(shell gcc -dumpmachine)
ifneq (, $(findstring alpine, $(SYS)))
# Alpine Linux uses external libraries
LDFLAGS += -largp -lfts
endif

EXE = precizer

STATIC = -static -static-libgcc
SRC = src
STRIP = -s
# Flags for additional checks. Must have!
WFLAGS += -Wall -Wextra -Wpedantic -Wshadow
WFLAGS += -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code -Wformat-y2k
WFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs
WFLAGS += -Wswitch-default -Wtrigraphs -Wstrict-overflow=5
WFLAGS += -Wfloat-equal -Wundef -Wshadow
WFLAGS += -Wbad-function-cast -Wcast-qual -Wcast-align
WFLAGS += -Wsuggest-attribute=const -Wsuggest-attribute=pure -Wsuggest-attribute=noreturn
WFLAGS += -Wsuggest-attribute=format -Wmissing-format-attribute
WFLAGS += -Wwrite-strings
WFLAGS += -Winline
# If it is not clang, then these options are for gcc
ifneq ($(CC), clang)
WFLAGS += -Wlogical-op
endif

# Arguments for tests
ARGS = --update tests/examples/diffs

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered

# Build of dependent static library
SUBDIRS = libs

# Test directory
TESTDIR = tests

LIBS = sqlite sha512 mem rational

# libc lib for static
LDFLAGS += $(foreach d,$(LIBS),-l$d) -lpcre2-8

# Additional include headers of external libraries
INCPATH += $(foreach d,$(LIBS),-Ilibs/$d)

#
# Project files
#
SRCS = $(wildcard $(SRC)/*.c)
HDRS = $(wildcard $(SRC)/*.h)
BUILDS = .builds
# Exclude a file
OBJS = $(SRCS:.c=.o)
PREPROC = $(SRCS:.c=.i) # Preproc files http://www.viva64.com/en/t/0076/
PREPROC += $(SRCS:.c=.i.h)
# Asm
ASM = $(SRCS:.c=.asm)

#
# Sanitize build settings
#
STZDIR = $(BUILDS)/sanitize
STZEXE = $(STZDIR)/$(EXE)
STZOBJDIR = $(STZDIR)/obj
STZOBJS = $(addprefix $(STZOBJDIR)/, $(notdir $(OBJS)))
STZLIBDIR = $(DBGLIBDIR)
STZLIBS = $(DBGLIBS)
STZDYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBGLIBDIR),-rpath,\$$ORIGIN/libs,-rpath,\$$ORIGIN/../debug/libs
STZCFLAGS += $(DBGFLAGS)
STZCFLAGS += -fsanitize=address,undefined -static-libasan -fno-omit-frame-pointer

#
# Debug build settings
#
DBGDIR = $(BUILDS)/debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJDIR = $(DBGDIR)/obj
DBGOBJS = $(addprefix $(DBGOBJDIR)/, $(notdir $(OBJS)))
DBGLIBDIR = $(DBGDIR)/libs
DBGLIBS = -L$(DBGLIBDIR)
DBGDYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBGLIBDIR),-rpath,\$$ORIGIN/libs
DBGFLAGS = -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG
DBGCFLAGS += $(DBGFLAGS)
DBGCFLAGS += -Wl,--as-needed
# Activation of the Gprof profiler.
# Works incorrectly with Valgrind.
# It is better to use Callgrind - the call graph format
# is supported by visualization tools like kcachegrind.
#DBGCFLAGS += -pg

#
# Release build settings
#
RELDIR = $(BUILDS)/release
RELEXE = $(RELDIR)/$(EXE)
RELFINAL = $(CURDIR)/$(EXE)
RELOBJDIR = $(RELDIR)/obj
RELOBJS = $(addprefix $(RELOBJDIR)/, $(notdir $(OBJS)))
RELLIBDIR = $(RELDIR)/libs
RELLIBS = -L$(RELLIBDIR)
RELDYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(RELLIBDIR),-rpath,\$$ORIGIN/libs
RELCFLAGS = -funroll-loops -DNDEBUG
RELLDFLAGS += -Wl,--as-needed

# If static build, then add flags
ifdef STATIC
RELLDFLAGS += -lc
endif
# If it is not clang, then these options are for gcc
ifneq ($(CC), clang)
RELCFLAGS += -flto
endif

# https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory
TOPTARGETS := all

.PHONY: all clean debug prep release remake clang openmp one test sanitize banner run format portable production prod $(SUBDIRS)

# Default build
all: portable

$(SUBDIRS):
	@$(MAKE) -s -C $(SUBDIRS) all

# Clang
clang: CC = clang
clang: all

# Portable release
portable: RELCFLAGS += -O2 -mtune=generic
portable: RELLDFLAGS += -O2 -mtune=generic -Wl,--hash-style=both
portable: release

prod: production
production: RELCFLAGS += -O3 -march=native
production: RELLDFLAGS += -O3 -march=native -Wl,--hash-style=gnu
production: $(SUBDIRS) release

#
# Sanitize rules
#
sanitize: $(SUBDIRS) $(STZEXE)

run:
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(STZEXE) $(ARGS)

$(STZEXE): $(STZOBJS)
	@$(CC) $(CFLAGS) $(STZCFLAGS) $(STZLIBS) $(STZDYNLIB) $(WFLAGS) -o $(STZEXE) $^ $(LDFLAGS)
	@echo "$@ linked"

$(STZOBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(STZOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(STZCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(STZOBJDIR):
	@mkdir -p $(STZOBJDIR)

#
# Debug rules
#
debug: $(SUBDIRS) $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	@$(CC) $(CFLAGS) $(DBGCFLAGS) $(STATIC) $(DBGLIBS) $(DBGDYNLIB) $(WFLAGS) -o $(DBGEXE) $^ $(LDFLAGS)
	@echo "$@ linked"

$(DBGOBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(DBGOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(DBGCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled"

$(DBGOBJDIR):
	@mkdir -p $(DBGOBJDIR)

#
# Release rules
#
release: $(SUBDIRS) $(RELEXE) $(RELFINAL) banner

# Linking problem with "undefined reference to 'dlopen' "
# https://stackoverflow.com/a/11221504/7104681
# take in account that this doesn't work:
# gcc -ldl dlopentest.c
# But this does:
# gcc dlopentest.c -ldl
#
$(RELFINAL): $(RELEXE)
	@cp $(RELEXE) ./
	@echo "The $(RELEXE) has been copied to the current directory"

$(RELEXE): $(RELOBJS)
	@$(CC) $(CFLAGS) $(WFLAGS) $(STATIC) $(STRIP) $(RELLIBS) $(RELDYNLIB) $(RELLDFLAGS) -o $(RELEXE) $^ $(LDFLAGS)
	@echo "$@ linked"

$(RELOBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(RELOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(WFLAGS) $(RELWFLAGS) $(RELCFLAGS) -o $@ $<
	@echo $<" compiled"

$(RELOBJDIR):
	@mkdir -p $(RELOBJDIR)

#
# Build and test within Docker images
#
docker:
	@docker build -t precizer .
	@docker run precizer

clean-docker:
	@docker image prune -f
	@docker image prune -af
	@docker rmi -f $(shell docker images -q)

#
# Format rules
#
format:
	@echo "Formatting source files..."
	@for file in $(SRCS) $(HDRS); do \
		echo "Formatting $$file"; \
		uncrustify -c Uncrustify.cfg --replace --no-backup $$file; \
	done
	@echo "All files formatted."

# Optional preprocessor files
%.i:%.c clean-preproc
	@$(CC) -E -C -o $@ $(INCPATH) $(CFLAGS) $<
# C-C++ Beautifier
#	@bcpp -na $@ > $@.h
	@bcpp -na -s -i 4 $@ > $@.h
	@sed -i 's/[ \t]*\# [[:digit:]]\+ \".*//g' $@.h
#	@sed -i '/^ *$//d' $@.h

# Optional Assembler files
%.asm:%.c clean-asm
	@$(CC) -S -C $(INCPATH) $(CFLAGS) $(WFLAGS) $(RELWFLAGS) $(RELCFLAGS) $(RELLDFLAGS) -o $@ $(LDFLAGS) $<

#
# Other rules
#

remake: clean all

# Tests
test: sanitize clang-analyzer cachegrind callgrind massif cppcheck memtest gcc-analyzer perf

#
# GCC Static Analysis
#
gcc-analyzer: WFLAGS += -fanalyzer -fno-analyzer-state-purge -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3 -flto
# -Wanalyzer-too-complex
gcc-analyzer: CC = gcc
gcc-analyzer: debug

cppcheck:
	cppcheck --enable=all --platform=unix64 --std=c11 -q --force -i libs -i tests --inconclusive .

memtest: debug
	valgrind -v --tool=memcheck --leak-check=full --leak-resolution=high --undef-value-errors=no --show-reachable=yes --num-callers=20 $(DBGDIR)/$(EXE) $(ARGS)

cachegrind: debug
	valgrind --tool=cachegrind --branch-sim=yes $(DBGDIR)/$(EXE) $(ARGS)

callgrind: debug
	valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes $(DBGDIR)/$(EXE) $(ARGS)

helgrind: debug
	valgrind --tool=helgrind --read-var-info=yes --track-origins=yes --num-callers=20 $(DBGDIR)/$(EXE) $(ARGS)

massif: debug
	valgrind --tool=massif --stacks=yes --num-callers=20 $(DBGDIR)/$(EXE) $(ARGS)
	ms_print ./massif.out.*

SPARSE=sparse
SPARSE_FLAGS=-Wsparse-all -nostdinc
sparse-analyzer:
	$(foreach src,$(SRCS),$(SPARSE) $(SPARSE_FLAGS) $(INCPATH) $(CFLAGS) $(DBGCFLAGS) $(DBGLIBS) $(WFLAGS) $(src);)

clang-analyzer: CC = clang
clang-analyzer:
	# Run clang static analyzer and view analysis results in a web browser when the build command completes
	scan-build -V make debug

splint:
	splint -I /usr/include/x86_64-linux-gnu +posixlib $(SRCS) $(INCPATH)

doc:
	@doxygen Doxyfile

spellcheck:
	@~/.cargo/bin/typos libs/sha512/ libs/rational/ libs/mem/ src/ README.md README.ru.md TODO

gource:
	gource --seconds-per-day 0.1 --auto-skip-seconds 1

#https://eax.me/c-cpp-profiling/
#https://perf.wiki.kernel.org/index.php/Main_Page
perf:
	sudo perf stat $(DBGDIR)/$(EXE) $(ARGS)

# Statistic code info and count of lines
stat: cloc
cloc:
#	@cloc --exclude-dir=$(STZDIR),$(DBGDIR),$(RELDIR) ./src
	@cloc ./src

# Character | prevent threading with clean
clean-all: clean-tests clean clean-docker
	@$(MAKE) -C $(SUBDIRS) clean

clean: | clean-preproc clean-asm
	@rm -rf *.out.* doc \
		$(DBGEXE) $(STZEXE) $(RELEXE) \
		$(STZOBJS) $(DBGOBJS) $(RELOBJS)
	@test -d $(STZOBJDIR) && rm -d $(STZOBJDIR) 2>/dev/null || true
	@test -d $(STZDIR) && rm -d $(STZDIR) 2>/dev/null || true
	@test -d $(DBGOBJDIR) && rm -d $(DBGOBJDIR) 2>/dev/null || true
	@test -d $(DBGDIR) && rm -d $(DBGDIR) 2>/dev/null || true
	@test -d $(RELOBJDIR) && rm -d $(RELOBJDIR) 2>/dev/null || true
	@test -d $(RELDIR) && rm -d $(RELDIR) 2>/dev/null || true
	@test -f $(EXE) && rm $(EXE) || true
	@echo $(EXE) cleared.

clean-tests:
	@$(MAKE) -C $(TESTDIR) clean

clean-preproc:
	@rm -rf $(PREPROC)

clean-asm:
	@rm -rf $(ASM)

hugetestfile:
	@echo Creating a guge file for testing
	@mkdir -p tests/examples/huge/
	@dd if=/dev/urandom of=tests/examples/huge/hugetestfile bs=1M count=10
	@echo The file has been created

banner:
	@printf "Now some tests could be running:\n"
	@printf "\033[1mStage 1. Adding:\033[0m\n./precizer --progress --database=database1.db tests/examples/diffs/diff1\n"
	@printf "\033[1mStage 2. Adding:\033[0m\n./precizer --progress --database=database2.db tests/examples/diffs/diff2\n"
	@printf "\033[1mFinal stage. Comparing:\033[0m\n./precizer --compare database1.db database2.db\n"

#
# Print of variables
#
# If you want to find out the value of a makefile variable, just:
#make print-VARIABLE
# and it will return:
#VARIABLE = the_value_of_the_variable
#
print-% : ; @echo $* = $($*)
