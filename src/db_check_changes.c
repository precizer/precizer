#include "precizer.h"

/**
 * @brief Verify that the primary database file changed only when expected
 *
 * Compares the saved primary-database metadata captured earlier in the run with
 * the current metadata on disk. A real metadata change is expected only when
 * `config->db_primary_file_modified` is true. If the file changed without that
 * flag, or the flag is set but metadata did not change, the function reports an
 * internal consistency warning
 *
 * @return SUCCESS when the metadata and modification flag agree, WARNING when
 *         they disagree, or FAILURE when current database metadata cannot be read
 */
Return db_check_changes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	struct stat db_current_stat = {0};

#ifdef TESTITALL_TEST_HOOKS
	if(SUCCESS == status)
	{
		status = testitall_db_bump_timestamps();
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
		status = testitall_db_resync_stat_baseline(&db_current_stat);
	}
#endif

	CmpctStat before = {0};
	CmpctStat after = {0};

	run(stat_copy(&config->db_file_stat,&before));
	run(stat_copy(&db_current_stat,&after));

	Changed changes = file_compare_metadata_equivalence(&before,&after);

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
