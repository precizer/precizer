#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Copy one data descriptor into another through @ref mem_core_data
 *
 * This wrapper is the public replace entry point for raw descriptor-to-descriptor
 * transfers. Both descriptors must be in data mode. The actual transfer is
 * delegated to @ref mem_core_data, so the payload is copied byte for byte,
 * self-aliasing is supported, and only the destination element size controls
 * whether the source payload size is acceptable
 *
 * @param destination Pointer to the destination descriptor in data mode
 * @param source Pointer to the source descriptor in data mode
 * @return `SUCCESS` on success; `FAILURE` otherwise
 */
Return mem_copy_data(
	memory       *destination,
	const memory *source)
{
	return(mem_core_data(TRANSFER_REPLACE,destination,source));
}
