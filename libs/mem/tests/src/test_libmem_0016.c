#include "test_libmem_all.h"

/**
 * @brief Verify fixed-size string copy and append helpers
 *
 * Copies a fixed-size `alpha` string into a string descriptor, then
 * appends the fixed-size `-beta` suffix. Fixed-size mode trusts that
 * the final element inside each source array is the terminator, so this
 * test checks the resulting text and both cached string counters
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0016_1(void)
{
	INITTEST;

	m_create(char,string_buffer,MEMORY_STRING);

	const char alpha_text[] = "alpha";
	const char beta_suffix[] = "-beta";
	static const char expected[] = "alpha-beta";

	/* Replace the empty descriptor with one fixed-size source */
	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(alpha_text),alpha_text));
	ASSERT(string_buffer->string_length == sizeof(alpha_text) - 1);
	ASSERT(string_buffer->length == sizeof(alpha_text));
	ASSERT(0 == strcmp(m_text(string_buffer),alpha_text));

	/* Append another fixed-size source and verify the complete string view */
	ASSERT(SUCCESS == m_concat_fixed_string(string_buffer,sizeof(beta_suffix),beta_suffix));
	ASSERT(string_buffer->string_length == sizeof(expected) - 1);
	ASSERT(string_buffer->length == sizeof(expected));
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),expected));

	call(m_del(string_buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify bounded string copy stops at the first terminator
 *
 * Copies from a bounded source buffer that contains `-gamma`, a zero
 * terminator, and one extra byte after that terminator. The copied
 * descriptor must expose only the visible `-gamma` text and must not
 * include the trailing byte in its logical string length
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0016_2(void)
{
	INITTEST;

	m_create(char,extra_buffer,MEMORY_STRING);

	const char bounded_gamma[] = {'-','g','a','m','m','a','\0','x'};
	static const char expected[] = "-gamma";

	/* The three-argument m_copy_string form routes to the bounded scanner */
	ASSERT(SUCCESS == m_copy_string(extra_buffer,sizeof(bounded_gamma),bounded_gamma));
	ASSERT(extra_buffer->string_length == sizeof(expected) - 1);
	ASSERT(extra_buffer->length == sizeof(expected));
	ASSERT(extra_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(extra_buffer),expected));

	call(m_del(extra_buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify descriptor-to-descriptor append uses cached source string length
 *
 * Appends a managed string descriptor into another managed string
 * descriptor. The source is deliberately grown after receiving
 * `+delta`, so its logical descriptor length is larger than the
 * visible payload. m_concat_strings must append only the cached visible
 * source prefix plus one terminator, ignoring any reserved tail
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0016_3(void)
{
	INITTEST;

	m_create(char,string_buffer,MEMORY_STRING);
	m_create(char,extra_buffer,MEMORY_STRING);

	static const char alpha_beta_text[] = "alpha-beta";
	static const char plus_delta_text[] = "+delta";
	static const char expected[] = "alpha-beta+delta";

	/* Prepare the destination and source as managed string descriptors */
	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(alpha_beta_text),alpha_beta_text));
	ASSERT(SUCCESS == m_copy_fixed_string(extra_buffer,sizeof(plus_delta_text),plus_delta_text));

	/* Grow the source descriptor to prove concat uses string_length rather than source length */
	ASSERT(SUCCESS == m_resize(extra_buffer,16));
	ASSERT(extra_buffer->string_length == sizeof(plus_delta_text) - 1);
	ASSERT(extra_buffer->length == 16);

	/* Append from descriptor to descriptor and verify that the source reserve tail was ignored */
	ASSERT(SUCCESS == m_concat_strings(string_buffer,extra_buffer));
	ASSERT(string_buffer->string_length == sizeof(expected) - 1);
	ASSERT(string_buffer->length == sizeof(expected));
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),expected));

	call(m_del(string_buffer));
	call(m_del(extra_buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify bounded string append stops at the first terminator
 *
 * Starts from the combined `alpha-beta-gamma+delta` text and appends a
 * bounded `-epsilon` source that has two extra bytes after its
 * terminator. The final descriptor must contain only the visible
 * bounded suffix and keep length/string_length coherent
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0016_4(void)
{
	INITTEST;

	m_create(char,string_buffer,MEMORY_STRING);

	static const char prefix[] = "alpha-beta-gamma+delta";
	const char bounded_suffix[] = {'-','e','p','s','i','l','o','n','\0','x','x'};
	static const char expected[] = "alpha-beta-gamma+delta-epsilon";

	/* Seed the descriptor with the prefix that the bounded append should extend */
	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(prefix),prefix));
	ASSERT(string_buffer->string_length == sizeof(prefix) - 1);
	ASSERT(string_buffer->length == sizeof(prefix));

	/* The three-argument m_concat_string form routes to the bounded scanner */
	ASSERT(SUCCESS == m_concat_string(string_buffer,sizeof(bounded_suffix),bounded_suffix));
	ASSERT(string_buffer->string_length == sizeof(expected) - 1);
	ASSERT(string_buffer->length == sizeof(expected));
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),expected));

	call(m_del(string_buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify string copy and concatenation helper combinations
 *
 * In plain terms, this suite checks the string-transfer helpers used to
 * build the example text `alpha-beta-gamma+delta-epsilon`. The nested
 * tests keep each source mode separate: fixed-size copy and append,
 * bounded copy that ignores bytes after the first terminator,
 * descriptor-to-descriptor append that trusts cached string_length, and
 * bounded append that also ignores trailing bytes after a terminator.
 * Each step verifies both the visible text and the descriptor metadata
 * that makes later string operations safe
 *
 * @return Return describing success or failure
 */
Return test_libmem_0016(void)
{
	INITTEST;

	TEST(test_libmem_0016_1,"Fixed-size string copy and append build alpha-beta");
	TEST(test_libmem_0016_2,"Bounded string copy ignores bytes after the terminator");
	TEST(test_libmem_0016_3,"Descriptor string append uses cached source length");
	TEST(test_libmem_0016_4,"Bounded string append ignores bytes after the terminator");

	RETURN_STATUS;
}
