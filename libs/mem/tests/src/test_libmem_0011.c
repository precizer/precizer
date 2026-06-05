#include "test_libmem_all.h"

/**
 * @brief Verify that typed and raw accessors expose the same descriptor storage
 *
 * Seeds a point descriptor through m_data, then obtains writable raw,
 * read-only raw, and read-only typed views. For a valid descriptor all
 * those views must point at the same backing allocation. A mutation
 * through the raw writable view must be visible through both read-only
 * views, proving that the raw helpers expose the live descriptor
 * storage rather than a detached buffer
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0011_1(void)
{
	INITTEST;

	m_create(point,points);

	/* Seed the working descriptor through the typed write helper with a predictable per-index pattern */
	ASSERT(SUCCESS == m_resize(points,5));
	ASSERT(SUCCESS == fill_points(points));

	point *typed_points = m_data(point,points);
	point *raw_points = (point *)m_raw_data(points);
	const point *readonly_raw_points = (const point *)m_raw_data_ro(points);
	const point *readonly_typed_points = m_data_ro(point,points);

	ASSERT(typed_points != NULL);
	ASSERT(raw_points != NULL);
	ASSERT(readonly_raw_points != NULL);
	ASSERT(readonly_typed_points != NULL);

	IF(typed_points != NULL &&
		raw_points != NULL &&
		readonly_raw_points != NULL &&
		readonly_typed_points != NULL)
	{
		/* All accessor families expose the same live allocation for a valid descriptor */
		ASSERT((const void *)typed_points == (const void *)raw_points);
		ASSERT((const void *)typed_points == (const void *)readonly_raw_points);
		ASSERT((const void *)typed_points == (const void *)readonly_typed_points);

		/* The asymmetric mutation makes detached views or wrong point-field interpretation visible on read-back */
		raw_points[0].x += 100;
		raw_points[0].y += 200;

		ASSERT(readonly_raw_points[0].x == 101);
		ASSERT(readonly_raw_points[0].y == 202);
		ASSERT(readonly_typed_points[0].x == 101);
		ASSERT(readonly_typed_points[0].y == 202);
	}

	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	call(m_del(points));

	RETURN_STATUS;
}

/**
 * @brief Verify that m_copy duplicates every typed point element
 *
 * Copies a populated point descriptor into an empty mirror descriptor.
 * The source first receives a raw-side mutation at index zero so the
 * copied payload includes both the deterministic fill_points pattern
 * and a later raw write. The mirror is then checked element by element,
 * which catches implementations that copy metadata or only a prefix of
 * the payload
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0011_2(void)
{
	INITTEST;

	m_create(point,points);
	m_create(point,mirror);

	ASSERT(SUCCESS == m_resize(points,5));
	ASSERT(SUCCESS == fill_points(points));

	point *raw_points = (point *)m_raw_data(points);
	ASSERT(raw_points != NULL);

	IF(raw_points != NULL)
	{
		raw_points[0].x += 100;
		raw_points[0].y += 200;
	}

	ASSERT(SUCCESS == m_copy(mirror,points));

	/* Typed read-only view on the mirror confirms the copy carried length and every point element */
	const point *mirror_points = m_data_ro(point,mirror);
	ASSERT(mirror_points != NULL);

	IF(mirror_points != NULL)
	{
		ASSERT(mirror->length == 5);

		for(size_t point_index = 0; point_index < mirror->length; point_index++)
		{
			int expected_x = (int)(point_index * 2 + 1);
			int expected_y = (int)(point_index * 2 + 2);

			if(point_index == 0)
			{
				expected_x += 100;
				expected_y += 200;
			}

			ASSERT(mirror_points[point_index].x == expected_x);
			ASSERT(mirror_points[point_index].y == expected_y);
		}
	}

	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	ASSERT(mirror->string_length == 0);
	ASSERT(mirror->is_string == false);
	call(m_del(points));
	call(m_del(mirror));

	RETURN_STATUS;
}

/**
 * @brief Verify raw pointer access and descriptor copying for typed point arrays
 *
 * Groups tests for the direct access contract of m_data, m_data_ro,
 * m_raw_data, and m_raw_data_ro, plus data-mode descriptor copying
 * through m_copy
 *
 * @return Return describing success or failure
 */
Return test_libmem_0011(void)
{
	INITTEST;

	TEST(test_libmem_0011_1,"Typed and raw point accessors share descriptor storage");
	TEST(test_libmem_0011_2,"m_copy duplicates every typed point element");

	RETURN_STATUS;
}
