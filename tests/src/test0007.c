#include "sute.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sha512.h"

#if 0
#define SHOW
#endif

#ifdef SHOW

/**
 * @param
 * @param
 *
 */
static void print_hash(unsigned char *hash){
	for(int i = 0; i < SHA512_DIGEST_LENGTH; i++)
	{
		echo(STDERR,"%02x",hash[i]);
	}
	echo(STDERR,"\n");
}
#endif

// TEST 1
static Return libmem_test_1(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];

	size_t array_length = 1792;
	size_t array_size = array_length * sizeof(int);
	unsigned char *int_array = (unsigned char *)calloc(array_length,sizeof(int));

	if(int_array == NULL)
	{
		report("Memory callocation failed with bytes %zu",array_length * sizeof(int));
		return(FAILURE);
	}

	// Seed random number generator
	srand((unsigned int)time(NULL));

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		int_array[i] = (unsigned char)(rand() % 256);
	}

	// Calculate SHA-512 hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,int_array,array_size);
	sha512_final(&ctx,hash_1);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 1 array size: %zu bytes, array_length=%zu, sizeof(int)=%zu bytes\n",array_size,array_length,sizeof(int));
	echo(STDERR,"Test 1 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure int
	MSTRUCT(mem_int,test1);

	// Create an int memory
	realloc_int(test1,array_length);

	// Test memeory edges
	memcpy(test1->mem,int_array,test1->length * sizeof(test1->mem[0]));
	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,(const unsigned char *)test1->mem,test1->length * sizeof(test1->mem[0]));
	sha512_final(&ctx,hash_2);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 1 Array size: %zu bytes\n",test1->length * sizeof(test1->mem[0]));
	echo(STDERR,"Test 1 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 1 fail\n");
		status = FAILURE;
	}

	// free an empty int array
	del_int(&test1);
	free(int_array);

	RETURN_STATUS;
}

// TEST 2
static Return libmem_test_2(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];

	size_t array_length = 512;
	size_t array_size = array_length * sizeof(char);
	unsigned char *char_array = (unsigned char *)calloc(array_length,sizeof(char));

	// Seed random number generator
	srand((unsigned int)time(NULL));

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		char_array[i] = (unsigned char)(rand() % 256);
	}

	// Calculate SHA-512 hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,char_array,array_size);
	sha512_final(&ctx,hash_1);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 2 array size: %zu bytes, array_length=%zu, sizeof(char)=%zu bytes\n",array_size,array_length,sizeof(char));
	echo(STDERR,"Test 2 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure char
	MSTRUCT(mem_char,test2);

	// Create an char memory
	realloc_char(test2,array_length);

	// Test memeory edges
	memcpy(test2->mem,char_array,test2->length * sizeof(test2->mem[0]));

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,(const unsigned char *)test2->mem,test2->length * sizeof(test2->mem[0]));
	sha512_final(&ctx,hash_2);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 2 array size: %zu bytes\n",test2->length * sizeof(test2->mem[0]));
	echo(STDERR,"Test 2 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 2 fail\n");
		status = FAILURE;
	}

	// free an empty char array
	del_char(&test2);
	free(char_array);

	RETURN_STATUS;
}

// TESTS 3,4,5
static Return libmem_test_3_4_5(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];

	// TEST 3
	size_t array_length = 4096;
	size_t array_size = array_length * sizeof(unsigned long long int);
	unsigned char *ullint_array = (unsigned char *)calloc(array_length,sizeof(unsigned long long int));

	// Seed random number generator
	srand((unsigned int)time(NULL));

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ullint_array[i] = (unsigned char)(rand() % 256);
	}

	// Calculate SHA-512 hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 3 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 3 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure char
	MSTRUCT(mem_ullint_t,test);

	// Create an unsigned long long int memory
	realloc_ullint(test,array_length);

	// Test memeory edges
	memcpy(test->mem,ullint_array,test->length * sizeof(test->mem[0]));

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,(const unsigned char *)test->mem,test->length * sizeof(test->mem[0]));
	sha512_final(&ctx,hash_2);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 3 array size: %zu bytes\n",test->length * sizeof(test->mem[0]));
	echo(STDERR,"Test 3 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 3 fail\n");
		status = FAILURE;
	}

	// TEST 4
	array_length = 256;
	array_size = array_length * sizeof(unsigned long long int);
	ullint_array = (unsigned char *)realloc(ullint_array,array_size);

	// Seed random number generator
	srand((unsigned int)time(NULL));

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ullint_array[i] = (unsigned char)(rand() % 256);
	}

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 4 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 4 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Create an unsigned long long int memory
	realloc_ullint(test,array_length);

	// Test memeory edges
	memcpy(test->mem,ullint_array,test->length * sizeof(test->mem[0]));

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,(const unsigned char *)test->mem,test->length * sizeof(test->mem[0]));
	sha512_final(&ctx,hash_2);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 4 array size: %zu bytes\n",test->length * sizeof(test->mem[0]));
	echo(STDERR,"Test 4 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 4 fail\n");
		status = FAILURE;
	}

	// TEST 5
	array_length = 128;
	array_size = array_length * sizeof(unsigned long long int);
	ullint_array = (unsigned char *)realloc(ullint_array,array_size);

	// Seed random number generator
	srand((unsigned int)time(NULL));

	// Fill array with random bytes
	for(size_t i = 0; i < array_size; i++)
	{
		ullint_array[i] = (unsigned char)(rand() % 256);
	}

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,ullint_array,array_size);
	sha512_final(&ctx,hash_1);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 5 array size: %zu bytes, array_length=%zu, sizeof(unsigned long long int)=%zu bytes\n",array_size,array_length,sizeof(unsigned long long int));
	echo(STDERR,"Test 5 SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Create an unsigned long long int memory
	realloc_ullint(test,array_length,true);

	// Test memeory edges
	memcpy(test->mem,ullint_array,test->length * sizeof(test->mem[0]));

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,(const unsigned char *)test->mem,test->length * sizeof(test->mem[0]));
	sha512_final(&ctx,hash_2);

	#ifdef SHOW
	// Print array summary and hash
	echo(STDERR,"Test 5 array size: %zu bytes\n",test->length * sizeof(test->mem[0]));
	echo(STDERR,"Test 5 SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test 5 fail\n");
		status = FAILURE;
	}

	// free an empty unsigned long long int array
	del_ullint(&test);
	free(ullint_array);

	#ifdef SHOW
	telemetry_show();
	#endif

	return(status);
}

// Main test runner
Return test0007(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(libmem_test_1,"libmem Memory allocator test 1…");
	TEST(libmem_test_2,"libmem Memory allocator test 2…");
	TEST(libmem_test_3_4_5,"libmem Memory allocator test 3,4,5…");

	RETURN_STATUS;
}
