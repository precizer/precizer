#include "precizer.h"

/**
 *
 * Reflect global change status against database
 * @arg @c config Main global configuration structure
 *
 */
void status_of_changes(void)
{

	if(config->compare != true && global_interrupt_flag == false)
	{
		if(config->something_has_been_changed == false)
		{
			slog(EVERY, BOLD "Nothing have been changed against the database since the last probe (neither added nor updated or deleted information about files)" RESET "\n");
		} else {
			slog(EVERY, BOLD "The database %s has been modified since the last check (files were added, removed, or updated)" RESET "\n",config->db_file_name);
		}
	}
}
