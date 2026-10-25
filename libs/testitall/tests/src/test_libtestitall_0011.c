#include "test_libtestitall_all.h"

/**
 * @brief Require a failed test-file read to clear a previously populated buffer
 *
 * @param[in] relative_path Test resource path, or NULL to exercise invalid input
 * @return SUCCESS when reading fails and the destination is cleared
 */
static Return expect_test_file_read_failure(const char *relative_path)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,content,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(content,"previous file contents"));
	ASSERT(FAILURE & read_file_content(relative_path,content));
	ASSERT(content->length == 0U);

	if(SUCCESS == status)
	{
		/* Expected failures leave diagnostics in the runner's shared buffer */
		call(m_del(STDERR));
	}
	call(m_del(content));
	deliver(status);
}

/**
 * @brief Read staged test resources without falling back to the working directory
 *
 * A different file at the same relative path in the working directory detects
 * incorrect path resolution and fallback after the resource is removed.
 * Missing settings, empty files, and NULL arguments must fail. The original
 * environment and working directory are restored after every result
 *
 * @return Return status for file contents, path selection, and read failures
 */
Return test_libtestitall_0011(void)
{
	INITTEST;
	const char relative_filename[] = "templates/resource 'quoted' [1].txt";
	const char expected_content[] = "first line\nsecond line without final newline";
	const char *previous_tmpdir = getenv("TMPDIR");
	char *saved_tmpdir = NULL;
	char *saved_cwd = getcwd(NULL,0);
	bool cwd_changed = false;
	bool directories_created[3] = {false,false,false};
	bool files_created[2] = {false,false};
	m_create(char,temporary_directory,MEMORY_STRING);
	m_create(char,tests_directory,MEMORY_STRING);
	m_create(char,templates_directory,MEMORY_STRING);
	m_create(char,cwd_templates_directory,MEMORY_STRING);
	m_create(char,resource_path,MEMORY_STRING);
	m_create(char,cwd_resource_path,MEMORY_STRING);
	m_create(char,tmpdir_with_slash,MEMORY_STRING);
	m_create(char,content,MEMORY_STRING);

	ASSERT(saved_cwd != NULL);

	if(SUCCESS == status && previous_tmpdir != NULL)
	{
		saved_tmpdir = strdup(previous_tmpdir);
		ASSERT(saved_tmpdir != NULL);
	}
	const bool environment_saved = SUCCESS == status;
	ASSERT(SUCCESS == create_tmpdir(temporary_directory));
	const bool temporary_directory_created = SUCCESS == status;
	ASSERT(SUCCESS == m_formatted_string(tests_directory,"%s/tests",m_text(temporary_directory)));
	ASSERT(SUCCESS == m_formatted_string(templates_directory,"%s/templates",m_text(tests_directory)));
	ASSERT(SUCCESS == m_formatted_string(cwd_templates_directory,"%s/templates",m_text(temporary_directory)));
	ASSERT(SUCCESS == m_formatted_string(resource_path,"%s/tests/%s",m_text(temporary_directory),relative_filename));
	ASSERT(SUCCESS == m_formatted_string(cwd_resource_path,"%s/%s",m_text(temporary_directory),relative_filename));
	memory *directories[] = {tests_directory,templates_directory,cwd_templates_directory};
	memory *files[] = {resource_path,cwd_resource_path};
	const char *file_contents[] = {expected_content,"wrong file from working directory"};

	for(size_t i = 0U; i < 3U && SUCCESS == status; ++i)
	{
		const int create_status = mkdir(m_text(directories[i]),0700);
		directories_created[i] = create_status == 0;
		ASSERT(create_status == 0);
	}

	for(size_t i = 0U; i < 2U && SUCCESS == status; ++i)
	{
		FILE *file = fopen(m_text(files[i]),"wb");
		ASSERT(file != NULL);

		if(file != NULL)
		{
			files_created[i] = true;
			ASSERT(fputs(file_contents[i],file) != EOF);
			const int close_status = fclose(file);
			ASSERT(close_status == 0);
		}
	}

	ASSERT(setenv("TMPDIR",m_text(temporary_directory),1) == 0);

	if(SUCCESS == status)
	{
		const int change_status = chdir(m_text(temporary_directory));
		cwd_changed = change_status == 0;
		ASSERT(change_status == 0);
	}
	ASSERT(SUCCESS == read_file_content(relative_filename,content));
	ASSERT(strcmp(m_text(content),expected_content) == 0);
	ASSERT(content->string_length == sizeof(expected_content) - 1U);

	ASSERT(SUCCESS == m_formatted_string(tmpdir_with_slash,"%s/",m_text(temporary_directory)));
	ASSERT(setenv("TMPDIR",m_text(tmpdir_with_slash),1) == 0);
	ASSERT(SUCCESS == read_file_content(relative_filename,content));
	ASSERT(strcmp(m_text(content),expected_content) == 0);

	ASSERT(truncate(m_text(resource_path),0) == 0);
	ASSERT(SUCCESS == expect_test_file_read_failure(relative_filename));

	if(SUCCESS == status)
	{
		const int remove_status = unlink(m_text(resource_path));

		if(remove_status == 0)
		{
			files_created[0] = false;
		}
		ASSERT(remove_status == 0);
	}
	ASSERT(SUCCESS == expect_test_file_read_failure(relative_filename));
	ASSERT(unsetenv("TMPDIR") == 0);
	ASSERT(SUCCESS == expect_test_file_read_failure(relative_filename));
	ASSERT(setenv("TMPDIR","",1) == 0);
	ASSERT(SUCCESS == expect_test_file_read_failure(relative_filename));
	ASSERT(setenv("TMPDIR",m_text(temporary_directory),1) == 0);
	ASSERT(SUCCESS == expect_test_file_read_failure(NULL));
	ASSERT(FAILURE & read_file_content(relative_filename,NULL));

	if(SUCCESS == status)
	{
		call(m_del(STDERR));
	}

	if(cwd_changed == true)
	{
		const int restore_status = chdir(saved_cwd);
		ASSERT(restore_status == 0);
	}

	for(size_t i = 0U; i < 2U; ++i)
	{
		if(files_created[i] == true)
		{
			const int remove_status = unlink(m_text(files[i]));
			ASSERT(remove_status == 0);
		}
	}

	for(size_t i = 3U; i > 0U; --i)
	{
		if(directories_created[i - 1U] == true)
		{
			const int remove_status = rmdir(m_text(directories[i - 1U]));
			ASSERT(remove_status == 0);
		}
	}

	if(temporary_directory_created == true)
	{
		const int remove_status = rmdir(m_text(temporary_directory));
		ASSERT(remove_status == 0);
	}

	if(environment_saved == true)
	{
		int restore_status;

		if(saved_tmpdir != NULL)
		{
			restore_status = setenv("TMPDIR",saved_tmpdir,1);
		} else {
			restore_status = unsetenv("TMPDIR");
		}
		ASSERT(restore_status == 0);
	}
	free(saved_cwd);
	free(saved_tmpdir);
	call(m_del(content));
	call(m_del(tmpdir_with_slash));
	call(m_del(cwd_resource_path));
	call(m_del(resource_path));
	call(m_del(cwd_templates_directory));
	call(m_del(templates_directory));
	call(m_del(tests_directory));
	call(m_del(temporary_directory));
	RETURN_STATUS;
}
