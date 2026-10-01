#include "precizer.h"
#include <errno.h>

/**
 * @brief Rebuild a database and report the space saved and elapsed time
 *
 * The VACUUM command rebuilds the database file, repacking it into a minimal
 * amount of disk space. File sizes are measured immediately before and after
 * the SQL maintenance sequence, including its optimization steps. The reported
 * percentage is relative to the initial file size and is negative if the file
 * grows. An initially empty file has no defined percentage, reported as n/a
 *
 * @param[in] db_file_path Path to the database file to vacuum
 * @return SUCCESS on completion or in dry-run mode, otherwise FAILURE
 */
Return db_vacuum(const char *db_file_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	sqlite3 *db = NULL;
	char *err_msg = NULL;
	bool db_is_primary = false;
	bool db_file_modified = false;
	struct stat db_stat_before = {0};
	struct stat db_stat_after = {0};

	/* Validate input parameters */
	if(db_file_path == NULL)
	{
		slog(ERROR,"Invalid input parameters: db_file_path\n");
		provide(FAILURE);
	}

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. The database doesn't require vacuuming\n");
		provide(status);
	}

	if(strcmp(db_file_path,confstr(db_primary_file_path)) == 0)
	{
		db_is_primary = true;
	}

	/* Open database in safe mode */
	int rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Failed to open database");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(stat(db_file_path,&db_stat_before) != 0)
		{
			slog(ERROR,"Cannot stat database %s before vacuuming: %s\n",db_file_path,strerror(errno));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Create SQL statement */
		const char *sql =
		        "PRAGMA analyze;"
		        "PRAGMA optimize;"
		        "VACUUM;"
		        "PRAGMA analyze;"
		        "PRAGMA optimize;";

		if(db_is_primary == true)
		{
			slog(EVERY,"Start vacuuming the primary database…\n");
		} else {
			slog(EVERY,"Start vacuuming…\n");
		}

		/* Execute SQL statement */
		const long long int vacuum_start_ns = cur_time_monotonic_ns();
		rc = sqlite3_exec(db,sql,NULL,NULL,&err_msg);
		long long int vacuum_elapsed_ns = cur_time_monotonic_ns() - vacuum_start_ns;

		if(SQLITE_OK != rc)
		{

			log_sqlite_error(db,rc,err_msg,"Can't execute vacuum");
			status = FAILURE;

		} else {

			db_file_modified = true;

			if(stat(db_file_path,&db_stat_after) != 0)
			{
				slog(ERROR,"Cannot stat database %s after vacuuming: %s\n",db_file_path,strerror(errno));
				status = FAILURE;
			} else {
				if(vacuum_elapsed_ns < 0LL)
				{
					vacuum_elapsed_ns = 0LL;
				}

				char elapsed_string[50] = {0};
				(void)form_date_r(vacuum_elapsed_ns,MAJOR_VIEW,elapsed_string,sizeof(elapsed_string));

				const char *db_label = "DB";

				if(db_is_primary == true)
				{
					db_label = "Primary DB";
				}

				if(db_stat_before.st_size > 0)
				{
					const long double saved_percent =
					        ((long double)db_stat_before.st_size - (long double)db_stat_after.st_size)
					        * 100.0L / (long double)db_stat_before.st_size;

					slog(EVERY,"%s vacuumed: saved %.2Lf%%, elapsed %s\n",db_label,saved_percent,elapsed_string);
				} else {
					slog(EVERY,"%s vacuumed: saved n/a, elapsed %s\n",db_label,elapsed_string);
				}
			}
		}
	}

	if(db_file_modified == true)
	{
		/**
		 *
		 * If the database being updated is the primary one, adjust the global
		 * flag indicating that the main database file has been
		 * modified (this will consequently update the file's ctime, mtime, and size)
		 */
		if(db_is_primary == true)
		{
			/* Changes have been made to the database. Update
			   this in the global variable value. */
			config->db_primary_file_modified = true;
		}

	}

	/* Cleanup */
	call(db_close(db,&db_file_modified));

	provide(status);
}
