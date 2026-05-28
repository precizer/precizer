#include "test_libmem_all.h"

/**
 * @brief Invoke public data replacement through the operation callback shape
 *
 * @param destination Data descriptor that receives source contents
 * @param source Data descriptor whose contents replace the destination payload
 * @return Return describing success or failure
 */
static Return invoke_m_copy_data(
	memory       *destination,
	const memory *source)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	run(m_copy_data(destination,source));

	provide(status);
}

/**
 * @brief Invoke public data append through the operation callback shape
 *
 * @param destination Data descriptor that receives appended contents
 * @param source Data descriptor whose contents are appended
 * @return Return describing success or failure
 */
static Return invoke_m_concat_data(
	memory       *destination,
	const memory *source)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	run(m_concat_data(destination,source));

	provide(status);
}

/**
 * @brief Check data-wrapper append and copy across five different data types
 *
 * @return Return describing success or failure
 */
Return test_libmem_0061(void)
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

	const unsigned char replace_word_seed[] = {
		(unsigned char)0x01,
		(unsigned char)0x02
	};
	const unsigned char replace_byte_source[] = {
		(unsigned char)0x11,
		(unsigned char)0x12,
		(unsigned char)0x13,
		(unsigned char)0x14
	};
	const unsigned char replace_rgb_seed[] = {
		(unsigned char)0x21,
		(unsigned char)0x22,
		(unsigned char)0x23
	};
	const unsigned char replace_word_source[] = {
		(unsigned char)0x31,
		(unsigned char)0x32,
		(unsigned char)0x33,
		(unsigned char)0x34,
		(unsigned char)0x35,
		(unsigned char)0x36
	};
	const unsigned char replace_dword_seed[] = {
		(unsigned char)0x41,
		(unsigned char)0x42,
		(unsigned char)0x43,
		(unsigned char)0x44
	};
	const unsigned char replace_rgb_source[] = {
		(unsigned char)0x51,
		(unsigned char)0x52,
		(unsigned char)0x53,
		(unsigned char)0x54,
		(unsigned char)0x55,
		(unsigned char)0x56,
		(unsigned char)0x57,
		(unsigned char)0x58,
		(unsigned char)0x59,
		(unsigned char)0x5A,
		(unsigned char)0x5B,
		(unsigned char)0x5C
	};
	const unsigned char replace_qword_seed[] = {
		(unsigned char)0x61,
		(unsigned char)0x62,
		(unsigned char)0x63,
		(unsigned char)0x64,
		(unsigned char)0x65,
		(unsigned char)0x66,
		(unsigned char)0x67,
		(unsigned char)0x68
	};
	const unsigned char replace_dword_source[] = {
		(unsigned char)0x71,
		(unsigned char)0x72,
		(unsigned char)0x73,
		(unsigned char)0x74,
		(unsigned char)0x75,
		(unsigned char)0x76,
		(unsigned char)0x77,
		(unsigned char)0x78
	};
	const unsigned char replace_byte_seed[] = {
		(unsigned char)0x81,
		(unsigned char)0x82
	};
	const unsigned char replace_qword_source[] = {
		(unsigned char)0x91,
		(unsigned char)0x92,
		(unsigned char)0x93,
		(unsigned char)0x94,
		(unsigned char)0x95,
		(unsigned char)0x96,
		(unsigned char)0x97,
		(unsigned char)0x98
	};
	const unsigned char append_word_seed[] = {
		(unsigned char)0xA1,
		(unsigned char)0xA2
	};
	const unsigned char append_byte_source[] = {
		(unsigned char)0xA3,
		(unsigned char)0xA4,
		(unsigned char)0xA5,
		(unsigned char)0xA6
	};
	const unsigned char append_word_expected[] = {
		(unsigned char)0xA1,
		(unsigned char)0xA2,
		(unsigned char)0xA3,
		(unsigned char)0xA4,
		(unsigned char)0xA5,
		(unsigned char)0xA6
	};
	const unsigned char append_rgb_seed[] = {
		(unsigned char)0xB1,
		(unsigned char)0xB2,
		(unsigned char)0xB3
	};
	const unsigned char append_word_source[] = {
		(unsigned char)0xB4,
		(unsigned char)0xB5,
		(unsigned char)0xB6,
		(unsigned char)0xB7,
		(unsigned char)0xB8,
		(unsigned char)0xB9
	};
	const unsigned char append_rgb_expected[] = {
		(unsigned char)0xB1,
		(unsigned char)0xB2,
		(unsigned char)0xB3,
		(unsigned char)0xB4,
		(unsigned char)0xB5,
		(unsigned char)0xB6,
		(unsigned char)0xB7,
		(unsigned char)0xB8,
		(unsigned char)0xB9
	};
	const unsigned char append_dword_seed[] = {
		(unsigned char)0xC1,
		(unsigned char)0xC2,
		(unsigned char)0xC3,
		(unsigned char)0xC4
	};
	const unsigned char append_rgb_source[] = {
		(unsigned char)0xC5,
		(unsigned char)0xC6,
		(unsigned char)0xC7,
		(unsigned char)0xC8,
		(unsigned char)0xC9,
		(unsigned char)0xCA,
		(unsigned char)0xCB,
		(unsigned char)0xCC,
		(unsigned char)0xCD,
		(unsigned char)0xCE,
		(unsigned char)0xCF,
		(unsigned char)0xD0
	};
	const unsigned char append_dword_expected[] = {
		(unsigned char)0xC1,
		(unsigned char)0xC2,
		(unsigned char)0xC3,
		(unsigned char)0xC4,
		(unsigned char)0xC5,
		(unsigned char)0xC6,
		(unsigned char)0xC7,
		(unsigned char)0xC8,
		(unsigned char)0xC9,
		(unsigned char)0xCA,
		(unsigned char)0xCB,
		(unsigned char)0xCC,
		(unsigned char)0xCD,
		(unsigned char)0xCE,
		(unsigned char)0xCF,
		(unsigned char)0xD0
	};
	const unsigned char append_qword_seed[] = {
		(unsigned char)0xD1,
		(unsigned char)0xD2,
		(unsigned char)0xD3,
		(unsigned char)0xD4,
		(unsigned char)0xD5,
		(unsigned char)0xD6,
		(unsigned char)0xD7,
		(unsigned char)0xD8
	};
	const unsigned char append_dword_source[] = {
		(unsigned char)0xD9,
		(unsigned char)0xDA,
		(unsigned char)0xDB,
		(unsigned char)0xDC,
		(unsigned char)0xDD,
		(unsigned char)0xDE,
		(unsigned char)0xDF,
		(unsigned char)0xE0
	};
	const unsigned char append_qword_expected[] = {
		(unsigned char)0xD1,
		(unsigned char)0xD2,
		(unsigned char)0xD3,
		(unsigned char)0xD4,
		(unsigned char)0xD5,
		(unsigned char)0xD6,
		(unsigned char)0xD7,
		(unsigned char)0xD8,
		(unsigned char)0xD9,
		(unsigned char)0xDA,
		(unsigned char)0xDB,
		(unsigned char)0xDC,
		(unsigned char)0xDD,
		(unsigned char)0xDE,
		(unsigned char)0xDF,
		(unsigned char)0xE0
	};
	const unsigned char append_byte_seed[] = {
		(unsigned char)0xE1,
		(unsigned char)0xE2
	};
	const unsigned char append_qword_source[] = {
		(unsigned char)0xE3,
		(unsigned char)0xE4,
		(unsigned char)0xE5,
		(unsigned char)0xE6,
		(unsigned char)0xE7,
		(unsigned char)0xE8,
		(unsigned char)0xE9,
		(unsigned char)0xEA
	};
	const unsigned char append_byte_expected[] = {
		(unsigned char)0xE1,
		(unsigned char)0xE2,
		(unsigned char)0xE3,
		(unsigned char)0xE4,
		(unsigned char)0xE5,
		(unsigned char)0xE6,
		(unsigned char)0xE7,
		(unsigned char)0xE8,
		(unsigned char)0xE9,
		(unsigned char)0xEA
	};

	const mem_core_data_case cases[] = {
		{
			MEM_CORE_DATA_TYPE_WORD,
			MEM_CORE_DATA_TYPE_BYTE,
			invoke_m_copy_data,
			replace_word_seed,
			sizeof(replace_word_seed),
			replace_byte_source,
			sizeof(replace_byte_source),
			replace_byte_source,
			sizeof(replace_byte_source),
			2
		},
		{
			MEM_CORE_DATA_TYPE_RGB,
			MEM_CORE_DATA_TYPE_WORD,
			invoke_m_copy_data,
			replace_rgb_seed,
			sizeof(replace_rgb_seed),
			replace_word_source,
			sizeof(replace_word_source),
			replace_word_source,
			sizeof(replace_word_source),
			2
		},
		{
			MEM_CORE_DATA_TYPE_DWORD,
			MEM_CORE_DATA_TYPE_RGB,
			invoke_m_copy_data,
			replace_dword_seed,
			sizeof(replace_dword_seed),
			replace_rgb_source,
			sizeof(replace_rgb_source),
			replace_rgb_source,
			sizeof(replace_rgb_source),
			3
		},
		{
			MEM_CORE_DATA_TYPE_QWORD,
			MEM_CORE_DATA_TYPE_DWORD,
			invoke_m_copy_data,
			replace_qword_seed,
			sizeof(replace_qword_seed),
			replace_dword_source,
			sizeof(replace_dword_source),
			replace_dword_source,
			sizeof(replace_dword_source),
			1
		},
		{
			MEM_CORE_DATA_TYPE_BYTE,
			MEM_CORE_DATA_TYPE_QWORD,
			invoke_m_copy_data,
			replace_byte_seed,
			sizeof(replace_byte_seed),
			replace_qword_source,
			sizeof(replace_qword_source),
			replace_qword_source,
			sizeof(replace_qword_source),
			sizeof(replace_qword_source)
		},
		{
			MEM_CORE_DATA_TYPE_WORD,
			MEM_CORE_DATA_TYPE_BYTE,
			invoke_m_concat_data,
			append_word_seed,
			sizeof(append_word_seed),
			append_byte_source,
			sizeof(append_byte_source),
			append_word_expected,
			sizeof(append_word_expected),
			3
		},
		{
			MEM_CORE_DATA_TYPE_RGB,
			MEM_CORE_DATA_TYPE_WORD,
			invoke_m_concat_data,
			append_rgb_seed,
			sizeof(append_rgb_seed),
			append_word_source,
			sizeof(append_word_source),
			append_rgb_expected,
			sizeof(append_rgb_expected),
			3
		},
		{
			MEM_CORE_DATA_TYPE_DWORD,
			MEM_CORE_DATA_TYPE_RGB,
			invoke_m_concat_data,
			append_dword_seed,
			sizeof(append_dword_seed),
			append_rgb_source,
			sizeof(append_rgb_source),
			append_dword_expected,
			sizeof(append_dword_expected),
			4
		},
		{
			MEM_CORE_DATA_TYPE_QWORD,
			MEM_CORE_DATA_TYPE_DWORD,
			invoke_m_concat_data,
			append_qword_seed,
			sizeof(append_qword_seed),
			append_dword_source,
			sizeof(append_dword_source),
			append_qword_expected,
			sizeof(append_qword_expected),
			2
		},
		{
			MEM_CORE_DATA_TYPE_BYTE,
			MEM_CORE_DATA_TYPE_QWORD,
			invoke_m_concat_data,
			append_byte_seed,
			sizeof(append_byte_seed),
			append_qword_source,
			sizeof(append_qword_source),
			append_byte_expected,
			sizeof(append_byte_expected),
			sizeof(append_byte_expected)
		}
	};

	for(size_t case_index = 0; case_index < (sizeof(cases) / sizeof(cases[0])); ++case_index)
	{
		ASSERT(SUCCESS == run_mem_core_data_case(descriptors,&cases[case_index]));
	}

	for(size_t descriptor_index = 0; descriptor_index < (sizeof(descriptors) / sizeof(descriptors[0])); ++descriptor_index)
	{
		call(m_del(descriptors[descriptor_index]));
	}

	RETURN_STATUS;
}
