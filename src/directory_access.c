#include "precizer.h"

/**
 * @brief Check directory accessibility and skip its subtree when needed
 *
 * Uses @p entry->fts_path for the access check itself.
 * Uses @p root_path only to derive a relative-path view for log output
 *
 * @param[in] file_systems FTS traversal handle
 * @param[in] entry Current FTS directory entry
 * @param[in] root_path Descriptor holding the traversal root without a trailing slash
 * @param[in,out] first_iteration Banner sentinel for the first visible output line
 * @param[in,out] summary Traversal state used by slog_show()
 * @return SUCCESS when the directory was handled cleanly, otherwise FAILURE
 */
Return directory_access_verify(
	FTS              *file_systems,
	FTSENT           *entry,
	const memory     *root_path,
	bool             *first_iteration,
	TraversalSummary *summary)
{
	if(root_path == NULL)
	{
		return(SUCCESS);
	}

	FileAccessStatus access_status = file_check_access(entry->fts_path,(size_t)entry->fts_pathlen,R_OK | X_OK);

	if(access_status == FILE_ACCESS_ERROR)
	{
		return(FAILURE);
	}

	if(access_status == FILE_ACCESS_DENIED || access_status == FILE_NOT_FOUND)
	{
		const char *relative_path = extract_relative_path(entry->fts_path,root_path);

		slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,"inaccessible directory %s\n",relative_path);

		if(config->include_specified == false)
		{
			(void)fts_set(file_systems,entry,FTS_SKIP);
		}
	}

	return(SUCCESS);
}
