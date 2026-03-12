#include "sute.h"

/**
 *
 * --watch-timestamps argument testing
 *
 */
Return test0016(void)
{
	INITTEST;

	create(char,pattern);

	// Create memory for the result
	create(char,result);
	create(char,chunk);

	// Preparation for the test
	const char *diff1_fixture_path = "tests/fixtures/diffs/diff1";
	const char *diff1_backup_path = "tests/fixtures/diff1_backup";
	ASSERT(SUCCESS == move_path(diff1_fixture_path,diff1_backup_path));
	ASSERT(SUCCESS == copy_path(diff1_backup_path,diff1_fixture_path));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--start-device-only --database=database1.db tests/fixtures/diffs/diff1",chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	ASSERT(SUCCESS == copy_path("database1.db","database2.db"));

	ASSERT(SUCCESS == add_string_to("PWOEUNVSODNLKUHGE","tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt"));

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt"));

	// Bump file mtime by a nanosecond delta without changing file content
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,"tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt",999));

	ASSERT(SUCCESS == runit("--update --check-level=QUICK --database=database1.db tests/fixtures/diffs/diff1",chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	const char *compare_arguments = "--compare database1.db database2.db";
	ASSERT(SUCCESS == runit(compare_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == copy_path("database2.db","database1.db"));

	const char *watch_update_arguments = "--watch-timestamps --update --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(watch_update_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == runit(compare_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	const char *filename = "templates/0016_001_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
	del(chunk);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	ASSERT(SUCCESS == copy_path("database2.db","database1.db"));

	ASSERT(SUCCESS == runit("--update --database=database1.db tests/fixtures/diffs/diff1",chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	ASSERT(SUCCESS == runit(compare_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == copy_path("database2.db","database1.db"));

	ASSERT(SUCCESS == runit(watch_update_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == runit(compare_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	filename = "templates/0016_001_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
	del(chunk);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == delete_path(diff1_fixture_path));

	ASSERT(SUCCESS == move_path(diff1_backup_path,diff1_fixture_path));

	RETURN_STATUS;
}
