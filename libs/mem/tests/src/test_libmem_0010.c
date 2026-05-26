#include "test_libmem_utils.h"

/**
 * @brief Verify m_resize behavior with flag combinations on typed point descriptors
 *
 * Drives two descriptors over a user-defined point element type
 * (struct with two int fields, see test_libmem_utils.h) through a
 * chain of m_resize calls covering the practically interesting flag
 * combinations. The plain descriptor exercises an unflagged grow.
 * The second descriptor is taken through a fresh grow with
 * ZERO_NEW_MEMORY, then through a deterministic combined-flag grow:
 * the test exposes a tail, writes non-zero typed elements into it,
 * hides it again without releasing storage, and grows with
 * ZERO_NEW_MEMORY | RELEASE_UNUSED. That grow has to preserve the
 * existing payload while clearing every re-exposed tail element. A
 * shrink with the same combined flags then has to keep the surviving
 * payload intact. Both read-only and writable views are exercised:
 * m_data_ro confirms the zero fill through the typed element layout,
 * and m_data prepares the non-zero payload that resize must clear or
 * preserve as appropriate.
 * The final block asserts that both descriptors stayed in data mode
 * (is_string == false, string_length == 0) for the whole sequence
 * and that m_del releases each cleanly
 *
 * The test guards against two classes of regressions: confusion
 * between byte size and logical element size in resize bookkeeping
 * (checking both .x and .y makes the assertion depend on the full
 * point element, not only its first field), and silent loss of
 * existing payload across grow/shrink with combined flags
 *
 * @return Return describing success or failure
 */
Return test_libmem_0010(void)
{
	INITTEST;

	/* Two typed descriptors over the user-defined point element so resize bookkeeping is exercised on a non-byte payload */
	m_create(point,points);
	m_create(point,zeroed_points);

	/* Both descriptors must enter the test in data mode with zero cached string length */
	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	ASSERT(zeroed_points->string_length == 0);
	ASSERT(zeroed_points->is_string == false);

	/* Exercise an ordinary typed grow and a successful fresh grow with ZERO_NEW_MEMORY */
	ASSERT(SUCCESS == m_resize(points,5));
	ASSERT(SUCCESS == m_resize(zeroed_points,3,ZERO_NEW_MEMORY));

	/* Writable view receives a non-zero surviving payload that later resize calls must preserve */
	point *zeroed_writer = m_data(point,zeroed_points);
	ASSERT(zeroed_writer != NULL);

	IF(zeroed_writer != NULL)
	{
		zeroed_writer[0] = (point){1,1};
	}

	/* Expose and dirty the future tail before hiding it inside the retained allocation */
	ASSERT(SUCCESS == m_resize(zeroed_points,6));

	zeroed_writer = m_data(point,zeroed_points);
	ASSERT(zeroed_writer != NULL);

	IF(zeroed_writer != NULL)
	{
		zeroed_writer[3] = (point){31,32};
		zeroed_writer[4] = (point){41,42};
		zeroed_writer[5] = (point){51,52};
	}

	ASSERT(SUCCESS == m_resize(zeroed_points,3));

	/* Grow with the combined flags must preserve the prefix and clear the retained non-zero tail */
	ASSERT(SUCCESS == m_resize(zeroed_points,6,ZERO_NEW_MEMORY | RELEASE_UNUSED));

	/* Index 0 remains (1,1), while every re-exposed tail point must be all-zero */
	const point *expanded_view = m_data_ro(point,zeroed_points);
	ASSERT(expanded_view != NULL);

	IF(expanded_view != NULL)
	{
		ASSERT(expanded_view[0].x == 1);
		ASSERT(expanded_view[0].y == 1);
		ASSERT(expanded_view[3].x == 0);
		ASSERT(expanded_view[3].y == 0);
		ASSERT(expanded_view[4].x == 0);
		ASSERT(expanded_view[4].y == 0);
		ASSERT(expanded_view[5].x == 0);
		ASSERT(expanded_view[5].y == 0);
	}

	/* Shrink with the same flag combination has to keep the previously written surviving element intact */
	ASSERT(SUCCESS == m_resize(zeroed_points,2,ZERO_NEW_MEMORY | RELEASE_UNUSED));

	/* Logical length now matches the requested two and the surviving element still reads (1,1) */
	const point *shrunk_view = m_data_ro(point,zeroed_points);
	ASSERT(shrunk_view != NULL);

	IF(shrunk_view != NULL)
	{
		ASSERT(zeroed_points->length == 2);
		ASSERT(shrunk_view[0].x == 1);
		ASSERT(shrunk_view[0].y == 1);
	}

	/* Mode invariants survived the full resize chain and m_del releases both descriptors cleanly */
	ASSERT(points->string_length == 0);
	ASSERT(points->is_string == false);
	ASSERT(zeroed_points->string_length == 0);
	ASSERT(zeroed_points->is_string == false);
	call(m_del(points));
	call(m_del(zeroed_points));

	RETURN_STATUS;
}
