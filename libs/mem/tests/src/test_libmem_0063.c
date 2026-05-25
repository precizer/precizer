#include "test_libmem_utils.h"

/**
 * @brief Check same-mode descriptor-to-descriptor transfers
 *
 * @return Return describing success or failure
 */
Return test_libmem_0063(void)
{
	INITTEST;

	static const char seed_text[] = "seed";
	static const char xyz_text[] = "xyz";
	static const char ab_text[] = "ab";
	static const char cd_text[] = "cd";

	m_create(char,source);
	ASSERT(SUCCESS == m_to_string(source));
	m_create(char,destination);
	ASSERT(SUCCESS == m_to_string(destination));
	m_create(char,left_binary);
	m_create(char,right_binary);

	const char left_binary_bytes[] = {'a','b','\0'};
	const char right_binary_bytes[] = {'c','d','\0'};

	ASSERT(SUCCESS == m_copy_fixed_string(destination,sizeof(seed_text),seed_text));
	ASSERT(SUCCESS == m_copy_fixed_string(source,sizeof(xyz_text),xyz_text));
	ASSERT(SUCCESS == m_resize(source,32));
	ASSERT(source->string_length == 3);
	ASSERT(source->length == 32);
	ASSERT(SUCCESS == m_copy(destination,source));
	ASSERT(destination->string_length == 3);
	ASSERT(destination->is_string == true);
	ASSERT(destination->length == 4);
	ASSERT(0 == strcmp(m_text(destination),"xyz"));

	ASSERT(SUCCESS == m_copy_fixed_string(source,sizeof(cd_text),cd_text));
	ASSERT(SUCCESS == m_copy_fixed_string(destination,sizeof(ab_text),ab_text));
	ASSERT(SUCCESS == m_concat_strings(destination,source));
	ASSERT(destination->string_length == 4);
	ASSERT(destination->is_string == true);
	ASSERT(0 == strcmp(m_text(destination),"abcd"));

	ASSERT(SUCCESS == m_copy_fixed_string(source,sizeof(cd_text),cd_text));
	ASSERT(SUCCESS == m_resize(source,32));
	ASSERT(source->string_length == 2);
	ASSERT(source->length == 32);
	ASSERT(SUCCESS == m_copy_fixed_string(destination,sizeof(ab_text),ab_text));
	ASSERT(SUCCESS == mem_concat_strings(destination,source));
	ASSERT(destination->string_length == 4);
	ASSERT(destination->is_string == true);
	ASSERT(destination->length == 5);
	ASSERT(0 == strcmp(m_text(destination),"abcd"));

	ASSERT(SUCCESS == m_copy_buffer(left_binary,sizeof(left_binary_bytes),left_binary_bytes));
	ASSERT(SUCCESS == m_copy_buffer(right_binary,sizeof(right_binary_bytes),right_binary_bytes));
	ASSERT(left_binary->string_length == 0);
	ASSERT(left_binary->is_string == false);
	ASSERT(right_binary->string_length == 0);
	ASSERT(right_binary->is_string == false);
	ASSERT(SUCCESS == m_concat_data(left_binary,right_binary));
	ASSERT(left_binary->string_length == 0);
	ASSERT(left_binary->is_string == false);
	ASSERT(left_binary->length == sizeof(left_binary_bytes) + sizeof(right_binary_bytes));

	ASSERT(SUCCESS == m_copy(right_binary,left_binary));
	ASSERT(right_binary->string_length == 0);
	ASSERT(right_binary->is_string == false);
	ASSERT(right_binary->length == left_binary->length);

	const unsigned char *binary_view = (const unsigned char *)m_raw_data_ro(left_binary);
	ASSERT(binary_view != NULL);

	IF(binary_view != NULL)
	{
		ASSERT(binary_view[0] == (unsigned char)'a');
		ASSERT(binary_view[1] == (unsigned char)'b');
		ASSERT(binary_view[2] == (unsigned char)'\0');
		ASSERT(binary_view[3] == (unsigned char)'c');
		ASSERT(binary_view[4] == (unsigned char)'d');
		ASSERT(binary_view[5] == (unsigned char)'\0');
	}

	const unsigned char *right_binary_view = (const unsigned char *)m_raw_data_ro(right_binary);
	ASSERT(right_binary_view != NULL);

	IF(right_binary_view != NULL)
	{
		ASSERT(right_binary_view[0] == (unsigned char)'a');
		ASSERT(right_binary_view[1] == (unsigned char)'b');
		ASSERT(right_binary_view[2] == (unsigned char)'\0');
		ASSERT(right_binary_view[3] == (unsigned char)'c');
		ASSERT(right_binary_view[4] == (unsigned char)'d');
		ASSERT(right_binary_view[5] == (unsigned char)'\0');
	}

	call(m_del(source));
	call(m_del(destination));
	call(m_del(left_binary));
	call(m_del(right_binary));

	RETURN_STATUS;
}
