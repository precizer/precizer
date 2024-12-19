#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "xdiff.h"
#include "sute.h"

static size_t read_file_to_array(
	const char *filename,
	char       **array
){
	FILE *file = fopen(filename,"rb");

	if(!file)
	{
		return 0;
	}

	// Получаем размер файла
	struct stat st;

	if(stat(filename,&st) != 0)
	{
		fclose(file);
		return 0;
	}

	// Выделяем память с учетом нуль-терминатора
	*array = (char *)malloc(st.st_size + 1);

	if(!*array)
	{
		fclose(file);
		return 0;
	}

	// Читаем файл
	size_t bytes_read = fread(*array,1,st.st_size,file);
	fclose(file);

	if(bytes_read != st.st_size)
	{
		free(*array);
		*array = NULL;
		return 0;
	}

	// Добавляем нуль-терминатор
	(*array)[bytes_read] = '\0';

	return bytes_read;
}

// Main test runner
Return test0015(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *text1 = NULL;
	size_t bytes_read = read_file_to_array("templates/0002.txt",&text1);

	char *text2 = NULL;
	bytes_read = read_file_to_array("templates/0003.txt",&text2);

	ASSERT(SUCCESS != compare_texts(text1,text2));

	free(text1);
	free(text2);

	RETURN_STATUS;
}
