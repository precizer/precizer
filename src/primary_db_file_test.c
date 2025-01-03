#include "precizer.h"

/**
 *
 * Validate the integrity of primary database file
 *
 */
Return primary_db_file_test(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Primary database file integrity check
	if(config->db_file_exists == true)
	{
		run(db_test(config->db_file_path));

	} else if(config->sqlite_open_flag & (SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE))
	{
		slog(TRACE,"The primary database file was just created and doesn't require verification\n");
	}

	return(status);
}
