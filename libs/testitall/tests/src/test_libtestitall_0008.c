#include "test_libtestitall_all.h"

/**
 * @brief Verify direct small-string diff generation through compare_memory_strings
 *
 * This test prepares two short multi-line strings in string-mode memory descriptors
 * and compares them through compare_memory_strings(). The produced unified diff
 * is checked against the exact expected text for this small input pair
 *
 * The test verifies that compare_memory_strings():
 * - accepts byte-sized string descriptors as input
 * - stores the produced diff into the destination memory descriptor
 * - produces the expected compact unified diff for a simple one-line replacement
 *
 * @return Return status code
 */
Return test_libtestitall_0008(void)
{
	INITTEST;

	const char expected_diff[] = "-line2\n+line3\n";

	m_create(char,left_text_buffer,MEMORY_STRING);
	m_create(char,right_text_buffer,MEMORY_STRING);
	m_create(char,actual_diff_buffer,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(left_text_buffer,"line1\nline2\n"));
	ASSERT(SUCCESS == m_copy_literal(right_text_buffer,"line1\nline3\n"));

	ASSERT(SUCCESS == compare_memory_strings(
		actual_diff_buffer,
		left_text_buffer,
		right_text_buffer));

	ASSERT(0 == strcmp(expected_diff,m_text(actual_diff_buffer)));

	m_del(actual_diff_buffer);
	m_del(right_text_buffer);
	m_del(left_text_buffer);

	RETURN_STATUS;
}
