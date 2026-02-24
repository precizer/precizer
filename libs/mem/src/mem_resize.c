#include "mem.h"
#include <stdarg.h>
#include <string.h>
#include <limits.h>

/**
 * @brief Round a byte size up to the allocator block boundary.
 *
 * Aligns @p requested_bytes to the next multiple of @ref MEMORY_BLOCK_BYTES, preserving zero
 * and guarding against overflow. Returns non-zero on error so callers can log and fail early.
 *
 * @param requested_bytes Number of bytes requested by the caller.
 * @param aligned_bytes   Output pointer receiving the aligned size when successful.
 * @return 0 on success; non-zero if @p aligned_bytes is NULL or an overflow occurred.
 */
static int align_to_block_boundary(
	size_t requested_bytes,
	size_t *aligned_bytes)
{
	if(aligned_bytes == NULL)
	{
		return 1;
	}

	if(requested_bytes == 0)
	{
		*aligned_bytes = 0;
		return 0;
	}

	const size_t remainder = requested_bytes % MEMORY_BLOCK_BYTES;

	if(remainder == 0)
	{
		*aligned_bytes = requested_bytes;
		return 0;
	}

	const size_t padding = MEMORY_BLOCK_BYTES - remainder;

	if(requested_bytes > SIZE_MAX - padding)
	{
		return 1;
	}

	*aligned_bytes = requested_bytes + padding;
	return 0;
}

Return memory_resize(
	memory *memory_structure,
	size_t new_count,
	...)
{
	/** Return status
	 *  The status that will be passed to provide() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;
	unsigned char behavior_flags = 0U;

	va_list optional_arguments;
	va_start(optional_arguments,new_count);
	const unsigned int provided_flags = va_arg(optional_arguments,unsigned int);

	if(provided_flags == UCHAR_MAX)
	{
		behavior_flags = 0U;
	} else {
		behavior_flags = (unsigned char)provided_flags;
		const unsigned int terminator = va_arg(optional_arguments,unsigned int);

		if(terminator != UCHAR_MAX)
		{
			slog(ERROR,"Memory management; Resize flags terminator missing");
			status = FAILURE;
		}
	}
	va_end(optional_arguments);

	const bool zero_new_memory = (behavior_flags & ZERO_NEW_MEMORY) != 0U;
	const bool allow_shrink = (behavior_flags & RELEASE_UNUSED) != 0U;

	if(TRIUMPH & status)
	{
		if(memory_structure == NULL || memory_structure->element_size == 0)
		{
			slog(ERROR,"Memory management; Descriptor is NULL or not initialized");
			provide(FAILURE);
		}
	}

	size_t previous_effective_bytes = 0;
	size_t previous_allocated_bytes = 0;
	size_t previous_elements = 0;
	size_t previous_alignment_overhead = 0;
	size_t total_size_in_bytes = 0;

	if(TRIUMPH & status)
	{
		previous_elements = memory_structure->length;
		previous_allocated_bytes = memory_structure->actually_allocated_bytes;
		previous_effective_bytes = 0;

		run(memory_guarded_size(memory_structure->element_size,
			previous_elements,
			&previous_effective_bytes));

		if(previous_allocated_bytes > previous_effective_bytes)
		{
			previous_alignment_overhead = previous_allocated_bytes - previous_effective_bytes;
		}
	}

	if((TRIUMPH & status) && new_count == previous_elements)
	{
		if(new_count == 0)
		{
			if(memory_structure->data == NULL)
			{
				telemetry_realloc_noop_counter();
				telemetry_noop_resize_event();
				provide(status);
			}
		} else {
			telemetry_realloc_noop_counter();
			telemetry_noop_resize_event();
			provide(status);
		}
	}

	telemetry_reset_noop_streak();

	run(memory_guarded_size(memory_structure->element_size,new_count,&total_size_in_bytes));

	if(CRITICAL & status)
	{
		slog(ERROR,"Memory management; Overflow for length=%zu (element_size=%zu)",new_count,memory_structure->element_size);
	}

	if(TRIUMPH & status)
	{
		if(new_count == 0)
		{
			call(del(memory_structure));
		} else {
			size_t aligned_size_in_bytes = 0;

			if(align_to_block_boundary(total_size_in_bytes,&aligned_size_in_bytes) != 0)
			{
				slog(ERROR,"Memory management; Allocation alignment overflow for %zu bytes",total_size_in_bytes);
				status = FAILURE;
			} else {
				const bool needs_fresh_allocation = memory_structure->data == NULL;
				const bool needs_growth = aligned_size_in_bytes > memory_structure->actually_allocated_bytes;
				const bool should_shrink = allow_shrink &&
				        aligned_size_in_bytes < memory_structure->actually_allocated_bytes;

				if(needs_fresh_allocation || needs_growth || should_shrink)
				{
					void *resized_pointer = NULL;

					if(needs_fresh_allocation)
					{
						resized_pointer = malloc(aligned_size_in_bytes);
					} else {
						resized_pointer = realloc(memory_structure->data,aligned_size_in_bytes);
					}

					if(resized_pointer == NULL)
					{
						slog(ERROR,
							"Memory management; Memory allocation failed for %zu bytes",
							aligned_size_in_bytes);
						status = FAILURE;

						if(needs_fresh_allocation)
						{
							telemetry_allocation_failure();
						} else {
							telemetry_reallocation_failure();
						}
					} else {
						memory_structure->data = resized_pointer;

						if(needs_fresh_allocation)
						{
							telemetry_active_descriptor_acquire();
							telemetry_new_allocations_counter();

							if(aligned_size_in_bytes > 0)
							{
								telemetry_add(aligned_size_in_bytes);
							}
						} else if(needs_growth){
							telemetry_aligned_reallocations_counter();
							telemetry_add(aligned_size_in_bytes - previous_allocated_bytes);
						} else if(should_shrink){
							telemetry_aligned_reallocations_counter();
							const size_t reclaimed_bytes = previous_allocated_bytes - aligned_size_in_bytes;

							if(reclaimed_bytes > 0)
							{
								telemetry_release_unused_operation();
								telemetry_release_unused_bytes(reclaimed_bytes);
								telemetry_reduce(reclaimed_bytes);
								telemetry_free_total_bytes(reclaimed_bytes);
							}
						}

						memory_structure->actually_allocated_bytes = aligned_size_in_bytes;
					}
				} else {
					telemetry_realloc_optimized_counter();
				}

				if(TRIUMPH & status)
				{
					size_t bytes_to_zero = 0;

					if(zero_new_memory && total_size_in_bytes > previous_effective_bytes)
					{
						bytes_to_zero = total_size_in_bytes - previous_effective_bytes;
					}

					memory_structure->length = new_count;

					if(bytes_to_zero > 0)
					{
						unsigned char *byte_view = (unsigned char *)memory_structure->data;

						if(byte_view == NULL)
						{
							slog(ERROR,"Memory management; Data pointer is NULL during zero-fill");
							status = FAILURE;
						} else {
							memset(byte_view + previous_effective_bytes,0,bytes_to_zero);
							telemetry_new_callocations_counter();
						}
					}

					if(TRIUMPH & status)
					{
						if(total_size_in_bytes > previous_effective_bytes)
						{
							telemetry_effective_add(total_size_in_bytes - previous_effective_bytes);
						} else if(total_size_in_bytes < previous_effective_bytes){
							telemetry_effective_reduce(previous_effective_bytes - total_size_in_bytes);
						}
					}
				}
			}
		}
	}

	if((TRIUMPH & status) && new_count != 0)
	{
		const size_t resulting_allocated_bytes = memory_structure->actually_allocated_bytes;
		size_t new_alignment_overhead = 0;

		if(resulting_allocated_bytes > total_size_in_bytes)
		{
			new_alignment_overhead = resulting_allocated_bytes - total_size_in_bytes;
		}

		if(new_alignment_overhead > previous_alignment_overhead)
		{
			telemetry_alignment_overhead_add(new_alignment_overhead - previous_alignment_overhead);
		} else if(new_alignment_overhead < previous_alignment_overhead){
			telemetry_alignment_overhead_reduce(previous_alignment_overhead - new_alignment_overhead);
		}
	}

	provide(status);
}
