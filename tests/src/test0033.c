#include "sute.h"

/**
 * Compute SHA512 for a file using the project SHA512 library.
 */
static Return compute_file_sha512(
	const char    *file_path,
	unsigned char *sha512_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	unsigned char buffer[65536];
	SHA512_Context context = {0};

	if(file_path == NULL || sha512_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		file = fopen(file_path,"rb");
		if(file == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && sha512_init(&context) == 1)
	{
		status = FAILURE;
	}

	while(SUCCESS == status)
	{
		const size_t bytes_read = fread(buffer,sizeof(unsigned char),sizeof(buffer),file);

		if(bytes_read == 0U)
		{
			if(ferror(file) != 0)
			{
				status = FAILURE;
			}
			break;
		}

		if(sha512_update(&context,buffer,bytes_read) == 1)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && sha512_final(&context,sha512_out) == 1)
	{
		status = FAILURE;
	}

	if(file != NULL)
	{
		(void)fclose(file);
	}

	return(status);
}

/**
 * Append one byte to a file using native C file I/O
 */
static Return append_byte_to_file(
	const char   *file_path,
	unsigned char byte)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(file_path == NULL)
	{
		status = FAILURE;
	}

	FILE *file = NULL;

	if(SUCCESS == status)
	{
		file = fopen(file_path,"ab");
		if(file == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(fwrite(&byte,sizeof(unsigned char),1U,file) != 1U)
		{
			status = FAILURE;
		}
	}

	if(file != NULL)
	{
		if(fclose(file) != 0)
		{
			status = FAILURE;
		}
	}

	return(status);
}

/**
 * Read intermediate offset and mdContext state for one file from DB.
 */
static Return read_resume_state_from_db(
	const char     *db_filename,
	const char     *relative_path,
	sqlite3_int64  *offset_out,
	int            *md_context_bytes_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT offset, mdContext FROM files WHERE relative_path = ?1;";
	create(char,db_path);

	if(db_filename == NULL
	        || relative_path == NULL
	        || offset_out == NULL
	        || md_context_bytes_out == NULL)
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		status = construct_path(db_filename,db_path);
	}

	if((TRIUMPH & status) && SQLITE_OK != sqlite3_open_v2(getcstring(db_path),&db,SQLITE_OPEN_READONLY,NULL))
	{
		status = FAILURE;
	}

	if((TRIUMPH & status) && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	if((TRIUMPH & status) && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		const int step_rc = sqlite3_step(stmt);
		if(step_rc != SQLITE_ROW)
		{
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		if(sqlite3_column_type(stmt,0) == SQLITE_NULL)
		{
			*offset_out = 0;
		} else {
			*offset_out = sqlite3_column_int64(stmt,0);
		}

		*md_context_bytes_out = sqlite3_column_bytes(stmt,1);

		const int done_rc = sqlite3_step(stmt);
		if(done_rc != SQLITE_DONE)
		{
			status = FAILURE;
		}
	}

	if(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	if(db != NULL)
	{
		(void)sqlite3_close(db);
	}

	del(db_path);

	return(status);
}

/**
 * Read final offset and SHA512 checksum for one file from DB.
 */
static Return read_final_sha512_from_db(
	const char     *db_filename,
	const char     *relative_path,
	sqlite3_int64  *offset_out,
	unsigned char  *sha512_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT offset, sha512 FROM files WHERE relative_path = ?1;";
	create(char,db_path);

	if(db_filename == NULL
	        || relative_path == NULL
	        || offset_out == NULL
	        || sha512_out == NULL)
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		status = construct_path(db_filename,db_path);
	}

	if((TRIUMPH & status) && SQLITE_OK != sqlite3_open_v2(getcstring(db_path),&db,SQLITE_OPEN_READONLY,NULL))
	{
		status = FAILURE;
	}

	if((TRIUMPH & status) && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	if((TRIUMPH & status) && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		const int step_rc = sqlite3_step(stmt);
		if(step_rc != SQLITE_ROW)
		{
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		if(sqlite3_column_type(stmt,0) == SQLITE_NULL)
		{
			*offset_out = 0;
		} else {
			*offset_out = sqlite3_column_int64(stmt,0);
		}

		const void *sha512_blob = sqlite3_column_blob(stmt,1);
		const int sha512_bytes = sqlite3_column_bytes(stmt,1);

		if(sha512_blob == NULL || sha512_bytes != SHA512_DIGEST_LENGTH)
		{
			status = FAILURE;
		} else {
			memcpy(sha512_out,sha512_blob,(size_t)SHA512_DIGEST_LENGTH);
		}

		const int done_rc = sqlite3_step(stmt);
		if((TRIUMPH & status) && done_rc != SQLITE_DONE)
		{
			status = FAILURE;
		}
	}

	if(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	if(db != NULL)
	{
		(void)sqlite3_close(db);
	}

	del(db_path);

	return(status);
}

/**
 * Run background scenario with SIGTERM and verify output template.
 */
static Return test0033_1(void)
{
	INITTEST;

	const char *arguments = "--progress --database=0033_interrupt_resume.db tests/fixtures/diffs/";
	const char *filename = "templates/0033_001.txt";
	const char *cleanup_command = "rm -f \"${TMPDIR}/0033_interrupt_resume.db\"";

	create(char,stdout_result);
	create(char,stderr_result);
	create(char,stdout_pattern);
	create(char,stderr_pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit_background(
		arguments,
		stdout_result,
		stderr_result,
		SUCCESS|HALTED,
		ALLOW_BOTH,
		100U,
		1000U,
		SIGTERM,
		1U));

	ASSERT(SUCCESS == get_file_content(filename,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,filename));

	ASSERT(SUCCESS == copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(stderr_pattern);
	del(stdout_pattern);
	del(stderr_result);
	del(stdout_result);

	RETURN_STATUS;
}

/**
 * Run background scenario with SIGINT and verify output template.
 */
static Return test0033_2(void)
{
	INITTEST;

	const char *arguments = "--progress --database=0033_interrupt_resume.db tests/fixtures/diffs/";
	const char *filename = "templates/0033_002.txt";
	const char *cleanup_command = "rm -f \"${TMPDIR}/0033_interrupt_resume.db\"";

	create(char,stdout_result);
	create(char,stderr_result);
	create(char,stdout_pattern);
	create(char,stderr_pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit_background(
		arguments,
		stdout_result,
		stderr_result,
		SUCCESS|HALTED,
		ALLOW_BOTH,
		100U,
		1000U,
		SIGINT,
		1U));

	ASSERT(SUCCESS == get_file_content(filename,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,filename));

	ASSERT(SUCCESS == copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(stderr_pattern);
	del(stdout_pattern);
	del(stderr_result);
	del(stdout_result);

	RETURN_STATUS;
}

/**
 * Interrupt hashing of hugetestfile, resume with --update, and verify SHA512.
 */
static Return test0033_3(void)
{
	INITTEST;

	const char *relative_path = "hugetestfile";
	const char *first_run_template = "templates/0033_003_1.txt";
	const char *second_run_template = "templates/0033_003_2.txt";
	const char *prepare_command = "cd ${TMPDIR};"
	        "mkdir -p tests/fixtures/;"
	        "rm -rf tests/fixtures/huge/;"
	        "cp -a \"$ORIGIN_DIR/tests/fixtures/huge\" tests/fixtures/;";
	const char *cleanup_command = "cd ${TMPDIR};"
	        "rm -f 0033_interrupt_resume.db;"
	        "rm -rf tests/fixtures/huge/;";

	create(char,stdout_result);
	create(char,stderr_result);
	create(char,stdout_pattern);
	create(char,stderr_pattern);
	create(char,huge_file_path);

	/*
	 * Step 1: Prepare isolated test data in TMPDIR and start from a clean DB
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == external_call(prepare_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == construct_path("tests/fixtures/huge/hugetestfile",huge_file_path));

	/*
	 * Read the real file size from the filesystem once and use it as
	 * an upper bound for the interrupted offset assertions below
	 */
	struct stat huge_file_stat = {0};
	ASSERT(0 == stat(getcstring(huge_file_path),&huge_file_stat));
	ASSERT(huge_file_stat.st_size > 0);

	const char *arguments = "--progress --database=0033_interrupt_resume.db tests/fixtures/huge";

	/*
	 * Step 2: Run in background, wait until hashing reaches wait-point 2,
	 * then deliver SIGINT. The process must exit as SUCCESS|HALTED
	 */
	ASSERT(SUCCESS == runit_background(
		arguments,
		stdout_result,
		stderr_result,
		SUCCESS|HALTED,
		ALLOW_BOTH,
		500U,
		5000U,
		SIGINT,
		2U));

	/*
	 * Step 3: Validate first-run output.
	 * It must contain the interruption scenario messages and no stderr output
	 */
	ASSERT(SUCCESS == get_file_content(first_run_template,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,first_run_template));

	ASSERT(SUCCESS == copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	sqlite3_int64 interrupted_offset = 0;
	int interrupted_md_context_bytes = 0;

	/*
	 * Step 4: Read intermediate resume state from DB.
	 * The interrupted offset must be inside (0, real_file_size),
	 * and mdContext blob must be non-empty for resume.
	 */
	ASSERT(SUCCESS == read_resume_state_from_db("0033_interrupt_resume.db",relative_path,&interrupted_offset,&interrupted_md_context_bytes));

	ASSERT(interrupted_offset > 0);
	ASSERT(interrupted_offset < (sqlite3_int64)huge_file_stat.st_size);

	ASSERT(interrupted_md_context_bytes > 0);

	/*
	 * Step 5: Resume hashing with --update and verify second-run output
	 */
	arguments = "--update --progress --database=0033_interrupt_resume.db tests/fixtures/huge";
	ASSERT(SUCCESS == runit(arguments,stdout_result,stderr_result,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(second_run_template,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,second_run_template));

	ASSERT(SUCCESS == copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	sqlite3_int64 final_offset = -1;
	unsigned char db_sha512[SHA512_DIGEST_LENGTH] = {0};
	unsigned char expected_sha512[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Step 6: Verify final DB state after resume.
	 * Offset must be reset to 0 and the stored SHA512 must match file content
	 */
	ASSERT(SUCCESS == read_final_sha512_from_db("0033_interrupt_resume.db",relative_path,&final_offset,db_sha512));

	ASSERT(0 == final_offset);

	ASSERT(SUCCESS == compute_file_sha512(getcstring(huge_file_path),expected_sha512));
	ASSERT(0 == memcmp(db_sha512,expected_sha512,(size_t)SHA512_DIGEST_LENGTH));

	/* Step 7: Cleanup temporary test artifacts */
	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(huge_file_path);
	del(stderr_pattern);
	del(stdout_pattern);
	del(stderr_result);
	del(stdout_result);

	RETURN_STATUS;
}

/**
 * Interrupt hashing, modify file metadata, and verify restart from beginning
 */
static Return test0033_4(void)
{
	INITTEST;

	const char *db_filename = "0033_interrupt_rehash.db";
	const char *relative_path = "hugetestfile";
	const char *first_run_template = "templates/0033_004_1.txt";
	const char *second_run_template = "templates/0033_004_2.txt";
	const char *prepare_command = "cd ${TMPDIR};"
	        "mkdir -p tests/fixtures/;"
	        "rm -rf tests/fixtures/huge/;"
	        "cp -a \"$ORIGIN_DIR/tests/fixtures/huge\" tests/fixtures/;";
	const char *cleanup_command = "cd ${TMPDIR};"
	        "rm -f 0033_interrupt_rehash.db;"
	        "rm -rf tests/fixtures/huge/;";

	create(char,stdout_result);
	create(char,stderr_result);
	create(char,stdout_pattern);
	create(char,stderr_pattern);
	create(char,huge_file_path);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == external_call(prepare_command,NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == construct_path("tests/fixtures/huge/hugetestfile",huge_file_path));

	const char *arguments = "--progress --database=0033_interrupt_rehash.db tests/fixtures/huge";

	ASSERT(SUCCESS == runit_background(
		arguments,
		stdout_result,
		stderr_result,
		SUCCESS|HALTED,
		ALLOW_BOTH,
		500U,
		5000U,
		SIGINT,
		2U));

	ASSERT(SUCCESS == get_file_content(first_run_template,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,first_run_template));
	ASSERT(SUCCESS == copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	sqlite3_int64 interrupted_offset = 0;
	int interrupted_md_context_bytes = 0;

	ASSERT(SUCCESS == read_resume_state_from_db(db_filename,relative_path,&interrupted_offset,&interrupted_md_context_bytes));
	ASSERT(interrupted_offset > 0);
	ASSERT(interrupted_md_context_bytes > 0);

	ASSERT(SUCCESS == append_byte_to_file(getcstring(huge_file_path),(unsigned char)'X'));

	arguments = "--update --progress --database=0033_interrupt_rehash.db tests/fixtures/huge";
	ASSERT(SUCCESS == runit(arguments,stdout_result,stderr_result,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(second_run_template,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,second_run_template));
	ASSERT(SUCCESS == copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	const char *expected_paths[] =
	{
		"hugetestfile"
	};

	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(huge_file_path);
	del(stderr_pattern);
	del(stdout_pattern);
	del(stderr_result);
	del(stdout_result);

	RETURN_STATUS;
}

/**
 * Background interruption tests grouped as a separate suite.
 */
Return test0033(void)
{
	INITTEST;

	TEST(test0033_1,"Background run receives SIGTERM and exits with HALTED…");
	TEST(test0033_2,"Background run receives SIGINT and exits with HALTED…");
	TEST(test0033_3,"Random interruption on hugetestfile with resume and SHA512 verification…");
	TEST(test0033_4,"Interrupted hash with file change restarts rehash from the beginning…");

	RETURN_STATUS;
}
