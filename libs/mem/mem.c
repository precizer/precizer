/**
 * @file mem.c
 * @brief Memory management implementation for basic data types
 * @details Provides functions for dynamic memory allocation and management
 * with telemetry tracking capabilities
 */

#include "structs.h"
#include "mem.h"

Telemetry telemetry = {0}; // Initialize with zeros

/**
 * @brief Aligns memory size to page boundary
 * @param size Original size in bytes to be aligned
 * @return Aligned size in bytes
 * @details Calculates the next multiple of PAGE_BYTES for memory allocation
 */
__attribute__((always_inline)) static inline size_t get_aligned_bytes(
	const size_t size
){
	// Reallocate size to the next multiple of PAGE_BYTES
	// (size + (PAGE_BYTES - 1)) rounds up the size
	// & ~(PAGE_BYTES - 1) creates a mask to truncate to nearest multiple
	size_t allocated_size = (size + (PAGE_BYTES - 1)) & ~(PAGE_BYTES - 1);

	#if 0
	printf("size=%zu,allocated_size=%zu\n", size, allocated_size);
	#endif

	return allocated_size;
}

/*
 * Char type memory management functions
 */

/**
 * @brief Reallocates memory for char array
 * @param structure Pointer to mem_char structure
 * @param length New desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return realloc_char(
	mem_char *structure,
	const size_t length,
	bool true_reduce
){
	#define TYPE char
	#include "realloc.d"
	#undef TYPE
}

/**
 * @brief Reallocate destination and make a copy from source to destination
 * @param destination Pointer to mem_char structure
 * @param source Pointer to mem_char structure
 */
Return copy_char(
	mem_char *destination,
	mem_char *source
){
	#define TYPE char
	#include "copy.d"
	#undef TYPE
}

/**
 * @brief Concatenates the contents of two mem_char structures, appending source data to the destination.
 *
 * @param destination Pointer to the target mem_char structure where data will be added.
 * @param source Pointer to the source mem_char structure whose data will be appended.
 *
 * @return Operation status (SUCCESS on success, FAILURE on error).
 *
 * @details This function performs concatenation of data from the source structure
 * to the end of the destination structure. The operation includes:
 * - Validating input pointers for correctness
 * - Allocating additional memory for data merging
 * - Copying source contents to the end of destination
 *
 * @note Careful memory management is required after the operation.
 *
 * @warning Improper use may result in data loss or memory leaks.
 *
 * @see mem_char
 */
Return append_char(
	mem_char *destination,
	mem_char *source
){
	#define TYPE char
	#include "append.d"
	#undef TYPE
}

/**
 * @brief Allocates and zeros memory for char array
 * @param structure Pointer to mem_char structure
 * @param length Desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return calloc_char(
	mem_char *structure,
	const size_t length,
	bool true_reduce
){
	#define CALLOC 0
	#define TYPE char
	#include "realloc.d"
	#undef TYPE
	#undef CALLOC
}


/**
 * @brief Deallocates memory for char array
 * @param structure Pointer to pointer to mem_char structure
 */
Return del_char(mem_char **structure)
{
	#define TYPE char
	#include "del.d"
	#undef TYPE
}

/*
 * Int type memory management functions
 */

/**
 * @brief Reallocates memory for int array
 * @param structure Pointer to mem_int structure
 * @param length New desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return realloc_int(
	mem_int *structure,
	const size_t length,
	bool true_reduce
){
	#define TYPE int
	#include "realloc.d"
	#undef TYPE
}

/**
 * @brief Allocates and zeros memory for int array
 * @param structure Pointer to mem_int structure
 * @param length Desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return calloc_int(
	mem_int *structure,
	const size_t length,
	bool true_reduce
){
	#define CALLOC 0
	#define TYPE int
	#include "realloc.d"
	#undef TYPE
	#undef CALLOC
}

/**
 * @brief Deallocates memory for int array
 * @param structure Pointer to pointer to mem_int structure
 */
Return del_int(mem_int **structure)
{
	#define TYPE int
	#include "del.d"
	#undef TYPE
}

/*
 * Unsigned long long int type memory management functions
 */

/**
 * @brief Reallocates memory for unsigned long long int array
 * @param structure Pointer to mem_ullint_t structure
 * @param length New desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return realloc_ullint(
	mem_ullint_t *structure,
	const size_t length,
	bool true_reduce
){
	#define TYPE ullint_t
	#include "realloc.d"
	#undef TYPE
}

/**
 * @brief Allocates and zeros memory for unsigned long long int array
 * @param structure Pointer to mem_ullint_t structure
 * @param length Desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return calloc_ullint(
	mem_ullint_t *structure,
	const size_t length,
	bool true_reduce
){
	#define CALLOC 0
	#define TYPE ullint_t
	#include "realloc.d"
	#undef TYPE
	#undef CALLOC
}

/**
 * @brief Deallocates memory for unsigned long long int array
 * @param structure Pointer to pointer to mem_ullint_t structure
 */
Return del_ullint(mem_ullint_t **structure)
{
	#define TYPE ullint_t
	#include "del.d"
	#undef TYPE
}

#ifdef UNITTEST
// Build it
// gcc -I../rational/ mem.c telemetry.c -L../../.builds/debug/libs -static -lrational -lssl -lcrypto

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <time.h>

/**
 * @param len Количество байт
 * @param p Указатель на начало массива
 *
 */
void writesum(char *source, void *destination, size_t len)
{
	unsigned char *d = (unsigned char*)destination;
	unsigned char *s = (unsigned char*)source;

	for (size_t i = 0; i < len; i++)
	{
		d[i] = s[i];
	}
}

void print_hash(unsigned char *hash) {
	for (int i = 0; i < SHA512_DIGEST_LENGTH; i++) {
		printf("%02x", hash[i]);
	}
	printf("\n");
}

int main(void)
{
	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];

	// TEST 1
	size_t array_length = 1792;
	size_t array_size = array_length * sizeof(int);
	unsigned char *int_array = (unsigned char *)calloc(array_length, sizeof(int));

	// Seed random number generator
	srand(time(NULL));

	// Fill array with random bytes
	for (size_t i = 0; i < array_size; i++) {
		int_array[i] = rand() % 256;
	}

	// Calculate SHA-512 hash
	SHA512(int_array, array_size, hash_1);

	#if 0
	// Print array summary and hash
	printf("Array size: %ld bytes, array_length=%ld, sizeof(int)=%ld bytes\n", array_size, array_length, sizeof(int));
	printf("SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure int
	MSTRUCT(mem_int,test1);

	// Create an int memory
	realloc_int(test1,array_length);

	// Test memeory edges
	writesum(int_array, test1->mem, test1->length * sizeof(test1->mem[0]));

	// Calculate SHA-512 hash
	SHA512((const unsigned char *)test1->mem, test1->length * sizeof(test1->mem[0]), hash_2);

	#if 0
	// Print array summary and hash
	printf("Array size: %d bytes\n", test1->length * sizeof(test1->mem[0]));
	printf("SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 == strncmp(hash_1, hash_2, (size_t)SHA512_DIGEST_LENGTH))
	{
		printf("Test 1 successful\n");
	} else {
		printf("Test 1 UNSUCCESSFUL\n");
	}

	// free an empty int array
	del_int(&test1);
	free(int_array);

	// TEST 2
	array_length = 512;
	array_size = array_length * sizeof(char);
	unsigned char *char_array = (unsigned char *)calloc(array_length, sizeof(char));

	// Seed random number generator
	srand(time(NULL));

	// Fill array with random bytes
	for (size_t i = 0; i < array_size; i++) {
		char_array[i] = rand() % 256;
	}

	// Calculate SHA-512 hash
	SHA512(char_array, array_size, hash_1);

	#if 0
	// Print array summary and hash
	printf("Array size: %ld bytes, array_length=%ld, sizeof(char)=%ld bytes\n", array_size,array_length, sizeof(char));
	printf("SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure char
	MSTRUCT(mem_char,test2);

	// Create an char memory
	realloc_char(test2,array_length);

	// Test memeory edges
	writesum(char_array, test2->mem, test2->length * sizeof(test2->mem[0]));

	// Calculate SHA-512 hash
	SHA512(test2->mem, test2->length * sizeof(test2->mem[0]), hash_2);

	#if 0
	// Print array summary and hash
	printf("Array size: %d bytes\n", test2->length * sizeof(test2->mem[0]));
	printf("SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 == strncmp(hash_1, hash_2, (size_t)SHA512_DIGEST_LENGTH))
	{
		printf("Test 2 successful\n");
	} else {
		printf("Test 2 UNSUCCESSFUL\n");
	}

	// free an empty char array
	del_char(&test2);
	free(char_array);

	// TEST 3
	array_length = 4096;
	array_size = array_length * sizeof(unsigned long long int);
	unsigned char *ullint_array = (unsigned char *)calloc(array_length, sizeof(unsigned long long int));

	// Seed random number generator
	srand(time(NULL));

	// Fill array with random bytes
	for (size_t i = 0; i < array_size; i++) {
		ullint_array[i] = rand() % 256;
	}

	// Calculate SHA-512 hash
	SHA512(ullint_array, array_size, hash_1);

	#if 0
	// Print array summary and hash
	printf("Array size: %ld bytes, array_length=%ld, sizeof(unsigned long long int)=%ld bytes\n", array_size, array_length, sizeof(unsigned long long int));
	printf("SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure char
	MSTRUCT(mem_ullint_t,test3);

	// Create an unsigned long long int memory
	realloc_ullint(test3,array_length);

	// Test memeory edges
	writesum(ullint_array, test3->mem, test3->length * sizeof(test3->mem[0]));

	// Calculate SHA-512 hash
	SHA512((const unsigned char *)test3->mem, test3->length * sizeof(test3->mem[0]), hash_2);

	#if 0
	// Print array summary and hash
	printf("Array size: %d bytes\n", test3->length * sizeof(test3->mem[0]));
	printf("SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 == strncmp(hash_1, hash_2, (size_t)SHA512_DIGEST_LENGTH))
	{
		printf("Test 3 successful\n");
	} else {
		printf("Test 3 UNSUCCESSFUL\n");
	}

	// TEST 4
	array_length = 256;
	array_size = array_length * sizeof(unsigned long long int);
	ullint_array = (unsigned char *)realloc(ullint_array, array_size);

	// Seed random number generator
	srand(time(NULL));

	// Fill array with random bytes
	for (size_t i = 0; i < array_size; i++) {
		ullint_array[i] = rand() % 256;
	}

	// Calculate SHA-512 hash
	SHA512(ullint_array, array_size, hash_1);

	#if 0
	// Print array summary and hash
	printf("Array size: %ld bytes, array_length=%ld, sizeof(unsigned long long int)=%ld bytes\n", array_size, array_length, sizeof(unsigned long long int));
	printf("SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Create an unsigned long long int memory
	realloc_ullint(test3,array_length);

	// Test memeory edges
	writesum(ullint_array, test3->mem, test3->length * sizeof(test3->mem[0]));

	// Calculate SHA-512 hash
	SHA512((const unsigned char *)test3->mem, test3->length * sizeof(test3->mem[0]), hash_2);

	#if 0
	// Print array summary and hash
	printf("Array size: %d bytes\n", test3->length * sizeof(test3->mem[0]));
	printf("SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 == strncmp(hash_1, hash_2, (size_t)SHA512_DIGEST_LENGTH))
	{
		printf("Test 4 successful\n");
	} else {
		printf("Test 4 UNSUCCESSFUL\n");
	}

	// TEST 5
	array_length = 128;
	array_size = array_length * sizeof(unsigned long long int);
	ullint_array = (unsigned char *)realloc(ullint_array, array_size);

	// Seed random number generator
	srand(time(NULL));

	// Fill array with random bytes
	for (size_t i = 0; i < array_size; i++) {
		ullint_array[i] = rand() % 256;
	}

	// Calculate SHA-512 hash
	SHA512(ullint_array, array_size, hash_1);

	#if 0
	// Print array summary and hash
	printf("Array size: %ld bytes, array_length=%ld, sizeof(unsigned long long int)=%ld bytes\n", array_size, array_length, sizeof(unsigned long long int));
	printf("SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Create an unsigned long long int memory
	realloc_ullint(test3,array_length,true);

	// Test memeory edges
	writesum(ullint_array, test3->mem, test3->length * sizeof(test3->mem[0]));

	// Calculate SHA-512 hash
	SHA512((const unsigned char *)test3->mem, test3->length * sizeof(test3->mem[0]), hash_2);

	#if 0
	// Print array summary and hash
	printf("Array size: %d bytes\n", test3->length * sizeof(test3->mem[0]));
	printf("SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 == strncmp(hash_1, hash_2, (size_t)SHA512_DIGEST_LENGTH))
	{
		printf("Test 5 successful\n");
	} else {
		printf("Test 5 UNSUCCESSFUL\n");
	}

	// free an empty unsigned long long int array
	del_ullint(&test3);
	free(ullint_array);

	telemetry_show();
}
#endif /* UNITTEST */
