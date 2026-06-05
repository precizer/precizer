#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Concatenate one data descriptor onto another through @ref mem_core_data
 *
 * This wrapper is the public append entry point for raw descriptor-to-descriptor
 * transfers. Both descriptors must be in data mode. The actual transfer is
 * delegated to @ref mem_core_data, so the payload is appended byte for byte,
 * self-aliasing is supported, and only the destination element size controls
 * whether the source payload size is acceptable
 *
 * @param destination Pointer to the destination descriptor in data mode
 * @param source Pointer to the source descriptor in data mode
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_concat_data(
	memory       *destination,
	const memory *source)
{
	return(mem_core_data(TRANSFER_APPEND,destination,source));
}
