#include "precizer.h"

/**
 * @brief Print aggregated traversal totals from TraversalSummary.
 *
 * Emits a single totals line with allocated size and item counts
 * (directories, files, symlinks).
 *
 * @param summary Traversal counters produced by file_list().
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

	slog(EVERY,"Total allocated size: %s, total items: %zu, dirs: %zu, files: %zu, symlnks: %zu\n",
		bkbmbgbtbpbeb(summary->total_allocated_bytes,FULL_VIEW),
		total_items,
		summary->count_dirs,
		summary->count_files,
		summary->count_symlnks);
}
