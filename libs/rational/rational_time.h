/**
 *
 * @file
 * @brief Time Logging structures and functions prototypes
 *
 */

// Functions of working with time
#include <time.h>
#include <sys/time.h>

/// Structure to record a date
typedef struct
{
	long long int nanoseconds;	// ns 1/1,000,000,000 of a second.
	long long int microseconds;	// μs 1/1,000,000 of a second.
	long long int milliseconds;	// ms 1/1000) of a second
	long long int seconds;
	long long int minutes;
	long long int hours;
	long long int days;
	long long int weeks;
	long long int months;
	long long int years;
} Date;

long long int cur_time_ns(void);

long long int cur_time_ms(void);

char *seconds_to_ISOdate(
	time_t seconds
);

char *form_date(
	const long long int
);
