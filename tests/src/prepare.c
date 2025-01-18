#include "sute.h"

Return prepare(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	const char *command = NULL;

	ASSERT(SUCCESS == execute_and_set_variable("ORIGIN_DIR","readlink -f ${PWD}/..",0));

	ASSERT(SUCCESS == execute_and_set_variable("TMPDIR","mktemp -d /tmp/precizer.XXXXXXXXXXXXXXXXXX",0));

	ASSERT(SUCCESS == execute_and_set_variable("DBNAME","echo \"$(hostname).db\"",0));

#if 0 // These operations moved to Makefile
	ASSERT(SUCCESS == set_environment_variable("ASAN_OPTIONS","symbolize=1"));
	ASSERT(SUCCESS == execute_and_set_variable("ASAN_SYMBOLIZER_PATH","which llvm-symbolizer",0));
	ASSERT(SUCCESS == set_environment_variable("CC","gcc-12"));
	ASSERT(SUCCESS == external_call("cd $ORIGIN_DIR;make sanitize > /dev/null 2>&1",SUCCESS,false,false));
#endif

	command = "export TESTDIRS=${TMPDIR}/tests/examples/diffs/;"
	        "mkdir -p ${TESTDIRS};"
	        "cd ${TMPDIR};"
	        "cp -apr $ORIGIN_DIR/tests/examples/diffs/diff* ${TESTDIRS};"
	        "cp -apr $ORIGIN_DIR/tests/examples/levels/ ${TESTDIRS}/../;"
	        "cp -apr $ORIGIN_DIR/tests/templates/0015_database_v0.db ${TESTDIRS}/../../;"
	        "test -f $ORIGIN_DIR/precizer && cp -apr $ORIGIN_DIR/precizer .;"
	        "mkdir -p .builds/debug/;"
	        "mkdir -p .builds/sanitize/;"
	        "test -d $ORIGIN_DIR/.builds/debug/libs/ && cp -apr $ORIGIN_DIR/.builds/debug/libs/ .builds/debug/;"
	        "test -d $ORIGIN_DIR/.builds/sanitize/ && cp -apr $ORIGIN_DIR/.builds/sanitize/ .builds/;"
	        "true";

	ASSERT(SUCCESS == external_call(command,SUCCESS,false,false));

	bool file_exists = false;

	char *path = NULL;

	const char *filename = "precizer";

	ASSERT(SUCCESS == construct_path(filename,&path));

	ASSERT(SUCCESS == check_file_exists(&file_exists,path));

	free(path);

	if(file_exists == true)
	{
		ASSERT(SUCCESS == execute_and_set_variable("BINDIR","echo \"${TMPDIR}/\"",0));
	} else {
		ASSERT(SUCCESS == execute_and_set_variable("BINDIR","echo \"${TMPDIR}/.builds/sanitize/\"",0));
	}

	command = "export TESTING=true;"
	        "export ASAN_OPTIONS;"
	        "export ASAN_SYMBOLIZER_PATH;PATH=.:${PATH}";

	ASSERT(SUCCESS == external_call(command,SUCCESS,false,false));

	return(status);
}
