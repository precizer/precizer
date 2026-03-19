#include "precizer.h"

/**
 * @brief Decide whether to protect a relative path from checksum recalculation
 *
 * Compares the path against PCRE2 patterns supplied via --lock-checksum=
 *
 * @param[in] relative_path Relative path to test
 * @return LOCK_CHECKSUM if matched, DO_NOT_LOCK_CHECKSUM if not,
 *         FAIL_REGEXP_LOCK_CHECKSUM on PCRE2 error
 */
LockChecksum match_checksum_lock_pattern(
	const char *relative_path)
{
	if(config->lock_checksum_pcre_compiled == NULL)
	{
		// Nothing to lock
		return(DO_NOT_LOCK_CHECKSUM);
	}

	for(int i = 0; config->lock_checksum_pcre_compiled[i] != NULL; ++i)
	{
		REGEXP result = match_regexp(config->lock_checksum_pcre_compiled[i],relative_path);

		if(MATCH == result)
		{
			// Lock that file checksum from recalculation
			return(LOCK_CHECKSUM);

		} else if(REGEXP_ERROR == result){

			return(FAIL_REGEXP_LOCK_CHECKSUM);

		}
	}

	// No regexp matched
	return(DO_NOT_LOCK_CHECKSUM);
}
