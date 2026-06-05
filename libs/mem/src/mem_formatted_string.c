#include "mem.h"
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

/**
 * @brief Render a narrow printf-style formatted string into a string descriptor
 *
 * Formats @p source_string together with the trailing variadic arguments and
 * writes the result into @p destination, which must already be a string
 * descriptor whose elements have `sizeof(char)` width. The format string is
 * processed through `vsnprintf`
 *
 * Any previous visible content in @p destination is replaced. The format
 * string must not point inside the destination allocation because self-aliasing
 * of @p source_string is not currently supported by this helper
 *
 * @param destination String descriptor of `char` width
 * @param source_string Narrow format string, followed
 *        by the variadic arguments that supply the format conversions
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_formatted_string_char(
	memory     *destination,
	const char *source_string,
	...)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(destination == NULL)
	{
		report("Memory management; Destination must be non-NULL");
		provide(FAILURE);
	}

	if(destination->is_string == false)
	{
		report("Memory management; Destination must be a string descriptor");
		provide(FAILURE);
	}

	/* A NULL format string cannot describe a formatting operation. Reject it
	   before reading the variadic arguments or modifying the destination */
	if(source_string == NULL)
	{
		report("Memory management; Format string must be non-NULL");
		provide(FAILURE);
	}

	if(destination->single_element_size != sizeof(char))
	{
		report("Memory management; Narrow formatted destination requires char elements");
		provide(FAILURE);
	}

	va_list args;
	va_start(args,source_string);

	va_list probe;
	va_copy(probe,args);

	const int rendered = vsnprintf(NULL,0,source_string,probe);

	va_end(probe);

	/* A negative vsnprintf result is a runtime flow condition (encoding
	   issue in the format/args), not a programmer frame mistake. Exit
	   quietly with SUCCESS instead of reporting a frame error */
	if(rendered < 0)
	{
		va_end(args);
		provide(status);
	}

	if(TRIUMPH & status)
	{
		run(m_resize(destination,(size_t)rendered + 1));

		if(TRIUMPH & status)
		{
			(void)vsnprintf((char *)destination->data,
				(size_t)rendered + 1,
				source_string,
				args);

			destination->string_length = (size_t)rendered;
		}
	}

	va_end(args);

	provide(status);
}

/**
 * @brief Render a wide printf-style formatted string into a string descriptor
 *
 * Formats @p source_string together with the trailing variadic arguments and
 * writes the result into @p destination, which must already be a string
 * descriptor whose elements have `sizeof(wchar_t)` width. The format string
 * is processed through `vswprintf`
 *
 * Any previous visible content in @p destination is replaced. The format
 * string must not point inside the destination allocation because self-aliasing
 * of @p source_string is not currently supported by this helper
 *
 * @param destination String descriptor of `wchar_t` width
 * @param source_string Wide format string, followed by the variadic arguments
 *        that supply the format conversions
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_formatted_string_wchar(
	memory        *destination,
	const wchar_t *source_string,
	...)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(destination == NULL)
	{
		report("Memory management; Destination must be non-NULL");
		provide(FAILURE);
	}

	if(destination->is_string == false)
	{
		report("Memory management; Destination must be a string descriptor");
		provide(FAILURE);
	}

	/* A NULL format string cannot describe a formatting operation. Reject it
	   before reading the variadic arguments or modifying the destination */
	if(source_string == NULL)
	{
		report("Memory management; Format string must be non-NULL");
		provide(FAILURE);
	}

	if(destination->single_element_size != sizeof(wchar_t))
	{
		report("Memory management; Wide formatted destination requires wchar_t elements");
		provide(FAILURE);
	}

	va_list args;
	va_start(args,source_string);

	/* Wide-character path. Unlike vsnprintf, the standard does not
	   define a measure-only mode for vswprintf: passing a NULL buffer
	   or a zero size is not portable, and a buffer that is too small
	   simply returns a negative value with no hint about how much
	   room would have been needed. Without a measurement pass the
	   only portable way to find the right size is to render
	   speculatively and grow on failure.

	   Strategy: start with a reasonable initial capacity, try to
	   render in place, and if vswprintf reports the buffer was too
	   small, double the capacity and retry. The doubling loop is
	   bounded both by an iteration limit and by SIZE_MAX/2 so a
	   pathological format cannot drive the descriptor into an
	   unbounded growth spin. va_copy creates a fresh argument list
	   for each retry because vswprintf consumes the va_list on every
	   call. After a successful render the destination is resized
	   down to the exact length plus one terminator, so the leftover
	   slack from the last doubling is not retained */
	size_t guessed_length = 128;
	const int max_doublings = 20;
	int rendered = -1;

	for(int iteration = 0; iteration <= max_doublings; ++iteration)
	{
		run(m_resize(destination,guessed_length + 1));

		if(CRITICAL & status)
		{
			break;
		}

		va_list probe;
		va_copy(probe,args);

		rendered = vswprintf((wchar_t *)destination->data,
			guessed_length + 1,
			source_string,
			probe);

		va_end(probe);

		if(rendered >= 0)
		{
			break;
		}

		if(iteration == max_doublings || guessed_length > SIZE_MAX / 2)
		{
			report("Memory management; vswprintf failed after maximum growth iterations");
			status = FAILURE;
			break;
		}

		guessed_length *= 2;
	}

	if((TRIUMPH & status) && rendered >= 0)
	{
		/* Tell the descriptor about the rendered length BEFORE the trim
		   resize. String-mode shrink uses string_length to decide how
		   much of the visible payload to preserve, so leaving the old
		   value in place would make m_resize treat the just-written
		   text as overflow and overwrite its prefix with a terminator */
		destination->string_length = (size_t)rendered;

		run(m_resize(destination,(size_t)rendered + 1));
	}

	va_end(args);

	provide(status);
}
