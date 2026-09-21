#include "test_libtestitall_all.h"

#include <errno.h>

/**
 * @brief Require a launch error rather than an accepted child exit code
 *
 * The caller buffers start with stale content and must be cleared even when
 * no child starts. A requested exit code of 127 must not make a spawn error
 * successful. The complete diagnostic remains in the shared STDERR buffer
 *
 * @param expected_errno Platform error code returned when the executable cannot start
 * @return SUCCESS when the launch failure and output buffers match expectations
 */
static Return check_external_spawn_error(int expected_errno)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	char expected_error_pattern[128];

	ASSERT(SUCCESS == m_copy_literal(captured_stdout,"stale stdout"));
	ASSERT(SUCCESS == m_copy_literal(captured_stderr,"stale stderr"));
	ASSERT(FAILURE & runit("",captured_stdout,captured_stderr,127,STDERR_ALLOW));
	ASSERT(captured_stdout->string_length == 0U);
	ASSERT(captured_stderr->string_length == 0U);
	ASSERT(STDOUT->string_length == 0U);
	ASSERT(snprintf(expected_error_pattern,sizeof(expected_error_pattern),
		"\\AFailed to spawn process for EXTERNAL_CALL \\(errno %d\\)\\z",expected_errno) > 0);
	ASSERT(SUCCESS == assert_stderr_matches_pattern(expected_error_pattern));

	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	deliver(status);
}

/**
 * @brief Check external runit execution context and executable launch failures
 *
 * A temporary precizer script runs /bin/sh to expose the child's working
 * directory, inherited environment, and quoted arguments through captured
 * output. A real exit code of 127 is accepted when requested, while missing,
 * nonexecutable, and unsupported executable files remain launch failures.
 * The test restores the runner mode, environment, and parent working directory
 * and removes its temporary files even after a failed assertion
 *
 * @return Return status code
 */
Return test_libtestitall_0009(void)
{
	INITTEST;

	static const char *const environment_names[] = {
		"BINDIR","TMPDIR","TESTITALL_SPAWN_TEST_VALUE"
	};
	const char inherited_value[] = "inherited value ; $literal";
	const char arguments[] =
	        "-c 'pwd -P; "
	        "printf \"env=<%s>\\nargc=<%s>\\narg1=<%s>\\narg2=<%s>\\narg3=<%s>\\n\" "
	        "\"$TESTITALL_SPAWN_TEST_VALUE\" \"$#\" \"$1\" \"$2\" \"$3\"; "
	        "printf \"captured stderr\\n\" >&2' "
	        "child 'two words' '' 'literal ; | & > $dollar *'";
	char *saved_environment[3] = {NULL,NULL,NULL};
	const size_t environment_count = sizeof(environment_names) / sizeof(environment_names[0]);
	const enum run_mode saved_run_mode = testitall_runit_mode;
	bool environment_snapshot_ready = false;
	bool directory_created = false;
	bool program_created = false;
	char *original_directory = NULL;
	char *current_directory = NULL;
	char *canonical_temporary_directory = NULL;
	char *expected_stdout = NULL;
	FILE *program_file = NULL;
	m_create(char,temporary_directory,MEMORY_STRING);
	m_create(char,program_path,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	for(size_t index = 0U; index < environment_count && SUCCESS == status; ++index)
	{
		const char *value = getenv(environment_names[index]);

		if(value != NULL)
		{
			saved_environment[index] = strdup(value);
			ASSERT(saved_environment[index] != NULL);
		}
	}

	if(SUCCESS == status)
	{
		environment_snapshot_ready = true;
		original_directory = getcwd(NULL,0);
		ASSERT(original_directory != NULL);
	}

	ASSERT(SUCCESS == create_tmpdir(temporary_directory));

	if(SUCCESS == status)
	{
		directory_created = true;
		canonical_temporary_directory = realpath(m_text(temporary_directory),NULL);
		ASSERT(canonical_temporary_directory != NULL);
	}

	ASSERT(SUCCESS == m_copy(program_path,temporary_directory));
	ASSERT(SUCCESS == m_concat_literal(program_path,"/precizer"));
	ASSERT(setenv("BINDIR",m_text(temporary_directory),1) == 0);
	ASSERT(setenv("TMPDIR",m_text(temporary_directory),1) == 0);
	ASSERT(setenv("TESTITALL_SPAWN_TEST_VALUE",inherited_value,1) == 0);

	if(SUCCESS == status)
	{
		testitall_runit_mode = EXTERNAL_CALL;
	}

	ASSERT(SUCCESS == check_external_spawn_error(ENOENT));

	if(SUCCESS == status)
	{
		program_file = fopen(m_text(program_path),"wb");
		ASSERT(program_file != NULL);
	}

	if(SUCCESS == status)
	{
		program_created = true;
		ASSERT(fputs("#!/bin/sh\nexec /bin/sh \"$@\"\n",program_file) != EOF);
	}

	if(program_file != NULL)
	{
		const int close_status = fclose(program_file);
		program_file = NULL;
		ASSERT(close_status == 0);
	}

	ASSERT(chmod(m_text(program_path),0700) == 0);

	ASSERT(asprintf(&expected_stdout,
		"%s\nenv=<%s>\nargc=<3>\narg1=<two words>\narg2=<>\narg3=<literal ; | & > $dollar *>\n",
		canonical_temporary_directory,inherited_value) >= 0);
	ASSERT(SUCCESS == runit(arguments,captured_stdout,captured_stderr,0,STDERR_ALLOW));
	ASSERT(strcmp(m_text(captured_stdout),expected_stdout) == 0);
	ASSERT(strcmp(m_text(captured_stderr),"captured stderr\n") == 0);

	ASSERT(SUCCESS == runit("-c 'exit 127'",captured_stdout,captured_stderr,127,STDERR_ALLOW));
	ASSERT(captured_stdout->string_length == 0U);
	ASSERT(captured_stderr->string_length == 0U);
	ASSERT(unlink(m_text(program_path)) == 0);

	if(SUCCESS == status)
	{
		program_created = false;
		program_file = fopen(m_text(program_path),"wb");
		ASSERT(program_file != NULL);
	}

	if(SUCCESS == status)
	{
		program_created = true;
		ASSERT(fputs("printf unexpected-shell-fallback\n",program_file) != EOF);
	}

	if(program_file != NULL)
	{
		const int close_status = fclose(program_file);
		program_file = NULL;
		ASSERT(close_status == 0);
	}

	ASSERT(chmod(m_text(program_path),0600) == 0);
	ASSERT(SUCCESS == check_external_spawn_error(EACCES));
	ASSERT(chmod(m_text(program_path),0700) == 0);
	ASSERT(SUCCESS == check_external_spawn_error(ENOEXEC));

	if(SUCCESS == status)
	{
		current_directory = getcwd(NULL,0);
		ASSERT(current_directory != NULL);
		ASSERT(strcmp(current_directory,original_directory) == 0);
	}

	/* Restore shared state even when an assertion stopped the test */
	testitall_runit_mode = saved_run_mode;

	if(environment_snapshot_ready == true)
	{
		for(size_t index = 0U; index < environment_count; ++index)
		{
			int restore_status = 0;

			if(saved_environment[index] != NULL)
			{
				restore_status = setenv(environment_names[index],saved_environment[index],1);
			} else {
				restore_status = unsetenv(environment_names[index]);
			}

			ASSERT(restore_status == 0);
		}
	}

	if(original_directory != NULL)
	{
		const int restore_status = chdir(original_directory);
		ASSERT(restore_status == 0);
	}

	if(program_created == true)
	{
		const int remove_status = unlink(m_text(program_path));
		ASSERT(remove_status == 0);
	}

	if(directory_created == true)
	{
		const int remove_status = rmdir(m_text(temporary_directory));
		ASSERT(remove_status == 0);
	}

	for(size_t index = 0U; index < environment_count; ++index)
	{
		free(saved_environment[index]);
	}

	free(expected_stdout);
	free(canonical_temporary_directory);
	free(current_directory);
	free(original_directory);
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	call(m_del(program_path));
	call(m_del(temporary_directory));

	RETURN_STATUS;
}
