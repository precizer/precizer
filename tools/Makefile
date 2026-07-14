# Config settings:
# The --no-print-directory option of make tells make not to print the message about entering and leaving the working directory.
MAKEFLAGS += --no-print-directory

CC := gcc

#
# Compiler flags
#
C_STANDARD := -std=c2x
CPPFLAGS ?=
CFLAGS ?= -finline-functions
LDFLAGS ?=
# Callers may replace these defaults through OPT_CFLAGS and OPT_LDFLAGS.
# Tool optimization flags follow CFLAGS and LDFLAGS and win conflicts
OPT_CFLAGS ?= -pipe -fbuiltin -flto -O2
TOOL_CFLAGS ?= $(OPT_CFLAGS)
OPT_LDFLAGS ?=
TOOL_LDFLAGS ?= $(OPT_LDFLAGS)

# Detect whether we're using GCC
GCC := $(findstring gcc,$(notdir $(firstword $(CC))))

# Warning flags for additional checks
WFLAGS += -Werror
WFLAGS += -Wall
WFLAGS += -Wextra
WFLAGS += -Wpedantic
WFLAGS += -Wshadow
WFLAGS += -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code -Wformat-y2k
WFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs
WFLAGS += -Wswitch-default -Wtrigraphs -Wstrict-overflow=5
WFLAGS += -Wfloat-equal -Wundef
WFLAGS += -Wbad-function-cast -Wcast-qual -Wcast-align
WFLAGS += -Wwrite-strings
WFLAGS += -Winline
ifneq ($(GCC),)
WFLAGS += -Wlogical-op
WFLAGS += -Wsuggest-attribute=const
WFLAGS += -Wsuggest-attribute=pure
WFLAGS += -Wsuggest-attribute=noreturn
WFLAGS += -Wsuggest-attribute=format
WFLAGS += -Wmissing-format-attribute
endif

#
# Project files
#
SRC_DIR = src
BIN_DIR = .
SRCS = $(wildcard $(SRC_DIR)/*.c)
BINS = $(patsubst $(SRC_DIR)/%.c,%,$(SRCS))

# Default target
all: $(BINS)

# Rule to build binaries
%: $(SRC_DIR)/%.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(TOOL_CFLAGS) $(WFLAGS) $(C_STANDARD) $< $(LDFLAGS) $(TOOL_LDFLAGS) -o $(BIN_DIR)/$@
	@echo $@ done

#
# Format rules
#
format:
	@echo "Formatting source files..."
	@for file in $(SRCS); do \
		echo "Formatting $$file"; \
		uncrustify -c ../Uncrustify.cfg --replace --no-backup $$file; \
	done
	@echo "All files formatted."

# Clean target
clean:
	@rm -f $(BINS)
	@echo "Tools are cleaned"

# Show help
help:
	@echo "Available targets:"
	@echo "  all    - build all binaries (default)"
	@echo "  clean  - remove all built binaries"
	@echo "  help   - show this help message"

# Declare phony targets
.PHONY: all clean help format
