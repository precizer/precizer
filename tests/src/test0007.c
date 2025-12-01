#include "sute.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sha512.h"

#ifndef ullint
typedef unsigned long long int ullint;
#endif

#ifndef uchar
typedef unsigned char uchar;
#endif

#define CYCLES 10
#define SHOW_TEST 0 /* Change to 1 to print out debug details */

#if SHOW_TEST

/**
 * @brief Prints SHA-512 hash in hexadecimal format
 * @details Outputs each byte of the hash as a two-digit hexadecimal number
 *
 * @param hash Pointer to SHA-512 hash array to be printed
 *
 * @note Only compiled when SHOW_TEST is set to 1
 * @note Requires STDERR to be properly initialized
 */
static void print_hash(const unsigned char *hash)
{
	for(int i = 0; i < SHA512_DIGEST_LENGTH; i++)
	{
		echo(STDERR,"%02x",hash[i]);
	}
	echo(STDERR,"\n");
}
#endif

/**
 * @brief Tests copying of empty memory unsigned long long int structures
 * @details Verifies that copying an empty memory structure works correctly
 *         by creating two unsigned long long int memory structures and copying one to another
 *
 * @return Return enum indicating success or failure of the test
 * @retval SUCCESS if test passed
 * @retval FAILURE if test failed
 */
static Return test0007_1_libmem(void)
{
	INITTEST;

	// Allocate memory for the structure int
	create(unsigned long long int,test0_0);
	create(unsigned long long int,test0_1);

	// Create and copy of an unsigned long long int memory arrays
	ASSERT(SUCCESS == resize(test0_0,0));
	ASSERT(SUCCESS == copy(test0_1,test0_0));

	// Cleanup
	ASSERT(SUCCESS == del(test0_0));
	ASSERT(SUCCESS == del(test0_1));

	RETURN_STATUS;
}

/**
 * @brief Tests memory allocation and integrity for integer arrays
 * @details Creates a random array of integers, computes its SHA-512 hash,
 *         copies the data to a managed memory structure, and verifies
 *         data integrity by comparing hashes
 *
 * @return Return enum indicating success or failure of the test
 * @retval SUCCESS if memory allocation worked and hashes match
 * @retval FAILURE if memory allocation failed or hashes don't match
 */
static Return test0007_2_libmem(void)
{
	INITTEST;

	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];
	uint64_t random = 0;

	size_t array_length = 1792;
	size_t array_size = array_length * sizeof(int);
	unsigned char *int_array = (unsigned char *)calloc(array_length,sizeof(int));

	if(int_array == NULL)
	{
		report("Memory callocation failed with bytes %zu",array_length * sizeof(int));
		return(FAILURE);
	}

	// Fill the array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ASSERT(SUCCESS == random_number_generator(&random,0,255));
		int_array[i] = (unsigned char)random;
	}

	// Calculate SHA-512 initial hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,int_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test 1 array size: %zu bytes, array_length=%zu, sizeof(int)=%zu bytes\n",
		array_size,array_length,sizeof(int));
	echo(STDERR,"Test 1 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure int
	create(int,test1);

	// Create an int memory
	ASSERT(SUCCESS == resize(test1,array_length,ZERO_NEW_MEMORY));

	// Test memory edges
	int *test1_data = data(int,test1);
	ASSERT(test1_data != NULL);

	if(test1_data != NULL)
	{
		memcpy(test1_data,int_array,test1->length * test1->element_size);
	}

	// Calculate hash of copied data
	sha512_init(&ctx);
	const int *test1_view = cdata(int,test1);
	ASSERT(test1_view != NULL);

	if(test1_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test1_view,
			test1->length * test1->element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	echo(STDERR,"Test 1 Array size: %zu bytes\n",
		test1->length * test1->element_size);
	echo(STDERR,"Test 1 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	// Verify data integrity
	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 1 fail\n");
		status = FAILURE;
	}

	// Cleanup int array
	del(test1);
	reset(&int_array);

	RETURN_STATUS;
}

/**
 * @brief Tests memory allocation and integrity for character arrays
 * @details Creates a random array of characters, computes its SHA-512 hash,
 *         copies the data to a managed memory structure, and verifies
 *         data integrity by comparing hashes
 *
 * @return Return enum indicating success or failure of the test
 * @retval SUCCESS if memory allocation worked and hashes match
 * @retval FAILURE if memory allocation failed or hashes don't match
 */
static Return test0007_3_libmem(void)
{
	INITTEST;

	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];
	uint64_t random = 0;

	size_t array_length = 512;
	size_t array_size = array_length * sizeof(char);
	unsigned char *char_array = (unsigned char *)calloc(array_length,sizeof(char));
	ASSERT(char_array != NULL);

	if(char_array == NULL)
	{
		status = FAILURE;
		RETURN_STATUS;
	}

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ASSERT(SUCCESS == random_number_generator(&random,0,255));
		char_array[i] = (unsigned char)random;
	}

	// Calculate initial hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,char_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	echo(STDERR,"Test 2 array size: %zu bytes, array_length=%zu, sizeof(char)=%zu bytes\n",
		array_size,array_length,sizeof(char));
	echo(STDERR,"Test 2 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Test managed memory structure
	create(char,test2);

	// Create a char memory
	ASSERT(SUCCESS == resize(test2,array_length));

	// Test memory edges
	char *test2_data = data(char,test2);
	ASSERT(test2_data != NULL);

	if(test2_data != NULL)
	{
		memcpy(test2_data,char_array,test2->length * test2->element_size);
	}

	// Calculate hash of copied data
	sha512_init(&ctx);
	const char *test2_view = cdata(char,test2);
	ASSERT(test2_view != NULL);

	if(test2_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test2_view,
			test2->length * test2->element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	echo(STDERR,"Test 2 array size: %zu bytes\n",
		test2->length * test2->element_size);
	echo(STDERR,"Test 2 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	// Verify data integrity
	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 2 fail\n");
		status = FAILURE;
	}

	// Cleanup char array
	del(test2);
	reset(&char_array);

	RETURN_STATUS;
}

/**
 * @brief Comprehensive test of memory allocation and reallocation
 * @details Performs three sequential tests:
 *         1. Large allocation (4096 elements)
 *         2. Reduction to medium size (256 elements)
 *         3. Further reduction with forced memory shrinking (128 elements)
 *         Each test verifies data integrity using SHA-512 hashes
 *
 * @return Return enum indicating success or failure of the tests
 * @retval SUCCESS if all three tests pass
 * @retval FAILURE if any test fails or memory allocation fails
 */
static Return test0007_4_5_6_libmem(void)
{
	INITTEST;

	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];
	uint64_t random = 0;
	unsigned long long int *test_data = NULL;
	const unsigned long long int *test_view = NULL;

	// TEST 4: Large allocation
	size_t array_length = 4096;
	size_t array_size = array_length * sizeof(unsigned long long int);
	unsigned char *ullint_array = (unsigned char *)calloc(array_length,
		sizeof(unsigned long long int));

	if(ullint_array == NULL)
	{
		return(FAILURE);
	}

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ASSERT(SUCCESS == random_number_generator(&random,0,255));
		ullint_array[i] = (unsigned char)random;
	}

	// Calculate SHA-512 hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test 4 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 4 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure unsigned long long int
	create(unsigned long long int,test);

	// Create an unsigned long long int memory
	ASSERT(SUCCESS == resize(test,array_length));

	// Test memory edges
	test_data = data(unsigned long long int,test);
	ASSERT(test_data != NULL);

	if(test_data != NULL)
	{
		memcpy(test_data,ullint_array,test->length * test->element_size);
	}

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	test_view = cdata(unsigned long long int,test);
	ASSERT(test_view != NULL);

	if(test_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test_view,test->length * test->element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test 4 array size: %zu bytes\n",test->length * test->element_size);
	echo(STDERR,"Test 4 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 4 fail\n");
		status = FAILURE;
	}

	/**
	 * @brief TEST 5 verifies correct memory reallocation with size reduction
	 * @details Checks that:
	 * 1. Memory block can be correctly reallocated to a smaller size
	 * 2. Memory contents are preserved during reduction
	 * 3. Memory integrity is maintained after reduction
	 */
	array_length = 256;
	array_size = array_length * sizeof(unsigned long long int);
	ullint_array = (unsigned char *)realloc(ullint_array,array_size);

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ASSERT(SUCCESS == random_number_generator(&random,0,255));
		ullint_array[i] = (unsigned char)random;
	}

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test 5 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 5 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Create an unsigned long long int memory
	ASSERT(SUCCESS == resize(test,array_length));

	test_data = data(unsigned long long int,test);
	ASSERT(test_data != NULL);

	if(test_data != NULL)
	{
		memcpy(test_data,ullint_array,test->length * test->element_size);
	}

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	test_view = cdata(unsigned long long int,test);
	ASSERT(test_view != NULL);

	if(test_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test_view,test->length * test->element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test 5 array size: %zu bytes\n",test->length * test->element_size);
	echo(STDERR,"Test 5 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 5 fail\n");
		status = FAILURE;
	}

	/**
	 * @brief TEST 6 validates memory cleanup and deallocation
	 * @details Verifies:
	 * 1. Memory can be properly freed
	 * 2. All memory counters are correctly updated
	 * 3. Memory structure is reset to initial state
	 * 4. No memory leaks occur during cleanup
	 * 5. Telemetry accurately reflects the deallocation
	 */
	array_length = 128;
	array_size = array_length * sizeof(unsigned long long int);
	ullint_array = (unsigned char *)realloc(ullint_array,array_size);

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ASSERT(SUCCESS == random_number_generator(&random,0,255));
		ullint_array[i] = (unsigned char)random;
	}

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test 6 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 6 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Create an unsigned long long int memory
	ASSERT(SUCCESS == resize(test,array_length));

	test_data = data(unsigned long long int,test);
	ASSERT(test_data != NULL);

	if(test_data != NULL)
	{
		memcpy(test_data,ullint_array,test->length * test->element_size);
	}

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	test_view = cdata(unsigned long long int,test);
	ASSERT(test_view != NULL);

	if(test_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test_view,test->length * test->element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test 6 array size: %zu bytes\n",test->length * test->element_size);
	echo(STDERR,"Test 6 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 6 fail\n");
		status = FAILURE;
	}

	// free an empty unsigned long long int array
	del(test);
	reset(&ullint_array);

	#if SHOW_TEST
	telemetry_show();
	#endif

	RETURN_STATUS;
}

/**
 * @brief Multiple tests with unsigned long long int type and different array sizes
 *
 */
static Return test0007_7_libmem_multiple(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE ullint
	#include "test0007.cc"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Multiple tests with char type and different array sizes
 *
 */
static Return test0007_8_libmem_multiple(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE char
	#include "test0007.cc"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Multiple tests with int type and different array sizes
 *
 */
static Return test0007_9_libmem_multiple(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE int
	#include "test0007.cc"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Multiple tests with undigned char type and different array sizes
 *
 */
static Return test0007_10_libmem_multiple(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE uchar
	#include "test0007.cc"
	#undef TYPE

	RETURN_STATUS;
}


/**
 * @brief Main test runner for memory management tests
 * @details Executes a series of tests to verify memory management functionality:
 *         - Empty structure copying
 *         - Integer array allocation and verification
 *         - Character array allocation and verification
 *         - Multiple reallocation scenarios
 *
 * @return Return enum indicating overall test success or failure
 * @retval SUCCESS if all tests pass
 * @retval FAILURE if any test fails
 */
Return test0007(void)
{
	INITTEST;

	TEST(test0007_1_libmem,"Copy an array of 0 size…");
	TEST(test0007_2_libmem,"libmem Memory allocator test 1…");
	TEST(test0007_3_libmem,"libmem Memory allocator test 2…");
	TEST(test0007_4_5_6_libmem,"libmem Memory allocator tests 4,5,6…");
	TEST(test0007_7_libmem_multiple,"libmem generate multiple tests unsigned long long int type…");
	TEST(test0007_8_libmem_multiple,"libmem generate multiple tests char type…");
	TEST(test0007_9_libmem_multiple,"libmem generate multiple tests int type…");
	TEST(test0007_10_libmem_multiple,"libmem generate multiple tests unsigned char type…");

	RETURN_STATUS;
}
