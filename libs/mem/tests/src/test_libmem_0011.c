#include "test_libmem_utils.h"

/**
 * @brief Check raw pointer helpers and descriptor copying for typed point arrays
 *
 * @return Return describing success or failure
 */
Return test_libmem_0011(void)
{
	INITTEST;

	m_create(point,points);
	m_create(point,mirror);

	ASSERT(SUCCESS == m_resize(points,5));
	ASSERT(SUCCESS == fill_points(points));

	point *raw_points = (point *)m_raw_data(points);
	ASSERT(raw_points != NULL);

	if(raw_points != NULL)
	{
		raw_points[0].x += 100;
		raw_points[0].y += 200;
	}

	const point *readonly_points = (const point *)m_raw_data_ro(points);
	ASSERT(readonly_points != NULL);

	if(readonly_points != NULL)
	{
		ASSERT(readonly_points[0].x == 101);
		ASSERT(readonly_points[0].y == 202);
	}

	ASSERT(SUCCESS == m_copy(mirror,points));

	const point *mirror_points = m_data_ro(point,mirror);
	ASSERT(mirror_points != NULL);

	if(mirror_points != NULL)
	{
		ASSERT(mirror->length == 5);
		ASSERT(mirror_points[0].x == 101);
		ASSERT(mirror_points[0].y == 202);
	}

	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	ASSERT(mirror->string_length == 0);
	ASSERT(mirror->is_string == false);
	ASSERT(SUCCESS == m_del(points));
	ASSERT(SUCCESS == m_del(mirror));

	RETURN_STATUS;
}
