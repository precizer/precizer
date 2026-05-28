#include "test_libmem_all.h"

/**
 * @brief Capture negative cases for data helpers on inconsistent descriptors
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_data_descriptor(void)
{
	INITTEST;

	m_create(int,valid_destination);

	const int sample_values[] = {11,22};
	int sample_storage[2] = {7,9};
	memory invalid_descriptor = m_init(int,MEMORY_DATA);
	memory zero_sized_descriptor = m_init(int,MEMORY_DATA);
	memory phantom_reserve_descriptor = m_init(int,MEMORY_DATA);
	memory undersized_reserve_descriptor = m_init(int,MEMORY_DATA);

	invalid_descriptor.length = 2;
	invalid_descriptor.is_string = true;
	invalid_descriptor.string_length = 1;
	zero_sized_descriptor.single_element_size = 0;
	phantom_reserve_descriptor.actually_allocated_bytes = sizeof(sample_values);
	undersized_reserve_descriptor.data = sample_storage;
	undersized_reserve_descriptor.length = 2;
	undersized_reserve_descriptor.actually_allocated_bytes = sizeof(int);

	ASSERT(NULL == m_data_ro(int,NULL));
	ASSERT(NULL == m_data(int,NULL));
	ASSERT(NULL == m_raw_data_ro(NULL));
	ASSERT(NULL == m_raw_data(NULL));

	ASSERT(NULL == m_data_ro(int,&invalid_descriptor));
	ASSERT(NULL == m_data(int,&invalid_descriptor));
	ASSERT(NULL == m_raw_data_ro(&invalid_descriptor));
	ASSERT(NULL == m_raw_data(&invalid_descriptor));

	ASSERT(NULL == m_data_ro(int,&zero_sized_descriptor));
	ASSERT(NULL == m_data(int,&zero_sized_descriptor));
	ASSERT(NULL == m_raw_data_ro(&zero_sized_descriptor));
	ASSERT(NULL == m_raw_data(&zero_sized_descriptor));

	ASSERT(NULL == m_data_ro(int,&phantom_reserve_descriptor));
	ASSERT(NULL == m_data(int,&phantom_reserve_descriptor));
	ASSERT(NULL == m_raw_data_ro(&phantom_reserve_descriptor));
	ASSERT(NULL == m_raw_data(&phantom_reserve_descriptor));

	ASSERT(NULL == m_data_ro(int,&undersized_reserve_descriptor));
	ASSERT(NULL == m_data(int,&undersized_reserve_descriptor));
	ASSERT(sample_storage == m_raw_data_ro(&undersized_reserve_descriptor));
	ASSERT(sample_storage == m_raw_data(&undersized_reserve_descriptor));

	ASSERT(FAILURE == m_copy_buffer(&invalid_descriptor,sizeof(sample_values),sample_values));
	ASSERT(FAILURE == m_resize(&invalid_descriptor,1));
	ASSERT(FAILURE == m_copy(valid_destination,&invalid_descriptor));
	ASSERT(FAILURE == m_concat_data(valid_destination,&invalid_descriptor));
	ASSERT(FAILURE == m_del(&invalid_descriptor));
	ASSERT(invalid_descriptor.length == 2);
	ASSERT(invalid_descriptor.data == NULL);
	ASSERT(invalid_descriptor.is_string == true);
	ASSERT(invalid_descriptor.string_length == 1);
	ASSERT(invalid_descriptor.actually_allocated_bytes == 0);

	call(m_del(valid_destination));

	deliver(status);
}

/**
 * @brief Check data-facing helpers reject descriptors with NULL data and non-zero length
 *
 * @return Return describing success or failure
 */
Return test_libmem_0055(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0055[] =
	        "\\A.*Descriptor is NULL"
	        ".*Descriptor has non-zero length with NULL data pointer"
	        ".*Descriptor element size is zero \\(uninitialized\\)"
	        ".*Descriptor has reserved bytes with NULL data pointer"
	        ".*Descriptor reserve is smaller than logical payload.*\\Z";

	m_create(int,graceful_descriptor);

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0055,
		capture_libmem_inconsistent_data_descriptor));

	IF(SUCCESS == status)
	{
		Return saved_global_status = SUCCESS;
		int *writable_view = NULL;
		const int *readonly_view = NULL;

		ASSERT(SUCCESS == m_resize(graceful_descriptor,2));
		saved_global_status = atomic_exchange(&global_return_status,INFO);
		writable_view = m_data(int,graceful_descriptor);
		readonly_view = m_data_ro(int,graceful_descriptor);
		atomic_store(&global_return_status,saved_global_status);

		ASSERT(writable_view != NULL);
		ASSERT(readonly_view != NULL);
	}

	call(m_del(graceful_descriptor));

	RETURN_STATUS;
}
