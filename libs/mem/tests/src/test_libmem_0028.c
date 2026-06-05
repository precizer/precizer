#include "test_libmem_all.h"

/**
 * @brief Capture the bounded-string negative case with a non-divisible source size
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_invalid_bounded_string_size(void)
{
	INITTEST;

	m_create(uint32_t,code_units);

	const uint32_t initial_code_units[] = {
		UINT32_C(0x00010000),
		UINT32_C(0x00000000)
	};
	const unsigned char invalid_suffix[] = {0x11,0x22,0x33,0x44,0x55};

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(initial_code_units),initial_code_units));
	ASSERT(SUCCESS == m_to_string(code_units));
	ASSERT(FAILURE == m_concat_string(code_units,sizeof(invalid_suffix),invalid_suffix));
	call(m_del(code_units));

	deliver(status);
}

/**
 * @brief Check bounded string concat rejects source sizes that do not fit whole elements
 *
 * @return Return describing success or failure
 */
Return test_libmem_0028(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0028[] =
	        "\\A.*not divisible by element size.*\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0028,capture_libmem_invalid_bounded_string_size));

	RETURN_STATUS;
}
