#include "sute.h"
#include "testmocking.h"

/* Test-only sysconf() control for file_buffer_memory().
   file_buffer_memory.c routes only its own TESTITALL sysconf() calls through
   libtestmocking, so the rest of the test binary keeps native sysconf()
   behavior */

static Return test0005_1(void)
{
	INITTEST;

	testmocking_sysconf_return_next(1,-1,4096);

	size_t result = file_buffer_memory();
	testmocking_sysconf_disable();
	ASSERT(result == (size_t)(1024*1024));

	RETURN_STATUS;
}

static Return test0005_2(void)
{
	INITTEST;

	testmocking_sysconf_return_next(2,1000,-1);

	size_t result = file_buffer_memory();
	testmocking_sysconf_disable();
	ASSERT(result == (size_t)(1024*1024));

	RETURN_STATUS;
}

static Return test0005_3(void)
{
	INITTEST;

	testmocking_sysconf_return_next(2,0,4096);

	size_t result = file_buffer_memory();
	testmocking_sysconf_disable();
	ASSERT(result == (size_t)0);

	RETURN_STATUS;
}

static Return test0005_4(void)
{
	INITTEST;

	testmocking_sysconf_return_next(2,12345,0);

	size_t result = file_buffer_memory();
	testmocking_sysconf_disable();
	ASSERT(result == (size_t)0);

	RETURN_STATUS;
}

static Return test0005_5(void)
{
	INITTEST;

	testmocking_sysconf_return_next(2,12345,1);

	size_t result = file_buffer_memory();
	testmocking_sysconf_disable();
	ASSERT(result == (size_t)123);

	RETURN_STATUS;
}

static Return test0005_6(void)
{
	INITTEST;

	testmocking_sysconf_return_next(2,1000000,4096);

	size_t result = file_buffer_memory();
	testmocking_sysconf_disable();
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

	/* The sysconf() mock state is shared by the whole test binary.
	   Keep the suite closed by leaving the libtestmocking hook disabled
	   after all file_buffer_memory() unit checks have finished */
	testmocking_sysconf_disable();

	RETURN_STATUS;
}
