#include "precizer.h"

/**
 *
 * The VACUUM command rebuilds the database file,
 * repacking it into a minimal amount of disk space.
 *
 */
Return db_primary_consider_vacuum(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	// Don't do anything
	if(config->compare == true)
	{
		slog(TRACE,"Comparison mode is enabled. The primary database doesn't require vacuuming\n");
		provide(status);

	} else if(config->dry_run == true){
		slog(TRACE,"Dry Run mode is enabled. The primary database doesn't require vacuuming\n");
		provide(status);

	} else if(config->db_primary_file_modified == false){
		slog(TRACE,"No changes were made. The primary database doesn't require vacuuming\n");
		provide(status);
	}

	/* Vacuum the primary database */
	status = db_vacuum(config->db_primary_file_path);

	provide(status);
}
