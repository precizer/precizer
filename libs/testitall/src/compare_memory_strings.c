#include "testitall.h"

/**
 * @brief Compare two managed byte strings and store the unified diff in a descriptor
 *
 * This helper is the descriptor-based counterpart to @ref compare_strings.
 * The input descriptors are interpreted as byte-sized C strings through
 * @ref m_text, and the resulting unified diff is copied into @p diff
 * through the public string helpers from `libmem`
 *
 * The destination descriptor may start either in data mode or in string mode.
 * On success it contains the produced diff as a byte-sized string descriptor
 *
 * @param[out] diff Destination descriptor receiving the unified diff text
 * @param[in] string1 First managed string to compare
 * @param[in] string2 Second managed string to compare
 * @return `SUCCESS` on success or `FAILURE` on invalid arguments or allocation errors
 */
Return compare_memory_strings(
	memory       *diff,
	const memory *string1,
	const memory *string2)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Heap-allocated unified diff returned by compare_strings() */
	char *diff_text = NULL;

	if(diff == NULL || string1 == NULL || string2 == NULL)
	{
		status = FAILURE;
	}

	if((TRIUMPH & status) &&
		(diff->single_element_size != sizeof(char) ||
		string1->single_element_size != sizeof(char) ||
		string2->single_element_size != sizeof(char)))
	{
		status = FAILURE;
	}

	if((TRIUMPH & status) &&
		((diff->length > 0 && diff->data == NULL) ||
		(string1->length > 0 && string1->data == NULL) ||
		(string2->length > 0 && string2->data == NULL)))
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		status = compare_strings(
			&diff_text,
			m_text(string1),
			m_text(string2));
	}

	if((TRIUMPH & status) && diff_text == NULL)
	{
		status = FAILURE;
	}

	if((TRIUMPH & status) && diff_text != NULL)
	{
		status = m_copy_string(diff,diff_text);
	}

	if(diff_text != NULL)
	{
		free(diff_text);
		diff_text = NULL;
	}

	return(status);
}
