#include "sute.h"

Return prefix_path_with_apostrophe_test(void)
{
	INITTEST;

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	/*
	 * Checking how paths are handled when the directory name starts
	 * with an apostrophe
	 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --progress --database=database1.db tests/examples/\\'apostrophe;";

	const char *filename = "templates/0024_001.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};rm database1.db;",SUCCESS,false,false));

	RETURN_STATUS;
}

Return another_prefix_path_with_apostrophe_test(void)
{
	INITTEST;

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	/*
	 * Checking how paths are handled when the directory name ends
	 * with an apostrophe
	 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --progress --database=database1.db tests/examples/apostrophe\\';";

	const char *filename = "templates/0024_002.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};rm database1.db;",SUCCESS,false,false));

	RETURN_STATUS;
}

/**
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/examples/\'apostrophe/\'apostrophe/
 * Stage 2. Adding:
 * precizer --progress --database=database2.db tests/examples/apostrophe\'/\'apostrophe/apostrophe\'/
 * Final stage. Comparing:
 * precizer --compare database1.db database2.db
 */
static Return adding_and_comparing_with_apostrophe(void)
{
	INITTEST;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --progress --database=database1.db tests/examples/\\'apostrophe/\\'apostrophe/;"
	        "${BINDIR}/precizer --progress --database=database2.db tests/examples/apostrophe\\'/\\'apostrophe/apostrophe\\'/;"
	        "${BINDIR}/precizer --compare database1.db database2.db";

	// Create memory for the result
	create(char,result);

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	create(char,pattern);

	const char *filename = "templates/0024_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db database2.db",SUCCESS,false,false));

	del(pattern);

	del(result);

	RETURN_STATUS;
}

// Main test runner
Return test0024(void)
{
	INITTEST;

	TEST(prefix_path_with_apostrophe_test,"Prefix path with apostrophe…");
	TEST(another_prefix_path_with_apostrophe_test,"Another prefix and apostrophe combination in the name…");
	TEST(adding_and_comparing_with_apostrophe,"Adding and comparing with apostrophe…");

	RETURN_STATUS;
}
