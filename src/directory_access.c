#include "precizer.h"

/**
 * @brief Check directory accessibility and skip its subtree if needed.
 *
 * Builds an absolute path from runtime_root_path and the FTS entry, calls
 * file_check_access and:
 * - on success returns SUCCESS;
 * - on denied/not found logs a message and sets FTS_SKIP (unless --include was specified);
 * - on access error returns FAILURE.
 *
 * @param file_systems  FTS traversal handle.
 * @param entry         Current FTS directory entry.
 * @param runtime_root_path  Absolute traversal root without trailing slash.
 * @param first_iteration  Banner sentinel for first visible output.
 * @param summary       Traversal state used by slog_show() banners/flags.
 * @return SUCCESS or FAILURE.
 */
Return directory_access_verify(
	FTS              *file_systems,
	FTSENT           *entry,
	const char       *runtime_root_path,
	bool             *first_iteration,
	TraversalSummary *summary)
{
	if(runtime_root_path == NULL)
	{
		return(SUCCESS);
	}

	const char *relative_path = extract_relative_path(entry->fts_path,runtime_root_path);

	FileAccessStatus access_status = file_check_access(entry->fts_path,(size_t)entry->fts_pathlen,R_OK | X_OK);

	if(access_status == FILE_ACCESS_ERROR)
	{
		return(FAILURE);
	}

	if(access_status == FILE_ACCESS_DENIED || access_status == FILE_NOT_FOUND)
	{
		slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,"inaccessible directory %s\n",relative_path);

		if(config->include_specified == false)
		{
			(void)fts_set(file_systems,entry,FTS_SKIP);
		}
	}

	return(SUCCESS);
}
