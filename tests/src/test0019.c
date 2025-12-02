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

	// Preparation for the test
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"cp -pr tests/examples/ tests/examples_backup;",COMPLETED,false,false));

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "ln -s ../../../../1/AAA/BCB/CCC/a.txt tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt;"
	        "ln -s ../../../../AAA/ZAW/D/e/f tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f;"
	        "ln -s /to/nowhere tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink;"
	        "${BINDIR}/precizer --database=database1.db tests/examples/diffs/diff1";

	const char *filename = "templates/0019_001.txt";

	ASSERT(SUCCESS == execute_command(command,result,COMPLETED,false,false));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt;"
	        "rm tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f;"
	        "rm tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink;"
	        "${BINDIR}/precizer --update --database=database1.db tests/examples/diffs/diff1;";

	filename = "templates/0019_002.txt";

	ASSERT(SUCCESS == execute_command(command,result,COMPLETED,false,false));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "ln -s ../../../../1/AAA/BCB/CCC/a.txt tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt;"
	        "ln -s ../../../../AAA/ZAW/D/e/f tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f;"
	        "ln -s /to/nowhere tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink;"
	        "${BINDIR}/precizer --update --database=database1.db tests/examples/diffs/diff1;";

	filename = "templates/0019_003.txt";

	ASSERT(SUCCESS == execute_command(command,result,COMPLETED,false,false));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db && "
		"rm -rf tests/examples/ && "
		"mv tests/examples_backup/ tests/examples/",COMPLETED,false,false));

	RETURN_STATUS;
}
