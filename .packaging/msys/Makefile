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
PAYLOAD_HASH = $(shell precizer_hash="$$(sha256sum "$(PRECIZER_PAYLOAD)" | awk '{print $$1}')"; msys_hash="$$(sha256sum "$(MSYS_RUNTIME)" | awk '{print $$1}')"; printf '%s%s' "$$precizer_hash" "$$msys_hash" | sha256sum | awk '{print $$1}')
PAYLOAD_HASH_PATH = sha256-$(PAYLOAD_HASH)
PAYLOAD_HASH_DEFINE = -DPRECIZER_LAUNCHER_PAYLOAD_HASH=L\"$(PAYLOAD_HASH_PATH)\"

.PHONY: all launcher clean

all: launcher

launcher: $(OUTPUT_EXE)

$(OUTPUT_EXE): $(PRECIZER_PAYLOAD) $(MSYS_RUNTIME) $(PROJECT_DIR)/src/version.h Makefile launcher.c launcher.rc.in | $(PAYLOAD_DIR)
	@cp "$(PRECIZER_PAYLOAD)" "$(PAYLOAD_DIR)/precizer.exe"
	@cp "$(MSYS_RUNTIME)" "$(PAYLOAD_DIR)/msys-2.0.dll"
	@cp launcher.rc.in "$(BUILD_DIR)/launcher.rc"
	@echo "Embedded payload hash path: $(PAYLOAD_HASH_PATH)"
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
		$(PAYLOAD_HASH_DEFINE) \
		-I"$(PROJECT_DIR)/src" \
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
