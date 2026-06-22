#include "precizer.h"
#include <errno.h>

/**
 * @brief Check whether an unavailable file must be reported as a lock violation
 *
 * @param[in] relative_path Relative path descriptor from the database
 * @param[in] access_status Current unavailable status for the path
 *
 * @return Return status code:
 *         - SUCCESS|YES: A checksum-locked unavailable violation was reported
 *         - SUCCESS|NO: No checksum-locked unavailable violation was reported
 *         - FAILURE|YES: A checksum-locked unavailable violation was detected, but reporting failed
 *         - FAILURE|NO: Lock-checksum evaluation or reporting failed
 */
static Return db_check_locked_unavailable_violation(
	const memory           *relative_path,
	const FileAccessStatus access_status)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

  /* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	bool locked_unavailable_violation = false;

	/*
	 * YES means the unavailable path is checksum-locked, so it must be reported as a violation.
	 * NO means the path is not checksum-locked, so this unavailable state is not a lock violation
	 */
	if(ask(path_check_locked_checksum(relative_path)))
	{
		status = show_locked_checksum_unavailable_violation(relative_path,
			access_status,
			NULL,
			NULL);

		locked_unavailable_violation = true;
	}

	if(locked_unavailable_violation == true)
	{
		status |= YES;

	} else {
		status |= NO;
	}

	provide(status);
}

/**
 * @brief Keep checksum-locked rows even when --db-drop-ignored matched them
 *
 * The function checks the locked path relative to an already opened root
 * directory. An unavailable locked path is reported and kept
 *
 * @param[in] root_directory_fd Open descriptor for the root directory. Used
 *            as the base for the relative access check
 * @param[in] relative_path Relative path descriptor from the database
 * @param[out] locked_unavailable_violation_out Set to true when this call reported a locked unavailable path
 *
 * @return Return status code:
 *         - SUCCESS|YES: The row must stay in the DB
 *         - SUCCESS|NO: The row does not need checksum-lock preservation
 *         - FAILURE|NO: Validation, lock-checksum evaluation, or reporting failed
 */
static Return db_preserve_locked_ignored_record(
	const int    root_directory_fd,
	const memory *relative_path,
	bool         *locked_unavailable_violation_out)
{
	/* This changed function requires a new line-by-line human review before it is considered trusted */

  /* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(locked_unavailable_violation_out == NULL)
	{
		provide(FAILURE | NO);
	}

	*locked_unavailable_violation_out = false;

	bool keep_locked_record = false;

	/*
	 * YES means the ignored DB row is checksum-locked and must be checked for preservation.
	 * NO means the row is not checksum-locked, so this helper does not preserve it
	 */
	if(ask(path_check_locked_checksum(relative_path)))
	{
		/*
		 * Check the path relative to the open root directory without
		 * constructing an absolute path
		 */
		FileAccessStatus access_status = file_check_access(
			root_directory_fd,
			m_text(relative_path),
			R_OK);

		if(SUCCESS == status && access_status != FILE_ACCESS_ALLOWED)
		{
			/*
			 * YES means the unavailable checksum-locked row was reported as a violation.
			 * NO means no violation was reported for this unavailable row
			 */
			if(ask(db_check_locked_unavailable_violation(relative_path,access_status)))
			{
				*locked_unavailable_violation_out = true;
			}

		}

		if(TRIUMPH & status)
		{
			keep_locked_record = true;
		}
	}

	if(keep_locked_record == true)
	{
		status |= YES;

	} else {
		status |= NO;
	}

	provide(status);
}

/**
 * @brief Remove stale file records from the primary database after an update
 *
 * @details
 * The function walks through file records stored in the primary database and
 * decides whether each row should remain there
 *
 * One root prefix is retrieved from the `paths` table and opened once. Every
 * file row is checked relative to that same directory descriptor. The current
 * cleanup path therefore requires the database to describe a single root and
 * does not distinguish file rows belonging to different stored roots
 *
 * A row can be removed when the file is no longer present on disk, when it is
 * inaccessible or its access check fails and `--db-drop-inaccessible` is active,
 * or when the path is ignored and `--db-drop-ignored` is enabled
 *
 * If the root cannot be opened for any reason, its cleanup is skipped without
 * changing file rows or returning an error. This prevents a temporary root
 * access problem from being mistaken for missing or inaccessible files
 *
 * Checksum-locked paths are file paths matched by a `--lock-checksum` pattern.
 * Their stored checksums serve as protected integrity references, so cleanup
 * must not silently remove their database rows when the files cannot be verified
 * If a locked file is missing or unavailable, the function keeps the database
 * row, reports the violation, and returns a warning instead of deleting it
 * A NULL `files.relative_path` value violates the database contract and stops
 * cleanup with `FAILURE`
 *
 * In `--compare` mode, or when `--update` is not active, the function exits
 * without performing cleanup
 * In `--dry-run` mode it evaluates the same cleanup decisions without deleting
 * rows from the database
 *
 * @return Return status code:
 *         - SUCCESS: Cleanup completed without lock-checksum warnings
 *         - WARNING: One or more unavailable checksum-locked paths were found
 *         - FAILURE: Cleanup was interrupted by an internal error
 */
Return db_delete_missing_metadata(void)
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

	/* Skip in comparison mode */
	if(config->compare == true)
	{
		slog(TRACE,"Comparison mode is enabled. The primary database does not require cleanup\n");
		provide(status);
	}

	/* Update mode should be enabled */
	if(config->update == true)
	{
		slog(EVERY,"Searching for files that no longer exist on the file system…\n");

	} else {
		// Don't do anything
		provide(status);
	}

	if(config->dry_run == true && config->db_primary_file_exists == true)
	{
		slog(TRACE,"Dry-run mode is enabled. The primary database will remain unchanged\n");
	}

	// Print deletion banners only once per run
	bool first_iteration = true;

	// Tracks whether cleanup reported an unavailable checksum-locked path
	bool locked_unavailable_violation_detected = false;

	// Stores the root directory path retrieved from the database
	m_create(char,root_path,MEMORY_STRING);

	// Stores the relative path of the file record currently being processed
	m_create(char,relative_path,MEMORY_STRING);

	// Holds the prepared SQLite statement used to iterate over file records
	sqlite3_stmt *select_stmt = NULL;

	// Stores the result code returned by the most recent SQLite operation
	int rc = 0;

	// Identifies the open root directory used as the base for relative access checks
	int root_directory_fd = -1;

	/*
	 * Load the shared root path and open it once before processing file rows.
	 * Each file record stores only a relative path, so access checks use this directory descriptor
	 */
	run(db_retrieve_root_path(root_path));

	if(SUCCESS == status)
	{
		const char *runtime_root_path = m_text(root_path);

		const FileAccessStatus root_access_status = directory_open(runtime_root_path,&root_directory_fd);

		if(root_access_status != FILE_ACCESS_ALLOWED)
		{
			const int root_open_errno = errno;

			slog(EVERY,
				"Skipping metadata cleanup for unavailable root %s: %s\n",
				runtime_root_path,
				strerror(root_open_errno));
		}
	}

	/*
	 * Without an open root, file rows cannot be classified safely.
	 * Release local memory and keep the current status, which remains SUCCESS
	 * when root opening alone was unavailable
	 */
	if(root_directory_fd < 0)
	{
		call(m_del(relative_path));
		call(m_del(root_path));

		provide(status);
	}

	if(SUCCESS == status)
	{
#if 0 // Disabled multi-root path index implementation
		const char *select_sql = "SELECT files.ID,paths.prefix,files.relative_path FROM files LEFT JOIN paths ON files.root_path_index = paths.ID;";
#endif
		const char *select_sql = "SELECT ID,relative_path FROM files;";

		rc = sqlite3_prepare_v2(config->db,select_sql,-1,&select_stmt,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement");
			status = FAILURE;
		}
	}

	/*
	 * Iterate over every file record selected from the files table
	 */
	while(SUCCESS == status && SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		/* Interrupt the loop smoothly */
		/* Interrupt when Ctrl+C */
		if(global_interrupt_flag == true)
		{
			break;
		}

		sqlite_int64 ID = sqlite3_column_int64(select_stmt,0);

		const unsigned char *db_relative_path = sqlite3_column_text(select_stmt,1);

		// Marks deletions triggered by --db-drop-ignored
		bool drop_ignored = false;

		// Marks deletions triggered by unavailable access when --db-drop-inaccessible is active
		bool inaccessible = false;

		// Marks deletions triggered by a missing path on disk
		bool file_not_found = false;

		// Aggregates whether the current record should be deleted for any reason
		bool should_delete = false;

		/*
		 * files.relative_path is required by the database contract to be non-NULL.
		 * This is the single boundary check before the helper chain below relies on that invariant
		 */
		if(db_relative_path == NULL)
		{
			slog(ERROR,"The files table returned a NULL relative path\n");
			status = FAILURE;
			break;
		}

		/*
		 * sqlite3_column_bytes() returns the relative path length without the
		 * terminating NUL byte, so add one byte to copy a complete C string
		 */
		run(m_copy_fixed_string(relative_path,(size_t)sqlite3_column_bytes(select_stmt,1) + 1U,db_relative_path));

		if(SUCCESS != status)
		{
			break;
		}

		const char *runtime_relative_path = m_text(relative_path);

		/*
		 * Remove from the database mention of
		 * files that matches the regular expression
		 * passed through the ignore option(s)
		 *
		 */
		if(config->db_drop_ignored == true)
		{
			status = match_include_ignore(relative_path,NULL,&drop_ignored);

			if(SUCCESS != status)
			{
				break;
			}
		}

		if(drop_ignored == true)
		{
			bool locked_unavailable_violation = false;

			/*
			 * YES means the ignored DB row must be kept because it is checksum-locked.
			 * NO means checksum-lock protection does not apply here, so the ignored row may be deleted
			 */
			if(ask(db_preserve_locked_ignored_record(root_directory_fd,relative_path,&locked_unavailable_violation)))
			{
				if(locked_unavailable_violation == true)
				{
					locked_unavailable_violation_detected = true;
				}

				continue;
			}

			if(SUCCESS != status)
			{
				break;
			}

			should_delete = true;

		} else {

			/*
			 * This changed access-check block requires a new line-by-line human
			 * review before it is considered trusted
			 *
			 * Check the path relative to the open root directory without
			 * constructing an absolute path
			 */
			FileAccessStatus access_status = file_check_access(root_directory_fd,m_text(relative_path),R_OK);

			if(access_status == FILE_ACCESS_ALLOWED)
			{
				continue;

			} else if(access_status == FILE_ACCESS_DENIED || access_status == FILE_ACCESS_ERROR){

				/*
				 * YES means the unavailable DB row is checksum-locked and was reported as a violation.
				 * NO means no checksum-lock violation was reported, so normal inaccessible handling continues
				 */
				if(ask(db_check_locked_unavailable_violation(relative_path,access_status)))
				{
					locked_unavailable_violation_detected = true;
					continue;
				}

				if(SUCCESS != status)
				{
					break;
				}

				if(config->db_drop_inaccessible == true)
				{
					inaccessible = true;
					should_delete = true;

				} else {
					slog(EVERY|UNDECOR,"kept inaccessible %s\n",runtime_relative_path);
					continue;
				}

			} else if(access_status == FILE_NOT_FOUND){

				/*
				 * YES means the missing DB row is checksum-locked and was reported as a violation.
				 * NO means no checksum-lock violation was reported, so normal missing-file handling continues
				 */
				if(ask(db_check_locked_unavailable_violation(relative_path,access_status)))
				{
					locked_unavailable_violation_detected = true;
					continue;
				}

				if(SUCCESS != status)
				{
					break;
				}

				file_not_found = true;
				should_delete = true;

			}
		}

		if(should_delete == true)
		{
			if(first_iteration == true)
			{
				first_iteration = false;

				if(config->ignore != NULL)
				{
					if(config->db_drop_ignored == false)
					{
						slog(EVERY,"If the information about ignored files should be removed from the database the " BOLD "--db-drop-ignored" RESET " option must be specified. This is special protection against accidental deletion of information from the database\n");
					} else {
						slog(TRACE,"The " BOLD "--db-drop-ignored" RESET " option has been used, so the information about ignored files will be removed against the database %s\n",confstr(db_file_name));
					}
				}

				if(config->db_drop_inaccessible)
				{
					slog(EVERY,BOLD "Dropping DB records for missing, inaccessible, or ignored paths in %s:" RESET "\n",confstr(db_file_name));
				} else {
					slog(EVERY,BOLD "Dropping DB records for missing or ignored paths in %s:" RESET "\n",confstr(db_file_name));
				}
			}

			status = db_delete_the_record_by_id(&ID);

			if(SUCCESS != status)
			{
				break;
			}

			if(drop_ignored == true)
			{
				slog(EVERY|UNDECOR|REMEMBER,"drop ignored %s\n",runtime_relative_path);

			} else if(inaccessible == true){
				slog(EVERY|UNDECOR|REMEMBER,"drop due to inaccessible %s\n",runtime_relative_path);

			} else if(file_not_found == true){
				slog(EVERY|UNDECOR,"no longer exists %s\n",runtime_relative_path);
			}
		}
	}

	if(SUCCESS == status && SQLITE_DONE != rc)
	{
		if(global_interrupt_flag == false)
		{
			log_sqlite_error(config->db,rc,NULL,"Select statement didn't finish with DONE");
			status = FAILURE;
		}
	}

	rc = sqlite3_finalize(select_stmt);

	if(SUCCESS == status && SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to finalize select statement");
		status = FAILURE;
	}

	if(close(root_directory_fd) != 0)
	{
		slog(ERROR,"Failed to close root directory descriptor: %s\n",strerror(errno));
		status = FAILURE;
	}

	if(SUCCESS == status && global_interrupt_flag == false)
	{
		slog(EVERY,"Missing file search finished\n");
	}

	call(m_del(relative_path));
	call(m_del(root_path));

	if(locked_unavailable_violation_detected == true)
	{
		slog(EVERY,BOLD "Warning! Data corruption detected for checksum-locked file!" RESET "\n");

		if(SUCCESS == status)
		{
			status = WARNING;
		}
	}

	provide(status);
}
