/**
 * @file mem.h
 * @brief Memory management header with telemetry support
 * @details Provides structures and functions for dynamic memory allocation
 * with built-in telemetry tracking
 */

#pragma once

#define SHOW 0  /* Change to 1 to print out debug details */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "rational.h"
#include "telemetry.h"

// Package complex types containing spaces into single tokens
// for macro usage (CONCAT and MEM_TYPE)
typedef unsigned long long int ullint;

/**
 * @brief Memory alignment for allocation (4KB pages)
 * @note Can be changed to 2MB for huge pages: 2*1024*1024 = 2097152 bytes
 */
#define PAGE_BYTES (4UL*1024) // 4KB standard page size

/**
 * @brief Helper macro for token concatenation
 * @param a First token to concatenate
 * @param b Second token to concatenate
 * @return Concatenated tokens as a single identifier
 *
 * This macro uses the C preprocessor's token concatenation operator (##)
 * to join two tokens together. It's used as a helper for CONCAT macro
 * to ensure proper macro expansion.
 */
#define CONCAT_HELPER(a,b) a ## b

/**
 * @brief Safe macro for token concatenation
 * @param a First token to concatenate
 * @param b Second token to concatenate
 * @return Result of CONCAT_HELPER macro
 *
 * This macro ensures proper macro expansion before concatenation.
 * It passes its arguments to CONCAT_HELPER after they are fully expanded.
 *
 * Example:
 * If TYPE is defined as double:
 * CONCAT(mem_, TYPE) -> CONCAT(mem_, double) -> CONCAT_HELPER(mem_, double) -> mem_double
 */
#define CONCAT(a,b) CONCAT_HELPER(a,b)

/**
 * @brief Type-specific memory type definition
 *
 * Creates a new type identifier by concatenating "mem_" with the value of TYPE.
 * Used for creating type-specific memory allocation functions and types.
 *
 * Example:
 * If TYPE is defined as double:
 * MEM_TYPE expands to mem_double
 *
 * This allows for generic code that can work with different types by just
 * changing the TYPE definition.
 */
#define MEM_TYPE CONCAT(mem_,TYPE)
#define REALLOC_TYPE CONCAT(realloc_,TYPE)
#define DEL_TYPE CONCAT(del_,TYPE)

/**
 * @brief Creates and initializes a structure variable with its pointer
 *
 * This macro performs three operations:
 * 1. Declares a structure variable with underscore prefix
 * 2. Creates a pointer to this structure
 * 3. Initializes the structure memory to zero
 *
 * @param struct_type The type of the structure to create (must be a valid structure type)
 * @param struct_name Base name for the structure variable (will be used to create both the structure and pointer names)
 *
 * @note The macro creates two variables:
 *       - _struct_name: The actual structure variable (prefixed with underscore)
 *       - struct_name: A pointer to the structure
 *
 * @warning Make sure struct_type is a valid structure type defined before using this macro
 * @warning The macro uses memset, so include <string.h> before using it
 *
 * @example
 * // Example usage:
 * typedef struct {
 *     int id;
 *     float value;
 * } MyStruct;
 *
 * void function() {
 *     MSTRUCT(MyStruct, data);  // Creates _data structure and data pointer
 *     data->id = 1;            // Access via pointer
 * }
 *
 * @see memset
 */
#define MSTRUCT(struct_type,struct_name) \
	struct_type _ ## struct_name; \
	struct_type *struct_name = &_ ## struct_name; \
	memset(struct_name,0,sizeof(struct_type));

/* The same w/o memset to use as global structures */
#define GSTRUCT(struct_type,struct_name) \
	struct_type _ ## struct_name; \
	struct_type *struct_name = &_ ## struct_name; \

// Macros to define default value of third аrgument
// https://stackoverflow.com/questions/1472138/c-default-arguments
// https://gustedt.wordpress.com/2010/06/03/default-arguments-for-c99/
#define realloc_char(...) REALLOC_CHAR(__VA_ARGS__,false,0)
#define REALLOC_CHAR(a,b,c,...) realloc_char(a,b,c)

#define realloc_int(...) REALLOC_INT(__VA_ARGS__,false,0)
#define REALLOC_INT(a,b,c,...) realloc_int(a,b,c)

#define realloc_ullint(...) REALLOC_ULLINT(__VA_ARGS__,false,0)
#define REALLOC_ULLINT(a,b,c,...) realloc_ullint(a,b,c)

/**
 * @brief Structure for char array management
 */
typedef struct {
	size_t length;    ///< Current array length
	size_t allocated; ///< Actually allocated memory in bytes
	char *mem;        ///< Pointer to dynamic array
} mem_char;

/**
 * @brief Structure for int array management
 */
typedef struct {
	size_t length;    ///< Current array length
	size_t allocated; ///< Actually allocated memory in bytes
	int *mem;         ///< Pointer to dynamic array
} mem_int;

/**
 * @brief Structure for unsigned long long int array management
 */
typedef struct {
	size_t length;    ///< Current array length
	size_t allocated; ///< Actually allocated memory in bytes
	unsigned long long int *mem; ///< Pointer to dynamic array
} mem_ullint;

/**
 *
 * @brief Memory management function declarations
 *
 */

/**
 * @brief Resizes heap memory to a specified size
 * @param prevent_or_decrease_an_allocated_memory
 * true - Actually reduces the size of already allocated memory during reallocation.
 * Real memory reduction during reallocation consumes CPU time and will impact
 * performance, especially if it's followed by another reallocation to increase
 * memory size.
 *
 * false - Default value (argument can be omitted). Prevents actual memory
 * reduction, assuming that the already allocated memory segment will be
 * reused again.
 *
 * Parameter passing is implemented using a macro. For details, see:
 * https://stackoverflow.com/questions/1472138/c-default-arguments
 * https://gustedt.wordpress.com/2010/06/03/default-arguments-for-c99/
 *
 */
Return realloc_char(
	mem_char *,
	const size_t,
	bool
);

Return copy_char(
	mem_char *,
	mem_char *
);

Return append_char(
	mem_char *,
	mem_char *
);

Return del_char(mem_char **);

Return realloc_int(
	mem_int *,
	const size_t,
	bool
);

Return del_int(mem_int **);

Return realloc_ullint(
	mem_ullint *,
	const size_t,
	bool
);

Return del_ullint(mem_ullint **);

Return copy_ullint(
	mem_ullint *,
	mem_ullint *
);