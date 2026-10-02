#include "sute.h"

static const TraversalSummary *statistics_summary = NULL;

/**
 * @brief Print the selected traversal summary through the output capture API
 */
static void capture_statistics(void)
{
	show_statistics(statistics_summary);
}

/**
 * @brief Compare one statistics report with its complete expected output
 *
 * The supplied configuration and counters are used only for this capture.
 * Shared configuration, logger mode, and callback state are restored before
 * returning, including when capture or comparison fails
 *
 * @param statistics_config Per-case configuration with its runtime start set for capture
 * @param summary Fully initialized counters for the report
 * @param template_path File containing the expected complete output
 * @return SUCCESS when stdout matches the template and stderr is empty
 */
static Return check_statistics_output(
	Config                 *statistics_config,
	const TraversalSummary *summary,
	const char             *template_path)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Save the shared configuration, logger mode, and capture callback state */
	Config *initial_config = config;
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const TraversalSummary *initial_summary = statistics_summary;
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	/* Enable the report without traversing or reading files */
	statistics_config->app_start_time_ns = cur_time_monotonic_ns();
	config = statistics_config;
	rational_logger_mode = REGULAR;
	statistics_summary = summary;

	/* Capture one report and check both output streams */
	ASSERT(SUCCESS == function_capture(capture_statistics,captured_stdout,captured_stderr));
	ASSERT(captured_stderr->length == 0U);
	ASSERT(SUCCESS == get_file_content(template_path,pattern));
	ASSERT(SUCCESS == match_pattern(captured_stdout,pattern,template_path));

	/* Restore shared state and release buffers even after an assertion fails */
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	statistics_summary = initial_summary;
	call(m_del(pattern));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	deliver(status);
}

/**
 * @brief Verify that scan and hashing rates use their respective intervals
 * @return Return status for the complete output check
 */
static Return test0043_1(void)
{
	INITTEST;

	/* One MiB over two seconds gives 512 KiB/s; hashing over half a second
	   gives 2 MiB/s, so the two rates must use their respective intervals */
	Config statistics_config = {0};
	const TraversalSummary summary = {
		.total_hashed_bytes = 1048576U,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = 500000000LL,
		.scan_elapsed_ns = 2000000000LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_001.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify unavailable rates when no bytes were hashed
 * @return Return status for the complete output check
 */
static Return test0043_2(void)
{
	INITTEST;

	/* No hashed bytes means both rates are unavailable despite positive times */
	Config statistics_config = {0};
	const TraversalSummary summary = {
		.total_hashed_bytes = 0U,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = 500000000LL,
		.scan_elapsed_ns = 2000000000LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_002.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify unavailable rates for zero measurement intervals
 * @return Return status for the complete output check
 */
static Return test0043_3(void)
{
	INITTEST;

	/* Positive bytes with zero intervals must not produce a division result */
	Config statistics_config = {0};
	const TraversalSummary summary = {
		.total_hashed_bytes = 1048576U,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = 0LL,
		.scan_elapsed_ns = 0LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_003.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify that negative intervals display zero time and unavailable rates
 * @return Return status for the complete output check
 */
static Return test0043_4(void)
{
	INITTEST;

	/* Negative intervals produce unavailable rates and a zero time display */
	Config statistics_config = {0};
	const TraversalSummary summary = {
		.total_hashed_bytes = 1048576U,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = -1LL,
		.scan_elapsed_ns = -1LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_003.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify the wording for positive rates below one byte per second
 * @return Return status for the complete output check
 */
static Return test0043_5(void)
{
	INITTEST;

	/* A positive rate below one byte per second has its own explicit wording */
	Config statistics_config = {0};
	const TraversalSummary summary = {
		.total_hashed_bytes = 1U,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = 3000000000LL,
		.scan_elapsed_ns = 2000000000LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_004.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify that a rate of 1.5 bytes per second is displayed as 1B/s
 * @return Return status for the complete output check
 */
static Return test0043_6(void)
{
	INITTEST;

	/* Fractional rates above one byte per second are truncated, not rounded */
	Config statistics_config = {0};
	const TraversalSummary summary = {
		.total_hashed_bytes = 3U,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = 2000000000LL,
		.scan_elapsed_ns = 2000000000LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_005.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify saturation before converting an oversized rate to size_t
 * @return Return status for the complete output check
 */
static Return test0043_7(void)
{
	INITTEST;

	/* A one-nanosecond interval makes the rate exceed SIZE_MAX. Both rates
	   must saturate at the largest representable byte count before conversion */
	Config statistics_config = {0};
	const TraversalSummary summary = {
		.total_hashed_bytes = SIZE_MAX,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = 1LL,
		.scan_elapsed_ns = 1LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_006.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify that ordinary dry-run reports no hashed data or rates
 * @return Return status for the complete output check
 */
static Return test0043_8(void)
{
	INITTEST;

	/* Ordinary dry-run suppresses hashed data and rates even if counters exist */
	Config statistics_config = {
		.dry_run = true,
		.dry_run_with_checksums = false
	};
	const TraversalSummary summary = {
		.total_hashed_bytes = 1048576U,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = 500000000LL,
		.scan_elapsed_ns = 2000000000LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_007.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify measured rates when dry-run includes checksum calculation
 * @return Return status for the complete output check
 */
static Return test0043_9(void)
{
	INITTEST;

	/* Dry-run with checksums reports the same measured rates as ordinary hashing */
	Config statistics_config = {
		.dry_run = true,
		.dry_run_with_checksums = true
	};
	const TraversalSummary summary = {
		.total_hashed_bytes = 1048576U,
		.at_least_one_file_was_shown = true,
		.total_hashing_elapsed_ns = 500000000LL,
		.scan_elapsed_ns = 2000000000LL
	};

	ASSERT(SUCCESS == check_statistics_output(&statistics_config,&summary,"templates/0043_001.txt"));

	RETURN_STATUS;
}

/**
 * @brief Verify complete statistics reports and independently calculated rates
 *
 * Fixed byte counts and intervals distinguish scan throughput from hashing
 * throughput. The cases also exercise unavailable measurements, fractional
 * rates, conversion bounds, and dry-run behavior. Every report is compared
 * with a complete template; only the actual application runtime is variable.
 * Each subtest supplies its own configuration, counters, and expected output.
 * The tests check reporting from supplied counters, not traversal timing
 *
 * @return Return status for the output and rate checks
 */
Return test0043(void)
{
	INITTEST;

	TEST(test0043_1,"Scan and hashing rates use their respective intervals");
	TEST(test0043_2,"No hashed bytes makes both rates unavailable");
	TEST(test0043_3,"Zero intervals make both rates unavailable");
	TEST(test0043_4,"Negative intervals display zero time and unavailable rates");
	TEST(test0043_5,"Positive rates below one byte per second are identified");
	TEST(test0043_6,"Fractional rates are truncated to whole bytes per second");
	TEST(test0043_7,"Oversized rates saturate before integer conversion");
	TEST(test0043_8,"Ordinary dry-run reports no hashed data or rates");
	TEST(test0043_9,"Dry-run with checksums reports measured rates");

	RETURN_STATUS;
}
