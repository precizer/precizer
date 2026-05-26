#include "test_libmem_utils.h"

/**
 * @brief Capture the m_copy_data divisibility negative case
 *
 * The helper is expected to reject source payloads whose byte size cannot be
 * represented as a whole number of destination elements
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_core_data_non_divisible_source(void)
{
	INITTEST;

	m_create(unsigned short,destination);
	m_create(unsigned char,source);

	const unsigned char odd_source_bytes[] = {
		(unsigned char)'a',
		(unsigned char)'b',
		(unsigned char)'c'
	};

	ASSERT(SUCCESS == m_copy_buffer(source,sizeof(odd_source_bytes),odd_source_bytes));
	ASSERT(FAILURE == m_copy_data(destination,source));
	ASSERT(destination->length == 0);
	ASSERT(destination->string_length == 0);
	ASSERT(destination->is_string == false);

	call(m_del(destination));
	call(m_del(source));

	deliver(status);
}

/**
 * @brief Check that m_copy_data rejects source byte counts with destination tails
 *
 * @return Return describing success or failure
 */
Return test_libmem_0060(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0060[] =
	        "\\A.*Source byte count 3 is not divisible by destination element size 2.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0060,
		capture_libmem_core_data_non_divisible_source));

	RETURN_STATUS;
}
