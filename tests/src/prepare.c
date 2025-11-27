#include "sute.h"

Return prepare(void)
{
	INITTEST;

	const char *command = NULL;

	ASSERT(SUCCESS == execute_and_set_variable("ORIGIN_DIR","readlink -f ${PWD}/..",0));

	ASSERT(SUCCESS == execute_and_set_variable("TMPDIR","mktemp -d /tmp/precizer.XXXXXXXXXXXXXXXXXX",0));

	char environment[PATH_MAX];

	run(extract_current_executable_directory_name(environment,sizeof(environment)));

	ASSERT(SUCCESS == set_environment_variable("ENVIRONMENT",environment));

	ASSERT(SUCCESS == execute_and_set_variable("DBNAME","echo \"$(hostname).db\"",0));

	command = "export TESTDIRS=${TMPDIR}/tests/examples/diffs/;"
	        "mkdir -p ${TESTDIRS};"
	        "cd ${TMPDIR};"
	        "cp -apr $ORIGIN_DIR/tests/examples/diffs/diff* ${TESTDIRS};"
	        "cp -apr $ORIGIN_DIR/tests/examples/*apos* ${TESTDIRS}/../;"
	        "cp -apr $ORIGIN_DIR/tests/examples/levels/ ${TESTDIRS}/../;"
	        "cp -apr $ORIGIN_DIR/tests/examples/4/ ${TESTDIRS}/../;"
	        "cp -apr $ORIGIN_DIR/tests/examples/long/ ${TESTDIRS}/../;"
	        "cp -apr $ORIGIN_DIR/tests/templates/0015_database_v*.db ${TESTDIRS}/../../;"
	        "test -f $ORIGIN_DIR/.builds/${ENVIRONMENT}/precizer && cp -apr $ORIGIN_DIR/.builds/${ENVIRONMENT}/precizer .;"
	        "mkdir -p .builds/${ENVIRONMENT}/;"
	        "test -d $ORIGIN_DIR/.builds/${ENVIRONMENT}/ && cp -apr $ORIGIN_DIR/.builds/${ENVIRONMENT}/ .builds/;"
	        "true";

	ASSERT(SUCCESS == external_call(command,SUCCESS,false,false));

	bool file_exists = false;

	create(char,path);

	const char *filename = "precizer";

	ASSERT(SUCCESS == construct_path(filename,path));

	ASSERT(SUCCESS == check_file_exists(&file_exists,getcstring(path)));

	del(path);

	if(file_exists == true)
	{
		ASSERT(SUCCESS == execute_and_set_variable("BINDIR","echo \"${TMPDIR}/\"",0));
	} else {
		provide(FAILURE);
	}

	/* Enable UTF-8 */
	ASSERT(SUCCESS == set_environment_variable("LC_ALL","C.UTF-8"));
	ASSERT(SUCCESS == set_environment_variable("LANG","C.UTF-8"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	return(status);
}
