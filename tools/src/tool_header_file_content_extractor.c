/**
 * @file get_all_headers.c
 * @brief Program to extract contents of local header files with include paths support
 *        and optional comment removal
 * @author Claude
 * @date 2024-12-03
 *
 * This program recursively processes C source files to find and display
 * the contents of all locally included header files (those included with "")
 * while avoiding cyclic dependencies. Can optionally remove C-style comments.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

/** Maximum allowed path length */
#define MAX_PATH 4096
/** Maximum line length when reading files */
#define MAX_LINE 1024
/** Maximum number of header files to process */
#define MAX_HEADERS 100
/** Maximum number of include paths */
#define MAX_INCLUDE_PATHS 100
/** Size of buffer for comment processing */
#define BUFFER_SIZE 4096
/** Application version */
#define VERSION "0.1"

/**
 * @brief Structure to track processed header files
 *
 * Stores information about header files that have been processed
 * to prevent cyclic dependencies and duplicate processing.
 */
typedef struct {
	char path[MAX_PATH];  /**< Full path to the header file */
	int processed;        /**< Flag indicating if file has been processed */
} HeaderFile;

/** Structure to manage include paths */
typedef struct {
	char paths[MAX_INCLUDE_PATHS][MAX_PATH];  /**< Array of include paths */
	int count;                                /**< Number of paths */
} IncludePaths;

/** Global array to track processed header files */
HeaderFile processed_headers[MAX_HEADERS];
/** Counter for number of processed headers */
int header_count = 0;
/** Flag to control comment removal */
int remove_comments = 0;
/** Global include paths structure */
IncludePaths include_paths = {{{0}}, 0};

/**
 * @brief Remove C-style comments from a string
 *
 * Handles both single-line (//) and multi-line comments (/ * * /)
 *
 * @param input Input string to process
 * @param output Buffer for processed string
 * @param in_comment Pointer to flag tracking multi-line comment state
 */
static void strip_comments(const char* input, char* output, int* in_comment) {
	char* out = output;
	const char* in = input;
	int inside_string = 0;

	// Continue from previous state if we're in a multi-line comment
	if (*in_comment) {
		while (*in) {
			if (in[0] == '*' && in[1] == '/') {
				*in_comment = 0;
				in += 2;
				break;
			}
			in++;
		}
	}

	// Process the rest of the line
	while (*in) {
		// Handle string literals
		if (*in == '"' && (in == input || *(in-1) != '\\')) {
			inside_string = !inside_string;
			*out++ = *in++;
			continue;
		}

		// Skip everything if we're inside a string
		if (inside_string) {
			*out++ = *in++;
			continue;
		}

		// Check for start of comments
		if (in[0] == '/' && in[1] == '*') {
			*in_comment = 1;
			in += 2;
			while (*in) {
				if (in[0] == '*' && in[1] == '/') {
					*in_comment = 0;
					in += 2;
					break;
				}
				in++;
			}
			continue;
		}

		// Handle single-line comments
		if (in[0] == '/' && in[1] == '/') {
			break;  // Skip rest of the line
		}

		// Copy normal characters
		*out++ = *in++;
	}

	// Null terminate and trim trailing whitespace
	*out = '\0';
	while (out > output && isspace(*(out-1))) {
		out--;
	}
	*out = '\0';
}

/**
 * @brief Add new include path to the search list
 *
 * @param path Path to add to include paths
 * @return int 0 on success, -1 if too many paths
 */
static int add_include_path(const char* path) {
	if (include_paths.count >= MAX_INCLUDE_PATHS) {
		return -1;
	}

	// Remove trailing slash if present
	size_t len = strlen(path);
	strncpy(include_paths.paths[include_paths.count], path, MAX_PATH - 1);
	if (len > 0 && path[len-1] == '/') {
		include_paths.paths[include_paths.count][len-1] = '\0';
	}

	include_paths.count++;
	return 0;
}

/**
 * @brief Check if a file exists
 *
 * @param path Path to the file to check
 * @return int 1 if file exists, 0 otherwise
 */
static int file_exists(const char* path) {
	struct stat buffer;
	return (stat(path, &buffer) == 0);
}

/**
 * @brief Try to find header file in all include paths
 *
 * @param header_name Name of the header to find
 * @param current_dir Current directory for relative path
 * @param result Buffer to store found path
 * @return int 1 if found, 0 if not found
 */
static int find_header_file(const char* header_name, const char* current_dir, char* result) {
	char temp_path[MAX_PATH];

	// First try current directory
	snprintf(temp_path, sizeof(temp_path), "%s%s", current_dir, header_name);
	if (file_exists(temp_path)) {
		strcpy(result, temp_path);
		return 1;
	}

	// Then try all include paths
	for (int i = 0; i < include_paths.count; i++) {
		snprintf(temp_path, sizeof(temp_path), "%s/%s",
			include_paths.paths[i], header_name);
		if (file_exists(temp_path)) {
			strcpy(result, temp_path);
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Check if a file has already been processed
 *
 * @param path Full path to the file to check
 * @return int 1 if file was processed, 0 otherwise
 */
static int was_processed(const char* path) {
	for (int i = 0; i < header_count; i++) {
		if (strcmp(processed_headers[i].path, path) == 0) {
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Add a file to the list of processed files
 *
 * @param path Full path to the file being processed
 * @note Prints warning if maximum header limit is reached
 */
static void add_processed(const char* path) {
	if (header_count >= MAX_HEADERS) {
		printf("Warning: Too many header files!\n");
		return;
	}
	strcpy(processed_headers[header_count].path, path);
	header_count++;
}

/**
 * @brief Extract directory path from full file path
 *
 * @param dir Buffer to store the extracted directory path
 * @param path Full file path to process
 * @note Modifies dir in-place to contain the directory portion of the path
 */
static void get_directory(char* dir, const char* path) {
	strcpy(dir, path);
	char* last_slash = strrchr(dir, '/');
	if (last_slash) {
		*(last_slash + 1) = '\0';
	} else {
		dir[0] = '\0';
	}
}

/**
 * @brief Recursively process a header file and its dependencies
 *
 * This function:
 * - Checks if file was already processed
 * - Opens and reads the file content
 * - Identifies local include directives
 * - Recursively processes included files
 * - Strip comments
 *
 * @param file_path Path to the file to process
 */
static void process_header(const char* file_path) {
	if (was_processed(file_path)) {
		return;
	}

	FILE* file = fopen(file_path, "r");
	if (!file) {
		printf("Error: Cannot open file %s\n", file_path);
		return;
	}

	add_processed(file_path);

	char current_dir[MAX_PATH];
	get_directory(current_dir, file_path);

	printf("\n/* === Content of %s === */\n", file_path);

	char line[MAX_LINE];
	char processed_line[MAX_LINE];
	int in_comment = 0;  // Track multi-line comment state

	while (fgets(line, sizeof(line), file)) {
		if (remove_comments) {
			strip_comments(line, processed_line, &in_comment);
			// Only print non-empty lines
			if (processed_line[0] != '\0') {
				printf("%s\n", processed_line);
			}
		} else {
			printf("%s", line);
		}

		// Look for local includes (#include "file.h")
		if (strncmp(line, "#include", 8) == 0) {
			char* start = strchr(line, '"');
			if (start) {
				start++;
				char* end = strchr(start, '"');
				if (end) {
					*end = '\0';

					char include_path[MAX_PATH];
					if (find_header_file(start, current_dir, include_path)) {
						process_header(include_path);
					} else {
						printf("Warning: Cannot find header file: %s\n", start);
					}
				}
			}
		}
	}

	printf("/* === End of %s === */\n", file_path);
	fclose(file);
}

/**
 * @brief Display detailed program usage information and help
 *
 * Prints comprehensive information about:
 * - Program purpose and functionality
 * - Command-line syntax
 * - Available options and their effects
 * - Usage examples
 *
 * @param program_name Name of the executable
 */
static void print_usage(const char* program_name) {
    printf("\nHeader Files Content Extractor v1.0\n");
    printf("====================================\n\n");

    printf("Description:\n");
    printf("  Recursively processes C source files to extract and display the contents of all locally\n");
    printf("  included header files while preventing cyclic dependencies. Supports multiple include\n");
    printf("  paths and optional comment removal.\n\n");

    printf("Usage:\n");
    printf("  %s [-s] [-I include_path ...] <source_file>\n\n", program_name);

    printf("Arguments:\n");
    printf("  source_file    Path to the main C source file to process\n\n");

    printf("Options:\n");
    printf("  -s            Strip all C-style comments (// and /* */) from the output\n");
    printf("  -I <path>     Add directory to header search path. Multiple -I options are allowed\n");
    printf("                Headers are searched in the specified order:\n");
    printf("                1. Current directory of the including file\n");
    printf("                2. Directories specified by -I options (in order of appearance)\n\n");

    printf("Examples:\n");
    printf("  # Basic usage. Process a source file with default settings:\n");
    printf("  %s main.c\n\n", program_name);

    printf("  # With comment stripping:\n");
    printf("  %s -s source.c\n\n", program_name);

    printf("  # Include multiple search paths and strip comments:\n");
    printf("  %s -s -I ./include -I ../common/headers -I /usr/local/include main.c\n\n", program_name);

    printf("  # Process with a single include path:\n");
    printf("  %s -I ./project/headers source.c\n\n", program_name);

    printf("  # Show help:\n");
    printf("  %s --help\n\n", program_name);

    printf("  # Show version:\n");
    printf("  %s --version\n\n", program_name);

    printf("Limitations:\n");
    printf("  - Maximum path length: %d characters\n", MAX_PATH);
    printf("  - Maximum number of include paths: %d\n", MAX_INCLUDE_PATHS);
    printf("  - Maximum number of processed headers: %d\n\n", MAX_HEADERS);

    printf("Notes:\n");
    printf("  - Only processes local header includes (#include \"file.h\")\n");
    printf("  - System includes (#include <file.h>) are ignored\n");
    printf("  - Detects and prevents circular dependencies\n");
    printf("  - Maintains header inclusion order as specified in source\n");
}

/**
 * @brief Program entry point
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return int 0 on success, 1 on error
 */
int main(int argc, char* argv[]) {
	char* input_file = NULL;

	// Parse command line arguments
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-I") == 0) {
			if (++i >= argc) {
				printf("Error: -I option requires a directory path\n");
				print_usage(argv[0]);
				return 1;
			}
			if (add_include_path(argv[i]) != 0) {
				printf("Error: Too many include paths\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-s") == 0) {
			remove_comments = 1;
		} else if (strcmp(argv[i], "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		} else if (strcmp(argv[i], "--version") == 0) {
			printf("%s version: %s\n", argv[0], VERSION);
			return 0;
		} else if (input_file == NULL) {
			input_file = argv[i];
		} else {
			print_usage(argv[0]);
			return 1;
		}
	}

	if (input_file == NULL) {
		print_usage(argv[0]);
		return 1;
	}

	if (!file_exists(input_file)) {
		printf("Error: File %s does not exist\n", input_file);
		return 1;
	}

	memset(processed_headers, 0, sizeof(processed_headers));
	header_count = 0;

	process_header(input_file);

	printf("\n/* Processed %d header files */\n", header_count);
	return 0;
}
