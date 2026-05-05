#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture the internal bounded-source negative case past the visible terminator
 */
static void capture_libmem_invalid_internal_bounded_source(void)
{
	INITTEST;

	static const char base_text[] = "base";

	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(base_text),base_text));
	ASSERT(SUCCESS == m_resize(string_buffer,string_buffer->length + 2,ZERO_NEW_MEMORY));

	const char *invalid_internal_source = m_text(string_buffer) + string_buffer->string_length + 1;

	ASSERT(invalid_internal_source != NULL);

	if(invalid_internal_source != NULL)
	{
		ASSERT(FAILURE == m_concat_string(
			string_buffer,
			1,
			invalid_internal_source));
	}

	ASSERT(SUCCESS == m_del(string_buffer));

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check internal bounded mode still rejects starts past the visible terminator
 *
 * @return Return describing success or failure
 */
Return test_libmem_0035(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_invalid_internal_bounded_source,
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
		ASSERT(strstr(captured_report,"Source start exceeds destination visible string bounds") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
