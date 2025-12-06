#include "testitall.h"
#include <stdio.h>
#include <stdarg.h>

/**
 * @brief Append formatted text to a memory descriptor.
 * @param buffer Target descriptor that stores the growing string.
 * @param format printf-style format string.
 * @param ... Values consumed by the format string.
 */
void echo(
	memory     *buffer,
	const char *format,
	...)
{
	if(buffer == NULL || format == NULL)
	{
		return;
	}

	va_list args;
	va_start(args,format);

	va_list args_copy;
	va_copy(args_copy,args);
	int characters = vsnprintf(NULL,0,format,args_copy);
	va_end(args_copy);

	if(characters < 0)
	{
		va_end(args);
		return;
	}

	size_t former_length = buffer->length;
	size_t shift = former_length;
	size_t new_size = former_length + (size_t)characters + 1U;

	if(former_length > 0U)
	{
		shift -= 1U;

		if(new_size > 0U)
		{
			new_size -= 1U;
		}
	}

	if(SUCCESS == resize(buffer,new_size))
	{
		if(buffer->length > 0U)
		{
			char *buffer_data = data(char,buffer);

			if(buffer_data == NULL)
			{
				va_end(args);
				return;
			}

			const size_t writable_capacity = buffer->length - shift;
			int written = vsnprintf(
				buffer_data + shift,
				writable_capacity,
				format,
				args);

			if(written < 0 || (size_t)written >= writable_capacity)
			{
				report("Formatting failed while writing into buffer");
			} else {
				buffer_data[buffer->length - 1U] = '\0';
			}
		}
	} else {
		report("Memory allocation failed, requested size: %zu bytes",new_size);
	}

	va_end(args);
}
