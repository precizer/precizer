#include "precizer.h"

/**
 * @brief Print traversal size and item counts on separate lines
 *
 * Traversal size is the sum of allocated file storage collected during the
 * pass. Item counts cover directories, regular files, and symbolic links
 *
 * @param summary Traversal counters produced by file_list()
 */
void show_statistics(const TraversalSummary *summary)
{
	// Don't do anything
	if(config->compare == true)
	{
		return;
	}

	if(summary->stats_only_pass == false && summary->at_least_one_file_was_shown == false)
	{
		return;
	}

	size_t total_items = summary->count_dirs
	        + summary->count_files
	        + summary->count_symlnks;

	slog(EVERY,"Total traversal size: %s\n",bkbmbgbtbpbeb(summary->total_allocated_bytes,FULL_VIEW));
	slog(EVERY,"Total items: %zu\n",total_items);
	slog(EVERY,"Directories: %zu\n",summary->count_dirs);
	slog(EVERY,"Files: %zu\n",summary->count_files);
	slog(EVERY,"Symlinks: %zu\n",summary->count_symlnks);
}
