#include "precizer.h"

/**
 *
 * Reflect global change status against database
 * @arg @c config Main global configuration structure
 *
 */
Return status_of_changes(void){
	/** @var Return status
	 *  @brief The status that will be passed to return() before exiting
	 *  @details By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(config->compare != true && global_interrupt_flag == false)
	{
		if(config->something_has_been_changed == false)
		{
			slog(EVERY,BOLD "Nothing have been changed against the database since the last probe (neither added nor updated or deleted information about files)" RESET "\n");
		} else {
			slog(EVERY,BOLD "The database %s has been modified since the last check (files were added, removed, or updated)" RESET "\n",config->db_file_name);
		}

		if(config->db_file_exists == true)
		{
			struct stat db_current_stat = {0};

			int rc = stat(config->db_file_path,&db_current_stat);

			if(rc < 0)
			{
				report("Stat of %s failed with error code: %d",config->db_file_path,rc);
				status = FAILURE;
			}

			if(memcmp(&db_current_stat,&(config->db_file_stat),sizeof(struct stat)) != 0)
			{
				slog(EVERY,BOLD "The database file %s has been modified since the program was launched" RESET "\n",config->db_file_name);
			} else {
				slog(EVERY,BOLD "The database file %s has NOT been modified since the program was launched" RESET "\n",config->db_file_name);
			}
		}
	}

	return(status);
}
