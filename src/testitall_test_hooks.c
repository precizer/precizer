#include "precizer.h"

#ifdef TESTITALL_TEST_HOOKS
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <utime.h>

static const char *const signal_wait_ready_fd_env_name = "TESTITALL_SIGNAL_WAIT_READY_FD";
static const int testitall_hash_checkpoint_exit_code_default = 77;

/**
 * @brief Check whether a boolean test hook environment flag is enabled
 *
 * Only the exact string `true` enables the hook. Empty values, missing
 * variables, and all other values leave the hook disabled
 *
 * @param[in] env_name Environment variable name to inspect
 * @return true when the flag is explicitly enabled, otherwise false
 */
static bool testitall_env_flag_is_true(const char *env_name)
{
	const char *flag_value = NULL;

	if(env_name == NULL)
	{
		return(false);
	}

	flag_value = getenv(env_name);

	if(flag_value == NULL || strcmp(flag_value,"true") != 0)
	{
		return(false);
	}

	return(true);
}

/**
 * @brief Notify the background test parent that the configured wait point was reached
 *
 * The notification descriptor is supplied by runit_background() only for
 * zero-min-delay scenarios. The helper closes the descriptor after one
 * notification so repeated visits to the same wait point do not emit another
 * readiness byte
 */
static void signal_wait_notify_ready(void)
{
	const char *ready_fd_text = getenv(signal_wait_ready_fd_env_name);

	if(NULL == ready_fd_text || '\0' == ready_fd_text[0])
	{
		return;
	}

	errno = 0;
	char *end_ptr = NULL;
	unsigned long long parsed_ready_fd = strtoull(ready_fd_text,&end_ptr,10);

	if(errno != 0
	        || end_ptr == ready_fd_text
	        || '\0' != *end_ptr
	        || parsed_ready_fd > (unsigned long long)INT_MAX)
	{
		return;
	}

	const int ready_fd = (int)parsed_ready_fd;
	const char notification = 'R';
	ssize_t bytes_written = 0;

	do {
		bytes_written = write(ready_fd,&notification,sizeof(notification));
	} while(bytes_written == -1 && errno == EINTR);

	(void)close(ready_fd);
	(void)unsetenv(signal_wait_ready_fd_env_name);
}

/**
 * @brief Pause a test run at a configured wait point
 *
 * Used by signal-driven tests to delay a known execution point. The wait point
 * and duration are selected with `TESTITALL_SIGNAL_WAIT_POINT` and
 * `TESTITALL_SIGNAL_WAIT_MS`. The delay ends early when `global_interrupt_flag`
 * is set. When `TESTITALL_SIGNAL_WAIT_READY_FD` is present, the function first
 * notifies runit_background() that the configured wait point was reached
 *
 * @param point_id Wait point identifier reached by the caller
 */
void signal_wait_at_point(unsigned int point_id)
{
	const char *configured_point = getenv("TESTITALL_SIGNAL_WAIT_POINT");

	if(NULL == configured_point || '\0' == configured_point[0])
	{
		return;
	}

	errno = 0;
	char *point_end_ptr = NULL;
	unsigned long long parsed_point_id = strtoull(configured_point,&point_end_ptr,10);

	if(errno != 0 || point_end_ptr == configured_point || '\0' != *point_end_ptr)
	{
		return;
	}

	if(parsed_point_id != (unsigned long long)point_id)
	{
		return;
	}

	signal_wait_notify_ready();

	const char *timeout_text = getenv("TESTITALL_SIGNAL_WAIT_MS");

	if(NULL == timeout_text || '\0' == timeout_text[0])
	{
		return;
	}

	errno = 0;
	char *end_ptr = NULL;
	unsigned long long parsed_timeout_ms = strtoull(timeout_text,&end_ptr,10);

	if(errno != 0 || end_ptr == timeout_text || '\0' != *end_ptr || parsed_timeout_ms == 0ULL)
	{
		return;
	}

	uint64_t remaining_timeout_ms = (uint64_t)parsed_timeout_ms;

	while(remaining_timeout_ms > 0U)
	{
		/* Allow tests to release the delay as soon as the signal handler sets the flag. */
		if(atomic_load(&global_interrupt_flag) == true)
		{
			return;
		}

		uint64_t chunk_ms = remaining_timeout_ms;

		if(chunk_ms > 10U)
		{
			chunk_ms = 10U;
		}

		struct timespec delay = {
			.tv_sec = 0,
			.tv_nsec = (long)(chunk_ms * 1000000ULL)
		};

		while(nanosleep(&delay,&delay) == -1 && errno == EINTR)
		{
			if(atomic_load(&global_interrupt_flag) == true)
			{
				return;
			}
		}

		remaining_timeout_ms -= chunk_ms;
	}
}

/**
 * @brief Check whether a path matches a configured test suffix
 *
 * Accepts an exact match or a suffix that begins after a path separator, so a
 * relative test target can match both relative and absolute runtime paths
 *
 * @param[in] path Runtime path to inspect
 * @param[in] suffix Test suffix to match
 * @return true when the path matches the suffix, otherwise false
 */
static bool testitall_path_matches_suffix(
	const char *path,
	const char *suffix)
{
	size_t path_len = 0;
	size_t suffix_len = 0;

	if(path == NULL || suffix == NULL)
	{
		return false;
	}

	if(strcmp(path,suffix) == 0)
	{
		return true;
	}

	path_len = strlen(path);
	suffix_len = strlen(suffix);

	if(path_len < suffix_len + 1U)
	{
		return false;
	}

	if(path[path_len - suffix_len - 1U] != '/')
	{
		return false;
	}

	return strcmp(path + (path_len - suffix_len),suffix) == 0;
}

/**
 * @brief Apply a test-requested filesystem access status override
 *
 * Reads the target suffix and forced status from the test environment. When
 * both values are present and the path matches, the requested status is written
 * to @p access_status_out
 *
 * @param[in] path Runtime path being checked
 * @param[out] access_status_out Receives the forced access status
 * @return true when an override was applied, otherwise false
 */
bool testitall_file_access_status_override(
	const char       *path,
	FileAccessStatus *access_status_out)
{
	const char *target_suffix = getenv("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX");
	const char *forced_status = getenv("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS");

	if(path == NULL
	        || access_status_out == NULL
	        || target_suffix == NULL
	        || forced_status == NULL
	        || target_suffix[0] == '\0'
	        || forced_status[0] == '\0')
	{
		return false;
	}

	if(testitall_path_matches_suffix(path,target_suffix) == false)
	{
		return false;
	}

	if(strcmp(forced_status,"FILE_ACCESS_ALLOWED") == 0)
	{
		*access_status_out = FILE_ACCESS_ALLOWED;

	} else if(strcmp(forced_status,"FILE_ACCESS_DENIED") == 0){
		*access_status_out = FILE_ACCESS_DENIED;

	} else if(strcmp(forced_status,"FILE_NOT_FOUND") == 0){
		*access_status_out = FILE_NOT_FOUND;

	} else if(strcmp(forced_status,"FILE_ACCESS_ERROR") == 0){
		*access_status_out = FILE_ACCESS_ERROR;

	} else {
		slog(ERROR,"Test hook failed: unsupported TESTITALL_TEST_ENV_FILE_ACCESS_STATUS value %s\n",forced_status);
		*access_status_out = FILE_ACCESS_ERROR;
	}

	return true;
}

/**
 * @brief Simulate an unexpected database metadata change in dry-run mode
 *
 * When enabled by environment variable
 * `TESTITALL_TEST_ENV_DB_FILE_TIMESTAMPS_WILL_BUMPED=true`, this helper updates
 * database file timestamps via utime(), forcing metadata drift before the final
 * `stat()` comparison in db_check_changes()
 *
 * The hook is intentionally limited to dry-run on an existing primary DB file
 *
 * @return SUCCESS when the hook is inactive or updated the timestamps,
 *         otherwise FAILURE
 */
Return testitall_db_bump_timestamps(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	const char *flag_value = getenv("TESTITALL_TEST_ENV_DB_FILE_TIMESTAMPS_WILL_BUMPED");

	if(flag_value == NULL || strcmp(flag_value,"true") != 0)
	{
		return(status);
	}

	if(config->dry_run != true)
	{
		slog(ERROR,"Test hook failed: TESTITALL_TEST_ENV_DB_FILE_TIMESTAMPS_WILL_BUMPED requires --dry-run\n");
		return(FAILURE);
	}

	if(config->db_primary_file_exists != true)
	{
		return(status);
	}

	const char *db_path = confstr(db_primary_file_path);

	if(db_path == NULL || db_path[0] == '\0')
	{
		slog(ERROR,"Test hook failed: database path is empty\n");
		return(FAILURE);
	}

	if(utime(db_path,NULL) != 0)
	{
		slog(ERROR,"Test hook failed: unable to bump timestamps for %s\n",db_path);
		return(FAILURE);
	}

	slog(TESTING,"Test hook: database file timestamps were bumped for %s\n",confstr(db_file_name));

	return(status);
}

/**
 * @brief Simulate missing database metadata drift during update mode
 *
 * When enabled by environment variable
 * `TESTITALL_TEST_ENV_DB_FILE_STAT_WILL_BE_RESYNCED=true`, this helper overwrites
 * the saved baseline stat (`config->db_file_stat`) with the current DB file
 * stat. This forces metadata comparison in db_check_changes() to report
 * IDENTICAL, even if the database was modified earlier in the same run
 *
 * The hook is intentionally limited to non-dry-run mode and to cases where
 * `config->db_primary_file_modified` is already true
 *
 * @param[in] db_current_stat Current metadata for the primary database file
 * @return SUCCESS when the hook is inactive or resynchronized the baseline,
 *         otherwise FAILURE
 */
Return testitall_db_resync_stat_baseline(const struct stat *db_current_stat)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	const char *flag_value = getenv("TESTITALL_TEST_ENV_DB_FILE_STAT_WILL_BE_RESYNCED");

	if(flag_value == NULL || strcmp(flag_value,"true") != 0)
	{
		return(status);
	}

	if(config->dry_run == true)
	{
		slog(ERROR,"Test hook failed: TESTITALL_TEST_ENV_DB_FILE_STAT_WILL_BE_RESYNCED requires non-dry-run mode\n");
		return(FAILURE);
	}

	if(config->db_primary_file_exists != true)
	{
		return(status);
	}

	if(config->db_primary_file_modified != true)
	{
		slog(ERROR,"Test hook failed: baseline resync requested, but database modification flag is false\n");
		return(FAILURE);
	}

	if(db_current_stat == NULL)
	{
		slog(ERROR,"Test hook failed: current database stat is unavailable\n");
		return(FAILURE);
	}

	/*
	 * Intentionally corrupt the comparison baseline for test coverage:
	 * make "before" equal to "after" even after a real DB update.
	 */
	config->db_file_stat = *db_current_stat;

	slog(TESTING,"Test hook: baseline database stat was resynced to current metadata for %s\n",confstr(db_file_name));

	return(status);
}

/**
 * @brief Check whether a path points to the large interruption test file
 *
 * @param[in] path Path being hashed during a test build
 * @return true when the path ends with the interruption fixture name
 */
bool testitall_is_huge_interruption_target(const char *path)
{
	if(path == NULL)
	{
		return(false);
	}

	const char *needle = "hugetestfile";
	const size_t path_length = strlen(path);
	const size_t needle_length = strlen(needle);

	if(path_length < needle_length)
	{
		return(false);
	}

	return(0 == strcmp(path + (path_length - needle_length),needle));
}

/**
 * @brief Generate a pseudo-random stop byte in the closed range [1, file_size]
 *
 * @param[in] file_size File size used as the inclusive upper bound
 * @return Selected byte offset, or zero when @p file_size is zero
 */
uint64_t testitall_random_stop_byte(const uint64_t file_size)
{
	if(file_size == 0U)
	{
		return(0U);
	}

	struct timespec now = {0};
	(void)clock_gettime(CLOCK_MONOTONIC,&now);

	uint64_t seed = (uint64_t)now.tv_nsec;
	seed ^= ((uint64_t)now.tv_sec << 32);
	seed ^= (uint64_t)getpid();

	return((seed % file_size) + 1U);
}

/**
 * @brief Check whether SHA512 checkpoint tests should checkpoint at a random byte
 *
 * Enabled by `TESTITALL_TEST_ENV_HASH_CHECKPOINT_AT_RANDOM_BYTE=true`
 *
 * @return true when the hook is enabled, otherwise false
 */
bool testitall_hash_checkpoint_at_random_byte_enabled(void)
{
	return(testitall_env_flag_is_true("TESTITALL_TEST_ENV_HASH_CHECKPOINT_AT_RANDOM_BYTE"));
}

/**
 * @brief Check whether SHA512 checkpoint tests should terminate after checkpoint
 *
 * Enabled by `TESTITALL_TEST_ENV_EXIT_AFTER_HASH_CHECKPOINT=true`
 *
 * @return true when the hook is enabled, otherwise false
 */
bool testitall_exit_after_hash_checkpoint_enabled(void)
{
	return(testitall_env_flag_is_true("TESTITALL_TEST_ENV_EXIT_AFTER_HASH_CHECKPOINT"));
}

/**
 * @brief Terminate the current process after a test-controlled hash checkpoint
 *
 * The exit status is read from `TESTITALL_TEST_ENV_HASH_CHECKPOINT_EXIT_CODE`.
 * Invalid, empty, zero, and out-of-range values fall back to 77 so the process
 * never exits successfully by accident
 */
void testitall_exit_after_hash_checkpoint(void)
{
	int exit_code = testitall_hash_checkpoint_exit_code_default;
	const char *exit_code_text = getenv("TESTITALL_TEST_ENV_HASH_CHECKPOINT_EXIT_CODE");

	if(exit_code_text != NULL && exit_code_text[0] != '\0')
	{
		errno = 0;
		char *end_ptr = NULL;
		const long parsed_exit_code = strtol(exit_code_text,&end_ptr,10);

		if(errno == 0
		        && end_ptr != exit_code_text
		        && *end_ptr == '\0'
		        && parsed_exit_code > 0L
		        && parsed_exit_code <= 255L)
		{
			exit_code = (int)parsed_exit_code;
		}
	}

	slog(TESTING,"Test hook: exiting after hash checkpoint with code %d\n",exit_code);
	exit(exit_code);
}
#endif
