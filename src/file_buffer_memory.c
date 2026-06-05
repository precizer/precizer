#include "precizer.h"

#ifdef TESTITALL
#include "testmocking.h"

/* In test builds route only this file's sysconf() calls through testmocking.
   This avoids overriding the libc symbol globally because sanitizers and
   runtime helpers may rely on the real sysconf() behavior */
#define sysconf(name) testmocking_sysconf(name)
#endif

/**
 * @brief Estimate the file-read buffer size
 *
 * Uses one percent of currently available physical memory when the platform
 * reports it. On platforms that only expose total physical pages, the estimate
 * is based on total memory instead. If page-count or page-size queries fail,
 * the function falls back to a 1 MB buffer
 *
 * @note The one-percent heuristic may still be too large for constrained
 *       embedded or IoT devices
 *
 * @return Selected buffer size in bytes
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
