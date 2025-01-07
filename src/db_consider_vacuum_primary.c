#include "precizer.h"

/**
 *
 * The VACUUM command rebuilds the database file,
 * repacking it into a minimal amount of disk space.
 *
 */
Return db_consider_vacuum_primary(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		slog(TRACE,"Comparison mode is enabled. The primary database doesn't require vacuuming\n");
		return(status);

	} else if(config->dry_run == true){
		slog(TRACE,"Dry Run mode is enabled. The primary database doesn't require vacuuming\n");
		return(status);

	} else if(config->something_has_been_changed == false){
		slog(TRACE,"No changes were made. The primary database doesn't require vacuuming\n");
		return(status);
	}

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		return(status);
	}

	/* Vacuum the primary database */
	status = db_vacuum(config->db_file_path);

	return(status);
}
