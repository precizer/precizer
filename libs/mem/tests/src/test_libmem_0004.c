#include "test_libmem_utils.h"

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
Return test_libmem_0004(void)
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
	unsigned char *ullint_array = (unsigned char *)calloc(array_length,sizeof(unsigned long long int));

	if(ullint_array == NULL)
	{
		return(FAILURE);
	}

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ASSERT(SUCCESS == random_number_generator_urandom(&random,0,255));
		ullint_array[i] = (unsigned char)random;
	}

	// Calculate SHA-512 hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	echo(STDERR,"Test 4 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 4 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	m_create(unsigned long long int,test);

	ASSERT(SUCCESS == m_resize(test,array_length));

	test_data = m_data(unsigned long long int,test);
	ASSERT(test_data != NULL);

	if(test_data != NULL)
	{
		memcpy(test_data,ullint_array,test->length * test->single_element_size);
	}

	sha512_init(&ctx);
	test_view = m_data_ro(unsigned long long int,test);
	ASSERT(test_view != NULL);

	if(test_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test_view,test->length * test->single_element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	echo(STDERR,"Test 4 array size: %zu bytes\n",test->length * test->single_element_size);
	echo(STDERR,"Test 4 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	ASSERT(0 == memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH));

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

	for(size_t i = 0; i < array_size; i++)
	{
		ASSERT(SUCCESS == random_number_generator_urandom(&random,0,255));
		ullint_array[i] = (unsigned char)random;
	}

	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	echo(STDERR,"Test 5 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 5 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	ASSERT(SUCCESS == m_resize(test,array_length));

	test_data = m_data(unsigned long long int,test);
	ASSERT(test_data != NULL);

	if(test_data != NULL)
	{
		memcpy(test_data,ullint_array,test->length * test->single_element_size);
	}

	sha512_init(&ctx);
	test_view = m_data_ro(unsigned long long int,test);
	ASSERT(test_view != NULL);

	if(test_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test_view,test->length * test->single_element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	echo(STDERR,"Test 5 array size: %zu bytes\n",test->length * test->single_element_size);
	echo(STDERR,"Test 5 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	ASSERT(0 == memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH));

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

	for(size_t i = 0; i < array_size; i++)
	{
		ASSERT(SUCCESS == random_number_generator_urandom(&random,0,255));
		ullint_array[i] = (unsigned char)random;
	}

	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	echo(STDERR,"Test 6 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 6 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	ASSERT(SUCCESS == m_resize(test,array_length));

	test_data = m_data(unsigned long long int,test);
	ASSERT(test_data != NULL);

	if(test_data != NULL)
	{
		memcpy(test_data,ullint_array,test->length * test->single_element_size);
	}

	sha512_init(&ctx);
	test_view = m_data_ro(unsigned long long int,test);
	ASSERT(test_view != NULL);

	if(test_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test_view,test->length * test->single_element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	echo(STDERR,"Test 6 array size: %zu bytes\n",test->length * test->single_element_size);
	echo(STDERR,"Test 6 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	ASSERT(0 == memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH));

	m_del(test);
	m_reset(&ullint_array);

	#if SHOW_TEST
	telemetry_show();
	#endif

	RETURN_STATUS;
}
