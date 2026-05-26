#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Shared internal data-descriptor core for append and copy entry points
 *
 * This helper is the descriptor-to-descriptor raw-data counterpart of
 * @ref mem_core_string. Both operands must be in data mode, and the transfer
 * is always performed byte for byte without any string interpretation
 *
 * Source and destination may use different element sizes. The only size-compatibility
 * rule is that the full source payload, measured in bytes, must be divisible by
 * the destination element size. That rule prevents the result from ending with
 * a partial destination element whose ownership would be unclear
 *
 * Self-aliasing is supported because the actual byte transfer is delegated to
 * @ref mem_core_buffer. If the source descriptor points into the destination
 * allocation, the raw-pointer core saves the source offset before resizing and
 * rebuilds the source pointer afterwards before calling `memmove(...)`
 *
 * Example:
 * @code
 * m_create(unsigned short,destination);
 * m_create(unsigned char,source);
 *
 * const unsigned char bytes[] = {'a','b','c','d'};
 *
 * if((TRIUMPH & mem_copy_buffer(source,sizeof(bytes),bytes)) == 0) { return FAILURE; }
 * if((TRIUMPH & mem_core_data(TRANSFER_REPLACE,destination,source)) == 0) { return FAILURE; }
 * // destination now stores two elements whose raw byte view is "abcd"
 * @endcode
 *
 * @param mode Binary mode flags. Data-descriptor cores only use transfer flags
 * @param destination Pointer to the destination descriptor in data mode
 * @param source Pointer to the source descriptor in data mode
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_core_data(
	const MEM_CORE_MODE mode,
	memory              *destination,
	const memory        *source)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Transfer-mode subset extracted from the combined mode flags */
	const MEM_CORE_MODE transfer_mode = mode & TRANSFER_MASK;

	/* Exact source payload size expressed in bytes */
	size_t source_bytes = 0;

	if(destination == NULL || source == NULL)
	{
		report("Memory management; Arguments must be non-NULL");
		provide(FAILURE);
	}

	if(destination->length > 0 && destination->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(source->length > 0 && source->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(destination->is_string == true)
	{
		report("Memory management; Destination must be in data mode, but it is a string");
		provide(FAILURE);
	}

	if(source->is_string == true)
	{
		report("Memory management; Source must be in data mode, but it is a string");
		provide(FAILURE);
	}

	if(destination->single_element_size == 0)
	{
		report("Memory management; Destination element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(source->single_element_size == 0)
	{
		report("Memory management; Source element size is zero (uninitialized)");
		provide(FAILURE);
	}

	run(mem_guarded_byte_size(source,source->length,&source_bytes));

	if(CRITICAL & status)
	{
		report("Memory management; Source byte count overflows");
		provide(status);
	}

	if((source_bytes % destination->single_element_size) != 0)
	{
		report(
			"Memory management; Source byte count %zu is not divisible by destination element size %zu",
			source_bytes,
			destination->single_element_size);
		provide(FAILURE);
	}

	provide(mem_core_buffer(transfer_mode,destination,source_bytes,source->data));
}
