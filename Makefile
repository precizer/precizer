# How to install dependencies and build the app:
#
# GCC
# sudo apt -y install gcc make libpcre2-dev lvm
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
# make analize
#
# Autobated build with GitHub Actions:
#
# * Create an annotated tag:
#   ```sh
#   git tag -a v0.2.0 -m "Release version 0.2.0"
#   ```
# * Push the tag to the remote server for release creation:
#   ```sh
#   git push origin v0.2.0
#   ```
# * Delete a failed local tag:
#   ```sh
#   git tag -d v0.2.0
#   ```

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

STATIC = -static -static-libgcc -Wl,--gc-sections
SRC = src
STRIP = -s
# Flags for additional checks. Must have!
WFLAGS += -Wall -Wextra -Wpedantic -Wshadow
WFLAGS += -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code -Wformat-y2k
WFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs
WFLAGS += -Wswitch-default -Wtrigraphs -Wstrict-overflow=5
WFLAGS += -Wfloat-equal -Wundef
WFLAGS += -Wbad-function-cast -Wcast-qual -Wcast-align
WFLAGS += -Wwrite-strings
WFLAGS += -Winline
# If not clang, then these options are for gcc
ifneq ($(CC), clang)
WFLAGS += -Wlogical-op
WFLAGS += -Wsuggest-attribute=const
WFLAGS += -Wsuggest-attribute=pure
WFLAGS += -Wsuggest-attribute=noreturn
WFLAGS += -Wsuggest-attribute=format
WFLAGS += -Wmissing-format-attribute
endif

# Arguments for the test
ARGS = --update tests/examples/diffs

# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory
CONFIG += ordered

# Test directory
TESTDIR = tests

# Tools directory
TOOLSDIR = tools

LIBS = sqlite sha512 mem rational

# Extra libs for linking
LDFLAGS += $(foreach d,$(LIBS),-l$d) -lpcre2-8

# For old gcc versions
GCC_VERSION := $(shell gcc -dumpversion)
# Checking if the GCC version is less than 10
ifeq ($(shell expr $(GCC_VERSION) \< 10), 1)
LDFLAGS += -pthread
endif

# Additional include headers of external libraries
INCPATH += $(foreach d,$(LIBS),-Ilibs/$d)

#
# Project files
#
SRCS = $(wildcard $(SRC)/*.c)
HDRS = $(wildcard $(SRC)/*.h)
BUILDDIR = .builds
# Exclude a file
OBJS = $(SRCS:.c=.o)
PREPROC = $(SRCS:.c=.i) # Preproc files http://www.viva64.com/en/t/0076/
PREPROC += $(SRCS:.c=.i.h)
# Asm
ASM = $(SRCS:.c=.asm)

#
# Sanitize build settings
#
STZDIR = $(BUILDDIR)/sanitize
STZEXE = $(STZDIR)/$(EXE)
STZOBJDIR = $(STZDIR)/obj
STZOBJS = $(addprefix $(STZOBJDIR)/, $(notdir $(OBJS)))
STZLIBS = $(DBGLIBS)
STZDYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBGLIBDIR),-rpath,\$$ORIGIN/libs,-rpath,\$$ORIGIN/../debug/libs
STZCFLAGS += $(DBGFLAGS)
STZCFLAGS += -fsanitize=address,undefined -static-libasan -fno-omit-frame-pointer

#
# Debug build settings
#
DBGDIR = $(BUILDDIR)/debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJDIR = $(DBGDIR)/obj
DBGOBJS = $(addprefix $(DBGOBJDIR)/, $(notdir $(OBJS)))
DBGLIBDIR = $(DBGDIR)/libs
DBGLIBS = -L$(DBGLIBDIR)
DBGDYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBGLIBDIR),-rpath,\$$ORIGIN/libs
DBGFLAGS = -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG
DBGCFLAGS += $(DBGFLAGS)
DBGLDFLAGS += -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG -Wl,--as-needed
# Activation of the Gprof profiler.
# Works incorrectly with Valgrind.
# It is better to use Callgrind - the call graph format
# is supported by visualization tools like kcachegrind.
#DBGCFLAGS += -pg

#
# Production build settings
#
PRDDIR = $(BUILDDIR)/release
PRDEXE = $(PRDDIR)/$(EXE)
PRDOBJDIR = $(PRDDIR)/obj
PRDOBJS = $(addprefix $(PRDOBJDIR)/, $(notdir $(OBJS)))
PRDLIBDIR = $(PRDDIR)/libs
PRDLIBS = -L$(PRDLIBDIR)
PRDDYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(PRDLIBDIR),-rpath,\$$ORIGIN/libs
PRDCFLAGS = -flto=auto -O3 -march=native -funroll-loops -DNDEBUG
PRDLDFLAGS += -flto=auto -w -O3 -march=native -Wl,--hash-style=gnu -Wl,--as-needed

#
# Portable build settings
#
PRTDIR = $(BUILDDIR)/portable
PRTEXE = $(PRTDIR)/$(EXE)
PRTOBJDIR = $(PRTDIR)/obj
PRTOBJS = $(addprefix $(PRTOBJDIR)/, $(notdir $(OBJS)))
PRTLIBDIR = $(PRTDIR)/libs
PRTLIBS = -L$(PRTLIBDIR)
PRTDYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(PRTLIBDIR),-rpath,\$$ORIGIN/libs
PRTCFLAGS = -flto=auto -O2 -mtune=generic -funroll-loops -DNDEBUG
PRTLDFLAGS += -flto=auto -w -O2 -mtune=generic -Wl,--hash-style=both -Wl,--as-needed 

# https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory
TOPTARGETS := all

.PHONY: all clean debug release remake clang tests sanitize banner run format portable production prod

# Default build
all: production

$(DBGLIBDIR):
	@$(MAKE) -s -C libs debug

$(PRDLIBDIR):
	@$(MAKE) -s -C libs production

$(PRTLIBDIR):
	@$(MAKE) -s -C libs portable

# Clang
clang: CC = clang
clang: all

# Portable rules
#
portable: $(PRTLIBDIR) $(PRTEXE) portfinal banner

portfinal: $(PRTEXE)
	@cp $(PRTEXE) $(EXE)
	@upx --best --lzma -qqq $(EXE)
	@echo "The $(PRTEXE) has been copied to the current directory"

$(PRTEXE): $(PRTOBJS)
	@$(CC) $(CFLAGS) $(WFLAGS) $(STATIC) $(STRIP) $(PRTLIBS) $(PRTDYNLIB) $(PRTLDFLAGS) -o $(PRTEXE) $^ $(LDFLAGS)
	@echo "$@ linked"

$(PRTOBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(PRTOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(WFLAGS) $(PRTCFLAGS) -o $@ $<
	@echo $<" compiled"

$(PRTOBJDIR):
	@mkdir -p $(PRTOBJDIR)

#
# Sanitize rules
#
sanitize: $(STZLIBDIR) $(STZEXE)

run:
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(STZEXE) $(ARGS)

$(STZEXE): $(STZOBJS)
	@$(CC) $(CFLAGS) $(WFLAGS) $(STZCFLAGS) $(STZLIBS) $(STZDYNLIB) -o $(STZEXE) $^ $(LDFLAGS)
	@echo "$@ linked"

$(STZOBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(STZOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(WFLAGS) $(STZCFLAGS) -o $@ $<
	@echo $<" compiled"

$(STZOBJDIR):
	@mkdir -p $(STZOBJDIR)

#
# Debug rules
#
debug: $(DBGLIBDIR) $(DBGEXE) debugfinal

debugfinal: $(DBGEXE)
	@cp $(DBGEXE) $(EXE)
	@echo "The $(DBGEXE) has been copied to the current directory"

$(DBGEXE): $(DBGOBJS)
	@$(CC) $(CFLAGS) $(WFLAGS) $(STATIC) $(DBGCFLAGS) $(DBGLIBS) $(DBGDYNLIB) $(DBGLDFLAGS) -o $(DBGEXE) $^ $(LDFLAGS)
	@echo "$@ linked"

$(DBGOBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(DBGOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(WFLAGS) $(DBGCFLAGS) -o $@ $<
	@echo $<" compiled"

$(DBGOBJDIR):
	@mkdir -p $(DBGOBJDIR)

#
# Production rules
#
release: production
prod: production
production: $(PRDLIBDIR) $(PRDEXE) prodfinal banner

prodfinal: $(PRDEXE)
	@cp $(PRDEXE) $(EXE)
	@upx --best --lzma -qqq $(EXE)
	@echo "The $(PRDEXE) has been copied to the current directory"

$(PRDEXE): $(PRDOBJS)
	@$(CC) $(CFLAGS) $(WFLAGS) $(STATIC) $(STRIP) $(PRDCFLAGS) $(PRDLIBS) $(PRDDYNLIB) $(PRDLDFLAGS) -o $(PRDEXE) $^ $(LDFLAGS)
	@echo "$@ linked"

$(PRDOBJDIR)/%.o: $(SRC)/%.c $(HDRS) | $(PRDOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(WFLAGS) $(PRDCFLAGS) -o $@ $<
	@echo $<" compiled"

$(PRDOBJDIR):
	@mkdir -p $(PRDOBJDIR)

tests: debug
	@$(MAKE) -C $(TESTDIR) tests

tests-sanitize: sanitize
	@$(MAKE) -C $(TESTDIR) tests-sanitize

#
# Build and test within Docker container
#
docker: build-docker run-docker copy-from-docker clean-docker
docker-portable: build-docker-portable run-docker copy-from-docker clean-docker

# Build image and create application container
build-docker:
	@docker build -t $(EXE) .
	@docker create --name $(EXE) $(EXE)

build-docker-portable:
	@docker build --build-arg OS=ubuntu:18.04 --build-arg BUILD=portable -t precizer .
	@docker create --name $(EXE) $(EXE)

# Copying a statically compiled application from a container
# to the current directory on the system
copy-from-docker:
	@docker cp precizer:/$(EXE)/$(EXE) $(EXE)

# Run the application within the built container
run-docker:
	@docker run $(EXE)

# Run it 1000 times
tests-in-docker: build-docker
	for i in {1..1000}; do docker run $(EXE) || break; done

# Clean the built container
clean-docker:
	@docker rm -f $(EXE) > /dev/null 2>&1
	@echo Docker image $(EXE) cleared

clean-all-dockers:
	@docker image prune -f > /dev/null 2>&1
	@docker image prune -af > /dev/null 2>&1
	@docker rm -f $(shell docker ps -aq) > /dev/null 2>&1
	@docker rmi -f $(shell docker images -q) > /dev/null 2>&1
	@echo All docker images cleared

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
	@$(CC) -S -C $(INCPATH) $(CFLAGS) $(WFLAGS) $(PRDCFLAGS) $(PRDLDFLAGS) -o $@ $(LDFLAGS) $<

#
# Other rules
#

remake: clean all

# Static analysers and sanitizers
analyze: sanitize clang-analyzer cachegrind callgrind massif cppcheck memtest gcc-analyzer perf

#
# GCC Static Analysis
#
gcc-analyzer: WFLAGS += -fanalyzer -fno-analyzer-state-purge -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3
# -Wanalyzer-too-complex
gcc-analyzer: CC = gcc
gcc-analyzer: debug

REDUCEDLIBS = $(subst -Ilibs/sqlite,,$(INCPATH))

cppcheck:
	cppcheck --enable=all --platform=unix64 --std=c11 -q --force -i libs -i tests $(REDUCEDLIBS) --suppress=missingIncludeSystem --inconclusive .

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
#	@cloc --exclude-dir=$(STZDIR),$(DBGDIR),$(PRDDIR) $(PRTDIR) ./src
	@cloc ./src

# Character | prevent threading with clean
clean-all: clean-tests clean clean-tools clean-docker
	@$(MAKE) -C libs clean

clean-tools:
	@$(MAKE) -C $(TOOLSDIR) clean

clean: | clean-preproc clean-asm clean-tests
	@rm -rf *.out.* doc \
		$(DBGEXE) $(STZEXE) $(PRTEXE) $(PRDEXE) \
		$(STZOBJS) $(DBGOBJS) $(PRTOBJS) $(PRDOBJS)

	@test -d $(STZOBJDIR) && rm -d $(STZOBJDIR) 2>/dev/null || true
	@test -d $(STZDIR) && rm -d $(STZDIR) 2>/dev/null || true

	@test -d $(DBGOBJDIR) && rm -d $(DBGOBJDIR) 2>/dev/null || true
	@test -d $(DBGDIR) && rm -d $(DBGDIR) 2>/dev/null || true

	@test -d $(PRDOBJDIR) && rm -d $(PRDOBJDIR) 2>/dev/null || true
	@test -d $(PRDDIR) && rm -d $(PRDDIR) 2>/dev/null || true

	@test -d $(PRTOBJDIR) && rm -d $(PRTOBJDIR) 2>/dev/null || true
	@test -d $(PRTDIR) && rm -d $(PRTDIR) 2>/dev/null || true

	@test -d $(BUILDDIR) && rm -d $(BUILDDIR) 2>/dev/null || true

	@test -f $(EXE) && rm $(EXE) || true
	@echo $(EXE) cleared

clean-tests:
	@$(MAKE) -C $(TESTDIR) clean

clean-preproc:
	@rm -rf $(PREPROC)

clean-asm:
	@rm -rf $(ASM)

banner:
	@printf "Now some tests could be running:\n"
	@printf "\033[1mStage 1. Adding:\033[0m\n./$(EXE) --progress --database=database1.db tests/examples/diffs/diff1\n"
	@printf "\033[1mStage 2. Adding:\033[0m\n./$(EXE) --progress --database=database2.db tests/examples/diffs/diff2\n"
	@printf "\033[1mFinal stage. Comparing:\033[0m\n./$(EXE) --compare database1.db database2.db\n"

#
# Print of variables
#
# If you want to find out the value of a makefile variable, just:
#make print-VARIABLE
# and it will return:
#VARIABLE = the_value_of_the_variable
#
print-% : ; @echo $* = $($*)
