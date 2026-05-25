#include "test_libmem_utils.h"

/**
 * @brief Check raw data append and copy wrappers on ordinary and aliased payloads
 *
 * @return Return describing success or failure
 */
Return test_libmem_0059(void)
{
	INITTEST;

	m_create(unsigned short,destination_words);
	m_create(unsigned char,source_bytes);

	const unsigned char external_prefix[] = {
		(unsigned char)'A',
		(unsigned char)'B',
		(unsigned char)'C',
		(unsigned char)'D'
	};
	const unsigned char external_suffix[] = {
		(unsigned char)'E',
		(unsigned char)'F'
	};
	const unsigned char aliased_append_seed[] = {
		(unsigned char)'1',
		(unsigned char)'2',
		(unsigned char)'3',
		(unsigned char)'4'
	};
	const unsigned char aliased_replace_seed[] = {
		(unsigned char)'a',
		(unsigned char)'b',
		(unsigned char)'c',
		(unsigned char)'d',
		(unsigned char)'e',
		(unsigned char)'f'
	};

	ASSERT(SUCCESS == m_copy_buffer(destination_words,sizeof(external_prefix),external_prefix));
	ASSERT(SUCCESS == m_copy_buffer(source_bytes,sizeof(external_suffix),external_suffix));
	ASSERT(SUCCESS == mem_concat_data(destination_words,source_bytes));

	ASSERT(destination_words->length == 3);
	ASSERT(destination_words->string_length == 0);
	ASSERT(destination_words->is_string == false);

	const unsigned char *destination_view = (const unsigned char *)m_raw_data_ro(destination_words);
	ASSERT(destination_view != NULL);

	IF(destination_view != NULL)
	{
		ASSERT(destination_view[0] == (unsigned char)'A');
		ASSERT(destination_view[1] == (unsigned char)'B');
		ASSERT(destination_view[2] == (unsigned char)'C');
		ASSERT(destination_view[3] == (unsigned char)'D');
		ASSERT(destination_view[4] == (unsigned char)'E');
		ASSERT(destination_view[5] == (unsigned char)'F');
	}

	ASSERT(SUCCESS == m_copy_buffer(destination_words,sizeof(aliased_append_seed),aliased_append_seed));

	memory aliased_append_source = m_init(unsigned char,MEMORY_DATA);
	unsigned char *destination_data = (unsigned char *)m_raw_data(destination_words);
	ASSERT(destination_data != NULL);

	IF(destination_data != NULL)
	{
		aliased_append_source.data = destination_data + 1;
	}

	aliased_append_source.length = 2;

	ASSERT(SUCCESS == mem_concat_data(destination_words,&aliased_append_source));
	ASSERT(destination_words->length == 3);

	destination_view = (const unsigned char *)m_raw_data_ro(destination_words);
	ASSERT(destination_view != NULL);

	IF(destination_view != NULL)
	{
		ASSERT(destination_view[0] == (unsigned char)'1');
		ASSERT(destination_view[1] == (unsigned char)'2');
		ASSERT(destination_view[2] == (unsigned char)'3');
		ASSERT(destination_view[3] == (unsigned char)'4');
		ASSERT(destination_view[4] == (unsigned char)'2');
		ASSERT(destination_view[5] == (unsigned char)'3');
	}

	ASSERT(SUCCESS == m_copy_buffer(destination_words,sizeof(aliased_replace_seed),aliased_replace_seed));

	memory aliased_replace_source = m_init(unsigned char,MEMORY_DATA);
	destination_data = (unsigned char *)m_raw_data(destination_words);
	ASSERT(destination_data != NULL);

	IF(destination_data != NULL)
	{
		aliased_replace_source.data = destination_data + 1;
	}

	aliased_replace_source.length = 4;

	ASSERT(SUCCESS == mem_copy_data(destination_words,&aliased_replace_source));
	ASSERT(destination_words->length == 2);
	ASSERT(destination_words->string_length == 0);
	ASSERT(destination_words->is_string == false);

	destination_view = (const unsigned char *)m_raw_data_ro(destination_words);
	ASSERT(destination_view != NULL);

	IF(destination_view != NULL)
	{
		ASSERT(destination_view[0] == (unsigned char)'b');
		ASSERT(destination_view[1] == (unsigned char)'c');
		ASSERT(destination_view[2] == (unsigned char)'d');
		ASSERT(destination_view[3] == (unsigned char)'e');
	}

	call(m_del(source_bytes));
	call(m_del(destination_words));

	RETURN_STATUS;
}
