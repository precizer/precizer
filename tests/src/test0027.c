#include "sute.h"

/**
 *
 * Attempt to modify files protected by the --lock-checksum
 *
 */
static Return test0027_1_lock_checksum(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=lock.db --lock-checksum=\"^diff1/1/.*\" --lock-checksum=\"^diff2/path1/.*\" tests/examples/diffs";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0027_001_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	arguments = "--update --database=lock.db --lock-checksum=\"^diff1/1/.*\" --lock-checksum=\"^diff2/path1/.*\" tests/examples/diffs";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	filename = "templates/0027_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	filename = "templates/0027_001_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	command = "cd ${TMPDIR};"
		"echo 'corrupted' >> tests/examples/diffs/diff1/1/AAA/ZAW/A/b/c/a_file.txt;"
		"echo 'corrupted' >> tests/examples/diffs/diff2/path1/AAA/BCB/CCC/b.txt;"
		"echo 'changed' >> tests/examples/diffs/diff2/3/AAA/BBB/CCC/a.txt;"
		"touch tests/examples/diffs/diff2/2/AAA/BBB/CZC/a.txt";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	arguments = "--progress --update --database=lock.db --lock-checksum=\"^diff1/1/.*\" --lock-checksum=\"^diff2/path1/.*\" tests/examples/diffs";

	ASSERT(SUCCESS == runit(arguments,result,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	command = "cd ${TMPDIR} && "
		"rm lock.db && "
		"rm -rf tests/examples/ && "
		"mv tests/examples_backup/ tests/examples/";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	del(result);
	del(pattern);

	RETURN_STATUS;
}

Return test0027(void)
{
	INITTEST;

	TEST(test0027_1_lock_checksum,"Attempt to modify files protected by the --lock-checksum…");

	RETURN_STATUS;
}
