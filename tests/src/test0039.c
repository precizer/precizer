#include "sute.h"

/**
 * @brief Verify that a legacy CLI spelling maps to its canonical diagnostic
 *
 * @param[in] arguments Complete CLI arguments for the compatibility check
 * @param[in] canonical_diagnostic Canonical TESTING diagnostic expected in stdout
 * @return Return status code
 */
static Return assert_legacy_cli_argument(
	const char *arguments,
	const char *canonical_diagnostic)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	m_create(char,result,MEMORY_STRING);

	if(arguments == NULL || canonical_diagnostic == NULL)
	{
		status = FAILURE;
	}

	run(set_environment_variable("TESTING","true"));
	run(runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if((SUCCESS & status) && strstr(m_text(result),canonical_diagnostic) == NULL)
	{
		status = FAILURE;
	}

	call(m_del(result));

	deliver(status);
}

/**
 * @brief Verify the legacy --db-clean-ignored spelling
 *
 * @return Return status code
 */
static Return test0039_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == assert_legacy_cli_argument(
		"--dry-run --db-clean-ignored --database=0039_1.db tests/fixtures/diffs/diff1",
		"TESTING:argument:db-drop-ignored=yes"));

	RETURN_STATUS;
}

/**
 * @brief Verify the legacy --drop-inaccessible spelling
 *
 * @return Return status code
 */
static Return test0039_2(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == runit(
		"--database=0039_2.db tests/fixtures/diffs/diff1",
		NULL,
		NULL,
		COMPLETED,
		ALLOW_BOTH));

	ASSERT(SUCCESS == assert_legacy_cli_argument(
		"--dry-run --update --drop-inaccessible --database=0039_2.db tests/fixtures/diffs/diff1",
		"TESTING:argument:db-drop-inaccessible=yes"));

	ASSERT(SUCCESS == delete_path("0039_2.db"));

	RETURN_STATUS;
}

/**
 * @brief Verify the legacy --start-device-only spelling
 *
 * @return Return status code
 */
static Return test0039_3(void)
{
	INITTEST;

	ASSERT(SUCCESS == assert_legacy_cli_argument(
		"--dry-run --start-device-only --database=0039_3.db tests/fixtures/diffs/diff1",
		"TESTING:argument:one-file-system=yes"));

	RETURN_STATUS;
}

/**
 * @brief Verify the legacy -o short spelling
 *
 * @return Return status code
 */
static Return test0039_4(void)
{
	INITTEST;

	ASSERT(SUCCESS == assert_legacy_cli_argument(
		"--dry-run -o --database=0039_4.db tests/fixtures/diffs/diff1",
		"TESTING:argument:one-file-system=yes"));

	RETURN_STATUS;
}

/**
 * @brief Verify that help shows canonical spellings and hides legacy spellings
 *
 * @return Return status code
 */
static Return test0039_5(void)
{
	INITTEST;

	m_create(char,help_output,MEMORY_STRING);
	m_create(char,usage_output,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == runit("--help",help_output,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == runit("--usage",usage_output,NULL,COMPLETED,ALLOW_BOTH));

	const char *canonical_arguments[] = {
		"--db-drop-ignored",
		"--db-drop-inaccessible",
		"--one-file-system"
	};

	for(size_t i = 0; i < sizeof(canonical_arguments) / sizeof(canonical_arguments[0]); i++)
	{
		ASSERT(NULL != strstr(m_text(help_output),canonical_arguments[i]));
		ASSERT(NULL != strstr(m_text(usage_output),canonical_arguments[i]));
	}

	const char *legacy_arguments[] = {
		"--db-clean-ignored",
		"--drop-inaccessible",
		"--start-device-only"
	};

	for(size_t i = 0; i < sizeof(legacy_arguments) / sizeof(legacy_arguments[0]); i++)
	{
		ASSERT(NULL == strstr(m_text(help_output),legacy_arguments[i]));
		ASSERT(NULL == strstr(m_text(usage_output),legacy_arguments[i]));
	}

	ASSERT(NULL == strstr(m_text(help_output),"  -o,"));

	const char *short_options = strstr(m_text(usage_output),"Usage: precizer [-");
	ASSERT(short_options != NULL);

	if(short_options != NULL)
	{
		const char *short_options_end = strchr(short_options,']');
		ASSERT(short_options_end != NULL);

		if(short_options_end != NULL)
		{
			const char *legacy_short_option = strchr(short_options,'o');
			ASSERT(legacy_short_option == NULL || legacy_short_option > short_options_end);
		}
	}

	m_del(usage_output);
	m_del(help_output);

	RETURN_STATUS;
}

/**
 * @brief Run legacy CLI argument compatibility tests
 *
 * @return Return status code
 */
Return test0039(void)
{
	INITTEST;

	TEST(test0039_1,"Legacy --db-clean-ignored maps to --db-drop-ignored");
	TEST(test0039_2,"Legacy --drop-inaccessible maps to --db-drop-inaccessible");
	TEST(test0039_3,"Legacy --start-device-only maps to --one-file-system");
	TEST(test0039_4,"Legacy -o maps to -x");
	TEST(test0039_5,"Help and usage hide legacy CLI spellings");

	RETURN_STATUS;
}
