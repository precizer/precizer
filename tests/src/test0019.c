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

	const char *create_symlinks_command = "cd ${TMPDIR} && ln -s ../../../../1/AAA/BCB/CCC/a.txt tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt && "
	        "ln -s ../../../../AAA/ZAW/D/e/f tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f && "
	        "ln -s /to/nowhere tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink";
	const char *restore_fixture_command = "cd ${TMPDIR} && mv tests/fixtures/diff1_backup tests/fixtures/diffs/diff1";
	const char *update_arguments = "--update --database=database1.db tests/fixtures/diffs/diff1";

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/fixtures/diffs/diff1 tests/fixtures/diff1_backup;"
	        "cp -a tests/fixtures/diff1_backup tests/fixtures/diffs/diff1;";

	// Preparation for the test
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == external_call(create_symlinks_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--database=database1.db tests/fixtures/diffs/diff1";

	const char *filename = "templates/0019_001.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink"));

	filename = "templates/0019_002.txt";

	ASSERT(SUCCESS == runit(update_arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == external_call(create_symlinks_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0019_003.txt";

	ASSERT(SUCCESS == runit(update_arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == external_call(restore_fixture_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}
