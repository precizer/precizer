#include "rational.h"
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

/**
 * @brief Safely prints an error message with system error description to stderr
 *
 * @param prefix Custom message to prepend to the error (must not be NULL)
 * @param file Source file name where the function was called (must not be NULL)
 * @param func Name of the calling function (must not be NULL)
 * @return void
 *
 * @note This function doesn't allocate memory dynamically and is safe to use
 *       in low-memory conditions
 *
 * @example
 *    serp("Failed to allocate memory"); <- NO EOL (\n)!
 */
void SERP(
	const char *prefix,
	const char *file,
	const char *func
){
	/* Buffer for converting errno to string, size 32 is sufficient for max int */
	char itoa_buf[32];

	fputs("ERROR: ",stderr);
	fputs(prefix,stderr);
	fputs(" [File: ",stderr);
	fputs(file,stderr);
	fputs(", Function: ",stderr);
	fputs(func,stderr);
	fputs("] Errno: (",stderr);
	fputs(itoa(errno,itoa_buf,10),stderr);
	fputs(") ",stderr);
	fputs(strerror(errno),stderr);
	fputs("\n",stderr);
}

/**
 * @brief Reports an error message with debug info to stderr
 *
 * This function generates a formatted error message with variable arguments and
 * writes it directly to stderr without using dynamic memory allocation.
 * Includes source location information and system error details.
 *
 * @param source_file Name of the source file where error occurred (must not be NULL)
 * @param func_name Name of the function where error occurred (must not be NULL)
 * @param line_num Line number where error occurred (must be positive)
 * @param format Printf-style format string for the error message (must not be NULL)
 * @param ... Variable arguments matching format specifiers
 * @return void
 *
 * @note Maximum message length is limited by MAX_NUMBER_CHARACTERS
 *
 * @example
 *    REPORT(__FILE__, __func__, __LINE__, "Failed to allocate %d bytes", size);
 */
__attribute__((format(gnu_printf,4,5)))
void REPORT(
	const char *source_file,
	const char *func_name,
	int        line_num,
	const char *format,
	...
){
	/* Buffers for message construction, zero-initialized for safety */
	char msg_buffer[MAX_NUMBER_CHARACTERS] = {0};    /* User message buffer */
	char final_buffer[MAX_NUMBER_CHARACTERS] = {0};  /* Complete error message */

	va_list args;
	va_start(args,format);
	vsnprintf(msg_buffer,sizeof(msg_buffer),format,args);
	va_end(args);

	/* Format complete error message with debug info and errno details */
	ssize_t len = (size_t)snprintf(final_buffer,
		sizeof(final_buffer),
		"ERROR: %s:%s:%d %s Errno: %s (errno: %d)\n",
		source_file,
		func_name,
		line_num,
		msg_buffer,
		strerror(errno),
		errno
	);

	/* Write message to stderr, handling potential buffer size limitations */
	if(len > 0)
	{
		size_t write_size = sizeof(final_buffer);

		if((size_t)len < sizeof(final_buffer))
		{
			write_size = (size_t)len;
		}

		/* Attempt to write error message */
		if(write(STDERR_FILENO,final_buffer,write_size) < 0)
		{
			/* Fallback error message if primary write fails */
			const char fallback_msg[] = "ERROR: Failed to write error message\n";

			if(write(STDERR_FILENO,fallback_msg,sizeof(fallback_msg) - 1) < 0)
			{
				return;
			}
		}
	}
}
