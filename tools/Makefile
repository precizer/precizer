# Compiler and flags
CC := gcc
CFLAGS += -pipe -std=c11
CFLAGS += -fbuiltin
CFLAGS += -O2
CFLAGS += -Werror
CFLAGS += -Wall -Wextra -Wpedantic -Wshadow
CFLAGS += -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code -Wformat-y2k
CFLAGS += -Wformat-nonliteral -Wformat-security -Wmissing-include-dirs
CFLAGS += -Wswitch-default -Wtrigraphs -Wstrict-overflow=5
CFLAGS += -Wfloat-equal -Wundef -Wshadow
CFLAGS += -Wbad-function-cast -Wcast-qual -Wcast-align
CFLAGS += -Wsuggest-attribute=const -Wsuggest-attribute=pure -Wsuggest-attribute=noreturn
CFLAGS += -Wsuggest-attribute=format -Wmissing-format-attribute
CFLAGS += -Wwrite-strings
CFLAGS += -Winline
# If it is not clang, then these options are for gcc
ifneq ($(CC), clang)
CFLAGS += -Wlogical-op
endif

# Directories
SRC_DIR := src
BIN_DIR := .

# Find all .c files in src directory
SRCS := $(wildcard $(SRC_DIR)/*.c)
# Generate binary names by stripping .c and src/ prefix
BINS := $(patsubst $(SRC_DIR)/%.c,%,$(SRCS))

# Default target
all: $(BINS)

# Rule to build binaries
%: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) $< -o $(BIN_DIR)/$@
	@echo $@ done

# Clean target
clean:
	rm -f $(BINS)

# Show help
help:
	@echo "Available targets:"
	@echo "  all    - build all binaries (default)"
	@echo "  clean  - remove all built binaries"
	@echo "  help   - show this help message"

# Declare phony targets
.PHONY: all clean help
