#include "precizer.h"

Return db_check_changes(void)
{
	/** @var Return status
	 *  @brief The status that will be passed to return() before exiting
	 *  @details By default, the function worked without errors
	 */
	Return status = SUCCESS;

	struct stat db_current_stat = {0};

	int rc = stat(config->db_primary_file_path,&db_current_stat);

	if(rc < 0)
	{
		report("Stat of %s failed with error code: %d",config->db_primary_file_path,rc);
		status = FAILURE;
	}

	CmpctStat before = {0};
	CmpctStat after = {0};

	stat_copy(&config->db_file_stat,&before);
	stat_copy(&db_current_stat,&after);

	Changed changes = compare_file_metadata_equivalence(&before,&after);

	if(IDENTICAL != changes)
	{
		if(config->db_primary_file_modified == true)
		{
			slog(EVERY,BOLD "The database file %s has been modified since the program was launched" RESET "\n",config->db_file_name);
		} else {
			slog(ERROR,"Internal error: The database file %s has changed, but according to the global variable tracking modification status, this should not have happened!\n",config->db_file_name);
			if(!(rational_logger_mode & SILENT))
			{
				show_difference(changes,&before,&after);
			}
			status = WARNING;
		}
	} else {
		if(config->db_primary_file_modified == true)
		{
			slog(ERROR,"Internal error. The database file %s has NOT changed, but according to the state of the global variable tracking modifications, it should have!\n",config->db_file_name);
			status = WARNING;
		} else {
			slog(EVERY,BOLD "The database file %s has NOT been modified since the program was launched" RESET "\n",config->db_file_name);
		}
	}

	provide(status);
}
