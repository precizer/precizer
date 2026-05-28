#include "test_libmem_all.h"

/**
 * @brief Capture cross-type data-wrapper divisibility failures
 *
 * The helper should reject both append and replace when the full source
 * payload leaves a partial destination element tail
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_core_data_non_divisible_cross_type_cases(void)
{
	INITTEST;

	m_create(unsigned char,byte_buffer);
	m_create(unsigned short,word_buffer);
	m_create(mem_core_data_rgb,rgb_buffer);
	m_create(uint32_t,dword_buffer);
	m_create(uint64_t,qword_buffer);

	memory *descriptors[] = {
		byte_buffer,
		word_buffer,
		rgb_buffer,
		dword_buffer,
		qword_buffer
	};

	const unsigned char word_seed[] = {
		(unsigned char)0x10,
		(unsigned char)0x11
	};
	const unsigned char rgb_seed[] = {
		(unsigned char)0x20,
		(unsigned char)0x21,
		(unsigned char)0x22
	};
	const unsigned char dword_seed[] = {
		(unsigned char)0x30,
		(unsigned char)0x31,
		(unsigned char)0x32,
		(unsigned char)0x33
	};
	const unsigned char qword_seed[] = {
		(unsigned char)0x40,
		(unsigned char)0x41,
		(unsigned char)0x42,
		(unsigned char)0x43,
		(unsigned char)0x44,
		(unsigned char)0x45,
		(unsigned char)0x46,
		(unsigned char)0x47
	};
	const unsigned char invalid_byte_source[] = {
		(unsigned char)0x50,
		(unsigned char)0x51,
		(unsigned char)0x52
	};
	const unsigned char invalid_word_source[] = {
		(unsigned char)0x60,
		(unsigned char)0x61,
		(unsigned char)0x62,
		(unsigned char)0x63
	};
	const unsigned char invalid_rgb_source[] = {
		(unsigned char)0x70,
		(unsigned char)0x71,
		(unsigned char)0x72,
		(unsigned char)0x73,
		(unsigned char)0x74,
		(unsigned char)0x75
	};
	const unsigned char invalid_dword_source[] = {
		(unsigned char)0x80,
		(unsigned char)0x81,
		(unsigned char)0x82,
		(unsigned char)0x83
	};
	const unsigned char invalid_qword_source[] = {
		(unsigned char)0x90,
		(unsigned char)0x91,
		(unsigned char)0x92,
		(unsigned char)0x93,
		(unsigned char)0x94,
		(unsigned char)0x95,
		(unsigned char)0x96,
		(unsigned char)0x97
	};

	ASSERT(SUCCESS == set_raw_descriptor_bytes(word_buffer,word_seed,sizeof(word_seed)));
	ASSERT(SUCCESS == set_raw_descriptor_bytes(byte_buffer,invalid_byte_source,sizeof(invalid_byte_source)));
	ASSERT(FAILURE == m_copy_data(word_buffer,byte_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(word_buffer,word_seed,sizeof(word_seed),1));
	ASSERT(FAILURE == m_concat_data(word_buffer,byte_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(word_buffer,word_seed,sizeof(word_seed),1));

	ASSERT(SUCCESS == set_raw_descriptor_bytes(rgb_buffer,rgb_seed,sizeof(rgb_seed)));
	ASSERT(SUCCESS == set_raw_descriptor_bytes(word_buffer,invalid_word_source,sizeof(invalid_word_source)));
	ASSERT(FAILURE == m_copy_data(rgb_buffer,word_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(rgb_buffer,rgb_seed,sizeof(rgb_seed),1));
	ASSERT(FAILURE == m_concat_data(rgb_buffer,word_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(rgb_buffer,rgb_seed,sizeof(rgb_seed),1));

	ASSERT(SUCCESS == set_raw_descriptor_bytes(dword_buffer,dword_seed,sizeof(dword_seed)));
	ASSERT(SUCCESS == set_raw_descriptor_bytes(rgb_buffer,invalid_rgb_source,sizeof(invalid_rgb_source)));
	ASSERT(FAILURE == m_copy_data(dword_buffer,rgb_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(dword_buffer,dword_seed,sizeof(dword_seed),1));
	ASSERT(FAILURE == m_concat_data(dword_buffer,rgb_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(dword_buffer,dword_seed,sizeof(dword_seed),1));

	ASSERT(SUCCESS == set_raw_descriptor_bytes(qword_buffer,qword_seed,sizeof(qword_seed)));
	ASSERT(SUCCESS == set_raw_descriptor_bytes(dword_buffer,invalid_dword_source,sizeof(invalid_dword_source)));
	ASSERT(FAILURE == m_copy_data(qword_buffer,dword_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(qword_buffer,qword_seed,sizeof(qword_seed),1));
	ASSERT(FAILURE == m_concat_data(qword_buffer,dword_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(qword_buffer,qword_seed,sizeof(qword_seed),1));

	ASSERT(SUCCESS == set_raw_descriptor_bytes(rgb_buffer,rgb_seed,sizeof(rgb_seed)));
	ASSERT(SUCCESS == set_raw_descriptor_bytes(qword_buffer,invalid_qword_source,sizeof(invalid_qword_source)));
	ASSERT(FAILURE == m_copy_data(rgb_buffer,qword_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(rgb_buffer,rgb_seed,sizeof(rgb_seed),1));
	ASSERT(FAILURE == m_concat_data(rgb_buffer,qword_buffer));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(rgb_buffer,rgb_seed,sizeof(rgb_seed),1));

	for(size_t descriptor_index = 0; descriptor_index < (sizeof(descriptors) / sizeof(descriptors[0])); ++descriptor_index)
	{
		call(m_del(descriptors[descriptor_index]));
	}

	deliver(status);
}

/**
 * @brief Check data wrappers reject five incompatible cross-type payloads
 *
 * @return Return describing success or failure
 */
Return test_libmem_0062(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0062[] =
	        "\\A.*Source byte count 3 is not divisible by destination element size 2"
	        ".*Source byte count 4 is not divisible by destination element size 3"
	        ".*Source byte count 6 is not divisible by destination element size 4"
	        ".*Source byte count 4 is not divisible by destination element size 8"
	        ".*Source byte count 8 is not divisible by destination element size 3.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0062,
		capture_libmem_core_data_non_divisible_cross_type_cases));

	RETURN_STATUS;
}
