#include "sute.h"

static const char *test0041_database_path = NULL;
static Return test0041_vacuum_status = FAILURE;

/**
 * @brief Capture vacuum output while preserving the operation's return status
 */
static void capture_vacuum(void)
{
	test0041_vacuum_status = db_vacuum(test0041_database_path);
}

/**
 * @brief Check reported savings against file sizes and bound the reported duration
 *
 * The percentage may differ from the exact size ratio only by decimal rounding.
 * The duration is truncated to one unit and must fit within the independently
 * measured interval enclosing the complete captured vacuum call
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

	ASSERT(before_size > 0);
	ASSERT(after_size > 0);
	ASSERT(enclosing_elapsed_ns >= 0);
	ASSERT(metrics != NULL);
	ASSERT(3 == sscanf(metrics,"vacuumed: saved %Lf%%, elapsed %Lf%7s",
		&reported_savings,&reported_elapsed,elapsed_unit));
	ASSERT(reported_savings >=
		100.0L - 100.0L * (long double)after_size / (long double)before_size - 0.0051L);
	ASSERT(reported_savings <=
		100.0L - 100.0L * (long double)after_size / (long double)before_size + 0.0051L);

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

	ASSERT(duration_unit_index < duration_unit_count);
	ASSERT(reported_elapsed >= 0.0L);
	ASSERT(reported_elapsed * duration_units[duration_unit_index].nanoseconds <=
		(long double)enclosing_elapsed_ns);

	provide(status);
}

/**
 * @brief Verify that primary database vacuum reports reclaimed space
 *
 * A deleted large BLOB leaves free pages that vacuum must reclaim.
 * Reported savings and duration are checked against independent measurements.
 * The primary database is marked as modified while its startup metadata baseline
 * remains intact
 *
 * @return Return status for the numeric measurements and complete output checks
 */
static Return test0041_1(void)
{
	INITTEST;

	Config *initial_config = config;
	Config vacuum_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *initial_database_path = test0041_database_path;
	const Return initial_vacuum_status = test0041_vacuum_status;
	sqlite3 *database = NULL;
	struct stat before = {0};
	struct stat after = {0};
	const char *database_filename = "0041_primary_vacuum.db";

	m_create(char,database_path,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	vacuum_config.db_primary_file_path = m_init(char,MEMORY_STRING);
	vacuum_config.db_file_stat.st_size = 137;

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

	const int close_status = sqlite3_close(database);
	ASSERT(SQLITE_OK == close_status);

	ASSERT(SUCCESS == m_copy(&vacuum_config.db_primary_file_path,database_path));
	ASSERT(SUCCESS == get_file_stat(m_text(database_path),&before));

	config = &vacuum_config;
	rational_logger_mode = REGULAR;
	test0041_database_path = m_text(database_path);
	test0041_vacuum_status = FAILURE;
	const long long int capture_start_ns = cur_time_monotonic_ns();
	ASSERT(SUCCESS == function_capture(capture_vacuum,captured_stdout,captured_stderr));
	const long long int capture_elapsed_ns = cur_time_monotonic_ns() - capture_start_ns;

	ASSERT(SUCCESS == test0041_vacuum_status);
	ASSERT(captured_stderr->length == 0U);
	ASSERT(SUCCESS == get_file_stat(m_text(database_path),&after));
	ASSERT(after.st_size < before.st_size);
	ASSERT(SUCCESS == get_file_content("templates/0041_001.txt",pattern));
	ASSERT(SUCCESS == match_pattern(captured_stdout,pattern,"templates/0041_001.txt"));
	ASSERT(SUCCESS == check_metrics(captured_stdout,before.st_size,after.st_size,capture_elapsed_ns));
	ASSERT(vacuum_config.db_primary_file_modified == true);
	ASSERT(vacuum_config.db_file_stat.st_size == 137);

	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	test0041_database_path = initial_database_path;
	test0041_vacuum_status = initial_vacuum_status;

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
 * A separate database is compacted before the measured vacuum call.
 * File size must remain unchanged, the reported duration must fit within the
 * enclosing measurement, and primary database state must remain intact
 *
 * @return Return status for the numeric measurements and complete output checks
 */
static Return test0041_2(void)
{
	INITTEST;

	Config *initial_config = config;
	Config vacuum_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *initial_database_path = test0041_database_path;
	const Return initial_vacuum_status = test0041_vacuum_status;
	sqlite3 *database = NULL;
	struct stat before = {0};
	struct stat after = {0};
	const char *database_filename = "0041_non_primary_vacuum.db";

	m_create(char,database_path,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	vacuum_config.db_primary_file_path = m_init(char,MEMORY_STRING);
	vacuum_config.db_file_stat.st_size = 137;

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

	const int close_status = sqlite3_close(database);
	ASSERT(SQLITE_OK == close_status);

	ASSERT(SUCCESS == m_copy_literal(&vacuum_config.db_primary_file_path,"another_database.db"));
	ASSERT(SUCCESS == get_file_stat(m_text(database_path),&before));

	config = &vacuum_config;
	rational_logger_mode = REGULAR;
	test0041_database_path = m_text(database_path);
	test0041_vacuum_status = FAILURE;
	const long long int capture_start_ns = cur_time_monotonic_ns();
	ASSERT(SUCCESS == function_capture(capture_vacuum,captured_stdout,captured_stderr));
	const long long int capture_elapsed_ns = cur_time_monotonic_ns() - capture_start_ns;

	ASSERT(SUCCESS == test0041_vacuum_status);
	ASSERT(captured_stderr->length == 0U);
	ASSERT(SUCCESS == get_file_stat(m_text(database_path),&after));
	ASSERT(after.st_size == before.st_size);
	ASSERT(SUCCESS == get_file_content("templates/0041_002.txt",pattern));
	ASSERT(SUCCESS == match_pattern(captured_stdout,pattern,"templates/0041_002.txt"));
	ASSERT(SUCCESS == check_metrics(captured_stdout,before.st_size,after.st_size,capture_elapsed_ns));
	ASSERT(vacuum_config.db_primary_file_modified == false);
	ASSERT(vacuum_config.db_file_stat.st_size == 137);

	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	test0041_database_path = initial_database_path;
	test0041_vacuum_status = initial_vacuum_status;

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
 * @return Return status for both vacuum scenarios
 */
Return test0041(void)
{
	INITTEST;

	TEST(test0041_1,"Primary vacuum reclaims space and preserves the startup stat");
	TEST(test0041_2,"Non-primary vacuum reports zero savings for a compact database");

	RETURN_STATUS;
}
