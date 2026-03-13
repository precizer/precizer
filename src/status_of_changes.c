#include "precizer.h"

/**
 * @brief Report whether the primary database changed during the current run
 *
 * @details Skips output in compare mode and after an interrupt. For an existing
 * primary database the function delegates to `db_check_changes()`. For a
 * brand-new primary database it prints whether any rows were added, removed, or
 * updated since startup
 *
 * @return Return status code
 */
Return status_of_changes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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
				slog(EVERY,BOLD "The brand-new primary database file %s was created and modified since the program started (files were added, removed, or updated)" RESET "\n",confstr(db_file_name));
			}
		}
	}

	provide(status);
}
