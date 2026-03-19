#include "precizer.h"

/**
 * @brief Apply the shared --include/--ignore decision logic to one relative path
 *
 * Checks --include first so explicitly included paths stay visible even when they
 * also match --ignore. If a regexp evaluation fails, reports the error and
 * returns FAILURE
 *
 * @param[in] relative_path Relative path to test
 * @param[out] include True when the path matched --include. May be NULL when the caller
 *             does not need to distinguish explicitly included paths from default-visible ones
 * @param[out] ignore True when the path matched --ignore without being restored by --include
 * @return SUCCESS on a valid decision, otherwise FAILURE
 */
Return match_include_ignore(
	const char *relative_path,
	bool       *include,
	bool       *ignore)
{
	// include is optional: callers that only need ignore-or-show pass NULL
	if(include != NULL)
	{
		*include = false;
	}

	*ignore = false;

	Include match_include_response = match_include_pattern(relative_path);

	if(DO_NOT_INCLUDE == match_include_response)
	{
		Ignore match_ignore_response = match_ignore_pattern(relative_path);

		if(IGNORE == match_ignore_response)
		{
			*ignore = true;

		} else if(FAIL_REGEXP_IGNORE == match_ignore_response){

			slog(ERROR,"Fail ignore REGEXP for a string: %s\n",relative_path);
			provide(FAILURE);
		}

	} else if(FAIL_REGEXP_INCLUDE == match_include_response){

		slog(ERROR,"Fail include REGEXP for a string: %s\n",relative_path);
		provide(FAILURE);

	} else if(INCLUDE == match_include_response){

		// Skipped when caller passed NULL: the path stays visible, distinction is just not reported
		if(include != NULL)
		{
			*include = true;
		}
	}

	provide(SUCCESS);
}
