#include "mem.h"
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

/**
 * @brief Render a printf-style formatted string into a string descriptor
 *
 * Formats @p source_string together with the trailing variadic arguments and
 * writes the result into @p destination, which must already be a string
 * descriptor. The destination element width selects how the format string is
 * interpreted: `sizeof(char)` for `vsnprintf`, `sizeof(wchar_t)` for
 * `vswprintf`. Any other element width is rejected
 *
 * Any previous visible content in @p destination is replaced. The format
 * string must not point inside the destination allocation because self-aliasing
 * of @p source_string is not currently supported by this helper
 *
 * @param destination String descriptor of `char`- or `wchar_t`-width
 * @param source_string Format string in the matching element width, followed
 *        by the variadic arguments that supply the format conversions
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_formatted_string(
	memory *destination,
	const void *const source_string,
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

	/* A NULL format string is treated as empty runtime flow data, not as a
	   programmer frame mistake. Per the library's frame-and-flow contract,
	   stable program code must never raise errors on flow inputs, so this
	   branch exits quietly with SUCCESS instead of reporting a frame error */
	if(source_string == NULL)
	{
		provide(status);
	}

	const bool is_byte_width = (destination->single_element_size == sizeof(char));
	const bool is_wide_width = (destination->single_element_size == sizeof(wchar_t));

	if(is_byte_width == false && is_wide_width == false)
	{
		report("Memory management; Formatted mode supports only char and wchar_t element widths");
		provide(FAILURE);
	}

	va_list args;
	va_start(args,source_string);

	if(is_byte_width == true)
	{
		va_list probe;
		va_copy(probe,args);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
		const int rendered = vsnprintf(NULL,0,(const char *)source_string,probe);
#pragma GCC diagnostic pop

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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
				(void)vsnprintf((char *)destination->data,
					(size_t)rendered + 1,
					(const char *)source_string,
					args);
#pragma GCC diagnostic pop

				destination->string_length = (size_t)rendered;
			}
		}
	} else {
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
			rendered = vswprintf((wchar_t *)destination->data,
				guessed_length + 1,
				(const wchar_t *)source_string,
				probe);
#pragma GCC diagnostic pop

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
	}

	va_end(args);

	provide(status);
}
