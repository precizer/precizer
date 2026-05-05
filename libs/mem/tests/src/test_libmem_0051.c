#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture mem_finalize_string rejection for data-mode descriptors
 */
static void capture_libmem_invalid_finalize_string_write_mode(void)
{
	INITTEST;

	m_create(char,data_buffer);

	const char draft[] = "draft";

	ASSERT(SUCCESS == m_resize(data_buffer,sizeof(draft)));
	ASSERT(FAILURE == mem_finalize_string(data_buffer,strlen(draft),WRITE_TERMINATOR_ALWAYS));
	ASSERT(SUCCESS == m_del(data_buffer));

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check mem_finalize_string with explicit terminator write and mode rejection
 *
 * @return Return describing success or failure
 */
Return test_libmem_0051(void)
{
	INITTEST;

	m_create(char,title,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	Return capture_status = SUCCESS;

	const char draft[] = "draft";

	ASSERT(SUCCESS == m_resize(title,sizeof(draft)));

	char *title_view = m_data(char,title);
	ASSERT(title_view != NULL);

	if(title_view != NULL)
	{
		memcpy(title_view,draft,strlen(draft));
	}

	ASSERT(SUCCESS == mem_finalize_string(title,strlen(draft),WRITE_TERMINATOR_ALWAYS));
	ASSERT(title->length == sizeof(draft));
	ASSERT(title->string_length == strlen("draft"));
	ASSERT(title->is_string == true);
	ASSERT(0 == strcmp(m_text(title),"draft"));

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_invalid_finalize_string_write_mode,
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
		ASSERT(strstr(captured_report,"String write finalization requires a string descriptor") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	ASSERT(SUCCESS == m_del(title));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
