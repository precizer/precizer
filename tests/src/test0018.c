#include "sute.h"

Return test0018_1_maxdepth_argument(void)
{
	INITTEST;

	/* File system traversal with a maximum depth of 3 */
	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0018_001_1.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--maxdepth=3 --database=database3.db $TMPDIR/tests/examples/levels",result,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	/* File system traversal with unlimited depth */
	filename = "templates/0018_001_2.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--update --database=database3.db $TMPDIR/tests/examples/levels",result,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/database3.db\"",COMPLETED,ALLOW_BOTH));
	RETURN_STATUS;
}

Return test0018_2_comparing_templates(void)
{
	INITTEST;

	create(char,text1);
	create(char,text2);
	char *diff = NULL;
	create(char,pattern);
	const char *filename = NULL;

	/* 0018 001 */
	ASSERT(SUCCESS == get_file_content("templates/0018_001_1.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0018_001_2.txt",text2));

	ASSERT(SUCCESS == compare_strings(&diff,getcstring(text1),getcstring(text2)));

	filename = "templates/0018_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	create(char,diff_buffer);
	ASSERT(SUCCESS == copy_literal(diff_buffer,diff));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	del(text1);
	del(text2);
	reset(&diff);
	del(pattern);
	del(diff_buffer);

	RETURN_STATUS;
}

/**
 *
 * --maxdepth argument testing
 *
 */
Return test0018(void)
{
	INITTEST;

	TEST(test0018_1_maxdepth_argument,"Traversal with limited depth…")
	TEST(test0018_2_comparing_templates,"Comparing templates w/ and w/o depth limits…")

	RETURN_STATUS;
}
