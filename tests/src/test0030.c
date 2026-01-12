#include "sute.h"

#define TARGET_FILE "${TMPDIR}/tests/examples/diffs/diff1/path1/AAA/BCB/CCC/a.txt"

/**
 * Size change with locked checksums should raise a warning during update.
 */
static Return test0030_1_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s1.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_001_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("printf 'pad' >> " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--update --rehash-locked " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s1.db tests/examples/diffs/diff1",
		result,
		WARNING,
		ALLOW_BOTH));

	filename = "templates/0030_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s1.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Timestamp drift with --watch-timestamps produces a warning for locked entries.
 */
static Return test0030_2_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s2.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s2.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("touch -m " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--update --watch-timestamps " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s2.db tests/examples/diffs/diff1",
		result,
		WARNING,
		ALLOW_BOTH));

	filename = "templates/0030_002_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s2.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Timestamp drift without --watch-timestamps should complete successfully.
 */
static Return test0030_3_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s3.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s3.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("touch -m " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--update " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s3.db tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	filename = "templates/0030_003_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s3.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Rehashing locked files while watching timestamps should still succeed.
 */
static Return test0030_4_test(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;"
	        "rm -f lock_s4.db;";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--database=lock_s4.db --progress " \
		"--lock-checksum=\"^path1/.*\" tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	const char *filename = "templates/0030_004_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("touch -m " TARGET_FILE,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == runit("--update --watch-timestamps --rehash-locked " \
		"--lock-checksum=\"^path1/.*\" --database=lock_s4.db tests/examples/diffs/diff1",
		result,
		COMPLETED,
		ALLOW_BOTH));

	filename = "templates/0030_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm -f lock_s4.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Example 10: scenarios with --lock-checksum, --rehash-locked, and --watch-timestamps
 *
 */
Return test0030(void)
{
	INITTEST;

	TEST(test0030_1_test,"Size change with locked checksum triggers a warning…");
	TEST(test0030_2_test,"Timestamp drift with --watch-timestamps triggers a warning…");
	TEST(test0030_3_test,"Timestamp drift without --watch-timestamps completes successfully…");
	TEST(test0030_4_test,"Timestamp drift with --watch-timestamps and --rehash-locked completes successfully…");

	RETURN_STATUS;
}
