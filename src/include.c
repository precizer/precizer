#include "precizer.h"

/**
 *
 * Decide whether or not to include previously ignored relative
 * path to the file by comparing it with PCRE2 regular
 * expressions passed as arguments with --include=
 *
 */
Include include
(
	const char *relative_path,
	bool *include_showed_once
){
	if(config->include == NULL)
	{
		// Nothing to include
		return(DO_NOT_INCLUDE);
	}

	for(int i = 0; config->include[i] != NULL; ++i)
	{
		REGEXP result = regexp_match(config->include[i],relative_path,include_showed_once);

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
