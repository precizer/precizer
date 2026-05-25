#include "test_libmem_utils.h"

/**
 * @brief Verify empty descriptor state produced by m_create
 *
 * Creates one default data descriptor, one byte string descriptor, and
 * one uint32_t string descriptor. The checks make sure that m_create
 * sets the element width, mode flag, empty logical length, empty cached
 * string length, zero reserve, and NULL data pointer consistently for
 * every initial mode used by the public macro
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0013_1(void)
{
	INITTEST;

	/* Cover the default data mode and explicit string mode for byte and wide elements */
	m_create(char,data_buffer);
	m_create(char,string_buffer,MEMORY_STRING);
	m_create(uint32_t,wide_string,MEMORY_STRING);

	/* Validate the empty data descriptor created by the default macro form */
	ASSERT(data_buffer->single_element_size == sizeof(char));
	ASSERT(data_buffer->actually_allocated_bytes == 0);
	ASSERT(data_buffer->length == 0);
	ASSERT(data_buffer->string_length == 0);
	ASSERT(data_buffer->is_string == false);
	ASSERT(data_buffer->data == NULL);

	/* Validate the empty byte string descriptor created with MEMORY_STRING */
	ASSERT(string_buffer->single_element_size == sizeof(char));
	ASSERT(string_buffer->actually_allocated_bytes == 0);
	ASSERT(string_buffer->length == 0);
	ASSERT(string_buffer->string_length == 0);
	ASSERT(string_buffer->is_string == true);
	ASSERT(string_buffer->data == NULL);

	/* Validate the empty wide string descriptor created with the same MEMORY_STRING flag */
	ASSERT(wide_string->single_element_size == sizeof(uint32_t));
	ASSERT(wide_string->actually_allocated_bytes == 0);
	ASSERT(wide_string->length == 0);
	ASSERT(wide_string->string_length == 0);
	ASSERT(wide_string->is_string == true);
	ASSERT(wide_string->data == NULL);

	call(m_del(wide_string));
	call(m_del(string_buffer));
	call(m_del(data_buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify fixed-size append for a byte string descriptor
 *
 * Starts with an empty MEMORY_STRING char descriptor, appends the
 * fixed-size "abc" literal including its terminator, and checks both
 * string counters. The typed read-only view then confirms that the
 * visible bytes and the trailing zero reached descriptor storage in
 * the expected order
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0013_2(void)
{
	INITTEST;

	m_create(char,string_buffer,MEMORY_STRING);

	/* Fixed-string mode trusts that the last element in this array is the terminator */
	const char literal_suffix[] = "abc";

	/* Confirm the byte string starts empty before append */
	ASSERT(string_buffer->is_string == true);
	ASSERT(string_buffer->string_length == 0);
	ASSERT(string_buffer->length == 0);

	/* Append three visible characters plus the trailing terminator */
	ASSERT(SUCCESS == m_concat_fixed_string(string_buffer,sizeof(literal_suffix),literal_suffix));
	ASSERT(string_buffer->string_length == 3);
	ASSERT(string_buffer->length == 4);

	/* Read back the full fixed-string payload, including the terminator slot */
	const char *string_view = m_data_ro(char,string_buffer);
	ASSERT(string_view != NULL);

	IF(string_view != NULL)
	{
		ASSERT(string_view[0] == 'a');
		ASSERT(string_view[1] == 'b');
		ASSERT(string_view[2] == 'c');
		ASSERT(string_view[3] == '\0');
	}

	call(m_del(string_buffer));

	RETURN_STATUS;
}

/**
 * @brief Verify fixed-size append for a uint32_t string descriptor
 *
 * Starts with an empty MEMORY_STRING uint32_t descriptor, appends a
 * three-element fixed-size source whose last element is the wide
 * terminator, and checks that string_length counts only visible
 * elements while length includes the terminator slot. Reading the
 * terminator as a full uint32_t covers the all-bytes-zero requirement
 * for multi-byte string elements
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0013_3(void)
{
	INITTEST;

	m_create(uint32_t,wide_string,MEMORY_STRING);

	/* The last uint32_t element is the terminator trusted by fixed-string mode */
	const uint32_t wide_literal_suffix[] = {
		UINT32_C(10),
		UINT32_C(20),
		UINT32_C(0)
	};

	/* Confirm the wide string starts empty before append */
	ASSERT(wide_string->is_string == true);
	ASSERT(wide_string->string_length == 0);
	ASSERT(wide_string->length == 0);

	/* Append two visible uint32_t elements plus the wide terminator */
	ASSERT(SUCCESS == m_concat_fixed_string(wide_string,sizeof(wide_literal_suffix),wide_literal_suffix));
	ASSERT(wide_string->string_length == 2);
	ASSERT(wide_string->length == 3);

	/* Read back the full wide payload, including the all-zero terminator element */
	const uint32_t *wide_view = m_data_ro(uint32_t,wide_string);
	ASSERT(wide_view != NULL);

	IF(wide_view != NULL)
	{
		ASSERT(wide_view[0] == UINT32_C(10));
		ASSERT(wide_view[1] == UINT32_C(20));
		ASSERT(wide_view[2] == UINT32_C(0));
	}

	call(m_del(wide_string));

	RETURN_STATUS;
}

/**
 * @brief Verify m_create initial modes and fixed-size string append behavior
 *
 * In plain terms, this suite checks two related things. First, m_create
 * must build empty descriptors with the right mode, element size, length
 * fields, reserve, and data pointer. Second, descriptors created in
 * MEMORY_STRING mode must be usable immediately with
 * m_concat_fixed_string for both ordinary byte strings and wider
 * uint32_t string elements. The nested tests keep the creation contract
 * separate from byte and wide fixed-string append behavior, so failures
 * point at the broken layer instead of one large mixed scenario
 *
 * @return Return describing success or failure
 */
Return test_libmem_0013(void)
{
	INITTEST;

	TEST(test_libmem_0013_1,"m_create initializes empty data and string descriptors…");
	TEST(test_libmem_0013_2,"m_concat_fixed_string appends a byte string literal…");
	TEST(test_libmem_0013_3,"m_concat_fixed_string appends a uint32_t fixed string…");

	RETURN_STATUS;
}
