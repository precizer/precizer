#include "sute.h"

/* Store the expected value in this library suite instead of an application template file */
static const char expected_capability_demonstration_output[] =
        "The gentle melody floated through\n"
        "the afternoon air as children played in the garden,\n"
        "their laughter mixing with birdsong.\n"
        "Butterflies danced among colorful flowers while a\n"
        "tabby cat watched lazily from its sunny spot on the\n"
        "wooden fence, tail swaying gently in the warm summer breeze.\n";

/**
 * @brief Print demonstration output for function_capture()
 */
static void print_capability_demonstration_output(void)
{
	// Produce output independently from the expected string stored by this test
	printf(
		"The %se melody floated through\n"
		"the afternoon air as children played in the garden,\n"
		"their laughter mixing with birdsong.\n"
		"Butterflies danced among colorful flowers while a\n"
		"tabby cat watched lazily from its sunny spot on the\n"
		"wooden fence, tail swaying %sy in the warm summer breeze.\n",
		"gentl",
		"gentl");
}

/**
 * @brief Check stdout capture against an expected string kept in the library test
 * @details The test is self-contained and does not depend on application template files
 *
 * @return Return status code
 */
Return test_libtestitall_0010(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	// Capture output before comparing it with the inline expectation
	ASSERT(SUCCESS == function_capture(
		print_capability_demonstration_output,
		captured_stdout,
		captured_stderr));

	ASSERT(captured_stdout->length > 0U);
	ASSERT(captured_stderr->length == 0U);
	ASSERT(m_text(captured_stdout) != NULL);

	IF(m_text(captured_stdout) != NULL)
	{
		/* Compare the materialized capture only after its text view is confirmed available */
		ASSERT(0 == strcmp(m_text(captured_stdout),expected_capability_demonstration_output));
	}

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	RETURN_STATUS;
}
