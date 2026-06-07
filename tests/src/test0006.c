#include "sute.h"

/**
 * @brief Compile a PCRE2 pattern for match_regexp() unit tests
 *
 * @param[in] pattern_text Pattern text to compile
 * @param[in] options PCRE2 compile options
 * @return Compiled pattern or NULL on compilation failure
 */
static pcre2_code *compile_test_pattern(
	const char *pattern_text,
	uint32_t   options)
{
	int error_code = 0;
	PCRE2_SIZE error_offset = 0;

	return pcre2_compile(
		(const unsigned char *)pattern_text,
		strlen(pattern_text),
		options,
		&error_code,
		&error_offset,
		NULL);
}

static REGEXP noisy_null_relative_path_response = MATCH;
static REGEXP noisy_damaged_path_response = MATCH;
static REGEXP noisy_invalid_utf8_path_response = MATCH;

/**
 * @brief Capture the NULL relative-path negative case
 *
 * @return SUCCESS when the negative call was exercised
 */
static Return capture_noisy_null_relative_path_case(void)
{
	INITTEST;

	pcre2_code *compiled_pattern = compile_test_pattern(".",0);

	ASSERT(compiled_pattern != NULL);
	noisy_null_relative_path_response = match_regexp(compiled_pattern,NULL);

	pcre2_code_free(compiled_pattern);

	deliver(status);
}

/**
 * @brief Capture the damaged relative-path descriptor negative case
 *
 * @return SUCCESS when the negative call was exercised
 */
static Return capture_noisy_damaged_path_case(void)
{
	INITTEST;

	memory damaged_path = {
		.single_element_size = sizeof(char),
		.actually_allocated_bytes = sizeof(char),
		.length = 1,
		.string_length = 0,
		.is_string = true,
		.data = NULL
	};
	pcre2_code *compiled_pattern = compile_test_pattern(".",0);

	ASSERT(compiled_pattern != NULL);
	noisy_damaged_path_response = match_regexp(compiled_pattern,&damaged_path);

	pcre2_code_free(compiled_pattern);

	deliver(status);
}

/**
 * @brief Capture the invalid UTF-8 PCRE2 runtime error case
 *
 * @return SUCCESS when the negative call was exercised
 */
static Return capture_noisy_invalid_utf8_path_case(void)
{
	INITTEST;

	static const char invalid_utf8_subject[] = {
		(char)0xC3,'(',0
	};
	m_create(char,relative_path,MEMORY_STRING);
	pcre2_code *compiled_pattern = compile_test_pattern(".",PCRE2_UTF);

	ASSERT(compiled_pattern != NULL);
	ASSERT(SUCCESS == m_copy_string(relative_path,sizeof(invalid_utf8_subject),invalid_utf8_subject));
	noisy_invalid_utf8_path_response = match_regexp(compiled_pattern,relative_path);

	pcre2_code_free(compiled_pattern);
	call(m_del(relative_path));

	deliver(status);
}

/**
 * @brief match_regexp() should report MATCH for a matching relative path
 */
static Return test0006_1(void)
{
	INITTEST;

	m_create(char,relative_path,MEMORY_STRING);
	pcre2_code *compiled_pattern = compile_test_pattern("^src/.*\\.c$",0);

	ASSERT(compiled_pattern != NULL);
	ASSERT(SUCCESS == m_copy_string(relative_path,"src/match_regexp.c"));
	ASSERT(MATCH == match_regexp(compiled_pattern,relative_path));

	pcre2_code_free(compiled_pattern);
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief match_regexp() should report NOT_MATCH for a non-matching relative path
 */
static Return test0006_2(void)
{
	INITTEST;

	m_create(char,relative_path,MEMORY_STRING);
	pcre2_code *compiled_pattern = compile_test_pattern("^tests/",0);

	ASSERT(compiled_pattern != NULL);
	ASSERT(SUCCESS == m_copy_string(relative_path,"src/match_regexp.c"));
	ASSERT(NOT_MATCH == match_regexp(compiled_pattern,relative_path));

	pcre2_code_free(compiled_pattern);
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief match_regexp() should allow an empty subject to match a pattern for an empty string
 */
static Return test0006_3(void)
{
	INITTEST;

	m_create(char,relative_path,MEMORY_STRING);
	pcre2_code *compiled_pattern = compile_test_pattern("^$",0);

	ASSERT(compiled_pattern != NULL);
	ASSERT(MATCH == match_regexp(compiled_pattern,relative_path));

	pcre2_code_free(compiled_pattern);
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief match_regexp() should allow an empty subject to fail normally
 */
static Return test0006_4(void)
{
	INITTEST;

	m_create(char,relative_path,MEMORY_STRING);
	pcre2_code *compiled_pattern = compile_test_pattern(".+",0);

	ASSERT(compiled_pattern != NULL);
	ASSERT(NOT_MATCH == match_regexp(compiled_pattern,relative_path));

	pcre2_code_free(compiled_pattern);
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief match_regexp() should reject a missing compiled pattern
 */
static Return test0006_5(void)
{
	INITTEST;

	m_create(char,relative_path,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_string(relative_path,"src/match_regexp.c"));
	ASSERT(REGEXP_ERROR == match_regexp(NULL,relative_path));

	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief match_regexp() should reject a missing relative path descriptor
 */
static Return test0006_6(void)
{
	INITTEST;

	noisy_null_relative_path_response = MATCH;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_noisy_null_relative_path_case));
	ASSERT(REGEXP_ERROR == noisy_null_relative_path_response);

	RETURN_STATUS;
}

/**
 * @brief match_regexp() should report REGEXP_ERROR for a damaged path descriptor
 */
static Return test0006_7(void)
{
	INITTEST;

	static const char expected_stderr_pattern[] =
	        "\\A"
	        "ERROR: [^\\n]*Memory management; Descriptor has non-zero length with NULL data pointer[^\\n]*\n"
	        "\\Z";

	noisy_damaged_path_response = MATCH;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern,
		capture_noisy_damaged_path_case));
	ASSERT(REGEXP_ERROR == noisy_damaged_path_response);

	RETURN_STATUS;
}

/**
 * @brief match_regexp() should report REGEXP_ERROR for a PCRE2 runtime match error
 */
static Return test0006_8(void)
{
	INITTEST;

	static const char expected_stdout_pattern[] =
	        "\\A"
	        "ERROR: PCRE2 match error -8: UTF-8 error: byte 2 top bits not 0x80 for path: \\\\xC3\\(\n"
	        "\\Z";

	noisy_invalid_utf8_path_response = MATCH;

	ASSERT(SUCCESS == match_function_output(
		expected_stdout_pattern,
		NULL,
		capture_noisy_invalid_utf8_path_case));
	ASSERT(REGEXP_ERROR == noisy_invalid_utf8_path_response);

	RETURN_STATUS;
}

/**
 * @brief Run unit tests for match_regexp()
 */
Return test0006(void)
{
	INITTEST;

	TEST(test0006_1,"match_regexp(): matching path");
	TEST(test0006_2,"match_regexp(): non-matching path");
	TEST(test0006_3,"match_regexp(): empty subject can match");
	TEST(test0006_4,"match_regexp(): empty subject can fail normally");
	TEST(test0006_5,"match_regexp(): NULL compiled pattern");
	TEST(test0006_6,"match_regexp(): NULL relative path");
	TEST(test0006_7,"match_regexp(): damaged relative path descriptor");
	TEST(test0006_8,"match_regexp(): PCRE2 runtime match error");

	RETURN_STATUS;
}
