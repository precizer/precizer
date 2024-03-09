#include "precizer.h"

/**
 *
 * Show global change status against database
 * @arg @c config Main configuration structure
 *
 */
void changes_status
(
	const Config *config
){

	if(config->compare != true
		&& config->something_has_been_changed == false)
	{
		slog(config,false,"\033[1mNothing have been changed since the last probe (neither added nor updated or deleted files)\033[m\n");
	}
}
