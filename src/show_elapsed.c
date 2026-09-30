#include "precizer.h"

/**
 * @brief Print runtime and hashing metrics on separate lines
 *
 * Uses total_hashing_elapsed_ns and total_hashed_bytes from
 * TraversalSummary to report hashing time, hashed data, and hashing rate
 *
 * @param summary Traversal timing and hashing counters from file_list()
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

	long long int total_runtime_ns = cur_time_monotonic_ns() - config->app_start_time_ns;

	if(total_runtime_ns < 0LL)
	{
		total_runtime_ns = 0LL;
	}

	static const char *const rate_na = "n/a";
	static const char *const rate_less_than_1bps = "less than 1B/s";
	const bool perform_file_hashing = config->dry_run == false
	        || config->dry_run_with_checksums == true;

	// Sum of per-file hashing time collected in sha512sum() during file_list().
	long long int elapsed_ns = summary->total_hashing_elapsed_ns;

	if(elapsed_ns < 0LL)
	{
		elapsed_ns = 0LL;
	}

	char total_runtime_string[50] = {0};
	(void)form_date_r(total_runtime_ns,FULL_VIEW,total_runtime_string,sizeof(total_runtime_string));

	char elapsed_string[50] = {0};
	(void)form_date_r(elapsed_ns,FULL_VIEW,elapsed_string,sizeof(elapsed_string));

	char hashed_string[50] = {0};
	const char *hashed = rate_na;
	const char *rate = rate_na;
	const char *suffix = "";

	if(perform_file_hashing == true)
	{
		(void)bkbmbgbtbpbeb_r(summary->total_hashed_bytes,MAJOR_VIEW,hashed_string,sizeof(hashed_string));
		hashed = hashed_string;

		if(elapsed_ns > 0LL)
		{
			long double bytes_per_second = ((long double)summary->total_hashed_bytes * 1000000000.0L) / (long double)elapsed_ns;

			if(bytes_per_second < 1.0L)
			{
				rate = rate_less_than_1bps;
			} else {
				size_t speed_value = (size_t)bytes_per_second;
				rate = bkbmbgbtbpbeb(speed_value,MAJOR_VIEW);
				suffix = "/s";
			}
		}
	}

	slog(EVERY,"Total runtime: %s\n",total_runtime_string);
	slog(EVERY,"Hashing time: %s\n",elapsed_string);
	slog(EVERY,"Hashed data: %s\n",hashed);
	slog(EVERY,"Hashing rate: %s%s\n",rate,suffix);
}
