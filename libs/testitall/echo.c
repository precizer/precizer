#include "testitall.h"
#include <stdio.h>
#include <stdarg.h>

/**
 * Appends formatted text to a dynamically allocated buffer.
 * If the buffer is not large enough, it is automatically resized.
 * @param buffer Pointer to the buffer.
 * @param ... The first argument is the format string (like printf),
 *            followed by variable arguments for the format string.
 */
void echo(
	mem_char *buffer,
	const char *format,
	...
){
	char* result = NULL;
	va_list args;
	va_start(args, format);
	int characters = vasprintf(&result, format, args);
	va_end(args);

	if(characters < 0)
	{
		// Failure
		return;
	}

	size_t former_length = buffer->length;
	size_t shift = former_length;
	size_t new_size = former_length + (size_t)characters + 1;

	if(former_length > 0)
	{
		shift -= 1;

		if(new_size > 0)
		{
			new_size -=1;
		}
	}

	if(SUCCESS == realloc_char(buffer,new_size))
	{
		if(buffer->length > 0)
		{
			// Add new string to the buffer
			memcpy(buffer->mem + shift, result, (size_t)characters * sizeof(char));

			// Null termination of the string
			buffer->mem[buffer->length - 1] = '\0';
		}
	} else {
		report("Memory allocation failed, requested size: %zu bytes", new_size);
	}

	// Temporary buffer cleanup
	free(result);
}
