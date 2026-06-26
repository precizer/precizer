#include "sute.h"

#define READ_FAIL_REL_PATH "path1/AAA/ZAW/D/e/f/b_file.txt"

/**
 * @brief Reset read-error mocks used by this test group
 */
static void test0031_reset_read_mocks(void)
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
static Return test0031_prepare_case(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Compare captured application output and restore test resources
 *
 * @param[in] result Captured application output
 * @param[in] pattern Expected output pattern buffer
 * @param[in] error_buffer Captured stderr output
 * @param[in] template_name Expected output template file
 * @return Test status
 */
static Return test0031_check_case(
	memory     *result,
	memory     *pattern,
	memory     *error_buffer,
	const char *template_name)
{
	INITTEST;

	ASSERT(error_buffer->length == 0);
	ASSERT(SUCCESS == get_file_content(template_name,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,template_name));

	ASSERT(SUCCESS == delete_path("read_fail.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
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

	ASSERT(SUCCESS == test0031_prepare_case());

	const char *arguments = "--database=read_fail.db --progress"
	        " tests/fixtures/diffs/diff1";

	test0031_reset_read_mocks();
	mocks_openat_set_target_suffix(READ_FAIL_REL_PATH);
	mocks_openat_set_errno(EIO);
	mocks_openat_enable(true);

	ASSERT(SUCCESS == runit(arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

	mocks_openat_enable(false);

	ASSERT(SUCCESS == test0031_check_case(result,pattern,error_buffer,"templates/0031_001.txt"));

	ASSERT(mocks_openat_call_count() == 1);
	ASSERT(mocks_fdopen_call_count() == 0);
	ASSERT(mocks_fread_call_count() == 0);
	ASSERT(mocks_ferror_call_count() == 0);

	test0031_reset_read_mocks();

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

	ASSERT(SUCCESS == test0031_prepare_case());

	const char *arguments = "--database=read_fail.db --progress"
	        " tests/fixtures/diffs/diff1";

	test0031_reset_read_mocks();
	mocks_fdopen_set_target_suffix(READ_FAIL_REL_PATH);
	mocks_fdopen_set_errno(EIO);
	mocks_fdopen_enable(true);

	ASSERT(SUCCESS == runit(arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

	mocks_fdopen_enable(false);

	ASSERT(SUCCESS == test0031_check_case(result,pattern,error_buffer,"templates/0031_001.txt"));

	ASSERT(mocks_openat_call_count() == 0);
	ASSERT(mocks_fdopen_call_count() == 1);
	ASSERT(mocks_fread_call_count() == 0);
	ASSERT(mocks_ferror_call_count() == 0);

	test0031_reset_read_mocks();

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

	ASSERT(SUCCESS == test0031_prepare_case());

	const char *arguments = "--database=read_fail.db --progress"
	        " tests/fixtures/diffs/diff1";

	test0031_reset_read_mocks();
	mocks_fread_set_target_suffix(READ_FAIL_REL_PATH);
	mocks_fread_set_errno(EIO);
	mocks_fread_enable(true);
	mocks_ferror_enable(false);

	ASSERT(SUCCESS == runit(arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

	mocks_fread_enable(false);
	mocks_ferror_enable(true);

	ASSERT(SUCCESS == test0031_check_case(result,pattern,error_buffer,"templates/0031_002.txt"));

	ASSERT(mocks_openat_call_count() == 0);
	ASSERT(mocks_fdopen_call_count() == 0);
	ASSERT(mocks_fread_call_count() == 1);
	ASSERT(mocks_ferror_call_count() == 1);

	test0031_reset_read_mocks();

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

	ASSERT(SUCCESS == test0031_prepare_case());

	const char *arguments = "--database=read_fail.db --progress"
	        " tests/fixtures/diffs/diff1";

	test0031_reset_read_mocks();
	mocks_fread_set_target_suffix(READ_FAIL_REL_PATH);
	mocks_fread_set_errno(EIO);
	mocks_fread_enable(true);
	mocks_ferror_set_errno(EIO);
	mocks_ferror_enable(true);

	ASSERT(SUCCESS == runit(arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

	mocks_fread_enable(false);
	mocks_ferror_enable(true);

	ASSERT(SUCCESS == test0031_check_case(result,pattern,error_buffer,"templates/0031_001.txt"));

	ASSERT(mocks_openat_call_count() == 0);
	ASSERT(mocks_fdopen_call_count() == 0);
	ASSERT(mocks_fread_call_count() == 1);
	ASSERT(mocks_ferror_call_count() == 1);

	test0031_reset_read_mocks();

	m_del(pattern);
	m_del(result);
	m_del(error_buffer);

	RETURN_STATUS;
}

/**
 * @brief Verify read-error handling at each file hashing I/O stage
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

	RETURN_STATUS;
}
