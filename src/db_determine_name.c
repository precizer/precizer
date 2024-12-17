#include "precizer.h"
#include <sys/utsname.h>
#define IN_MEMORY_DB_NAME "DisposableDB"

/**
 *
 * Determine file name of the database.
 * This database file name can be passed as an argument --database=FILE
 * Unless specified, the default database filename
 * will be the hostname and ".db" as the filename extension
 *
 */
Return db_determine_name(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(config->compare == true)
	{
		if(config->db_file_path == NULL)
		{
			config->db_file_name = NULL;

			// In-memory database
			const char *in_memory_db_path = ":memory:";
			config->db_file_path = strdup(in_memory_db_path);
			config->db_in_memory = true;

			if(config->db_file_path == NULL)
			{
				report("Memory allocation failed for config->db_file_path");
				status = FAILURE;
			}

			if(SUCCESS == status)
			{
				config->db_file_name = strdup(IN_MEMORY_DB_NAME);

				if(config->db_file_name == NULL)
				{
					report("Memory allocation failed for config->db_file_name");
					free(config->db_file_path);
					config->db_file_path = NULL;
					status = FAILURE;
				}
			}
		} else {
			slog(ERROR,"General failure. config->db_file_path should be NULL in this case");
			status = FAILURE;
		}

	} else {

		if(config->db_file_path == NULL)
		{

			config->db_file_name = NULL;

			struct utsname utsname;
			memset(&utsname,0,sizeof(utsname));

			// Determine local host name
			if(uname(&utsname) != 0)
			{
				slog(ERROR,"Failed to get hostname\n");
				status = FAILURE;
			}

			if(SUCCESS == status)
			{
				// Create temporary string with full path
				if(asprintf(&config->db_file_path,"%s.db",utsname.nodename) == -1)
				{
					report("Failed to allocate memory for database path");
					status = FAILURE;
				}
			}

			if(SUCCESS == status)
			{
				// Copy the same path to db_file_name
				config->db_file_name = strdup(config->db_file_path);

				if(config->db_file_name == NULL)
				{
					report("Memory allocation failed, requested size: %zu bytes",strlen(config->db_file_path) + 1 * sizeof(char));
					free(config->db_file_path);
					config->db_file_path = NULL;
					status = FAILURE;
				}
			}
		}
	}

	// Log message if the database file name has been determined and
	if(SUCCESS == status && config->db_file_path != NULL && config->db_file_path != NULL)
	{
		slog(EVERY,"Database file name: %s\n",config->db_file_name);
		slog(TRACE,"Database file path: %s\n",config->db_file_path);
		slog(TRACE,"In-memory database: %s\n",config->db_in_memory ? "yes" : "no");
	}

	slog(TRACE,"DB name determined\n");

	return(status);
}
