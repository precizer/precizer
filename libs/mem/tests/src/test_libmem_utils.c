#include "test_libmem_utils.h"

/**
 * @brief Prints SHA-512 hash in hexadecimal format
 * @details Outputs each byte of the hash as a two-digit hexadecimal number
 *
 * @param hash Pointer to SHA-512 hash array to be printed
 *
 * @note Only compiled when SHOW_TEST is set to 1
 * @note Requires STDERR to be properly initialized
 */
void print_hash(const unsigned char *hash)
{
	for(int i = 0; i < SHA512_DIGEST_LENGTH; i++)
	{
		echo(STDERR,"%02x",hash[i]);
	}
	echo(STDERR,"\n");
}

/**
 * @brief Fill the descriptor with sequential point data
 *
 * @param points Descriptor to populate
 * @return Return describing success or failure
 */
Return fill_points(memory *points)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	point *typed_points = m_data(point,points);

	if(typed_points == NULL)
	{
		status = FAILURE;
	} else {
		for(size_t point_index = 0; point_index < points->length; ++point_index)
		{
			typed_points[point_index] = (point){
				(int)(point_index * 2 + 1),
				(int)(point_index * 2 + 2)
			};
		}
	}

	provide(status);
}

/**
 * @brief Load exact bytes into a data descriptor
 *
 * @param descriptor Destination descriptor in data mode
 * @param bytes Exact source bytes
 * @param byte_count Number of bytes to import
 * @return Return describing success or failure
 */
Return set_raw_descriptor_bytes(
	memory              *descriptor,
	const unsigned char *bytes,
	size_t              byte_count)
{
	Return status = SUCCESS;
	int failed_line = 0;

	ASSERT(SUCCESS == m_copy_buffer(descriptor,byte_count,bytes));

	(void)failed_line;
	return(status);
}

/**
 * @brief Check raw descriptor state against an expected byte sequence
 *
 * @param descriptor Descriptor to validate
 * @param expected_bytes Expected raw payload bytes
 * @param expected_size Expected raw payload size in bytes
 * @param expected_length Expected descriptor length in destination elements
 * @return Return describing success or failure
 */
Return expect_raw_descriptor_bytes(
	const memory        *descriptor,
	const unsigned char *expected_bytes,
	size_t              expected_size,
	size_t              expected_length)
{
	Return status = SUCCESS;
	int failed_line = 0;

	size_t actual_size = 0;

	ASSERT(descriptor != NULL);
	ASSERT(descriptor->length == expected_length);
	ASSERT(descriptor->string_length == 0);
	ASSERT(descriptor->is_string == false);
	ASSERT(SUCCESS == m_guarded_byte_size(descriptor,descriptor->length,&actual_size));
	ASSERT(actual_size == expected_size);

	if(expected_size > 0)
	{
		const unsigned char *actual_bytes = (const unsigned char *)m_raw_data_ro(descriptor);
		ASSERT(actual_bytes != NULL);

		if(actual_bytes != NULL)
		{
			ASSERT(0 == memcmp(actual_bytes,expected_bytes,expected_size));
		}
	}

	(void)failed_line;
	return(status);
}

/**
 * @brief Run one raw cross-type data-wrapper scenario
 *
 * @param descriptors Descriptor table indexed by the local type enum
 * @param case_data Scenario description
 * @return Return describing success or failure
 */
Return run_mem_core_data_case(
	memory * const           descriptors[],
	const mem_core_data_case *case_data)
{
	Return status = SUCCESS;
	int failed_line = 0;

	ASSERT(descriptors != NULL);
	ASSERT(case_data != NULL);
	ASSERT(SUCCESS == set_raw_descriptor_bytes(
		descriptors[case_data->destination_index],
		case_data->destination_seed_bytes,
		case_data->destination_seed_size));
	ASSERT(SUCCESS == set_raw_descriptor_bytes(
		descriptors[case_data->source_index],
		case_data->source_bytes,
		case_data->source_size_bytes));
	ASSERT(SUCCESS == case_data->operation(
		descriptors[case_data->destination_index],
		descriptors[case_data->source_index]));
	ASSERT(SUCCESS == expect_raw_descriptor_bytes(
		descriptors[case_data->destination_index],
		case_data->expected_bytes,
		case_data->expected_size,
		case_data->expected_length));

	(void)failed_line;
	return(status);
}
