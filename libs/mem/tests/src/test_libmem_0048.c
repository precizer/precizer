#include "test_libmem_utils.h"

/**
 * @brief Truncate a string through direct buffer access using the cached length
 *
 * The descriptor is created as a string, modified via the writable pointer
 * returned from m_data(...), and then finalized with a shorter length derived
 * from the descriptor's own cached string_length. The terminator is written
 * by the test before finalization, so the default flag is used
 *
 * @return Return describing success or failure
 */
Return test_libmem_0048(void)
{
	INITTEST;

	static const char drafting_text[] = "drafting";
	m_create(char,title,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_fixed_string(title,sizeof(drafting_text),drafting_text));
	ASSERT(title->string_length == 8);
	ASSERT(title->is_string == true);

	char *title_view = m_data(char,title);
	ASSERT(title_view != NULL);

	size_t title_length = 0U;
	ASSERT(SUCCESS == m_string_length(title,&title_length));

	const size_t truncated_length = title_length - 3U;

	IF(title_view != NULL)
	{
		title_view[truncated_length] = '\0';
	}

	ASSERT(SUCCESS == m_finalize_string(title,truncated_length));
	ASSERT(title->string_length == 5);
	ASSERT(title->is_string == true);
	ASSERT(0 == strcmp(m_text(title),"draft"));
	call(m_del(title));

	RETURN_STATUS;
}
