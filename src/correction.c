#include "precizer.h"

/**
 *
 * This function helps calculate a relative path
 * from the data provided by the FTSENT structure
 * TODO! Explain what exactly this function does
 *
*/
size_t correction
(
	char* path
){

	size_t drift = 0;

	for(size_t i = 0; i < strlen(path);i++){
		if(path[i] == '/'){
			drift++;
		} else {
			break;
		}
	}

	return drift;
}
