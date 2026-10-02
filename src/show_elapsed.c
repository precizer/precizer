#include "precizer.h"

/**
 * @brief Print the average processing rate for a measured interval
 *
 * No hashed data or a nonpositive interval produces n/a. Rates below one
 * byte per second retain that distinction, while larger rates are limited
 * to SIZE_MAX before conversion to the byte formatter's integer type
 *
 * @param label Name of the rate shown in the output
 * @param hashed_bytes Bytes actually hashed during the measured interval
 * @param elapsed_ns Measured interval in nanoseconds
 */
static void show_rate(
	const char          *label,
	const size_t        hashed_bytes,
	const long long int elapsed_ns)
{
	if(hashed_bytes == 0 || elapsed_ns <= 0LL)
	{
		slog(EVERY,"%s: n/a\n",label);
		return;
	}

	const long double bytes_per_second =
	        ((long double)hashed_bytes * 1000000000.0L) / (long double)elapsed_ns;

	if(bytes_per_second < 1.0L)
	{
		slog(EVERY,"%s: less than 1B/s\n",label);
		return;
	}

	// Bound the floating-point value before converting it to size_t
	size_t speed_value = SIZE_MAX;

	if(bytes_per_second < (long double)SIZE_MAX)
	{
		speed_value = (size_t)bytes_per_second;
	}

	slog(EVERY,"%s: %s/s\n",label,bkbmbgbtbpbeb(speed_value,MAJOR_VIEW));
}

/**
 * @brief Print runtime, scan, and hashing metrics on separate lines
 *
 * Both rates use bytes actually hashed in this pass. Scan time includes the
 * whole main traversal and its final transaction cleanup. Hashing time sums
 * the per-file read loops, including their periodic database checkpoints.
 * Scan and hashing durations show only the largest nonzero unit; rates use
 * the full measured intervals
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

	const bool perform_file_hashing = config->dry_run == false
	        || config->dry_run_with_checksums == true;

	// Sum of per-file hashing time collected in sha512sum() during file_list()
	long long int elapsed_ns = summary->total_hashing_elapsed_ns;

	if(elapsed_ns < 0LL)
	{
		elapsed_ns = 0LL;
	}

	long long int scan_elapsed_ns = summary->scan_elapsed_ns;

	if(scan_elapsed_ns < 0LL)
	{
		scan_elapsed_ns = 0LL;
	}

	char total_runtime_string[50] = {0};
	(void)form_date_r(total_runtime_ns,FULL_VIEW,total_runtime_string,sizeof(total_runtime_string));

	char scan_elapsed_string[50] = {0};
	(void)form_date_r(scan_elapsed_ns,MAJOR_VIEW,scan_elapsed_string,sizeof(scan_elapsed_string));

	char elapsed_string[50] = {0};
	(void)form_date_r(elapsed_ns,MAJOR_VIEW,elapsed_string,sizeof(elapsed_string));

	char hashed_string[50] = {0};
	const char *hashed = "n/a";
	size_t hashed_bytes = 0;

	if(perform_file_hashing == true)
	{
		hashed_bytes = summary->total_hashed_bytes;
		(void)bkbmbgbtbpbeb_r(hashed_bytes,MAJOR_VIEW,hashed_string,sizeof(hashed_string));
		hashed = hashed_string;
	}

	slog(EVERY,"Total runtime: %s\n",total_runtime_string);
	slog(EVERY,"Scan time: %s\n",scan_elapsed_string);
	show_rate("Scan rate",hashed_bytes,scan_elapsed_ns);
	slog(EVERY,"Hashing time: %s\n",elapsed_string);
	show_rate("Hashing rate",hashed_bytes,elapsed_ns);
	slog(EVERY,"Hashed data: %s\n",hashed);
}
