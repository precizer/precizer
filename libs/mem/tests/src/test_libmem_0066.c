#include "test_libmem_all.h"

/**
 * @brief Check aliased replacement from the current terminator of the same string
 *
 * The test first stores "abcdef" in a string descriptor. It then takes a source pointer to the current '\0' terminator inside the same descriptor and performs bounded replacement with a size of one byte. This verifies that self-aliased replacement from an empty suffix is handled correctly and leaves the descriptor as a valid empty string
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0066_1(void)
{
	INITTEST;

	static const char abcdef_text[] = "abcdef";

	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(abcdef_text),abcdef_text));

	const char *aliased_string_suffix = m_text(string_buffer);
	ASSERT(aliased_string_suffix != NULL);

	size_t string_buffer_length = 0U;
	ASSERT(SUCCESS == m_string_length(string_buffer,&string_buffer_length));

	IF(aliased_string_suffix != NULL)
	{
		aliased_string_suffix += string_buffer_length;
		ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,1U,aliased_string_suffix));
	}

	ASSERT(string_buffer->length == 1);
	ASSERT(string_buffer->string_length == 0);
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),""));
	call(m_del(string_buffer));

	RETURN_STATUS;
}

/**
 * @brief Check self-copy through m_copy on a string descriptor
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0066_2(void)
{
	INITTEST;

	static const char xyz_text[] = "xyz";

	m_create(char,string_buffer);

	ASSERT(SUCCESS == m_to_string(string_buffer));
	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(xyz_text),xyz_text));
	ASSERT(SUCCESS == m_copy(string_buffer,string_buffer));
	ASSERT(string_buffer->length == 4);
	ASSERT(string_buffer->string_length == 3);
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),"xyz"));
	call(m_del(string_buffer));

	RETURN_STATUS;
}

/**
 * @brief Check aliased raw-buffer replacement and data self-copy
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0066_3(void)
{
	INITTEST;

	m_create(char,raw_buffer);

	const unsigned char raw_bytes[] = {
		(unsigned char)'a',
		(unsigned char)'b',
		(unsigned char)'c',
		(unsigned char)'d',
		(unsigned char)'e',
		(unsigned char)'f'
	};
	const unsigned char expected_raw_tail[] = {
		(unsigned char)'c',
		(unsigned char)'d',
		(unsigned char)'e',
		(unsigned char)'f'
	};

	ASSERT(SUCCESS == m_copy_buffer(raw_buffer,sizeof(raw_bytes),raw_bytes));

	const unsigned char *aliased_raw_suffix = (const unsigned char *)m_raw_data_ro(raw_buffer);
	ASSERT(aliased_raw_suffix != NULL);

	IF(aliased_raw_suffix != NULL)
	{
		aliased_raw_suffix += 2;
		ASSERT(SUCCESS == m_copy_buffer(
			raw_buffer,
			sizeof(raw_bytes) - 2U,
			aliased_raw_suffix));
	}

	ASSERT(raw_buffer->length == sizeof(expected_raw_tail));
	ASSERT(raw_buffer->string_length == 0);
	ASSERT(raw_buffer->is_string == false);
	ASSERT(SUCCESS == m_copy(raw_buffer,raw_buffer));

	const unsigned char *raw_view = (const unsigned char *)m_raw_data_ro(raw_buffer);
	ASSERT(raw_view != NULL);

	IF(raw_view != NULL)
	{
		ASSERT(0 == memcmp(raw_view,expected_raw_tail,sizeof(expected_raw_tail)));
	}

	call(m_del(raw_buffer));

	RETURN_STATUS;
}

/**
 * @brief Check aliased raw-buffer core replacement and clearing
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0066_4(void)
{
	INITTEST;

	m_create(char,raw_core_buffer);

	const unsigned char raw_bytes[] = {
		(unsigned char)'a',
		(unsigned char)'b',
		(unsigned char)'c',
		(unsigned char)'d',
		(unsigned char)'e',
		(unsigned char)'f'
	};
	const unsigned char expected_raw_core_tail[] = {
		(unsigned char)'b',
		(unsigned char)'c',
		(unsigned char)'d',
		(unsigned char)'e'
	};

	ASSERT(SUCCESS == m_copy_buffer(raw_core_buffer,sizeof(raw_bytes),raw_bytes));

	const unsigned char *aliased_core_suffix = (const unsigned char *)m_raw_data_ro(raw_core_buffer);
	ASSERT(aliased_core_suffix != NULL);

	IF(aliased_core_suffix != NULL)
	{
		aliased_core_suffix += 1;
		ASSERT(SUCCESS == m_copy_buffer(
			raw_core_buffer,
			sizeof(raw_bytes) - 2U,
			aliased_core_suffix));
	}

	ASSERT(raw_core_buffer->length == sizeof(expected_raw_core_tail));
	ASSERT(raw_core_buffer->string_length == 0);
	ASSERT(raw_core_buffer->is_string == false);

	const unsigned char *raw_core_view = (const unsigned char *)m_raw_data_ro(raw_core_buffer);
	ASSERT(raw_core_view != NULL);

	IF(raw_core_view != NULL)
	{
		ASSERT(0 == memcmp(
			raw_core_view,
			expected_raw_core_tail,
			sizeof(expected_raw_core_tail)));
	}

	ASSERT(SUCCESS == m_copy_buffer(raw_core_buffer,0,NULL));
	ASSERT(raw_core_buffer->length == 0);
	ASSERT(raw_core_buffer->string_length == 0);
	ASSERT(raw_core_buffer->is_string == false);
	call(m_del(raw_core_buffer));

	RETURN_STATUS;
}

/**
 * @brief Run aliasing scenarios for string and data descriptors
 *
 * @return Return describing success or failure
 */
Return test_libmem_0066(void)
{
	INITTEST;

	TEST(test_libmem_0066_1,"Aliased string replacement from the current terminator");
	TEST(test_libmem_0066_2,"m_copy preserves string self-copy");
	TEST(test_libmem_0066_3,"Aliased raw-buffer replacement preserves data self-copy");
	TEST(test_libmem_0066_4,"Aliased raw-buffer replacement supports clearing");

	RETURN_STATUS;
}
