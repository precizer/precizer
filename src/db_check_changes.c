#include "precizer.h"
#include <utime.h>

#ifdef TESTITALL_TEST_HOOKS
/**
 * @brief Test-only hook to simulate unexpected DB metadata change in dry-run mode.
 *
 * When enabled by environment variable
 * `PRECIZER_TEST_DB_FILE_TIMESTAMPS_WILL_BUMPED=true`, this helper updates
 * database file timestamps via utime(), forcing metadata drift before the final
 * `stat()` comparison in db_check_changes().
 *
 * The hook is intentionally limited to dry-run on an existing primary DB file.
 */
static Return run_test_hook_bump_db_timestamps(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	const char *flag_value = getenv("PRECIZER_TEST_DB_FILE_TIMESTAMPS_WILL_BUMPED");

	if(flag_value == NULL || strcmp(flag_value,"true") != 0)
	{
		return(status);
	}

	if(config->dry_run != true)
	{
		slog(ERROR,"Test hook failed: PRECIZER_TEST_DB_FILE_TIMESTAMPS_WILL_BUMPED requires --dry-run\n");
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
 * @brief Test-only hook to simulate missing DB metadata drift during update mode.
 *
 * When enabled by environment variable
 * `PRECIZER_TEST_DB_FILE_STAT_WILL_BE_RESYNCED=true`, this helper overwrites
 * the saved baseline stat (`config->db_file_stat`) with the current DB file
 * stat. This forces metadata comparison in db_check_changes() to report
 * IDENTICAL, even if the database was modified earlier in the same run.
 *
 * The hook is intentionally limited to non-dry-run mode and to cases where
 * `config->db_primary_file_modified` is already true.
 */
static Return run_test_hook_resync_db_stat_baseline(const struct stat *db_current_stat)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	const char *flag_value = getenv("PRECIZER_TEST_DB_FILE_STAT_WILL_BE_RESYNCED");

	if(flag_value == NULL || strcmp(flag_value,"true") != 0)
	{
		return(status);
	}

	if(config->dry_run == true)
	{
		slog(ERROR,"Test hook failed: PRECIZER_TEST_DB_FILE_STAT_WILL_BE_RESYNCED requires non-dry-run mode\n");
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
#endif

Return db_check_changes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	struct stat db_current_stat = {0};

#ifdef TESTITALL_TEST_HOOKS
	if(SUCCESS == status)
	{
		status = run_test_hook_bump_db_timestamps();
	}
#endif

	int rc = stat(confstr(db_primary_file_path),&db_current_stat);

	if(rc < 0)
	{
		report("Stat of %s failed with error code: %d",confstr(db_primary_file_path),rc);
		status = FAILURE;
	}

#ifdef TESTITALL_TEST_HOOKS
	if(SUCCESS == status)
	{
		status = run_test_hook_resync_db_stat_baseline(&db_current_stat);
	}
#endif

	CmpctStat before = {0};
	CmpctStat after = {0};

	run(stat_copy(&config->db_file_stat,&before));
	run(stat_copy(&db_current_stat,&after));

	Changed changes = compare_file_metadata_equivalence(&before,&after);

	if(IDENTICAL != changes)
	{
		if(config->db_primary_file_modified == true)
		{
			slog(EVERY,BOLD "The database file %s has been modified since the program was launched" RESET "\n",confstr(db_file_name));
		} else {
			slog(ERROR,"Internal error: The database file %s has changed, but according to the global variable tracking modification status, this should not have happened!\n",confstr(db_file_name));

			if(!(rational_logger_mode & SILENT))
			{
				show_difference(changes,&before,&after);
			}
			status = WARNING;
		}
	} else {
		if(config->db_primary_file_modified == true)
		{
			slog(ERROR,"Internal error. The database file %s has NOT changed, but according to the state of the global variable tracking modifications, it should have!\n",confstr(db_file_name));
			status = WARNING;
		} else {
			slog(EVERY,BOLD "The database file %s has NOT been modified since the program was launched" RESET "\n",confstr(db_file_name));
		}
	}

	provide(status);
}
