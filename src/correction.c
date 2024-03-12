#include "precizer.h"

/**
 *
 * @brief This function helps calculate a relative path
 * from the data provided by the FTSENT structure
 * @details This function searches from left to right for the first
 * occurrence of the directory character '/' in the string passed
 * as an argument (that is the path) and returns the number of bytes
 * from the beginning of the string to that character.
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
