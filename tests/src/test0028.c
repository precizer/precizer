#include "sute.h"

/**
 * One file is removed, updated, and added at a time
 */
static Return test0028_1_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;" // Remove
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;" // Modify
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;"; // New file

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db database2.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * One file is removed. It should be reflected as a change in one of the databases.
 */
static Return test0028_2_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;"; // Remove

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_002.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db database2.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * One file is added. It should be reflected as a change in one of the databases.
 */
static Return test0028_3_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;"; // New file

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db database2.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * One file is updated and its checksum should change
 */
static Return test0028_4_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;"; // Modify

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_004.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db database2.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * Nothing changes. The databases should be equivalent
 */
static Return test0028_5_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *arguments = "--database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_005.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db database2.db;",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Testing the --compare mode across different types of responses
 *
 */
Return test0028(void)
{
	INITTEST;

	TEST(test0028_1_test,"One file is removed, updated, and added at a time…");
	TEST(test0028_2_test,"One file is removed. It should be reflected as a change in one of the databases…");
	TEST(test0028_3_test,"One file is added. It should be reflected as a change in one of the databases…");
	TEST(test0028_4_test,"One file is updated and its checksum should change…");
	TEST(test0028_5_test,"Nothing changes. The databases should be equivalent…");

	RETURN_STATUS;
}
