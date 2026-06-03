#include "sute.h"
#include <unistd.h>

/* Test-only hook for sysconf() to control outputs in file_buffer_memory().
   The macro in file_buffer_memory.c aliases sysconf() to testitall_sysconf()
   in TESTITALL builds, so only that code path receives the controlled values */
static long mock_avphys_pages = 0;
static long mock_page_size = 0;

/**
 * @brief Return controlled sysconf() values for file_buffer_memory() tests
 *
 * @param[in] name sysconf() selector
 * @return Controlled sysconf() value or -1 for unexpected selectors
 */
long testitall_sysconf(int name)
{
	switch(name)
	{
#ifdef _SC_AVPHYS_PAGES
		case _SC_AVPHYS_PAGES:
#elif defined(_SC_PHYS_PAGES)
		case _SC_PHYS_PAGES:
#endif
			return mock_avphys_pages;
#ifdef _SC_PAGESIZE
		case _SC_PAGESIZE:
#elif defined(_SC_PAGE_SIZE)
		case _SC_PAGE_SIZE:
#endif
			return mock_page_size;
		default:
			return -1;
	}
}

static Return test0005_1(void)
{
	INITTEST;

	mock_avphys_pages = -1;
	mock_page_size = 4096;

	size_t result = file_buffer_memory();
	ASSERT(result == (size_t)(1024*1024));

	RETURN_STATUS;
}

static Return test0005_2(void)
{
	INITTEST;

	mock_avphys_pages = 1000;
	mock_page_size = -1;

	size_t result = file_buffer_memory();
	ASSERT(result == (size_t)(1024*1024));

	RETURN_STATUS;
}

static Return test0005_3(void)
{
	INITTEST;

	mock_avphys_pages = 0;
	mock_page_size = 4096;

	size_t result = file_buffer_memory();
	ASSERT(result == (size_t)0);

	RETURN_STATUS;
}

static Return test0005_4(void)
{
	INITTEST;

	mock_avphys_pages = 12345;
	mock_page_size = 0;

	size_t result = file_buffer_memory();
	ASSERT(result == (size_t)0);

	RETURN_STATUS;
}

static Return test0005_5(void)
{
	INITTEST;

	mock_avphys_pages = 12345;
	mock_page_size = 1;

	size_t result = file_buffer_memory();
	ASSERT(result == (size_t)123);

	RETURN_STATUS;
}

static Return test0005_6(void)
{
	INITTEST;

	mock_avphys_pages = 1000000;
	mock_page_size = 4096;

	size_t result = file_buffer_memory();
	ASSERT(result == (size_t)40960000);

	RETURN_STATUS;
}

/**
 * @brief Run unit tests for file_buffer_memory()
 *
 * @details
 * The tests use a test-only sysconf() hook to make failure, zero-value,
 * rounding, and normal calculation paths deterministic
 */
Return test0005(void)
{
	INITTEST;

	TEST(test0005_1,"file_buffer_memory(): returns default on pages failure");
	TEST(test0005_2,"file_buffer_memory(): returns default on page size failure");
	TEST(test0005_3,"file_buffer_memory(): 0 pages yields 0");
	TEST(test0005_4,"file_buffer_memory(): 0 page size yields 0");
	TEST(test0005_5,"file_buffer_memory(): integer division rounding down");
	TEST(test0005_6,"file_buffer_memory(): normal case computation");

	/* These variables are file-scope state for the sysconf() test hook.
	   The hook is shared by every file_buffer_memory() call in this test
	   binary, so the values left here affect all later uses of this mock */
	mock_avphys_pages = 0;
	mock_page_size = 0;

	RETURN_STATUS;
}
