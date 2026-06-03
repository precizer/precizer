#include "precizer.h"

#ifdef TESTITALL
#include "testmocking.h"

/* In test builds route only this file's sysconf() calls through testmocking.
   This avoids overriding the libc symbol globally because sanitizers and
   runtime helpers may rely on the real sysconf() behavior */
#define sysconf(name) testmocking_sysconf(name)
#endif

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

#if (defined(_SC_AVPHYS_PAGES) || defined(_SC_PHYS_PAGES)) && (defined(_SC_PAGESIZE) || defined(_SC_PAGE_SIZE))
	// Number of actually free pages
	long pages;

#ifdef _SC_AVPHYS_PAGES
	pages = sysconf(_SC_AVPHYS_PAGES);
#elif defined(_SC_PHYS_PAGES)
	// Fallback for platforms without _SC_AVPHYS_PAGES — use total pages
	pages = sysconf(_SC_PHYS_PAGES);
#endif

	if(pages == -1)
	{
		return(buffer_size);
	}

	/* Page size in bytes */
	#ifdef _SC_PAGESIZE
	long page_size = sysconf(_SC_PAGESIZE);
	#else
	long page_size = sysconf(_SC_PAGE_SIZE);
	#endif

	if(page_size == -1)
	{
		return(buffer_size);
	}

	// Only 1% of available RAM
	size_t avail_bytes = (size_t)pages * (size_t)page_size;

	size_t one_percent = avail_bytes / 100;

	slog(TRACE,"Bytes that can be allocated for the file buffer: %s\n",bkbmbgbtbpbeb(one_percent,FULL_VIEW));

	return(one_percent);
#else
	return(buffer_size);
#endif
}
