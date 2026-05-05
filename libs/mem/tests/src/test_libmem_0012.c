#include "test_libmem_utils.h"

/**
 * @brief Check append and shrink-regrow scenarios for typed point arrays
 *
 * @return Return describing success or failure
 */
Return test_libmem_0012(void)
{
	INITTEST;

	m_create(point,points);
	m_create(point,mirror);

	ASSERT(SUCCESS == m_resize(points,5));
	ASSERT(SUCCESS == fill_points(points));
	ASSERT(SUCCESS == m_copy(mirror,points));
	ASSERT(SUCCESS == m_resize(points,8));

	point *writable_points = m_data(point,points);
	ASSERT(writable_points != NULL);

	if(writable_points != NULL)
	{
		writable_points[5] = (point){11,12};
		writable_points[6] = (point){13,14};
		writable_points[7] = (point){15,16};
	}

	ASSERT(SUCCESS == m_resize(points,4,RELEASE_UNUSED));
	ASSERT(points->length == 4);
	ASSERT(SUCCESS == m_resize(points,8));

	point *regrown_points = m_data(point,points);
	ASSERT(regrown_points != NULL);

	if(regrown_points != NULL)
	{
		regrown_points[4] = (point){9,10};
		regrown_points[5] = (point){11,12};
		regrown_points[6] = (point){13,14};
		regrown_points[7] = (point){15,16};
	}

	ASSERT(SUCCESS == m_concat_data(mirror,points));

	const point *mirror_view = m_data_ro(point,mirror);
	ASSERT(mirror_view != NULL);

	if(mirror_view != NULL)
	{
		const size_t expected_length = 13;

		ASSERT(mirror->length == expected_length);
		ASSERT(mirror_view[expected_length - 1].x == 15);
		ASSERT(mirror_view[expected_length - 1].y == 16);
	}

	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	ASSERT(mirror->string_length == 0);
	ASSERT(mirror->is_string == false);
	ASSERT(SUCCESS == m_del(points));
	ASSERT(SUCCESS == m_del(mirror));

	RETURN_STATUS;
}
