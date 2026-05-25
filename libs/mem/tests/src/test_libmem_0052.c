#include "test_libmem_utils.h"

/**
 * @brief Check m_copy_buffer with string and typed payloads
 *
 * @return Return describing success or failure
 */
Return test_libmem_0052(void)
{
	INITTEST;

	m_create(char,db_path);
	m_create(point,point_buffer);

	const char in_memory_db_path[] = ":memory:";
	const point sample_points[] = {
		{3,4},
		{5,6}
	};

	ASSERT(SUCCESS == m_copy_buffer(db_path,sizeof(in_memory_db_path),in_memory_db_path));
	ASSERT(db_path->length == sizeof(in_memory_db_path));
	ASSERT(db_path->string_length == 0);
	ASSERT(db_path->is_string == false);

	ASSERT(SUCCESS == m_copy_buffer(point_buffer,sizeof(sample_points),sample_points));

	const point *point_view = m_data_ro(point,point_buffer);
	ASSERT(point_view != NULL);

	IF(point_view != NULL)
	{
		ASSERT(point_buffer->length == 2);
		ASSERT(point_view[0].x == 3);
		ASSERT(point_view[0].y == 4);
		ASSERT(point_view[1].x == 5);
		ASSERT(point_view[1].y == 6);
	}

	ASSERT(point_buffer->string_length == 0);
	ASSERT(point_buffer->is_string == false);
	ASSERT(SUCCESS == m_copy_buffer(point_buffer,0,NULL));
	ASSERT(point_buffer->length == 0);
	call(m_del(db_path));
	call(m_del(point_buffer));

	RETURN_STATUS;
}
