#include "precizer.h"

/**
 * @brief Decide whether to ignore a relative path
 *
 * Compares the path against PCRE2 patterns supplied via --ignore=
 *
 * @param[in] relative_path Relative path to test
 * @return IGNORE if matched, DO_NOT_IGNORE if not,
 *         FAIL_REGEXP_IGNORE on PCRE2 error
 */
Ignore match_ignore_pattern(
	const char *relative_path)
{
	if(config->ignore_pcre_compiled == NULL)
	{
		// Nothing to ignore
		return(DO_NOT_IGNORE);
	}

	for(int i = 0; config->ignore_pcre_compiled[i] != NULL; ++i)
	{
		REGEXP result = match_regexp(config->ignore_pcre_compiled[i],relative_path);

		if(MATCH == result)
		{
			// Ignore that file
			return(IGNORE);

		} else if(REGEXP_ERROR == result){

			return(FAIL_REGEXP_IGNORE);

		}
	}

	// No --ignore pattern matched
	return(DO_NOT_IGNORE);
}
