#include "precizer.h"

/**
 * @brief Determines the maximum amount of memory that can be allocated for the buffer.
 *
 * This function estimates how much memory can be allocated for a buffer based on
 * available physical memory. It defaults to 1MB if system calls fail.
 *
 * @note The function assumes that only 1% of available RAM should be used for the buffer.
 *       It may not be suitable for embedded or IoT devices with constrained memory.
 *
 * @return The maximum buffer size in bytes. Defaults to 1MB if system information is unavailable.
 */
size_t file_buffer_memory(void)
{
	// Default value is 1MB buffer. Is it too big for embedded and IoT?
	const size_t buffer_size = 1024*1024;

	long pages = sysconf(_SC_AVPHYS_PAGES); // Number of actually free pages
	long page_size = sysconf(_SC_PAGESIZE); // Page size in bytes

	if(pages == -1 || page_size == -1)
	{
		return(buffer_size);
	}

	// Only 1% of available RAM
	size_t avail_bytes = (size_t)pages * (size_t)page_size;

	size_t one_percent = avail_bytes / 100;

	slog(TRACE,"Bytes that can be allocated for the file buffer: %s\n",bkbmbgbtbpbeb(one_percent));

	return(one_percent);
}
