#include "precizer.h"

/**
 *
 * Remove trailing slash at the end of the directory path
 *
*/
void remove_trailing_slash
(
	char* path
){
	for(size_t i = strlen(path) - 1;i > 0 && path[i] == '/';i--){
		path[i]='\0';
	}
}
