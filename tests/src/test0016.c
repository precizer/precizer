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
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"cp -pr tests/examples/ tests/examples_backup;",COMPLETED,false,false));

	const char *filename = "templates/0016_001_1.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--database=database1.db tests/examples/diffs/diff1",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == copy(result,chunk));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && cp -p database1.db database2.db",COMPLETED,false,false));
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"echo -n 'PWOEUNVSODNLKUHGE' >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt && "
		"touch tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt && "
		"rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt",COMPLETED,false,false));

	ASSERT(SUCCESS == runit("--update --database=database1.db tests/examples/diffs/diff1",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == runit("--compare database1.db database2.db",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && cp -p database2.db database1.db",COMPLETED,false,false));

	ASSERT(SUCCESS == runit("--watch-timestamps --update --database=database1.db tests/examples/diffs/diff1",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == runit("--compare database1.db database2.db",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
	del(chunk);

	filename = "templates/0016_001_2.txt";
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && cp -p database2.db database1.db",COMPLETED,false,false));

	ASSERT(SUCCESS == runit("--update --database=database1.db tests/examples/diffs/diff1",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == copy(result,chunk));

	ASSERT(SUCCESS == runit("--compare database1.db database2.db",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && cp -p database2.db database1.db",COMPLETED,false,false));

	ASSERT(SUCCESS == runit("--watch-timestamps --update --database=database1.db tests/examples/diffs/diff1",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == runit("--compare database1.db database2.db",chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
	del(chunk);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db && "
		"rm database2.db && "
		"rm -rf tests/examples/ && "
		"mv tests/examples_backup/ tests/examples/",COMPLETED,false,false));

	RETURN_STATUS;
}
