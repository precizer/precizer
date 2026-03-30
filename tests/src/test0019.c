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

	const char *update_arguments = "--update --database=database1.db tests/fixtures/diffs/diff1";

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	// Preparation for the test
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == create_symlink("../../../../1/AAA/BCB/CCC/a.txt","tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt"));
	ASSERT(SUCCESS == create_symlink("../../../../AAA/ZAW/D/e/f","tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f"));
	ASSERT(SUCCESS == create_symlink("/to/nowhere","tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink"));

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

	ASSERT(SUCCESS == create_symlink("../../../../1/AAA/BCB/CCC/a.txt","tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt"));
	ASSERT(SUCCESS == create_symlink("../../../../AAA/ZAW/D/e/f","tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f"));
	ASSERT(SUCCESS == create_symlink("/to/nowhere","tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink"));

	filename = "templates/0019_003.txt";

	ASSERT(SUCCESS == runit(update_arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}
