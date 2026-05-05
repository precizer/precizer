#include "test_libmem_utils.h"

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
Return test_libmem_0003(void)
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
		ASSERT(SUCCESS == random_number_generator_urandom(&random,0,255));
		char_array[i] = (unsigned char)random;
	}

	// Calculate initial hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,char_array,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	echo(STDERR,"Test 2 array size: %zu bytes, array_length=%zu, sizeof(char)=%zu bytes\n",array_size,array_length,sizeof(char));
	echo(STDERR,"Test 2 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Test managed memory structure
	m_create(char,test2);

	// Create a char memory
	ASSERT(SUCCESS == m_resize(test2,array_length));

	// Test memory edges
	char *test2_data = m_data(char,test2);
	ASSERT(test2_data != NULL);

	if(test2_data != NULL)
	{
		memcpy(test2_data,char_array,test2->length * test2->single_element_size);
	}

	// Calculate hash of copied data
	sha512_init(&ctx);
	const char *test2_view = m_data_ro(char,test2);
	ASSERT(test2_view != NULL);

	if(test2_view != NULL)
	{
		sha512_update(&ctx,(const unsigned char *)test2_view,test2->length * test2->single_element_size);
	}
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	echo(STDERR,"Test 2 array size: %zu bytes\n",test2->length * test2->single_element_size);
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
	m_del(test2);
	m_reset(&char_array);

	RETURN_STATUS;
}
