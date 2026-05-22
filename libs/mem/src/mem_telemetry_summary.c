#include "mem.h"

/**
 * @brief Print live-memory balance counters
 *
 * @param buf Numeric formatting buffer
 * @param byte_size_buffer Byte-size formatting buffer
 */
static void telemetry_summary_print_memory_balance(
	char *buf,
	char *byte_size_buffer)
{
	printf(BOLD "Memory balance" RESET "\n");
	printf(GRAY "Shows what is still alive at shutdown. In a clean run, reserved memory, useful payload, and active descriptors should all return to zero.\n" RESET WHITE);

	printf("Current heap reserved bytes (expected 0B), now is: ");
	if(telemetry.current_heap_reserved_bytes == 0)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.current_heap_reserved_bytes,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));

	printf("Current payload bytes (expected 0B), now is: ");
	if(telemetry.current_payload_bytes == 0)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.current_payload_bytes,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));

	printf("Current active descriptors (expected 0), now is: ");
	if(telemetry.current_active_descriptors == 0)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",form(telemetry.current_active_descriptors,buf,FORM_OUTPUT_BUFFER_SIZE));
}

/**
 * @brief Print heap reserve and release counters
 *
 * @param buf Numeric formatting buffer
 * @param byte_size_buffer Byte-size formatting buffer
 * @param expected_buf Byte-size formatting buffer for expected values
 * @param now_buf Byte-size formatting buffer for current values
 */
static void telemetry_summary_print_heap_reserve(
	char *buf,
	char *byte_size_buffer,
	char *expected_buf,
	char *now_buf)
{
	printf("\n" BOLD "Heap reserve" RESET "\n");
	printf(GRAY "Shows how much memory was reserved from the system allocator and how much was returned. Reserved memory can be larger than useful data because memory is kept in chunks.\n" RESET WHITE);

	printf("Peak heap reserved bytes");
	if(telemetry.total_heap_reserved_bytes_acquired == 0)
	{
		printf(" (expected 0B), now is: ");
		if(telemetry.peak_heap_reserved_bytes == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.peak_heap_reserved_bytes > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.peak_heap_reserved_bytes,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));

	printf("Total heap reserved bytes acquired");
	if(telemetry.fresh_heap_allocations == 0)
	{
		printf(" (expected 0B), now is: ");
		if(telemetry.total_heap_reserved_bytes_acquired == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.total_heap_reserved_bytes_acquired > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.total_heap_reserved_bytes_acquired,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));

	printf("Total heap reserved bytes released (expected %s), now is: ",bkbmbgbtbpbeb_r(telemetry.total_heap_reserved_bytes_acquired,FULL_VIEW,expected_buf,MAX_CHARACTERS));
	if(telemetry.total_heap_reserved_bytes_released == telemetry.total_heap_reserved_bytes_acquired)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.total_heap_reserved_bytes_released,FULL_VIEW,now_buf,MAX_CHARACTERS));

	printf("Fresh heap allocations");
	if(telemetry.total_heap_reserved_bytes_acquired == 0)
	{
		printf(" (expected 0), now is: ");
		if(telemetry.fresh_heap_allocations == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.fresh_heap_allocations > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",form(telemetry.fresh_heap_allocations,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Heap reallocations: %s\n",form(telemetry.heap_reallocations,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Heap buffer releases (expected %zu), now is: ",telemetry.fresh_heap_allocations);
	if(telemetry.heap_buffer_releases == telemetry.fresh_heap_allocations)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",form(telemetry.heap_buffer_releases,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Heap allocation failures");
	if(telemetry.expected_heap_allocation_failures == 0)
	{
		printf(" (expected 0), now is: ");
	} else {
		printf(" (normal 0, expected here %s), now is: ",form(telemetry.expected_heap_allocation_failures,buf,FORM_OUTPUT_BUFFER_SIZE));
	}
	if(telemetry.heap_allocation_failures == telemetry.expected_heap_allocation_failures)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",form(telemetry.heap_allocation_failures,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Heap reallocation failures");
	if(telemetry.expected_heap_reallocation_failures == 0)
	{
		printf(" (expected 0), now is: ");
	} else {
		printf(" (normal 0, expected here %s), now is: ",form(telemetry.expected_heap_reallocation_failures,buf,FORM_OUTPUT_BUFFER_SIZE));
	}
	if(telemetry.heap_reallocation_failures == telemetry.expected_heap_reallocation_failures)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",form(telemetry.heap_reallocation_failures,buf,FORM_OUTPUT_BUFFER_SIZE));
}

/**
 * @brief Print logical payload counters
 *
 * @param buf Numeric formatting buffer
 * @param byte_size_buffer Byte-size formatting buffer
 */
static void telemetry_summary_print_payload(
	char *buf,
	char *byte_size_buffer)
{
	printf("\n" BOLD "Payload" RESET "\n");
	printf(GRAY "Shows the useful data area requested by the program. Spare room kept for future growth is counted separately.\n" RESET WHITE);

	printf("Total payload bytes added");
	if(telemetry.fresh_heap_allocations == 0)
	{
		printf(" (expected 0B), now is: ");
		if(telemetry.total_payload_bytes_added == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.total_payload_bytes_added > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.total_payload_bytes_added,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));

	printf("Zero-initialized payload growths: %s\n",form(telemetry.zero_initialized_payload_growths,buf,FORM_OUTPUT_BUFFER_SIZE));
}

/**
 * @brief Print release-unused counters
 *
 * @param buf Numeric formatting buffer
 * @param byte_size_buffer Byte-size formatting buffer
 */
static void telemetry_summary_print_release_unused(
	char *buf,
	char *byte_size_buffer)
{
	printf("\n" BOLD "Release-unused" RESET "\n");
	printf(GRAY "Shows how often spare memory was intentionally trimmed, and how much reserved memory was returned instead of being kept for reuse.\n" RESET WHITE);

	printf("Release-unused shrink operations");
	if(telemetry.total_release_unused_heap_reserved_bytes_released == 0)
	{
		printf(" (expected 0), now is: ");
		if(telemetry.release_unused_shrinks == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.release_unused_shrinks > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",form(telemetry.release_unused_shrinks,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Total release-unused heap reserved bytes released");
	if(telemetry.release_unused_shrinks == 0)
	{
		printf(" (expected 0B), now is: ");
		if(telemetry.total_release_unused_heap_reserved_bytes_released == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.total_release_unused_heap_reserved_bytes_released > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.total_release_unused_heap_reserved_bytes_released,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));
}

/**
 * @brief Print block-overhead counters
 *
 * @param byte_size_buffer Byte-size formatting buffer
 */
static void telemetry_summary_print_block_overhead(char *byte_size_buffer)
{
	printf("\n" BOLD "Block overhead" RESET "\n");
	printf(GRAY "Shows spare bytes inside reserved blocks. This is not leaked memory: it is unused capacity kept so nearby growth can often avoid another allocation.\n" RESET WHITE);

	printf("Current block overhead bytes (expected 0B), now is: ");
	if(telemetry.current_block_overhead_bytes == 0)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.current_block_overhead_bytes,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));

	printf("Peak block overhead bytes");
	if(telemetry.total_block_overhead_bytes_added == 0)
	{
		printf(" (expected 0B), now is: ");
		if(telemetry.peak_block_overhead_bytes == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.peak_block_overhead_bytes > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.peak_block_overhead_bytes,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));

	printf("Total block overhead bytes added");
	if(telemetry.peak_block_overhead_bytes == 0)
	{
		printf(" (expected 0B), now is: ");
		if(telemetry.total_block_overhead_bytes_added == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.total_block_overhead_bytes_added > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",bkbmbgbtbpbeb_r(telemetry.total_block_overhead_bytes_added,FULL_VIEW,byte_size_buffer,MAX_CHARACTERS));
}

/**
 * @brief Print resize-behavior counters
 *
 * @param buf Numeric formatting buffer
 */
static void telemetry_summary_print_resize_behavior(char *buf)
{
	printf("\n" BOLD "Resize behavior" RESET "\n");
	printf(GRAY "Shows how resize requests were handled: by reusing existing storage, changing the allocation, or noticing that the requested size was already current.\n" RESET WHITE);

	printf("In-place resizes: %s\n",form(telemetry.in_place_resizes,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("No-op resizes");
	if(telemetry.peak_consecutive_noop_resizes == 0)
	{
		printf(" (expected 0), now is: ");
		if(telemetry.noop_resizes == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.noop_resizes > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",form(telemetry.noop_resizes,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Current consecutive no-op resizes (expected 0), now is: ");
	if(telemetry.current_consecutive_noop_resizes == 0)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",form(telemetry.current_consecutive_noop_resizes,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Peak consecutive no-op resizes");
	if(telemetry.noop_resizes == 0)
	{
		printf(" (expected 0), now is: ");
		if(telemetry.peak_consecutive_noop_resizes == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.peak_consecutive_noop_resizes > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",form(telemetry.peak_consecutive_noop_resizes,buf,FORM_OUTPUT_BUFFER_SIZE));
}

/**
 * @brief Print string and mode conversion counters
 *
 * @param buf Numeric formatting buffer
 */
static void telemetry_summary_print_string_and_mode_conversions(char *buf)
{
	printf("\n" BOLD "String and mode conversions" RESET "\n");
	printf(GRAY "Shows string maintenance work, such as writing terminators, finalizing manually written strings, and switching storage between raw data and text.\n" RESET WHITE);

	printf("Data-to-string conversions: %s\n",form(telemetry.data_to_string_conversions,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("String-to-data conversions: %s\n",form(telemetry.string_to_data_conversions,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Finalize string terminator already present: %s\n",form(telemetry.finalize_string_terminator_already_present,buf,FORM_OUTPUT_BUFFER_SIZE));

	printf("Finalize string terminator written when missing");
	if(telemetry.string_terminator_writes == 0)
	{
		printf(" (expected 0), now is: ");
		if(telemetry.finalize_string_terminator_written_when_missing == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
		printf("%s" RESET WHITE "\n",form(telemetry.finalize_string_terminator_written_when_missing,buf,FORM_OUTPUT_BUFFER_SIZE));
	} else {
		printf(": %s\n",form(telemetry.finalize_string_terminator_written_when_missing,buf,FORM_OUTPUT_BUFFER_SIZE));
	}

	printf("String terminator writes");
	if(telemetry.finalize_string_terminator_written_when_missing > 0)
	{
		printf(" (expected > 0), now is: ");
		if(telemetry.string_terminator_writes > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
		printf("%s" RESET WHITE "\n",form(telemetry.string_terminator_writes,buf,FORM_OUTPUT_BUFFER_SIZE));
	} else {
		printf(": %s\n",form(telemetry.string_terminator_writes,buf,FORM_OUTPUT_BUFFER_SIZE));
	}
}

/**
 * @brief Print active-descriptor counters
 *
 * @param buf Numeric formatting buffer
 */
static void telemetry_summary_print_descriptor_activity(char *buf)
{
	printf("\n" BOLD "Descriptor activity" RESET "\n");
	printf(GRAY "Shows the highest number of heap-owning descriptors alive at the same time.\n" RESET WHITE);

	printf("Peak active descriptors");
	if(telemetry.fresh_heap_allocations == 0)
	{
		printf(" (expected 0), now is: ");
		if(telemetry.peak_active_descriptors == 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	} else {
		printf(" (expected > 0), now is: ");
		if(telemetry.peak_active_descriptors > 0)
		{
			printf(BOLDGREEN);
		} else {
			printf(BOLDRED);
		}
	}
	printf("%s" RESET WHITE "\n",form(telemetry.peak_active_descriptors,buf,FORM_OUTPUT_BUFFER_SIZE));
}

/**
 * @brief Print safety counters
 *
 * @param buf Numeric formatting buffer
 */
static void telemetry_summary_print_safety(char *buf)
{
	printf("\n" BOLD "Safety" RESET "\n");
	printf(GRAY "Shows size-calculation problems caught before they could lead to unsafe memory work.\n" RESET WHITE);

	printf("Arithmetic guard failures");
	if(telemetry.expected_arithmetic_guard_failures == 0)
	{
		printf(" (expected 0), now is: ");
	} else {
		printf(" (normal 0, expected here %s), now is: ",form(telemetry.expected_arithmetic_guard_failures,buf,FORM_OUTPUT_BUFFER_SIZE));
	}
	if(telemetry.arithmetic_guard_failures == telemetry.expected_arithmetic_guard_failures)
	{
		printf(BOLDGREEN);
	} else {
		printf(BOLDRED);
	}
	printf("%s" RESET WHITE "\n",form(telemetry.arithmetic_guard_failures,buf,FORM_OUTPUT_BUFFER_SIZE));
}

void telemetry_summary(void)
{
	char buf[FORM_OUTPUT_BUFFER_SIZE];
	char byte_size_buffer[MAX_CHARACTERS];
	char expected_buf[MAX_CHARACTERS];
	char now_buf[MAX_CHARACTERS];

	telemetry_summary_print_memory_balance(buf,byte_size_buffer);
	telemetry_summary_print_heap_reserve(buf,byte_size_buffer,expected_buf,now_buf);
	telemetry_summary_print_payload(buf,byte_size_buffer);
	telemetry_summary_print_release_unused(buf,byte_size_buffer);
	telemetry_summary_print_block_overhead(byte_size_buffer);
	telemetry_summary_print_resize_behavior(buf);
	telemetry_summary_print_string_and_mode_conversions(buf);
	telemetry_summary_print_descriptor_activity(buf);
	telemetry_summary_print_safety(buf);
}
