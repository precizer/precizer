#include "sute.h"

/**
 *
 * Testing symlink operations
 *
 * Test Scenario:
 * 1. Add symlinks, create database
 * 2. Remove symlinks, update database
 * 3. Add symlinks, update database
 *
 */
Return test0019(void)
{
	INITTEST;

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/diff1 tests/examples/diff1_backup;"
	        "cp -a tests/examples/diff1_backup tests/examples/diffs/diff1;";

	// Preparation for the test
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	command = "cd ${TMPDIR} && "
	        "ln -s ../../../../1/AAA/BCB/CCC/a.txt tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt && "
	        "ln -s ../../../../AAA/ZAW/D/e/f tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f && "
	        "ln -s /to/nowhere tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--database=database1.db tests/examples/diffs/diff1";

	const char *filename = "templates/0019_001.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	command = "cd ${TMPDIR} && "
	        "rm tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt && "
	        "rm tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f && "
	        "rm tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--update --database=database1.db tests/examples/diffs/diff1";

	filename = "templates/0019_002.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	command = "cd ${TMPDIR} && "
	        "ln -s ../../../../1/AAA/BCB/CCC/a.txt tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt && "
	        "ln -s ../../../../AAA/ZAW/D/e/f tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f && "
	        "ln -s /to/nowhere tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--update --database=database1.db tests/examples/diffs/diff1";

	filename = "templates/0019_003.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db && "
	        "rm -rf tests/examples/diffs/diff1 && "
	        "mv tests/examples/diff1_backup tests/examples/diffs/diff1";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}
