#include "rational.h"

/**
 * @brief Current time in milliseconds
 * @return Returns long long int the number of milliseconds since the UNIX epoch
 */
long long int cur_time_ms(void)
{
	struct timeval t;
	gettimeofday(&t,NULL);
	long long mt = (long long)t.tv_sec * 1000 + t.tv_usec / 1000;
	return(mt);
}

/**
 * @brief Current time in nanoseconds
 * @return long long int number of nanoseconds, count starts at the Unix Epoch on January 1st, 1970 at UTC
 * @details Source: https://stackoverflow.com/questions/39439268/printing-time-since-epoch-in-nanoseconds
 */
long long int cur_time_ns(void)
{
	long long int ns;
	time_t sec;
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME,&spec);
	sec = spec.tv_sec;
	ns = spec.tv_nsec;
	return(((long long int)sec * 1000000000LL) + ns);
}

/**
 * @brief Current monotonic time in nanoseconds
 * @return long long int number of nanoseconds from a monotonic clock source
 * @details Counter starts at an unspecified point and is intended for interval measurement.
 * Unlike CLOCK_REALTIME, this source is not affected by wall-clock adjustments.
 */
#if defined(CLOCK_MONOTONIC) && (!defined(_POSIX_MONOTONIC_CLOCK) || (_POSIX_MONOTONIC_CLOCK >= 0))
long long int cur_time_monotonic_ns(void)
{
	long long int ns;
	time_t sec;
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC,&spec);
	sec = spec.tv_sec;
	ns = spec.tv_nsec;
	return(((long long int)sec * 1000000000LL) + ns);
}
#endif

/**
 *
 * @brief Convert from UNIXtime seconds to ISO datetimes
 * @param seconds - if a parameter is passed in the form of milliseconds,
 * then exactly the specified time will be converted to ISO format.
 * If 0 is passed, the current time will be printed out in ISO format.
 *
 */
char *seconds_to_ISOdate(time_t seconds)
{
	struct timeval curTime;
	gettimeofday(&curTime,NULL);

	// String to store converted time
	static char str_t[sizeof "2011-10-18 07:07:09"] = "";
	str_t[0] = '\0';  /* Initialize buffer as empty string */

	// Pointer to a structure with local time
	struct tm cur_time;

	// Convert system time to local time
	localtime_r(&seconds,&cur_time);

	// Create a string with date and time accurate to seconds
	strftime(str_t,sizeof(str_t),"%Y-%m-%d %H:%M:%S",&cur_time);

	#if 0
	printf("current time: %s \n",str_t);
	#endif

	return(str_t);
}

/**
 *
 * @brief "As a date", Convert nanoseconds to date format
 *
 */
__attribute__((always_inline)) static inline Date asadate(const long long int nanoseconds)
{
	/// Number of nanoseconds in a year
	/// 365*24*60*60*1000*1000*1000
	const long long int ns_in_year = 31536000000000000LL;

	/// Number of nanoseconds in a month
	/// ns_in_year/12
	const long long int ns_in_month = 2628000000000000LL;

	/// Number of nanoseconds in a week
	/// 7*24*60*60*1000*1000*1000
	const long long int ns_in_week = 604800000000000LL;

	/// Number of nanoseconds in a day
	/// 24*60*60*1000*1000*1000
	const long long int ns_in_day = 86400000000000LL;

	/// Number of nanoseconds in an hour
	/// 60*60*1000*1000*1000
	const long long int ns_in_hour = 3600000000000LL;

	/// Number of nanoseconds in a minute
	/// 60*1000*1000*1000
	const long long int ns_in_minute = 60000000000LL;

	/// Number of nanoseconds in a second
	/// 1000*1000*1000
	const long long int ns_in_second = 1000000000LL;

	/// Number of nanoseconds in a millisecond
	/// 1000*1000
	const long long int ns_in_millisecond = 1000000LL;

	/// Number of nanoseconds in a microsecond
	/// 1000
	const long long int ns_in_microsecond = 1000LL;

	// Initializing the structure that will be returned from the function
	Date date = {0};

	date.years = nanoseconds/ns_in_year;

	const long long int years_ns = date.years * ns_in_year;
	date.months = (nanoseconds - years_ns)/ns_in_month;

	const long long int months_ns = date.months * ns_in_month;
	date.weeks = (nanoseconds - years_ns - months_ns)/ns_in_week;

	const long long int weeks_ns = date.weeks * ns_in_week;
	date.days = (nanoseconds - years_ns - months_ns - weeks_ns)/ns_in_day;

	const long long int days_ns = date.days * ns_in_day;
	date.hours = (nanoseconds - years_ns - months_ns - weeks_ns - days_ns)/ns_in_hour;

	const long long int hours_ns = date.hours * ns_in_hour;
	date.minutes = (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns)/ns_in_minute;

	const long long int minutes_ns = date.minutes * ns_in_minute;
	date.seconds = (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns - minutes_ns)/ns_in_second;

	const long long int seconds_ns = date.seconds * ns_in_second;
	date.milliseconds = (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns - minutes_ns - seconds_ns)/ns_in_millisecond;

	const long long int milliseconds_ns = date.milliseconds * ns_in_millisecond;
	date.microseconds = (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns - minutes_ns - seconds_ns - milliseconds_ns)/ns_in_microsecond;

	const long long int microseconds_ns = date.microseconds * ns_in_microsecond;
	date.nanoseconds = (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns - minutes_ns - seconds_ns - milliseconds_ns - microseconds_ns);

	return(date);
}

/**
 *
 * The function for convert nanoseconds to a readable date. The function
 * generates a string if the structure element contains time data greater
 * than zero.
 *
 */
static void catdate_r(
	char *const         result,
	const size_t        result_size,
	size_t *const       used_len,
	const long long int number,
	const char *const   suffix)
{
	if(number <= 0LL || *used_len >= result_size)
	{
		return;
	}

	const int written = snprintf(result + *used_len,result_size - *used_len,"%lld%s ",number,suffix);

	if(written < 0)
	{
		return;
	}

	const size_t write_size = (size_t)written;

	if(write_size >= result_size - *used_len)
	{
		*used_len = result_size - 1ULL;
		result[result_size - 1ULL] = '\0';
	} else {
		*used_len += write_size;
	}
}

/**
 *
 * Convert nanoseconds to human-readable date as a string
 *
 */
char *form_date_r(
	const long long int nanoseconds,
	const ByteFormat    format,
	char                *buffer,
	const size_t        buffer_size)
{
	if(buffer == NULL || buffer_size == 0ULL)
	{
		return(NULL);
	}

	buffer[0] = '\0';  /* Initialize buffer as empty string */
	size_t used_len = 0ULL;

	// If the time passed as argument is less than one nanosecond
	if(nanoseconds == 0LL)
	{
		(void)snprintf(buffer,buffer_size,"0ns");
		return(buffer);
	}

	Date date = asadate(nanoseconds);

	if(format == MAJOR_VIEW)
	{
		if(date.years > 0LL)
		{
			catdate_r(buffer,buffer_size,&used_len,date.years,"y");
		} else if(date.months > 0LL){
			catdate_r(buffer,buffer_size,&used_len,date.months,"mon");
		} else if(date.weeks > 0LL){
			catdate_r(buffer,buffer_size,&used_len,date.weeks,"w");
		} else if(date.days > 0LL){
			catdate_r(buffer,buffer_size,&used_len,date.days,"d");
		} else if(date.hours > 0LL){
			catdate_r(buffer,buffer_size,&used_len,date.hours,"h");
		} else if(date.minutes > 0LL){
			catdate_r(buffer,buffer_size,&used_len,date.minutes,"min");
		} else if(date.seconds > 0LL){
			catdate_r(buffer,buffer_size,&used_len,date.seconds,"s");
		} else if(date.milliseconds > 0LL){
			catdate_r(buffer,buffer_size,&used_len,date.milliseconds,"ms");
		} else if(date.microseconds > 0LL){
			catdate_r(buffer,buffer_size,&used_len,date.microseconds,"μs");
		} else {
			catdate_r(buffer,buffer_size,&used_len,date.nanoseconds,"ns");
		}
	} else {
		catdate_r(buffer,buffer_size,&used_len,date.years,"y");
		catdate_r(buffer,buffer_size,&used_len,date.months,"mon");
		catdate_r(buffer,buffer_size,&used_len,date.weeks,"w");
		catdate_r(buffer,buffer_size,&used_len,date.days,"d");
		catdate_r(buffer,buffer_size,&used_len,date.hours,"h");
		catdate_r(buffer,buffer_size,&used_len,date.minutes,"min");
		catdate_r(buffer,buffer_size,&used_len,date.seconds,"s");
		catdate_r(buffer,buffer_size,&used_len,date.milliseconds,"ms");
		catdate_r(buffer,buffer_size,&used_len,date.microseconds,"μs");
		catdate_r(buffer,buffer_size,&used_len,date.nanoseconds,"ns");
	}

	// Remove trailing space at the end of the line.
	if(used_len > 0ULL && buffer[used_len - 1ULL] == ' ')
	{
		buffer[used_len - 1ULL] = '\0';
	}

	return(buffer);
}

/**
 *
 * Convert nanoseconds to human-readable date as a string
 *
 */
char *form_date(
	const long long int nanoseconds,
	const ByteFormat    format)
{
	// Zero out a static memory area with a string array
	static char result[MAX_CHARACTERS];

	return(form_date_r(nanoseconds,format,result,sizeof(result)));
}
#if 0
/// Test

/// To build
/// gcc -I../../logger/lib/ time.c

/// 339800645368118513 = ((365*24*60*60*1000*1000*1000)*10) + (((365*24*60*60*1000*1000*1000)/12)*9)+((7*24*60*60*1000*1000*1000)*1)+((24*60*60*1000*1000*1000)*2)+((60*60*1000*1000*1000)*3)+((60*1000*1000*1000)*4)+((1000*1000*1000)*5)+((1000*1000)*368)+((1000)*118)+513
/// Should be 10y 9mon 1w 2d 3h 4min 5s 368ms 118μs 513ns

int main(void)
{
	long long int ns = 339800645368118513LL;
	printf("%s\n",form_date(ns,FULL_VIEW));

	printf("%s\n",form_date(273522528,FULL_VIEW));

	return 0;
}
#endif
