#include "rational.h"
#include <stdarg.h>

// Global flag to manage output of all logging messages
// in an application and its default value
char rational_logger_mode = REGULAR;

/**
 * @brief Converts LOGMODES bit flags to their string representation
 *
 * @details This function takes a combination of LOGMODES flags and converts them
 *          into a human-readable string representation where individual flags
 *          are separated by " | ". For example, (VERBOSE | SILENT) will be
 *          converted to "VERBOSE | SILENT"
 *
 * @param mode Integer containing the combination of LOGMODES flags
 * @return char* Pointer to static string containing flag names
 *
 * @note The function uses a static buffer which means:
 *       1. No memory allocation/deallocation is needed
 *       2. The buffer contents will be overwritten on next function call
 *       3. The function is not thread-safe
 *       4. The returned pointer should not be freed
 *
 * @warning Maximum resulting string length is limited to 256 characters
 */
char *rational_reconvert(int mode){
	/* Static buffer to store the resulting string */
	static char buffer[256];
	buffer[0] = '\0';  /* Initialize buffer as empty string */

	/* Flag to track if we're adding the first item (for | separator) */
	int first = 1;

	/* Define mapping between flag values and their string representations
	 * The array is terminated with {0, NULL} for easy iteration
	 */
	struct {
		int flag;          /* Flag value from LOGMODES enum */
		const char *name;  /* String representation of the flag */
	} mapping[] = {
		{REGULAR,"REGULAR"},
		{VERBOSE,"VERBOSE"},
		{TESTING,"TESTING"},
		{ERROR,"ERROR"},
		{SILENT,"SILENT"},
		{0,NULL}   /* Terminator element */
	};

	/* Iterate through all possible flags */
	for(int i = 0; mapping[i].name != NULL; i++)
	{
		/* Check if current flag is set in mode using bitwise AND */
		if(mode & mapping[i].flag)
		{
			/* Add separator before all elements except the first one */
			if(!first)
			{
				strcat(buffer," | ");
			}

			/* Add flag name to the result string */
			strcat(buffer,mapping[i].name);

			/* Clear first flag as we've added an element */
			first = 0;
		}
	}

	return buffer;
}

/**
 *
 * @brief Print out current date and time in ISO format
 * @return A pointer to a char array that could be used
 * like this, for example: printf("Start at %s\n",logger_show_time());
 *
 */
static char *logger_show_time(void){
	struct timeval curTime;
	gettimeofday(&curTime,NULL);
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
	strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",&cur_time);

	//Add a string with milliseconds
	static char str_t[sizeof(buffer) + sizeof(".000")] = "";
	sprintf(str_t,"%s:%03llu",buffer,(unsigned long long int)milliseconds);
	#if 0
	printf("current time: %s \n",str_t);
	#endif

	return(str_t);
}

/**
 *
 * @brief Logging to the screen with the source file name, line number
 * and name of the function that generated the message itself
 *
 */
__attribute__((format(printf,5,6)))   //Without this we shall get warning
void rational_logger(
	const char        level,
	const char *const filename,
	size_t            line,
	const char *const funcname,
	const char        *fmt,
	...
){
	if(rational_logger_mode & SILENT)
	{
		// Output nothing
		return;
	}

	if(level & TESTING && rational_logger_mode & TESTING)
	{
		// Print out the word "TESTING:"
		printf("TESTING:");
	}

	if(level & (VERBOSE|ERROR) && rational_logger_mode & VERBOSE)
	{
		// Print out current time
		printf("%s ",logger_show_time());

		// Print out the source file name
		printf("%s:",filename);

		// Print out line number in source file
		printf("%03zu:",line);

		// Print out name of the function itself
		printf("%s:",funcname);
	}

	if(level & ERROR && rational_logger_mode & (REGULAR | ERROR))
	{
		// Print out error prefix
		printf("ERROR: ");

	} else if(level & ERROR && rational_logger_mode & (TESTING | VERBOSE)){
		// Print out the word "ERROR:"
		printf("ERROR:");
	}

	if(level & ERROR && rational_logger_mode & ERROR)
	{
		// Print out other arguments
		va_list args;
		va_start(args,fmt);
		vprintf(fmt,args);
		va_end(args);

	} else if(level & (REGULAR|ERROR) && rational_logger_mode & REGULAR){
		// Print out other arguments
		va_list args;
		va_start(args,fmt);
		vprintf(fmt,args);
		va_end(args);

	} else if(level & (VERBOSE|ERROR) && rational_logger_mode & VERBOSE){
		// Print out other arguments
		va_list args;
		va_start(args,fmt);
		vprintf(fmt,args);
		va_end(args);

	} else if(level & (TESTING|ERROR) && rational_logger_mode & TESTING){
		// Print out other arguments
		va_list args;
		va_start(args,fmt);
		vprintf(fmt,args);
		va_end(args);
	}
}

#if 0
/**
 * @file test_slog.c
 * @brief Complete test suite for log functionality
 */
int main(void){
	printf("All available combinations:\n");
	printf("%s\n",rational_convert(REGULAR));
	printf("%s\n",rational_convert(VERBOSE));
	printf("%s\n",rational_convert(TESTING));
	printf("%s\n",rational_convert(SILENT));
	printf("%s\n",rational_convert(REGULAR|VERBOSE));
	printf("%s\n",rational_convert(REGULAR|TESTING));
	printf("%s\n",rational_convert(VERBOSE|TESTING));
	printf("%s\n",rational_convert(REGULAR|VERBOSE|TESTING));
	printf("%s\n",rational_convert(ERROR));

	/* Test REGULAR mode combinations */
	rational_logger_mode = REGULAR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("1.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("2. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("3. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("4.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test VERBOSE mode combinations */
	rational_logger_mode = VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("5. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("6.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("7. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("8.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test TESTING mode combinations */
	rational_logger_mode = TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("9.  Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("10. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("11.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("12.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test SILENT mode combinations */
	rational_logger_mode = SILENT;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("13. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("14. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("15. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("16. Won't print:");   slog(ERROR,"but printed!"); printf("\n");

	/* Test REGULAR|VERBOSE combinations */
	rational_logger_mode = REGULAR|VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("17.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("18.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("19. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("20.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test REGULAR|TESTING combinations */
	rational_logger_mode = REGULAR|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("21.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("22. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("23.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("24.  Must print:"); slog(ERROR,"true"); printf("\n");

	/* Test VERBOSE|TESTING combinations */
	rational_logger_mode = VERBOSE|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("25. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("26.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("27.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("28.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test REGULAR|VERBOSE|TESTING combinations */
	rational_logger_mode = REGULAR|VERBOSE|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("29. Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("30. Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("31. Must print:"); slog(TESTING,"true"); printf("\n");
	printf("32. Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test ERROR mode combinations */
	rational_logger_mode = ERROR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("33. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("34. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("35. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("36.  Must print:");   slog(ERROR,"true"); printf("\n");

	return 0;
}
#endif
