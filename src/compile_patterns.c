#include "precizer.h"

/**
 * @brief Attempt optional JIT compilation for a compiled PCRE2 pattern
 *
 * JIT is a performance optimization only. Its absence must not prevent the
 * pattern from being used through the normal interpretive matcher.
 *
 * @param[in] compiled_pattern Successfully compiled PCRE2 pattern
 * @param[in] pattern_text     Original pattern text used for logging
 */
static void try_jit_compile(
	pcre2_code *compiled_pattern,
	const char *pattern_text)
{
	if(compiled_pattern == NULL || pattern_text == NULL)
	{
		return;
	}

	int jit_status = pcre2_jit_compile(compiled_pattern,PCRE2_JIT_COMPLETE);

	if(
		jit_status == 0 ||
		jit_status == PCRE2_ERROR_JIT_BADOPTION ||
		jit_status == PCRE2_ERROR_NOMEMORY)
	{
		return;
	}

	PCRE2_UCHAR8 error_message_buffer[MAX_CHARACTERS];
	pcre2_get_error_message(jit_status,error_message_buffer,MAX_CHARACTERS);
	slog(ERROR,"PCRE2 JIT compilation error %d for pattern \"%s\": %s\n",jit_status,pattern_text,error_message_buffer);
}

/**
 * @brief Compile one string array of PCRE2 pattern strings into a pcre2_code array
 *
 * Allocates a NULL-terminated array of pcre2_code pointers parallel to @p patterns.
 * On the first compilation error the function logs the message and returns FAILURE;
 * any patterns compiled before that point are freed before returning.
 *
 * @param[in]  patterns  NULL-terminated array of pattern strings to compile.
 *                       If NULL the function is a no-op and returns SUCCESS.
 * @param[out] compiled  Receives the allocated pcre2_code pointer array on SUCCESS.
 *                       Set to NULL on FAILURE.
 * @return SUCCESS or FAILURE
 */
static Return compile_one_array(
	char       **patterns,
	pcre2_code ***compiled)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	*compiled = NULL;

	if(patterns == NULL)
	{
		provide(status);
	}

	/* Count patterns */
	int pattern_count = 0;

	while(patterns[pattern_count] != NULL)
	{
		pattern_count++;
	}

	/* Allocate pointer array: pattern_count entries + NULL terminator */
	pcre2_code **compiled_patterns = (pcre2_code **)calloc((size_t)(pattern_count + 1),sizeof(pcre2_code *));

	if(compiled_patterns == NULL)
	{
		report("Memory allocation failed for compiled pattern array");
		provide(FAILURE);
	}

	/* Compile each pattern */
	for(int i = 0; i < pattern_count; i++)
	{
		PCRE2_SIZE compile_error_offset;
		int compile_error_code;

		compiled_patterns[i] = pcre2_compile(
			(const unsigned char *)patterns[i],
			strlen(patterns[i]),
			0,
			&compile_error_code,
			&compile_error_offset,
			NULL);

		if(compiled_patterns[i] == NULL)
		{
			PCRE2_UCHAR8 error_message_buffer[MAX_CHARACTERS];
			pcre2_get_error_message(compile_error_code,error_message_buffer,MAX_CHARACTERS);
			slog(ERROR,"PCRE2 failed to compile pattern \"%s\" at offset %zu: %s\n",patterns[i],(size_t)compile_error_offset,error_message_buffer);

			free_compiled_array(&compiled_patterns);
			provide(FAILURE);
		}

		try_jit_compile(compiled_patterns[i],patterns[i]);
	}

	*compiled = compiled_patterns;

	provide(status);
}

/**
 * @brief Compile all PCRE2 pattern strings stored in Config into pcre2_code objects
 *
 * Processes the ignore, include and lock_checksum string arrays.
 * Each non-NULL array gets a matching _pcre_compiled array stored in Config.
 * Called once from main() after parse_arguments() and before file traversal.
 *
 * @return SUCCESS or FAILURE
 */
Return compile_patterns(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	run(compile_one_array(config->ignore,&config->ignore_pcre_compiled));
	run(compile_one_array(config->include,&config->include_pcre_compiled));
	run(compile_one_array(config->lock_checksum,&config->lock_checksum_pcre_compiled));

	provide(status);
}
