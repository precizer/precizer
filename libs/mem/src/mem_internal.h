#ifndef MEM_INTERNAL_H
#define MEM_INTERNAL_H

#include "mem.h"

/**
 * @brief Internal zero-terminator helpers shared by string paths
 *
 * These declarations cover the low-level helpers that answer two practical
 * questions for byte strings and wider string element types alike:
 * "is this logical element a terminator" and "write one terminator here"
 */
bool mem_is_zero_element(
	const unsigned char *element_view,
	size_t              single_element_size);

Return mem_string_measure_length(
	const void *source_string,
	size_t     source_limit_bytes,
	size_t     single_element_size,
	bool       source_limit_is_active,
	size_t     *length_out,
	bool       *terminator_found_out);

Return mem_write_zero_terminator(
	memory *memory_structure,
	size_t terminator_index);

/**
 * @brief Internal binary mode flags shared by append and copy cores
 *
 * Source flags describe how string cores interpret the source payload.
 * Transfer flags describe whether the visible payload is appended or replaces
 * the destination
 */
typedef enum MEM_CORE_MODE : unsigned int
{
	SOURCE_BOUNDED_STRING = 1u << 0,
	SOURCE_UNBOUNDED_STRING = 1u << 1,
	SOURCE_FIXED_STRING = 1u << 2,
	TRANSFER_APPEND = 1u << 3,
	TRANSFER_REPLACE = 1u << 4,
	SOURCE_MASK = SOURCE_BOUNDED_STRING | SOURCE_UNBOUNDED_STRING | SOURCE_FIXED_STRING, // Hex: 0x07. Dec: 7. Bin: 00111
	TRANSFER_MASK = TRANSFER_APPEND | TRANSFER_REPLACE // Hex: 0x18. Dec: 24. Bin: 11000
} MEM_CORE_MODE;

/**
 * @brief Shared internal string-transfer core for append and copy entry points
 *
 * Public wrappers route here after choosing how the source should be measured
 * and whether the visible payload should be appended or should replace the
 * destination. The detailed behavior is documented in the implementation file
 */
Return mem_core_string(
	const MEM_CORE_MODE mode,
	memory              *destination,
	const size_t        source_range_bytes,
	const void *const   source_string);

/**
 * @brief Shared internal append core for arrays of inline string descriptors
 *
 * Public string-array append wrappers route here after deciding whether the
 * source should be interpreted as bounded or unbounded string input. This
 * helper validates the root descriptor, grows it by one inline `memory`
 * element, initializes that new slot as a string descriptor, and delegates the
 * actual copy to the matching public string-copy backend
 */
Return mem_string_array_core(
	MEM_CORE_MODE     source_mode,
	memory            *descriptor_array,
	size_t            single_element_size,
	size_t            source_limit_bytes,
	const void *const source_string);

/**
 * @brief Shared internal data-descriptor core for append and copy entry points
 *
 * Public or internal descriptor-facing data helpers can route here after they
 * decide whether the source payload should be appended or should replace the
 * destination. This helper treats both descriptors as raw byte containers and
 * only requires that the source byte count cleanly fits the destination
 * element size so no partial destination element remains
 */
Return mem_core_data(
	const MEM_CORE_MODE mode,
	memory              *destination,
	const memory        *source);

/**
 * @brief Shared internal raw-buffer core for append and copy entry points
 *
 * Public data-mode wrappers route here after deciding whether the exact source
 * byte range should be appended or should replace the destination payload
 */
Return mem_core_buffer(
	const MEM_CORE_MODE mode,
	memory              *destination,
	const size_t        source_buffer_size_bytes,
	const void *const   source_buffer);

/**
 * @brief Internal bounded terminator search helper
 *
 * Callers use this declaration when they need the first zero-valued logical
 * element inside the current descriptor bounds. The full behavioral contract
 * is documented in the corresponding implementation file
 */
Return mem_find_zero_terminator(
	const memory *memory_structure,
	size_t       *terminator_position_out);
#endif /* MEM_INTERNAL_H */
