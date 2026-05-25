#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Finalize a direct string-buffer write by caching the visible length
 *
 * Use this helper after low-level code has written string payload directly into
 * a descriptor's existing writable buffer. The descriptor must already be in
 * string mode. On return the descriptor is always zero-terminated at
 * @p written_length; @p flags only selects how the terminator is produced.
 * With @ref WRITE_TERMINATOR_ALWAYS the helper always writes the zero
 * terminator unconditionally. With @ref WRITE_TERMINATOR_IF_MISSING the
 * helper first inspects the element at @p written_length and writes a
 * terminator only when one is not already present. Either way
 * @ref memory::string_length is then updated to @p written_length
 *
 * Small example:
 * @code
 * m_create(char,title,MEMORY_STRING);
 * const char draft[] = "draft";
 *
 * if((TRIUMPH & m_resize(title,sizeof(draft))) == 0) { return FAILURE; }
 *
 * char *title_view = m_data(char,title);
 * if(title_view == NULL) { return FAILURE; }
 *
 * memcpy(title_view,draft,sizeof(draft));
 *
 * if((TRIUMPH & m_finalize_string(title,sizeof(draft) - 1U)) == 0) { return FAILURE; }
 * @endcode
 *
 * @param destination String descriptor whose direct write should become visible
 * @param written_length Visible string length measured in whole elements
 * @param flags Whether the helper should write the zero terminator unconditionally or only when it is missing
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_finalize_string(
	memory *destination,
	size_t written_length,
	TERMINATOR_WRITE_MODE flags)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Byte offset of the terminator slot within the data buffer
	   Computed by mem_guarded_byte_size as written_length * single_element_size */
	size_t terminator_offset = 0;

	/* Total bytes needed from the start of the buffer through the terminator
	   Computed as terminator_offset + single_element_size */
	size_t required_bytes = 0;

	if(destination == NULL)
	{
		report("Memory management; String write finalization destination must be non-NULL");
		status = FAILURE;
	}

	if((TRIUMPH & status) && destination->single_element_size == 0)
	{
		report("Memory management; String write finalization destination element size is zero");
		status = FAILURE;
	}

	if((TRIUMPH & status) && destination->length > 0 && destination->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		status = FAILURE;
	}

	if((TRIUMPH & status) &&
		destination->actually_allocated_bytes > 0 &&
		destination->data == NULL)
	{
		report("Memory management; Descriptor has reserved bytes with NULL data pointer during string write finalization");
		status = FAILURE;
	}

	if((TRIUMPH & status) && destination->is_string == false)
	{
		report("Memory management; String write finalization requires a string descriptor");
		status = FAILURE;
	}

	if((TRIUMPH & status) &&
		flags != WRITE_TERMINATOR_IF_MISSING &&
		flags != WRITE_TERMINATOR_ALWAYS)
	{
		report("Memory management; Unknown string write finalization flag");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		if(destination->length == 0)
		{
			report("Memory management; String write finalization requires reserved space for the visible string and terminator");
			status = FAILURE;
		} else if(written_length >= destination->length) {
			report("Memory management; String write finalization visible length exceeds descriptor capacity");
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		run(mem_guarded_byte_size(destination,written_length,&terminator_offset));
		run(mem_guarded_add(terminator_offset,destination->single_element_size,&required_bytes));

		if((TRIUMPH & status) && required_bytes > destination->actually_allocated_bytes)
		{
			report("Memory management; String write finalization exceeds reserved capacity");
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		if(flags == WRITE_TERMINATOR_ALWAYS)
		{
			run(mem_write_zero_terminator(destination,written_length));
		} else {
			/* Read-only view of the raw buffer used to inspect
			   whether a zero terminator already exists at the expected slot */
			const unsigned char *destination_data_view = (const unsigned char *)destination->data;

			if(mem_is_zero_element(destination_data_view + terminator_offset,destination->single_element_size) == false)
			{
				run(mem_write_zero_terminator(destination,written_length));

				if(TRIUMPH & status)
				{
					telemetry_finalize_string_terminator_written_when_missing();
				}
			} else {
				telemetry_finalize_string_terminator_already_present();
			}
		}
	}

	if(TRIUMPH & status)
	{
		destination->string_length = written_length;
	}

	provide(status);
}
