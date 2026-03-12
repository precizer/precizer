#include "precizer.h"

/**
 * @brief Display include/ignore status for a directory path.
 *
 * Logs matches for --include/--ignore and triggers traversal banners
 * before the first visible log line. Respects --quiet-ignored.
 *
 */
void directory_show(
	const char       *relative_path,
	bool             *first_iteration,
	TraversalSummary *summary,
	const bool       ignore,
	const bool       include)
{
	if(ignore == true)
	{
		slog_show(EVERY|UNDECOR,true,first_iteration,summary,"ignore directory %s\n",relative_path);

	} else if(include == true){

		slog_show(EVERY|UNDECOR,true,first_iteration,summary,"include directory %s\n",relative_path);
	}
}
