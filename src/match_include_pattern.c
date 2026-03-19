#include "precizer.h"

/**
 * @brief Decide whether to explicitly include a relative path
 *
 * Compares the path against PCRE2 patterns supplied via --include=
 * A match overrides any --ignore pattern for the same path
 *
 * @param[in] relative_path Relative path to test
 * @return INCLUDE if matched, DO_NOT_INCLUDE if not,
 *         FAIL_REGEXP_INCLUDE on PCRE2 error
 */
Include match_include_pattern(
	const char *relative_path)
{
	if(config->include_pcre_compiled == NULL)
	{
		// Nothing to include
		return(DO_NOT_INCLUDE);
	}

	for(int i = 0; config->include_pcre_compiled[i] != NULL; ++i)
	{
		REGEXP result = match_regexp(config->include_pcre_compiled[i],relative_path);

		if(MATCH == result)
		{
			// Include that file
			return(INCLUDE);

		} else if(REGEXP_ERROR == result){

			return(FAIL_REGEXP_INCLUDE);

		}
	}

	// No --include pattern matched
	return(DO_NOT_INCLUDE);
}
