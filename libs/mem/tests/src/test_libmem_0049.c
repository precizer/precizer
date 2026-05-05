#include "test_libmem_utils.h"

/**
 * @brief Truncate a string through direct buffer access and let the helper add the terminator
 *
 * The descriptor is created as a string, modified via the writable pointer
 * returned from m_data(...), and then finalized with a shorter length derived
 * from the descriptor's own cached string_length. The terminator is materialized
 * by m_finalize_string through WRITE_TERMINATOR_ALWAYS
 *
 * @return Return describing success or failure
 */
	static const char drafting_text[] = "drafting";
Return test_libmem_0049(void)
{
	INITTEST;

	m_create(char,title,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_fixed_string(title,sizeof(drafting_text),drafting_text));
	ASSERT(title->string_length == 8);
	ASSERT(title->is_string == true);

	char *title_view = m_data(char,title);
	ASSERT(title_view != NULL);

	const size_t truncated_length = title->string_length - 3U;

	if(title_view != NULL)
	{
		memcpy(title_view,"hello",truncated_length);
	}

	ASSERT(SUCCESS == m_finalize_string(title,truncated_length,WRITE_TERMINATOR_ALWAYS));
	ASSERT(title->string_length == 5);
	ASSERT(title->is_string == true);
	ASSERT(0 == strcmp(m_text(title),"hello"));
	ASSERT(SUCCESS == m_del(title));

	RETURN_STATUS;
}
