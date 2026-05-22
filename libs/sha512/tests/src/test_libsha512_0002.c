#include "test_libsha512_utils.h"

/**
 * @brief Check that callers can hash a message in several updates
 * @details Real callers often read data in pieces from files or streams. This
 * test proves that feeding the same message in chunks produces the same digest
 * as hashing the whole message in one call
 *
 * @return SUCCESS when whole-message and chunked hashing produce one digest
 */
Return test_libsha512_0002(void)
{
	INITTEST;

	/* Pick a familiar 43-byte pangram so the test data is easy to recognize */
	static const char message_text[] = "The quick brown fox jumps over the lazy dog";

	/* Several deliberately different chunk plans for the same 43-byte message.
	   Each plan sums to 43, so applying any of them through sha512_update must
	   reproduce the one-shot reference digest. Together they cover obvious
	   pathological shapes: a single big update, an irregular Fibonacci-like
	   split, a heavy-front/tiny-tail split, and one update per byte */
	static const size_t plan_singleton[]      = {43U};
	static const size_t plan_fibonacci[]      = {1U,2U,3U,5U,8U,13U,11U};
	static const size_t plan_skewed[]         = {40U,2U,1U};
	static const size_t plan_byte_at_a_time[] = {
		1U,1U,1U,1U,1U,1U,1U,1U,1U,1U,
		1U,1U,1U,1U,1U,1U,1U,1U,1U,1U,
		1U,1U,1U,1U,1U,1U,1U,1U,1U,1U,
		1U,1U,1U,1U,1U,1U,1U,1U,1U,1U,
		1U,1U,1U
	};

	const struct {
		const size_t *sizes;
		size_t        count;
	} chunk_plans[] = {
		{ plan_singleton,      sizeof(plan_singleton)      / sizeof(plan_singleton[0])      },
		{ plan_fibonacci,      sizeof(plan_fibonacci)      / sizeof(plan_fibonacci[0])      },
		{ plan_skewed,         sizeof(plan_skewed)         / sizeof(plan_skewed[0])         },
		{ plan_byte_at_a_time, sizeof(plan_byte_at_a_time) / sizeof(plan_byte_at_a_time[0]) }
	};

	size_t message_length = 0U;

	/* Allocate all test data through libmem, matching the style used by the
	   other library test suites */
	m_create(char,message,MEMORY_STRING);
	m_create(size_t,chunks);
	m_create(unsigned char,one_shot_digest);
	m_create(unsigned char,chunked_digest);

	/* Prepare the message, resolve its libmem-tracked length through the public
	   helper, and compute the one-shot reference digest that every chunked
	   variant below must reproduce byte-for-byte */
	ASSERT(SUCCESS == m_copy_fixed_string(message,sizeof(message_text),message_text));
	ASSERT(SUCCESS == m_string_length(message,&message_length));
	ASSERT(SUCCESS == calculate_sha512_digest(
		(const unsigned char *)m_text(message),
		message_length,
		one_shot_digest));

	/* Replay the message through every chunk plan. m_copy_buffer rewrites the
	   chunks descriptor from scratch on each iteration, so plans are isolated
	   from one another */
	for(size_t plan_index = 0U; SUCCESS == status && plan_index < sizeof(chunk_plans) / sizeof(chunk_plans[0]); plan_index++)
	{
		ASSERT(SUCCESS == m_copy_buffer(
			chunks,
			chunk_plans[plan_index].count * sizeof(size_t),
			chunk_plans[plan_index].sizes));
		ASSERT(SUCCESS == calculate_sha512_digest_in_chunks(
			(const unsigned char *)m_text(message),
			message_length,
			chunks,
			chunked_digest));
		ASSERT(SUCCESS == assert_sha512_digest_matches(one_shot_digest,chunked_digest));
	}

	/* Release every descriptor through call() so cleanup still runs after an
	   earlier assertion marks the test as failed */
	call(m_del(message));
	call(m_del(chunks));
	call(m_del(one_shot_digest));
	call(m_del(chunked_digest));

	RETURN_STATUS;
}
