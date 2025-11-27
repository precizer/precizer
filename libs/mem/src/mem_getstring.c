#include "mem.h"

/**
 * @brief Shared zero-terminated buffer used when descriptors are unavailable.
 */
static char empty_string[] = "";

const char *memory_getcstring(const memory *memory_object)
{
	if(memory_object == NULL)
	{
		return empty_string;
	}

	const char *text = (const char *)memory_const_data_checked(memory_object,sizeof(char));

	if(text == NULL)
	{
		return empty_string;
	}

	if(memory_object->length == 0)
	{
		return empty_string;
	}

	size_t visible_length = 0;

	if(memory_string_length(memory_object,&visible_length) != SUCCESS)
	{
		return empty_string;
	}

	if(visible_length >= memory_object->length)
	{
		return empty_string;
	}

	return text;
}

char *memory_getstring(memory *memory_object)
{
	if(memory_object == NULL)
	{
		return empty_string;
	}

	char *text = (char *)memory_data_checked(memory_object,sizeof(char));

	if(text == NULL)
	{
		return empty_string;
	}

	if(memory_object->length == 0)
	{
		if(memory_resize(memory_object,1,UCHAR_MAX) != SUCCESS)
		{
			return empty_string;
		}

		text = (char *)memory_data_checked(memory_object,sizeof(char));

		if(text == NULL)
		{
			return empty_string;
		}

		text[0] = '\0';
		return text;
	}

	size_t visible_length = 0;

	if(memory_string_length(memory_object,&visible_length) != SUCCESS)
	{
		return empty_string;
	}

	if(visible_length >= memory_object->length)
	{
		text[0] = '\0';
		return text;
	}

	return text;
}
