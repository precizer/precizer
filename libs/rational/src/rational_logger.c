#include "rational.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Global flag to manage output of all logging messages
// in an application and its default value
_Atomic LOGMODES rational_logger_mode = REGULAR;
_Atomic Return global_return_status = SUCCESS;

/**
 * @brief Converts LOGMODES bit flags to their string representation
 *
 * @details This function takes a combination of LOGMODES flags and converts them
 *          into a human-readable string representation where individual flags
 *          are separated by " | ". For example, (VERBOSE | SILENT) will be
 *          converted to "VERBOSE | SILENT"
 *
 * @param mode Combination of LOGMODES flags
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
char *rational_reconvert(LOGMODES mode)
{
	/* Static buffer to store the resulting string */
	static char buffer[MAX_CHARACTERS];
	buffer[0] = '\0';  /* Initialize buffer as empty string */

	/* Flag to track if we're adding the first item (for | separator) */
	int first = 1;

	/* Define mapping between flag values and their string representations
	 * The array is terminated with {0, NULL} for easy iteration
	 */
	static const struct {
		LOGMODES flag;     /* Flag value from LOGMODES constants */
		const char *name;  /* String representation of the flag */
	} mapping[] = {
		{REGULAR,"REGULAR"},
		{VERBOSE,"VERBOSE"},
		{TESTING,"TESTING"},
		{ERROR,"ERROR"},
		{SILENT,"SILENT"},
		{UNDECOR,"UNDECOR"},
		{REMEMBER,"REMEMBER"},
		{VISIBLE_IN_SILENT,"VISIBLE_IN_SILENT"},
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
 * @brief Format current date and time in ISO format
 * @param time_string Pointer to the destination buffer that receives the timestamp.
 * @param buffer_size Size of the destination buffer in bytes.
 * @return Return SUCCESS on success, FAILURE on error (buffer contents will be empty on failure).
 *
 */
static Return logger_show_time(
	char   *time_string,
	size_t buffer_size)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	struct timeval current_time;
	struct tm local_time;

	if(gettimeofday(&current_time,NULL) != 0)
	{
		time_string[0] = '\0';
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(localtime_r(&current_time.tv_sec,&local_time) == NULL)
		{
			time_string[0] = '\0';
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		const int milliseconds = (int)(current_time.tv_usec / 1000);

		if(snprintf(time_string,
			buffer_size,
			"%04d-%02d-%02d %02d:%02d:%02d:%03d",
			local_time.tm_year + 1900,
			local_time.tm_mon + 1,
			local_time.tm_mday,
			local_time.tm_hour,
			local_time.tm_min,
			local_time.tm_sec,
			milliseconds) < 0)
		{
			time_string[0] = '\0';
			status = FAILURE;
		}
	}

	return(status);
}

__attribute__((format(printf,3,0)))
static void logger_line_append_va(
	char       **line,
	int        *line_len,
	const char *fmt,
	va_list    args)
{
	va_list args_copy;
	va_copy(args_copy,args);
	const int needed = vsnprintf(NULL,0,fmt,args_copy);
	va_end(args_copy);

	if(needed < 0)
	{
		return;
	}

	const size_t new_len = (size_t)(*line_len) + (size_t)needed;
	char *tmp = realloc(*line,new_len + 1);

	if(tmp == NULL)
	{
		return;
	}

	*line = tmp;

	va_list args_copy2;
	va_copy(args_copy2,args);
	vsnprintf(*line + *line_len,(size_t)needed + 1,fmt,args_copy2);
	va_end(args_copy2);

	*line_len = (int)new_len;
}

__attribute__((format(printf,3,4)))
static void logger_line_append(
	char       **line,
	int        *line_len,
	const char *fmt,
	...)
{
	va_list args;
	va_start(args,fmt);
	logger_line_append_va(line,line_len,fmt,args);
	va_end(args);
}

__attribute__((format(printf,7,0)))
static void logger_line(
	char              **line,
	int               *line_len,
	const LOGMODES    level,
	const char *const filename,
	size_t            line_number,
	const char *const funcname,
	const char        *fmt,
	va_list           args)
{
	if(rational_logger_mode & SILENT)
	{
		if(level & VISIBLE_IN_SILENT)
		{
			logger_line_append_va(line,line_len,fmt,args);
		}

		return;
	}

	if(!(level & UNDECOR) && (level & TESTING) && (rational_logger_mode & TESTING))
	{
		// Print out the word "TESTING:"
		logger_line_append(line,line_len,"TESTING:");
	}

	if(!(level & UNDECOR) && (level & (VERBOSE|ERROR)) && (rational_logger_mode & VERBOSE))
	{
		char time_string[sizeof "2011-10-18 07:07:09:000"];
		(void)logger_show_time(time_string,sizeof(time_string));

		// Print out current time
		logger_line_append(line,line_len,"%s ",time_string);

		// Print out the source file name
		logger_line_append(line,line_len,"%s:",filename);

		// Print out line number in source file
		logger_line_append(line,line_len,"%03zu:",line_number);

		// Print out name of the function itself
		logger_line_append(line,line_len,"%s:",funcname);
	}

	if(!(level & UNDECOR) && (level & ERROR) && (rational_logger_mode & (REGULAR | ERROR)))
	{
		// Print out error prefix
		logger_line_append(line,line_len,"ERROR: ");

	} else if(!(level & UNDECOR) && (level & ERROR) && (rational_logger_mode & (TESTING | VERBOSE))){
		// Print out the word "ERROR:"
		logger_line_append(line,line_len,"ERROR:");
	}

	if(level & ERROR && rational_logger_mode & ERROR)
	{
		// Print out other arguments
		logger_line_append_va(line,line_len,fmt,args);

	} else if(level & (REGULAR|ERROR) && rational_logger_mode & REGULAR){
		// Print out other arguments
		logger_line_append_va(line,line_len,fmt,args);

	} else if(level & (VERBOSE|ERROR) && rational_logger_mode & VERBOSE){
		// Print out other arguments
		logger_line_append_va(line,line_len,fmt,args);

	} else if(level & (TESTING|ERROR) && rational_logger_mode & TESTING){
		// Print out other arguments
		logger_line_append_va(line,line_len,fmt,args);
	}
}

/**
 *
 * @brief Build and print a formatted log line with file, line, and function metadata
 *
 * @details When REMEMBER is set and the weak rational_remember() symbol is defined,
 *          the formatted line (without a trailing newline) and its length are
 *          passed to that callback.
 *
 */
__attribute__((format(printf,5,6))) // Without this we will get warning
void rational_logger(
	const LOGMODES    level,
	const char *const filename,
	size_t            line,
	const char *const funcname,
	const char        *fmt,
	...)
{

	char *logger_line_text = NULL;
	int line_len = 0;

	va_list args;
	va_start(args,fmt);
	logger_line(&logger_line_text,&line_len,level,filename,line,funcname,fmt,args);
	va_end(args);

	if((level & REMEMBER) && rational_remember && logger_line_text != NULL && line_len > 0)
	{
		rational_remember(logger_line_text,line_len);
	}

	if(logger_line_text != NULL)
	{
		fwrite(logger_line_text,sizeof(char),(size_t)line_len,stdout);
	}

	free(logger_line_text);
}

#ifdef TEST
/**
 * @file test_slog.c
 * @brief Complete test suite for log functionality
 */
int main(void)
{
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
	printf("%s\n",rational_convert(UNDECOR));
	printf("%s\n",rational_convert(EVERY|UNDECOR));
	printf("%s\n",rational_convert(ERROR|UNDECOR));
	printf("%s\n",rational_convert(VISIBLE_IN_SILENT));

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

	/*
	 * Test UNDECOR flag: suppress logger prefixes (TESTING:, time/file/line/func, ERROR:)
	 * The output between the '|' markers should contain only the message payload.
	 */

	rational_logger_mode = EVERY|ERROR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("37. Must print no prefixes:|"); slog(EVERY|UNDECOR,"true"); printf("|\n");
	printf("38. Must print no ERROR prefix:|"); slog(ERROR|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("39. Must print no time/file/line/func:|"); slog(VERBOSE|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("40. Must print no TESTING prefix:|"); slog(TESTING|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = REGULAR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("41. Must not print (VERBOSE not enabled):|"); slog(VERBOSE|UNDECOR,"but printed!"); printf("|\n");

	rational_logger_mode = SILENT;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("42. Must print in SILENT without prefixes:|"); slog(EVERY|VISIBLE_IN_SILENT,"true"); printf("|\n");
	printf("43. Must print no ERROR prefix in SILENT:|"); slog(ERROR|VISIBLE_IN_SILENT,"true"); printf("|\n");

	return 0;
}
#endif
