#include "test_libmem_utils.h"

/**
 * @brief Capture the negative resize case for a descriptor with a broken string cache
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_resize_string_cache(void)
{
	INITTEST;

	char materialized_string[] = "alpha";
	memory invalid_string = m_init(char,MEMORY_STRING);

	invalid_string.data = materialized_string;
	invalid_string.length = sizeof(materialized_string);
	invalid_string.actually_allocated_bytes = sizeof(materialized_string);
	invalid_string.string_length = sizeof(materialized_string);
	invalid_string.is_string = true;

	ASSERT(FAILURE == m_resize(&invalid_string,sizeof(materialized_string) + 2));
	ASSERT(invalid_string.data == materialized_string);
	ASSERT(invalid_string.length == sizeof(materialized_string));
	ASSERT(invalid_string.string_length == sizeof(materialized_string));
	ASSERT(invalid_string.is_string == true);

	deliver(status);
}

/**
 * @brief Check resize rejection for descriptors with an inconsistent cached string length
 *
 * @return Return describing success or failure
 */
Return test_libmem_0038(void)
{
	INITTEST;

	/* Expected stderr layout for the inconsistent cached-length resize
	   rejection. The source line number and errno wording are intentionally
	   flexible because they are build- and platform-dependent */
	static const char expected_stderr_pattern_libmem_0038[] =
	        "\\A"
	        "ERROR: src/mem_resize\\.c:mem_resize:\\d+ Memory management; String descriptor cache is inconsistent during resize Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	        "\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0038,capture_libmem_inconsistent_resize_string_cache));

	RETURN_STATUS;
}
