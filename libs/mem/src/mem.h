#ifndef MEM_H
#define MEM_H

#include "rational.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h> /* SIZE_MAX */
#include <limits.h>
#include "mem_telemetry.h"

/** Minimum allocation block size (in bytes) for the helper. */
#define MEMORY_BLOCK_BYTES (4UL * 1024)

/**
 * @file mem.h
 * @brief Public API for a small, typed dynamic-memory helper.
 *
 * This library provides a tiny descriptor (`struct memory`) that stores the size
 * of one element, the current element count (`length`), and a data pointer. The element
 * size is fixed once via the `create(T, name)` macro and reused by allocation
 * routines.
 *
 * @see mem_resize.c and related implementation units for details.
 */

/**
 * @brief Memory Helper API
 *
 * Top-level group for all public symbols of the memory helper.
 */

/**
 * @struct memory
 * @brief Describes a typed dynamic memory block.
 *
 * @var memory::element_size
 * Size in bytes of one array element. Set once by @ref create.
 *
 * @var memory::length
 * Current number of elements in the allocated block.
 *
 * @var memory::actually_allocated_bytes
 * Actually allocated memory in bytes
 *
 * @var memory::data
 * Pointer to the beginning of the allocated block (or NULL if none).
 */
typedef struct memory {
	size_t element_size;
	size_t length;
	size_t actually_allocated_bytes;
	void *data;
} memory;

/**
 * @brief Resize behavior flags for @ref memory_resize.
 *
 * These masks can be combined to tune how @ref memory_resize behaves:
 * - `ZERO_NEW_MEMORY` mirrors `calloc` semantics by clearing any bytes that
 *   become newly addressable when a descriptor grows.
 * - `RELEASE_UNUSED` instructs the helper to release excess capacity immediately
 *   when the requested length decreases, instead of holding on to the buffer.
 *
 * Flags may be OR-ed together (for example, `ZERO_NEW_MEMORY | RELEASE_UNUSED`)
 * so that both behaviors are enabled during one call.
 */
typedef enum
{
	ZERO_NEW_MEMORY = 0x01u,
	RELEASE_UNUSED = 0x02u
} RESIZEMODES;

/**
 * @brief Allocation functions
 *
 * Functions for allocating, resizing, and freeing memory.
 */

/**
 * @brief Resize the managed block to hold the given number of elements.
 *
 * @param memory_object  Pointer to a descriptor initialized via @ref create.
 * @param element_count  New number of elements.
 * @param ...            Optional @ref RESIZEMODES mask controlling zero-fill or shrink behavior.
 * @return `SUCCESS` on success; `FAILURE` otherwise. All failures are reported
 *         through @ref slog for easier diagnostics.
 *
 * @post If @p element_count is 0, the function frees the block and sets
 *       @ref memory::data to NULL and @ref memory::length to 0.
 *
 * @warning The returned data pointer may change; always refresh any cached
 *          pointers after a successful resize.
 */
Return memory_resize(
	memory *memory_object,
	size_t element_count,
	...);

/**
 * @brief Free the allocated block and reset the descriptor (except element size).
 *
 * @param memory_object  Pointer to a descriptor.
 *
 * @post Sets @ref memory::data to NULL and @ref memory::length to 0.
 *       The @ref memory::element_size remains unchanged so the descriptor can be
 *       allocated again for the same element type.
 */
Return memory_delete(memory *memory_object);

/**
 * @brief Copy the contents of @p source descriptor into @p destination.
 *
 * @param destination Pointer to the destination descriptor (resized if needed).
 * @param source      Pointer to the source descriptor to copy from.
 * @return `SUCCESS` on success; `FAILURE` otherwise.
 */
Return memory_copy(
	memory       *destination,
	const memory *source);

/**
 * @brief Append the contents of @p source descriptor to @p destination.
 *
 * @param destination Pointer to the destination descriptor to extend.
 * @param source      Pointer to the source descriptor to append from.
 * @return `SUCCESS` on success; `FAILURE` otherwise.
 */
Return memory_append(
	memory       *destination,
	const memory *source);

/**
 * @brief Concatenate string data held in descriptors, keeping exactly one trailing NUL.
 *
 * Treats the managed memory as byte-oriented strings (element size must be 1). The
 * resulting descriptor is resized to `len(destination) + len(source) + 1` and made
 * null-terminated even if the inputs lacked a terminator.
 *
 * @param destination Pointer to the descriptor receiving the concatenated string.
 * @param source      Pointer to the descriptor providing the appended string.
 * @return `SUCCESS` on success; `FAILURE` otherwise.
 */
Return memory_concat_strings(
	memory       *destination,
	const memory *source);

/**
 * @brief Append a C-style literal string to a descriptor holding byte-sized elements.
 *
 * Resizes @p destination to `len(destination) + strlen(literal) + 1`, copies the literal,
 * and guarantees a single trailing null terminator.
 *
 * @param destination Pointer to the descriptor receiving the literal contents.
 * @param literal     Pointer to a null-terminated C string.
 * @return `SUCCESS` on success; `FAILURE` otherwise.
 */
Return memory_concat_literal(
	memory     *destination,
	const char *literal);

/**
 * @brief Copy a C-style literal string into a descriptor holding byte-sized elements.
 *
 * Resizes @p destination to `strlen(literal) + 1`, copies the literal, and guarantees a
 * trailing null terminator. Previous contents of @p destination are discarded.
 *
 * @param destination Pointer to the descriptor receiving the literal.
 * @param literal     Pointer to a null-terminated C string.
 * @return `SUCCESS` on success; `FAILURE` otherwise.
 */
Return memory_copy_literal(
	memory     *destination,
	const char *literal);

/** @cond INTERNAL */
/**
 * @brief Multiply two size_t values with overflow detection.
 *
 * Used by implementation files to detect overflows when computing byte counts.
 *
 * @param left     Left operand.
 * @param right    Right operand.
 * @param product  Output pointer for the product on success.
 * @return Return status indicating whether the multiplication succeeded.
 */
Return memory_guarded_size(
	size_t left,
	size_t right,
	size_t *product);
/** @endcond */

/**
 * @brief Compute the visible length of string data stored in a descriptor.
 *
 * The scan stops either at the first null byte or once @ref memory::length bytes
 * have been inspected. This ensures the function respects both fully utilized blocks
 * and partially filled buffers.
 *
 * @param memory_object Descriptor whose contents are interpreted as a string.
 * @param length_out    Output pointer that receives the computed length.
 * @return `SUCCESS` on success; `FAILURE` otherwise.
 */
Return memory_string_length(
	const memory *memory_object,
	size_t       *length_out);

/**
 * @brief Provide a safe read-only pointer to descriptor-backed string data.
 *
 * Guarantees that callers always receive a valid, null-terminated byte sequence:
 * - When @p memory_object is NULL, uninitialized, or sized for non-byte
 *   elements, the function returns an empty string.
 * - When the descriptor lacks a null terminator within @ref memory::length
 *   bytes, the function also falls back to an empty string rather than exposing
 *   potentially uninitialized memory.
 *
 * This helper is ideal when passing managed buffers to functions such as
 * `printf`, `puts`, or regex engines where a missing terminator would otherwise
 * lead to undefined behavior.
 *
 * @param memory_object Descriptor interpreted as a character buffer.
 * @return Pointer to a guaranteed null-terminated string (never NULL).
 */
const char *memory_getcstring(const memory *memory_object);

/**
 * @brief Provide a writable pointer to descriptor-backed string data, creating
 *        an empty string fallback when needed.
 *
 * Ensures that callers can safely treat a descriptor as holding a mutable
 * C-style string:
 * - If the descriptor is NULL or its metadata is invalid, the helper returns a
 *   pointer to a shared zero byte instead of NULL.
 * - If the descriptor has zero length, it is resized to hold at least one
 *   null terminator.
 * - If the descriptor lacks a terminator within @ref memory::length bytes, the
 *   first byte is set to `'\0'` before returning.
 *
 * @param memory_object Descriptor interpreted as a mutable character buffer.
 * @return Pointer to a writable string (never NULL). When fallbacks are used,
 *         modifications affect only the shared zero byte.
 */
char *memory_getstring(memory *memory_object);

/**
 * @brief Checked typed access
 *
 * Runtime type verification and typed data access.
 */

/**
 * @brief Verify that the descriptor's element size matches @p expected_element_size.
 *
 * @param memory_object         Pointer to a descriptor.
 * @param expected_element_size Expected element size in bytes (typically `sizeof(T)`).
 * @return `SUCCESS` if the sizes match; `FAILURE` otherwise (or when
 *         @p memory_object is NULL).
 */
Return memory_verify_type(
	const memory *memory_object,
	size_t       expected_element_size);

/**
 * @brief Return a writable data pointer after verifying the element size.
 *
 * Performs a runtime check that the descriptor's element size matches
 * @p expected_element_size. On mismatch, the function logs the error and
 * returns `NULL`.
 *
 * @param memory_object         Pointer to a descriptor.
 * @param expected_element_size Expected element size in bytes (typically `sizeof(T)`).
 * @return Non-NULL `void*` on success; `NULL` on error.
 */
void *memory_data_checked(
	memory *memory_object,
	size_t expected_element_size);

/**
 * @brief Return a read-only data pointer after verifying the element size.
 *
 * Same behavior as @ref memory_data_checked but returns a `const void*`.
 *
 * @param memory_object         Pointer to a descriptor.
 * @param expected_element_size Expected element size in bytes (typically `sizeof(T)`).
 * @return Non-NULL `const void*` on success; `NULL` on error.
 */
const void *memory_const_data_checked(
	const memory *memory_object,
	size_t       expected_element_size);

/**
 * @brief Return the descriptor's raw data pointer without additional checks.
 *
 * @param memory_object Pointer to the descriptor.
 * @return Underlying pointer or NULL when @p memory_object itself is NULL.
 */
static inline void *memory_rawdata(memory * const memory_object)
{
	if(memory_object == NULL)
	{
		return NULL;
	}
	return memory_object->data;
}

/**
 * @brief Return the descriptor's raw read-only pointer without additional checks.
 *
 * @param memory_object Pointer to the descriptor.
 * @return Underlying pointer or NULL when @p memory_object itself is NULL.
 */
static inline const void *memory_raw_const_data(const memory * const memory_object)
{
	if(memory_object == NULL)
	{
		return NULL;
	}
	return memory_object->data;
}

/**
 * @brief Convenience macros
 *
 * Declarative and helper macros for user code.
 */

/**
 * @def create(T, variable_name)
 * @brief Declare and initialize a @ref memory descriptor on the stack.
 *
 * Sets @ref memory::element_size to `sizeof(T)`, zeroes the count, and sets
 * @ref memory::data to `NULL`.
 *
 * @param T             The element type (e.g., `int`, `struct point`, `double`).
 * @param variable_name The descriptor variable name to declare.
 *
 * Internally it declares a storage descriptor named `_variable_name` and exposes
 * `memory *variable_name` pointing to that storage so client code can always use
 * pointer-style access.
 *
 * @warning This macro expands to a variable declaration. Use it only where
 *          declarations are allowed. The macro does not include a trailing
 *          semicolon; add one at the call site.
 */
#define create(T,variable_name) \
	memory _ ## variable_name = (memory){sizeof(T),0,0,NULL}; \
	memory *variable_name = &_ ## variable_name

/**
 * @def resize(variable_name, number_of_elements)
 * @brief Resize (or allocate) the block to @p number_of_elements elements.
 */
#define resize(descriptor_expression,number_of_elements,...) \
	memory_resize((descriptor_expression),(number_of_elements) \
	__VA_OPT__( ,__VA_ARGS__),UCHAR_MAX)

/**
 * @def del(variable_name)
 * @brief Free the allocation and reset the descriptor (except element size).
 */
#define del(descriptor_expression) \
	memory_delete((descriptor_expression))

/**
 * @def data(T, variable_name)
 * @brief Get a writable typed pointer with a runtime check.
 *
 * Internally calls @ref memory_data_checked with `sizeof(T)` and casts the result
 * to `T*`. Returns `NULL` on mismatch and logs the error.
 */
#define data(T,descriptor_expression) \
	((T *)memory_data_checked((descriptor_expression),sizeof(T)))

/**
 * @def rawdata(variable_name)
 * @brief Obtain the raw writable pointer (may be NULL if descriptor is NULL).
 */
#define rawdata(descriptor_expression) \
	memory_rawdata((descriptor_expression))

/**
 * @def cdata(T, variable_name)
 * @brief Get a read-only typed pointer with a runtime check.
 *
 * Internally calls @ref memory_const_data_checked with `sizeof(T)` and casts the result
 * to `const T*`. Returns `NULL` on mismatch and logs the error.
 */
#define cdata(T,descriptor_expression) \
	((const T *)memory_const_data_checked((descriptor_expression),sizeof(T)))

/**
 * @def rawcdata(variable_name)
 * @brief Obtain the raw read-only pointer (may be NULL if descriptor is NULL).
 */
#define rawcdata(descriptor_expression) \
	memory_raw_const_data((descriptor_expression))

/**
 * @def getstring(variable_name)
 * @brief Obtain a writable C-style string pointer that is always safe to use.
 *
 * Internally calls @ref memory_getstring to guarantee that a null terminator is
 * available. The descriptor must describe byte-sized elements.
 */
#define getstring(descriptor_expression) \
	memory_getstring((descriptor_expression))

/**
 * @def getcstring(variable_name)
 * @brief Obtain a read-only C-style string pointer that is always safe to use.
 *
 * Internally calls @ref memory_getcstring to guard against NULL descriptors,
 * missing allocations, or absent null terminators.
 */
#define getcstring(descriptor_expression) \
	memory_getcstring((descriptor_expression))

/**
 * @def copy(destination, source)
 * @brief Copy one descriptor into another (resizing destination if needed).
 */
#define copy(destination,source) \
	memory_copy((destination),(source))

/**
 * @def append(destination, source)
 * @brief Append @p source contents to @p destination (resizing destination as needed).
 */
#define append(destination,source) \
	memory_append((destination),(source))

/**
 * @def concat_strings(destination, source)
 * @brief Concatenate string descriptors, ensuring a single trailing `'\0'`.
 */
#define concat_strings(destination,source) \
	memory_concat_strings((destination),(source))

/**
 * @def concat_literal(destination, literal_string)
 * @brief Append a null-terminated literal C string to a descriptor with byte-sized elements.
 */
#define concat_literal(destination,literal_string) \
	memory_concat_literal((destination),(literal_string))

/**
 * @def copy_literal(destination, literal_string)
 * @brief Copy a null-terminated literal C string into a descriptor with byte-sized elements.
 */
#define copy_literal(destination,literal_string) \
	memory_copy_literal((destination),(literal_string))

/**
 * @brief Free an arbitrary pointer and reset it to NULL.
 *
 * This helper mirrors the legacy `memold` API and works for any pointer, not just
 * descriptors created via @ref create.
 *
 * @param pointer_handle Address of the pointer to release.
 */
void FREE_AND_RESET(void **pointer_handle);

/**
 * @def reset(pointer_expression)
 * @brief Convenience macro that casts arguments to `void **` for @ref FREE_AND_RESET.
 */
#define reset(pointer_expression) \
	FREE_AND_RESET((void **)(pointer_expression))

/**
 * @def string_length(descriptor, length_out)
 * @brief Measure the utilized byte length within a descriptor interpreted as a string.
 */
#define string_length(descriptor_expression,length_out) \
	memory_string_length((descriptor_expression),(length_out))

/**
 * @page mem_usage Usage Guide & Best Practices
 *
 * @section mem_usage_intro Introduction
 * This page demonstrates typical patterns when using the memory helper.
 *
 * @section mem_usage_quick Quick start
 * @code
 * #include "mem.h"
 * typedef struct { int x, y; } point;
 *
 * int main(void)
 * {
 *   create(point,points);                 // declare + initialize descriptor
 *   if(resize(points,10) != SUCCESS) { }  // handle error
 *   point *p = data(point,points);        // checked typed access
 *   p[0] = (point){1,2};
 *   if(resize(points,20) != SUCCESS) { }  // handle error
 *   p = data(point,points);               // refresh pointer after resizing
 *   del(points);
 *   return 0;
 * }
 * @endcode
 *
 * @section mem_usage_notes Notes & Recommendations
 * - Always refresh any cached typed pointer after a successful @ref resize.
 * - Consider wrapping error codes with your project’s error-handling utilities.
 * - `resize(...,0)` is equivalent to `free` and resets the descriptor
 *   (`data = NULL`, `length = 0`).
 * - Always go through the helper macros so pointer conversions stay explicit and safe.
 */

#endif /* MEM_H */
