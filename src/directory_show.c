#include "precizer.h"

/**
 * @brief Show the visible include or ignore status for one directory path
 *
 * Logs directory matches for --include and --ignore and triggers traversal banners
 * before the first visible line. Respects --quiet-ignored through slog_show()
 *
 * @param[in] relative_path Relative path descriptor being reported
 * @param[in,out] first_iteration Banner sentinel for the first visible output line
 * @param[in,out] summary Traversal state used by slog_show()
 * @param[in] ignore True when the directory matched --ignore
 * @param[in] include True when the directory matched --include
 */
void directory_show(
	const memory     *relative_path,
	bool             *first_iteration,
	TraversalSummary *summary,
	const bool       ignore,
	const bool       include)
{
	const char *runtime_relative_path = m_text(relative_path);

	if(ignore == true)
	{
		slog_show(EVERY|UNDECOR,true,first_iteration,summary,"ignore directory %s\n",runtime_relative_path);

	} else if(include == true){

		slog_show(EVERY|UNDECOR,true,first_iteration,summary,"include directory %s\n",runtime_relative_path);
	}
}
