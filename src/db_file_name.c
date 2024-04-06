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

	if(config->compare == true || config->dry_run == true)
	{
		// In-memory database
		const char *inmemory_db_name = ":memory:";
		config->db_file_path = (char *)calloc(strlen(inmemory_db_name) + 1,sizeof(char));
		if(config->db_file_path == NULL)
		{
			slog(false,"ERROR: Memory allocation did not complete successfully!\n");
			status = FAILURE;
		} else {
			strcpy(config->db_file_path,inmemory_db_name);
		}
		config->db_file_name = (char *)calloc(strlen("disposable") + 1,sizeof(char));
		if(config->db_file_name == NULL)
		{
			slog(false,"ERROR: Memory allocation did not complete successfully!\n");
			status = FAILURE;
		} else {
			strcpy(config->db_file_name,"disposable");
		}
	} else {
		if(config->db_file_path == NULL)
		{
			// File database
			struct utsname utsname;
			memset(&utsname,0,sizeof(utsname));
			// Local host name
			uname(&utsname);

			// DB file will be named as "nameoflocalhost.db"
			config->db_file_path = (char *)calloc(strlen(utsname.nodename) + 4,sizeof(char));
			if(config->db_file_path == NULL)
			{
				slog(false,"ERROR: Memory allocation did not complete successfully!\n");
				status = FAILURE;
			} else {
				strcpy(config->db_file_path,utsname.nodename);
				strcat(config->db_file_path,".db");
			}

			// Copy to db_file_path to config->db_file_name
			if(config->db_file_path != NULL)
			{
				// Copy to db_file_name
				config->db_file_name = (char *)calloc(strlen(config->db_file_path) + 1,sizeof(char));
				if(config->db_file_name == NULL)
				{
					slog(false,"ERROR: Memory allocation did not complete successfully!\n");
					status = FAILURE;
				} else {
					strcpy(config->db_file_name,config->db_file_path);
				}
			}
		}
	}

	// If the database file name has been determined and the database has not in-memory type
	if(config->db_file_path != NULL && (strcmp(config->db_file_path,":memory:") != 0))
	{
		slog(false,"Database file name: %s\n",config->db_file_name);
	}

	return(status);
}
