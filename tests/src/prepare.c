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
	struct utsname utsname = {0};

	m_create(char,path,MEMORY_STRING);
	m_create(char,environment_binary_path,MEMORY_STRING);

	ASSERT(SUCCESS == determine_origin_dir(path));
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

	// Build the expected default DB name from the same hostname source as the application
	ASSERT(uname(&utsname) == 0);
	ASSERT(SUCCESS == m_copy_string(path,utsname.nodename));
	ASSERT(SUCCESS == m_concat_literal(path,".db"));
	ASSERT(SUCCESS == set_environment_variable("DBNAME",m_text(path)));
	call(m_del(path));

	ASSERT(SUCCESS == create_directory("tests/fixtures/diffs"));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/diffs/diff1","tests/fixtures/diffs/diff1",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/diffs/diff2","tests/fixtures/diffs/diff2",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/'apostrophe","tests/fixtures/'apostrophe",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/apostrophe'","tests/fixtures/apostrophe'",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/levels","tests/fixtures/levels",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/4","tests/fixtures/4",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/ignore_include_cases","tests/fixtures/ignore_include_cases",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("tests/templates","tests/templates",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == copy_from_origin("libs/rational/tests/templates","tests/templates/librational",REQUIRE_SOURCE_EXISTS));

#if 0
	/* Fixture used only by the disabled test0014 suite */
	ASSERT(SUCCESS == copy_from_origin("tests/fixtures/long","tests/fixtures/long",ALLOW_MISSING_SOURCE));
#endif

	ASSERT(SUCCESS == m_copy_literal(environment_binary_path,".builds/"));
	ASSERT(SUCCESS == m_concat_string(environment_binary_path,environment_name));
	ASSERT(SUCCESS == m_concat_literal(environment_binary_path,"/precizer"));
	ASSERT(SUCCESS == copy_from_origin(m_text(environment_binary_path),"precizer",ALLOW_MISSING_SOURCE));

	call(m_del(environment_binary_path));

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
