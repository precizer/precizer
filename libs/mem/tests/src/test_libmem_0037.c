#include "test_libmem_utils.h"


static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture the mem_string_truncate negative case for data-mode descriptors
 */
static void capture_libmem_invalid_string_truncate_mode(void)
{
	INITTEST;

	m_create(char,data_buffer);

	ASSERT(SUCCESS == m_copy_buffer(data_buffer,sizeof("raw"),"raw"));
	ASSERT(FAILURE == mem_string_truncate(data_buffer,1));
	ASSERT(SUCCESS == m_del(data_buffer));

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check multi-byte truncation and reject data-mode descriptors
 *
 * @return Return describing success or failure
 */
Return test_libmem_0037(void)
{
	INITTEST;

	m_create(uint32_t,code_units);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	Return capture_status = SUCCESS;

	const uint32_t source_units[] = {
		UINT32_C(10),
		UINT32_C(20),
		UINT32_C(30),
		UINT32_C(40),
		UINT32_C(0)
	};

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(source_units),source_units));
	ASSERT(SUCCESS == m_to_string(code_units));

	const size_t original_length = code_units->length;

	ASSERT(SUCCESS == mem_string_truncate(code_units,2));
	ASSERT(code_units->length == original_length);
	ASSERT(code_units->string_length == 2);
	ASSERT(code_units->is_string == true);

	const uint32_t *truncated_view = m_data_ro(uint32_t,code_units);
	ASSERT(truncated_view != NULL);

	if(truncated_view != NULL)
	{
		ASSERT(truncated_view[0] == UINT32_C(10));
		ASSERT(truncated_view[1] == UINT32_C(20));
		ASSERT(truncated_view[2] == UINT32_C(0));
	}

	ASSERT(SUCCESS == mem_string_truncate(code_units,8));
	ASSERT(code_units->string_length == 2);

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_libmem_invalid_string_truncate_mode,
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
		ASSERT(strstr(captured_report,"string truncate requires a string descriptor") != NULL);
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	ASSERT(SUCCESS == m_del(code_units));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
