#include "sute.h"

Return prepare(void)
{
	INITTEST;

	const char *command = NULL;

	char path[PATH_MAX] = {0};

	ASSERT(SUCCESS == get_origin_dir(path,sizeof(path)));

	ASSERT(SUCCESS == set_environment_variable("ORIGIN_DIR",path));

	ASSERT(SUCCESS == create_tmpdir(path,sizeof(path)));

	ASSERT(SUCCESS == set_environment_variable("TMPDIR",path));

	ASSERT(SUCCESS == set_environment_variable("BINDIR",path));

	ASSERT(SUCCESS == extract_current_executable_directory_name(path,sizeof(path)));

	ASSERT(SUCCESS == set_environment_variable("ENVIRONMENT",path));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == execute_and_set_variable("DBNAME","echo \"$(hostname).db\"",0));

	command = "mkdir -p ${TMPDIR}/tests/fixtures/diffs/;"
	        "cp -a $ORIGIN_DIR/tests/fixtures/diffs/diff* ${TMPDIR}/tests/fixtures/diffs/;"
	        "cp -a $ORIGIN_DIR/tests/fixtures/*apos* ${TMPDIR}/tests/fixtures/;"
	        "cp -a $ORIGIN_DIR/tests/fixtures/levels ${TMPDIR}/tests/fixtures/;"
	        "cp -a $ORIGIN_DIR/tests/fixtures/4 ${TMPDIR}/tests/fixtures/;"
	        "cp -a $ORIGIN_DIR/tests/fixtures/ignore_include_cases ${TMPDIR}/tests/fixtures/;"
	        "mkdir -p ${TMPDIR}/.builds;"
	        "test -d $ORIGIN_DIR/.builds/${ENVIRONMENT} && cp -a $ORIGIN_DIR/.builds/${ENVIRONMENT} ${TMPDIR}/.builds/;"
	        "test -f $ORIGIN_DIR/.builds/${ENVIRONMENT}/precizer && cp -a $ORIGIN_DIR/.builds/${ENVIRONMENT}/precizer ${TMPDIR};"
	        "test -d $ORIGIN_DIR/tests/fixtures/long && cp -a $ORIGIN_DIR/tests/fixtures/long ${TMPDIR}/tests/fixtures/;"
	        "true";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Bump diff2 fixture mtime relative to diff1 for stat-only test coverage
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt","tests/fixtures/diffs/diff2/1/AAA/BCB/CCC/a.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/1/AAA/ZAW/A/b/c/a_file.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt","tests/fixtures/diffs/diff2/1/AAA/ZAW/D/e/f/b_file.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt","tests/fixtures/diffs/diff2/path1/AAA/BCB/CCC/a.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/path1/AAA/ZAW/A/b/c/a_file.txt",999));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path2/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/path2/AAA/ZAW/A/b/c/a_file.txt",999));

	bool file_exists = false;

	create(char,absolute_path);

	const char *filename = "precizer";

	ASSERT(SUCCESS == construct_path(filename,absolute_path));

	ASSERT(SUCCESS == check_file_exists(&file_exists,getcstring(absolute_path)));

	del(absolute_path);

	ASSERT(file_exists == true);

	/* Enable UTF-8 */
#if 0
	ASSERT(SUCCESS == set_environment_variable("LC_ALL","C.UTF-8"));
	ASSERT(SUCCESS == set_environment_variable("LANG","C.UTF-8"));
#endif

	return(status);
}
