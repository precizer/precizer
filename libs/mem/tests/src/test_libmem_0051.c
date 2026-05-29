#include "test_libmem_all.h"

/**
 * @brief Capture m_finalize_string rejection for data-mode descriptors
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_invalid_finalize_string_write_mode(void)
{
	INITTEST;

	m_create(char,data_buffer);

	const char draft[] = "draft";

	ASSERT(SUCCESS == m_resize(data_buffer,sizeof(draft)));
	ASSERT(FAILURE == m_finalize_string(data_buffer,strlen(draft),WRITE_TERMINATOR_ALWAYS));
	call(m_del(data_buffer));

	deliver(status);
}

/**
 * @brief Check m_finalize_string with explicit terminator write and mode rejection
 *
 * @return Return describing success or failure
 */
Return test_libmem_0051(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0051[] =
	        "\\A.*String write finalization requires a string descriptor.*\\Z";

	m_create(char,title,MEMORY_STRING);

	const char draft[] = "draft";

	ASSERT(SUCCESS == m_resize(title,sizeof(draft)));

	char *title_view = m_data(char,title);
	ASSERT(title_view != NULL);

	IF(title_view != NULL)
	{
		memcpy(title_view,draft,strlen(draft));
	}

	ASSERT(SUCCESS == m_finalize_string(title,strlen(draft),WRITE_TERMINATOR_ALWAYS));
	ASSERT(title->length == sizeof(draft));
	ASSERT(title->string_length == strlen("draft"));
	ASSERT(title->is_string == true);
	ASSERT(0 == strcmp(m_text(title),"draft"));

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0051,capture_libmem_invalid_finalize_string_write_mode));

	call(m_del(title));

	RETURN_STATUS;
}
