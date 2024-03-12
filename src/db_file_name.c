#include "precizer.h"
#include <sys/utsname.h>

/**
 *
 * Determine file name of the database.
 * This database file name can be passed as an argument --database=FILE
 * Unless specified, the default database filename
 * will be the hostname and ".db" as the filename extension
 *
 */
Return db_file_name(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(config->db_file_name == NULL)
	{
		if(config->compare == true)
		{
			// In-memory database
			const char *inmemory_db_name = ":memory:";
			config->db_file_name = (char *)calloc(strlen(inmemory_db_name) + 1,sizeof(char));
			if(config->db_file_name == NULL)
			{
				slog(false,"ERROR: Memory allocation did not complete successfully!\n");
				status = FAILURE;
			} else {
				strcpy(config->db_file_name,inmemory_db_name);
			}
		} else {
			// File database
			struct utsname utsname;
			memset(&utsname,0,sizeof(utsname));
			uname(&utsname);
			config->db_file_name = (char *)calloc(strlen(utsname.nodename) + 4,sizeof(char));
			if(config->db_file_name == NULL)
			{
				slog(false,"ERROR: Memory allocation did not complete successfully!\n");
				status = FAILURE;
			} else {
				strcpy(config->db_file_name,utsname.nodename);
				strcat(config->db_file_name,".db");
			}
		}
	}

	// If the database file name has been determined and the database has not in-memory type
	if(config->db_file_name != NULL && (strcmp(config->db_file_name,":memory:") != 0))
	{
		slog(false,"Database file name: %s\n",config->db_file_name);
	}

	return(status);
}
