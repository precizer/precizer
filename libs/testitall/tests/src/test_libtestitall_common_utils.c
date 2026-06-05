#include "test_libtestitall_all.h"

/**
 * @file test_libtestitall_common_utils.c
 * @brief Common helper functions shared by multiple libtestitall tests
 */

/**
 * @brief Match the shared STDERR buffer against an inline PCRE2 pattern
 *
 * @param[in] pattern_text Inline PCRE2 pattern expected in STDERR
 * @return Return status code
 */
Return assert_stderr_matches_pattern(const char *pattern_text)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,pattern,MEMORY_STRING);

	if(pattern_text == NULL)
	{
		status = FAILURE;
	}

	run(m_copy_string(pattern,pattern_text));
	run(match_pattern(STDERR,pattern,"shared STDERR"));

	if(SUCCESS == status)
	{
		call(m_del(STDERR));
	}

	call(m_del(pattern));

	deliver(status);
}
