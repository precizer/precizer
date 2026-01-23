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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR} && "
	        "cp -a tests/0015_database_v0.db .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--database=./0015_database_v0.db tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_001.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	command = "rm \"${TMPDIR}/0015_database_v0.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 0 to the current version as the primary database.
 * Running the test with the --update parameter to ensure the update
 * completes successfully
 *
 */
Return test0015_2_1_upgrade_db(void)
{
	INITTEST;

	const char *command = "cd ${TMPDIR} && "
	        "cp -a tests/0015_database_v0.db .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=0015_database_v0.db "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_002_1.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	command = "rm \"${TMPDIR}/0015_database_v0.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 0 to the current version as the primary database.
 * Running the test with the --update and --watch-timestamps parameters to ensure
 * the update completes successfully with according details in output
 *
 */
Return test0015_2_2_upgrade_db(void)
{
	INITTEST;

	const char *command = "cd ${TMPDIR} && "
	        "cp -a tests/0015_database_v0.db .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--watch-timestamps --update --database=0015_database_v0.db "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_002_2.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

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

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_003.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=./0015_database_v0.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	const char *command = "rm \"${TMPDIR}/0015_database_v0.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *arguments = "tests/examples/diffs/diff1";

	const char *filename = "templates/0015_004.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *command = "cd ${TMPDIR} && "
	        "cp -a tests/0015_database_v0.db .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--compare $DBNAME 0015_database_v0.db";

	const char *filename = "templates/0015_005.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,WARNING));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *arguments = "--compare --update $DBNAME 0015_database_v0.db";

	const char *filename = "templates/0015_006.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	const char *command = "rm \"${TMPDIR}/0015_database_v0.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR} && "
	        "cp -a tests/0015_database_v1.db .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--update --database=0015_database_v1.db "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_007.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

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

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_008.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=./0015_database_v1.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	const char *command = "rm \"${TMPDIR}/0015_database_v1.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *command = "cd ${TMPDIR} && "
	        "cp -a tests/0015_database_v1.db .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--compare --update $DBNAME 0015_database_v1.db";

	const char *filename = "templates/0015_009.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	command = "rm \"${TMPDIR}/0015_database_v1.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *command = "cd ${TMPDIR} && "
	        "cp -a tests/0015_database_v2.db .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--update --database=0015_database_v2.db --verbose "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0015_010.txt";

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *command = "cd ${TMPDIR} && "
	        "cp -a tests/0015_database_v2.db .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--compare --update $DBNAME 0015_database_v2.db";

	const char *filename = "templates/0015_011.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	command = "rm \"${TMPDIR}/${DBNAME}\" && "
	        "rm \"${TMPDIR}/0015_database_v2.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * Create a fresh database inside tests/examples/diffs/ with the UTF-8 name
 * "Это новая база данных.db" and ensure the app can read/write it despite
 * spaces and non-ASCII characters.
 * Then compare it against the legacy database
 * "0015_database_v3 это база данных с пробелами и символами UTF-8.db" that was
 * produced by a well-tested older release when upgraded to the version 3.
 * If the files and checksums match, the current checksum calculation is
 * considered compatible with the legacy well-tested algorithm.
 */
Return test0015_12_checksum_compare(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR} && "
	        "cp -a \"$ORIGIN_DIR/tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db\" .";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	create(char,pattern);
	create(char,result);
	create(char,chunk);

	const char *arguments = "--database=\"Это новая база данных.db\" "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	arguments = "--compare \"Это новая база данных.db\" \"0015_database_v3 "
	        "это база данных с пробелами и символами UTF-8.db\"";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	const char *filename = "templates/0015_012.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
	del(chunk);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm \"${TMPDIR}/Это новая база данных.db\" \"${TMPDIR}/0015_database_v3 это база данных с пробелами и символами UTF-8.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * Testing scenario 15
 *
 * Database upgrade testing:
 * - Upgrade DBs from versions 0, 1, and 2 to the current version as the primary database and verify reruns
 * - Launch the program without specifying a database to ensure that a new database is created with the correct version
 * - Compare a current database with outdated versions (0/1/2) without --update and check for the expected errors
 * - Compare and upgrade outdated databases (0/1/2) using the --compare and --update parameters
 * - Verify upgraded databases version 1 and 2 directly with --update
 */
Return test0015(void)
{
	INITTEST;

	TEST(test0015_1_upgrade_db,"Upgrade a DB from v0 to the current version. Error handling…");
	TEST(test0015_2_1_upgrade_db,"Upgrade a DB from v0 to the current version as the primary database…");
	TEST(test0015_2_2_upgrade_db,"Upgrade a DB from v0 to the current version with --watch-timestamps…");
	TEST(test0015_3_upgrade_db,"Verify that the DB is actually at the current version…");
	TEST(test0015_4_upgrade_db,"Create default name database…");
	TEST(test0015_5_upgrade_db,"Attempting an upgrade with a single --compare parameter…");
	TEST(test0015_6_upgrade_db,"Upgrading from 0 to the last version using the --compare and --update…");
	TEST(test0015_7_upgrade_db,"Upgrade a DB from v1 to the current version as the primary database…");
	TEST(test0015_8_upgrade_db,"Verify that the DB is actually at the current version…");
	TEST(test0015_9_upgrade_db,"Upgrading from 1 to the last version using the --compare and --update…");
	TEST(test0015_10_upgrade_db,"Upgrade a DB from v2 to the current version as the primary database…");
	TEST(test0015_11_upgrade_db,"Upgrading from 2 to the last version using the --compare and --update…");
	TEST(test0015_12_checksum_compare,"Create and compare DBs with UTF-8 names and checksums from legacy DB…");

	RETURN_STATUS;
}
