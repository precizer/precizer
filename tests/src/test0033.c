#include "sute.h"

/**
 * @brief Verify SIGTERM before hashing in full, compact, and machine formats
 */
static Return test0033_1(void)
{
	INITTEST;

	const char *cleanup_path = "0033_interrupt_resume.db";

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/* Interrupt before hashing in each format and require an empty database */
	const struct argument_test_case cases[] = {
		{"--progress --database=0033_interrupt_resume.db tests/fixtures/diffs/",SUCCESS|HALTED,"templates/0033_001.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --progress --database=0033_interrupt_resume.db tests/fixtures/diffs/",SUCCESS|HALTED,"templates/0045_033.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --progress --database=0033_interrupt_resume.db tests/fixtures/diffs/",SUCCESS|HALTED,"templates/0045_033.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == runit_background(
			cases[i].arguments,
			stdout_result,
			stderr_result,
			SUCCESS|HALTED,
			ALLOW_BOTH,
			0U,
			5000U,
			SIGTERM,
			1U));

		ASSERT(SUCCESS == get_file_content(cases[i].stdout_template,stdout_pattern));
		ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,cases[i].stdout_template));

		ASSERT(stderr_result->length == 0U);

		int files_count = -1;
		ASSERT(SUCCESS == db_read_files_count(cleanup_path,&files_count));
		ASSERT(files_count == 0);
		ASSERT(SUCCESS == delete_path(cleanup_path));
	}

	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

	RETURN_STATUS;
}

/**
 * @brief Verify SIGINT before hashing in full, compact, and machine formats
 */
static Return test0033_2(void)
{
	INITTEST;

	const char *cleanup_path = "0033_interrupt_resume.db";

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/* Interrupt before hashing in each format and require an empty database */
	const struct argument_test_case cases[] = {
		{"--progress --database=0033_interrupt_resume.db tests/fixtures/diffs/",SUCCESS|HALTED,"templates/0033_002.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --progress --database=0033_interrupt_resume.db tests/fixtures/diffs/",SUCCESS|HALTED,"templates/0045_033.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --progress --database=0033_interrupt_resume.db tests/fixtures/diffs/",SUCCESS|HALTED,"templates/0045_033.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == runit_background(
			cases[i].arguments,
			stdout_result,
			stderr_result,
			SUCCESS|HALTED,
			ALLOW_BOTH,
			0U,
			5000U,
			SIGINT,
			1U));

		ASSERT(SUCCESS == get_file_content(cases[i].stdout_template,stdout_pattern));
		ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,cases[i].stdout_template));

		ASSERT(stderr_result->length == 0U);

		int files_count = -1;
		ASSERT(SUCCESS == db_read_files_count(cleanup_path,&files_count));
		ASSERT(files_count == 0);
		ASSERT(SUCCESS == delete_path(cleanup_path));
	}

	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

	RETURN_STATUS;
}

/**
 * @brief Verify interruption, repeated interruption, and resumed SHA512 in all output formats
 */
static Return test0033_3(void)
{
	INITTEST;

	const char *relative_path = "hugetestfile";

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,huge_file_path,MEMORY_STRING);

	/* Recreate the file and database for full, compact, and machine runs */
	const struct argument_test_case first_cases[] = {
		{"--progress --database=0033_interrupt_resume.db tests/fixtures/huge",SUCCESS|HALTED,"templates/0033_003_1.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --progress --database=0033_interrupt_resume.db tests/fixtures/huge",SUCCESS|HALTED,"templates/0033_003_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --progress --database=0033_interrupt_resume.db tests/fixtures/huge",SUCCESS|HALTED,"templates/0033_003_1_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	const struct argument_test_case resume_cases[] = {
		{"--update --progress --database=0033_interrupt_resume.db tests/fixtures/huge",COMPLETED,"templates/0033_003_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --update --progress --database=0033_interrupt_resume.db tests/fixtures/huge",COMPLETED,"templates/0033_003_2_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --update --progress --database=0033_interrupt_resume.db tests/fixtures/huge",COMPLETED,"templates/0033_003_2_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; first_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		/*
		 * Step 1: Prepare isolated test data in TMPDIR and start from a clean DB
		 */
		struct stat huge_file_stat = {0};
		ASSERT(SUCCESS == prepare_huge_fixture(huge_file_path,&huge_file_stat));

		/*
		 * Use the real file size from the prepared fixture as
		 * an upper bound for the interrupted offset assertions below
		 */
		ASSERT(huge_file_stat.st_size > 0);

		/*
		 * Step 2: Run in background, wait until hashing reaches wait-point 2,
		 * then deliver SIGINT. The process must exit as SUCCESS|HALTED
		 */
		ASSERT(SUCCESS == runit_background(
			first_cases[i].arguments,
			stdout_result,
			stderr_result,
			SUCCESS|HALTED,
			ALLOW_BOTH,
			0U,
			5000U,
			SIGINT,
			2U));

		/*
		 * Step 3: Validate first-run output.
		 * It must contain the interruption scenario messages and no stderr output
		 */
		ASSERT(SUCCESS == get_file_content(first_cases[i].stdout_template,stdout_pattern));
		ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,first_cases[i].stdout_template));

		ASSERT(stderr_result->length == 0U);

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

		/* Interrupt a resumed hash again and require the saved offset to advance */
		if(i > 0)
		{
			ASSERT(SUCCESS == runit_background(resume_cases[i].arguments,stdout_result,stderr_result,
				SUCCESS|HALTED,ALLOW_BOTH,0U,5000U,SIGINT,2U));
			const char *repeat_template = "templates/0033_003_3_3.txt";

			if(i == 1)
			{
				repeat_template = "templates/0033_003_3_2.txt";
			}
			ASSERT(SUCCESS == get_file_content(repeat_template,stdout_pattern));
			ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,repeat_template));
			ASSERT(stderr_result->length == 0);
			sqlite3_int64 repeated_offset = 0;
			int repeated_context_bytes = 0;
			ASSERT(SUCCESS == read_resume_state_from_db("0033_interrupt_resume.db",relative_path,
				&repeated_offset,&repeated_context_bytes));
			ASSERT(repeated_offset > interrupted_offset);
			ASSERT(repeated_offset < (sqlite3_int64)huge_file_stat.st_size);
			ASSERT(repeated_context_bytes > 0);
		}

		/*
		 * Step 5: Resume hashing with --update and verify second-run output
		 */
		ASSERT(SUCCESS == runit(resume_cases[i].arguments,stdout_result,stderr_result,COMPLETED,ALLOW_BOTH));

		ASSERT(SUCCESS == get_file_content(resume_cases[i].stdout_template,stdout_pattern));
		ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,resume_cases[i].stdout_template));

		ASSERT(stderr_result->length == 0U);

		sqlite3_int64 final_offset = -1;
		unsigned char db_sha512[SHA512_DIGEST_LENGTH] = {0};
		unsigned char expected_sha512[SHA512_DIGEST_LENGTH] = {0};

		/*
		 * Step 6: Verify final DB state after resume.
		 * Offset must be reset to 0 and the stored SHA512 must match file content
		 */
		ASSERT(SUCCESS == read_final_sha512_from_db("0033_interrupt_resume.db",relative_path,&final_offset,db_sha512));

		ASSERT(0 == final_offset);
		ASSERT(SUCCESS == db_resume_state_is_empty("0033_interrupt_resume.db",relative_path));

		ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(huge_file_path),expected_sha512));
		ASSERT(0 == memcmp(db_sha512,expected_sha512,(size_t)SHA512_DIGEST_LENGTH));

		/* Step 7: Cleanup temporary test artifacts */
		ASSERT(SUCCESS == delete_path("0033_interrupt_resume.db"));
		ASSERT(SUCCESS == delete_path("tests/fixtures/huge"));
	}

	m_del(huge_file_path);
	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

	RETURN_STATUS;
}

/**
 * @brief Verify a complete restart after an interrupted file changes in all output formats
 */
static Return test0033_4(void)
{
	INITTEST;

	const char *db_filename = "0033_interrupt_rehash.db";
	const char *relative_path = "hugetestfile";

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,huge_file_path,MEMORY_STRING);

	/* Recreate the file and database for full, compact, and machine runs */
	const struct argument_test_case first_cases[] = {
		{"--progress --database=0033_interrupt_rehash.db tests/fixtures/huge",SUCCESS|HALTED,"templates/0033_004_1.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --progress --database=0033_interrupt_rehash.db tests/fixtures/huge",SUCCESS|HALTED,"templates/0033_003_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --progress --database=0033_interrupt_rehash.db tests/fixtures/huge",SUCCESS|HALTED,"templates/0033_003_1_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --verbose --progress --database=0033_interrupt_rehash.db tests/fixtures/huge",SUCCESS|HALTED,"templates/0033_003_1_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	const struct argument_test_case resume_cases[] = {
		{"--update --progress --database=0033_interrupt_rehash.db tests/fixtures/huge",COMPLETED,"templates/0033_004_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --update --progress --database=0033_interrupt_rehash.db tests/fixtures/huge",COMPLETED,"templates/0033_004_2_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --update --progress --database=0033_interrupt_rehash.db tests/fixtures/huge",COMPLETED,"templates/0033_004_2_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --verbose --update --progress --database=0033_interrupt_rehash.db tests/fixtures/huge",COMPLETED,"templates/0033_004_2_4.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; first_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == prepare_huge_fixture(huge_file_path,NULL));

		ASSERT(SUCCESS == runit_background(
			first_cases[i].arguments,
			stdout_result,
			stderr_result,
			SUCCESS|HALTED,
			ALLOW_BOTH,
			0U,
			5000U,
			SIGINT,
			2U));

		ASSERT(SUCCESS == get_file_content(first_cases[i].stdout_template,stdout_pattern));
		ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,first_cases[i].stdout_template));
		ASSERT(stderr_result->length == 0U);

		sqlite3_int64 interrupted_offset = 0;
		int interrupted_md_context_bytes = 0;

		ASSERT(SUCCESS == read_resume_state_from_db(db_filename,relative_path,&interrupted_offset,&interrupted_md_context_bytes));
		ASSERT(interrupted_offset > 0);
		ASSERT(interrupted_md_context_bytes > 0);

		ASSERT(SUCCESS == append_byte_to_file(huge_file_path,(unsigned char)'X'));

		ASSERT(SUCCESS == runit(resume_cases[i].arguments,stdout_result,stderr_result,COMPLETED,ALLOW_BOTH));

		ASSERT(SUCCESS == get_file_content(resume_cases[i].stdout_template,stdout_pattern));
		ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,resume_cases[i].stdout_template));
		ASSERT(stderr_result->length == 0U);

		const char *expected_paths[] =
		{
			"hugetestfile"
		};

		ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
		/* A restart must hash the changed file completely and discard the old checkpoint */
		ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,relative_path,m_text(huge_file_path)));
		ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,relative_path));
		ASSERT(SUCCESS == delete_path("0033_interrupt_rehash.db"));
		ASSERT(SUCCESS == delete_path("tests/fixtures/huge"));
	}

	m_del(huge_file_path);
	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

	RETURN_STATUS;
}

/**
 * Background interruption tests grouped as a separate suite.
 */
Return test0033(void)
{
	INITTEST;

	SLOWTEST;

	TEST(test0033_1,"Background run receives SIGTERM and exits with HALTED");
	TEST(test0033_2,"Background run receives SIGINT and exits with HALTED");
	TEST(test0033_3,"Random interruption on hugetestfile with resume and SHA512 verification");
	TEST(test0033_4,"Interrupted hash with file change restarts rehash from the beginning");

	RETURN_STATUS;
}
