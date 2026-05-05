#include "test_libmem_utils.h"

static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

/**
 * @brief Capture invalid descriptor cases for the soft string view helper
 */
static void capture_invalid_mem_string_descriptors(void)
{
	INITTEST;

	memory zero_sized_string = m_init(char,MEMORY_STRING);
	memory data_descriptor = m_init(char,MEMORY_DATA);
	memory missing_data_string = m_init(char,MEMORY_STRING);
	memory invalid_cached_length = m_init(char,MEMORY_STRING);
	char cached_text[] = "ab";

	zero_sized_string.single_element_size = 0;
	missing_data_string.length = 3;
	missing_data_string.string_length = 1;
	invalid_cached_length.data = cached_text;
	invalid_cached_length.length = sizeof(cached_text);
	invalid_cached_length.string_length = invalid_cached_length.length;

	const unsigned char *empty_view = (const unsigned char *)m_string(NULL);
	ASSERT(empty_view != NULL);

	if(empty_view != NULL)
	{
		ASSERT(empty_view[0] == 0U);
	}

	empty_view = (const unsigned char *)m_string(&zero_sized_string);
	ASSERT(empty_view != NULL);

	if(empty_view != NULL)
	{
		ASSERT(empty_view[0] == 0U);
	}

	empty_view = (const unsigned char *)m_string(&data_descriptor);
	ASSERT(empty_view != NULL);

	if(empty_view != NULL)
	{
		ASSERT(empty_view[0] == 0U);
	}

	empty_view = (const unsigned char *)m_string(&missing_data_string);
	ASSERT(empty_view != NULL);

	if(empty_view != NULL)
	{
		ASSERT(empty_view[0] == 0U);
	}

	empty_view = (const unsigned char *)m_string(&invalid_cached_length);
	ASSERT(empty_view != NULL);

	if(empty_view != NULL)
	{
		ASSERT(empty_view[0] == 0U);
	}

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check soft read-only string access for byte and multi-byte descriptors
 *
 * @return Return describing success or failure
 */
Return test_libmem_0067(void)
{
	INITTEST;

	static const char alpha_text[] = "alpha";
	static const uint32_t wide_text[] = {
		UINT32_C(100),
		UINT32_C(200),
		UINT32_C(0)
	};

	m_create(char,byte_string,MEMORY_STRING);
	m_create(uint32_t,wide_string,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_fixed_string(byte_string,sizeof(alpha_text),alpha_text));

	const char *byte_view = (const char *)m_string(byte_string);
	ASSERT(byte_view != NULL);

	if(byte_view != NULL)
	{
		ASSERT(0 == strcmp(byte_view,"alpha"));
	}

	const char *text_view = m_text(byte_string);
	ASSERT(text_view != NULL);

	if(text_view != NULL)
	{
		ASSERT(0 == strcmp(text_view,"alpha"));
	}

	ASSERT(byte_string->is_string == true);
	ASSERT(byte_string->string_length == strlen("alpha"));

	ASSERT(SUCCESS == m_copy_fixed_string(wide_string,sizeof(wide_text),wide_text));

	const uint32_t *wide_view = (const uint32_t *)m_string(wide_string);
	ASSERT(wide_view != NULL);

	if(wide_view != NULL)
	{
		ASSERT(wide_view[0] == UINT32_C(100));
		ASSERT(wide_view[1] == UINT32_C(200));
		ASSERT(wide_view[2] == UINT32_C(0));
	}

	ASSERT(wide_string->is_string == true);
	ASSERT(wide_string->string_length == 2);

	m_del(wide_string);

	m_create(uint32_t,empty_wide_string,MEMORY_STRING);

	const uint32_t *empty_wide_view = (const uint32_t *)m_string(empty_wide_string);
	ASSERT(empty_wide_view != NULL);

	if(empty_wide_view != NULL)
	{
		ASSERT(empty_wide_view[0] == UINT32_C(0));
	}

	captured_status = FAILURE;
	captured_failed_line = 0;

	ASSERT(SUCCESS == function_capture(
		capture_invalid_mem_string_descriptors,
		captured_stdout,
		captured_stderr));
	ASSERT(captured_stdout->length == 0);
	ASSERT(captured_stderr->length > 0);

	const char *captured_report = m_text(captured_stderr);
	ASSERT(captured_report != NULL);

	if(captured_report != NULL)
	{
		ASSERT(strstr(captured_report,"Soft string view requires a non-zero element size") != NULL);
		ASSERT(strstr(captured_report,"Soft string view requires a non-NULL descriptor") != NULL);
		ASSERT(strstr(captured_report,"Soft string view requires a string descriptor") != NULL);
		ASSERT(strstr(captured_report,"Descriptor has non-zero length with NULL data pointer") != NULL);
		ASSERT(strstr(captured_report,"Soft string view requires string_length to stay below length") != NULL);
	}

	call(m_del(empty_wide_string));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	call(m_del(byte_string));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
