#include "precizer.h"

/**
 *
 * Reflect global change status against database
 * @arg @c config Main global configuration structure
 *
 */
void status_of_changes(void)
{

	if(config->compare != true
		&& config->something_has_been_changed == false
			&& global_interrupt_flag == false)
	{
		slog(false,"\033[1mNothing have been changed since the last probe (neither added nor updated or deleted files)\033[m\n");
	}
}
