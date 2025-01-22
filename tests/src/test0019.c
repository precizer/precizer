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
Return test0019(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *pattern = NULL;

	// Create memory for the result
	MSTRUCT(mem_char,result);

	// Preparation for the test
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"cp -pr tests/examples/ tests/examples_backup;",SUCCESS,false,false));

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "ln -s ../../../../1/AAA/BCB/CCC/a.txt tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt;"
	        "ln -s ../../../../AAA/ZAW/D/e/f tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f;"
	        "ln -s /to/nowhere tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink;"
	        "${BINDIR}/precizer --database=database1.db tests/examples/diffs/diff1";

	const char *filename = "templates/0019_001.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt;"
	        "rm tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f;"
	        "rm tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink;"
	        "${BINDIR}/precizer --update --database=database1.db tests/examples/diffs/diff1;";

	filename = "templates/0019_002.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "ln -s ../../../../1/AAA/BCB/CCC/a.txt tests/examples/diffs/diff1/path1/AAA/BCB/CCC/symlink_to_the_file_a.txt;"
	        "ln -s ../../../../AAA/ZAW/D/e/f tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/symlink_to_dir_f;"
	        "ln -s /to/nowhere tests/examples/diffs/diff1/path1/AAA/ZAW/A/b/broken_symlink;"
	        "${BINDIR}/precizer --update --database=database1.db tests/examples/diffs/diff1;";

	filename = "templates/0019_003.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db;"
		"rm -rf tests/examples/;"
		"mv tests/examples_backup/ tests/examples/",SUCCESS,false,false));

	RETURN_STATUS;
}