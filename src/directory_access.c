#include "precizer.h"

/**
 * @brief Check directory accessibility and skip its subtree if needed.
 *
 * Builds an absolute path from runtime_root and the FTS entry, calls
 * file_check_access and:
 * - on success returns SUCCESS;
 * - on denied/not found logs a message and sets FTS_SKIP;
 * - on access error returns FAILURE.
 *
 * @param file_systems  FTS traversal handle.
 * @param entry         Current FTS directory entry.
 * @param runtime_root  Absolute traversal root without trailing slash.
 * @return SUCCESS or FAILURE.
 */
Return verify_directory_access(
	FTS        *file_systems,
	FTSENT     *entry,
	const char *runtime_root)
{
	if(runtime_root == NULL)
	{
		return(SUCCESS);
	}

	const char *relative_path = extract_relative_path(entry->fts_path,runtime_root);

	char *absolute_path = NULL;
	int length = asprintf(&absolute_path,"%s/%s",runtime_root,relative_path);

	if(length == -1)
	{
		free(absolute_path);
		return(FAILURE);
	}

	FileAccessStatus access_status = file_check_access(absolute_path,(size_t)length);

	free(absolute_path);

	if(access_status == FILE_ACCESS_ERROR)
	{
		return(FAILURE);
	}

	if(access_status == FILE_ACCESS_DENIED || access_status == FILE_NOT_FOUND)
	{
		slog(EVERY|UNDECOR,"inaccessible %s\n",relative_path);
		(void)fts_set(file_systems,entry,FTS_SKIP);
	}

	return(SUCCESS);
}
