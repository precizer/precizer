#include "precizer.h"

/**
 *
 * Decide whether or not to include previously ignored relative
 * path to the file by comparing it with PCRE2 regular
 * expressions passed as arguments with --include=
 *
 */
Include match_include_pattern(
	const char *relative_path,
	bool       *include_showed_once)
{
	if(config->include_pcre_compiled == NULL)
	{
		// Nothing to include
		return(DO_NOT_INCLUDE);
	}

	for(int i = 0; config->include_pcre_compiled[i] != NULL; ++i)
	{
		REGEXP result = match_regexp(config->include_pcre_compiled[i],relative_path,include_showed_once);

		if(MATCH == result)
		{
			// Include that file
			return(INCLUDE);

		} else if(REGEXP_ERROR == result){

			return(FAIL_REGEXP_INCLUDE);

		}
	}

	// Don't ignore the file
	return(DO_NOT_INCLUDE);
}
