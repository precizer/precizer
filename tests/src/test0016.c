#include "sute.h"

Return test0016(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *pattern = NULL;

	// Create memory for the result
	MSTRUCT(mem_char,result);

	// Preparation for the test
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};" \
		"cp -r tests/examples/ tests/examples_backup;",SUCCESS,false,false));

	const char *command = "export TESTING=true;cd ${TMPDIR};" \
	        "./precizer --database=database1.db tests/examples/diffs/diff1;" \
	        "cp database1.db database2.db;" \
	        "echo -n 'PWOEUNVSODNLKUHGE' >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt;" \
	        "touch tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;" \
	        "rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt;" \
	        "./precizer --update --database=database1.db tests/examples/diffs/diff1;" \
	        "./precizer --compare database1.db database2.db;" \
	        "cp database2.db database1.db;" \
	        "./precizer --watch-timestamps --update --database=database1.db tests/examples/diffs/diff1;" \
	        "./precizer --compare database1.db database2.db";

	const char *filename = "templates/0016_001_1.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	free(pattern);
	pattern = NULL;
	del_char(&result);

	command = "export TESTING=false;cd ${TMPDIR};" \
	        "cp database2.db database1.db;" \
	        "./precizer --update --database=database1.db tests/examples/diffs/diff1;" \
	        "./precizer --compare database1.db database2.db;" \
	        "cp database2.db database1.db;" \
	        "./precizer --watch-timestamps --update --database=database1.db tests/examples/diffs/diff1;" \
	        "./precizer --compare database1.db database2.db";

	filename = "templates/0016_001_2.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	free(pattern);
	pattern = NULL;
	del_char(&result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};" \
		"rm database1.db;" \
		"rm database2.db;" \
		"rm -rf tests/examples/;" \
		"mv tests/examples_backup/ tests/examples/",SUCCESS,false,false));

	RETURN_STATUS;
}
