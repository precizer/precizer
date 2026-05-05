#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture the zero-sized string-length negative case
 */
static void capture_libmem_zero_sized_element_string_length(void)
{
	INITTEST;

	size_t measured_length = SIZE_MAX;
	memory invalid_descriptor = m_init(unsigned char,MEMORY_DATA);

	invalid_descriptor.single_element_size = 0;

	invalid_descriptor.length = 1;

	ASSERT(FAILURE == m_string_length(&invalid_descriptor,&measured_length));
	ASSERT(measured_length == SIZE_MAX);

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check string_length rejects zero-sized elements
 *
 * @return Return describing success or failure
 */
Return test_libmem_0024(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_zero_sized_element_string_length,
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
		ASSERT(strstr(captured_report,"Descriptor element size is zero") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
