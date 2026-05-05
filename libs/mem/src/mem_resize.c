#include "mem.h"
#include "mem_internal.h"
#include <string.h>

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

/**
 * @brief Resize a descriptor in logical elements while preserving its current mode
 *
 * Use this helper when a descriptor needs more or less logical room. The new
 * size is measured in elements, not in bytes, so `m_resize(points,10)` means
 * ten `point` objects, while `m_resize(text,10)` means ten characters or code
 * units
 *
 * In data mode the helper treats the descriptor as plain raw storage. In that
 * mode `string_length` must already be `0`, and every successful resize writes
 * `0` there again so raw buffers keep clean non-string metadata. In string
 * mode the helper preserves string mode and never invents new visible content
 * when capacity grows. After a successful resize the element at index
 * @ref memory::string_length is guaranteed to be a zero-valued terminator, so
 * the buffer is always a valid C string up to that point. Bytes that lie
 * beyond the terminator are not initialized by this function — pass
 * @ref ZERO_NEW_MEMORY if newly exposed bytes should be cleared. If the new
 * logical size leaves less room than the current visible payload, the visible
 * string is truncated to fit and a fresh terminator is written. Descriptors
 * whose reserved byte count no longer covers the current logical payload are
 * rejected instead of being treated as successful no-ops. In this mode
 * `string_length` describes only the visible string prefix, while `length`
 * describes the logical descriptor span in elements. For a tightly packed
 * string they usually differ by one because the terminator occupies the extra
 * slot, but they may differ by much more after proactive reserve growth. For
 * example, a descriptor that holds `"abc"` normally has `string_length == 3`
 * and `length == 4`; after `m_resize(text,64)` the visible text is still
 * `"abc"` and `string_length` stays `3`, while `length` becomes `64`
 *
 * Without @ref RELEASE_UNUSED the helper tries to reuse the current aligned
 * reserve whenever possible. That means a shrink may change only the logical
 * length while the underlying allocation stays in place for future growth.
 * With @ref RELEASE_UNUSED the helper is allowed to return spare aligned
 * capacity to the allocator during shrink. A resize to zero clears the logical
 * contents and may either keep or release the reserve, depending on that flag
 *
 * Supported behavior flags (combine with `|`):
 * - @ref ZERO_NEW_MEMORY zero-fills only the bytes that become newly
 *   addressable after growth, leaving previously reserved bytes untouched
 * - @ref RELEASE_UNUSED returns spare aligned capacity to the allocator on
 *   shrink, and physically frees the block on `m_resize(...,0,RELEASE_UNUSED)`
 *
 * Small example:
 * @code
 * m_create(char,greeting,MEMORY_STRING); // empty string descriptor
 * m_concat_literal(greeting,"Hi"); // length == 3, string_length == 2
 *
 * m_resize(greeting,32); // reserve more room
 * // length == 32, string_length == 2, visible payload still "Hi"
 *
 * m_concat_literal(greeting," there"); // appends without extra reallocation
 *
 * m_resize(greeting,0,RELEASE_UNUSED); // drop contents and release the buffer
 * @endcode
 *
 * @param memory_structure Descriptor to resize
 * @param new_count Requested logical size in elements
 * @param behavior_flags Resize behavior mask. Pass `0`, a single
 *        @ref RESIZEMODES flag, or any bitwise OR of flags
 * @return `SUCCESS` on success; `FAILURE` otherwise. Failures are reported through @ref report for easier diagnostics
 *
 * @pre @p memory_structure must be non-NULL and initialized
 *      (@ref memory::single_element_size must be non-zero)
 * @pre A valid descriptor must not advertise `length > 0` while
 *      @ref memory::data is `NULL`
 * @pre A valid descriptor must not advertise
 *      @ref memory::actually_allocated_bytes greater than `0` while
 *      @ref memory::data is `NULL`
 * @pre A valid descriptor must keep
 *      @ref memory::actually_allocated_bytes large enough to cover the
 *      current logical payload
 * @pre Data descriptors (`is_string == false`) must already have
 *      @ref memory::string_length equal to `0`
 *
 * @warning Any physical reallocation may move @ref memory::data, so refresh cached raw pointers after a successful resize
 */
Return mem_resize(
	memory *memory_structure,
	size_t new_count,
	RESIZEMODES behavior_flags)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Remembers whether bytes that become newly reachable after growth must be zero-filled */
	const bool zero_new_memory = (behavior_flags & ZERO_NEW_MEMORY) != 0;

	/* Remembers whether shrink operations may return spare aligned reserve to the allocator */
	const bool allow_shrink = (behavior_flags & RELEASE_UNUSED) != 0;

  if(memory_structure == NULL || memory_structure->single_element_size == 0)
  {
    report("Memory management; Descriptor is NULL or not initialized");
    provide(FAILURE);
  }

	if(memory_structure->length > 0 && memory_structure->data == NULL)
	{
		report("Memory management; Descriptor has non-zero length with NULL data pointer");
		provide(FAILURE);
	}

	if(memory_structure->is_string == false && memory_structure->string_length != 0)
	{
		report("Memory management; Data descriptor has non-zero string_length during resize");
		provide(FAILURE);
	}

	/* Byte size of the payload described by the descriptor before this resize starts */
	size_t previous_effective_bytes = 0;

  /* Total bytes the allocator says are currently reserved for the descriptor */
	size_t previous_allocated_bytes = 0;

  /* Element count visible to callers before any resize logic runs */
	size_t previous_length = 0;

  /* Old aligned slack beyond the logical payload, used only for telemetry accounting */
	size_t previous_alignment_overhead = 0;

  /* Byte size needed for the requested new_count payload */
	size_t total_size_in_bytes = 0;

  /* Cached visible string length captured before resize mutates any metadata */
	size_t previous_string_length = 0;

  previous_length = memory_structure->length;
  previous_allocated_bytes = memory_structure->actually_allocated_bytes;
  previous_string_length = memory_structure->string_length;

  run(mem_guarded_byte_size(memory_structure,previous_length,&previous_effective_bytes));

  if(previous_allocated_bytes > previous_effective_bytes)
  {
    run(mem_guarded_subtract(previous_allocated_bytes,
      previous_effective_bytes,
      &previous_alignment_overhead));
  }

	if((TRIUMPH & status) &&
		previous_allocated_bytes > 0 &&
		memory_structure->data == NULL)
	{
		report("Memory management; Descriptor has reserved bytes with NULL data pointer during resize");
		provide(FAILURE);
	}

	if((TRIUMPH & status) &&
		previous_length > 0 &&
		previous_allocated_bytes < previous_effective_bytes)
	{
		report("Memory management; Descriptor reserve is smaller than logical payload during resize");
		provide(FAILURE);
	}

	if((TRIUMPH & status) &&
		memory_structure->is_string == true &&
		previous_length > 0 &&
		previous_string_length >= previous_length)
	{
		report("Memory management; String descriptor cache is inconsistent during resize");
		provide(FAILURE);
	}

	if((TRIUMPH & status) &&
		memory_structure->is_string == true &&
		previous_length == 0 &&
		previous_string_length != 0)
	{
		report("Memory management; String descriptor has non-zero string_length with zero length during resize");
		provide(FAILURE);
	}

	if((TRIUMPH & status) && new_count == previous_length)
	{
		if(new_count != 0 || allow_shrink == false || previous_allocated_bytes == 0)
		{
			telemetry_realloc_noop_counter();
			telemetry_noop_resize_event();
			provide(status);
		}
	}

	telemetry_reset_noop_streak();

	if(TRIUMPH & status)
	{
		call(mem_guarded_byte_size(memory_structure,new_count,&total_size_in_bytes));
	}

	if(CRITICAL & status)
	{
		report("Memory management; Overflow for length=%zu (single_element_size=%zu)",new_count,memory_structure->single_element_size);
	}

	if(TRIUMPH & status)
	{
		if(new_count == 0)
		{
			if(allow_shrink == true && previous_allocated_bytes > 0)
			{
				telemetry_release_unused_operation();
				telemetry_release_unused_bytes(previous_allocated_bytes);

				run(mem_delete(memory_structure));
			} else {
				if(previous_effective_bytes > 0)
				{
					telemetry_effective_reduce(previous_effective_bytes);
					telemetry_alignment_overhead_add(previous_effective_bytes);
				}

				memory_structure->length = 0;
				memory_structure->string_length = 0;

				if(memory_structure->is_string == true &&
					memory_structure->data != NULL &&
					memory_structure->actually_allocated_bytes >= memory_structure->single_element_size)
				{
					run(mem_write_zero_terminator(memory_structure,0));
				}
			}
		} else {
			/* Requested payload rounded up to the allocator block size */
			size_t aligned_size_in_bytes = 0;

      if(align_to_block_boundary(total_size_in_bytes,&aligned_size_in_bytes) != 0)
      {
        report("Memory management; Allocation alignment overflow for %zu bytes",total_size_in_bytes);
        provide(FAILURE);
      } else {
				/* True when the descriptor has no allocation yet and needs its first buffer */
				const bool needs_fresh_allocation = memory_structure->data == NULL;
				/* True when the aligned target size is larger than the currently reserved block */
				const bool needs_growth = aligned_size_in_bytes > memory_structure->actually_allocated_bytes;
				/* True when RELEASE_UNUSED allows an immediate shrink of the reserved block */
				const bool should_shrink = (allow_shrink == true) &&
					aligned_size_in_bytes < memory_structure->actually_allocated_bytes;

				if((needs_fresh_allocation == true) || (needs_growth == true) || (should_shrink == true))
				{
					/* Result pointer returned by malloc()/realloc() before it is committed to the descriptor */
					void *resized_pointer = NULL;

					if(needs_fresh_allocation == true)
					{
						resized_pointer = malloc(aligned_size_in_bytes);
					} else {
						resized_pointer = realloc(memory_structure->data,aligned_size_in_bytes);
					}

					if(resized_pointer == NULL)
					{
						report("Memory management; Memory allocation failed for %zu bytes",aligned_size_in_bytes);
						status = FAILURE;

						if(needs_fresh_allocation == true)
						{
							telemetry_allocation_failure();
						} else {
							telemetry_reallocation_failure();
						}

					} else {
						memory_structure->data = resized_pointer;

						if(needs_fresh_allocation == true)
						{
							telemetry_active_descriptor_acquire();
							telemetry_new_allocations_counter();

							if(aligned_size_in_bytes > 0)
							{
								telemetry_add(aligned_size_in_bytes);
							}
						} else if(needs_growth == true){
							/* Number of newly reserved aligned bytes added by this growth step */
							size_t added_bytes = 0;

							telemetry_aligned_reallocations_counter();
							run(mem_guarded_subtract(aligned_size_in_bytes,
								previous_allocated_bytes,
								&added_bytes));

							if((TRIUMPH & status) && added_bytes > 0)
							{
								telemetry_add(added_bytes);
							}
						} else if(should_shrink == true){
							/* Number of aligned bytes returned to the allocator during forced shrink */
							size_t reclaimed_bytes = 0;

							telemetry_aligned_reallocations_counter();
							run(mem_guarded_subtract(previous_allocated_bytes,
								aligned_size_in_bytes,
								&reclaimed_bytes));

							if((TRIUMPH & status) && reclaimed_bytes > 0)
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
					/* Newly reachable payload bytes that must be cleared for ZERO_NEW_MEMORY */
					size_t bytes_to_zero = 0;

					if((zero_new_memory == true) && total_size_in_bytes > previous_effective_bytes)
					{
						run(mem_guarded_subtract(total_size_in_bytes,
							previous_effective_bytes,
							&bytes_to_zero));
					}

					if(bytes_to_zero > 0)
					{
						/* Writable byte view used by memset() when zero-filling fresh payload space */
						unsigned char *memory_structure_data_rewritable = (unsigned char *)memory_structure->data;

						if(memory_structure_data_rewritable == NULL)
						{
							report("Memory management; Data pointer is NULL during zero-fill");
							provide(FAILURE);
						} else {
							memset(memory_structure_data_rewritable + previous_effective_bytes,0,bytes_to_zero);
							telemetry_new_callocations_counter();
						}
					}

					if(TRIUMPH & status)
					{
						/* Change in logical payload bytes, used only for telemetry bookkeeping */
						size_t effective_delta = 0;

						if(total_size_in_bytes > previous_effective_bytes)
						{
							run(mem_guarded_subtract(total_size_in_bytes,
								previous_effective_bytes,
								&effective_delta));

							if((TRIUMPH & status) && effective_delta > 0)
							{
								telemetry_effective_add(effective_delta);
							}
						} else if(total_size_in_bytes < previous_effective_bytes){
							run(mem_guarded_subtract(previous_effective_bytes,
								total_size_in_bytes,
								&effective_delta));

							if((TRIUMPH & status) && effective_delta > 0)
							{
								telemetry_effective_reduce(effective_delta);
							}
						}
					}

					/* Growing a string changes only available capacity. The visible
					   payload is not rewritten here, so helper-driven normalization
					   may legitimately become a no-op and rely on the existing
					   zero terminator that all string mutators must preserve */
					if((TRIUMPH & status) && memory_structure->is_string == true)
					{
						/* Visible string length that should remain after resize finishes and terminator is normalized */
						size_t resulting_string_length = 0;

						if(previous_length > 0)
						{
							resulting_string_length = previous_string_length;

							if(resulting_string_length >= new_count)
							{
								resulting_string_length = new_count - 1;
							}
						}

						memory_structure->length = new_count;
						memory_structure->string_length = resulting_string_length;
						run(mem_string_truncate(memory_structure,resulting_string_length));
					} else if(TRIUMPH & status) {
						memory_structure->length = new_count;
						memory_structure->string_length = 0;
					}
				}
			}
		}
	}

	if((TRIUMPH & status) && new_count != 0)
	{
		/* Final reserve size after resize logic, used to recalculate alignment overhead */
		const size_t resulting_allocated_bytes = memory_structure->actually_allocated_bytes;
		/* New aligned slack beyond the logical payload after the resize completes */
		size_t new_alignment_overhead = 0;

		if(resulting_allocated_bytes > total_size_in_bytes)
		{
			run(mem_guarded_subtract(resulting_allocated_bytes,
				total_size_in_bytes,
				&new_alignment_overhead));
		}

		if(new_alignment_overhead > previous_alignment_overhead)
		{
			/* Difference between old and new alignment slack, used only for telemetry deltas */
			size_t alignment_delta = 0;

			run(mem_guarded_subtract(new_alignment_overhead,
				previous_alignment_overhead,
				&alignment_delta));

			if((TRIUMPH & status) && alignment_delta > 0)
			{
				telemetry_alignment_overhead_add(alignment_delta);
			}
		} else if(new_alignment_overhead < previous_alignment_overhead){
			/* Difference between old and new alignment slack, used only for telemetry deltas */
			size_t alignment_delta = 0;

			run(mem_guarded_subtract(previous_alignment_overhead,
				new_alignment_overhead,
				&alignment_delta));

			if((TRIUMPH & status) && alignment_delta > 0)
			{
				telemetry_alignment_overhead_reduce(alignment_delta);
			}
		}
	}

	provide(status);
}
