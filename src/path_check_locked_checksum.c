#include "precizer.h"

/**
 * @brief Check whether a relative path is protected by --lock-checksum
 *
 * @param[in] relative_path Relative path descriptor to test. Must not be NULL
 *
 * @return Return status code:
 *         - SUCCESS|YES: The path is checksum-locked
 *         - SUCCESS|NO: The path is not checksum-locked
 *         - FAILURE|NO: Lock-checksum regexp evaluation failed
 */
Return path_check_locked_checksum(const memory *relative_path)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	LockChecksum lock_checksum_response = match_checksum_lock_pattern(relative_path);

	if(FAIL_REGEXP_LOCK_CHECKSUM == lock_checksum_response)
	{
		slog(ERROR,"Fail lock-checksum REGEXP for a string: %s\n",m_text(relative_path));
		status = FAILURE | NO;

	} else if(LOCK_CHECKSUM == lock_checksum_response){
		status |= YES;

	} else {
		status |= NO;
	}

	provide(status);
}
