#include "sute.h"
#include "mocks_librational.h"

static Return return_yes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status |= YES;

	provide(status);
}

static Return return_no(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status |= NO;

	provide(status);
}

static Return return_yes_and_no(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status |= YES | NO;

	provide(status);
}

static Return return_failure_with_yes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status = FAILURE | YES;

	provide(status);
}

static Return return_success_with_conflicting_global_status(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	atomic_store(&global_return_status,SUCCESS | FAILURE | YES | NO | INFO | HALTED);

	provide(status);
}

static Return return_unhandled_provide(void)
{
	provide(SUCCESS | YES | AWAITING);
}

static Return return_unhandled_deliver(void)
{
	deliver(SUCCESS | YES | AWAITING);
}

static Return test_librational_0005_1(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	Return yes_status = return_yes();
	ASSERT(SUCCESS & yes_status);
	ASSERT(YES & yes_status);
	ASSERT((NO & yes_status) == 0);
	ASSERT(AWAITING & yes_status);

	Return no_status = return_no();
	ASSERT(SUCCESS & no_status);
	ASSERT((YES & no_status) == 0);
	ASSERT(NO & no_status);
	ASSERT(AWAITING & no_status);

	Return normalized_answer = return_yes_and_no();
	ASSERT(SUCCESS & normalized_answer);
	ASSERT((YES & normalized_answer) == 0);
	ASSERT(NO & normalized_answer);
	ASSERT(AWAITING & normalized_answer);

	RETURN_STATUS;
}

static Return test_librational_0005_2(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	bool answer = ask(return_yes());
	ASSERT(answer == true);
	ASSERT(status == SUCCESS);

	answer = ask(return_no());
	ASSERT(answer == false);
	ASSERT(status == SUCCESS);

	RETURN_STATUS;
}

static Return test_librational_0005_3(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	status = return_yes();

	bool answer = ask(status);
	ASSERT(answer == true);
	ASSERT(status == SUCCESS);

	answer = ask(status = return_no());
	ASSERT(answer == false);
	ASSERT(status == SUCCESS);

	RETURN_STATUS;
}

static Return test_librational_0005_4(void)
{
	Return status = SUCCESS;
	Return test_status = SUCCESS;

	atomic_store(&global_return_status,OK);

	bool answer = ask(return_failure_with_yes());

	if(answer == true)
	{
		test_status = FAILURE;
	}

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	deliver(test_status);
}

static Return capture_librational_0005_5(void)
{
	Return status = SUCCESS;
	Return test_status = SUCCESS;

	atomic_store(&global_return_status,OK);

	run(return_yes());

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	status = SUCCESS;

	call(return_yes());

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	deliver(test_status);
}

static Return test_librational_0005_5(void)
{
	INITTEST;

	static const char expected_stderr_pattern[] =
		"\\A"
		"ERROR: run\\(\\) received an unhandled yes/no Return answer\\. Use ask\\(\\)\n"
		"ERROR: call\\(\\) received an unhandled yes/no Return answer\\. Use ask\\(\\)\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_5));

	RETURN_STATUS;
}

static Return capture_librational_0005_6(void)
{
	Return status = SUCCESS;
	Return test_status = SUCCESS;

	atomic_store(&global_return_status,OK);

	/* A pending answer in status must stop run() before its argument is evaluated */
	status = return_yes();
	run(return_yes());

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	/* call() has the same pending-answer guard, but it does not use the SKIP gate */
	status = return_yes();
	call(return_yes());

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	deliver(test_status);
}

static Return test_librational_0005_6(void)
{
	INITTEST;

	static const char expected_stderr_pattern[] =
		"\\A"
		"ERROR: Unhandled yes/no Return answer before run\\(\\)\n"
		"ERROR: Unhandled yes/no Return answer before call\\(\\)\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_6));

	RETURN_STATUS;
}

static Return capture_librational_0005_7(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	ASSERT(FAILURE == return_unhandled_provide());
	ASSERT(FAILURE == return_unhandled_deliver());

	deliver(status);
}

static Return test_librational_0005_7(void)
{
	INITTEST;

	static const char expected_stderr_pattern[] =
		"\\A"
		"ERROR: Unhandled yes/no Return answer before provide\\(\\)\n"
		"ERROR: Unhandled yes/no Return answer before deliver\\(\\)\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_7));

	RETURN_STATUS;
}

static Return capture_librational_0005_8(void)
{
	Return status = SUCCESS;
	Return test_status = SUCCESS;

	atomic_store(&global_return_status,OK);

	bool answer = ask(SUCCESS | YES);

	if(answer == true)
	{
		test_status = FAILURE;
	}

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	deliver(test_status);
}

static Return test_librational_0005_8(void)
{
	INITTEST;

	static const char expected_stderr_pattern[] =
		"\\A"
		"ERROR: [A-Za-z0-9_]+:\\d+ ask\\(\\) expected a yes/no Return answer\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_8));

	RETURN_STATUS;
}

static Return test_librational_0005_9(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	(void)return_yes();

	ASSERT(status == SUCCESS);

	RETURN_STATUS;
}

static Return test_librational_0005_10(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	Return returned_status = return_success_with_conflicting_global_status();
	Return stored_global_status = atomic_load(&global_return_status);

	ASSERT(SUCCESS & returned_status);
	ASSERT(INFO & returned_status);
	ASSERT(HALTED & returned_status);
	ASSERT((FAILURE & returned_status) == 0);
	ASSERT((YES & returned_status) == 0);
	ASSERT((NO & returned_status) == 0);

	ASSERT(FAILURE & stored_global_status);
	ASSERT(NO & stored_global_status);
	ASSERT(INFO & stored_global_status);
	ASSERT(HALTED & stored_global_status);
	ASSERT((SUCCESS & stored_global_status) == 0);
	ASSERT((YES & stored_global_status) == 0);

	atomic_store(&global_return_status,OK);

	RETURN_STATUS;
}

static Return capture_librational_0005_11(void)
{
	Return status = SUCCESS;
	Return test_status = SUCCESS;

	atomic_store(&global_return_status,OK);

	status = SUCCESS | AWAITING;
	bool answer = ask(status);

	if(answer == true)
	{
		test_status = FAILURE;
	}

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	deliver(test_status);
}

static Return test_librational_0005_11(void)
{
	INITTEST;

	static const char expected_stderr_pattern[] =
		"\\A"
		"ERROR: [A-Za-z0-9_]+:\\d+ ask\\(\\) received a pending Return answer without YES or NO\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_11));

	RETURN_STATUS;
}

static Return capture_librational_0005_12(void)
{
	Return status = SUCCESS;

	atomic_store(&global_return_status,OK);

	bool answer = rational_ask(NULL,SUCCESS | YES | AWAITING,__func__,__LINE__);

	if(answer == true)
	{
		status = FAILURE;
	}

	deliver(status);
}

static Return test_librational_0005_12(void)
{
	INITTEST;

	static const char expected_stderr_pattern[] =
		"\\A"
		"ERROR: [A-Za-z0-9_]+:\\d+ ask\\(\\) received NULL status storage\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_12));

	RETURN_STATUS;
}

static Return test_librational_0005_13(void)
{
	INITTEST;

	ASSERT(0 == strcmp(show_status(OK),"OK"));
	ASSERT(0 == strcmp(show_status((Return)0x8000u),"UNKNOWN"));
	ASSERT(0 == strcmp(
		show_status(FAILURE | SUCCESS | HALTED | WARNING | DONOTHING | INFO | YES | NO | AWAITING),
		"FAILURE|SUCCESS|HALTED|WARNING|DONOTHING|INFO|YES|NO|AWAITING"));

	RETURN_STATUS;
}

static Return test_librational_0005_14(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	mocks_librational_snprintf_fail_next(1);
	const char *failed_format_status = show_status(FAILURE);
	mocks_librational_disable();

	ASSERT(0 == strcmp(failed_format_status,"UNKNOWN"));

	mocks_librational_snprintf_truncate_next(1);
	const char *truncated_format_status = show_status(FAILURE);
	mocks_librational_disable();

	ASSERT(0 == strcmp(truncated_format_status,"T"));

	RETURN_STATUS;
}

static Return test_librational_0005_15(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	Return normalized_status = rational_normalize_return(SUCCESS | FAILURE);
	ASSERT(FAILURE & normalized_status);
	ASSERT((SUCCESS & normalized_status) == 0);

	normalized_status = rational_normalize_return(SUCCESS | WARNING);
	ASSERT(WARNING & normalized_status);
	ASSERT((SUCCESS & normalized_status) == 0);

	normalized_status = rational_normalize_return(SUCCESS | YES | NO);
	ASSERT(SUCCESS & normalized_status);
	ASSERT(NO & normalized_status);
	ASSERT((YES & normalized_status) == 0);

	RETURN_STATUS;
}

static Return test_librational_0005_16(void)
{
	INITTEST;

	atomic_store(&global_return_status,SUCCESS | INFO | YES | NO | AWAITING);

	Return normalized_status = rational_normalize_return(SUCCESS);
	Return stored_global_status = atomic_load(&global_return_status);

	ASSERT(SUCCESS & normalized_status);
	ASSERT(INFO & normalized_status);
	ASSERT((YES & normalized_status) == 0);
	ASSERT((NO & normalized_status) == 0);
	ASSERT((AWAITING & normalized_status) == 0);

	ASSERT(SUCCESS & stored_global_status);
	ASSERT(INFO & stored_global_status);
	ASSERT(NO & stored_global_status);
	ASSERT(AWAITING & stored_global_status);
	ASSERT((YES & stored_global_status) == 0);

	atomic_store(&global_return_status,OK);

	RETURN_STATUS;
}

/**
 * @brief Exercise yes/no Return handling in provide, deliver, ask, run, and call
 *
 * The suite verifies the contract for check functions that return YES or NO.
 * Fresh yes/no answers must be marked as pending, ask() must consume them,
 * regular work macros must reject them, and function exits must not return an
 * unhandled pending answer. The diagnostics are checked through captured
 * stderr when the expected behavior is an intentional contract violation
 *
 * @return Return describing success or failure
 */
Return test_librational_0005(void)
{
	INITTEST;

	TEST(test_librational_0005_1,"provide() marks fresh yes/no answers as pending…");
	TEST(test_librational_0005_2,"ask() consumes direct yes/no check calls…");
	TEST(test_librational_0005_3,"ask() consumes stored and assigned yes/no Return values…");
	TEST(test_librational_0005_4,"ask() keeps technical failure stronger than a yes answer…");
	TEST(test_librational_0005_5,"run() and call() reject yes/no check functions…");
	TEST(test_librational_0005_6,"run() and call() reject previous unhandled yes/no answers…");
	TEST(test_librational_0005_7,"provide() and deliver() reject unhandled yes/no answers…");
	TEST(test_librational_0005_8,"ask() rejects raw yes/no flags without function-return marking…");
	TEST(test_librational_0005_9,"void cast intentionally discards the whole Return value…");
	TEST(test_librational_0005_10,"global_return_status is normalized and only global bits propagate…");
	TEST(test_librational_0005_11,"ask() rejects pending answers without YES or NO…");
	TEST(test_librational_0005_12,"rational_ask() rejects NULL status storage…");
	TEST(test_librational_0005_13,"show_status() reports OK, known flags, and unknown flags…");
	TEST(test_librational_0005_14,"show_status() tolerates snprintf failure and truncation…");
	TEST(test_librational_0005_15,"rational_normalize_return() resolves local flag conflicts…");
	TEST(test_librational_0005_16,"rational_normalize_return() keeps global yes/no flags local…");

	RETURN_STATUS;
}
