#include "rational.h"

/**
 * @brief Current time in milliseconds
 * @return Returns long long int the number of milliseconds since the UNIX epoch
 */
long long int time_ms(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	long long mt = (long long)t.tv_sec * 1000 + t.tv_usec / 1000;
	return(mt);
}

/**
 * @brief Current time in nanoseconds
 * @return long long int number of nanoseconds, count starts at the Unix Epoch on January 1st, 1970 at UTC
 * @details Source: https://stackoverflow.com/questions/39439268/printing-time-since-epoch-in-nanoseconds
 */
long long int time_ns(void)
{
	long long int ns;
	time_t sec;
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	sec = spec.tv_sec;
	ns = spec.tv_nsec;
	return(((long long int)sec * 1000000000LL) + ns);
}

/**
 *
 * @brief Convert from UNIXtime seconds to ISO datetimes
 * @param seconds - if a parameter is passed in the form of milliseconds,
 * then exactly the specified time will be converted to ISO format.
 * If 0 is passed, the current time will be printed out in ISO format.
 *
 */
char *seconds_to_ISOdate(
	time_t seconds
)
{
	struct timeval curTime;
	gettimeofday(&curTime, NULL);

	// String to store converted time
	static char str_t[sizeof "2011-10-18 07:07:09"] = "";

	// Pointer to a structure with local time
	struct tm cur_time;

	// Convert system time to local time
	localtime_r(&seconds,&cur_time);

	// Create a string with date and time accurate to seconds
	strftime(str_t, sizeof(str_t), "%Y-%m-%d %H:%M:%S", &cur_time);

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
__attribute__((always_inline)) static inline Date asadate(
	const long long int nanoseconds
){
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
	Date date;
	memset(&date,0,sizeof(Date));

	date.years			= nanoseconds/ns_in_year;

	const long long int years_ns	= date.years * ns_in_year;
	date.months						= (nanoseconds - years_ns)/ns_in_month;

	const long long int months_ns	= date.months * ns_in_month;
	date.weeks						= (nanoseconds - years_ns - months_ns)/ns_in_week;

	const long long int weeks_ns	= date.weeks * ns_in_week;
	date.days						= (nanoseconds - years_ns - months_ns - weeks_ns)/ns_in_day;

	const long long int days_ns		= date.days * ns_in_day;
	date.hours						= (nanoseconds - years_ns - months_ns - weeks_ns - days_ns)/ns_in_hour;

	const long long int hours_ns	= date.hours * ns_in_hour;
	date.minutes					= (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns)/ns_in_minute;

	const long long int minutes_ns	= date.minutes * ns_in_minute;
	date.seconds					= (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns - minutes_ns)/ns_in_second;

	const long long int seconds_ns	= date.seconds * ns_in_second;
	date.milliseconds				= (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns - minutes_ns - seconds_ns)/ns_in_millisecond;

	const long long int milliseconds_ns	= date.milliseconds * ns_in_millisecond;
	date.microseconds				= (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns - minutes_ns - seconds_ns - milliseconds_ns)/ns_in_microsecond;

	const long long int microseconds_ns	= date.microseconds * ns_in_microsecond;
	date.nanoseconds				= (nanoseconds - years_ns - months_ns - weeks_ns - days_ns - hours_ns - minutes_ns - seconds_ns - milliseconds_ns - microseconds_ns);

	return(date);
}

/**
 *
 * The function for convert nanoseconds to a readable date. The function
 * generates a string if the structure element contains time data greater
 * than zero.
 *
 */
static void catdate(
	char * const result,
	const long long int number,
	const char * const suffix
)
{
	if(number > 0LL){
		// Temporary array
		char tmp[MAX_NUMBER_CHARACTERS];
		// Put a number into the temporary string array
		snprintf(tmp,sizeof(tmp),"%lld",number);
		// Copy the tmp line to the end of the result line
		strcat(result,tmp);
		// Add suffix
		strcat(result,suffix);
		// Add a space after the suffix
		strcat(result," ");
	}
}

/**
 *
 * Convert nanoseconds to human-readable date as a string
 *
 */
char *form_date(
	const long long int nanoseconds
){
	static char result[MAX_NUMBER_CHARACTERS];

	// Zero out a static memory area with a string array
	memset(result,0,strlen(result) * sizeof(char));

	// If the time passed as argument is less than one nanosecond
	if(nanoseconds == 0LL){
		strcat(result,"0ns");
		return(result);
	}

	Date date = asadate(nanoseconds);

	catdate(result,date.years,"y");
	catdate(result,date.months,"mon");
	catdate(result,date.weeks,"w");
	catdate(result,date.days,"d");
	catdate(result,date.hours,"h");
	catdate(result,date.minutes,"min");
	catdate(result,date.seconds,"s");
	catdate(result,date.milliseconds,"ms");

	#if 0
	// Print out microseconds and nanoseconds only
	// when larger units of time are not exist.
	if(nanoseconds < 1000LL*1000LL){
	#endif
		catdate(result,date.microseconds,"μs");
		catdate(result,date.nanoseconds,"ns");
	#if 0
	}
	#endif

	// Remove space at the end of a line
	result[strlen(result) - 1ULL]='\0';

	return(result);
}
#if 0
/// Test

/// To build
/// gcc -I../../logger/lib/ time.c

/// 339800645368118513 = ((365*24*60*60*1000*1000*1000)*10) + (((365*24*60*60*1000*1000*1000)/12)*9)+((7*24*60*60*1000*1000*1000)*1)+((24*60*60*1000*1000*1000)*2)+((60*60*1000*1000*1000)*3)+((60*1000*1000*1000)*4)+((1000*1000*1000)*5)+((1000*1000)*368)+((1000)*118)+513
/// Should be 10y 9mon 1w 2d 3h 4min 5s 368ms 118μs 513ns


int main(void){

	long long int ns = 339800645368118513LL;
	printf("%s\n",form_date(ns));

	printf("%s\n",form_date(273522528));

	return 0;
}
#endif
