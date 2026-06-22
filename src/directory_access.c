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
 * @return SUCCESS when the directory access state was handled cleanly
 */
Return directory_access_verify(
	FTS              *file_systems,
	FTSENT           *entry,
	const memory     *root_path,
	bool             *first_iteration,
	TraversalSummary *summary)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(root_path == NULL)
	{
		provide(status);
	}

	/* Directory traversal needs both read permission and execute permission.
	   Read permission lets the program list entries inside the directory, while
	   execute permission lets it enter the directory and reach child paths */
	FileAccessStatus access_status = file_check_access_absolute(entry->fts_path,(size_t)entry->fts_pathlen,R_OK | X_OK);

	/* When a directory cannot be read or has disappeared during traversal, show
	   the user its relative path and remember the warning for the final summary.
	   If no --include rules were supplied, the whole subtree can be skipped
	   because there is no later include pattern that could make a child visible */
	if(access_status == FILE_ACCESS_DENIED
	        || access_status == FILE_NOT_FOUND
	        || access_status == FILE_ACCESS_ERROR)
	{
		m_create(char,relative_path,MEMORY_STRING);

		run(extract_relative_path(relative_path,entry->fts_path,(size_t)entry->fts_pathlen,root_path));

		if(TRIUMPH & status)
		{
			const char *runtime_relative_path = m_text(relative_path);

			slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,"inaccessible directory %s\n",runtime_relative_path);

			if(config->include_specified == false)
			{
				(void)fts_set(file_systems,entry,FTS_SKIP);
			}
		}

		call(m_del(relative_path));
	}

	provide(status);
}
