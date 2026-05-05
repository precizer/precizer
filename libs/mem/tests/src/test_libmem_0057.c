#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture the negative resize case for a data descriptor with stale string metadata
 */
static void capture_libmem_inconsistent_data_resize_cache(void)
{
	INITTEST;

	unsigned char materialized_bytes[] = {1u,2u,3u,4u};
	memory invalid_descriptor = m_init(unsigned char,MEMORY_DATA);

	invalid_descriptor.data = materialized_bytes;
	invalid_descriptor.length = sizeof(materialized_bytes);
	invalid_descriptor.actually_allocated_bytes = sizeof(materialized_bytes);
	invalid_descriptor.string_length = 2;
	invalid_descriptor.is_string = false;

	ASSERT(FAILURE == m_resize(&invalid_descriptor,sizeof(materialized_bytes) + 4u));
	ASSERT(invalid_descriptor.data == materialized_bytes);
	ASSERT(invalid_descriptor.length == sizeof(materialized_bytes));
	ASSERT(invalid_descriptor.actually_allocated_bytes == sizeof(materialized_bytes));
	ASSERT(invalid_descriptor.string_length == 2);
	ASSERT(invalid_descriptor.is_string == false);

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check data-mode resize rejects stale string metadata
 *
 * @return Return describing success or failure
 */
Return test_libmem_0057(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_inconsistent_data_resize_cache,
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
		ASSERT(strstr(captured_report,"Data descriptor has non-zero string_length during resize") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
