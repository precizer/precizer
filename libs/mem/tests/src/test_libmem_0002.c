#include "test_libmem_utils.h"

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
Return test_libmem_0002(void)
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
		ASSERT(SUCCESS == random_number_generator_urandom(&random,0,255));
		int_array[i] = (unsigned char)random;
	}

	// Calculate SHA-512 initial hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,int_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test 1 array size: %zu bytes, array_length=%zu, sizeof(int)=%zu bytes\n",array_size,array_length,sizeof(int));
	echo(STDERR,"Test 1 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure int
	m_create(int,test1);

	// Create an int memory
	ASSERT(SUCCESS == m_resize(test1,array_length,ZERO_NEW_MEMORY));

	// Test memory edges
	int *test1_data = m_data(int,test1);
	ASSERT(test1_data != NULL);

	if(test1_data != NULL)
	{
		memcpy(test1_data,int_array,test1->length * test1->single_element_size);
	}

	// Calculate hash of copied data
	sha512_init(&ctx);
	const int *test1_view = m_data_ro(int,test1);
	ASSERT(test1_view != NULL);

	if(test1_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test1_view,test1->length * test1->single_element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	echo(STDERR,"Test 1 Array size: %zu bytes\n",test1->length * test1->single_element_size);
	echo(STDERR,"Test 1 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	// Verify data integrity
	ASSERT(0 == memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH));

	// Cleanup int array
	m_del(test1);
	m_reset(&int_array);

	RETURN_STATUS;
}
