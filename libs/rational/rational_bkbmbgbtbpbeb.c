#include "rational.h"

/**
 *
 * @brief Convert bytes
 *
 */
static inline Byte tobyte(const ui64) __attribute__((always_inline));
static inline Byte tobyte(
	const ui64 bytes
){
	/// Number of bytes in a kilobyte
	/// 1024
	const ui64 bytes_in_kilobyte = 1024ULL;

	/// Number of bytes in a megabyte
	/// 1024*1024
	const ui64 bytes_in_megabyte = bytes_in_kilobyte * 1024ULL;

	/// Number of bytes in a gigabyte
	/// 1024*1024*1024
	const ui64 bytes_in_gigabyte = bytes_in_megabyte * 1024ULL;

	/// Number of bytes in a terabyte
	/// 1024*1024*1024*1024
	const ui64 bytes_in_terabyte = bytes_in_gigabyte * 1024ULL;

	/// Number of bytes in a petabyte
	/// 1024*1024*1024*1024*1024
	const ui64 bytes_in_petabyte = bytes_in_terabyte * 1024ULL;

	/// Number of bytes exabyte
	/// 1024*1024*1024*1024*1024*1024
	const ui64 bytes_in_exabyte = bytes_in_petabyte * 1024ULL;

	/*
	 *
	 * Fill out the corresponding values of the structure
	 *
	 */

	// Initializing of the structure that will be returned
	// from the function
	Byte byte;
	memset(&byte,0,sizeof(Byte));

	byte.exabytes			= bytes/bytes_in_exabyte;

	const ui64 exabytes		= byte.exabytes*bytes_in_exabyte;
	byte.petabytes			= (bytes - exabytes)/bytes_in_petabyte;

	const ui64 petabytes	= byte.petabytes*bytes_in_petabyte;
	byte.terabytes			= (bytes - exabytes - petabytes)/bytes_in_terabyte;

	const ui64 terabytes	= byte.terabytes*bytes_in_terabyte;
	byte.gigabytes			= (bytes - exabytes - petabytes - terabytes)/bytes_in_gigabyte;

	const ui64 gigabytes	= byte.gigabytes*bytes_in_gigabyte;
	byte.megabytes			= (bytes - exabytes - petabytes - terabytes - gigabytes)/bytes_in_megabyte;

	const ui64 megabytes	= byte.megabytes*bytes_in_megabyte;
	byte.kilobytes			= (bytes - exabytes - petabytes - terabytes - gigabytes - megabytes)/bytes_in_kilobyte;

	const ui64 kilobytes	= byte.kilobytes*bytes_in_kilobyte;
	byte.bytes				= (bytes - exabytes - petabytes - terabytes - gigabytes - megabytes - kilobytes);

	return(byte);
}

/**
 *
 * @brief The function for convert bytes to a readable date.
 * The function generates a string if the structure element
 * contains data greater than zero.
 *
 */
static void catbyte(
	char * const result,
	const ui64 bytes,
	const char * const suffix
)
{
	if(bytes > 0ULL){
		// Temporary array
		char tmp[MAX_NUMBER_CHARACTERS];
		// Put a number into the temporary string array
		snprintf(tmp,sizeof(tmp),"%zu",bytes);
		// Copy the tmp line to the end of the result line
		strcat(result,tmp);
		// Add suffix
		strcat(result,suffix);
		// Add a space after the suffix
		strcat(result," ");
	}
}

/**
 * @details Convert number of bytes to human-readable string:
 * b  - Byte
 * kb - Kilobyte
 * mb - Megabyte
 * gb - Gigabyte
 * tb - Terabyte
 * pb - Petabyte
 * eb - Exabyte
 *
 */
char *bkbmbgbtbpbeb(
	const ui64 bytes
)
{
	static char result[MAX_NUMBER_CHARACTERS];

	// Zero out a static memory area with a string array
	memset(result,0,strlen(result) * sizeof(char));

	// If the number passed is 0 Bytes
	if(bytes == 0ULL){
		// Compiling a string 0b
		strcat(result,"0B");
		return(result);
	}

	Byte byte = tobyte(bytes);

	catbyte(result,byte.exabytes,"EB");
	catbyte(result,byte.petabytes,"PB");
	catbyte(result,byte.terabytes,"TB");
	catbyte(result,byte.gigabytes,"GB");
	catbyte(result,byte.megabytes,"MB");
	catbyte(result,byte.kilobytes,"kB");
	catbyte(result,byte.bytes,"B");

	// Remove space at the end of a line
	result[strlen(result) - 1ULL]='\0';

	return(result);
}
#if 0
/// Test
/// 4617322122555958282 = ((1024*1024*1024*1024*1024*1024)*4)+((1024*1024*1024*1024*1024)*5)+((1024*1024*1024*1024)*6)+((1024*1024*1024)*7)+((1024*1024)*8)+((1024)*9)+10
/// Should be 4EB 5PB 6TB 7GB 8MB 9kB 10B
int main(void){
	const ui64 bytes = 4617322122555958282ULL;
	printf("%s\n",bkbmbgbtbpbeb(bytes));
	return 0;
}
#endif
