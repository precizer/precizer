#include "sute.h"

/**
 *
 * Testing database creation attempt in missing directory
 *
 */
Return test0020(void){
	Return status = SUCCESS;

	/* File system traversal with a maximum depth of 3 */
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update --database=nonexistent_directory/database1.db tests/examples/diffs/diff1";

	MSTRUCT(mem_char,result);

	char *pattern = NULL;

	const char *filename = "templates/0020.txt";

	ASSERT(SUCCESS == execute_command(command,result,FAILURE,false,false));

	ASSERT(SUCCESS == get_file_content(filename,&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	RETURN_STATUS;
}
