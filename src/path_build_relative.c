#include "precizer.h"

/**
 * @brief Build a root-relative path for an FTS entry
 *
 * @details
 * Builds a path from the FTS parent chain instead of trimming a textual root
 * prefix from `FTSENT::fts_path`. This keeps the stored file path independent
 * from how the traversal root was written by the user, while preserving that
 * original root spelling in the database `paths` table
 *
 * The root entry itself is represented as ".". Child entries are assembled
 * from their component names, for example `dir/subdir/file.txt`
 *
 * @param[out] relative_path String descriptor that receives the relative path
 * @param[in] entry FTS entry whose path should be built
 * @return SUCCESS when the path is built, otherwise FAILURE
 */
Return path_build_relative(
	memory       *relative_path,
	const FTSENT *entry)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(entry == NULL)
	{
		report("Relative path building; FTS entry must be non-NULL");
		provide(FAILURE);
	}

	if(entry->fts_level < FTS_ROOTLEVEL)
	{
		report("Relative path building; FTS entry is above the traversal root");
		provide(FAILURE);
	}

	if(entry->fts_level == FTS_ROOTLEVEL)
	{
		run(m_copy_literal(relative_path,"."));
		provide(status);
	}

	size_t relative_path_length = 0U;
	size_t component_count = 0U;

	for(const FTSENT *node = entry;
	        node != NULL && node->fts_level > FTS_ROOTLEVEL;
	        node = node->fts_parent)
	{
		if(node->fts_namelen == 0U)
		{
			report("Relative path building; FTS entry has an invalid component name");
			provide(FAILURE);
		}

		if(relative_path_length > SIZE_MAX - (size_t)node->fts_namelen)
		{
			report("Relative path building; Path length overflow");
			provide(FAILURE);
		}

		relative_path_length += (size_t)node->fts_namelen;

		if(component_count > 0U)
		{
			if(relative_path_length == SIZE_MAX)
			{
				report("Relative path building; Path separator overflow");
				provide(FAILURE);
			}

			relative_path_length++;
		}

		component_count++;
	}

	const FTSENT *root_entry = entry;

	while(root_entry != NULL && root_entry->fts_level > FTS_ROOTLEVEL)
	{
		root_entry = root_entry->fts_parent;
	}

	if(root_entry == NULL || root_entry->fts_level != FTS_ROOTLEVEL)
	{
		report("Relative path building; FTS parent chain does not reach the traversal root");
		provide(FAILURE);
	}

	if(relative_path_length == SIZE_MAX)
	{
		report("Relative path building; Path terminator overflow");
		provide(FAILURE);
	}

	run(m_resize(relative_path,relative_path_length + 1U));

	if(SUCCESS != status)
	{
		provide(status);
	}

	char *relative_path_data = m_data(char,relative_path);

	if(relative_path_data == NULL)
	{
		report("Relative path building; Destination buffer is unavailable");
		provide(FAILURE);
	}

	char *write_cursor = relative_path_data + relative_path_length;
	*write_cursor = '\0';

	for(const FTSENT *node = entry;
	        node != NULL && node->fts_level > FTS_ROOTLEVEL;
	        node = node->fts_parent)
	{
		const size_t component_length = (size_t)node->fts_namelen;

		write_cursor -= component_length;
		memcpy(write_cursor,node->fts_name,component_length);

		if(node->fts_parent != NULL && node->fts_parent->fts_level > FTS_ROOTLEVEL)
		{
			write_cursor--;
			*write_cursor = '/';
		}
	}

	if(write_cursor != relative_path_data)
	{
		report("Relative path building; Internal path assembly size mismatch");
		provide(FAILURE);
	}

	run(m_finalize_string(relative_path,relative_path_length));

	provide(status);
}
