#include "sute.h"

Return test0036(void)
{
	INITTEST;

	bool file_exists = false;
	create(char,absolute_path);

	ASSERT(SUCCESS == replase_to_string("delete_path regular file coverage","delete_path_regular_file.txt"));
	ASSERT(SUCCESS == construct_path("delete_path_regular_file.txt",absolute_path));
	ASSERT(SUCCESS == check_file_exists(&file_exists,getcstring(absolute_path)));
	ASSERT(file_exists == true);

	ASSERT(SUCCESS == delete_path("delete_path_regular_file.txt"));

	file_exists = true;
	ASSERT(SUCCESS == check_file_exists(&file_exists,getcstring(absolute_path)));
	ASSERT(file_exists == false);

	call(del(absolute_path));

	RETURN_STATUS;
}
