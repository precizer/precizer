#include "test_librational_all.h"

/* Counts side-effect helper calls in SKIP macro tests */
static unsigned int counted_return_calls = 0U;

/**
 * @brief Return a positive check-function answer
 *
 * @return SUCCESS with YES marked as pending by provide()
 */
static Return return_yes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status |= YES;

	provide(status);
}

/**
 * @brief Return a negative check-function answer
 *
 * @return SUCCESS with NO marked as pending by provide()
 */
static Return return_no(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status |= NO;

	provide(status);
}

/**
 * @brief Return conflicting binary answers
 *
 * Used to verify that normalization keeps NO stronger than YES
 *
 * @return SUCCESS with normalized NO and AWAITING flags
 */
static Return return_yes_and_no(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status |= YES | NO;

	provide(status);
}

/**
 * @brief Return a technical failure mixed with a positive answer
 *
 * @return FAILURE with YES marked as pending by provide()
 */
static Return return_failure_with_yes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status = FAILURE | YES;

	provide(status);
}

/**
 * @brief Return success while global_return_status contains conflicting flags
 *
 * @return SUCCESS merged with allowed normalized global flags
 */
static Return return_success_with_conflicting_global_status(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	atomic_store(&global_return_status,SUCCESS | FAILURE | YES | NO | INFO | HALTED);

	provide(status);
}

/**
 * @brief Try to exit through provide() with an already pending answer
 *
 * @return FAILURE because provide() must reject unhandled pending answers
 */
static Return return_unhandled_provide(void)
{
	provide(SUCCESS | YES | AWAITING);
}

/**
 * @brief Try to exit through deliver() with an already pending answer
 *
 * @return FAILURE because deliver() must reject unhandled pending answers
 */
static Return return_unhandled_deliver(void)
{
	deliver(SUCCESS | YES | AWAITING);
}

/**
 * @brief Return success and record that the expression was evaluated
 *
 * @return SUCCESS after incrementing counted_return_calls
 */
static Return return_success_and_count_call(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	counted_return_calls++;

	provide(status);
}

/**
 * @brief Return YES and record that the expression was evaluated
 *
 * @return SUCCESS with YES marked as pending after incrementing counted_return_calls
 */
static Return return_yes_and_count_call(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	counted_return_calls++;
	status |= YES;

	provide(status);
}

/**
 * @brief Check that provide() marks fresh YES and NO answers as pending
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_1(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	/* Fresh yes/no answers must be marked pending so callers cannot ignore them accidentally */
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

/**
 * @brief Check that ask() consumes direct yes/no check calls
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_2(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	/* ask() converts the binary answer to bool and clears the pending answer from status */
	bool answer = ask(return_yes());
	ASSERT(answer == true);
	ASSERT(status == SUCCESS);

	answer = ask(return_no());
	ASSERT(answer == false);
	ASSERT(status == SUCCESS);

	RETURN_STATUS;
}

/**
 * @brief Check that ask() consumes stored and assigned pending answers
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_3(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	/* A pending answer can be consumed after it has already been stored in status */
	status = return_yes();

	bool answer = ask(status);
	ASSERT(answer == true);
	ASSERT(status == SUCCESS);

	answer = ask(status = return_no());
	ASSERT(answer == false);
	ASSERT(status == SUCCESS);

	RETURN_STATUS;
}

/**
 * @brief Check that technical failure stays stronger than a YES answer
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_4(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	/* Preserve the critical status produced by ask(), then restore the test runner status */
	bool answer = ask(return_failure_with_yes());
	Return observed_status = status;
	status = SUCCESS;

	ASSERT(answer == false);
	ASSERT(FAILURE & observed_status);
	ASSERT((SUCCESS & observed_status) == 0);

	RETURN_STATUS;
}

/**
 * @brief Trigger run() and call() with yes/no functions for stderr capture
 *
 * @return Return describing whether both contract violations were detected
 */
static Return capture_librational_0005_5(void)
{
	Return status = SUCCESS;
	Return test_status = SUCCESS;

	atomic_store(&global_return_status,OK);

	/* run() is for regular work functions, not for yes/no check functions */
	run(return_yes());

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	status = SUCCESS;

	/* call() is also required to reject a pending yes/no answer from its callee */
	call(return_yes());

	if((CRITICAL & status) == 0)
	{
		test_status = FAILURE;
	}

	deliver(test_status);
}

/**
 * @brief Check diagnostics when run() and call() receive yes/no functions
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_5(void)
{
	INITTEST;

	/* The exact stderr text is part of the developer-facing contract */
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

/**
 * @brief Trigger run() and call() while local status already has a pending answer
 *
 * @return Return describing whether both contract violations were detected
 */
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

/**
 * @brief Check diagnostics for previous unhandled yes/no answers
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_6(void)
{
	INITTEST;

	/* A previous pending answer must be consumed before regular flow continues */
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

/**
 * @brief Trigger provide() and deliver() with an already pending answer
 *
 * @return Return describing whether both exits reject the pending answer
 */
static Return capture_librational_0005_7(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	/* Function exits must not leak an unconsumed yes/no answer to their caller */
	ASSERT(FAILURE == return_unhandled_provide());
	ASSERT(FAILURE == return_unhandled_deliver());

	deliver(status);
}

/**
 * @brief Check diagnostics for unhandled pending answers at function exit
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_7(void)
{
	INITTEST;

	/* provide() and deliver() intentionally reject the same misuse with different macro names */
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

/**
 * @brief Trigger ask() with raw YES flags that were not returned by provide()
 *
 * @return Return describing whether the misuse was detected
 */
static Return capture_librational_0005_8(void)
{
	Return status = SUCCESS;
	Return test_status = SUCCESS;

	atomic_store(&global_return_status,OK);

	/* A raw YES flag is not enough because ask() requires the AWAITING marker */
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

/**
 * @brief Check that ask() rejects raw yes/no flags without AWAITING
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_8(void)
{
	INITTEST;

	/* Keep the function name flexible because __func__ is intentionally part of the diagnostic */
	static const char expected_stderr_pattern[] =
	        "\\A"
	        "ERROR: [A-Za-z0-9_]+:\\d+ ask\\(\\) expected a yes/no Return answer\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_8));

	RETURN_STATUS;
}

/**
 * @brief Check that an explicit void cast discards a whole Return value
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_9(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	/* This documents the explicit escape hatch for callers that really ignore the whole Return */
	(void)return_yes();

	ASSERT(status == SUCCESS);

	RETURN_STATUS;
}

/**
 * @brief Check global_return_status normalization and propagation
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_10(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	/* Only GLOBAL bits may propagate from process-wide status into the local return */
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

	/* Reset global state so later tests do not inherit this intentionally dirty setup */
	atomic_store(&global_return_status,OK);

	RETURN_STATUS;
}

/**
 * @brief Trigger ask() with AWAITING but without YES or NO
 *
 * @return Return describing whether the misuse was detected
 */
static Return capture_librational_0005_11(void)
{
	Return status = SUCCESS;
	Return test_status = SUCCESS;

	atomic_store(&global_return_status,OK);

	/* A pending marker without a binary answer is malformed and must be rejected */
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

/**
 * @brief Check that ask() rejects pending answers without YES or NO
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_11(void)
{
	INITTEST;

	/* Keep the source location flexible while matching the diagnostic text strictly */
	static const char expected_stderr_pattern[] =
	        "\\A"
	        "ERROR: [A-Za-z0-9_]+:\\d+ ask\\(\\) received a pending Return answer without YES or NO\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_11));

	RETURN_STATUS;
}

/**
 * @brief Trigger rational_ask() with missing caller status storage
 *
 * @return Return describing whether NULL status storage was rejected
 */
static Return capture_librational_0005_12(void)
{
	Return status = SUCCESS;

	atomic_store(&global_return_status,OK);

	/* Directly call the implementation helper to cover the NULL storage guard */
	bool answer = rational_ask(NULL,SUCCESS | YES | AWAITING,__func__,__LINE__);

	if(answer == true)
	{
		status = FAILURE;
	}

	deliver(status);
}

/**
 * @brief Check that rational_ask() rejects NULL status storage
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_12(void)
{
	INITTEST;

	/* Keep the source location flexible while matching the diagnostic text strictly */
	static const char expected_stderr_pattern[] =
	        "\\A"
	        "ERROR: [A-Za-z0-9_]+:\\d+ ask\\(\\) received NULL status storage\\Z";

	ASSERT(SUCCESS == match_function_output(
		expected_stderr_pattern,
		NULL,
		capture_librational_0005_12));

	RETURN_STATUS;
}

/**
 * @brief Check human-readable status text for known, unknown, and mixed flags
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_13(void)
{
	INITTEST;

	/* Pin the exact flag order because diagnostics and tests compare this string directly */
	ASSERT(0 == strcmp(show_status(OK),"OK"));
	ASSERT(0 == strcmp(show_status((Return)0x8000u),"UNKNOWN"));
	ASSERT(0 == strcmp(show_status(FAILURE | (Return)0x8000u),"FAILURE"));
	ASSERT(0 == strcmp(
		show_status(FAILURE | SUCCESS | HALTED | WARNING | DONOTHING | INFO | YES | NO | AWAITING),
		"FAILURE|SUCCESS|HALTED|WARNING|DONOTHING|INFO|YES|NO|AWAITING"));

	RETURN_STATUS;
}

/**
 * @brief Check show_status() fallback behavior for snprintf problems
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_14(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	/* A formatting failure must fall back to UNKNOWN instead of exposing partial buffer state */
	testmocking_snprintf_fail_next(1);
	const char *failed_format_status = show_status(FAILURE);
	testmocking_snprintf_disable();

	ASSERT(0 == strcmp(failed_format_status,"UNKNOWN"));

	/* A truncated write must still leave a terminated static buffer */
	testmocking_snprintf_truncate_next(1);
	const char *truncated_format_status = show_status(FAILURE);
	testmocking_snprintf_disable();

	ASSERT(0 == strcmp(truncated_format_status,"T"));

	RETURN_STATUS;
}

/**
 * @brief Check local conflict resolution in rational_normalize_return()
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_15(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);

	/* Critical technical bits remove SUCCESS from the normalized result */
	Return normalized_status = rational_normalize_return(SUCCESS | FAILURE);
	ASSERT(FAILURE & normalized_status);
	ASSERT((SUCCESS & normalized_status) == 0);

	normalized_status = rational_normalize_return(SUCCESS | WARNING);
	ASSERT(WARNING & normalized_status);
	ASSERT((SUCCESS & normalized_status) == 0);

	/* NO is the dominant binary answer when both local answer flags are present */
	normalized_status = rational_normalize_return(SUCCESS | YES | NO);
	ASSERT(SUCCESS & normalized_status);
	ASSERT(NO & normalized_status);
	ASSERT((YES & normalized_status) == 0);

	RETURN_STATUS;
}

/**
 * @brief Check that global yes/no flags stay local to global_return_status
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_16(void)
{
	INITTEST;

	/* Only GLOBAL bits should reach the returned status from global_return_status */
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

	/* Reset global state so later tests do not inherit the deliberately unusual flags */
	atomic_store(&global_return_status,OK);

	RETURN_STATUS;
}

/**
 * @brief Check that ask() does not evaluate a new expression while status has SKIP
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_17(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);
	counted_return_calls = 0U;

	/* First prove that the helper is callable and increments the counter in normal ask() flow */
	bool direct_answer = ask(return_yes_and_count_call());
	ASSERT(direct_answer == true);
	ASSERT(counted_return_calls == 1U);
	ASSERT(status == SUCCESS);

	/* With SKIP set and no pending answer, ask() must short-circuit before evaluating its argument */
	counted_return_calls = 0U;
	status = FAILURE;
	bool answer = ask(return_yes_and_count_call());
	Return skipped_status = status;
	status = SUCCESS;

	ASSERT(answer == false);
	ASSERT(counted_return_calls == 0U);
	ASSERT(FAILURE & skipped_status);
	ASSERT((AWAITING & skipped_status) == 0);

	RETURN_STATUS;
}

/**
 * @brief Check that run() does not execute its function while status has SKIP
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_18(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);
	counted_return_calls = 0U;

	/* run() is gated by SKIP, so the work expression must not be evaluated */
	status = FAILURE;
	run(return_success_and_count_call());
	Return skipped_status = status;
	status = SUCCESS;

	ASSERT(counted_return_calls == 0U);
	ASSERT(FAILURE & skipped_status);

	RETURN_STATUS;
}

/**
 * @brief Check that call() still executes its function while status has SKIP
 *
 * @return Return describing success or failure
 */
static Return test_librational_0005_19(void)
{
	INITTEST;

	atomic_store(&global_return_status,OK);
	counted_return_calls = 0U;

	/* call() is for mandatory cleanup/final actions, so SKIP must not suppress it */
	status = FAILURE;
	call(return_success_and_count_call());
	Return accumulated_status = status;
	status = SUCCESS;

	ASSERT(counted_return_calls == 1U);
	ASSERT(FAILURE & accumulated_status);
	ASSERT((SUCCESS & accumulated_status) == 0);

	RETURN_STATUS;
}

/**
 * @brief Exercise yes/no Return handling in provide, deliver, ask, run, and call
 *
 * The suite verifies the contract for check functions that return YES or NO,
 * including pending-answer marking, ask() consumption, regular-work macro
 * rejection, function-exit guards, SKIP behavior, global_return_status
 * propagation, rational_normalize_return() conflicts, and show_status()
 * formatting. The diagnostics are checked through captured stderr when the
 * expected behavior is an intentional contract violation
 *
 * @return Return describing success or failure
 */
Return test_librational_0005(void)
{
	INITTEST;

	TEST(test_librational_0005_1,"provide() marks fresh yes/no answers as pending");
	TEST(test_librational_0005_2,"ask() consumes direct yes/no check calls");
	TEST(test_librational_0005_3,"ask() consumes stored and assigned yes/no Return values");
	TEST(test_librational_0005_4,"ask() keeps technical failure stronger than a yes answer");
	TEST(test_librational_0005_5,"run() and call() reject yes/no check functions");
	TEST(test_librational_0005_6,"run() and call() reject previous unhandled yes/no answers");
	TEST(test_librational_0005_7,"provide() and deliver() reject unhandled yes/no answers");
	TEST(test_librational_0005_8,"ask() rejects raw yes/no flags without function-return marking");
	TEST(test_librational_0005_9,"void cast intentionally discards the whole Return value");
	TEST(test_librational_0005_10,"global_return_status is normalized and only global bits propagate");
	TEST(test_librational_0005_11,"ask() rejects pending answers without YES or NO");
	TEST(test_librational_0005_12,"rational_ask() rejects NULL status storage");
	TEST(test_librational_0005_13,"show_status() reports OK, known, unknown and mixed flags");
	TEST(test_librational_0005_14,"show_status() tolerates snprintf failure and truncation");
	TEST(test_librational_0005_15,"rational_normalize_return() resolves local flag conflicts");
	TEST(test_librational_0005_16,"rational_normalize_return() keeps global yes/no flags local");
	TEST(test_librational_0005_17,"ask() skips new expressions when local status already has SKIP");
	TEST(test_librational_0005_18,"run() skips work functions when local status already has SKIP");
	TEST(test_librational_0005_19,"call() still executes mandatory functions when local status has SKIP");

	RETURN_STATUS;
}
