#ifndef LIBMEM_TEST_LIBMEM_UTILS_H
#define LIBMEM_TEST_LIBMEM_UTILS_H

#include "mem.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct point {
	int x;
	int y;
} point;

typedef struct mem_core_data_rgb {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} mem_core_data_rgb;

typedef struct mem_core_data_case {
	size_t destination_index;
	size_t source_index;
	Return (*operation)(
		memory *,
		const memory *);
	const unsigned char *destination_seed_bytes;
	size_t destination_seed_size;
	const unsigned char *source_bytes;
	size_t source_size_bytes;
	const unsigned char *expected_bytes;
	size_t expected_size;
	size_t expected_length;
} mem_core_data_case;

enum
{
	MEM_CORE_DATA_TYPE_BYTE = 0,
	MEM_CORE_DATA_TYPE_WORD = 1,
	MEM_CORE_DATA_TYPE_RGB = 2,
	MEM_CORE_DATA_TYPE_DWORD = 3,
	MEM_CORE_DATA_TYPE_QWORD = 4
};

Return fill_points(memory *points);

Return set_raw_descriptor_bytes(
	memory              *descriptor,
	const unsigned char *bytes,
	size_t              byte_count);

Return expect_raw_descriptor_bytes(
	const memory        *descriptor,
	const unsigned char *expected_bytes,
	size_t              expected_size,
	size_t              expected_length);

Return run_mem_core_data_case(
	memory * const           descriptors[],
	const mem_core_data_case *case_data);
#endif // LIBMEM_TEST_LIBMEM_UTILS_H
