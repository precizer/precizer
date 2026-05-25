#include "test_libmem_utils.h"
#include <wchar.h>

/**
 * @brief Capture noisy mem_formatted_string frame-error cases
 *
 * These cases intentionally exercise report() paths. match_function_output()
 * keeps the expected diagnostics out of the common test output
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_formatted_string_frame_errors(void)
{
	INITTEST;

	/* Frame error: a data-mode destination is rejected */
	{
		m_create(char,not_a_string);
		ASSERT(not_a_string->is_string == false);
		ASSERT(FAILURE == m_formatted_string(not_a_string,"anything"));
		ASSERT(not_a_string->is_string == false);
		call(m_del(not_a_string));
	}

	/* Frame error: an element width that is neither sizeof(char) nor
	   sizeof(wchar_t) is rejected even when the destination is a string
	   descriptor. uint16_t is two bytes on the supported platforms while
	   wchar_t is wider, so the dispatch must reject it */
	{
		m_create(uint16_t,unsupported_width,MEMORY_STRING);
		ASSERT(unsupported_width->is_string == true);
		ASSERT(unsupported_width->single_element_size == sizeof(uint16_t));
		ASSERT(unsupported_width->single_element_size != sizeof(char));
		ASSERT(unsupported_width->single_element_size != sizeof(wchar_t));
		ASSERT(FAILURE == m_formatted_string(unsupported_width,"anything"));
		call(m_del(unsupported_width));
	}

	deliver(status);
}

/**
 * @brief Verify mem_formatted_string for char and wchar_t string descriptors
 * @details Exercises the public mem_formatted_string helper and its
 *          m_formatted_string alias for both supported element widths. The
 *          coverage includes:
 *           - byte (char) destinations: basic formatting with mixed
 *             conversions, replacement of an existing payload with both
 *             shorter and longer rendered text, an empty rendered result
 *             that keeps the descriptor in string mode, and the quiet
 *             flow-data behavior on a NULL format string
 *           - wide (wchar_t) destinations: basic formatting plus a long
 *             format that intentionally exceeds the iterative-growth
 *             starting capacity so the doubling loop in the wide path is
 *             actually exercised
 *           - frame-error rejections: non-string destination, unsupported
 *             element width
 *
 * @return Return enum indicating success or failure of the test
 * @retval SUCCESS if test passed
 * @retval FAILURE if test failed
 */
Return test_libmem_0070(void)
{
	INITTEST;

	/* 1. Byte: basic format with mixed conversions writes the expected text */
	{
		m_create(char,byte_basic,MEMORY_STRING);
		ASSERT(SUCCESS == m_formatted_string(byte_basic,"Hello %s %d!","world",42));
		ASSERT(byte_basic->is_string == true);
		ASSERT(byte_basic->single_element_size == sizeof(char));
		ASSERT(byte_basic->string_length == strlen("Hello world 42!"));
		ASSERT(strcmp(m_text(byte_basic),"Hello world 42!") == 0);
		call(m_del(byte_basic));
	}

	/* 2. Byte: each call fully replaces the previous content (longer then shorter) */
	{
		m_create(char,byte_replace,MEMORY_STRING);
		ASSERT(SUCCESS == m_formatted_string(byte_replace,"first"));
		ASSERT(strcmp(m_text(byte_replace),"first") == 0);

		ASSERT(SUCCESS == m_formatted_string(byte_replace,"second %d goes here",100));
		ASSERT(strcmp(m_text(byte_replace),"second 100 goes here") == 0);
		ASSERT(byte_replace->string_length == strlen("second 100 goes here"));

		ASSERT(SUCCESS == m_formatted_string(byte_replace,"x"));
		ASSERT(strcmp(m_text(byte_replace),"x") == 0);
		ASSERT(byte_replace->string_length == 1U);

		call(m_del(byte_replace));
	}

	/* 3. Byte: an empty rendered result still leaves a valid empty string */
	{
		m_create(char,byte_empty,MEMORY_STRING);
		ASSERT(SUCCESS == m_formatted_string(byte_empty,"%s",""));
		ASSERT(byte_empty->is_string == true);
		ASSERT(byte_empty->string_length == 0U);
		ASSERT(m_text(byte_empty)[0] == '\0');
		call(m_del(byte_empty));
	}

	/* 4. Byte: NULL format string is quiet flow data and does not modify the descriptor */
	{
		m_create(char,byte_quiet,MEMORY_STRING);
		ASSERT(SUCCESS == m_formatted_string(byte_quiet,"seed"));
		ASSERT(strcmp(m_text(byte_quiet),"seed") == 0);

		ASSERT(SUCCESS == m_formatted_string(byte_quiet,NULL));
		ASSERT(strcmp(m_text(byte_quiet),"seed") == 0);
		ASSERT(byte_quiet->string_length == strlen("seed"));

		call(m_del(byte_quiet));
	}

	/* 5. Wide: basic format with mixed conversions writes the expected wide text */
	{
		m_create(wchar_t,wide_basic,MEMORY_STRING);
		ASSERT(SUCCESS == m_formatted_string(wide_basic,L"Wide %ls %d",L"text",7));
		ASSERT(wide_basic->is_string == true);
		ASSERT(wide_basic->single_element_size == sizeof(wchar_t));
		ASSERT(wide_basic->string_length == wcslen(L"Wide text 7"));
		ASSERT(wcscmp((const wchar_t *)wide_basic->data,L"Wide text 7") == 0);
		call(m_del(wide_basic));
	}

	/* 6. Wide: long format exercises the iterative-growth path past the
	   initial 128-element capacity guess inside the wide branch */
	{
		m_create(wchar_t,wide_long,MEMORY_STRING);

		/* This source is intentionally longer than 128 wchar_t elements so the
		   initial speculative render fails and the doubling loop runs */
		const wchar_t *long_source =
			L"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
			L"Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
			L"Ut enim ad minim veniam, quis nostrud exercitation ullamco.";

		ASSERT(wcslen(long_source) > 128U);
		ASSERT(SUCCESS == m_formatted_string(wide_long,L"%ls",long_source));
		ASSERT(wide_long->is_string == true);
		ASSERT(wide_long->string_length == wcslen(long_source));
		ASSERT(wcscmp((const wchar_t *)wide_long->data,long_source) == 0);
		call(m_del(wide_long));
	}

	/* Expected stderr layout for frame errors rejected by mem_formatted_string().
	   Source line numbers and errno wording are intentionally flexible because
	   they are build- and platform-dependent */
	static const char expected_stderr_pattern_libmem_0070[] =
		"\\A"
		"ERROR: src/mem_formatted_string\\.c:mem_formatted_string:\\d+ Memory management; Destination must be a string descriptor Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
		"ERROR: src/mem_formatted_string\\.c:mem_formatted_string:\\d+ Memory management; Formatted mode supports only char and wchar_t element widths Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
		"\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0070,
		capture_libmem_formatted_string_frame_errors));

	RETURN_STATUS;
}
