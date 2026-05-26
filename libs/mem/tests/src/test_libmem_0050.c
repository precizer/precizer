#include "test_libmem_utils.h"

/**
 * @brief Check m_finalize_string with the default flag after a complete direct write
 *
 * Reserves a string buffer, writes a complete zero-terminated text value through
 * m_data(...), and finalizes the visible length without forcing a replacement
 * terminator. The assertions verify that the already present terminator is
 * accepted and the descriptor exposes the written text as a valid string
 *
 * @return Return describing success or failure
 */
Return test_libmem_0050(void)
{
	INITTEST;

	m_create(char,title,MEMORY_STRING);

	const char draft[] = "draft";

	ASSERT(SUCCESS == m_resize(title,sizeof(draft)));

	char *title_view = m_data(char,title);
	ASSERT(title_view != NULL);

	IF(title_view != NULL)
	{
		memcpy(title_view,draft,sizeof(draft));
	}

	ASSERT(SUCCESS == m_finalize_string(title,sizeof(draft) - 1U));
	ASSERT(title->length == sizeof(draft));
	ASSERT(title->string_length == strlen("draft"));
	ASSERT(title->is_string == true);
	ASSERT(0 == strcmp(m_text(title),"draft"));
	call(m_del(title));

	RETURN_STATUS;
}
