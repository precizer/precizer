#include "sute.h"

/**
 * @brief Tests the extract_relative_path function with various path and root-path cases
 *
 * This function initializes a set of test cases containing different combinations
 * of file paths and root paths, both absolute and relative. It then builds the
 * relative path into a string descriptor for each case and compares the result
 * with the expected output. The function asserts correctness and returns a test status
 *
 * The test cases include:
 * - Absolute paths with different root paths
 * - Relative paths with different root paths
 * - Paths in the root directory
 * - Boundary cases where a matching string prefix is not a path prefix
 * - Empty root paths and backslash separators
 *
 * @return Test status indicating success or failure
 */
Return test0023(void)
{
	INITTEST;

	struct {
		const char *path;
		const char *root_path;
		const char *expected_result;
	} test_cases[] = {
		// Absolute paths with different root paths
		{"/home/user/project/file.txt","/home/user","project/file.txt"},
		{"/home/user/project/file.txt","/home/user/","project/file.txt"},
		{"/home/user/project/file.txt","/","home/user/project/file.txt"},
		{"/home/user/project/file.txt","/home/user/project","file.txt"},
		{"/home/user/project/file.txt","/home/user/project/file.txt","."},
		{"/home/user/project/file.txt","/invalid/prefix","/home/user/project/file.txt"},
		{"/home/user","/home/user/project","/home/user"},
		{"/home/user/project","/home/user/project","."},
		{"/home/user/project/","/home/user","project/"},
		{"/home/user/project/","/home/user/project","."},
		{"/home/user/project/","/","home/user/project/"},
		{"/home/user2/file.txt","/home/user","/home/user2/file.txt"},
		{"/home/user-backup/file.txt","/home/user","/home/user-backup/file.txt"},

		// Root path as a relative path
		{"home/user/project/file.txt","home/user","project/file.txt"},
		{"home/user/project/file.txt","home/user/","project/file.txt"},
		{"home/user/project/file.txt","home","user/project/file.txt"},
		{"home/user/project/file.txt","home/user/project","file.txt"},
		{"home/user/project/file.txt","home/user/project/file.txt","."},
		{"home/user/project/file.txt","invalid/prefix","home/user/project/file.txt"},
		{"home/user","home/user/project","home/user"},
		{"home/user/project","home/user/project","."},
		{"home/user/project/","home/user","project/"},
		{"home/user/project/","home/user/project","."},
		{"home/user/project/","home","user/project/"},
		{"home/user2/file.txt","home/user","home/user2/file.txt"},
		{"project_backup/file.txt","project","project_backup/file.txt"},

		// Absolute path as a relative path
		{"./file.txt","./","file.txt"},
		{"./project/file.txt","./project","file.txt"},
		{"./project/file.txt",".","project/file.txt"},
		{"./project2/file.txt","./project","./project2/file.txt"},
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
		{"/","/","."},
		{"/.package-cache-mutate","/",".package-cache-mutate"},
		{"/..package-cache-mutate","/","..package-cache-mutate"},
		{"/config/settings.json","/","config/settings.json"},
		{"/var/log/system.log","/var","log/system.log"},
		{"/var/log/system.log","/","var/log/system.log"},
		{"/tmp/file.txt","/tmp","file.txt"},
		{"/file.txt","/","file.txt"},

		// Paths with backslash separators
		{"root\\dir\\file.txt","root","dir\\file.txt"},
		{"root2\\file.txt","root","root2\\file.txt"},
		{"root\\dir\\file.txt","root\\","dir\\file.txt"},
		{"root\\dir","root\\dir","."},

		// Empty root paths
		{"file.txt","","file.txt"},
		{"/file.txt","","/file.txt"},
		{NULL,NULL,NULL}         // Sentinel value
	};

	// Run tests and check results
	for(size_t i = 0; test_cases[i].path != NULL; i++)
	{
		m_create(char,root_path,MEMORY_STRING);
		m_create(char,relative_path,MEMORY_STRING);
		const char *expected = test_cases[i].expected_result;

		ASSERT(SUCCESS == m_copy_string(root_path,test_cases[i].root_path));
		ASSERT(SUCCESS == extract_relative_path(relative_path,test_cases[i].path,strlen(test_cases[i].path),root_path));

		// Compare result with expected value
		ASSERT(COMPLETED == strcmp(m_text(relative_path),expected));

		m_del(relative_path);
		m_del(root_path);
	}

	RETURN_STATUS;
}
