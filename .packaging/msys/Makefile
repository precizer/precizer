.SUFFIXES:

PROJECT_DIR = ../..
BUILD_DIR = $(PROJECT_DIR)/.builds/msys-launcher
PAYLOAD_DIR = $(BUILD_DIR)/payload
PRECIZER_PAYLOAD = $(PROJECT_DIR)/package/msys-payload/precizer.exe
MSYS_RUNTIME = $(PROJECT_DIR)/package/msys-payload/msys-2.0.dll
OUTPUT_EXE = $(PROJECT_DIR)/precizer_windows_x64_portable.exe
CC = $(shell cygpath -am /ucrt64/bin/gcc.exe)
WINDRES = $(shell cygpath -am /ucrt64/bin/windres.exe)
PATH := /ucrt64/bin:$(PATH)

.PHONY: all launcher clean

all: launcher

launcher: $(OUTPUT_EXE)

$(OUTPUT_EXE): $(PRECIZER_PAYLOAD) $(MSYS_RUNTIME) Makefile launcher.c launcher.h launcher.rc.in | $(PAYLOAD_DIR)
	@cp "$(PRECIZER_PAYLOAD)" "$(PAYLOAD_DIR)/precizer.exe"
	@cp "$(MSYS_RUNTIME)" "$(PAYLOAD_DIR)/msys-2.0.dll"
	@cp launcher.h "$(BUILD_DIR)/launcher.h"
	@cp launcher.rc.in "$(BUILD_DIR)/launcher.rc"
	@version_path="$$(sed -n 's/^#define APP_VERSION "\([^"]*\)"/\1/p' "$(PROJECT_DIR)/src/version.h" | sed 's/[^A-Za-z0-9._-]/_/g')"; \
	if [ -z "$$version_path" ]; then \
		echo "Unable to read APP_VERSION from $(PROJECT_DIR)/src/version.h" >&2; \
		exit 1; \
	fi; \
	precizer_hash="$$(sha256sum "$(PRECIZER_PAYLOAD)" | awk '{print $$1}')"; \
	msys_hash="$$(sha256sum "$(MSYS_RUNTIME)" | awk '{print $$1}')"; \
	payload_hash="$$(printf '%s%s' "$$precizer_hash" "$$msys_hash" | sha256sum | awk '{print $$1}')"; \
	payload_hash_path="sha256-$$payload_hash"; \
	version_literal="$$(printf '%s' "$$version_path" | sed 's/\\/\\\\/g; s/"/\\"/g')"; \
	hash_literal="$$(printf '%s' "$$payload_hash_path" | sed 's/\\/\\\\/g; s/"/\\"/g')"; \
	{ \
		echo '#ifndef PRECIZER_MSYS_LAUNCHER_CONFIG_H'; \
		echo '#define PRECIZER_MSYS_LAUNCHER_CONFIG_H'; \
		echo; \
		echo "#define PRECIZER_LAUNCHER_VERSION L\"$$version_literal\""; \
		echo "#define PRECIZER_LAUNCHER_PAYLOAD_HASH L\"$$hash_literal\""; \
		echo; \
		echo '#endif /* PRECIZER_MSYS_LAUNCHER_CONFIG_H */'; \
	} > "$(BUILD_DIR)/launcher_config.h"; \
	echo "Embedded version path: $$version_path"; \
	echo "Embedded payload hash path: $$payload_hash_path"
	@cd "$(BUILD_DIR)" && "$(WINDRES)" \
		--preprocessor="$(CC)" \
		--preprocessor-arg=-E \
		--preprocessor-arg=-xc \
		--preprocessor-arg=-DRC_INVOKED \
		-O coff \
		-i launcher.rc \
		-o launcher.res
	@"$(CC)" \
		-std=c17 \
		-O2 \
		-Wall \
		-Wextra \
		-Wpedantic \
		-DUNICODE \
		-D_UNICODE \
		-I"$(BUILD_DIR)" \
		-mconsole \
		-static \
		-static-libgcc \
		launcher.c \
		"$(BUILD_DIR)/launcher.res" \
		-o "$(OUTPUT_EXE)" \
		-lshell32
	@test -f "$(OUTPUT_EXE)"
	@echo "Built $(OUTPUT_EXE)"

$(PAYLOAD_DIR):
	@mkdir -p "$@"

clean:
	@rm -rf "$(BUILD_DIR)" "$(OUTPUT_EXE)"
