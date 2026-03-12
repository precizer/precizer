#include "precizer.h"
#define IN_MEMORY_DB_NAME "DisposableDB"

/**
 * Determine file name of the database.
 * This database file name can be passed as an argument --database=FILE
 * Unless specified, the default database filename
 * will be the hostname and ".db" as the filename extension
 */
Return db_determine_name(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	if(config->compare == true)
	{
		if(config->db_primary_file_path.length == 0)
		{
			// In-memory database
			const char in_memory_db_path[] = ":memory:";
			config->db_primary_path_is_memory = true;

			// Set primary DB path to SQLite in-memory marker.
			run(copy_buffer(conf(db_primary_file_path),in_memory_db_path,sizeof(in_memory_db_path)));
			// Set display-friendly DB name for logs.
			run(copy_literal(conf(db_file_name),IN_MEMORY_DB_NAME));

			if(CRITICAL & status)
			{
				slog(ERROR,"Failed to initialize in-memory database names\n");
				call(del(conf(db_primary_file_path)));
			}
		} else {
			slog(ERROR,"General failure. config->db_primary_file_path must be blank for this case\n");
			status = FAILURE;
		}

	} else {

		if(config->db_primary_file_path.length == 0)
		{
			call(del(conf(db_file_name)));
			config->db_primary_path_is_memory = false;

			struct utsname utsname = {0};
			const char db_file_extension[] = ".db";

			// Determine local host name
			if(uname(&utsname) != 0)
			{
				slog(ERROR,"Failed to get hostname\n");
				status = FAILURE;
			} else {
				// Build default DB path from hostname plus ".db" suffix.
				run(copy_cstring(conf(db_primary_file_path),utsname.nodename,sizeof(utsname.nodename)));
				run(concat_cstring(conf(db_primary_file_path),db_file_extension,sizeof(db_file_extension)));
				// Copy the same path to db_file_name.
				run(copy(conf(db_file_name),conf(db_primary_file_path)));

				if(CRITICAL & status)
				{
					slog(ERROR,"Failed to build default database name from hostname\n");
					call(del(conf(db_primary_file_path)));
				}
			}
		}
	}

	// Log message when database file is specified and confirmed as persistent storage (non-memory database)
	if(TRIUMPH & status)
	{
		if(config->db_primary_path_is_memory == false || ((rational_logger_mode & REGULAR) == 0))
		{
			slog(EVERY,"Primary database file name: %s\n",confstr(db_file_name));
		}

		slog(TRACE,"Primary database file path: %s\n",confstr(db_primary_file_path));
	}

	slog(TRACE,"Finished determining database name\n");

	provide(status);
}
