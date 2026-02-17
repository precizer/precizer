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
	        "mv tests/examples/diffs/diff1 tests/examples/diff1_backup;"
	        "mv tests/examples/diffs/diff2 tests/examples/diff2_backup;"
	        "cp -a tests/examples/diff1_backup tests/examples/diffs/diff1;"
	        "cp -a tests/examples/diff2_backup tests/examples/diffs/diff2;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=lock.db --lock-checksum=\"^diff1/1/.*\" "
	        "--lock-checksum=\"^diff2/path1/.*\" tests/examples/diffs";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0027_001_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	arguments = "--update --database=lock.db --lock-checksum=\"^diff1/1/.*\" "
	        "--lock-checksum=\"^diff2/path1/.*\" tests/examples/diffs";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0027_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	filename = "templates/0027_001_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	command = "cd ${TMPDIR};"
	        "echo 'corrupted' >> tests/examples/diffs/diff1/1/AAA/ZAW/A/b/c/a_file.txt;"
	        "echo 'corrupted' >> tests/examples/diffs/diff2/path1/AAA/BCB/CCC/b.txt;"
	        "echo 'changed' >> tests/examples/diffs/diff2/3/AAA/BBB/CCC/a.txt;"
	        "touch tests/examples/diffs/diff2/2/AAA/BBB/CZC/a.txt;"
	        "cp lock.db lock1.db";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--progress --update --database=lock.db "
	        "--lock-checksum=\"^diff1/1/.*\" "
	        "--lock-checksum=\"^diff2/path1/.*\" tests/examples/diffs";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	arguments = "--progress --update --database=lock1.db "
	        "--lock-checksum=\"^diff1/1/.*\" "
	        "--lock-checksum=\"^diff2/path1/.*\" tests/examples/diffs";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0027_001_4.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm lock.db lock1.db && "
	        "rm -rf tests/examples/diffs/diff1 tests/examples/diffs/diff2 && "
	        "mv tests/examples/diff1_backup tests/examples/diffs/diff1 && "
	        "mv tests/examples/diff2_backup tests/examples/diffs/diff2";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

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
