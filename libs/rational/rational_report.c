#include "rational.h"
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

/**
 * @brief Reports an error message to the specified file descriptor with debug info.
 *
 * This function generates a formatted error message with variable arguments and
 * writes it directly to the file descriptor without relying on dynamic memory
 * allocation. Includes source file, function name and line number information.
 *
 * @param source_file Name of the source file where error occurred
 * @param func_name Name of the function where error occurred
 * @param line_num Line number where error occurred
 * @param format Format string for the error message
 * @param ... Variable arguments to be formatted according to the format string
 */
void REPORT(
	const char *source_file,
	const char *func_name,
	int line_num,
	const char *format,
	...
){
	char msg_buffer[MAX_NUMBER_CHARACTERS]; ///< Buffer for storing the formatted user message
	char final_buffer[MAX_NUMBER_CHARACTERS]; ///< Buffer for storing the complete error message
	va_list args;

	// Format the user message with variable arguments
	va_start(args, format);
	vsnprintf(msg_buffer, sizeof(msg_buffer), format, args);
	va_end(args);

	// Format the complete error message with debug info
	size_t len = (size_t)snprintf(final_buffer,
				sizeof(final_buffer),
				"ERROR: %s:%s:%d %s Errno: %s (errno: %d)\n",
				source_file,
				func_name,
				line_num,
				msg_buffer,
				strerror(errno),
				errno
	);

	// Write the message to the file descriptor
	// The length is limited to the smaller of `len` or buffer size to avoid overflows
	if (len > 0)
	{
		size_t write_size = sizeof(final_buffer);
		if(len < sizeof(final_buffer))
		{
			write_size = len;
		}
		write(STDERR_FILENO, final_buffer, write_size);
	}
}

#if 0
/// Test

// Example of usage
int main(void)
{
	// Report an error with formatted message
	report("Memory reallocation failed with bytes %d", 10);

	// Report an error with multiple arguments
	report("Buffer overflow at position %d with value %s", 42, "overflow");

	return 0; // Success
}
#endif
