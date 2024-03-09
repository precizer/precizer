#include "precizer.h"

/**
 *
 * @brief Logging to the screen with the source file name, line number
 * and name of the function that generated the message itself
 *
 */
__attribute__((format(printf, 5, 6))) //Without this we shall get warning: fmt string is not a string literal
void logger(
	const bool verbose_only,
	const char * const filename,
	size_t line,
	const char * const funcname,
	const char *fmt,
	...
)
{
	va_list args;

	if(config->silent == false)
	{

		if(config->verbose == true)
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

		if(verbose_only == false || config->verbose == true){
			va_start(args, fmt);
			vprintf(fmt, args);
			va_end(args);
		}

	}

	/*
	 * Finish
	 *
	 */
}
