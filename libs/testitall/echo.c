#include "testitall.h"

/**
 * Appends formatted text to a dynamically allocated buffer.
 * If the buffer is not large enough, it is automatically resized.
 * @param buffer Pointer to the buffer.
 * @param ... The first argument is the format string (like printf), 
 *            followed by variable arguments for the format string.
 */
void echo(
	mem_char *,
	const char *,
	...
) __attribute__((format(gnu_printf, 2, 3)));
void echo(
	mem_char *buffer,
	const char *format,
	...
){
	va_list args;
	va_start(args, format);

	if (format == NULL)
	{
		return;
	}

	// Создаём новый формат, объединяя prefix и format
	size_t size = buffer->length + strlen(format) + 1; // +1 для завершающего нуля

	char *new_format = (char *)malloc(size * sizeof(char));
	if (new_format != NULL)
	{
		// Объединяем prefix и format
		if(buffer->length > 0)
		{
			strcpy(new_format, buffer->mem);
		} else if (size > 0){
			new_format[0] = '\0';
		}
		strcat(new_format, format);
	} else {
		report("Memory allocation failed, requested size: %zu bytes",size * sizeof(char));
		return;
	}

	char *str = NULL;

	// Используем vasprintf для выделения и форматирования строки
	int bytes = vasprintf(&str, new_format, args);

	free(new_format); // Освобождаем временный буфер

	if (bytes > -1) {
		// Copy str to buffer->mem
		if(SUCCESS == realloc_char(buffer,(size_t)bytes + 1))
		{
			memcpy(buffer->mem, str, buffer->length);
		}

	} else {
		report("Memory allocation failed, requested size: %zu bytes", bytes + 1);
	}

	free(str);

	va_end(args);
}
