/**
 * @file mem.h
 * @brief Public API for a small, typed dynamic-memory helper.
 *
 * This library provides a tiny descriptor (`struct memory`) that stores the size
 * of one element, the current element count (`length`), and a data pointer. The element
 * size is fixed via @ref m_init, @ref m_init_static, or the `m_create(T, name)` macro
 * and reused by allocation routines
 */

#ifndef MEM_H
#define MEM_H

#include "rational.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include "mem_telemetry.h"

/** Minimum allocation block size (in bytes) for the helper. */
#define MEMORY_BLOCK_BYTES (4UL * 1024)

/**
 * @brief Memory Helper API
 *
 * Top-level group for all public symbols of the memory helper.
 */

/**
 * @struct memory
 * @brief Describes a typed dynamic memory block.
 *
 * @var memory::single_element_size
 * Size in bytes of one array element. Set once by @ref m_create.
 *
 * @var memory::actually_allocated_bytes
 * Total number of bytes currently reserved by the allocator for this descriptor.
 * This may exceed the logical payload size because the helper aligns allocations
 * to fixed-size blocks. For a valid descriptor it must still cover the current
 * logical payload whenever @ref memory::length is non-zero.
 *
 * @var memory::length
 * Current logical number of elements in the descriptor.
 * In string mode this typically includes the trailing zero element when that
 * terminator is part of the logical descriptor view.
 *
 * @var memory::string_length
 * Number of visible string elements, excluding the terminating zero element.
 * When @ref memory::is_string is `false`, this field must be `0`.
 *
 * @var memory::is_string
 * True when the descriptor is currently treated as a string descriptor.
 * False when the descriptor is treated as generic data.
 *
 * @var memory::data
 * Pointer to the beginning of the allocated block (or NULL if none).
 * A valid descriptor must not combine `data == NULL` with `length > 0` or with
 * `actually_allocated_bytes > 0`.
 */
typedef struct memory {
	size_t single_element_size;
	size_t actually_allocated_bytes;
	size_t length;
	size_t string_length;
	bool is_string;
	void *data;
} memory;

/**
 * @brief Resize behavior flags for @ref mem_resize and @ref m_resize.
 *
 * These masks can be combined to tune how @ref m_resize behaves:
 * - `ZERO_NEW_MEMORY` mirrors `calloc` semantics by clearing any bytes that
 *   become newly addressable when a descriptor grows.
 * - `RELEASE_UNUSED` instructs the helper to release excess capacity immediately
 *   when the requested length decreases, instead of holding on to the buffer.
 *
 * Flags may be OR-ed together (for example, `ZERO_NEW_MEMORY | RELEASE_UNUSED`)
 * so that both behaviors are enabled during one call.
 */
typedef enum RESIZEMODES : unsigned int
{
	ZERO_NEW_MEMORY = 0x01u,
	RELEASE_UNUSED = 0x02u
} RESIZEMODES;

/**
 * @brief Descriptor initialization modes for @ref m_init and @ref m_init_static
 *
 * `MEMORY_DATA` initializes a generic data descriptor. `MEMORY_STRING`
 * initializes a descriptor whose subsequent operations should use string
 * semantics until code explicitly switches the descriptor back to data mode
 */
typedef enum MEMORYMODE : unsigned int
{
	MEMORY_DATA = 0u,
	MEMORY_STRING = 1u
} MEMORYMODE;

/**
 * @brief Terminator policy for direct string-buffer write finalization
 *
 * Both values guarantee that the descriptor is zero-terminated at
 * `written_length` on return. They only choose how the terminator is
 * produced:
 * `WRITE_TERMINATOR_IF_MISSING` first inspects the element at `written_length`
 * and writes a zero terminator only when one is not already present, so a
 * terminator that the caller has already materialized is left untouched.
 * `WRITE_TERMINATOR_ALWAYS` overwrites that slot with a zero terminator
 * unconditionally, regardless of its previous contents
 */
typedef enum TERMINATOR_WRITE_MODE : unsigned int
{
	WRITE_TERMINATOR_IF_MISSING = 0u,
	WRITE_TERMINATOR_ALWAYS = 1u
} TERMINATOR_WRITE_MODE;

Return mem_resize(
	memory *,
	size_t,
	RESIZEMODES);

Return mem_delete(memory *);

Return mem_copy(
	memory *,
	const memory *);

Return mem_concat_data(
	memory *,
	const memory *);

Return mem_copy_data(
	memory *,
	const memory *);

Return mem_concat_buffer(
	memory *,
	const size_t,
	const void *const);

Return mem_copy_buffer(
	memory *,
	const size_t,
	const void *const);

Return mem_guarded_byte_size(
	const memory *,
	size_t,
	size_t *);

Return mem_guarded_add(
	size_t,
	size_t,
	size_t *);

Return mem_guarded_subtract(
	size_t,
	size_t,
	size_t *);

/*
 * Low-level and internal-facing helpers.
 * Normal user code should prefer the m_* entry points and wrappers below.
 */
Return mem_concat_bounded_string(
	memory *,
	const size_t,
	const void *const);

Return mem_concat_unbounded_string(
	memory *,
	const void *const);

Return mem_copy_bounded_string(
	memory *,
	const size_t,
	const void *const);

Return mem_copy_unbounded_string(
	memory *,
	const void *const);

Return mem_copy_fixed_string(
	memory *,
	const size_t,
	const void *const);

Return mem_string_array_append_bounded(
	memory *,
	size_t,
	const size_t,
	const void *const);

Return mem_string_array_append_unbounded(
	memory *,
	size_t,
	const void *const);

Return mem_array_delete(memory *);

Return mem_finalize_string(
	memory *,
	size_t,
	TERMINATOR_WRITE_MODE);

Return mem_string_truncate(
	memory *,
	size_t);

Return mem_concat_fixed_string(
	memory *,
	const size_t,
	const void *const);

Return mem_concat_strings(
	memory *,
	const memory *);

Return mem_formatted_string(
	memory *,
	const void *const,
	...);

Return mem_string_length(
	const memory *,
	size_t *);

const void *mem_string(const memory *);

Return mem_convert_data_to_string(memory *);

Return mem_convert_string_to_data(memory *);

void *mem_data_writable(
	memory *,
	size_t);

const void *mem_data_readonly(
	const memory *,
	size_t);

void *mem_array_item_writable(
	memory *,
	size_t,
	size_t);

const void *mem_array_item_readonly(
	const memory *,
	size_t,
	size_t);

void *mem_raw_data_writable(memory *);

const void *mem_raw_data_readonly(const memory *);

#define mem_init(element_size,memory_mode) \
	((memory){ \
		.single_element_size = (element_size), \
		.actually_allocated_bytes = 0, \
		.length = 0, \
		.string_length = 0, \
		.is_string = ((MEMORYMODE)(memory_mode)) == MEMORY_STRING, \
		.data = NULL \
	})

#define m_init(T,...) \
	mem_init(sizeof(T),(MEMORYMODE)(MEMORY_DATA __VA_OPT__(| (__VA_ARGS__))))

#define m_init_static(T,...) \
	{ \
		.single_element_size = sizeof(T), \
		.actually_allocated_bytes = 0, \
		.length = 0, \
		.string_length = 0, \
		.is_string = ((MEMORYMODE)(MEMORY_DATA __VA_OPT__(| (__VA_ARGS__)))) == MEMORY_STRING, \
		.data = NULL \
	}

#define m_create(T,variable_name,...) \
	memory _ ## variable_name = m_init(T __VA_OPT__(,) __VA_ARGS__); \
	memory *variable_name = &_ ## variable_name

#define m_resize(descriptor,number_of_elements,...) \
	mem_resize((descriptor),(number_of_elements), \
	(RESIZEMODES)(0 __VA_OPT__(| (__VA_ARGS__))))

#define m_del(descriptor) \
	mem_delete((descriptor))

#define m_data(T,descriptor) \
	((T *)mem_data_writable((descriptor),sizeof(T)))

#define m_raw_data(descriptor) \
	mem_raw_data_writable((descriptor))

#define m_data_ro(T,descriptor) \
	((const T *)mem_data_readonly((descriptor),sizeof(T)))

#define m_item(T,descriptor,index) \
	((T *)mem_array_item_writable((descriptor),(size_t)(index),sizeof(T)))

#define m_item_ro(T,descriptor,index) \
	((const T *)mem_array_item_readonly((descriptor),(size_t)(index),sizeof(T)))

#define m_raw_data_ro(descriptor) \
	mem_raw_data_readonly((descriptor))

#define m_string(descriptor) \
	mem_string((descriptor))

/**
 * @brief Thin `const char *` wrapper over @ref m_string
 *
 * Use this wrapper when the caller already knows that the descriptor stores a
 * byte-sized string and needs a `const char *` result instead of the generic
 * `const void *` that @ref m_string returns. The macro adds no extra runtime
 * checks and keeps the full soft-access contract of @ref m_string unchanged
 */
#define m_text(descriptor) \
	((const char *)m_string((descriptor)))

#define m_copy(destination,source) \
	mem_copy((destination),(source))

#define m_concat_data(destination,source) \
	mem_concat_data((destination),(source))

#define m_concat_buffer(destination,source_buffer_size_bytes,source_buffer) \
	mem_concat_buffer((destination),(source_buffer_size_bytes),(source_buffer))

#define m_copy_data(destination,source) \
	mem_copy_data((destination),(source))

#define m_copy_buffer(destination,source_buffer_size_bytes,source_buffer) \
	mem_copy_buffer((destination),(source_buffer_size_bytes),(source_buffer))

#define m_guarded_byte_size(memory_structure,element_count,size_in_bytes) \
	mem_guarded_byte_size((memory_structure),(element_count),(size_in_bytes))

#define m_guarded_add(left,right,sum) \
	mem_guarded_add((left),(right),(sum))

#define m_guarded_subtract(left,right,difference) \
	mem_guarded_subtract((left),(right),(difference))

#define m_concat_fixed_string(destination,source_size_bytes,source) \
	mem_concat_fixed_string((destination),(source_size_bytes),(source))

/**
 * @def m_concat_literal(destination,source)
 * @brief Convenience wrapper around @ref mem_concat_fixed_string for C string literals
 *
 * Computes the source size automatically via `sizeof` and forwards the call to
 * @ref mem_concat_fixed_string. Use this macro whenever you append a literal in
 * place — it is shorter than the explicit form and removes the risk of size
 * arithmetic mistakes
 *
 * @par Example
 * @code
 * m_concat_literal(title," world"); // equivalent to m_concat_fixed_string(title,sizeof(" world")," world")
 * @endcode
 *
 * @param destination Destination string descriptor
 * @param source String literal to append
 */
#define m_concat_literal(destination,source) \
	mem_concat_fixed_string((destination),sizeof("" source ""),"" source "")

#define m_concat_string_2(destination,source) \
	mem_concat_unbounded_string((destination),(source))

#define m_concat_string_3(destination,source_limit_bytes,source) \
	mem_concat_bounded_string((destination),(source_limit_bytes),(source))

#define m_concat_string_GET(_1,_2,_3,NAME,...) NAME

/**
 * @def m_concat_string(...)
 * @brief Append a zero-terminated source string with automatic mode selection
 *
 * Resolves to @ref mem_concat_unbounded_string when called with two arguments
 * `(destination, source)`, and to @ref mem_concat_bounded_string when called
 * with three arguments `(destination, source_limit_bytes, source)`. Both variants
 * actively scan the source for a zero terminator
 *
 * Choose the two-argument form only when the source is guaranteed to be a
 * real C-string. Prefer the three-argument form when an upper bound on the
 * source byte limit is known but a terminator is not guaranteed
 */
#define m_concat_string(...) \
	m_concat_string_GET(__VA_ARGS__,m_concat_string_3,m_concat_string_2)(__VA_ARGS__)

#define m_concat_strings(destination,source) \
	mem_concat_strings((destination),(source))

#define m_formatted_string(destination,source_string,...) \
	mem_formatted_string((destination),(source_string) __VA_OPT__(,) __VA_ARGS__)

#define m_copy_fixed_string(destination,source_size_bytes,source) \
	mem_copy_fixed_string((destination),(source_size_bytes),(source))

#define m_string_array_append_3(descriptor_array,element_type,source_text) \
	mem_string_array_append_unbounded((descriptor_array),sizeof(element_type),(source_text))

#define m_string_array_append_4(descriptor_array,element_type,source_limit_bytes,source_text) \
	mem_string_array_append_bounded((descriptor_array),sizeof(element_type),(source_limit_bytes),(source_text))

#define m_string_array_append_GET(_1,_2,_3,_4,NAME,...) NAME

#define m_string_array_append(...) \
	m_string_array_append_GET(__VA_ARGS__,m_string_array_append_4,m_string_array_append_3)(__VA_ARGS__)

#define m_array_del(descriptor_array) \
	mem_array_delete((descriptor_array))

/**
 * @def mem_core_array_foreach(descriptor_array,element_type,element)
 * @brief Iterate over a descriptor-backed array with writable typed element pointers
 *
 * Maps @p descriptor_array through @ref m_data and captures the initial logical
 * length before the first iteration. The loop body may modify the current
 * element, but must not resize or delete the root descriptor because that can
 * invalidate the cached element and end pointers
 *
 * @param descriptor_array Root descriptor that stores elements of @p element_type
 * @param element_type Element type stored in the root descriptor
 * @param element Loop variable name. The macro declares it as `element_type *`
 */
#define mem_core_array_foreach(descriptor_array,element_type,element) \
	for(element_type *element = m_data(element_type,(descriptor_array)); \
		element != NULL; \
		element = NULL) \
	for(size_t element ## _mem_core_array_index = 0, \
		element ## _mem_core_array_count = (descriptor_array)->length; \
		element ## _mem_core_array_index < element ## _mem_core_array_count; \
		++element,++element ## _mem_core_array_index)

/**
 * @def m_string_array_foreach(descriptor_array,string_descriptor)
 * @brief Iterate over inline string descriptors stored by @ref m_string_array_append
 *
 * This thin wrapper specializes @ref mem_core_array_foreach for descriptor-backed
 * string arrays whose root data descriptor stores inline @ref memory elements
 *
 * @param descriptor_array Root descriptor created as `m_init(memory)`
 * @param string_descriptor Loop variable declared as `memory *`
 */
#define m_string_array_foreach(descriptor_array,string_descriptor) \
	mem_core_array_foreach((descriptor_array),memory,string_descriptor)

/**
 * @def m_copy_literal(destination,source)
 * @brief Convenience wrapper around @ref mem_copy_fixed_string for C string literals
 *
 * Computes the source size automatically via `sizeof` and forwards the call to
 * @ref mem_copy_fixed_string. Use this macro whenever you replace destination
 * contents with a literal in place — it is shorter than the explicit form and
 * removes the risk of size arithmetic mistakes
 *
 * @par Example
 * @code
 * m_copy_literal(title,"draft"); // equivalent to m_copy_fixed_string(title,sizeof("draft"),"draft")
 * @endcode
 *
 * @param destination Destination string descriptor
 * @param source String literal to copy
 */
#define m_copy_literal(destination,source) \
	mem_copy_fixed_string((destination),sizeof("" source ""),"" source "")

#define m_copy_string_2(destination,source) \
	mem_copy_unbounded_string((destination),(source))

#define m_copy_string_3(destination,source_limit_bytes,source) \
	mem_copy_bounded_string((destination),(source_limit_bytes),(source))

#define m_copy_string_GET(_1,_2,_3,NAME,...) NAME

/**
 * @def m_copy_string(...)
 * @brief Replace destination contents with a source string, with automatic mode selection
 *
 * Resolves to @ref mem_copy_unbounded_string when called with two arguments
 * `(destination, source)`, and to @ref mem_copy_bounded_string when called
 * with three arguments `(destination, source_limit_bytes, source)`. Both variants
 * actively scan the source for a zero terminator
 *
 * Choose the two-argument form only when the source is guaranteed to be a
 * real C-string. Prefer the three-argument form when an upper bound on the
 * source byte limit is known but a terminator is not guaranteed
 */
#define m_copy_string(...) \
	m_copy_string_GET(__VA_ARGS__,m_copy_string_3,m_copy_string_2)(__VA_ARGS__)

#define m_finalize_string_2(destination,written_length) \
	mem_finalize_string((destination),(written_length),WRITE_TERMINATOR_IF_MISSING)

#define m_finalize_string_3(destination,written_length,flags) \
	mem_finalize_string((destination),(written_length),(flags))

#define m_finalize_string_GET(_1,_2,_3,NAME,...) NAME

#define m_finalize_string(...) \
	m_finalize_string_GET(__VA_ARGS__,m_finalize_string_3,m_finalize_string_2)(__VA_ARGS__)

void mem_free_and_reset(void **);

#define m_reset(pointer) \
	mem_free_and_reset((void **)(pointer))

#define m_string_length(descriptor,length_out) \
	mem_string_length((descriptor),(length_out))

/**
 * @def m_string_truncate(descriptor,visible_length)
 * @brief Public alias for @ref mem_string_truncate
 *
 * Shrinks the visible part of a string descriptor down to
 * @p visible_length elements, writes a fresh terminator at that boundary,
 * and updates the cached `string_length`. The total `length` of the
 * descriptor is preserved, so any reserved tail stays available for future
 * appends without reallocation
 *
 * Useful after direct buffer edits via @ref m_data, when the new visible
 * length differs from the cached one. Together with a manually written
 * `'\0'`, you can use @ref m_finalize_string instead — pick the helper that
 * matches whether you already wrote the terminator yourself
 */
#define m_string_truncate(descriptor,visible_length) \
	mem_string_truncate((descriptor),(visible_length))

#define m_to_string(descriptor) \
	mem_convert_data_to_string((descriptor))

#define m_to_data(descriptor) \
	mem_convert_string_to_data((descriptor))

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
 *   m_create(point,points);                 // declare + initialize descriptor
 *   if((TRIUMPH & m_resize(points,10)) == 0) { return 1; }
 *   point *p = m_data(point,points);        // checked typed access
 *   p[0] = (point){1,2};
 *   if((TRIUMPH & m_resize(points,20)) == 0) { return 1; }
 *   p = m_data(point,points);               // refresh pointer after resizing
 *   m_del(points);
 *   return 0;
 * }
 * @endcode
 *
 * @section mem_usage_notes Notes & Recommendations
 * - Always refresh any cached typed pointer after a successful @ref m_resize.
 * - Most examples in this guide check `TRIUMPH`. If your code only cares about hard failures, checking `CRITICAL` is also valid
 * - Consider wrapping error codes with your project’s error-handling utilities.
 * - `m_resize(...,0)` clears the logical contents but may keep reserved
 *   capacity. Use @ref m_del when the block must be physically released.
 * - Always go through the helper macros so pointer conversions stay explicit and safe.
 */
#endif /* MEM_H */
