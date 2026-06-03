#include "testmocking.h"

#include <unistd.h>

static size_t testmocking_sysconf_remaining = 0;
static long testmocking_sysconf_avphys_pages = 0;
static long testmocking_sysconf_page_size = 0;

/**
 * @brief Arm sysconf mock to return controlled memory values for the next n calls
 *
 * @param[in] n Number of consecutive sysconf calls that must use controlled values
 * @param[in] avphys_pages Value returned for the available-pages selector
 * @param[in] page_size Value returned for the page-size selector
 */
void testmocking_sysconf_return_next(
	size_t n,
	long   avphys_pages,
	long   page_size)
{
	testmocking_sysconf_remaining = n;
	testmocking_sysconf_avphys_pages = avphys_pages;
	testmocking_sysconf_page_size = page_size;
}

/**
 * @brief Disable sysconf mock unconditionally
 */
void testmocking_sysconf_disable(void)
{
	testmocking_sysconf_remaining = 0;
	testmocking_sysconf_avphys_pages = 0;
	testmocking_sysconf_page_size = 0;
}

/**
 * @brief Test-only entry point for sysconf-compatible controlled values
 *
 * When the mock is armed each call decrements the remaining counter and
 * returns a configured value for memory-size selectors. When the counter
 * reaches zero the call is forwarded to libc
 *
 * @param[in] name sysconf selector passed by the caller
 * @return Controlled value while the mock is armed, otherwise libc sysconf result
 */
long testmocking_sysconf(int name)
{
	if(testmocking_sysconf_remaining > 0)
	{
		testmocking_sysconf_remaining--;

		switch(name)
		{
#ifdef _SC_AVPHYS_PAGES
			case _SC_AVPHYS_PAGES:
#elif defined(_SC_PHYS_PAGES)
			case _SC_PHYS_PAGES:
#endif
				return(testmocking_sysconf_avphys_pages);
#ifdef _SC_PAGESIZE
			case _SC_PAGESIZE:
#elif defined(_SC_PAGE_SIZE)
			case _SC_PAGE_SIZE:
#endif
				return(testmocking_sysconf_page_size);
			default:
				return(-1);
		}
	}

	return(sysconf(name));
}
