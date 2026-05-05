#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture the bounded-string negative case with a non-divisible source size
 */
static void capture_libmem_invalid_bounded_string_size(void)
{
	INITTEST;

	m_create(uint32_t,code_units);

	const uint32_t initial_code_units[] = {
		UINT32_C(0x00010000),
		UINT32_C(0x00000000)
	};
	const unsigned char invalid_suffix[] = {0x11,0x22,0x33,0x44,0x55};

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(initial_code_units),initial_code_units));
	ASSERT(SUCCESS == m_to_string(code_units));
	ASSERT(FAILURE == m_concat_string(code_units,sizeof(invalid_suffix),invalid_suffix));
	ASSERT(SUCCESS == m_del(code_units));

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check bounded string concat rejects source sizes that do not fit whole elements
 *
 * @return Return describing success or failure
 */
Return test_libmem_0028(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	Return capture_status = SUCCESS;

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_invalid_bounded_string_size,
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
		ASSERT(strstr(captured_report,"not divisible by element size") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
