#include "sute.h"

/**
 * @brief Prepare the test environment
 *
 * Initializes the temporary workspace and required environment variables
 * so the test suite can run against a consistent isolated setup
 *
 * @return SUCCESS on success, FAILURE on error
 */
Return prepare(void)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	INITTEST;

	const char *environment_name = NULL;

	m_create(char,path,MEMORY_STRING);
	m_create(char,environment_build_path,MEMORY_STRING);
	m_create(char,environment_precizer_path,MEMORY_STRING);

	ASSERT(SUCCESS == get_origin_dir(path));
	ASSERT(SUCCESS == set_environment_variable("ORIGIN_DIR",m_text(path)));
	call(m_del(path));

	ASSERT(SUCCESS == create_tmpdir(path));
	ASSERT(SUCCESS == set_environment_variable("TMPDIR",m_text(path)));
	ASSERT(SUCCESS == set_environment_variable("BINDIR",m_text(path)));
	call(m_del(path));

	ASSERT(SUCCESS == extract_current_executable_directory_name(path));
	ASSERT(SUCCESS == set_environment_variable("ENVIRONMENT",m_text(path)));
	call(m_del(path));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	environment_name = getenv("ENVIRONMENT");
	ASSERT(environment_name != NULL);

	ASSERT(SUCCESS == execute_and_set_variable("DBNAME","echo \"$(hostname).db\"",0));

	ASSERT(SUCCESS == create_directory("tests/fixtures/diffs"));
	ASSERT(SUCCESS == create_directory("tests/templates"));
	ASSERT(SUCCESS == create_directory(".builds"));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/diffs/diff1","tests/fixtures/diffs/diff1",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/diffs/diff2","tests/fixtures/diffs/diff2",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/'apostrophe","tests/fixtures/'apostrophe",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/apostrophe'","tests/fixtures/apostrophe'",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/levels","tests/fixtures/levels",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/4","tests/fixtures/4",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/ignore_include_cases","tests/fixtures/ignore_include_cases",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/templates/0015_database_v0.db","tests/templates/0015_database_v0.db",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/templates/0015_database_v1.db","tests/templates/0015_database_v1.db",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/templates/0015_database_v2.db","tests/templates/0015_database_v2.db",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db","tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db","tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/long","tests/fixtures/long",ALLOW_MISSING_SOURCE));
	ASSERT(SUCCESS == m_copy_literal(environment_build_path,".builds/"));
	ASSERT(SUCCESS == m_concat_string(environment_build_path,environment_name));
	ASSERT(SUCCESS == copy_from_origin(m_text(environment_build_path),m_text(environment_build_path),ALLOW_MISSING_SOURCE));
	ASSERT(SUCCESS == m_copy(environment_precizer_path,environment_build_path));
	ASSERT(SUCCESS == m_concat_literal(environment_precizer_path,"/precizer"));
	ASSERT(SUCCESS == copy_from_origin(m_text(environment_precizer_path),"precizer",ALLOW_MISSING_SOURCE));

	call(m_del(environment_precizer_path));
	call(m_del(environment_build_path));

	// Bump diff2 fixture mtime relative to diff1 for stat-only test coverage
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt","tests/fixtures/diffs/diff2/1/AAA/BCB/CCC/a.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/1/AAA/ZAW/A/b/c/a_file.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt","tests/fixtures/diffs/diff2/1/AAA/ZAW/D/e/f/b_file.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt","tests/fixtures/diffs/diff2/path1/AAA/BCB/CCC/a.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/path1/AAA/ZAW/A/b/c/a_file.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path2/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/path2/AAA/ZAW/A/b/c/a_file.txt",999));

	bool file_exists = false;

	m_create(char,absolute_path,MEMORY_STRING);

	const char *filename = "precizer";

	ASSERT(SUCCESS == construct_path(filename,absolute_path));

	ASSERT(SUCCESS == check_file_exists(&file_exists,m_text(absolute_path)));

	call(m_del(absolute_path));

	ASSERT(file_exists == true);

	/* Enable UTF-8 */
#if 0
	ASSERT(SUCCESS == set_environment_variable("LC_ALL","C.UTF-8"));
	ASSERT(SUCCESS == set_environment_variable("LANG","C.UTF-8"));
#endif

	RETURN_STATUS;
}
