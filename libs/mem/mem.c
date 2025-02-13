/**
 * @file mem.c
 * @brief Memory management implementation for basic data types
 * @details Provides functions for dynamic memory allocation and management
 * with telemetry tracking capabilities
 */

#include "mem.h"

Telemetry telemetry = {0}; // Initialize with zeros

/**
 * @brief Aligns memory size to page boundary
 * @param size Original size in bytes to be aligned
 * @return Aligned size in bytes
 * @details Calculates the next multiple of PAGE_BYTES for memory allocation
 */
__attribute__((always_inline)) static inline size_t get_aligned_bytes(const size_t size){
	// Reallocate size to the next multiple of PAGE_BYTES
	// (size + (PAGE_BYTES - 1)) rounds up the size
	// & ~(PAGE_BYTES - 1) creates a mask to truncate to nearest multiple
	size_t allocated_size = (size + (PAGE_BYTES - 1)) & ~(PAGE_BYTES - 1);

	#if SHOW
	printf("requested size=%zu, aligned size=%zu\n",size,allocated_size);
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
	mem_char     *structure,
	const size_t newlength,
	const bool   true_reduce
){
	#define TYPE char
	#include "realloc.cc"
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
	#include "copy.cc"
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
	#include "append.cc"
	#undef TYPE
}

/**
 * @brief Allocates and zeros memory for char array
 * @param structure Pointer to mem_char structure
 * @param length Desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return calloc_char(
	mem_char     *structure,
	const size_t newlength,
	const bool true_reduce
){
	#define CALLOC 0
	#define TYPE char
	#include "realloc.cc"
	#undef TYPE
	#undef CALLOC
}


/**
 * @brief Deallocates memory for char array
 * @param structure Pointer to pointer to mem_char structure
 */
Return del_char(mem_char **structure){
	#define TYPE char
	#include "del.cc"
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
	mem_int      *structure,
	const size_t newlength,
	const bool true_reduce
){
	#define TYPE int
	#include "realloc.cc"
	#undef TYPE
}

/**
 * @brief Allocates and zeros memory for int array
 * @param structure Pointer to mem_int structure
 * @param length Desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return calloc_int(
	mem_int      *structure,
	const size_t newlength,
	const bool true_reduce
){
	#define CALLOC 0
	#define TYPE int
	#include "realloc.cc"
	#undef TYPE
	#undef CALLOC
}

/**
 * @brief Deallocates memory for int array
 * @param structure Pointer to pointer to mem_int structure
 */
Return del_int(mem_int **structure){
	#define TYPE int
	#include "del.cc"
	#undef TYPE
}

/*
 * Unsigned long long int type memory management functions
 */

/**
 * @brief Reallocates memory for unsigned long long int array
 * @param structure Pointer to mem_ullint structure
 * @param length New desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return realloc_ullint(
	mem_ullint   *structure,
	const size_t newlength,
	const bool true_reduce
){
	#define TYPE ullint
	#include "realloc.cc"
	#undef TYPE
}

/**
 * @brief Allocates and zeros memory for unsigned long long int array
 * @param structure Pointer to mem_ullint structure
 * @param length Desired length of the array
 * @param true_reduce If true, actually reduce allocated memory when shrinking
 */
Return calloc_ullint(
	mem_ullint   *structure,
	const size_t newlength,
	const bool true_reduce
){
	#define CALLOC 0
	#define TYPE ullint
	#include "realloc.cc"
	#undef TYPE
	#undef CALLOC
}

/**
 * @brief Deallocates memory for unsigned long long int array
 * @param structure Pointer to pointer to mem_ullint structure
 */
Return del_ullint(mem_ullint **structure){
	#define TYPE ullint
	#include "del.cc"
	#undef TYPE
}

/**
 * @brief Reallocate destination and make a copy from source to destination
 * @param destination Pointer to mem_ullint structure
 * @param source Pointer to mem_ullint structure
 */
Return copy_ullint(
	mem_ullint *destination,
	mem_ullint *source
){
	#define TYPE ullint
	#include "copy.cc"
	#undef TYPE
}
