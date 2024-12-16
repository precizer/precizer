/**
 * @file function_remover.c
 * @brief Tool for in-place removal of specified functions from C source files
 *
 * @details This program modifies C source files in specified directory by removing
 * specified function while preserving the rest of the code structure and formatting.
 */

/* Add required header for strdup() */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define MAX_PATH_LENGTH 4096
#define TEMP_FILENAME "_temp_file_"

/**
 * @brief Status codes for function returns
 */
typedef enum {
	SUCCESS = 0,
	FAILURE = 1,
	WARNING = 2,
	DONOTHING = 3
} Return;

/* Previous DeclarationState structure and helper functions remain the same */
typedef struct {
	int in_declaration;
	int parentheses_count;
	int found_semicolon;
} DeclarationState;

/**
 * @brief Container for function names to remove
 */
typedef struct {
	char**	names;		/**< Array of function names */
	int	count;		/**< Number of functions */
} FunctionNames;

/* Function declarations */
static Return init_declaration_state(DeclarationState*);
static Return update_declaration_state(DeclarationState*, char);
static Return is_function_declaration(const char*, const char*);
static Return is_target_function(
	const char*,
	const FunctionNames*
);
static void free_function_names(
	FunctionNames*	names
);

/**
 * @brief Check if file has C source code extension
 *
 * @param[in] filename Name of file to check
 *
 * @return Return status code
 * @retval SUCCESS File is a C source file
 * @retval FAILURE File is not a C source file
 */
__attribute__((pure)) static Return is_c_source_file(
	const char*	filename
){
	Return status = SUCCESS;
	const char* extension = NULL;

	if(NULL == filename)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		extension = strrchr(filename, '.');
		if(NULL == extension)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(strcmp(extension, ".c") != 0 && strcmp(extension, ".h") != 0)
		{
			status = FAILURE;
		}
	}

	return(status);
}

/**
 * @brief Update declaration state based on current character
 *
 * @param[in,out] state Current state
 * @param[in]     c     Character to process
 *
 * @return Return status code
 */
Return update_declaration_state(
	DeclarationState*	state,
	char			c
){
	Return status = SUCCESS;

	if(NULL == state)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		switch(c)
		{
			case '(':
				state->parentheses_count++;
				break;
			case ')':
				state->parentheses_count--;
				break;
			case ';':
				if(state->parentheses_count == 0)
				{
					state->found_semicolon = 1;
					state->in_declaration = 0;
				}
				break;
			default:
				break;
		}
	}

	return(status);
}

/**
 * @brief Initialize declaration state
 *
 * @param[out] state State structure to initialize
 *
 * @return Return status code
 */
static Return init_declaration_state(
	DeclarationState*	state
){
	Return status = SUCCESS;

	if(NULL == state)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		state->in_declaration = 0;
		state->parentheses_count = 0;
		state->found_semicolon = 0;
	}

	return(status);
}

/**
 * @brief Checks if a line contains potential function declaration
 *
 * @param[in] line          Input string to check
 * @param[in] function_name Name of the function to find
 *
 * @return Return status code
 * @retval SUCCESS Found potential function declaration
 * @retval FAILURE Not a function declaration
 */
__attribute__((pure)) static Return is_function_declaration(
	const char*	line,
	const char*	function_name
){
	Return status = SUCCESS;
	char* pos = NULL;
	char* name_end = NULL;

	if(NULL == line || NULL == function_name)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		pos = strstr(line, function_name);

		if(NULL == pos)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		name_end = pos + strlen(function_name);

		/* Check what's before the name - should be space/tab/newline */
		if(pos > line && !isspace(*(pos - 1)))
		{
			status = FAILURE;
		}

		/* Check what's after the name - should be space/tab/newline/( */
		if(*name_end != '\0' && !isspace(*name_end) && *name_end != '(')
		{
			status = FAILURE;
		}
	}

	return(status);
}

/**
 * @brief Remove all specified functions from source file
 *
 * @param[in] filename Source file path
 * @param[in] names    Container with function names
 *
 * @return Return status code
 */
static Return remove_functions(
	const char*		filename,
	const FunctionNames*	names
){
	Return status = SUCCESS;
	FILE* source = NULL;
	FILE* temp = NULL;
	char line[MAX_LINE_LENGTH];
	DeclarationState decl_state;
	int found_opening_brace = 0;
	char* p = NULL;

	if(NULL == filename || NULL == names)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = init_declaration_state(&decl_state);
	}

	if(SUCCESS == status)
	{
		source = fopen(filename, "r");
		if(NULL == source)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		temp = fopen(TEMP_FILENAME, "w");
		if(NULL == temp)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		while(fgets(line, sizeof(line), source))
		{
			if(!decl_state.in_declaration)
			{
				if(SUCCESS == is_target_function(line, names))
				{
					decl_state.in_declaration = 1;
					decl_state.parentheses_count = 0;
					decl_state.found_semicolon = 0;
					found_opening_brace = 0;

					/* Process current line */
					for(p = line; *p != '\0'; p++)
					{
						if(*p == '{')
						{
							found_opening_brace = 1;
							break;
						}
						status = update_declaration_state(&decl_state, *p);
						if(SUCCESS != status)
						{
							break;
						}
					}
				}
				else
				{
					fputs(line, temp);
				}
			}
			else
			{
				/* Still in declaration or definition */
				for(p = line; *p != '\0'; p++)
				{
					if(*p == '{')
					{
						found_opening_brace = 1;
						break;
					}
					status = update_declaration_state(&decl_state, *p);
					if(SUCCESS != status)
					{
						break;
					}
				}

				/* Check if we found function body */
				if(found_opening_brace && decl_state.parentheses_count == 0)
				{
					/* Function definition - switch to brace counting */
					int brace_count = 1;
					while(fgets(line, sizeof(line), source))
					{
						for(p = line; *p != '\0'; p++)
						{
							if(*p == '{') brace_count++;
							if(*p == '}') brace_count--;
						}
						if(brace_count == 0)
						{
							decl_state.in_declaration = 0;
							break;
						}
					}
				}
			}
		}
	}

	if(NULL != source)
	{
		fclose(source);
	}
	if(NULL != temp)
	{
		fclose(temp);
	}

	if(SUCCESS == status)
	{
		if(remove(filename) != 0)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(rename(TEMP_FILENAME, filename) != 0)
		{
			status = FAILURE;
		}
	}
	else
	{
		remove(TEMP_FILENAME);
	}

	return(status);
}

/**
 * @brief Process single directory entry
 *
 * @param[in] dir_path Path to directory
 * @param[in] entry    Directory entry
 * @param[in] names    Container with function names
 *
 * @return Return status code
 */
static Return process_directory_entry(
	const char*		dir_path,
	const struct dirent*	entry,
	const FunctionNames*	names
){
	Return status = SUCCESS;
	char full_path[MAX_PATH_LENGTH];
	struct stat path_stat;

	if(NULL == dir_path || NULL == entry || NULL == names)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Skip . and .. */
		if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			status = DONOTHING;
		}
	}

	if(SUCCESS == status)
	{
		/* Construct full path */
		snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

		/* Get file status */
		if(stat(full_path, &path_stat) != 0)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Handle only regular files */
		if(S_ISREG(path_stat.st_mode))
		{
			if(SUCCESS == is_c_source_file(entry->d_name))
			{
				#if 0
				printf("Processing file: %s\n", full_path);
				#endif
				status = remove_functions(full_path, names);
			}
		}
	}

	return(status);
}

/**
 * @brief Initialize function names container
 *
 * @param[in] argc     Number of command line arguments
 * @param[in] argv     Array of command line arguments
 * @param[out] names   Container to initialize
 *
 * @return Return status code
 */
static Return init_function_names(
	int		argc,
	char**		argv,
	FunctionNames*	names
){
	Return status = SUCCESS;
	int i = 0;
	size_t array_size = 0;
	int function_count = 0;

	if(NULL == argv || NULL == names || argc < 3)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		function_count = argc - 2; /* Subtract program name and directory path */
		
		/* Ensure we don't have negative count */
		if(function_count <= 0)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Calculate array size safely */
		array_size = (size_t)function_count * sizeof(char*);
		
		/* Check for potential overflow */
		if(array_size / sizeof(char*) != (size_t)function_count)
		{
			fputs("General Failure\n", stderr);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		names->count = function_count;
		names->names = (char**)malloc(array_size);

		if(NULL == names->names)
		{
			fputs("Memory allocation failed\n", stderr);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		for(i = 0; i < names->count; i++)
		{
			names->names[i] = strdup(argv[i + 2]);
			if(NULL == names->names[i])
			{
				status = FAILURE;
				break;
			}
		}
	}

	/* Clean up on failure */
	if(FAILURE == status && NULL != names->names)
	{
		free_function_names(names);
		names->names = NULL;
		names->count = 0;
	}

	return(status);
}

/**
 * @brief Free function names container
 *
 * @param[in] names Container to free
 */
static void free_function_names(
	FunctionNames*	names
){
	int i = 0;

	if(NULL != names && NULL != names->names)
	{
		for(i = 0; i < names->count; i++)
		{
			if(NULL != names->names[i])
			{
				free(names->names[i]);
			}
		}
		free(names->names);
	}
}

/**
 * @brief Check if line contains any of the specified functions
 *
 * @param[in] line  Line to check
 * @param[in] names Container with function names
 *
 * @return Return status code
 * @retval SUCCESS Found function declaration
 * @retval FAILURE Not a function declaration
 */
__attribute__((pure)) static Return is_target_function(
	const char*		line,
	const FunctionNames*	names
){
	Return status = FAILURE;
	int i = 0;

	if(NULL != line && NULL != names)
	{
		for(i = 0; i < names->count; i++)
		{
			if(SUCCESS == is_function_declaration(line, names->names[i]))
			{
				status = SUCCESS;
				break;
			}
		}
	}

	return(status);
}

/**
 * @brief Process all files in directory
 *
 * @param[in] dir_path Path to directory
 * @param[in] names    Container with function names
 *
 * @return Return status code
 */
static Return process_directory(
	const char*		dir_path,
	const FunctionNames*	names
){
	Return status = SUCCESS;
	DIR* dir = NULL;
	struct dirent* entry = NULL;

	if(NULL == dir_path || NULL == names)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		dir = opendir(dir_path);
		if(NULL == dir)
		{
			fprintf(stderr, "Error: Cannot open directory %s\n", dir_path);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		while((entry = readdir(dir)) != NULL)
		{
			status = process_directory_entry(dir_path, entry, names);
			if(SUCCESS != status && DONOTHING != status)
			{
				fprintf(stderr, "Error processing %s\n", entry->d_name);
				break;
			}
		}
	}

	if(NULL != dir)
	{
		closedir(dir);
	}

	return(status);
}

int main(
	int	argc,
	char**	argv
){
	Return status = SUCCESS;
	FunctionNames names = {NULL, 0};

	if(argc < 3)
	{
		fprintf(stderr, "Usage: %s <source_directory> <function_name1> [function_name2 ...]\n", argv[0]);
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = init_function_names(argc, argv, &names);
	}

	if(SUCCESS == status)
	{
		status = process_directory(argv[1], &names);
		if(SUCCESS != status)
		{
			fprintf(stderr, "Failed to process directory\n");
		}
	}

	free_function_names(&names);

	return(status);
}
