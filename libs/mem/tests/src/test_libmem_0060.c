#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture the mem_copy_data divisibility negative case
 *
 * The helper is expected to reject source payloads whose byte size cannot be
 * represented as a whole number of destination elements
 *
 * @return void
 */
static void capture_libmem_core_data_non_divisible_source(void)
{
	INITTEST;

	m_create(unsigned short,destination);
	m_create(unsigned char,source);

	const unsigned char odd_source_bytes[] = {
		(unsigned char)'a',
		(unsigned char)'b',
		(unsigned char)'c'
	};

	ASSERT(SUCCESS == m_copy_buffer(source,sizeof(odd_source_bytes),odd_source_bytes));
	ASSERT(FAILURE == mem_copy_data(destination,source));
	ASSERT(destination->length == 0);
	ASSERT(destination->string_length == 0);
	ASSERT(destination->is_string == false);

	call(m_del(destination));
	call(m_del(source));

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check that mem_copy_data rejects source byte counts with destination tails
 *
 * @return Return describing success or failure
 */
Return test_libmem_0060(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(capture_libmem_core_data_non_divisible_source,captured_stdout,captured_stderr);

	if(capture_status != SUCCESS)
	{
		captured_status = capture_status;
		captured_failed_line = __LINE__;
	}

	ASSERT(SUCCESS == capture_status);
	ASSERT(captured_stdout->length == 0);

	const char *captured_stderr_view = m_text(captured_stderr);
	ASSERT(captured_stderr_view != NULL);

	if(captured_stderr_view != NULL)
	{
		ASSERT(strstr(
			captured_stderr_view,
			"Source byte count 3 is not divisible by destination element size 2") != NULL);
	}

	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
