#include "sute.h"

Return prepare(void)
{
	INITTEST;

	const char *command = NULL;

	char path[PATH_MAX] = {0};

	ASSERT(SUCCESS == get_origin_dir(path,sizeof(path)));

	ASSERT(SUCCESS == set_environment_variable("ORIGIN_DIR",path));

	ASSERT(SUCCESS == create_tmpdir(path, sizeof(path)));

	ASSERT(SUCCESS == set_environment_variable("TMPDIR",path));

	ASSERT(SUCCESS == set_environment_variable("BINDIR",path));

	ASSERT(SUCCESS == extract_current_executable_directory_name(path,sizeof(path)));

	/**
	 * When the code coverage target from the Makefile is run,
	 * the runtime environment needs to be "debug", because
	 * coverage has to execute a binary with debug symbols and
	 * there is no need to build a separate binary in addition
	 * to the already compiled debug one
	*/
	if(strcmp(path, "coverage") == 0) {
		/* Replace to "debug" */
		strncpy(path, "debug", PATH_MAX - 1);
	}

	ASSERT(SUCCESS == set_environment_variable("ENVIRONMENT",path));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == execute_and_set_variable("DBNAME","echo \"$(hostname).db\"",0));

	command = "mkdir -p ${TMPDIR}/tests/examples/diffs/;"
	          "cp -a $ORIGIN_DIR/tests/examples/diffs/diff* ${TMPDIR}/tests/examples/diffs/;"
	          "cp -a $ORIGIN_DIR/tests/examples/*apos* ${TMPDIR}/tests/examples/;"
	          "cp -a $ORIGIN_DIR/tests/examples/levels ${TMPDIR}/tests/examples/; ls -laR ${TMPDIR}/tests/examples/levels;"
	          "cp -a $ORIGIN_DIR/tests/examples/4 ${TMPDIR}/tests/examples/;"
	          "cp -a $ORIGIN_DIR/tests/templates/0015_database_v*.db ${TMPDIR}/tests/;"
	          "mkdir -p ${TMPDIR}/.builds;"
	          "test -d $ORIGIN_DIR/.builds/${ENVIRONMENT} && cp -a $ORIGIN_DIR/.builds/${ENVIRONMENT} ${TMPDIR}/.builds/;"
	          "test -f $ORIGIN_DIR/.builds/${ENVIRONMENT}/precizer && cp -a $ORIGIN_DIR/.builds/${ENVIRONMENT}/precizer ${TMPDIR};"
	          "test -d $ORIGIN_DIR/tests/examples/long && cp -a $ORIGIN_DIR/tests/examples/long ${TMPDIR}/tests/examples/;"
	          "false";

	ASSERT(SUCCESS == external_call(command,COMPLETED,false,false));

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
