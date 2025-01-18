#include "sute.h"

Return test0018_1_maxdepth_argument(void){
	Return status = SUCCESS;

	/* File system traversal with a maximum depth of 3 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --maxdepth=3 --database=database3.db ${TMPDIR}/tests/examples/levels";

	MSTRUCT(mem_char,result);

	char *pattern = NULL;

	const char *filename = "templates/0018_001_1.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	free(pattern);
	pattern = NULL;
	del_char(&result);

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	/* File system traversal with unlimited depth */
	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update --database=database3.db ${TMPDIR}/tests/examples/levels";

	filename = "templates/0018_001_2.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	ASSERT(SUCCESS == get_file_content(filename,&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	free(pattern);
	pattern = NULL;
	del_char(&result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/database3.db\"",SUCCESS,false,false));
	RETURN_STATUS;
}

Return test0018_2_comparing_templates(void){
	Return status = SUCCESS;

	char *text1 = NULL;
	char *text2 = NULL;
	char *diff = NULL;
	char *pattern = NULL;
	const char *filename = NULL;

	/* 0018 001 */
	ASSERT(SUCCESS == get_file_content("templates/0018_001_1.txt",&text1));

	ASSERT(SUCCESS == get_file_content("templates/0018_001_2.txt",&text2));

	ASSERT(SUCCESS == compare_strings(&diff,text1,text2));

	filename = "templates/0018_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(diff,pattern,filename));

	free(text1);
	free(text2);
	free(diff);
	free(pattern);

	RETURN_STATUS;
}

/**
 *
 * --maxdepth argument testing
 *
 */
Return test0018(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(test0018_1_maxdepth_argument,"Traversal with limited depth…")
	TEST(test0018_2_comparing_templates,"Comparing templates w/ and w/o depth limits…")

	RETURN_STATUS;
}
