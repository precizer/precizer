#include "mem.h"
#include "mem_internal.h"

/**
 * @brief Compute the visible string length stored in a descriptor
 *
 * The library is being migrated to a unified string model where strings stored
 * in @ref memory descriptors may use arbitrary non-zero element widths instead
 * of being limited to byte-sized characters. Common representations include
 * `char`, `signed char`, `unsigned char`, `char8_t`, `wchar_t`, `char16_t`,
 * `char32_t`, and fixed-width code-unit storage such as `uint8_t`,
 * `uint16_t`, and `uint32_t`. String algorithms are defined in whole
 * elements, and a string terminator is an element whose bytes are all zero.
 *
 * The function accepts both cached string descriptors and generic data
 * descriptors. When @ref memory::is_string is `true`, the cached
 * @ref memory::string_length value is returned immediately without rescanning
 * the payload and without additional integrity checks. The cache is expected
 * to be computed and maintained by descriptor-mutating helpers, which carry
 * responsibility for the correctness of @ref memory::string_length so repeated
 * length queries stay cheap. When @ref memory::is_string is `false`, the
 * descriptor is scanned element by element until the first zero-valued element
 * or until @ref memory::length elements have been inspected. A zero-valued
 * element means that every byte in that element is zero. The reported length
 * is always measured in elements, not bytes
 *
 * Behavior details:
 * - Invalid arguments (`memory_structure == NULL` or `length_out == NULL`)
 *   return `FAILURE`.
 * - If @ref memory::single_element_size is 0, the function returns `FAILURE`.
 * - If @ref memory::length is 0, `*length_out` is set to 0.
 * - If @ref memory::data is `NULL` while @ref memory::length is also 0,
 *   `*length_out` is set to 0.
 * - If @ref memory::data is `NULL` while @ref memory::length is non-zero,
 *   the function returns `FAILURE` because the descriptor is inconsistent.
 * - When @ref memory::is_string is `true`, the cached
 *   @ref memory::string_length is returned immediately without extra integrity
 *   checks or recomputation.
 * - In data mode, `*length_out` receives the visible prefix length measured by
 *   a bounded scan up to the first zero-valued element or @ref memory::length
 *
 * Return-path details:
 * - The function returns via `provide(...)`.
 * - If global status (`global_return_status`) is not `SUCCESS`, the returned
 *   value may be overridden by that global status.
 * - For control-flow checks that treat graceful non-error statuses as
 *   acceptable, prefer `(status & TRIUMPH) != 0`
 *
 * @param memory_structure Descriptor whose contents are interpreted as a string
 * @param length_out Output pointer that receives the computed length
 * @return Local result is `SUCCESS`/`FAILURE`; final returned value is subject
 *         to `provide(...)` global-status propagation
 */
Return mem_string_length(
	const memory *memory_structure,
	size_t       *length_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(memory_structure == NULL || length_out == NULL)
	{
		report("Memory management; Invalid arguments for string length helper");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->length > 0 && memory_structure->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->single_element_size == 0)
	{
		report("Memory management; Descriptor element size is zero (uninitialized)");
		status = FAILURE;
	}

	if((TRIUMPH & status) && memory_structure->is_string == true)
	{
		*length_out = memory_structure->string_length;
	}

	if((TRIUMPH & status) && memory_structure->is_string == false)
	{
		run(mem_find_zero_terminator(memory_structure,length_out));
	}

	provide(status);
}
