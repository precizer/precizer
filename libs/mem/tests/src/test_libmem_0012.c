#include "test_libmem_utils.h"

/**
 * @brief Verify RELEASE_UNUSED physically shrinks a typed point descriptor while preserving the surviving prefix
 *
 * Grows the descriptor past one allocation slab, fills it with a
 * deterministic point pattern, then shrinks it back to four elements
 * with RELEASE_UNUSED. The allocation reserve must drop to one slab,
 * and the surviving prefix must remain readable after the physical
 * shrink. The descriptor is then grown again to prove that the same
 * prefix still survives the shrink/regrow cycle
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0012_1(void)
{
	INITTEST;

	m_create(point,points);

	/* Choose a source length that forces the descriptor to reserve more than one allocation slab */
	const size_t points_per_slab = MEMORY_BLOCK_BYTES / sizeof(point);
	const size_t large_length = points_per_slab + 1;
	const size_t surviving_length = 4;

	ASSERT(points_per_slab > 0);
	ASSERT(large_length > surviving_length);

	/* Grow past one slab and seed every element with the deterministic point pattern */
	ASSERT(SUCCESS == m_resize(points,large_length));
	ASSERT(SUCCESS == fill_points(points));

	/* Record the large reserve so the later RELEASE_UNUSED shrink can prove that capacity was returned */
	const size_t allocated_before_shrink = points->actually_allocated_bytes;
	ASSERT(allocated_before_shrink > MEMORY_BLOCK_BYTES);

	/* Shrink to a small prefix and require the reserve to drop back to exactly one slab */
	ASSERT(SUCCESS == m_resize(points,surviving_length,RELEASE_UNUSED));
	ASSERT(points->length == surviving_length);
	ASSERT(points->actually_allocated_bytes == MEMORY_BLOCK_BYTES);
	ASSERT(points->actually_allocated_bytes < allocated_before_shrink);

	/* Read the remaining prefix after the physical shrink to catch corrupted surviving elements */
	const point *shrunk_points = m_data_ro(point,points);
	ASSERT(shrunk_points != NULL);

	IF(shrunk_points != NULL)
	{
		for(size_t point_index = 0; point_index < surviving_length; point_index++)
		{
			ASSERT(shrunk_points[point_index].x == (int)(point_index * 2 + 1));
			ASSERT(shrunk_points[point_index].y == (int)(point_index * 2 + 2));
		}
	}

	/* Grow again inside the retained slab; the already-surviving prefix must still be intact */
	ASSERT(SUCCESS == m_resize(points,8));
	ASSERT(points->length == 8);
	ASSERT(points->actually_allocated_bytes == MEMORY_BLOCK_BYTES);

	/* Re-read through a fresh view because any resize may invalidate cached data pointers */
	const point *regrown_points = m_data_ro(point,points);
	ASSERT(regrown_points != NULL);

	IF(regrown_points != NULL)
	{
		for(size_t point_index = 0; point_index < surviving_length; point_index++)
		{
			ASSERT(regrown_points[point_index].x == (int)(point_index * 2 + 1));
			ASSERT(regrown_points[point_index].y == (int)(point_index * 2 + 2));
		}
	}

	/* Confirm the resize cycle did not accidentally switch the descriptor into string mode */
	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	call(m_del(points));

	RETURN_STATUS;
}

/**
 * @brief Verify m_concat_data appends a typed source after a RELEASE_UNUSED shrink/regrow cycle
 *
 * Builds a five-element destination prefix and a source descriptor
 * that first crosses a slab boundary, then shrinks with RELEASE_UNUSED,
 * then grows back to eight elements. The final byte-for-byte check
 * proves that m_concat_data keeps the destination prefix intact and
 * appends the full regrown source in order
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0012_2(void)
{
	INITTEST;

	m_create(point,points);
	m_create(point,mirror);

	/* Use the same slab-crossing source size as the shrink/regrow test */
	const size_t points_per_slab = MEMORY_BLOCK_BYTES / sizeof(point);
	const size_t large_length = points_per_slab + 1;

	ASSERT(points_per_slab > 0);

	/* Prepare the destination prefix that must survive the append untouched */
	ASSERT(SUCCESS == m_resize(mirror,5));
	ASSERT(SUCCESS == fill_points(mirror));

	/* Prepare a source descriptor large enough for RELEASE_UNUSED to reclaim real capacity */
	ASSERT(SUCCESS == m_resize(points,large_length));
	ASSERT(SUCCESS == fill_points(points));

	/* Capture the oversized reserve before shrinking the source */
	const size_t allocated_before_shrink = points->actually_allocated_bytes;
	ASSERT(allocated_before_shrink > MEMORY_BLOCK_BYTES);

	/* Physically shrink the source to its first four points before regrowing it for the append */
	ASSERT(SUCCESS == m_resize(points,4,RELEASE_UNUSED));
	ASSERT(points->length == 4);
	ASSERT(points->actually_allocated_bytes == MEMORY_BLOCK_BYTES);
	ASSERT(points->actually_allocated_bytes < allocated_before_shrink);

	/* Regrow the source and write the newly exposed tail so the expected concat payload is complete */
	ASSERT(SUCCESS == m_resize(points,8));

	point *regrown_points = m_data(point,points);
	ASSERT(regrown_points != NULL);

	IF(regrown_points != NULL)
	{
		regrown_points[4] = (point){9,10};
		regrown_points[5] = (point){11,12};
		regrown_points[6] = (point){13,14};
		regrown_points[7] = (point){15,16};
	}

	/* Append all eight source points after the five-point destination prefix */
	ASSERT(SUCCESS == m_concat_data(mirror,points));

	/* Check the entire destination, not only the last appended point */
	const point *mirror_view = m_data_ro(point,mirror);
	ASSERT(mirror_view != NULL);

	IF(mirror_view != NULL)
	{
		const point expected[] = {
			{1,2},{3,4},{5,6},{7,8},{9,10},
			{1,2},{3,4},{5,6},{7,8},{9,10},{11,12},{13,14},{15,16}
		};
		const size_t expected_length = sizeof(expected) / sizeof(point);

		ASSERT(mirror->length == expected_length);
		ASSERT(memcmp(mirror_view,expected,sizeof(expected)) == 0);
	}

	/* Both descriptors must remain ordinary data descriptors after concat */
	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	ASSERT(mirror->string_length == 0);
	ASSERT(mirror->is_string == false);
	call(m_del(points));
	call(m_del(mirror));

	RETURN_STATUS;
}

/**
 * @brief Verify typed point shrink/regrow behavior and concatenation after RELEASE_UNUSED
 *
 * In plain terms, this suite checks that an aggressively shrunk typed
 * descriptor can be safely regrown and then used as a concatenation
 * source. The first nested test proves that RELEASE_UNUSED really
 * returns spare slab capacity while keeping the surviving prefix
 * intact. The second nested test appends the regrown source to a
 * five-point destination and verifies the complete thirteen-point
 * result, so overwrites, missing source elements, and ordering bugs
 * are visible
 *
 * Groups the typed-data checks that prove RELEASE_UNUSED can physically
 * reduce a descriptor's reserve without corrupting the surviving prefix,
 * and that m_concat_data can append a regrown typed source without
 * overwriting the destination
 *
 * @return Return describing success or failure
 */
Return test_libmem_0012(void)
{
	INITTEST;

	/* Keep the physical shrink/regrow contract separate from the concat contract */
	TEST(test_libmem_0012_1,"RELEASE_UNUSED shrink across a slab boundary preserves the typed point prefix…");
	TEST(test_libmem_0012_2,"m_concat_data appends a regrown typed point source after the destination prefix…");

	RETURN_STATUS;
}
