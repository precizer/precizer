#include "precizer.h"

/**
 * @brief Apply the shared --include and --ignore decision logic to one path
 *
 * Applies --include first, then --ignore, so explicitly included paths stay visible
 *
 * @param[in] relative_path Relative path descriptor to test
 * @param[out] include True when the path matched --include. May be NULL when the caller
 *             only needs the final visibility decision
 * @param[out] ignore True when the path matched --ignore without being restored by --include
 * @return SUCCESS on a valid decision, otherwise FAILURE
 */
Return match_include_ignore(
	const memory *relative_path,
	bool       *include,
	bool       *ignore)
{
	const char *runtime_relative_path = m_text(relative_path);

	// Callers may omit include when only the final visibility decision is needed
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

			slog(ERROR,"Fail ignore REGEXP for a string: %s\n",runtime_relative_path);
			provide(FAILURE);
		}

	} else if(FAIL_REGEXP_INCLUDE == match_include_response){

		slog(ERROR,"Fail include REGEXP for a string: %s\n",runtime_relative_path);
		provide(FAILURE);

	} else if(INCLUDE == match_include_response){

		// Keep the path visible even when the include flag is not requested
		if(include != NULL)
		{
			*include = true;
		}
	}

	provide(SUCCESS);
}
