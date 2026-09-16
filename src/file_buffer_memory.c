#include "precizer.h"

/* Application and test builds use the same fixed file-read buffer limit.
   Selecting the size does not require system-memory queries */

/**
 * @brief Return the fixed file-read buffer size
 *
 * Returns 64 KiB regardless of available physical memory. The caller allocates
 * the buffer once on the heap and reuses it for files in the traversal
 *
 * The 64 KiB limit is based on x86_64 file-reading and SHA-512 benchmarks.
 * It was among the fastest tested sizes and keeps buffer memory use low.
 * The best-performing size may vary with the workload and platform
 *
 * @note This function selects the size but does not allocate the buffer
 *
 * @return 65536 bytes
 */
size_t file_buffer_memory(void)
{
	// Reserve 64 KiB for each traversal's reusable read buffer
	const size_t buffer_size = 64U*1024U;

	// Free-page counts do not affect this limit

	// Apply the same limit on every supported platform

	/* The buffer length is expressed in bytes */

	// Include the selected allocation size in trace diagnostics
	slog(TRACE,"Bytes that can be allocated for the file buffer: %s\n",bkbmbgbtbpbeb(buffer_size,FULL_VIEW));

	return(buffer_size);
}
