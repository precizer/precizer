#include "test_libmem_utils.h"

/**
 * @brief Check typed descriptors and m_resize flags on point arrays
 *
 * @return Return describing success or failure
 */
Return test_libmem_0010(void)
{
	INITTEST;

	m_create(point,points);
	m_create(point,zeroed_points);

	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	ASSERT(zeroed_points->string_length == 0);
	ASSERT(zeroed_points->is_string == false);

	ASSERT(SUCCESS == m_resize(points,5));
	ASSERT(SUCCESS == m_resize(zeroed_points,3,ZERO_NEW_MEMORY));

	const point *zeroed_view = m_data_ro(point,zeroed_points);
	ASSERT(zeroed_view != NULL);

	if(zeroed_view != NULL)
	{
		ASSERT(zeroed_view[0].x == 0);
		ASSERT(zeroed_view[0].y == 0);
	}

	point *zeroed_writer = m_data(point,zeroed_points);
	ASSERT(zeroed_writer != NULL);

	if(zeroed_writer != NULL)
	{
		zeroed_writer[0] = (point){1,1};
	}

	ASSERT(SUCCESS == m_resize(zeroed_points,6,ZERO_NEW_MEMORY | RELEASE_UNUSED));

	const point *expanded_view = m_data_ro(point,zeroed_points);
	ASSERT(expanded_view != NULL);

	if(expanded_view != NULL)
	{
		ASSERT(expanded_view[0].x == 1);
		ASSERT(expanded_view[0].y == 1);
		ASSERT(expanded_view[5].x == 0);
		ASSERT(expanded_view[5].y == 0);
	}

	ASSERT(SUCCESS == m_resize(zeroed_points,2,ZERO_NEW_MEMORY | RELEASE_UNUSED));

	const point *shrunk_view = m_data_ro(point,zeroed_points);
	ASSERT(shrunk_view != NULL);

	if(shrunk_view != NULL)
	{
		ASSERT(zeroed_points->length == 2);
		ASSERT(shrunk_view[0].x == 1);
		ASSERT(shrunk_view[0].y == 1);
	}

	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	ASSERT(zeroed_points->string_length == 0);
	ASSERT(zeroed_points->is_string == false);
	ASSERT(SUCCESS == m_del(points));
	ASSERT(SUCCESS == m_del(zeroed_points));

	RETURN_STATUS;
}
