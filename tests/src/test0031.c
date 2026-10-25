#include "sute.h"

#define READ_FAIL_REL_PATH "path1/AAA/ZAW/D/e/f/b_file.txt"

/**
 * @brief Reset read-error mocks used by this test group
 */
static void reset_read_mocks(void)
{
	mocks_openat_reset();
	mocks_fdopen_reset();
	mocks_fread_reset();
	mocks_ferror_reset();
}

/**
 * @brief Prepare the mutable fixture
 *
 * @return Test status
 */
static Return prepare_case(void)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	deliver(status);
}

/**
 * @brief Compare captured application output and restore test resources
 *
 * @param[in] result Captured application output
 * @param[in] pattern Expected output pattern buffer
 * @param[in] error_buffer Captured stderr output
 * @param[in] template_name Expected output template file
 * @param[in] read_failed True when the target must remain absent from the database
 * @return Test status
 */
static Return check_case(
	memory     *result,
	memory     *pattern,
	memory     *error_buffer,
	const char *template_name,
	const bool read_failed)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	ASSERT(error_buffer->length == 0);
	ASSERT(SUCCESS == get_file_content(template_name,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,template_name));

	/* A failed file must not acquire a completed record, and later files must still be hashed */
	bool row_exists = false;
	int files_count = 0;
	ASSERT(SUCCESS == db_relative_path_exists("read_fail.db",READ_FAIL_REL_PATH,&row_exists));
	ASSERT(row_exists == (read_failed == false));
	ASSERT(SUCCESS == db_read_files_count("read_fail.db",&files_count));

	if(read_failed == true)
	{
		ASSERT(files_count == 11);
	} else {
		ASSERT(files_count == 12);
	}
	m_create(char,later_path,MEMORY_STRING);
	ASSERT(SUCCESS == construct_path("tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt",later_path));
	ASSERT(SUCCESS == db_final_sha512_matches_file("read_fail.db","path2/AAA/ZAW/D/e/f/b_file.txt",m_text(later_path)));
	ASSERT(SUCCESS == db_resume_state_is_empty("read_fail.db","path2/AAA/ZAW/D/e/f/b_file.txt"));
	m_del(later_path);

	ASSERT(SUCCESS == delete_path("read_fail.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	deliver(status);
}

/**
 * @brief Verify read-error reporting when openat() fails for the target file
 *
 * @return Test status
 */
static Return test0031_1(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,error_buffer,MEMORY_STRING);

	/* Recreate the fixture and reset the same I/O failure for each output format */
	const struct argument_test_case cases[] = {
		{"--database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == prepare_case());

		reset_read_mocks();
		mocks_openat_set_target_suffix(READ_FAIL_REL_PATH);
		mocks_openat_set_errno(EIO);
		mocks_openat_enable(true);

		ASSERT(SUCCESS == runit(cases[i].arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

		mocks_openat_enable(false);

		ASSERT(SUCCESS == check_case(result,pattern,error_buffer,cases[i].stdout_template,true));

		ASSERT(mocks_openat_call_count() == 1);
		ASSERT(mocks_fdopen_call_count() == 0);
		ASSERT(mocks_fread_call_count() == 0);
		ASSERT(mocks_ferror_call_count() == 0);

		reset_read_mocks();
	}

	m_del(pattern);
	m_del(result);
	m_del(error_buffer);

	RETURN_STATUS;
}

/**
 * @brief Verify read-error reporting when fdopen() fails for the target file
 *
 * @return Test status
 */
static Return test0031_2(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,error_buffer,MEMORY_STRING);

	/* Recreate the fixture and reset the same I/O failure for each output format */
	const struct argument_test_case cases[] = {
		{"--database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == prepare_case());

		reset_read_mocks();
		mocks_fdopen_set_target_suffix(READ_FAIL_REL_PATH);
		mocks_fdopen_set_errno(EIO);
		mocks_fdopen_enable(true);

		ASSERT(SUCCESS == runit(cases[i].arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

		mocks_fdopen_enable(false);

		ASSERT(SUCCESS == check_case(result,pattern,error_buffer,cases[i].stdout_template,true));

		ASSERT(mocks_openat_call_count() == 0);
		ASSERT(mocks_fdopen_call_count() == 1);
		ASSERT(mocks_fread_call_count() == 0);
		ASSERT(mocks_ferror_call_count() == 0);

		reset_read_mocks();
	}

	m_del(pattern);
	m_del(result);
	m_del(error_buffer);

	RETURN_STATUS;
}

/**
 * @brief Verify that a zero fread without ferror is handled as EOF
 *
 * @return Test status
 */
static Return test0031_3(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,error_buffer,MEMORY_STRING);

	/* Recreate the fixture and reset the same I/O failure for each output format */
	const struct argument_test_case cases[] = {
		{"--database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_003_1.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_003_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_003_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == prepare_case());

		reset_read_mocks();
		mocks_fread_set_target_suffix(READ_FAIL_REL_PATH);
		mocks_fread_set_errno(EIO);
		mocks_fread_enable(true);
		mocks_ferror_enable(false);

		ASSERT(SUCCESS == runit(cases[i].arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

		mocks_fread_enable(false);
		mocks_ferror_enable(true);

		ASSERT(SUCCESS == check_case(result,pattern,error_buffer,cases[i].stdout_template,false));

		ASSERT(mocks_openat_call_count() == 0);
		ASSERT(mocks_fdopen_call_count() == 0);
		ASSERT(mocks_fread_call_count() == 1);
		ASSERT(mocks_ferror_call_count() == 1);

		reset_read_mocks();
	}

	m_del(pattern);
	m_del(result);
	m_del(error_buffer);

	RETURN_STATUS;
}

/**
 * @brief Verify read-error reporting when ferror() confirms a zero fread
 *
 * @return Test status
 */
static Return test0031_4(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,error_buffer,MEMORY_STRING);

	/* Recreate the fixture and reset the same I/O failure for each output format */
	const struct argument_test_case cases[] = {
		{"--database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == prepare_case());

		reset_read_mocks();
		mocks_fread_set_target_suffix(READ_FAIL_REL_PATH);
		mocks_fread_set_errno(EIO);
		mocks_fread_enable(true);
		mocks_ferror_set_errno(EIO);
		mocks_ferror_enable(true);

		ASSERT(SUCCESS == runit(cases[i].arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

		mocks_fread_enable(false);
		mocks_ferror_enable(true);

		ASSERT(SUCCESS == check_case(result,pattern,error_buffer,cases[i].stdout_template,true));

		ASSERT(mocks_openat_call_count() == 0);
		ASSERT(mocks_fdopen_call_count() == 0);
		ASSERT(mocks_fread_call_count() == 1);
		ASSERT(mocks_ferror_call_count() == 1);

		reset_read_mocks();
	}

	m_del(pattern);
	m_del(result);
	m_del(error_buffer);

	RETURN_STATUS;
}

/**
 * @brief Verify stat error records and continued traversal in every output format
 * @return Test status
 */
static Return test0031_5(void)
{
	INITTEST;
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,error_buffer,MEMORY_STRING);

	/* The failed entry is a real file, so later traversal and DB results remain reproducible */
	const struct argument_test_case cases[] = {
		{"--database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_005_1.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_005_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == prepare_case());
		mocks_fts_read_set_failure(READ_FAIL_REL_PATH,FTS_NS,EIO);
		ASSERT(SUCCESS == runit(cases[i].arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

		/* Full progress traverses twice; compact and machine skip preliminary counting */
		if(i == 0U)
		{
			ASSERT(mocks_fts_read_call_count() == 2U);
		} else {
			ASSERT(mocks_fts_read_call_count() == 1U);
		}
		mocks_fts_read_reset();
		ASSERT(SUCCESS == check_case(result,pattern,error_buffer,cases[i].stdout_template,true));
	}
	mocks_fts_read_reset();
	m_del(error_buffer);
	m_del(pattern);
	m_del(result);
	RETURN_STATUS;
}

/**
 * @brief Verify fts error records and continued traversal in every output format
 * @return Test status
 */
static Return test0031_6(void)
{
	INITTEST;
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,error_buffer,MEMORY_STRING);

	/* The failed entry is a real file, so later traversal and DB results remain reproducible */
	const struct argument_test_case cases[] = {
		{"--database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_006_1.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_006_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --database=read_fail.db --progress tests/fixtures/diffs/diff1",COMPLETED,"templates/0031_006_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == prepare_case());
		mocks_fts_read_set_failure(READ_FAIL_REL_PATH,FTS_ERR,EIO);
		ASSERT(SUCCESS == runit(cases[i].arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

		/* Full progress traverses twice; compact and machine skip preliminary counting */
		if(i == 0U)
		{
			ASSERT(mocks_fts_read_call_count() == 2U);
		} else {
			ASSERT(mocks_fts_read_call_count() == 1U);
		}
		mocks_fts_read_reset();
		ASSERT(SUCCESS == check_case(result,pattern,error_buffer,cases[i].stdout_template,true));
	}
	mocks_fts_read_reset();
	m_del(error_buffer);
	m_del(pattern);
	m_del(result);
	RETURN_STATUS;
}

/**
 * @brief Verify read errors and traversal-entry failures
 *
 * @return Test status
 */
Return test0031(void)
{
	INITTEST;

	TEST(test0031_1,"Read error handling when openat fails");
	TEST(test0031_2,"Read error handling when fdopen fails");
	TEST(test0031_3,"Zero fread without ferror stays non-error");
	TEST(test0031_4,"Read error handling when ferror confirms fread failure");
	TEST(test0031_5,"Stat errors preserve relative paths and allow later files to be hashed");
	TEST(test0031_6,"FTS errors preserve relative paths and allow later files to be hashed");

	RETURN_STATUS;
}
