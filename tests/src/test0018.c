#include "sute.h"

Return test0018_1(void)
{
	INITTEST;

	/* File system traversal with a maximum depth of 3 */
	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0018_001_1.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--maxdepth=3 --database=database3.db "
	        "$TMPDIR/tests/fixtures/levels";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

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

	arguments = "--database=database4.db "
	        "$TMPDIR/tests/fixtures/levels";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database4.db"));

	RETURN_STATUS;
}

Return test0018_2(void)
{
	INITTEST;

	/* File system traversal with a maximum depth of 3 */
	create(char,result);
	create(char,pattern);

	/* File system traversal with unlimited depth */
	const char *filename = "templates/0018_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=database3.db "
	        "$TMPDIR/tests/fixtures/levels";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database3.db"));

	RETURN_STATUS;
}


Return test0018_3(void)
{
	INITTEST;

	create(char,text1);
	create(char,text2);
	create(char,pattern);
	create(char,diff_buffer);
	char *diff = NULL;

	/* 0018 001 */
	ASSERT(SUCCESS == get_file_content("templates/0018_001_1.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0018_001_2.txt",text2));

	ASSERT(SUCCESS == compare_strings(&diff,getcstring(text1),getcstring(text2)));

	const char *filename = "templates/0018_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == copy_literal(diff_buffer,diff));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	del(text1);
	del(text2);
	del(pattern);
	del(diff_buffer);
	reset(&diff);

	RETURN_STATUS;
}

Return test0018_4(void)
{
	INITTEST;

	create(char,text1);
	create(char,text2);
	create(char,pattern);
	create(char,diff_buffer);
	char *diff = NULL;

	/* 0018 001 */
	ASSERT(SUCCESS == get_file_content("templates/0018_001_1.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0018_002.txt",text2));

	ASSERT(SUCCESS == compare_strings(&diff,getcstring(text1),getcstring(text2)));

	const char *filename = "templates/0018_004.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == copy_literal(diff_buffer,diff));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	del(text1);
	del(text2);
	del(pattern);
	del(diff_buffer);
	reset(&diff);

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

	TEST(test0018_1,"Traversal with limited depth…");
	TEST(test0018_2,"Update DB w/o depth limits…");
	TEST(test0018_3,"Comparing templates w/ and w/o depth limits…")
	TEST(test0018_4,"Comparing templates after update w/o depth limits…")

	RETURN_STATUS;
}
