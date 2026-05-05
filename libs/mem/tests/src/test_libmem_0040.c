#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture negative already-string cases for data-to-string conversion
 */
static void capture_libmem_inconsistent_data_to_string_string_mode(void)
{
	INITTEST;

	memory zero_sized_string = m_init(unsigned char,MEMORY_STRING);
	memory zero_length_cached_string = m_init(char,MEMORY_STRING);
	unsigned char materialized_string[] = {'a','b','\0'};
	memory invalid_string_descriptor = m_init(unsigned char,MEMORY_STRING);

	zero_sized_string.single_element_size = 0;
	zero_length_cached_string.string_length = 1;

	invalid_string_descriptor.data = materialized_string;
	invalid_string_descriptor.length = sizeof(materialized_string);
	invalid_string_descriptor.actually_allocated_bytes = sizeof(materialized_string);
	invalid_string_descriptor.string_length = sizeof(materialized_string);
	invalid_string_descriptor.is_string = true;

	ASSERT(FAILURE == m_to_string(&zero_sized_string));
	ASSERT(zero_sized_string.length == 0);
	ASSERT(zero_sized_string.string_length == 0);
	ASSERT(zero_sized_string.is_string == true);

	ASSERT(FAILURE == m_to_string(&zero_length_cached_string));
	ASSERT(zero_length_cached_string.length == 0);
	ASSERT(zero_length_cached_string.string_length == 1);
	ASSERT(zero_length_cached_string.is_string == true);

	ASSERT(FAILURE == m_to_string(&invalid_string_descriptor));
	ASSERT(invalid_string_descriptor.data == materialized_string);
	ASSERT(invalid_string_descriptor.length == sizeof(materialized_string));
	ASSERT(invalid_string_descriptor.actually_allocated_bytes == sizeof(materialized_string));
	ASSERT(invalid_string_descriptor.string_length == sizeof(materialized_string));
	ASSERT(invalid_string_descriptor.is_string == true);

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check data-to-string conversion rejection for inconsistent already-string metadata
 *
 * @return Return describing success or failure
 */
Return test_libmem_0040(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_inconsistent_data_to_string_string_mode,
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
		ASSERT(strstr(captured_report,"Descriptor element size is zero during string conversion") != NULL);
		ASSERT(strstr(captured_report,"String descriptor has non-zero string_length with zero length during string conversion") != NULL);
		ASSERT(strstr(captured_report,"String descriptor cache is inconsistent during string conversion") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
