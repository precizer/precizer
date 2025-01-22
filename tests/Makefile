# Name of the application
EXE=testitall

#
# Compiler flags
#

# Stop the build on any errors -Werror
CFLAGS += -pipe -std=c11 -finline-functions
CFLAGS += -fbuiltin
# The definition used to disable the main() function
# of the tested application
CFLAGS += -DTESTITALL

STRIP = -s
STATIC = -static
SRC = src
PRZRSRC = ../src
BUILDS = ../.builds
BUILEXE = $(BUILDS)/$(EXE)
LIBS = testitall mem sha512 rational sqlite xdiff

# libc lib for static
LDFLAGS += $(foreach d,$(LIBS),-l$d) -lpcre2-8

# Additional include headers of external libraries
INCPATH += $(foreach d,$(LIBS),-I../libs/$d)
## Source files of precizer project
INCPATH += -I$(PRZRSRC)

# Additional check flags. Must have!
WFLAGS += -Wall -Wpedantic
WFLAGS += -Wextra -Wshadow
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
SRCS = $(wildcard $(SRC)/*.c)
TMPLTS = $(wildcard $(SRC)/*.cc)
HDRS = $(wildcard $(SRC)/*.h)
PRZRHDRS = $(wildcard $(PRZRSRC)/*.h)
ALLHDRS = $(HDRS) $(PRZRHDRS)
OBJS = $(SRCS:.c=.o)

PRZRSRCS = $(wildcard $(PRZRSRC)/*.c)
PRZROBJS = $(PRZRSRCS:.c=.o)

#
# Coverage build settings
#
COV = coverage
COVDIR = $(BUILEXE)/$(COV)
COVEXE = $(COVDIR)/$(EXE)
COVOBJDIR = $(COVDIR)/obj
COVREPORTDIR = $(COVOBJDIR)/../report/
COVOBJS = $(addprefix $(COVOBJDIR)/, $(notdir $(OBJS)))
COVPRZROBJS = $(addprefix $(COVOBJDIR)/, $(notdir $(PRZROBJS)))
COVCFLAGS += -g -O0 -fprofile-arcs -ftest-coverage
COVLDFLAGS += -lgcov --coverage

#
# Sanitize build settings
#
STZ = sanitize
STZDIR = $(BUILEXE)/$(STZ)
STZEXE = $(STZDIR)/$(EXE)
STZMAIN = $(EXE)
STZOBJDIR = $(STZDIR)/obj
STZOBJS = $(addprefix $(STZOBJDIR)/, $(notdir $(OBJS)))
STZPRZROBJS = $(addprefix $(STZOBJDIR)/, $(notdir $(PRZROBJS)))
STZCFLAGS += $(DBGCFLAGS)
STZCFLAGS += -fsanitize=address,undefined -static-libasan -fno-omit-frame-pointer

#
# Debug build settings
#
DBG = debug
DBGDIR = $(BUILEXE)/$(DBG)
DBGEXE = $(DBGDIR)/$(EXE)
DBGMAIN = $(EXE)
DBGOBJDIR = $(DBGDIR)/obj
DBGOBJS = $(addprefix $(DBGOBJDIR)/, $(notdir $(OBJS)))
DBGPRZROBJS = $(addprefix $(DBGOBJDIR)/, $(notdir $(PRZROBJS)))
DBGLIBDIR = $(BUILEXE:$(EXE)=)$(DBG)/libs
DBGLIBS = -L$(DBGLIBDIR)
DBGDYNLIB = -Wl,-rpath,\$$ORIGIN,-rpath,\$$ORIGIN/$(DBGLIBDIR),-rpath,\$$ORIGIN/../../$(DBGLIBDIR)
DBGCFLAGS += -g -ggdb -ggdb1 -ggdb2 -ggdb3 -O0 -DDEBUG

.PHONY: all clean debug release prod production sanitize coverage run coverage-report coverage-html format

# Default build
all: sanitize
	@true

#
# Coverage rules
#
coverage: $(COVEXE)

$(COVEXE): $(COVOBJS) $(COVPRZROBJS)
	@$(CC) $(CFLAGS) $(COVCFLAGS) $(DBGLIBS) $(DBGDYNLIB) $(WFLAGS) -o $(COVEXE) $^ $(LDFLAGS) $(COVLDFLAGS)
	@echo "$@ linked."

$(COVOBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(COVOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(COVCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(COVOBJDIR)/%.o: $(PRZRSRC)/%.c $(ALLHDRS) $(TMPLTS) | $(COVOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(COVCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(COVOBJDIR):
	@mkdir -p "$(COVOBJDIR)"

coverage-report: coverage
	@echo "Generating coverage report..."
	./$(COVEXE)
	@echo "Coverage report generated. Check .gcov files"

coverage-html: coverage-report
	@echo "Generating HTML coverage report..."
	@lcov --capture --directory $(COVOBJDIR) --output-file $(COVOBJDIR)/coverage.info
	@genhtml $(COVOBJDIR)/coverage.info --output-directory $(COVREPORTDIR)
	@echo "HTML coverage report generated in $(COVREPORTDIR) directory"

#
# Sanitize rules
#
sanitize: $(STZEXE)
	@$(MAKE) -s -C ../ $@

run: $(STZEXE)
	ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) $(STZEXE)

$(STZEXE): $(STZOBJS) $(STZPRZROBJS)
	@$(CC) $(CFLAGS) $(STZCFLAGS) $(DBGLIBS) $(DBGDYNLIB) $(WFLAGS) -o $(STZEXE) $^ $(LDFLAGS)
	@echo "$@ linked."

$(STZOBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(STZOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(STZCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(STZOBJDIR)/%.o: $(PRZRSRC)/%.c $(ALLHDRS) $(TMPLTS) | $(STZOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(STZCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(STZOBJDIR):
	@mkdir -p "$(STZOBJDIR)"

#
# Debug rules
#
debug: $(DBGEXE) $(DBGMAIN)

$(DBGMAIN): $(DBGEXE)
	@cp $(DBGEXE) $(DBGMAIN)
	@echo "$(DBGMAIN) copied"

$(DBGEXE): $(DBGOBJS) $(DBGPRZROBJS)
	@$(CC) $(CFLAGS) $(DBGCFLAGS) $(DBGLIBS) $(DBGDYNLIB) $(WFLAGS) $(STATIC) -o $(DBGEXE) $^ $(LDFLAGS)
	@echo "$@ linked."

$(DBGOBJDIR)/%.o: $(SRC)/%.c $(ALLHDRS) $(TMPLTS) | $(DBGOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(DBGCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(DBGOBJDIR)/%.o: $(PRZRSRC)/%.c $(ALLHDRS) $(TMPLTS) | $(DBGOBJDIR)
	@$(CC) -c $(INCPATH) $(CFLAGS) $(DBGCFLAGS) $(WFLAGS) -o $@ $<
	@echo $<" compiled."

$(DBGOBJDIR):
	@mkdir -p "$(DBGOBJDIR)"

#
# Format rules
#
format:
	@echo "Formatting source files..."
	@for file in $(SRCS) $(HDRS) $(TMPLTS); do \
		echo "Formatting $$file"; \
		uncrustify -c ../Uncrustify.cfg --replace --no-backup $$file; \
	done
	@echo "All files formatted."

clean:
	@rm -f $(STZMAIN) $(STZEXE) $(DBGEXE) $(COVEXE)
	@rm -f $(DBGOBJS) $(DBGPRZROBJS)
	@rm -f $(STZOBJS) $(STZPRZROBJS)
	@rm -f $(COVOBJS) $(COVPRZROBJS)

	@test -d $(DBGLIBDIR) && rm -d $(DBGLIBDIR) 2>/dev/null || true
	@test -d $(DBGOBJDIR) && rm -d $(DBGOBJDIR) 2>/dev/null || true
	@test -d $(DBGDIR) && rm -d $(DBGDIR) 2>/dev/null || true

	@test -d $(STZOBJDIR) && rm -d $(STZOBJDIR) 2>/dev/null || true
	@test -d $(STZDIR) && rm -d $(STZDIR) 2>/dev/null || true

	@rm -f $(COVOBJDIR)/*.gcno $(COVOBJDIR)/*.gcda $(COVOBJDIR)/*.gcov $(COVOBJDIR)/coverage.info
	@rm -rf $(COVREPORTDIR)
	@test -d $(COVOBJDIR) && rm -d $(COVOBJDIR) 2>/dev/null || true
	@test -d $(COVDIR) && rm -d $(COVDIR) 2>/dev/null || true

	@test -d $(BUILEXE) && rm -d $(BUILEXE) 2>/dev/null || true
	@test -d $(BUILDS) && rm -d $(BUILDS) 2>/dev/null || true
	@echo $(EXE) cleared.

#
# Print of variables
#
print-% : ; @echo $* = $($*)

#
# GCC Static Analysis
#
gcc-analyzer: WFLAGS += -fanalyzer -fanalyzer-call-summaries -fanalyzer-transitivity -fanalyzer-verbose-edges -fanalyzer-verbose-state-changes -fanalyzer-verbosity=3 -flto
gcc-analyzer: CC = gcc
gcc-analyzer: debug

cppcheck:
	cppcheck --enable=all --platform=unix64 --std=c11 -q --force -i libs -i tests --inconclusive ./src
