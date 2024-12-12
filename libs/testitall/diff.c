/**
 * @file diff.c
 * @brief Implementation of git-style diff algorithm with line numbers
 *
 * This file contains implementation of text comparison functionality
 * similar to git diff command, with line numbers output
 */

#include "testitall.h"

/**
 * @brief Structure to store line information
 */
typedef struct
{
	char	*content;
	size_t	length;
} Line;

/**
 * @brief Structure to store a diff chunk information
 */
typedef struct
{
	size_t	source_start;
	size_t	source_count;
	size_t	compare_start;
	size_t	compare_count;
} DiffChunk;

/**
 * @brief Reallocates buffer and appends string to it
 *
 * @param[in,out] buffer Pointer to buffer to append to
 * @param[in] str String to append
 * @param[in,out] size Current buffer size
 * @return Return status code
 */
static Return append_to_buffer(
	char		**buffer,
	const char	*str,
	size_t		*size
){
	Return status = SUCCESS;
	size_t str_len;
	size_t new_size;
	char *new_buffer;

	if(buffer == NULL || str == NULL || size == NULL)
	{
		return FAILURE;
	}

	str_len = strlen(str);
	new_size = (*buffer == NULL) ? str_len + 1 : *size + str_len;
	new_buffer = (char *)realloc(*buffer, new_size);

	if(new_buffer == NULL)
	{
		return FAILURE;
	}

	if(*buffer == NULL)
	{
		strcpy(new_buffer, str);
	}
	else
	{
		strcat(new_buffer, str);
	}

	*buffer = new_buffer;
	*size = new_size;

	return status;
}

/**
 * @brief Writes chunk header to buffer in git style
 *
 * @param[in,out] buffer Buffer to write to
 * @param[in] chunk Diff chunk information
 * @param[in,out] size Current buffer size
 * @return Return status code
 */
static Return write_chunk_header(
	char			**buffer,
	const DiffChunk	*chunk,
	size_t			*size
){
	Return status = SUCCESS;
	char header[100];  /* Buffer for header string */

	snprintf(header, sizeof(header), "@@ -%zu,%zu +%zu,%zu @@\n",
		chunk->source_start + 1,
		chunk->source_count,
		chunk->compare_start + 1,
		chunk->compare_count
	);

	status = append_to_buffer(buffer, header, size);

	return status;
}

/**
 * @brief Checks if text matches PCRE2 pattern
 *
 * @param[in] text Text to check
 * @param[in] pattern PCRE2 pattern
 * @return Return status code (SUCCESS if matches, FAILURE if not or error)
 */
static Return check_pattern_match(
	const char	*text,
	const char	*pattern
){
	Return status = FAILURE;
	pcre2_code *re;
	int error_number;
	size_t error_offset;
	pcre2_match_data *match_data;
	int rc;

	if(text == NULL || pattern == NULL)
	{
		return FAILURE;
	}

	/* Compile pattern */
	re = pcre2_compile(
		(PCRE2_SPTR)pattern,
		PCRE2_ZERO_TERMINATED,
		0,
		&error_number,
		&error_offset,
		NULL
	);

	if(re == NULL)
	{
		/* Pattern compilation failed */
		return FAILURE;
	}

	/* Perform matching */
	match_data = pcre2_match_data_create_from_pattern(re, NULL);
	if(match_data != NULL)
	{
		rc = pcre2_match(
			re,
			(PCRE2_SPTR)text,
			strlen(text),
			0,
			0,
			match_data,
			NULL
		);

		if(rc >= 0)
		{
			status = SUCCESS;
		}

		pcre2_match_data_free(match_data);
	}

	pcre2_code_free(re);
	return status;
}

/**
 * @brief Splits text into lines
 *
 * @param[in] text Input null-terminated text
 * @param[out] lines Array of Line structures
 * @param[out] count Number of lines
 * @return Return status code
 */
static Return split_into_lines(
	const char	*text,
	Line		**lines,
	size_t		*count
){
	Return status = SUCCESS;
	size_t text_len;
	size_t line_count = 0;
	const char *start;
	const char *end;

	if(text == NULL || lines == NULL || count == NULL)
	{
		return FAILURE;
	}

	/* Count number of lines */
	text_len = strlen(text);
	line_count = 1;
	for(size_t i = 0; i < text_len; i++)
	{
		if(text[i] == '\n')
		{
			line_count++;
		}
	}

	/* Allocate memory for lines array */
	*lines = (Line *)malloc(sizeof(Line) * line_count);
	if(*lines == NULL)
	{
		return FAILURE;
	}

	/* Split text into lines */
	*count = 0;
	start = text;
	while(*start != '\0')
	{
		end = strchr(start, '\n');
		if(end == NULL)
		{
			end = start + strlen(start);
		}

		size_t line_len = (size_t)(end - start);
		(*lines)[*count].content = (char *)malloc(line_len + 1);

		if((*lines)[*count].content == NULL)
		{
			/* Clean up previously allocated memory */
			for(size_t i = 0; i < *count; i++)
			{
				free((*lines)[i].content);
			}
			free(*lines);
			return FAILURE;
		}

		strncpy((*lines)[*count].content, start, line_len);
		(*lines)[*count].content[line_len] = '\0';
		(*lines)[*count].length = line_len;

		(*count)++;
		start = (*end == '\0') ? end : end + 1;
	}

	return status;
}

/**
 * @brief Frees memory allocated for lines
 *
 * @param[in] lines Pointer to array of Line structures
 * @param[in] count Number of lines
 */
static void free_lines(
	Line	**lines,
	size_t	count
){
	if(NULL == lines || NULL == *lines)
	{
		return;
	}

	for(size_t i = 0; i < count; i++)
	{
		if(NULL != (*lines)[i].content)
		{
			free((*lines)[i].content);
		}
	}

	free(*lines);
	*lines = NULL;
}

/**
 * @brief Compares two texts and writes differences in git-style format to buffer
 * Uses PCRE2 pattern matching for comparison
 *
 * @param[in] source First text for comparison
 * @param[in] compare Second text for comparison
 * @param[out] result Pointer to buffer where result will be stored
 * @return Return status code
 */
Return diff(
	const char	*source,
	const char	*compare,
	char		**result
){
	Return status = SUCCESS;
	Line *source_lines = NULL;
	Line *compare_lines = NULL;
	size_t source_count = 0;
	size_t compare_count = 0;
	DiffChunk current_chunk = {0, 0, 0, 0};
	size_t chunk_start_i = 0;
	size_t chunk_start_j = 0;
	int is_chunk_open = 0;
	size_t buffer_size = 0;
	char line_buffer[1024];  /* Temporary buffer for formatting lines */

	if(source == NULL || compare == NULL || result == NULL)
	{
		return FAILURE;
	}

	*result = NULL;  /* Initialize result buffer */

	/* Split both texts into lines */
	if(SUCCESS == status)
	{
		status = split_into_lines(source, &source_lines, &source_count);
		if(SUCCESS != status)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = split_into_lines(compare, &compare_lines, &compare_count);
		if(SUCCESS != status)
		{
			free_lines(&source_lines, source_count);
			status = FAILURE;
		}
	}

	/* Compare and write differences */
	if(SUCCESS == status)
	{
		size_t i = 0, j = 0;
		while(i < source_count || j < compare_count)
		{
			int has_diff = 0;

			if(i >= source_count)
			{
				/* Remaining lines from compare */
				if(!is_chunk_open)
				{
					chunk_start_i = i;
					chunk_start_j = j;
					is_chunk_open = 1;
				}
				snprintf(line_buffer, sizeof(line_buffer), "+ %s\n",
					compare_lines[j].content);
				status = append_to_buffer(result, line_buffer, &buffer_size);
				j++;
				has_diff = 1;
			}
			else if(j >= compare_count)
			{
				/* Remaining lines from source */
				if(!is_chunk_open)
				{
					chunk_start_i = i;
					chunk_start_j = j;
					is_chunk_open = 1;
				}
				snprintf(line_buffer, sizeof(line_buffer), "- %s\n",
					source_lines[i].content);
				status = append_to_buffer(result, line_buffer, &buffer_size);
				i++;
				has_diff = 1;
			}
			else
			{
				Return match_status = check_pattern_match(
					source_lines[i].content,
					compare_lines[j].content
				);

				if(match_status != SUCCESS)
				{
					/* Lines don't match the pattern */
					if(!is_chunk_open)
					{
						chunk_start_i = i;
						chunk_start_j = j;
						is_chunk_open = 1;
					}
					snprintf(line_buffer, sizeof(line_buffer), "- %s\n",
						source_lines[i].content);
					status = append_to_buffer(result, line_buffer, &buffer_size);

					snprintf(line_buffer, sizeof(line_buffer), "+ %s\n",
						compare_lines[j].content);
					status = append_to_buffer(result, line_buffer, &buffer_size);
					has_diff = 1;
				}
				i++;
				j++;
			}

			if(SUCCESS != status)
			{
				break;
			}

			/* Check if we need to close and write the chunk */
			if(has_diff)
			{
				current_chunk.source_count = i - chunk_start_i;
				current_chunk.compare_count = j - chunk_start_j;
			}
			else if(is_chunk_open)
			{
				/* Write chunk header and reset */
				current_chunk.source_start = chunk_start_i;
				current_chunk.compare_start = chunk_start_j;
				status = write_chunk_header(result, &current_chunk, &buffer_size);
				is_chunk_open = 0;
			}
		}

		/* Write last chunk header if needed */
		if(is_chunk_open && SUCCESS == status)
		{
			current_chunk.source_start = chunk_start_i;
			current_chunk.compare_start = chunk_start_j;
			status = write_chunk_header(result, &current_chunk, &buffer_size);
		}
	}

	/* Cleanup */
	free_lines(&source_lines, source_count);
	free_lines(&compare_lines, compare_count);

	/* Clean up result buffer if error occurred */
	if(SUCCESS != status && *result != NULL)
	{
		free(*result);
		*result = NULL;
	}

	return status;
}

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

/**
 * @brief Status codes for function returns
 */
typedef enum
{
	SUCCESS = 0,
	FAILURE = 1,
	WARNING = 2,
	DONOTHING = 3
} Return;

/**
 * @brief Compares two files and prints differences
 *
 * @param[in] source_file Path to first file
 * @param[in] compare_file Path to second file
 * @return Return status code
 */
Return diff_files(
	const char	*source_file,
	const char	*compare_file
){
	Return status = SUCCESS;
	char *source_content = NULL;
	char *compare_content = NULL;

	if(source_file == NULL || compare_file == NULL)
	{
		return FAILURE;
	}

	/* Read source file */
	if(SUCCESS == status)
	{
		status = read_file(source_file, &source_content);
		if(SUCCESS != status)
		{
			printf("Error: Cannot read source file '%s'\n", source_file);
			status = FAILURE;
		}
	}

	/* Read compare file */
	if(SUCCESS == status)
	{
		status = read_file(compare_file, &compare_content);
		if(SUCCESS != status)
		{
			printf("Error: Cannot read compare file '%s'\n", compare_file);
			free(source_content);
			status = FAILURE;
		}
	}

	/* Compare file contents */
	if(SUCCESS == status)
	{
		status = diff(source_content, compare_content);
	}

	/* Cleanup */
	free(source_content);
	free(compare_content);

	return status;
}

/**
 * @brief Main function demonstrating file diff functionality
 */
int main(
	int		argc,
	char	*argv[]
){
	if(argc != 3)
	{
		printf("Usage: %s <source_file> <compare_file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	Return status = diff_files(argv[1], argv[2]);

	return (status == SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
