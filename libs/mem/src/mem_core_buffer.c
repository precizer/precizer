#include "mem.h"
#include "mem_internal.h"
#include <stdint.h>
#include <string.h>

/**
 * @brief Internal raw-buffer transfer core used by append and copy entry points
 *
 * This helper is the data-mode counterpart of @ref mem_core_string. It never
 * scans for terminators and always interprets @p source_buffer_size_bytes as an
 * exact byte count for raw payload transfer
 *
 * Transfer flags:
 * - `TRANSFER_APPEND`: append exact source bytes after the current
 *   destination payload
 * - `TRANSFER_REPLACE`: replace the current destination payload with the
 *   exact source byte range
 *
 * Self-aliasing is supported when @p source_buffer points inside the current
 * destination allocation. The source range must stay inside the current
 * logical destination payload, not merely inside allocator reserve. The helper
 * stores the source offset before @ref m_resize and rebuilds the source
 * pointer afterwards before copying with `memmove`
 *
 * Empty-source rules:
 * - append with `NULL + 0` or an explicit zero-size source is a no-op
 * - replace with `NULL + 0` or an explicit zero-size source clears the
 *   destination through @ref m_resize
 *
 * @param mode Binary mode flags. Raw-buffer cores only use transfer flags
 * @param destination Pointer to the destination descriptor in data mode
 * @param source_buffer_size_bytes Exact byte count to transfer
 * @param source_buffer Pointer to exact source bytes
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_core_buffer(
	const MEM_CORE_MODE mode,
	memory              *destination,
	const size_t        source_buffer_size_bytes,
	const void *const   source_buffer)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Transfer-mode subset extracted from the combined mode flags */
	const MEM_CORE_MODE transfer_mode = mode & TRANSFER_MASK;

	/* Number of destination elements represented by the exact source byte count */
	size_t transfer_elements = 0;

	/* Total destination element count after append or replace */
	size_t new_total_elements = 0;

	/* Byte offset where the transferred source bytes are written in destination */
	size_t offset_bytes = 0;

	/* Current logical destination payload size in bytes, used to validate self-aliased sources */
	size_t destination_bytes = 0;

	/* Offset of a self-aliased source from the beginning of destination data */
	size_t source_offset = 0;

	/* True when source_buffer points into the current destination allocation */
	bool source_is_inside_destination = false;

	if(destination == NULL)
	{
		report("Memory management; Destination must be non-NULL");
		provide(FAILURE);
	}

	if(destination->length > 0 && destination->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(destination->is_string == true)
	{
		report("Memory management; Destination must be in data mode, but it is a string");
		provide(FAILURE);
	}

	if(destination->single_element_size == 0)
	{
		report("Memory management; Destination element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(source_buffer == NULL)
	{
		if(source_buffer_size_bytes > 0)
		{
			report("Memory management; Source is NULL while size is %zu",source_buffer_size_bytes);
			provide(FAILURE);
		}

		if(transfer_mode & TRANSFER_APPEND)
		{
			provide(status);
		}

		run(m_resize(destination,0));
		provide(status);
	}

	if(source_buffer_size_bytes == 0)
	{
		if(transfer_mode & TRANSFER_APPEND)
		{
			provide(status);
		}

		run(m_resize(destination,0));
		provide(status);
	}

	if((source_buffer_size_bytes % destination->single_element_size) != 0)
	{
		report("Memory management; Size %zu is not divisible by element size %zu",source_buffer_size_bytes,destination->single_element_size);
		provide(FAILURE);
	}

	transfer_elements = source_buffer_size_bytes / destination->single_element_size;

	if(transfer_mode & TRANSFER_APPEND)
	{
		run(mem_guarded_add(destination->length,transfer_elements,&new_total_elements));

		if(CRITICAL & status)
		{
			report("Memory management; Concatenation would overflow element count");
			provide(FAILURE);
		}

		run(mem_guarded_byte_size(destination,destination->length,&offset_bytes));

		if(CRITICAL & status)
		{
			report("Memory management; Destination byte offset overflows");
			provide(status);
		}
	} else {
		new_total_elements = transfer_elements;
		offset_bytes = 0;
	}

	run(mem_guarded_byte_size(destination,destination->length,&destination_bytes));

	if((TRIUMPH & status) && destination->data != NULL && destination->actually_allocated_bytes > 0)
	{
		/* Use integer addresses so external source buffers can be compared without pointer-order UB */
		const uintptr_t destination_begin = (uintptr_t)destination->data;
		const uintptr_t source_begin = (uintptr_t)source_buffer;

		/* Both allocation and logical ends are derived from destination_begin and must not wrap */
		if(destination->actually_allocated_bytes > UINTPTR_MAX - destination_begin ||
		        destination_bytes > UINTPTR_MAX - destination_begin)
		{
			report("Memory management; Destination address range overflows");
			provide(FAILURE);
		}

		const uintptr_t allocation_end = destination_begin + destination->actually_allocated_bytes;
		const uintptr_t logical_end = destination_begin + destination_bytes;

		/* A source pointer inside the allocation is destination-owned, even if it points into spare capacity */
		if(source_begin >= destination_begin && source_begin < allocation_end)
		{
			source_is_inside_destination = true;
			source_offset = source_begin - destination_begin;

			/* Destination-owned sources may only expose the current logical payload */
			if(source_begin > logical_end || source_buffer_size_bytes > destination_bytes - source_offset)
			{
				report("Memory management; Source range exceeds destination logical bounds");
				provide(FAILURE);
			}
		}
	}

	/* m_resize may call realloc, so any source_buffer pointer into destination data becomes invalid here */
	run(m_resize(destination,new_total_elements));

	if(TRIUMPH & status)
	{
		unsigned char *destination_data_rewritable = (unsigned char *)destination->data;

		if(destination_data_rewritable == NULL)
		{
			report("Memory management; Destination data pointer is NULL after m_resize");
			status = FAILURE;
		} else {
			const unsigned char *source_data_view = (const unsigned char *)source_buffer;

			/* Rebase self-aliased sources onto the post-resize allocation using the saved offset */
			if(source_is_inside_destination == true)
			{
				source_data_view = destination_data_rewritable + source_offset;
			}

			/* memmove handles valid self-overlap between the rebuilt source slice and target location */
			memmove(destination_data_rewritable + offset_bytes,source_data_view,source_buffer_size_bytes);
		}
	}

	if(TRIUMPH & status)
	{
		destination->string_length = 0;
		destination->is_string = false;
	}

	provide(status);
}
