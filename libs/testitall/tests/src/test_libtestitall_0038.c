#include "sute.h"

static const char expected_noisy_trim_trailing_eol_stderr_pattern[] =
        "\\A"
        "ERROR: [^\\n]*Memory management; string truncate requires a string descriptor[^\\n]*\n"
        "ERROR: [^\\n]*Memory management; String descriptor is inconsistent during truncate[^\\n]*\n"
        "\\Z";

static memory *noisy_data_text = NULL;
static memory *noisy_inconsistent_text = NULL;
static Return noisy_data_text_status = SUCCESS;
static Return noisy_inconsistent_text_status = SUCCESS;

/**
 * @brief Exercise trim_trailing_eol() failures diagnosed by libmem
 *
 * The helper is invoked through match_function_output() so expected libmem
 * diagnostics stay captured instead of leaking into the common test output
 *
 * @return SUCCESS when both negative calls were exercised
 */
static Return capture_noisy_trim_trailing_eol_cases(void)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	noisy_data_text_status = trim_trailing_eol(noisy_data_text);
	noisy_inconsistent_text_status = trim_trailing_eol(noisy_inconsistent_text);

	deliver(status);
}

/**
 * @brief Apply trim_trailing_eol() to one byte string and check its metadata
 *
 * @param input Initial visible text
 * @param expected Expected visible text after trimming
 *
 * @return Return status code
 */
static Return check_trim_trailing_eol_case(
	const char *input,
	const char *expected)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,text,MEMORY_STRING);
	size_t original_length = 0U;

	run(m_copy_string(text,input));

	if(SUCCESS == status)
	{
		original_length = text->length;
	}

	run(trim_trailing_eol(text));

	if(SUCCESS == status)
	{
		if(strcmp(m_text(text),expected) != 0 ||
		        text->string_length != strlen(expected) ||
		        text->length != original_length)
		{
			status = FAILURE;
		}
	}

	call(m_del(text));

	deliver(status);
}

/**
 * @brief Check trailing-EOL removal through libmem string metadata and truncation
 *
 * Covers byte-string endings, an already reserved string buffer, and inputs
 * that do not satisfy or have corrupted the byte-string descriptor contract
 *
 * @return Return status code
 */
Return test_libtestitall_0038(void)
{
	INITTEST;

	static const struct {
		const char *input;
		const char *expected;
	} test_cases[] = {
		{"text\n","text"},
		{"text\r","text"},
		{"text\r\n","text"},
		{"text\n\n","text\n"},
		{"text","text"},
		{"",""}
	};

	for(size_t i = 0U; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i)
	{
		ASSERT(SUCCESS == check_trim_trailing_eol_case(
			test_cases[i].input,
			test_cases[i].expected));
	}

	m_create(char,reserved_text,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_string(reserved_text,"reserved\r\n"));
	ASSERT(SUCCESS == m_resize(reserved_text,64U));
	ASSERT(SUCCESS == trim_trailing_eol(reserved_text));
	ASSERT(0 == strcmp(m_text(reserved_text),"reserved"));
	ASSERT(reserved_text->string_length == strlen("reserved"));
	ASSERT(reserved_text->length == 64U);

	call(m_del(reserved_text));

	m_create(char,data_text);
	m_create(unsigned short,wide_text,MEMORY_STRING);

	char invalid_visible_value = '\0';
	memory inconsistent_text = m_init(char,MEMORY_STRING);
	inconsistent_text.data = &invalid_visible_value;
	inconsistent_text.length = 1U;
	inconsistent_text.actually_allocated_bytes = sizeof(invalid_visible_value);
	inconsistent_text.string_length = 1U;

	memory stale_empty_text = m_init(char,MEMORY_STRING);
	stale_empty_text.string_length = 1U;

	noisy_data_text = data_text;
	noisy_inconsistent_text = &inconsistent_text;
	noisy_data_text_status = SUCCESS;
	noisy_inconsistent_text_status = SUCCESS;

	ASSERT(FAILURE == trim_trailing_eol(NULL));
	ASSERT(FAILURE == trim_trailing_eol(wide_text));
	ASSERT(FAILURE == trim_trailing_eol(&stale_empty_text));
	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_noisy_trim_trailing_eol_stderr_pattern,
		capture_noisy_trim_trailing_eol_cases));
	ASSERT(FAILURE & noisy_data_text_status);
	ASSERT(FAILURE & noisy_inconsistent_text_status);

	call(m_del(wide_text));
	call(m_del(data_text));

	RETURN_STATUS;
}
