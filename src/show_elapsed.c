#include "precizer.h"

/**
 * @brief Print traversal elapsed time and effective hashing throughput.
 *
 * Uses traversal_start_time/traversal_stop_time and total_hashed_bytes from
 * TraversalSummary.
 *
 * @param summary Traversal timing and hashing counters from file_list().
 */
void show_elapsed(const TraversalSummary *summary)
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

	// Compute elapsed time from traversal timing fields collected in file_list().
	long long int elapsed_ns = summary->traversal_stop_time - summary->traversal_start_time;

	if(elapsed_ns < 0LL)
	{
		elapsed_ns = 0LL;
	}

	const char *elapsed_human = form_date(elapsed_ns);

	if(elapsed_ns == 0LL)
	{
		slog(EVERY,"Elapsed time: %s, hashing rate: n/a\n",elapsed_human);
		return;
	}

	long double bytes_per_second = ((long double)summary->total_hashed_bytes * 1000000000.0L) / (long double)elapsed_ns;

	if(bytes_per_second < 1.0L)
	{
		slog(EVERY,"Elapsed time: %s, hashing rate: less than 1B/s\n",elapsed_human);
		return;
	}

	size_t speed_value = (size_t)bytes_per_second;

	slog(EVERY,"Elapsed time: %s, hashing rate: %s/s\n",elapsed_human,bkbmbgbtbpbeb(speed_value,MAJOR_VIEW));
}
