#include "precizer.h"

/**
 *
 * Decide whether or not to protect the relative
 * path from checksum recalculation by comparing it
 * with PCRE2 regular expressions passed as arguments
 * with --lock-checksum=
 *
 */
LockChecksum match_checksum_lock_pattern(
	const char *relative_path,
	bool       *lock_checksum_showed_once)
{
	if(config->lock_checksum_pcre_compiled == NULL)
	{
		// Nothing to lock
		return(DO_NOT_LOCK_CHECKSUM);
	}

	for(int i = 0; config->lock_checksum_pcre_compiled[i] != NULL; ++i)
	{
		REGEXP result = match_regexp(config->lock_checksum_pcre_compiled[i],relative_path,lock_checksum_showed_once);

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
