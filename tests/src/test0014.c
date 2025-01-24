#include "sute.h"

Return long_relative_path_test(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *pattern = NULL;

	// Create memory for the result
	MSTRUCT(mem_char,result);

	/*
	 * First test in the series. Database update with a new path
	 * specification and the --force parameter. The new path consists
	 * of a very long chain of nested subdirectories.
	 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=database1.db tests/examples/diffs/diff1;"
	        "${BINDIR}/precizer --update --force --database=database1.db tests/examples/long;"
	        "rm database1.db;";

	const char *filename = "templates/0014_001_1.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Second test in the series. First, create a file database with a very
	 * long subdirectory path. Then update this database using a new, short
	 * path and the --force option.
	 */
	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=database1.db tests/examples/long;"
	        "${BINDIR}/precizer --update --force --database=database1.db tests/examples/diffs/diff2;"
	        "rm database1.db;";

	filename = "templates/0014_001_2.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Third test in the series. Very long path as the
	 * primamary directory for recursive file traversal
	 */
	FILE *file = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	filename = "examples/the_last_of_longest_path";

	ASSERT(NULL != (file = fopen(filename, "r")));

	ASSERT(-1 != (read = getline(&line, &len, file)));

	if(file != NULL)
	{
		fclose(file);
	}

	if(read > 0)
	{
		line[read] = '\0';
	}

	ASSERT(SUCCESS == set_environment_variable("LONGEST_PATH",line));

	reset(&line);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=database1.db tests/examples/long/${LONGEST_PATH};"
	        "rm database1.db;";

	filename = "templates/0014_001_3.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Test #4 in the series. A very long relative path will
	 * be used to create the database file. SQLite has a significantly
	 * lower built-in path length limit compared to the operating system.
	 */
	file = NULL;
	line = NULL;
	len = 0;
	read = 0;

	filename = "examples/maximum_path_length_of_sqlite";

	ASSERT(NULL != (file = fopen(filename, "r")));

	ASSERT(-1 != (read = getline(&line, &len, file)));

	if(file != NULL)
	{
		fclose(file);
	}

	if(read > 0)
	{
		line[read] = '\0';
	}

	ASSERT(SUCCESS == set_environment_variable("LONG_PATH",line));

	reset(&line);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer "
	        "--database=tests/examples/long/${LONG_PATH}/database1.db "
	        "--ignore=\"^${LONG_PATH}/database1\\.db$\" "
	        "tests/examples/long;"
	        "rm tests/examples/long/${LONG_PATH}/database1.db;";

	filename = "templates/0014_001_4.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	RETURN_STATUS;
}

Return long_absolute_path_test(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *pattern = NULL;

	// Create memory for the result
	MSTRUCT(mem_char,result);

	/*
	 * First test in the series. Database update with a new path
	 * specification and the --force parameter. The new path consists
	 * of a very long chain of nested subdirectories.
	 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/diffs/diff1;"
	        "${BINDIR}/precizer --update --force --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/long;"
	        "rm ${TMPDIR}/database1.db;";

	const char *filename = "templates/0014_002_1.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Second test in the series. First, create a file database with a very
	 * long subdirectory path. Then update this database using a new, short
	 * path and the --force option.
	 */
	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/long;"
	        "${BINDIR}/precizer --update --force --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/diffs/diff2;"
	        "rm ${TMPDIR}/database1.db;";

	filename = "templates/0014_002_2.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Third test in the series. Very long path as the
	 * primamary directory for recursive file traversal
	 */
	FILE *file = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	filename = "examples/the_last_of_longest_path";

	ASSERT(NULL != (file = fopen(filename, "r")));

	ASSERT(-1 != (read = getline(&line, &len, file)));

	if(file != NULL)
	{
		fclose(file);
	}

	if(read > 0)
	{
		line[read] = '\0';
	}

	ASSERT(SUCCESS == set_environment_variable("LONGEST_PATH",line));

	reset(&line);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/long/${LONGEST_PATH};"
	        "rm ${TMPDIR}/database1.db;";

	filename = "templates/0014_002_3.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Test #4 in the series. A very long relative path will
	 * be used to create the database file. SQLite has a significantly
	 * lower built-in path length limit compared to the operating system.
	 */
	file = NULL;
	line = NULL;
	len = 0;
	read = 0;

	filename = "examples/maximum_path_length_of_sqlite";

	ASSERT(NULL != (file = fopen(filename, "r")));

	ASSERT(-1 != (read = getline(&line, &len, file)));

	if(file != NULL)
	{
		fclose(file);
	}

	if(read > 0)
	{
		line[read] = '\0';
	}

	ASSERT(SUCCESS == set_environment_variable("LONG_PATH",line));

	reset(&line);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer "
	        "--database=${TMPDIR}/tests/examples/long/${LONG_PATH}/database1.db "
	        "--ignore=\"^${LONG_PATH}/database1\\.db$\" "
	        "${TMPDIR}/tests/examples/long;"
	        "rm ${TMPDIR}/tests/examples/long/${LONG_PATH}/database1.db;";

	filename = "templates/0014_002_4.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	RETURN_STATUS;
}

Return reset_relative_path_test(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *pattern = NULL;

	// Create memory for the result
	MSTRUCT(mem_char,result);

	/*
	 * First test in the series. Database update with a new path
	 * specification and the --force parameter. The new path consists
	 * of a very long chain of nested subdirectories.
	 */
	const char *command = "unset PATH;export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=database1.db tests/examples/diffs/diff1;"
	        "${BINDIR}/precizer --update --force --database=database1.db tests/examples/long;"
	        "/bin/rm database1.db;";

	const char *filename = "templates/0014_003_1.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Second test in the series. The PATH variable has been removed.
	 * First, create a file database with a very
	 * long subdirectory path. Then update this database using a new, short
	 * path and the --force option.
	 */
	command = "unset PATH;export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=database1.db tests/examples/long;"
	        "${BINDIR}/precizer --update --force --database=database1.db tests/examples/diffs/diff2;"
	        "/bin/rm database1.db;";

	filename = "templates/0014_003_2.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Third test in the series. The PATH variable has been removed.
	 * Very long path as the
	 * primamary directory for recursive file traversal
	 */
	FILE *file = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	filename = "examples/the_last_of_longest_path";

	ASSERT(NULL != (file = fopen(filename, "r")));

	ASSERT(-1 != (read = getline(&line, &len, file)));

	if(file != NULL)
	{
		fclose(file);
	}

	if(read > 0)
	{
		line[read] = '\0';
	}

	ASSERT(SUCCESS == set_environment_variable("LONGEST_PATH",line));

	reset(&line);

	command = "unset PATH;export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=database1.db tests/examples/long/${LONGEST_PATH};"
	        "/bin/rm database1.db;";

	filename = "templates/0014_003_3.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Test #4 in the series. The PATH variable has been removed.
	 * A very long relative path will
	 * be used to create the database file. SQLite has a significantly
	 * lower built-in path length limit compared to the operating system.
	 */
	file = NULL;
	line = NULL;
	len = 0;
	read = 0;

	filename = "examples/maximum_path_length_of_sqlite";

	ASSERT(NULL != (file = fopen(filename, "r")));

	ASSERT(-1 != (read = getline(&line, &len, file)));

	if(file != NULL)
	{
		fclose(file);
	}

	if(read > 0)
	{
		line[read] = '\0';
	}

	ASSERT(SUCCESS == set_environment_variable("LONG_PATH",line));

	reset(&line);

	command = "unset PATH;export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer "
	        "--database=tests/examples/long/${LONG_PATH}/database1.db "
	        "--ignore=\"^${LONG_PATH}/database1\\.db$\" "
	        "tests/examples/long;"
	        "/bin/rm tests/examples/long/${LONG_PATH}/database1.db;";

	filename = "templates/0014_003_4.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	RETURN_STATUS;
}

Return reset_absolute_path_test(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *pattern = NULL;

	// Create memory for the result
	MSTRUCT(mem_char,result);

	/*
	 * First test in the series. The PATH variable has been removed.
	 * Database update with a new path
	 * specification and the --force parameter. The new path consists
	 * of a very long chain of nested subdirectories.
	 */
	const char *command = "unset PATH;export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/diffs/diff1;"
	        "${BINDIR}/precizer --update --force --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/long;"
	        "/bin/rm ${TMPDIR}/database1.db;";

	const char *filename = "templates/0014_004_1.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Second test in the series. The PATH variable has been removed.
	 * First, create a file database with a very
	 * long subdirectory path. Then update this database using a new, short
	 * path and the --force option.
	 */
	command = "unset PATH;export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/long;"
	        "${BINDIR}/precizer --update --force --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/diffs/diff2;"
	        "/bin/rm ${TMPDIR}/database1.db;";

	filename = "templates/0014_004_2.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Third test in the series. The PATH variable has been removed.
	 * Very long path as the
	 * primamary directory for recursive file traversal
	 */
	FILE *file = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	filename = "examples/the_last_of_longest_path";

	ASSERT(NULL != (file = fopen(filename, "r")));

	ASSERT(-1 != (read = getline(&line, &len, file)));

	if(file != NULL)
	{
		fclose(file);
	}

	if(read > 0)
	{
		line[read] = '\0';
	}

	ASSERT(SUCCESS == set_environment_variable("LONGEST_PATH",line));

	reset(&line);

	command = "unset PATH;export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=${TMPDIR}/database1.db ${TMPDIR}/tests/examples/long/${LONGEST_PATH};"
	        "/bin/rm ${TMPDIR}/database1.db;";

	filename = "templates/0014_004_3.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	/*
	 * Test #4 in the series. The PATH variable has been removed.
	 * A very long relative path will
	 * be used to create the database file. SQLite has a significantly
	 * lower built-in path length limit compared to the operating system.
	 */
	file = NULL;
	line = NULL;
	len = 0;
	read = 0;

	filename = "examples/maximum_path_length_of_sqlite";

	ASSERT(NULL != (file = fopen(filename, "r")));

	ASSERT(-1 != (read = getline(&line, &len, file)));

	if(file != NULL)
	{
		fclose(file);
	}

	if(read > 0)
	{
		line[read] = '\0';
	}

	ASSERT(SUCCESS == set_environment_variable("LONG_PATH",line));

	reset(&line);

	command = "unset PATH;export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer "
	        "--database=${TMPDIR}/tests/examples/long/${LONG_PATH}/database1.db "
	        "--ignore=\"^${LONG_PATH}/database1\\.db$\" "
	        "${TMPDIR}/tests/examples/long;"
	        "/bin/rm ${TMPDIR}/tests/examples/long/${LONG_PATH}/database1.db;";

	filename = "templates/0014_004_4.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);


	RETURN_STATUS;
}

// Main test runner
Return test0014(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(long_relative_path_test,"Very long relative path…");
	TEST(long_absolute_path_test,"Long absolute path…");
	TEST(reset_relative_path_test,"Reset PATH and relative path…");
	TEST(reset_absolute_path_test,"Reset PATH and absolute path…");

	RETURN_STATUS;
}
