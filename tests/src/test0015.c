#include "sute.h"

/**
 *
 * Upgrade a DB from version 0 to the current version as the primary database.
 * Verify the run fails without the --update parameter and prints the proper error.
 *
 */
Return test0015_1_upgrade_db(void)
{
	INITTEST;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v0.db .;"
	        "${BINDIR}/precizer --database=./0015_database_v0.db tests/examples/diffs/diff1";

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0015_001.txt";

	ASSERT(SUCCESS == execute_command(command,result,WARNING,false,false));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/0015_database_v0.db\"",GRACEFUL,false,false));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 0 to the current version as the primary database.
 * Running the test with the --update parameter to ensure the update
 * completes successfully
 *
 */
Return test0015_2_upgrade_db(void)
{
	INITTEST;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v0.db .;"
	        "${BINDIR}/precizer --update --database=0015_database_v0.db tests/examples/diffs/diff1";

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0015_002.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Run the program again to verify that the database
 * is actually at the current version
 *
 */
Return test0015_3_upgrade_db(void)
{
	INITTEST;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update --database=./0015_database_v0.db tests/examples/diffs/diff1";

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0015_003.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/0015_database_v0.db\"",GRACEFUL,false,false));

	RETURN_STATUS;
}

/**
 *
 * Run the program again to verify that the database is actually at the current version
 * Create a database with the default name
 *
 */
Return test0015_4_upgrade_db(void)
{
	INITTEST;

	// Get the output of an external program
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer tests/examples/diffs/diff1";

	const char *filename = "templates/0015_004.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,SUCCESS));

	RETURN_STATUS;
}

/**
 *
 * Run the program with the --compare parameter to compare databases
 * when one of them has an older version — this should generate an
 * appropriate error message
 *
 */
Return test0015_5_upgrade_db(void)
{
	INITTEST;

	// Get the output of an external program
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v0.db .;"
	        "${BINDIR}/precizer --compare ${DBNAME} 0015_database_v0.db";

	const char *filename = "templates/0015_005.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	// Will store template content from file
	create(char,pattern);

	// Create memory for command output
	create(char,result);

	// Read template pattern from file
	status = get_file_content(filename,pattern);

	// Replace template placeholder with actual value
	ASSERT(SUCCESS == replace_placeholder(pattern,template,replacement));

	// Execute command and capture output
	ASSERT(SUCCESS == execute_command(command,result,WARNING,false,false));

	// Compare command output against modified template
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);

	del(result);

	RETURN_STATUS;
}

/**
 *
 * Run the database comparison again using the --compare parameter, but this time with
 * the --update option. The database should be upgraded accordingly.
 * Upgrading from 0 to the last version
 *
 */
Return test0015_6_upgrade_db(void)
{
	INITTEST;

	// Get the output of an external program
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --compare --update ${DBNAME} 0015_database_v0.db";

	const char *filename = "templates/0015_006.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,SUCCESS));

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/0015_database_v0.db\"",GRACEFUL,false,false));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 1 to the current version as the primary database.
 * Running the test with the --update parameter to ensure the update
 * completes successfully
 *
 */
Return test0015_7_upgrade_db(void)
{
	INITTEST;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v1.db .;"
	        "${BINDIR}/precizer --update --database=0015_database_v1.db tests/examples/diffs/diff1";

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0015_007.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Run the program again to verify that the database
 * is actually at the current version
 *
 */
Return test0015_8_upgrade_db(void)
{
	INITTEST;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update --database=./0015_database_v1.db tests/examples/diffs/diff1";

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0015_008.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/0015_database_v1.db\"",GRACEFUL,false,false));

	RETURN_STATUS;
}

/**
 *
 * Run the database comparison again using the --compare and --update parameters.
 * Upgrading from 1 to the last version
 *
 */
Return test0015_9_upgrade_db(void)
{
	INITTEST;

	// Get the output of an external program
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v1.db .;"
	        "${BINDIR}/precizer --compare --update ${DBNAME} 0015_database_v1.db";

	const char *filename = "templates/0015_009.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,SUCCESS));

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/0015_database_v1.db\"",GRACEFUL,false,false));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 2 to the current version as the primary database.
 * Running the test with the --update parameter to ensure the update
 * completes successfully
 *
 */
Return test0015_10_upgrade_db(void)
{
	INITTEST;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v2.db .;"
	        "${BINDIR}/precizer --update --database=0015_database_v2.db tests/examples/diffs/diff1";

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0015_010.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Run the database comparison again using the --compare and --update parameters.
 * Upgrading from 2 to the last version
 *
 */
Return test0015_11_upgrade_db(void)
{
	INITTEST;

	// Get the output of an external program
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p tests/0015_database_v2.db .;"
	        "${BINDIR}/precizer --compare --update ${DBNAME} 0015_database_v2.db";

	const char *filename = "templates/0015_011.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,SUCCESS));

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\" && rm \"${TMPDIR}/0015_database_v2.db\"",GRACEFUL,false,false));

	RETURN_STATUS;
}

/**
 * Testing scenario 15
 *
 * Database upgrade testing:
 * - Upgrade a DB from version 0 to version 1 as the primary database
 * - Run the program again to verify that the database is actually at version 1
 * - Launch the program without specifying a database to ensure that a new database is created with the correct version
 * - Run the program with the --compare parameter to compare databases when one of them has an older version — this should generate an appropriate error message
 * - Run the database comparison again using the --compare parameter, but this time with the --update option. The database should be upgraded accordingly.
 */
Return test0015(void)
{
	INITTEST;

	TEST(test0015_1_upgrade_db,"Upgrade a DB from v0 to the current version. Error handling…");
	TEST(test0015_2_upgrade_db,"Upgrade a DB from v0 to the current version as the primary database…");
	TEST(test0015_3_upgrade_db,"Verify that the DB is actually at the current version…");
	TEST(test0015_4_upgrade_db,"Create default name database…");
	TEST(test0015_5_upgrade_db,"Attempting an upgrade with a single --compare parameter…");
	TEST(test0015_6_upgrade_db,"Upgrading from 0 to the last version using the --compare and --update…");
	TEST(test0015_7_upgrade_db,"Upgrade a DB from v1 to the current version as the primary database…");
	TEST(test0015_8_upgrade_db,"Verify that the DB is actually at the current version…");
	TEST(test0015_9_upgrade_db,"Upgrading from 1 to the last version using the --compare and --update…");
	TEST(test0015_10_upgrade_db,"Upgrade a DB from v2 to the current version as the primary database…");
	TEST(test0015_11_upgrade_db,"Upgrading from 2 to the last version using the --compare and --update…");

	RETURN_STATUS;
}
