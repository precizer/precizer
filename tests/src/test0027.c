#include "sute.h"

/**
 *
 * Attempt to modify files protected by the --lock-checksum
 *
 */
static Return test0027_1(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff2"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=lock.db --lock-checksum=\"^diff1/1/.*\" "
	        "--lock-checksum=\"^diff2/path1/.*\" tests/fixtures/diffs";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0027_001_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	arguments = "--update --database=lock.db --lock-checksum=\"^diff1/1/.*\" "
	        "--lock-checksum=\"^diff2/path1/.*\" tests/fixtures/diffs";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0027_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	filename = "templates/0027_001_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == add_string_to("corrupted","tests/fixtures/diffs/diff1/1/AAA/ZAW/A/b/c/a_file.txt"));
	ASSERT(SUCCESS == add_string_to("corrupted","tests/fixtures/diffs/diff2/path1/AAA/BCB/CCC/b.txt"));
	ASSERT(SUCCESS == add_string_to("changed","tests/fixtures/diffs/diff2/3/AAA/BBB/CCC/a.txt"));

	ASSERT(SUCCESS == copy_path("lock.db","lock1.db"));

	// Bump file mtime by a nanosecond delta without changing file content
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,"tests/fixtures/diffs/diff2/2/AAA/BBB/CZC/a.txt",999));

	arguments = "--progress --update --database=lock.db "
	        "--lock-checksum=\"^diff1/1/.*\" "
	        "--lock-checksum=\"^diff2/path1/.*\" tests/fixtures/diffs";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	arguments = "--progress --update --database=lock1.db "
	        "--lock-checksum=\"^diff1/1/.*\" "
	        "--lock-checksum=\"^diff2/path1/.*\" tests/fixtures/diffs";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0027_001_4.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("lock.db"));
	ASSERT(SUCCESS == delete_path("lock1.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff2"));

	m_del(result);
	m_del(pattern);

	RETURN_STATUS;
}

Return test0027(void)
{
	INITTEST;

	TEST(test0027_1,"Attempt to modify files protected by the --lock-checksum");

	RETURN_STATUS;
}
