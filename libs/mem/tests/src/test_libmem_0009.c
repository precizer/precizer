#include "test_libmem_utils.h"
#include "testmocking.h"

/* Shared descriptors of the telemetry suite.
   The descriptors live for the whole suite so each subtest observes the
   state left by the previous scenario and can keep advancing the same
   buffer through every supported transition */
static memory shared_buffer = m_init_static(unsigned char);
static memory shared_sentinel = m_init_static(uint32_t);
static memory *buffer = &shared_buffer;
static memory *sentinel = &shared_sentinel;

/* Suite-wide baseline of the global Telemetry struct, captured by the
   first subtest. Counters are global, and the testitall framework can
   also move them while clearing its STDOUT, STDERR, and EXTEND
   descriptors between subtests, so the suite asserts deltas relative
   to this baseline rather than absolute counter values */
static Telemetry suite_baseline;

/**
 * @brief Reinitialize the shared descriptors and snapshot the suite baseline
 *
 * Resets the shared buffer and sentinel descriptors to an empty state
 * through m_init so the shared descriptors enter the suite in a known
 * empty state. Captures the current Telemetry struct as suite_baseline
 * so every subsequent subtest can express its expectations as deltas
 * relative to a known starting point. The function deliberately does
 * not reset global telemetry because previous tests in the runner may
 * still own descriptors registered in current_active_descriptors and a
 * hard counter reset would underflow the counter on their later teardown.
 * The descriptor reset is plain assignment, not m_del, so it does not
 * recover any blocks left allocated by an aborted previous run of this
 * same suite
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_01(void)
{
	INITTEST;

	shared_buffer = m_init(unsigned char);
	shared_sentinel = m_init(uint32_t);

	suite_baseline = telemetry;

	ASSERT(buffer->data == NULL);
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);
	ASSERT(buffer->actually_allocated_bytes == 0);

	ASSERT(sentinel->data == NULL);
	ASSERT(sentinel->length == 0);
	ASSERT(sentinel->string_length == 0);
	ASSERT(sentinel->is_string == false);
	ASSERT(sentinel->actually_allocated_bytes == 0);

	RETURN_STATUS;
}

/**
 * @brief Cover the first physical allocation of a descriptor
 *
 * Resizes the empty buffer to one element. The library must round the
 * request up to one slab block, register one fresh allocation, push
 * the payload counters by one byte, and create one extra active
 * descriptor relative to the entry baseline of this subtest
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_02(void)
{
	INITTEST;

	const size_t block = MEMORY_BLOCK_BYTES;

	const size_t baseline_current_heap = telemetry.current_heap_reserved_bytes;
	const size_t baseline_current_payload = telemetry.current_payload_bytes;
	const size_t baseline_current_block_overhead = telemetry.current_block_overhead_bytes;
	const size_t baseline_current_active = telemetry.current_active_descriptors;
	const size_t baseline_total_acquired = telemetry.total_heap_reserved_bytes_acquired;
	const size_t baseline_total_payload_added = telemetry.total_payload_bytes_added;
	const size_t baseline_total_block_overhead_added = telemetry.total_block_overhead_bytes_added;
	const size_t baseline_fresh = telemetry.fresh_heap_allocations;
	const size_t baseline_reallocations = telemetry.heap_reallocations;
	const size_t baseline_in_place = telemetry.in_place_resizes;
	const size_t baseline_noop = telemetry.noop_resizes;

	ASSERT(SUCCESS == m_resize(buffer,1));

	ASSERT(telemetry.current_heap_reserved_bytes == baseline_current_heap + block);
	ASSERT(telemetry.current_payload_bytes == baseline_current_payload + 1);
	ASSERT(telemetry.current_block_overhead_bytes == baseline_current_block_overhead + (block - 1));
	ASSERT(telemetry.current_active_descriptors == baseline_current_active + 1);

	ASSERT(telemetry.total_heap_reserved_bytes_acquired == baseline_total_acquired + block);
	ASSERT(telemetry.total_payload_bytes_added == baseline_total_payload_added + 1);
	ASSERT(telemetry.total_block_overhead_bytes_added == baseline_total_block_overhead_added + (block - 1));

	ASSERT(telemetry.fresh_heap_allocations == baseline_fresh + 1);
	ASSERT(telemetry.heap_reallocations == baseline_reallocations);
	ASSERT(telemetry.in_place_resizes == baseline_in_place);
	ASSERT(telemetry.noop_resizes == baseline_noop);

	ASSERT(telemetry.peak_heap_reserved_bytes >= telemetry.current_heap_reserved_bytes);
	ASSERT(telemetry.peak_block_overhead_bytes >= telemetry.current_block_overhead_bytes);
	ASSERT(telemetry.peak_active_descriptors >= telemetry.current_active_descriptors);

	RETURN_STATUS;
}

/**
 * @brief Cover growth that crosses the slab boundary
 *
 * Grows the buffer past one slab block so the library must hand the
 * request to the OS allocator as a real reallocation. heap_reallocations
 * advances by one, current and cumulative heap reserve grow by exactly
 * one slab block, and the payload counters move by exactly the added
 * element count. peak_heap_reserved_bytes is lifted to the new
 * post-resize value when that value exceeds the existing peak. Block
 * overhead stays at its entry baseline because the new logical payload
 * (block + 1) lands inside the new two-block reserve with the same
 * trailing overhead (block - 1) as before, so current_block_overhead_bytes,
 * total_block_overhead_bytes_added, and peak_block_overhead_bytes all
 * stay byte-for-byte equal to their pre-resize values. The full
 * Telemetry struct is compared against the expected post-state through
 * memcmp so any unexpected counter movement fails the test
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_03(void)
{
	INITTEST;

	const size_t block = MEMORY_BLOCK_BYTES;
	const size_t big_count = block + 1;

	const Telemetry before_grow = telemetry;

	ASSERT(SUCCESS == m_resize(buffer,big_count));

	/* The expected post-state is computed from the pre-state by applying
	   exactly the deltas the contract permits for a slab-crossing
	   reallocation: one slab block of fresh heap reserve, the matching
	   acquired-byte total, the payload growth from 1 to big_count
	   elements, and one heap reallocation event. Block-overhead counters
	   stay untouched because the overhead before and after the grow is
	   the same block - 1 bytes. The global peak counter is checked
	   separately because it is monotonic across the whole process */
	Telemetry expected_after_grow = before_grow;
	expected_after_grow.current_heap_reserved_bytes += block;
	expected_after_grow.total_heap_reserved_bytes_acquired += block;
	expected_after_grow.current_payload_bytes += (big_count - 1);
	expected_after_grow.total_payload_bytes_added += (big_count - 1);
	expected_after_grow.heap_reallocations += 1;

	/* peak_heap_reserved_bytes is global and monotonic, so compare the deterministic counters separately */
	Telemetry observed_after_grow = telemetry;
	observed_after_grow.peak_heap_reserved_bytes = 0;
	expected_after_grow.peak_heap_reserved_bytes = 0;

	ASSERT(memcmp(&observed_after_grow,&expected_after_grow,sizeof(Telemetry)) == 0);
	ASSERT(telemetry.peak_heap_reserved_bytes >= before_grow.peak_heap_reserved_bytes);
	ASSERT(telemetry.peak_heap_reserved_bytes >= telemetry.current_heap_reserved_bytes);

	RETURN_STATUS;
}

/**
 * @brief Cover RELEASE_UNUSED shrink that returns one slab block to the OS
 *
 * Shrinks the buffer back to one element with RELEASE_UNUSED set. The
 * library must release exactly one slab block back to the allocator,
 * bump release_unused_shrinks and the matching released-bytes totals
 * by one block, count this transition as one heap reallocation, and
 * keep the descriptor alive (data still non-NULL, length now 1)
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_04(void)
{
	INITTEST;

	const size_t block = MEMORY_BLOCK_BYTES;

	const size_t baseline_current_heap = telemetry.current_heap_reserved_bytes;
	const size_t baseline_current_payload = telemetry.current_payload_bytes;
	const size_t baseline_total_released = telemetry.total_heap_reserved_bytes_released;
	const size_t baseline_release_shrinks = telemetry.release_unused_shrinks;
	const size_t baseline_total_release_unused = telemetry.total_release_unused_heap_reserved_bytes_released;
	const size_t baseline_reallocations = telemetry.heap_reallocations;

	ASSERT(SUCCESS == m_resize(buffer,1,RELEASE_UNUSED));

	ASSERT(telemetry.current_heap_reserved_bytes == baseline_current_heap - block);
	ASSERT(telemetry.current_payload_bytes == baseline_current_payload - (MEMORY_BLOCK_BYTES + 1 - 1));
	ASSERT(telemetry.total_heap_reserved_bytes_released == baseline_total_released + block);
	ASSERT(telemetry.release_unused_shrinks == baseline_release_shrinks + 1);
	ASSERT(telemetry.total_release_unused_heap_reserved_bytes_released == baseline_total_release_unused + block);
	ASSERT(telemetry.heap_reallocations == baseline_reallocations + 1);

	ASSERT(buffer->length == 1);
	ASSERT(buffer->data != NULL);

	RETURN_STATUS;
}

/**
 * @brief Cover an in-place grow inside the retained slab block
 *
 * Grows the descriptor from one to two elements. The slab block
 * reserved after the previous shrink already covers two bytes, so
 * the resize must be served as a pure bookkeeping change.
 * heap_reallocations stays at the value left by the previous shrink,
 * while in_place_resizes advances by one and the payload counters
 * move by exactly the new element
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_05(void)
{
	INITTEST;

	const size_t baseline_current_heap = telemetry.current_heap_reserved_bytes;
	const size_t baseline_current_payload = telemetry.current_payload_bytes;
	const size_t baseline_total_payload_added = telemetry.total_payload_bytes_added;
	const size_t baseline_in_place = telemetry.in_place_resizes;
	const size_t baseline_reallocations = telemetry.heap_reallocations;

	ASSERT(SUCCESS == m_resize(buffer,2));

	ASSERT(telemetry.current_heap_reserved_bytes == baseline_current_heap);
	ASSERT(telemetry.current_payload_bytes == baseline_current_payload + 1);
	ASSERT(telemetry.total_payload_bytes_added == baseline_total_payload_added + 1);
	ASSERT(telemetry.in_place_resizes == baseline_in_place + 1);
	ASSERT(telemetry.heap_reallocations == baseline_reallocations);

	ASSERT(buffer->length == 2);
	ASSERT(buffer->data != NULL);

	RETURN_STATUS;
}

/**
 * @brief Cover a sequence of consecutive no-op resizes
 *
 * Issues three m_resize calls that ask for the size the descriptor
 * already has. Each call must skip every allocation path, increment
 * noop_resizes by one, and advance current_consecutive_noop_resizes by
 * one. peak_consecutive_noop_resizes must be at least three by the end of
 * this subtest. None of the allocation, payload, or block-overhead
 * totals are allowed to change here
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_06(void)
{
	INITTEST;

	const Telemetry before_consecutive_noops = telemetry;

	ASSERT(SUCCESS == m_resize(buffer,2));
	ASSERT(SUCCESS == m_resize(buffer,2));
	ASSERT(SUCCESS == m_resize(buffer,2));

	/* The expected post-state is computed from the pre-state by applying
	   exactly the deltas a no-op resize is contractually allowed to
	   produce: noop_resizes and current_consecutive_noop_resizes each grow by
	   one per call. The monotonic peak is checked separately because its
	   expected value is max(previous peak, current consecutive count), and the test
	   should not need its own branch to model that rule. Every other
	   counter must be byte-for-byte identical to the pre-state */
	const size_t previous_peak_consecutive_noops = before_consecutive_noops.peak_consecutive_noop_resizes;
	const size_t expected_current_consecutive_noops = before_consecutive_noops.current_consecutive_noop_resizes + 3;
	Telemetry expected_after_consecutive_noops = before_consecutive_noops;
	expected_after_consecutive_noops.noop_resizes += 3;
	expected_after_consecutive_noops.current_consecutive_noop_resizes = expected_current_consecutive_noops;

	Telemetry observed_after_consecutive_noops = telemetry;
	observed_after_consecutive_noops.peak_consecutive_noop_resizes = 0;
	expected_after_consecutive_noops.peak_consecutive_noop_resizes = 0;

	ASSERT(memcmp(&observed_after_consecutive_noops,&expected_after_consecutive_noops,sizeof(Telemetry)) == 0);
	ASSERT(telemetry.peak_consecutive_noop_resizes >= previous_peak_consecutive_noops);
	ASSERT(telemetry.peak_consecutive_noop_resizes >= expected_current_consecutive_noops);
	ASSERT(telemetry.peak_consecutive_noop_resizes == previous_peak_consecutive_noops
		|| telemetry.peak_consecutive_noop_resizes == expected_current_consecutive_noops);

	RETURN_STATUS;
}

/**
 * @brief Cover the consecutive no-op reset triggered by an effective resize
 *
 * Performs an in-place grow that actually changes the logical length.
 * The library must reset current_consecutive_noop_resizes to zero while
 * preserving peak_consecutive_noop_resizes from the previous subtest.
 * in_place_resizes advances by one and the payload counters move by
 * exactly the new element
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_07(void)
{
	INITTEST;

	const size_t baseline_peak_consecutive_noops = telemetry.peak_consecutive_noop_resizes;
	const size_t baseline_in_place = telemetry.in_place_resizes;
	const size_t baseline_current_payload = telemetry.current_payload_bytes;
	const size_t baseline_total_payload_added = telemetry.total_payload_bytes_added;
	const size_t baseline_noop = telemetry.noop_resizes;

	ASSERT(SUCCESS == m_resize(buffer,3));

	ASSERT(telemetry.current_consecutive_noop_resizes == 0);
	ASSERT(telemetry.peak_consecutive_noop_resizes == baseline_peak_consecutive_noops);
	ASSERT(telemetry.noop_resizes == baseline_noop);
	ASSERT(telemetry.in_place_resizes == baseline_in_place + 1);
	ASSERT(telemetry.current_payload_bytes == baseline_current_payload + 1);
	ASSERT(telemetry.total_payload_bytes_added == baseline_total_payload_added + 1);

	RETURN_STATUS;
}

/**
 * @brief Cover ZERO_NEW_MEMORY zero-fill on growth
 *
 * First exposes elements [3..7] without ZERO_NEW_MEMORY, fills them
 * with non-zero bytes, and shrinks the logical payload back to three
 * elements while retaining the slab block. It then grows the buffer
 * from three to eight elements with ZERO_NEW_MEMORY set. The tested
 * grow stays inside the retained slab block, so it counts as one
 * in-place resize. The library must zero exactly the newly exposed
 * bytes [3..7] and bump zero_initialized_payload_growths by one. The
 * newly exposed bytes are read back through m_data_ro to confirm that
 * the previously non-zero storage was cleared
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_08(void)
{
	INITTEST;

	/* Populate the reserved bytes that the tested grow will expose so
	   success cannot depend on previously zero allocator contents */
	ASSERT(SUCCESS == m_resize(buffer,8));

	unsigned char *raw = m_data(unsigned char,buffer);
	ASSERT(raw != NULL);
	IF(raw != NULL)
	{
		raw[3] = 0xa3U;
		raw[4] = 0xa4U;
		raw[5] = 0xa5U;
		raw[6] = 0xa6U;
		raw[7] = 0xa7U;
	}

	ASSERT(SUCCESS == m_resize(buffer,3));

	const size_t baseline_zero_growths = telemetry.zero_initialized_payload_growths;
	const size_t baseline_in_place = telemetry.in_place_resizes;
	const size_t baseline_current_payload = telemetry.current_payload_bytes;

	/* ZERO_NEW_MEMORY must clear the retained non-zero payload tail */
	ASSERT(SUCCESS == m_resize(buffer,8,ZERO_NEW_MEMORY));

	ASSERT(telemetry.zero_initialized_payload_growths == baseline_zero_growths + 1);
	ASSERT(telemetry.in_place_resizes == baseline_in_place + 1);
	ASSERT(telemetry.current_payload_bytes == baseline_current_payload + 5);
	ASSERT(buffer->length == 8);

	const unsigned char *view = m_data_ro(unsigned char,buffer);
	ASSERT(view != NULL);
	ASSERT(view[3] == 0);
	ASSERT(view[4] == 0);
	ASSERT(view[5] == 0);
	ASSERT(view[6] == 0);
	ASSERT(view[7] == 0);

	RETURN_STATUS;
}

/**
 * @brief Cover peak_active_descriptors at the entry of a second live descriptor
 *
 * Brings a second descriptor of a different element type into life
 * while the first one is still active. After the fresh allocation,
 * current_active_descriptors must be exactly two above the suite
 * baseline. peak_active_descriptors is verified only as the basic
 * peak >= current invariant, because the counter is global and
 * monotonic across the runner: when earlier tests have already pushed
 * the peak above the value reached here it stays unchanged, and only
 * when this fresh acquisition sets a new high does peak track current.
 * The test does not assert that this subtest itself lifted the peak.
 * fresh_heap_allocations advances by one to count the second fresh
 * acquisition
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_09(void)
{
	INITTEST;

	const size_t baseline_fresh = telemetry.fresh_heap_allocations;
	const size_t baseline_current_active = telemetry.current_active_descriptors;

	ASSERT(SUCCESS == m_resize(sentinel,1));

	ASSERT(telemetry.current_active_descriptors == baseline_current_active + 1);
	ASSERT(telemetry.current_active_descriptors == suite_baseline.current_active_descriptors + 2);
	ASSERT(telemetry.peak_active_descriptors >= telemetry.current_active_descriptors);
	ASSERT(telemetry.fresh_heap_allocations == baseline_fresh + 1);

	ASSERT(sentinel->length == 1);
	ASSERT(sentinel->data != NULL);

	RETURN_STATUS;
}

/**
 * @brief Cover arithmetic_guard_failures via the three guarded helpers
 *
 * Forces an overflow in m_guarded_byte_size, an overflow in
 * m_guarded_add, and an underflow in m_guarded_subtract. Each
 * guarded helper must reject the invalid input with FAILURE and bump
 * both the actual and expected arithmetic_guard_failures counters by
 * one, so each counter advances by exactly three relative to the entry baseline
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_10(void)
{
	INITTEST;

	const size_t baseline_guard = telemetry.arithmetic_guard_failures;
	const size_t baseline_expected_guard = telemetry.expected_arithmetic_guard_failures;
	size_t scratch = 0;

	telemetry_expected_arithmetic_guard_failures();
	ASSERT(FAILURE == m_guarded_byte_size(sentinel,SIZE_MAX,&scratch));
	ASSERT(telemetry.arithmetic_guard_failures == baseline_guard + 1);
	ASSERT(telemetry.expected_arithmetic_guard_failures == baseline_expected_guard + 1);

	telemetry_expected_arithmetic_guard_failures();
	ASSERT(FAILURE == m_guarded_add(SIZE_MAX,1,&scratch));
	ASSERT(telemetry.arithmetic_guard_failures == baseline_guard + 2);
	ASSERT(telemetry.expected_arithmetic_guard_failures == baseline_expected_guard + 2);

	telemetry_expected_arithmetic_guard_failures();
	ASSERT(FAILURE == m_guarded_subtract(0,1,&scratch));
	ASSERT(telemetry.arithmetic_guard_failures == baseline_guard + 3);
	ASSERT(telemetry.expected_arithmetic_guard_failures == baseline_expected_guard + 3);

	RETURN_STATUS;
}

/**
 * @brief Cover data_to_string_conversions through m_to_string
 *
 * Writes a known three-byte prefix into the buffer through m_data.
 * The trailing zero bytes from the previous ZERO_NEW_MEMORY subtest
 * let m_to_string measure string_length as three before it writes the
 * canonical terminator at that boundary. m_to_string must flip the
 * descriptor into string mode and bump data_to_string_conversions
 * exactly once. The logical length stays at eight because the current
 * descriptor span already has room for the visible payload and the
 * terminator
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_11(void)
{
	INITTEST;

	const size_t baseline_data_to_string = telemetry.data_to_string_conversions;

	unsigned char *raw = m_data(unsigned char,buffer);
	ASSERT(raw != NULL);
	IF(raw != NULL)
	{
		raw[0] = (unsigned char)'A';
		raw[1] = (unsigned char)'B';
		raw[2] = (unsigned char)'C';
	}

	ASSERT(SUCCESS == m_to_string(buffer));

	ASSERT(telemetry.data_to_string_conversions == baseline_data_to_string + 1);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->string_length == 3);
	ASSERT(buffer->length == 8);

	RETURN_STATUS;
}

/**
 * @brief Cover string_to_data_conversions through m_to_data
 *
 * Converts the buffer back to data mode. The cached string length is
 * three while the logical length is eight, so the trailing-terminator
 * trim path does not fire and the logical length stays at eight.
 * string_to_data_conversions advances by one and is_string flips
 * back to false while string_length resets to zero
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_12(void)
{
	INITTEST;

	const size_t baseline_string_to_data = telemetry.string_to_data_conversions;

	ASSERT(SUCCESS == m_to_data(buffer));

	ASSERT(telemetry.string_to_data_conversions == baseline_string_to_data + 1);
	ASSERT(buffer->is_string == false);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->length == 8);

	RETURN_STATUS;
}

/**
 * @brief Cover m_resize(...,0) without RELEASE_UNUSED preserving the reserve
 *
 * Snapshots the current data pointer and reserved byte count before
 * the call. After m_resize(buffer,0) the descriptor must report a
 * zero logical length and a zero string_length while still pointing
 * at the very same allocation as before, with actually_allocated_bytes
 * unchanged. heap_buffer_releases must stay at the entry baseline
 * because no memory has actually been handed back to the OS yet, and
 * release_unused_shrinks must also stay put because the call did not
 * carry the RELEASE_UNUSED flag. The previous logical payload (eight
 * bytes) is reclassified as block overhead because the slab reserve is
 * retained while the logical length drops to zero, so
 * current_block_overhead_bytes and total_block_overhead_bytes_added
 * each grow by exactly eight bytes and peak_block_overhead_bytes is
 * lifted whenever the new value exceeds the previous peak
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_13(void)
{
	INITTEST;

	const size_t baseline_current_heap = telemetry.current_heap_reserved_bytes;
	const size_t baseline_current_payload = telemetry.current_payload_bytes;
	const size_t baseline_releases = telemetry.heap_buffer_releases;
	const size_t baseline_release_shrinks = telemetry.release_unused_shrinks;
	const size_t baseline_total_release_unused = telemetry.total_release_unused_heap_reserved_bytes_released;
	const size_t baseline_current_active = telemetry.current_active_descriptors;
	const size_t baseline_current_block_overhead = telemetry.current_block_overhead_bytes;
	const size_t baseline_total_block_overhead_added = telemetry.total_block_overhead_bytes_added;

	void * const retained_data = buffer->data;
	const size_t retained_bytes = buffer->actually_allocated_bytes;

	ASSERT(SUCCESS == m_resize(buffer,0));

	ASSERT(buffer->data == retained_data);
	ASSERT(buffer->actually_allocated_bytes == retained_bytes);
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);

	ASSERT(telemetry.current_heap_reserved_bytes == baseline_current_heap);
	ASSERT(telemetry.current_payload_bytes == baseline_current_payload - 8);
	ASSERT(telemetry.current_active_descriptors == baseline_current_active);
	ASSERT(telemetry.heap_buffer_releases == baseline_releases);
	ASSERT(telemetry.release_unused_shrinks == baseline_release_shrinks);
	ASSERT(telemetry.total_release_unused_heap_reserved_bytes_released == baseline_total_release_unused);

	ASSERT(telemetry.current_block_overhead_bytes == baseline_current_block_overhead + 8);
	ASSERT(telemetry.total_block_overhead_bytes_added == baseline_total_block_overhead_added + 8);
	ASSERT(telemetry.peak_block_overhead_bytes >= telemetry.current_block_overhead_bytes);

	RETURN_STATUS;
}

/**
 * @brief Cover m_resize(...,0,RELEASE_UNUSED) returning the buffer to the OS
 *
 * The capacity that was retained in the previous subtest is now
 * released. The descriptor must drop its data pointer, drop the
 * actually_allocated_bytes, count one heap_buffer_releases for the
 * physical free, advance release_unused_shrinks by one, and add the
 * released slab bytes to total_release_unused_heap_reserved_bytes_released.
 * Active descriptors decrement by one because the sentinel is still
 * alive
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_14(void)
{
	INITTEST;

	const size_t block = MEMORY_BLOCK_BYTES;

	const size_t baseline_current_heap = telemetry.current_heap_reserved_bytes;
	const size_t baseline_releases = telemetry.heap_buffer_releases;
	const size_t baseline_release_shrinks = telemetry.release_unused_shrinks;
	const size_t baseline_total_release_unused = telemetry.total_release_unused_heap_reserved_bytes_released;
	const size_t baseline_total_released = telemetry.total_heap_reserved_bytes_released;
	const size_t baseline_current_active = telemetry.current_active_descriptors;

	ASSERT(SUCCESS == m_resize(buffer,0,RELEASE_UNUSED));

	ASSERT(buffer->data == NULL);
	ASSERT(buffer->actually_allocated_bytes == 0);
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);

	ASSERT(telemetry.current_heap_reserved_bytes == baseline_current_heap - block);
	ASSERT(telemetry.current_active_descriptors == baseline_current_active - 1);
	ASSERT(telemetry.heap_buffer_releases == baseline_releases + 1);
	ASSERT(telemetry.release_unused_shrinks == baseline_release_shrinks + 1);
	ASSERT(telemetry.total_release_unused_heap_reserved_bytes_released == baseline_total_release_unused + block);
	ASSERT(telemetry.total_heap_reserved_bytes_released == baseline_total_released + block);

	RETURN_STATUS;
}

/**
 * @brief Cover descriptor teardown and assert the suite-wide post-conditions
 *
 * Calls m_del on the already-empty buffer, which must be accepted
 * without bumping any counter, and then calls m_del on the sentinel,
 * which actually frees the sentinel's slab block. After the calls every
 * "current" counter must be back to its suite baseline value, the
 * "total" counters must be at least at their suite baseline values,
 * and peak_consecutive_noop_resizes must be at least three because subtest
 * 06 directly drives that consecutive no-op count inside the suite. The
 * remaining peak
 * counters (peak_heap_reserved_bytes, peak_block_overhead_bytes,
 * peak_active_descriptors) are global and monotonic across the runner,
 * so this finalization step does not re-check them. Allocator-failure
 * counters (heap_allocation_failures, heap_reallocation_failures) are
 * exercised by dedicated subtests 17 and 18 through the libmem
 * allocator mock, not by this finalization step
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_15(void)
{
	INITTEST;

	const size_t block = MEMORY_BLOCK_BYTES;

	/* m_del on an already-empty descriptor enters mem_delete with
	   data == NULL and must skip every counter-touching branch, so the
	   whole Telemetry struct has to stay byte-for-byte unchanged */
	const Telemetry before_buffer_del = telemetry;

	call(m_del(buffer));

	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);
	ASSERT(buffer->data == NULL);
	ASSERT(buffer->actually_allocated_bytes == 0);
	ASSERT(memcmp(&telemetry,&before_buffer_del,sizeof(Telemetry)) == 0);

	/* m_del on the still-allocated sentinel must move exactly the
	   counters that mem_delete touches when data != NULL: the heap
	   reserve and active-descriptor counts decrement, the
	   release-side totals advance by one slab block, and the
	   payload and block-overhead bookkeeping for the sentinel's
	   four-byte logical content is rolled back. Building the
	   expected Telemetry from the pre-state and asserting full-struct
	   equality verifies that no other counter slipped */
	const Telemetry before_sentinel_del = telemetry;
	const size_t sentinel_payload_bytes = sizeof(uint32_t);
	const size_t sentinel_block_overhead_bytes = block - sentinel_payload_bytes;

	call(m_del(sentinel));

	ASSERT(sentinel->length == 0);
	ASSERT(sentinel->string_length == 0);
	ASSERT(sentinel->is_string == false);
	ASSERT(sentinel->data == NULL);
	ASSERT(sentinel->actually_allocated_bytes == 0);

	Telemetry expected_after_sentinel_del = before_sentinel_del;
	expected_after_sentinel_del.current_heap_reserved_bytes -= block;
	expected_after_sentinel_del.total_heap_reserved_bytes_released += block;
	expected_after_sentinel_del.heap_buffer_releases += 1;
	expected_after_sentinel_del.current_active_descriptors -= 1;
	expected_after_sentinel_del.current_payload_bytes -= sentinel_payload_bytes;
	expected_after_sentinel_del.current_block_overhead_bytes -= sentinel_block_overhead_bytes;

	ASSERT(memcmp(&telemetry,&expected_after_sentinel_del,sizeof(Telemetry)) == 0);

	ASSERT(telemetry.current_heap_reserved_bytes == suite_baseline.current_heap_reserved_bytes);
	ASSERT(telemetry.current_payload_bytes == suite_baseline.current_payload_bytes);
	ASSERT(telemetry.current_active_descriptors == suite_baseline.current_active_descriptors);
	ASSERT(telemetry.current_block_overhead_bytes == suite_baseline.current_block_overhead_bytes);
	ASSERT(telemetry.current_consecutive_noop_resizes == 0);

	/* Peak counters are global and monotonic across the whole process,
	   so when the suite runs alongside other tests an earlier high mark
	   can already exceed anything this suite produces. The meaningful
	   peak driven by the suite itself (the three consecutive no-op resizes) is
	   verified directly. The remaining peak counters
	   (peak_heap_reserved_bytes, peak_block_overhead_bytes,
	   peak_active_descriptors) are not asserted here because no subtest
	   enforces a suite-driven lift: subtests 02 and 09 check only the
	   peak >= current invariant, and the full-struct memcmp in
	   subtest 03 pins peak to its conditional post-state without
	   requiring the suite to be the source of the maximum */
	ASSERT(telemetry.peak_consecutive_noop_resizes >= 3);

	/* The runner framework (testitall) may allocate and free its own
	   STDOUT, STDERR, and EXTEND capture descriptors between subtests,
	   so the monotonic byte and "fresh"/"release" counters can carry
	   framework contributions on top of the test-driven moves. The
	   asserts below use >= for those counters and == for the counters
	   that only this suite can move here: mode conversions,
	   guarded-arithmetic failures, RELEASE_UNUSED counters,
	   ZERO_NEW_MEMORY growths, and no-op resizes */
	ASSERT(telemetry.total_heap_reserved_bytes_acquired >= suite_baseline.total_heap_reserved_bytes_acquired + block * 3);
	ASSERT(telemetry.total_heap_reserved_bytes_released >= suite_baseline.total_heap_reserved_bytes_released + block * 3);
	ASSERT(telemetry.total_release_unused_heap_reserved_bytes_released == suite_baseline.total_release_unused_heap_reserved_bytes_released + block * 2);

	ASSERT(telemetry.fresh_heap_allocations >= suite_baseline.fresh_heap_allocations + 2);
	ASSERT(telemetry.heap_reallocations >= suite_baseline.heap_reallocations + 2);
	ASSERT(telemetry.in_place_resizes >= suite_baseline.in_place_resizes + 5);
	ASSERT(telemetry.noop_resizes == suite_baseline.noop_resizes + 3);
	ASSERT(telemetry.release_unused_shrinks == suite_baseline.release_unused_shrinks + 2);
	ASSERT(telemetry.heap_buffer_releases >= suite_baseline.heap_buffer_releases + 2);
	ASSERT(telemetry.zero_initialized_payload_growths == suite_baseline.zero_initialized_payload_growths + 1);

	ASSERT(telemetry.data_to_string_conversions == suite_baseline.data_to_string_conversions + 1);
	ASSERT(telemetry.string_to_data_conversions == suite_baseline.string_to_data_conversions + 1);
	ASSERT(telemetry.arithmetic_guard_failures == suite_baseline.arithmetic_guard_failures + 3);
	ASSERT(telemetry.expected_arithmetic_guard_failures == suite_baseline.expected_arithmetic_guard_failures + 3);

	/* mem_write_zero_terminator centralized helper fires for every
	   successful terminator write across the library. Up to this
	   checkpoint, before the dedicated finalize-string subtest runs,
	   the only suite-guaranteed write is the terminator placement
	   during subtest 11's m_to_string flip. The runner framework drives
	   additional writes between subtests, so the assert uses >= rather
	   than == */
	ASSERT(telemetry.string_terminator_writes >= suite_baseline.string_terminator_writes + 1);

	#if SHOW_TEST
	telemetry_summary();
	#endif

	RETURN_STATUS;
}

/**
 * @brief Cover the two finalize-string terminator branches under WRITE_TERMINATOR_IF_MISSING and the centralized terminator-write counter
 *
 * Drives a fresh local string descriptor through three cases of
 * mem_finalize_string. The first case places a zero element at the
 * boundary slot, so finalize must skip the write and bump
 * finalize_string_terminator_already_present by one while leaving
 * finalize_string_terminator_written_when_missing and
 * string_terminator_writes untouched. The second case places a
 * non-zero element at the boundary slot, so finalize must write the
 * terminator itself and bump finalize_string_terminator_written_when_missing
 * and string_terminator_writes by one each while leaving
 * finalize_string_terminator_already_present untouched. The third
 * case forces WRITE_TERMINATOR_ALWAYS to confirm that branch does
 * not move either IF_MISSING counter but still bumps
 * string_terminator_writes by one because the centralized helper
 * ran the memset
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_16(void)
{
	INITTEST;

	const size_t baseline_already_present = telemetry.finalize_string_terminator_already_present;
	const size_t baseline_written_when_missing = telemetry.finalize_string_terminator_written_when_missing;

	/* Local string descriptor with capacity for two visible payload elements
	   and one terminator slot. Owned by this subtest so the suite finalization
	   in test_libmem_0009_15 stays unaffected by the activity here */
	m_create(char,string_buffer,MEMORY_STRING);

	ASSERT(SUCCESS == m_resize(string_buffer,3));
	ASSERT(string_buffer->is_string == true);
	ASSERT(string_buffer->length == 3);

	/* Capture string_terminator_writes after m_resize so the per-case
	   asserts below depend only on the three explicit m_finalize_string
	   calls below and not on whatever the string-mode resize did
	   internally through mem_string_truncate */
	const size_t baseline_terminator_writes = telemetry.string_terminator_writes;

	char *raw = m_data(char,string_buffer);
	ASSERT(raw != NULL);

	/* Case 16a: slot already holds a zero element, IF_MISSING skips the write */
	IF(raw != NULL)
	{
		raw[0] = 'A';
		raw[1] = 'B';
		raw[2] = '\0';
	}

	ASSERT(SUCCESS == m_finalize_string(string_buffer,2,WRITE_TERMINATOR_IF_MISSING));
	ASSERT(telemetry.finalize_string_terminator_already_present == baseline_already_present + 1);
	ASSERT(telemetry.finalize_string_terminator_written_when_missing == baseline_written_when_missing);
	ASSERT(telemetry.string_terminator_writes == baseline_terminator_writes);
	ASSERT(string_buffer->string_length == 2);

	/* Case 16b: slot holds a non-zero element, IF_MISSING compensates by writing */
	IF(raw != NULL)
	{
		raw[0] = 'C';
		raw[1] = 'D';
		raw[2] = 'X';
	}

	ASSERT(SUCCESS == m_finalize_string(string_buffer,2,WRITE_TERMINATOR_IF_MISSING));
	ASSERT(telemetry.finalize_string_terminator_already_present == baseline_already_present + 1);
	ASSERT(telemetry.finalize_string_terminator_written_when_missing == baseline_written_when_missing + 1);
	ASSERT(telemetry.string_terminator_writes == baseline_terminator_writes + 1);
	ASSERT(string_buffer->string_length == 2);
	ASSERT(raw[2] == '\0');

	/* Case 16c: WRITE_TERMINATOR_ALWAYS writes unconditionally and must
	   not move either of the new IF_MISSING counters */
	IF(raw != NULL)
	{
		raw[0] = 'E';
		raw[1] = 'F';
		raw[2] = 'Y';
	}

	ASSERT(SUCCESS == m_finalize_string(string_buffer,2,WRITE_TERMINATOR_ALWAYS));
	ASSERT(telemetry.finalize_string_terminator_already_present == baseline_already_present + 1);
	ASSERT(telemetry.finalize_string_terminator_written_when_missing == baseline_written_when_missing + 1);
	ASSERT(telemetry.string_terminator_writes == baseline_terminator_writes + 2);
	ASSERT(string_buffer->string_length == 2);
	ASSERT(raw[2] == '\0');

	call(m_del(string_buffer));

	RETURN_STATUS;
}

/**
 * @brief Capture function for subtest 17 — force malloc to return NULL inside mem_resize
 *
 * Activates the libmem allocator mock for one malloc call, then asks
 * m_resize to grow a fresh descriptor from zero to one element. The
 * expected-failure marker is advanced immediately before the forced
 * call. The library must observe the NULL return, bump heap_allocation_failures
 * by one, and leave the descriptor in a clean unallocated state. Runs through
 * match_function_output in the driver, so the report() emission that mem_resize
 * makes lands in captured stderr instead of polluting the suite output
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_0009_17_malloc_fail(void)
{
	INITTEST;

	const size_t baseline = telemetry.heap_allocation_failures;
	const size_t baseline_expected = telemetry.expected_heap_allocation_failures;

	m_create(char,fail_buf,MEMORY_STRING);

	testmocking_malloc_fail_next(1);
	telemetry_expected_heap_allocation_failures();
	const Return result = m_resize(fail_buf,1);
	testmocking_malloc_disable();

	ASSERT(FAILURE == result);
	ASSERT(telemetry.heap_allocation_failures == baseline + 1);
	ASSERT(telemetry.expected_heap_allocation_failures == baseline_expected + 1);
	ASSERT(fail_buf->data == NULL);
	ASSERT(fail_buf->length == 0);
	ASSERT(fail_buf->actually_allocated_bytes == 0);

	deliver(status);
}

/**
 * @brief Cover heap_allocation_failures by forcing malloc to return NULL
 *
 * Runs capture_libmem_0009_17_malloc_fail under match_function_output
 * so the expected report() output from mem_resize stays inside captured
 * stderr and away from the visible suite log. The driver asserts that
 * stdout was silent, that stderr matches the expected single ERROR line
 * for a 4096-byte malloc failure (line-number flexible, errno
 * description system-dependent). The subtest is gated by
 * SKIP_ON_EVIL_EMPIRE_OS because Apple ld does not honor -Wl,--wrap, so
 * the mock cannot fire there and the assertions would not hold
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_17(void)
{
	INITTEST;
	SKIP_ON_EVIL_EMPIRE_OS;

	/* Expected stderr layout for subtest 17. mem_resize is forced to a
	   failed malloc by the libmem allocator mock and is expected to emit a
	   single report() line. The pattern leaves the source line number
	   flexible with \\d+ and the errno description flexible with [^\\n]+,
	   but pins the message body byte for byte */
	static const char expected_stderr_pattern_libmem_0009_17[] =
	        "\\A"
	        "ERROR: src/mem_resize\\.c:mem_resize:\\d+ Memory management; Memory allocation failed for 4096 bytes Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	        "\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0009_17,capture_libmem_0009_17_malloc_fail));

	RETURN_STATUS;
}

/**
 * @brief Capture function for subtest 18 — force realloc to return NULL inside mem_resize
 *
 * First grows a fresh descriptor through a successful initial malloc so
 * the next size change becomes a realloc instead of a fresh allocation.
 * Then activates the realloc mock for one call and asks m_resize to
 * grow past one slab block, which forces the library to call realloc.
 * The expected-failure marker is advanced immediately before that forced
 * call. The library must observe the NULL return, bump
 * heap_reallocation_failures by one, leave heap_allocation_failures
 * untouched, and keep the descriptor's previous allocation valid
 * because realloc returning NULL does not free the original block.
 * Runs through match_function_output in the driver
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_0009_18_realloc_fail(void)
{
	INITTEST;

	const size_t baseline_alloc = telemetry.heap_allocation_failures;
	const size_t baseline_realloc = telemetry.heap_reallocation_failures;
	const size_t baseline_expected_realloc = telemetry.expected_heap_reallocation_failures;

	m_create(char,grow_buf,MEMORY_STRING);

	ASSERT(SUCCESS == m_resize(grow_buf,1));

	testmocking_realloc_fail_next(1);
	telemetry_expected_heap_reallocation_failures();
	const Return result = m_resize(grow_buf,MEMORY_BLOCK_BYTES + 1);
	testmocking_realloc_disable();

	ASSERT(FAILURE == result);
	ASSERT(telemetry.heap_reallocation_failures == baseline_realloc + 1);
	ASSERT(telemetry.expected_heap_reallocation_failures == baseline_expected_realloc + 1);
	ASSERT(telemetry.heap_allocation_failures == baseline_alloc);
	ASSERT(grow_buf->data != NULL);
	ASSERT(grow_buf->length == 1);

	call(m_del(grow_buf));

	deliver(status);
}

/**
 * @brief Cover heap_reallocation_failures by forcing realloc to return NULL
 *
 * Runs capture_libmem_0009_18_realloc_fail under match_function_output
 * so the expected report() output from mem_resize stays inside captured
 * stderr. The driver asserts that stdout was silent and that stderr
 * matches the expected single ERROR line for an 8192-byte realloc failure.
 * Same SKIP_ON_EVIL_EMPIRE_OS gating as subtest 17
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0009_18(void)
{
	INITTEST;
	SKIP_ON_EVIL_EMPIRE_OS;

	/* Expected stderr layout for subtest 18. The realloc path inside
	   mem_resize prints the same report() format as the malloc path, only
	   the slab-rounded byte count differs */
	static const char expected_stderr_pattern_libmem_0009_18[] =
	        "\\A"
	        "ERROR: src/mem_resize\\.c:mem_resize:\\d+ Memory management; Memory allocation failed for 8192 bytes Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	        "\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0009_18,capture_libmem_0009_18_realloc_fail));

	RETURN_STATUS;
}

/**
 * @brief End-to-end suite that exercises every libmem telemetry counter at least once
 *
 * Drives a shared unsigned-char descriptor and an auxiliary uint32_t
 * descriptor through every transition that a Telemetry counter
 * observes. Each counter in the Telemetry struct is exercised at least
 * once by the actions of one of the subtests below and is then
 * asserted as a delta relative to the entry baseline of the
 * corresponding subtest, so the suite serves as the canonical proof
 * that telemetry stays consistent with descriptor state across every
 * supported transition. Allocator-failure counters
 * (heap_allocation_failures, heap_reallocation_failures) are driven
 * deterministically by the libmem allocator mock in subtests 17 and 18
 *
 * @return Return describing success or failure
 */
Return test_libmem_0009(void)
{
	INITTEST;

	TEST(test_libmem_0009_01,"Suite baseline captured and shared descriptors reset…");
	TEST(test_libmem_0009_02,"First allocation populates fresh heap and payload counters…");
	TEST(test_libmem_0009_03,"Growth across a slab boundary registers a heap reallocation…");
	TEST(test_libmem_0009_04,"RELEASE_UNUSED shrink returns one slab block to the OS…");
	TEST(test_libmem_0009_05,"In-place grow inside the retained slab block bumps in_place_resizes…");
	TEST(test_libmem_0009_06,"Three consecutive no-op resizes reach a count of three…");
	TEST(test_libmem_0009_07,"Effective resize resets consecutive no-op counting and preserves the peak…");
	TEST(test_libmem_0009_08,"ZERO_NEW_MEMORY growth zero-fills the newly exposed payload…");
	TEST(test_libmem_0009_09,"Second live descriptor brings active count to two above the suite baseline…");
	TEST(test_libmem_0009_10,"Guarded arithmetic helpers reject invalid math three times…");
	TEST(test_libmem_0009_11,"m_to_string flips the buffer into string mode…");
	TEST(test_libmem_0009_12,"m_to_data flips the buffer back into data mode…");
	TEST(test_libmem_0009_13,"m_resize to zero without flags keeps the underlying block…");
	TEST(test_libmem_0009_14,"m_resize to zero with RELEASE_UNUSED frees the buffer…");
	TEST(test_libmem_0009_15,"m_del tears down both descriptors and finalizes the suite…");
	TEST(test_libmem_0009_16,"m_finalize_string IF_MISSING records present-vs-written terminator counters…");
	TEST(test_libmem_0009_17,"Allocator returns NULL on initial malloc and bumps heap_allocation_failures…");
	TEST(test_libmem_0009_18,"Allocator returns NULL on grow realloc and bumps heap_reallocation_failures…");

	RETURN_STATUS;
}
