#include "sute.h"
#include <unistd.h>

// Test-only hook for sysconf() to control outputs in file_buffer_memory().
// Macro in file_buffer_memory.c (under TESTITALL) aliases sysconf() to
// test_sysconf() so only that code path is affected. No delegation to
// the real libc sysconf is needed here.
static long mock_avphys_pages = 0;   // _SC_AVPHYS_PAGES
static long mock_page_size   = 0;    // _SC_PAGESIZE

long testitall_sysconf(int name)
{
    switch(name)
    {
        case _SC_AVPHYS_PAGES:
            return mock_avphys_pages;
        case _SC_PAGESIZE:
            return mock_page_size;
        default:
            // Unknown names are not expected in these tests
            return -1;
    }
}

static Return pages_failure_returns_default(void)
{
    INITTEST;
    mock_avphys_pages = -1;   // Simulate sysconf failure for pages
    mock_page_size    = 4096; // Any value (won't be used)

    size_t result = file_buffer_memory();
    ASSERT(result == (size_t)(1024*1024));
    RETURN_STATUS;
}

static Return pagesize_failure_returns_default(void)
{
    INITTEST;
    mock_avphys_pages = 1000;
    mock_page_size    = -1;   // Simulate sysconf failure for page size

    size_t result = file_buffer_memory();
    ASSERT(result == (size_t)(1024*1024));
    RETURN_STATUS;
}

static Return zero_pages_results_in_zero(void)
{
    INITTEST;
    mock_avphys_pages = 0;
    mock_page_size    = 4096;

    size_t result = file_buffer_memory();
    ASSERT(result == (size_t)0);
    RETURN_STATUS;
}

static Return zero_pagesize_results_in_zero(void)
{
    INITTEST;
    mock_avphys_pages = 12345;
    mock_page_size    = 0;

    size_t result = file_buffer_memory();
    ASSERT(result == (size_t)0);
    RETURN_STATUS;
}

static Return tiny_values_integer_division(void)
{
    INITTEST;
    // 1% of 12,345 is 123 (integer division)
    mock_avphys_pages = 12345;
    mock_page_size    = 1;

    size_t result = file_buffer_memory();
    ASSERT(result == (size_t)123);
    RETURN_STATUS;
}

static Return normal_values_calculation(void)
{
    INITTEST;
    // 1% of (1,000,000 * 4096) = 40,960,000
    mock_avphys_pages = 1000000;
    mock_page_size    = 4096;

    size_t result = file_buffer_memory();
    ASSERT(result == (size_t)40960000);
    RETURN_STATUS;
}

/**
 * @brief Unit tests for file_buffer_memory().
 *
 * @details
 * - In TESTITALL builds, sysconf() inside file_buffer_memory() is macro-aliased
 *   to test_sysconf(), which supplies controlled values for _SC_AVPHYS_PAGES and
 *   _SC_PAGESIZE. Any other name returns -1 and is not used in these tests.
 * - Each subtest sets mock_avphys_pages and mock_page_size to drive scenarios:
 *   sysconf failures (expect default 1 MiB), zero inputs (expect 0), tiny product
 *   to observe integer-division truncation, and a normal 1% calculation for large
 *   inputs.
 * - ASSERT checks the returned size against the expected value; TEST aggregates
 *   subtests and reports status via the testitall harness.
 */
Return test0025(void)
{
    INITTEST;

    TEST(pages_failure_returns_default,"file_buffer_memory: returns default on pages failure…");
    TEST(pagesize_failure_returns_default,"file_buffer_memory: returns default on page size failure…");
    TEST(zero_pages_results_in_zero,"file_buffer_memory: 0 pages yields 0…");
    TEST(zero_pagesize_results_in_zero,"file_buffer_memory: 0 page size yields 0…");
    TEST(tiny_values_integer_division,"file_buffer_memory: integer division rounding down…");
    TEST(normal_values_calculation,"file_buffer_memory: normal case computation…");

    RETURN_STATUS;
}
