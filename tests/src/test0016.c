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

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/diff1 tests/examples/diff1_backup;"
	        "cp -a tests/examples/diff1_backup tests/examples/diffs/diff1;";

	// Preparation for the test
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0016_001_1.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--start-device-only --database=database1.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	command = "cd ${TMPDIR} && "
	        "cp -a database1.db database2.db";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	command = "cd ${TMPDIR} && "
	        "echo -n 'PWOEUNVSODNLKUHGE' >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt && "
	        "touch tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt && "
	        "rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--update --check-level=QUICK --database=database1.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	command = "cd ${TMPDIR} && "
	        "cp -a database2.db database1.db";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--watch-timestamps --update --database=database1.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
	del(chunk);

	filename = "templates/0016_001_2.txt";
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	command = "cd ${TMPDIR} && "
	        "cp -a database2.db database1.db";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	command = "cd ${TMPDIR} && "
	        "cp -a database2.db database1.db";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--watch-timestamps --update --database=database1.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
	del(chunk);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db && "
	        "rm database2.db && "
	        "rm -rf tests/examples/diffs/diff1 && "
	        "mv tests/examples/diff1_backup tests/examples/diffs/diff1";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}
