#include "sute.h"

/**
 * @brief Tests the extract_relative_path function with various path and prefix cases.
 *
 * This function initializes a set of test cases containing different combinations
 * of file paths and prefixes, both absolute and relative. It then calls the
 * extract_relative_path function for each case and compares the result with the
 * expected output. The function asserts correctness and returns a test status.
 *
 * The test cases include:
 * - Absolute paths with different prefixes
 * - Relative paths with different prefixes
 * - Paths in the root directory
 *
 * @return Test status indicating success or failure.
 */
Return test0023(void)
{
	INITTEST;

	struct {
		const char *path;
		const char *prefix;
		const char *expected_result;
	} test_cases[] = {
		// Absolute paths with different prefixes
		{"/home/user/project/file.txt","/home/user","project/file.txt"},
		{"/home/user/project/file.txt","/","home/user/project/file.txt"},
		{"/home/user/project/file.txt","/home/user/project","file.txt"},
		{"/home/user/project/file.txt","/home/user/project/file.txt","."},
		{"/home/user/project/file.txt","/invalid/prefix","/home/user/project/file.txt"},
		{"/home/user/project","/home/user/project","."},
		{"/home/user/project/","/home/user","project/"},
		{"/home/user/project/","/home/user/project","."},
		{"/home/user/project/","/","home/user/project/"},

		// Prefix as a relative path
		{"home/user/project/file.txt","home/user","project/file.txt"},
		{"home/user/project/file.txt","home","user/project/file.txt"},
		{"home/user/project/file.txt","home/user/project","file.txt"},
		{"home/user/project/file.txt","home/user/project/file.txt","."},
		{"home/user/project/file.txt","invalid/prefix","home/user/project/file.txt"},
		{"home/user/project","home/user/project","."},
		{"home/user/project/","home/user","project/"},
		{"home/user/project/","home/user/project","."},
		{"home/user/project/","home","user/project/"},

		// Absolute path as a relative path
		{"./file.txt","./","file.txt"},
		{"./project/file.txt","./project","file.txt"},
		{"./project/file.txt",".","project/file.txt"},
		{"../project/file.txt","../project","file.txt"},
		{"../../file.txt","../..","file.txt"},

		// Relative paths without "./"
		{"file.txt","file.txt","."},
		{"project/file.txt","project","file.txt"},
		{"project/subdir/file.txt","project/subdir","file.txt"},
		{"../file.txt","..","file.txt"},
		{"../project/file.txt","../project","file.txt"},
		{"../../file.txt","../..","file.txt"},

		// Paths in the root directory with prefix "/"
		{"/.package-cache-mutate","/",".package-cache-mutate"},
		{"/..package-cache-mutate","/","..package-cache-mutate"},
		{"/config/settings.json","/","config/settings.json"},
		{"/var/log/system.log","/var","log/system.log"},
		{"/var/log/system.log","/","var/log/system.log"},
		{"/tmp/file.txt","/tmp","file.txt"},
		{"/file.txt","/","file.txt"},
		{NULL,NULL,NULL} // Sentinel value
	};

	// Run tests and check results
	for(size_t i = 0; test_cases[i].path != NULL; i++)
	{
		const char *expected = test_cases[i].expected_result;
		const char *result = extract_relative_path(test_cases[i].path,test_cases[i].prefix);

		// Compare result with expected value
		ASSERT(GRACEFUL == strcmp(result,expected));
	}

	RETURN_STATUS;
}
