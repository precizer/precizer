#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture negative cases for descriptors with non-zero length and NULL data
 */
static void capture_libmem_inconsistent_string_descriptor(void)
{
	INITTEST;

	size_t measured_length = SIZE_MAX;
	memory invalid_data_descriptor = m_init(char,MEMORY_DATA);
	memory invalid_string_descriptor = m_init(char,MEMORY_STRING);

	invalid_data_descriptor.length = 3;
	invalid_string_descriptor.length = 3;
	invalid_string_descriptor.string_length = 2;

	ASSERT(FAILURE == m_string_length(&invalid_data_descriptor,&measured_length));
	ASSERT(measured_length == SIZE_MAX);
	ASSERT(FAILURE == m_to_string(&invalid_data_descriptor));
	ASSERT(FAILURE == m_to_data(&invalid_string_descriptor));

	const unsigned char *safe_string_view = (const unsigned char *)m_string(&invalid_string_descriptor);
	ASSERT(safe_string_view != NULL);

	if(safe_string_view != NULL)
	{
		ASSERT(safe_string_view[0] == 0U);
	}

	ASSERT(invalid_data_descriptor.length == 3);
	ASSERT(invalid_data_descriptor.data == NULL);
	ASSERT(invalid_data_descriptor.is_string == false);
	ASSERT(invalid_string_descriptor.length == 3);
	ASSERT(invalid_string_descriptor.data == NULL);
	ASSERT(invalid_string_descriptor.is_string == true);

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check string-facing helpers reject inconsistent descriptors
 *
 * @return Return describing success or failure
 */
Return test_libmem_0029(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_inconsistent_string_descriptor,
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
		ASSERT(strstr(captured_report,"Descriptor has non-zero length with NULL data pointer") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
