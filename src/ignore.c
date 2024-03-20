#include "precizer.h"

Ignore ignore
(
	const char *relative_path,
	bool *ignore_showed_once
){
	if(config->ignore == NULL)
	{
		// Nothing to ignore
		return(DO_NOT_IGNORE);
	}

	for(int i = 0; config->ignore[i] != NULL; ++i)
	{
		REGEXP result = regexp_match(config->ignore[i],relative_path,ignore_showed_once);

		if(MATCH == result)
		{
			// Ignore that file
			return(IGNORE);

		} else if(REGEXP_ERROR == result){

			return(FAIL_REGEXP_IGNORE);

		}
	}

	// Don't ignore the file
	return(DO_NOT_IGNORE);
}
