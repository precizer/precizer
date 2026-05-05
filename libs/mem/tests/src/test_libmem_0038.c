#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture the negative resize case for a descriptor with a broken string cache
 */
static void capture_libmem_inconsistent_resize_string_cache(void)
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

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check resize rejection for descriptors with an inconsistent cached string length
 *
 * @return Return describing success or failure
 */
Return test_libmem_0038(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_inconsistent_resize_string_cache,
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
		ASSERT(strstr(captured_report,"String descriptor cache is inconsistent during resize") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
