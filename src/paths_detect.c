#include "precizer.h"

/**
 * @brief Validate the traversal roots supplied for normal scanning mode
 *
 * The function is skipped in `--compare` mode because compare arguments are
 * database files, not directories to traverse. In normal mode it requires at
 * least one root path and checks every root as an existing directory before
 * traversal begins
 *
 * For example, `precizer tests/fixtures/diffs` stores that positional argument
 * in `config->roots`; this function verifies that the directory is available
 * before later code starts reading files from it
 *
 * @return `SUCCESS` when compare mode is active or all roots are available.
 *         `FAILURE` when no root was provided or at least one root is not an
 *         accessible directory
 */
Return paths_detect(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		// The option to compare databases has been selected.
		// There is no need to compare paths
		slog(TRACE,"Comparing databases. Directory path verification is not required\n");
		provide(status);

	} else {
		// Check directory paths passed as arguments, traverse
		// them for files, and store the file metadata in the database
		slog(TRACE,"Checking directory paths provided as arguments\n");
	}

	// Check traversal roots supplied as positional arguments
	if(config->roots.length == 0)
	{
		slog(ERROR,"The PATH is not defined\n");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		m_string_array_foreach(conf(roots),root)
		{
			const char *root_path = m_text(root);

			if(NOT_FOUND == file_availability(root_path,NULL,SHOULD_BE_A_DIRECTORY))
			{
				status = FAILURE;
				break;
			}
		}
	}

	slog(TRACE,"Path detection is finished\n");

	provide(status);
}
