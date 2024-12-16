#include "sute.h"

Return prepare(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	ASSERT(SUCCESS == execute_and_set_variable("ORIGIN_DIR","readlink -f ${PWD}/..",0));

	ASSERT(SUCCESS == set_environment_variable("CC","gcc-12"));

	ASSERT(SUCCESS == set_environment_variable("ASAN_OPTIONS","symbolize=1"));

	ASSERT(SUCCESS == execute_and_set_variable("ASAN_SYMBOLIZER_PATH","which llvm-symbolizer",0));

	ASSERT(SUCCESS == execute_and_set_variable("TMPDIR","mktemp -d /tmp/precizer.XXXXXXXXXXXXXXXXXX",0));

	ASSERT(SUCCESS == execute_and_set_variable("DBNAME","echo \"$(hostname).db\"",0));

	ASSERT(SUCCESS == external_call("cd $ORIGIN_DIR;make sanitize > /dev/null 2>&1",0));

	if(SUCCESS == status)
	{
		status = external_call("export TESTDIRS=${TMPDIR}/tests/examples/diffs/;" \
			"mkdir -p ${TESTDIRS};" \
			"cd ${TMPDIR};" \
			"cp -r $ORIGIN_DIR/tests/examples/diffs/diff* ${TESTDIRS};" \
			"cp -r $ORIGIN_DIR/.builds/debug/libs/*.so* ./;" \
			"cp $ORIGIN_DIR/.builds/sanitize/precizer ./",0);
	}

	if(SUCCESS == status)
	{
		status = external_call("export TESTING=true;" \
			"export ASAN_OPTIONS;" \
			"export ASAN_SYMBOLIZER_PATH;PATH=.:${PATH}",0);
	}

	return(status);
}
