#include "sute.h"

static const char *vacuum_database_path = NULL;
static Return vacuum_status = FAILURE;

/**
 * @brief Capture vacuum output while preserving the operation's return status
 *
 * The capture API accepts a void callback, so the result of db_vacuum() is
 * saved separately from the status of the capture operation
 */
static void capture_vacuum(void)
{
	vacuum_status = db_vacuum(vacuum_database_path);
}

/**
 * @brief Check reported savings against file sizes and bound the reported duration
 *
 * The percentage is compared with the independently measured file sizes,
 * allowing for rounding to two decimal places. The displayed duration keeps
 * only the largest time unit and must fit within the measured interval around
 * the complete captured vacuum call.
 *
 * That interval includes output capture and database opening and closing.
 * It provides an upper bound, but does not verify the exact SQL execution time
 * or detect zero or underestimated durations
 *
 * @param[in] output Captured vacuum output
 * @param before_size Database file size immediately before vacuuming
 * @param after_size Database file size immediately after vacuuming
 * @param enclosing_elapsed_ns Duration of the complete captured call in nanoseconds
 * @return SUCCESS when both measurements agree with their independent bounds
 */
static Return check_metrics(
	const memory        *output,
	const off_t         before_size,
	const off_t         after_size,
	const long long int enclosing_elapsed_ns)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	long double reported_savings = 0.0L;
	long double reported_elapsed = 0.0L;
	char elapsed_unit[8] = {0};
	const char *metrics = strstr(m_text(output),"vacuumed: saved ");

	/* Validate the measured bounds and parse the values printed by db_vacuum() */
	ASSERT(before_size > 0);
	ASSERT(after_size > 0);
	ASSERT(enclosing_elapsed_ns >= 0);
	ASSERT(metrics != NULL);
	ASSERT(3 == sscanf(metrics,"vacuumed: saved %Lf%%, elapsed %Lf%7s",
		&reported_savings,&reported_elapsed,elapsed_unit));

	/* Compare with the independently measured size reduction. The 0.0051
	   percentage-point tolerance allows rounding to two decimal places, with
	   a small margin for floating-point arithmetic */
	ASSERT(reported_savings >=
		100.0L - 100.0L * (long double)after_size / (long double)before_size - 0.0051L);
	ASSERT(reported_savings <=
		100.0L - 100.0L * (long double)after_size / (long double)before_size + 0.0051L);

	/* Convert the printed time unit back to nanoseconds for comparison */
	static const struct {
		const char *suffix;
		long double nanoseconds;
	} duration_units[] = {
		{"ns",1.0L},
		{"μs",1000.0L},
		{"ms",1000000.0L},
		{"s",1000000000.0L},
		{"min",60000000000.0L},
		{"h",3600000000000.0L},
		{"d",86400000000000.0L},
		{"w",604800000000000.0L},
		{"mon",2628000000000000.0L},
		{"y",31536000000000000.0L}
	};
	const size_t duration_unit_count = sizeof(duration_units) / sizeof(duration_units[0]);
	size_t duration_unit_index = 0;

	while(duration_unit_index < duration_unit_count
	        && strcmp(elapsed_unit,duration_units[duration_unit_index].suffix) != 0)
	{
		++duration_unit_index;
	}

	/* Require a known unit and a nonnegative duration within the outer bound */
	ASSERT(duration_unit_index < duration_unit_count);
	ASSERT(reported_elapsed >= 0.0L);
	ASSERT(reported_elapsed * duration_units[duration_unit_index].nanoseconds <=
		(long double)enclosing_elapsed_ns);

	provide(status);
}

/**
 * @brief Verify that primary database vacuum reports reclaimed space
 *
 * Inserting and deleting 1 MiB of data with automatic vacuuming disabled
 * leaves unused pages inside the database file. Vacuuming must make the file
 * smaller and report the percentage saved from its actual before/after sizes.
 * The full output must identify the primary database and report a duration
 * within the independently measured upper bound.
 *
 * The primary database must be marked as modified. A control value in its
 * saved startup size must remain unchanged, proving that the vacuum operation
 * did not replace that baseline with a new measurement
 *
 * @return Return status for the numeric measurements and complete output checks
 */
static Return test0041_1(void)
{
	INITTEST;

	/* Save shared configuration, logger settings, and capture callback state */
	Config *initial_config = config;
	Config vacuum_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *initial_database_path = vacuum_database_path;
	const Return initial_vacuum_status = vacuum_status;
	sqlite3 *database = NULL;
	struct stat before = {0};
	struct stat after = {0};
	const char *database_filename = "0041_primary_vacuum.db";

	m_create(char,database_path,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	vacuum_config.db_primary_file_path = m_init(char,MEMORY_STRING);
	/* Use an arbitrary control value to detect replacement of the saved startup
	   size. The expected savings come from separate stat measurements */
	vacuum_config.db_file_stat.st_size = 137;

	/* Create unused pages by inserting and deleting 1 MiB with automatic
	   vacuuming disabled, so the tested call has space to reclaim */
	ASSERT(SUCCESS == construct_path(database_filename,database_path));
	ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,&database));
	ASSERT(SQLITE_OK == sqlite3_exec(database,
		"PRAGMA page_size=4096;"
		"PRAGMA auto_vacuum=NONE;"
		"CREATE TABLE payload (value BLOB);"
		"INSERT INTO payload VALUES (zeroblob(1048576));"
		"DELETE FROM payload;",
		NULL,NULL,NULL));

	/* Close the preparation connection before db_vacuum() opens its own */
	const int close_status = sqlite3_close(database);
	ASSERT(SQLITE_OK == close_status);

	/* Identify this file as primary and measure its size independently */
	ASSERT(SUCCESS == m_copy(&vacuum_config.db_primary_file_path,database_path));
	ASSERT(SUCCESS == get_file_stat(m_text(database_path),&before));

	/* Route the captured operation through the isolated configuration */
	config = &vacuum_config;
	rational_logger_mode = REGULAR;
	vacuum_database_path = m_text(database_path);
	vacuum_status = FAILURE;

	/* Measure the complete call, including capture and connection overhead,
	   to obtain an upper bound for the reported SQL execution time */
	const long long int capture_start_ns = cur_time_monotonic_ns();
	ASSERT(SUCCESS == function_capture(capture_vacuum,captured_stdout,captured_stderr));
	const long long int capture_elapsed_ns = cur_time_monotonic_ns() - capture_start_ns;

	/* Require a successful operation, no stderr output, and a smaller file */
	ASSERT(SUCCESS == vacuum_status);
	ASSERT(captured_stderr->length == 0U);
	ASSERT(SUCCESS == get_file_stat(m_text(database_path),&after));
	ASSERT(after.st_size < before.st_size);

	/* Compare the full output and check its numbers against independent bounds */
	ASSERT(SUCCESS == get_file_content("templates/0041_001.txt",pattern));
	ASSERT(SUCCESS == match_pattern(captured_stdout,pattern,"templates/0041_001.txt"));
	ASSERT(SUCCESS == check_metrics(captured_stdout,before.st_size,after.st_size,capture_elapsed_ns));

	/* Vacuum must mark the primary file as modified without replacing its baseline */
	ASSERT(vacuum_config.db_primary_file_modified == true);
	ASSERT(vacuum_config.db_file_stat.st_size == 137);

	/* Restore shared state even if preparation or an assertion failed */
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	vacuum_database_path = initial_database_path;
	vacuum_status = initial_vacuum_status;

	/* Remove the fixture and release all buffers after any test result */
	call(delete_path_if_present(database_filename));
	call(m_del(&vacuum_config.db_primary_file_path));
	call(m_del(pattern));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	call(m_del(database_path));

	RETURN_STATUS;
}

/**
 * @brief Verify that vacuum reports zero savings for a compact non-primary database
 *
 * A separate database is compacted during preparation so the tested vacuum
 * call has no unused pages to reclaim. The file size must stay unchanged and
 * the full output must report zero savings without the primary-database label.
 * The reported duration must fit within the independently measured upper bound.
 *
 * The configuration names a different file as the primary database. Its
 * modification flag and saved startup size must remain unchanged
 *
 * @return Return status for the numeric measurements and complete output checks
 */
static Return test0041_2(void)
{
	INITTEST;

	/* Save shared configuration, logger settings, and capture callback state */
	Config *initial_config = config;
	Config vacuum_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *initial_database_path = vacuum_database_path;
	const Return initial_vacuum_status = vacuum_status;
	sqlite3 *database = NULL;
	struct stat before = {0};
	struct stat after = {0};
	const char *database_filename = "0041_non_primary_vacuum.db";

	m_create(char,database_path,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	vacuum_config.db_primary_file_path = m_init(char,MEMORY_STRING);
	/* Use an arbitrary control value to detect replacement of the saved startup
	   size. The expected savings come from separate stat measurements */
	vacuum_config.db_file_stat.st_size = 137;

	/* Build and compact the fixture before the measured call. The preparatory
	   VACUUM removes the space left by the inserted and deleted 1 MiB payload */
	ASSERT(SUCCESS == construct_path(database_filename,database_path));
	ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,&database));
	ASSERT(SQLITE_OK == sqlite3_exec(database,
		"PRAGMA page_size=4096;"
		"PRAGMA auto_vacuum=NONE;"
		"CREATE TABLE payload (value BLOB);"
		"INSERT INTO payload VALUES (zeroblob(1048576));"
		"DELETE FROM payload;"
		"VACUUM;",
		NULL,NULL,NULL));

	/* Close the preparation connection before db_vacuum() opens its own */
	const int close_status = sqlite3_close(database);
	ASSERT(SQLITE_OK == close_status);

	/* Name a different primary file and measure the compact fixture size */
	ASSERT(SUCCESS == m_copy_literal(&vacuum_config.db_primary_file_path,"another_database.db"));
	ASSERT(SUCCESS == get_file_stat(m_text(database_path),&before));

	/* Route the captured operation through the isolated configuration */
	config = &vacuum_config;
	rational_logger_mode = REGULAR;
	vacuum_database_path = m_text(database_path);
	vacuum_status = FAILURE;

	/* Measure the complete call, including capture and connection overhead,
	   to obtain an upper bound for the reported SQL execution time */
	const long long int capture_start_ns = cur_time_monotonic_ns();
	ASSERT(SUCCESS == function_capture(capture_vacuum,captured_stdout,captured_stderr));
	const long long int capture_elapsed_ns = cur_time_monotonic_ns() - capture_start_ns;

	/* Require a successful operation, no stderr output, and an unchanged size */
	ASSERT(SUCCESS == vacuum_status);
	ASSERT(captured_stderr->length == 0U);
	ASSERT(SUCCESS == get_file_stat(m_text(database_path),&after));
	ASSERT(after.st_size == before.st_size);

	/* Compare the full output and check its numbers against independent bounds */
	ASSERT(SUCCESS == get_file_content("templates/0041_002.txt",pattern));
	ASSERT(SUCCESS == match_pattern(captured_stdout,pattern,"templates/0041_002.txt"));
	ASSERT(SUCCESS == check_metrics(captured_stdout,before.st_size,after.st_size,capture_elapsed_ns));

	/* Vacuuming another database must preserve the primary flag and baseline */
	ASSERT(vacuum_config.db_primary_file_modified == false);
	ASSERT(vacuum_config.db_file_stat.st_size == 137);

	/* Restore shared state even if preparation or an assertion failed */
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	vacuum_database_path = initial_database_path;
	vacuum_status = initial_vacuum_status;

	/* Remove the fixture and release all buffers after any test result */
	call(delete_path_if_present(database_filename));
	call(m_del(&vacuum_config.db_primary_file_path));
	call(m_del(pattern));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	call(m_del(database_path));

	RETURN_STATUS;
}

/**
 * @brief Check primary and non-primary vacuum measurements independently
 *
 * Two real SQLite files exercise different outcomes. Vacuuming the primary
 * database must reclaim space left by deleted data and mark that database as
 * modified. Vacuuming an already compact non-primary database must report zero
 * savings and leave the primary database's modification flag unchanged.
 *
 * Both cases compare the complete output with a template and verify the saved
 * percentage against independent file-size measurements. A control value also
 * checks that the saved startup size is preserved. Reported time is checked
 * only against an outer duration that includes capture and connection overhead;
 * this does not detect zero or underestimated SQL execution times
 *
 * @return Return status for both vacuum scenarios
 */
Return test0041(void)
{
	INITTEST;

	TEST(test0041_1,"Primary vacuum reclaims space and preserves the startup stat");
	TEST(test0041_2,"Non-primary vacuum reports zero savings for a compact database");

	RETURN_STATUS;
}
