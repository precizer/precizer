#include "precizer.h"

/**
 *
 * Reflect global change status against database
 *
 */
Return status_of_changes(void)
{
	/** @var Return status
	 *  @brief The status that will be passed to return() before exiting
	 *  @details By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(config->compare != true && global_interrupt_flag == false)
	{
		if(config->db_primary_file_exists == true)
		{
			status = db_check_changes();

		} else {
			if(config->db_primary_file_modified == false)
			{
				slog(EVERY,BOLD "Nothing has changed in the primary database since the program was launched (no files were added, updated, or deleted)" RESET "\n");
			} else {
				slog(EVERY,BOLD "The brand-new primary database file %s was created and modified since the program started (files were added, removed, or updated)" RESET "\n",config->db_file_name);
			}
		}
	}

	provide(status);
}
