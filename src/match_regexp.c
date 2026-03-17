#include "precizer.h"

/**
 * @brief Match a relative path against a pre-compiled PCRE2 pattern
 *
 * @param compiled_pattern Pre-compiled PCRE2 pattern (must not be NULL)
 * @param relative_path    Path string to match against the pattern.
 * @param showed_once      Set to true on the first match-data allocation
 *                         failure so that the error is printed only once.
 * @return MATCH, NOT_MATCH, or REGEXP_ERROR
 */
REGEXP match_regexp(
	pcre2_code *compiled_pattern,
	const char *relative_path,
	bool       *showed_once)
{
	if(compiled_pattern == NULL || relative_path == NULL)
	{
		return(REGEXP_ERROR);
	}

	const unsigned char *path_as_pcre2_subject = (const unsigned char *)relative_path;
	size_t path_length = strlen(relative_path);
	uint32_t match_options = 0;

	pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(compiled_pattern,NULL);

	if(match_data == NULL)
	{
		if(*showed_once == false)
		{
			*showed_once = true;
			slog(ERROR,"PCRE2 failed to allocate match data for path: %s\n",relative_path);
		}
		return(REGEXP_ERROR);
	}

	int match_result = pcre2_match(compiled_pattern,path_as_pcre2_subject,path_length,0,match_options,match_data,NULL);

	pcre2_match_data_free(match_data);

	if(match_result > 0)
	{
		return(MATCH);
	}

	if(match_result == PCRE2_ERROR_NOMATCH)
	{
		return(NOT_MATCH);
	}

	/* match_result == 0: ovector too small (should not happen with create_from_pattern)
	   match_result < 0 and not NOMATCH: other PCRE2 error */
	if(*showed_once == false)
	{
		*showed_once = true;
		PCRE2_UCHAR8 error_message_buffer[MAX_CHARACTERS];
		pcre2_get_error_message(match_result,error_message_buffer,MAX_CHARACTERS);
		slog(ERROR,"PCRE2 match error %d: %s for path: %s\n",match_result,error_message_buffer,relative_path);
	}

	return(REGEXP_ERROR);
}
