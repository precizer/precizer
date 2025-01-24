#include "sute.h"

/**
 *
 * Upgrade a DB from version 0 to version 1 as the primary database.
 * Error handling for when the program is run without the --update
 * parameter and needs to display an error message.
 *
 */
Return test0015_1_upgrade_db(void){
	Return status = SUCCESS;

	/* File system traversal with a maximum depth of 3 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v0.db .;"
	        "${BINDIR}/precizer --database=./0015_database_v0.db tests/examples/diffs/diff1";

	MSTRUCT(mem_char,result);

	char *pattern = NULL;

	const char *filename = "templates/0015_001.txt";

	ASSERT(SUCCESS == execute_command(command,result,FAILURE,false,false));

	ASSERT(SUCCESS == get_file_content(filename,&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/0015_database_v0.db\"",SUCCESS,false,false));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 0 to version 1 as the primary database.
 * Running the test with the --update parameter to ensure the update
 * completes successfully
 *
 */
Return test0015_2_upgrade_db(void){
	Return status = SUCCESS;

	/* File system traversal with a maximum depth of 3 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v0.db .;"
	        "${BINDIR}/precizer --update --database=0015_database_v0.db tests/examples/diffs/diff1";

	MSTRUCT(mem_char,result);

	char *pattern = NULL;

	const char *filename = "templates/0015_002.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	RETURN_STATUS;
}

/**
 *
 * Run the program again to verify that the database is actually at version 1
 *
 *
 */
Return test0015_3_upgrade_db(void){
	Return status = SUCCESS;

	/* File system traversal with a maximum depth of 3 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update --database=./0015_database_v0.db tests/examples/diffs/diff1";

	MSTRUCT(mem_char,result);

	char *pattern = NULL;

	const char *filename = "templates/0015_003.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/0015_database_v0.db\"",SUCCESS,false,false));

	RETURN_STATUS;
}

/**
 * Testing scenario 15
 *
 * Database upgrade testing:
 * - Upgrade a DB from version 0 to version 1 as the primary database
 * - Run the program again to verify that the database is actually at version 1
 * - Launch the program without specifying a database to ensure that a new database is created with the correct version
 * - Run the program with the --compare parameter to compare databases when one of them has an older version - this should generate an appropriate error message
 * - Run the database comparison again using the --compare parameter, but this time with the --update option. The database should be upgraded accordingly.
 */
Return test0015(void){
	Return status = SUCCESS;

	TEST(test0015_1_upgrade_db,"Upgrade a DB from v0 to v1. Error handling…");
	TEST(test0015_2_upgrade_db,"Upgrade a DB from v0 to v1 as the primary database…");
	TEST(test0015_3_upgrade_db,"Verify that the DB is actually at version 1…");

	RETURN_STATUS;
}
