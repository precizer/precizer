#include "sute.h"

/**
 * Run background scenario with SIGTERM and verify output template.
 */
static Return test0033_1(void)
{
	INITTEST;

	const char *arguments = "--progress --database=0033_interrupt_resume.db tests/fixtures/diffs/";
	const char *filename = "templates/0033_001.txt";
	const char *cleanup_path = "0033_interrupt_resume.db";

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit_background(
		arguments,
		stdout_result,
		stderr_result,
		SUCCESS|HALTED,
		ALLOW_BOTH,
		0U,
		5000U,
		SIGTERM,
		1U));

	ASSERT(SUCCESS == get_file_content(filename,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,filename));

	ASSERT(SUCCESS == m_copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	ASSERT(SUCCESS == delete_path(cleanup_path));

	m_del(stderr_pattern);
	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

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
	const char *cleanup_path = "0033_interrupt_resume.db";

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit_background(
		arguments,
		stdout_result,
		stderr_result,
		SUCCESS|HALTED,
		ALLOW_BOTH,
		0U,
		5000U,
		SIGINT,
		1U));

	ASSERT(SUCCESS == get_file_content(filename,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,filename));

	ASSERT(SUCCESS == m_copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	ASSERT(SUCCESS == delete_path(cleanup_path));

	m_del(stderr_pattern);
	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

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

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);
	m_create(char,huge_file_path,MEMORY_STRING);

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
		0U,
		5000U,
		SIGINT,
		2U));

	/*
	 * Step 3: Validate first-run output.
	 * It must contain the interruption scenario messages and no stderr output
	 */
	ASSERT(SUCCESS == get_file_content(first_run_template,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,first_run_template));

	ASSERT(SUCCESS == m_copy_literal(stderr_pattern,"\\A\\Z"));
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

	ASSERT(SUCCESS == m_copy_literal(stderr_pattern,"\\A\\Z"));
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

	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(huge_file_path),expected_sha512));
	ASSERT(0 == memcmp(db_sha512,expected_sha512,(size_t)SHA512_DIGEST_LENGTH));

	/* Step 7: Cleanup temporary test artifacts */
	ASSERT(SUCCESS == delete_path("0033_interrupt_resume.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/huge"));

	m_del(huge_file_path);
	m_del(stderr_pattern);
	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

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

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);
	m_create(char,huge_file_path,MEMORY_STRING);

	ASSERT(SUCCESS == prepare_huge_fixture(huge_file_path,NULL));

	const char *arguments = "--progress --database=0033_interrupt_rehash.db tests/fixtures/huge";

	ASSERT(SUCCESS == runit_background(
		arguments,
		stdout_result,
		stderr_result,
		SUCCESS|HALTED,
		ALLOW_BOTH,
		0U,
		5000U,
		SIGINT,
		2U));

	ASSERT(SUCCESS == get_file_content(first_run_template,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,first_run_template));
	ASSERT(SUCCESS == m_copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	sqlite3_int64 interrupted_offset = 0;
	int interrupted_md_context_bytes = 0;

	ASSERT(SUCCESS == read_resume_state_from_db(db_filename,relative_path,&interrupted_offset,&interrupted_md_context_bytes));
	ASSERT(interrupted_offset > 0);
	ASSERT(interrupted_md_context_bytes > 0);

	ASSERT(SUCCESS == append_byte_to_file(huge_file_path,(unsigned char)'X'));

	arguments = "--update --progress --database=0033_interrupt_rehash.db tests/fixtures/huge";
	ASSERT(SUCCESS == runit(arguments,stdout_result,stderr_result,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(second_run_template,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,second_run_template));
	ASSERT(SUCCESS == m_copy_literal(stderr_pattern,"\\A\\Z"));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern));

	const char *expected_paths[] =
	{
		"hugetestfile"
	};

	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	ASSERT(SUCCESS == delete_path("0033_interrupt_rehash.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/huge"));

	m_del(huge_file_path);
	m_del(stderr_pattern);
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
