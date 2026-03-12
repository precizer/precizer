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
IncludePaths include_paths = {{{0}},0};

/**
 * @brief Copy string into fixed-size path buffer
 *
 * @param destination Destination buffer
 * @param destination_size Destination buffer size
 * @param source Source string
 * @return int 1 on success, 0 on truncation or invalid input
 */
static int copy_path_string(
	char       *destination,
	size_t     destination_size,
	const char *source)
{
	int written = 0;

	if(NULL == destination || NULL == source || destination_size == 0U)
	{
		return 0;
	}

	written = snprintf(destination,destination_size,"%s",source);

	if(written < 0 || (size_t)written >= destination_size)
	{
		destination[0] = '\0';
		return 0;
	}

	return 1;
}

/**
 * @brief Join directory path with relative file name
 *
 * @param destination Destination buffer
 * @param destination_size Destination buffer size
 * @param directory Directory path
 * @param file_name Relative file name
 * @return int 1 on success, 0 on truncation or invalid input
 */
static int join_path(
	char       *destination,
	size_t     destination_size,
	const char *directory,
	const char *file_name)
{
	int written = 0;

	if(NULL == destination || NULL == directory || NULL == file_name ||
		destination_size == 0U)
	{
		return 0;
	}

	if(directory[0] == '\0' || strcmp(directory,".") == 0)
	{
		written = snprintf(destination,destination_size,"%s",file_name);
	} else if(strcmp(directory,"/") == 0) {
		written = snprintf(destination,destination_size,"/%s",file_name);
	} else {
		written = snprintf(destination,destination_size,"%s/%s",
			directory,file_name);
	}

	if(written < 0 || (size_t)written >= destination_size)
	{
		destination[0] = '\0';
		return 0;
	}

	return 1;
}

/**
 * @brief Resolve existing path to canonical absolute path
 *
 * @param path Input path
 * @param resolved_path Buffer for canonical path
 * @return int 1 on success, 0 otherwise
 */
static int canonicalize_existing_path(
	const char *path,
	char       *resolved_path)
{
	if(NULL == path || NULL == resolved_path)
	{
		return 0;
	}

	if(NULL == realpath(path,resolved_path))
	{
		resolved_path[0] = '\0';
		return 0;
	}

	return 1;
}

/**
 * @brief Check whether path contains parent directory traversal
 *
 * @param path Path to inspect
 * @return int 1 if traversal is present, 0 otherwise
 */
__attribute__((pure)) static int path_contains_parent_reference(const char *path)
{
	const char *segment = NULL;

	if(NULL == path)
	{
		return 0;
	}

	segment = path;

	while(*segment != '\0')
	{
		const char *separator = strchr(segment,'/');
		size_t segment_length = 0U;

		if(NULL == separator)
		{
			segment_length = strlen(segment);
		} else {
			segment_length = (size_t)(separator - segment);
		}

		if(segment_length == 2U && segment[0] == '.' && segment[1] == '.')
		{
			return 1;
		}

		if(NULL == separator)
		{
			break;
		}

		segment = separator + 1;
	}

	return 0;
}

/**
 * @brief Check whether canonical path stays within canonical root
 *
 * @param path Canonical path
 * @param root Canonical root path
 * @return int 1 if path is inside root, 0 otherwise
 */
__attribute__((pure)) static int path_is_within_root(
	const char *path,
	const char *root)
{
	size_t root_length = 0U;

	if(NULL == path || NULL == root)
	{
		return 0;
	}

	if(strcmp(root,"/") == 0)
	{
		return path[0] == '/';
	}

	root_length = strlen(root);

	if(strncmp(path,root,root_length) != 0)
	{
		return 0;
	}

	return path[root_length] == '\0' || path[root_length] == '/';
}

/**
 * @brief Check whether header name is safe for local include resolution
 *
 * @param header_name Header name from source code
 * @return int 1 if header name is allowed, 0 otherwise
 */
__attribute__((pure)) static int is_allowed_header_name(const char *header_name)
{
	if(NULL == header_name || header_name[0] == '\0')
	{
		return 0;
	}

	if(header_name[0] == '/')
	{
		return 0;
	}

	if(path_contains_parent_reference(header_name))
	{
		return 0;
	}

	return 1;
}

/**
 * @brief Remove C-style comments from a string
 *
 * Handles both single-line (//) and multi-line comments (/ * * /)
 *
 * @param input Input string to process
 * @param output Buffer for processed string
 * @param in_comment Pointer to flag tracking multi-line comment state
 */
static void strip_comments(
	const char *input,
	char       *output,
	int        *in_comment)
{
	char *out = output;
	const char *in = input;
	int inside_string = 0;

	// Continue from previous state if we're in a multi-line comment
	if(*in_comment)
	{
		while(*in)
		{
			if(in[0] == '*' && in[1] == '/')
			{
				*in_comment = 0;
				in += 2;
				break;
			}
			in++;
		}
	}

	// Process the rest of the line
	while(*in)
	{
		// Handle string literals
		if(*in == '"' && (in == input || *(in-1) != '\\'))
		{
			inside_string = !inside_string;
			*out++ = *in++;
			continue;
		}

		// Skip everything if we're inside a string
		if(inside_string)
		{
			*out++ = *in++;
			continue;
		}

		// Check for start of comments
		if(in[0] == '/' && in[1] == '*')
		{
			*in_comment = 1;
			in += 2;

			while(*in)
			{
				if(in[0] == '*' && in[1] == '/')
				{
					*in_comment = 0;
					in += 2;
					break;
				}
				in++;
			}
			continue;
		}

		// Handle single-line comments
		if(in[0] == '/' && in[1] == '/')
		{
			break;  // Skip rest of the line
		}

		// Copy normal characters
		*out++ = *in++;
	}

	// Null terminate and trim trailing whitespace
	*out = '\0';

	while(out > output && isspace(*(out-1)))
	{
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
static int add_include_path(const char *path)
{
	char resolved_path[MAX_PATH];
	struct stat path_status;

	if(include_paths.count >= MAX_INCLUDE_PATHS)
	{
		return -1;
	}

	if(!canonicalize_existing_path(path,resolved_path))
	{
		return -2;
	}

	if(stat(resolved_path,&path_status) != 0 || !S_ISDIR(path_status.st_mode))
	{
		return -2;
	}

	if(!copy_path_string(include_paths.paths[include_paths.count],
		sizeof(include_paths.paths[include_paths.count]),resolved_path))
	{
		return -3;
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
static int file_exists(const char *path)
{
	struct stat buffer;
	return (stat(path,&buffer) == 0);
}

/**
 * @brief Try to find header file in all include paths
 *
 * @param header_name Name of the header to find
 * @param current_dir Current directory for relative path
 * @param result Buffer to store found path
 * @return int 1 if found, 0 if not found
 */
static int find_header_file(
	const char *header_name,
	const char *current_dir,
	char       *result,
	size_t     result_size)
{
	char temp_path[MAX_PATH];
	char resolved_path[MAX_PATH];

	if(!is_allowed_header_name(header_name))
	{
		return 0;
	}

	// First try current directory
	if(join_path(temp_path,sizeof(temp_path),current_dir,header_name) &&
		canonicalize_existing_path(temp_path,resolved_path) &&
		path_is_within_root(resolved_path,current_dir))
	{
		return copy_path_string(result,result_size,resolved_path);
	}

	// Then try all include paths
	for(int i = 0; i < include_paths.count; i++)
	{
		if(join_path(temp_path,sizeof(temp_path),include_paths.paths[i],header_name) &&
			canonicalize_existing_path(temp_path,resolved_path) &&
			path_is_within_root(resolved_path,include_paths.paths[i]))
		{
			return copy_path_string(result,result_size,resolved_path);
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
static int was_processed(const char *path)
{
	for(int i = 0; i < header_count; i++)
	{
		if(strcmp(processed_headers[i].path,path) == 0)
		{
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
static void add_processed(const char *path)
{
	if(header_count >= MAX_HEADERS)
	{
		printf("Warning: Too many header files!\n");
		return;
	}

	if(!copy_path_string(processed_headers[header_count].path,
		sizeof(processed_headers[header_count].path),path))
	{
		printf("Warning: Header path is too long: %s\n",path);
		return;
	}

	header_count++;
}

/**
 * @brief Extract directory path from full file path
 *
 * @param dir Buffer to store the extracted directory path
 * @param path Full file path to process
 * @note Modifies dir in-place to contain the directory portion of the path
 */
static void get_directory(
	char       *dir,
	size_t     dir_size,
	const char *path)
{
	if(!copy_path_string(dir,dir_size,path))
	{
		dir[0] = '\0';
		return;
	}

	char *last_slash = strrchr(dir,'/');

	if(last_slash)
	{
		if(last_slash == dir)
		{
			dir[1] = '\0';
		} else {
			*last_slash = '\0';
		}
	} else {
		if(dir_size > 1U)
		{
			dir[0] = '.';
			dir[1] = '\0';
		}
	}
}

/** Global array to track missing header files */
char missing_headers[MAX_HEADERS][MAX_PATH];
/** Counter for number of missing headers */
int missing_count = 0;

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
static void process_header(const char *file_path)
{
	if(was_processed(file_path))
	{
		return;
	}

	FILE *file = fopen(file_path,"r");

	if(!file)
	{
		printf("Error: Cannot open file %s\n",file_path);
		return;
	}

	add_processed(file_path);

	char current_dir[MAX_PATH];
	get_directory(current_dir,sizeof(current_dir),file_path);

	printf("\n/* === Content of %s === */\n",file_path);

	char line[MAX_LINE];
	char processed_line[MAX_LINE];
	int in_comment = 0;  // Track multi-line comment state

	while(fgets(line,sizeof(line),file))
	{
		if(remove_comments)
		{
			strip_comments(line,processed_line,&in_comment);

			// Only print non-empty lines
			if(processed_line[0] != '\0')
			{
				printf("%s\n",processed_line);
			}
		} else {
			printf("%s",line);
		}

		// Look for local includes (#include "file.h")
		if(strncmp(line,"#include",8) == 0)
		{
			char *start = strchr(line,'"');

			if(start)
			{
				start++;
				char *end = strchr(start,'"');

				if(end)
				{
					*end = '\0';

					char include_path[MAX_PATH];

					if(find_header_file(start,current_dir,include_path,
						sizeof(include_path)))
					{
						process_header(include_path);
					} else {
						printf("Warning: Cannot find header file: %s\n",start);

						if(missing_count < MAX_HEADERS)
						{
							strncpy(missing_headers[missing_count],start,MAX_PATH - 1);
							missing_headers[missing_count][MAX_PATH - 1] = '\0';
							missing_count++;
						}
					}
				}
			}
		}
	}

	printf("/* === End of %s === */\n",file_path);
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
static void print_usage(const char *program_name)
{
	printf("\nHeader Files Content Extractor v1.0\n");
	printf("====================================\n\n");

	printf("Description:\n");
	printf("  Recursively processes C source files to extract and display the contents of all locally\n");
	printf("  included header files while preventing cyclic dependencies. Supports multiple include\n");
	printf("  paths and optional comment removal.\n\n");

	printf("Usage:\n");
	printf("  %s [-s] [-I include_path ...] <source_file>\n\n",program_name);

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
	printf("  %s main.c\n\n",program_name);

	printf("  # With comment stripping:\n");
	printf("  %s -s source.c\n\n",program_name);

	printf("  # Include multiple search paths and strip comments:\n");
	printf("  %s -s -I ./include -I ../common/headers -I /usr/local/include main.c\n\n",program_name);

	printf("  # Process with a single include path:\n");
	printf("  %s -I ./project/headers source.c\n\n",program_name);

	printf("  # Show help:\n");
	printf("  %s --help\n\n",program_name);

	printf("  # Show version:\n");
	printf("  %s --version\n\n",program_name);

	printf("Limitations:\n");
	printf("  - Maximum path length: %d characters\n",MAX_PATH);
	printf("  - Maximum number of include paths: %d\n",MAX_INCLUDE_PATHS);
	printf("  - Maximum number of processed headers: %d\n\n",MAX_HEADERS);

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
int main(
	int  argc,
	char *argv[])
{
	const char *input_file = NULL;
	char resolved_input_file[MAX_PATH];

	// Parse command line arguments
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i],"-I") == 0)
		{
			if(++i >= argc)
			{
				printf("Error: -I option requires a directory path\n");
				print_usage(argv[0]);
				return 1;
			}

			int include_status = add_include_path(argv[i]);

			if(include_status == -1)
			{
				printf("Error: Too many include paths\n");
				return 1;
			}

			if(include_status == -2)
			{
				printf("Error: Invalid include path: %s\n",argv[i]);
				return 1;
			}

			if(include_status == -3)
			{
				printf("Error: Include path is too long: %s\n",argv[i]);
				return 1;
			}
		} else if(strcmp(argv[i],"-s") == 0){
			remove_comments = 1;
		} else if(strcmp(argv[i],"--help") == 0){
			print_usage(argv[0]);
			return 0;
		} else if(strcmp(argv[i],"--version") == 0){
			printf("%s version: %s\n",argv[0],VERSION);
			return 0;
		} else if(input_file == NULL){
			input_file = argv[i];
		} else {
			print_usage(argv[0]);
			return 1;
		}
	}

	if(input_file == NULL)
	{
		print_usage(argv[0]);
		return 1;
	}

	if(!file_exists(input_file))
	{
		printf("Error: File %s does not exist\n",input_file);
		return 1;
	}

	if(!canonicalize_existing_path(input_file,resolved_input_file))
	{
		printf("Error: Cannot resolve file %s\n",input_file);
		return 1;
	}

	memset(processed_headers,0,sizeof(processed_headers));
	header_count = 0;
	missing_count = 0;

	process_header(resolved_input_file);

	if(missing_count > 0)
	{
		printf("\n/* Missing header files: */\n");

		for(int i = 0; i < missing_count; i++)
		{
			printf(" * %s\n",missing_headers[i]);
		}
	}

	printf("\n/* Processed %d header files */\n",header_count);
	return 0;
}
