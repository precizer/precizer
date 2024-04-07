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
			slog(false,"\033[1mNothing have been changed against the database since the last probe (neither added nor updated or deleted information about files)\033[0m\n");
		} else {
			slog(false,"\033[1mThe database has been changed since the last probe (either added, removed, or updated information about files)\033[0m\n");
		}
	}
}
