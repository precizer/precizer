#include "precizer.h"

/**
 * @brief Print out current date and time in ISO format
 * @return A pointer to a char array that could be used
 * like this, for example: printf("Start at %s\n",logger_show_time());
 */
char *logger_show_time(void)
{
	struct timeval curTime;
	gettimeofday(&curTime, NULL);
	// Determine the number of milliseconds
	suseconds_t milliseconds = curTime.tv_usec / 1000;

	// The string to store converted time
	static char buffer[sizeof "2011-10-18 07:07:099"] = "";

	// The pointer to a structure with local time
	struct tm cur_time;

	// Read the system time into the corresponding variable
	time_t s_time = time(NULL);

	// Convert system time to the local one
	localtime_r(&s_time,&cur_time);

	// Format a string with date and time accurate to seconds
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &cur_time);

	//Add a string with milliseconds
	static char str_t[sizeof(buffer) + sizeof(".999") ] = "";
	sprintf(str_t, "%s:%03llu", buffer, (unsigned long long int)milliseconds);
	#if 0
	printf("current time: %s \n",str_t);
	#endif

	return(str_t);
}
