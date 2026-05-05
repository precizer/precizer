#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture string truncate rejection for descriptors whose reserve does not cover the payload
 */
static void capture_libmem_inconsistent_string_truncate_reserve(void)
{
	INITTEST;

	char materialized_same_length[] = "alpha";
	char materialized_growth_noop[] = "omega";
	memory invalid_same_length = m_init(char,MEMORY_STRING);
	memory invalid_growth_noop = m_init(char,MEMORY_STRING);

	invalid_same_length.data = materialized_same_length;
	invalid_same_length.length = sizeof(materialized_same_length);
	invalid_same_length.actually_allocated_bytes = sizeof(materialized_same_length) - 1u;
	invalid_same_length.string_length = sizeof(materialized_same_length) - 1u;
	invalid_same_length.is_string = true;

	invalid_growth_noop.data = materialized_growth_noop;
	invalid_growth_noop.length = sizeof(materialized_growth_noop);
	invalid_growth_noop.actually_allocated_bytes = sizeof(materialized_growth_noop) - 1u;
	invalid_growth_noop.string_length = sizeof(materialized_growth_noop) - 1u;
	invalid_growth_noop.is_string = true;

	ASSERT(FAILURE == mem_string_truncate(&invalid_same_length,invalid_same_length.string_length));
	ASSERT(invalid_same_length.data == materialized_same_length);
	ASSERT(invalid_same_length.length == sizeof(materialized_same_length));
	ASSERT(invalid_same_length.actually_allocated_bytes == sizeof(materialized_same_length) - 1u);
	ASSERT(invalid_same_length.string_length == sizeof(materialized_same_length) - 1u);
	ASSERT(invalid_same_length.is_string == true);

	ASSERT(FAILURE == mem_string_truncate(&invalid_growth_noop,invalid_growth_noop.string_length + 1u));
	ASSERT(invalid_growth_noop.data == materialized_growth_noop);
	ASSERT(invalid_growth_noop.length == sizeof(materialized_growth_noop));
	ASSERT(invalid_growth_noop.actually_allocated_bytes == sizeof(materialized_growth_noop) - 1u);
	ASSERT(invalid_growth_noop.string_length == sizeof(materialized_growth_noop) - 1u);
	ASSERT(invalid_growth_noop.is_string == true);

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check string truncate rejection for descriptors whose reserve does not cover the payload
 *
 * @return Return describing success or failure
 */
Return test_libmem_0042(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_inconsistent_string_truncate_reserve,
		captured_stdout,
		captured_stderr);

	if(capture_status != SUCCESS)
	{
		captured_status = capture_status;
		captured_failed_line = __LINE__;
	}

	ASSERT(SUCCESS == capture_status);
	ASSERT(captured_stdout->length == 0);
	ASSERT(captured_stderr->length > 0);

	const char *captured_report = m_text(captured_stderr);
	ASSERT(captured_report != NULL);

	if(captured_report != NULL)
	{
		ASSERT(strstr(captured_report,"Descriptor reserve is smaller than logical payload during string truncate") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
