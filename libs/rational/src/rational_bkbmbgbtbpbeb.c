#include "rational.h"

/**
 * @brief Convert bytes
 *
 */
static inline Byte tobyte(const size_t) __attribute__((always_inline));
static inline Byte tobyte(const size_t bytes)
{
	/// Number of bytes in a kibibyte
	/// 1024
	const size_t bytes_in_kibibyte = 1024ULL;

	/// Number of bytes in a mebibyte
	/// 1024*1024
	const size_t bytes_in_mebibyte = bytes_in_kibibyte * 1024ULL;

	/// Number of bytes in a gibibyte
	/// 1024*1024*1024
	const size_t bytes_in_gibibyte = bytes_in_mebibyte * 1024ULL;

	/// Number of bytes in a tebibyte
	/// 1024*1024*1024*1024
	const size_t bytes_in_tebibyte = bytes_in_gibibyte * 1024ULL;

	/// Number of bytes in a pebibyte
	/// 1024*1024*1024*1024*1024
	const size_t bytes_in_pebibyte = bytes_in_tebibyte * 1024ULL;

	/// Number of bytes in an exbibyte
	/// 1024*1024*1024*1024*1024*1024
	const size_t bytes_in_exbibyte = bytes_in_pebibyte * 1024ULL;

	/*
	 *
	 * Fill out the corresponding values of the structure
	 *
	 */

	// Initializing of the structure that will be returned
	// from the function
	Byte byte = {0};

	byte.exbibytes = bytes/bytes_in_exbibyte;

	const size_t exbibytes = byte.exbibytes*bytes_in_exbibyte;
	byte.pebibytes = (bytes - exbibytes)/bytes_in_pebibyte;

	const size_t pebibytes = byte.pebibytes*bytes_in_pebibyte;
	byte.tebibytes = (bytes - exbibytes - pebibytes)/bytes_in_tebibyte;

	const size_t tebibytes = byte.tebibytes*bytes_in_tebibyte;
	byte.gibibytes = (bytes - exbibytes - pebibytes - tebibytes)/bytes_in_gibibyte;

	const size_t gibibytes = byte.gibibytes*bytes_in_gibibyte;
	byte.mebibytes = (bytes - exbibytes - pebibytes - tebibytes - gibibytes)/bytes_in_mebibyte;

	const size_t mebibytes = byte.mebibytes*bytes_in_mebibyte;
	byte.kibibytes = (bytes - exbibytes - pebibytes - tebibytes - gibibytes - mebibytes)/bytes_in_kibibyte;

	const size_t kibibytes = byte.kibibytes*bytes_in_kibibyte;
	byte.bytes = (bytes - exbibytes - pebibytes - tebibytes - gibibytes - mebibytes - kibibytes);

	return(byte);
}

/**
 * @brief Append one non-zero unit to the resulting size string.
 *
 * @param result Output string being built.
 * @param bytes Unit value to append when it is non-zero.
 * @param suffix Unit suffix (B, KiB, MiB, GiB, TiB, PiB, EiB).
 */
static void catbyte(
	char *const       result,
	const size_t      bytes,
	const char *const suffix)
{
	if(bytes > 0ULL)
	{
		// Temporary array
		char tmp[MAX_CHARACTERS];
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
 * @brief Convert bytes to a human-readable size string.
 *
 * @details Convert number of bytes to human-readable string:
 * B   - Byte
 * KiB - Kibibyte
 * MiB - Mebibyte
 * GiB - Gibibyte
 * TiB - Tebibyte
 * PiB - Pebibyte
 * EiB - Exbibyte
 *
 * @param bytes Number of bytes to format.
 * @param format Output style:
 *               - FULL_VIEW: show all non-zero units.
 *               - MAJOR_VIEW: show only the highest non-zero unit.
 * @return Pointer to a static buffer with formatted text.
 */
char *bkbmbgbtbpbeb(
	const size_t     bytes,
	const ByteFormat format)
{
	// Zero out a static memory area with a string array
	static char result[MAX_CHARACTERS] = {0};
	result[0] = '\0';  /* Initialize buffer as empty string */

	// If the number passed is 0 Bytes
	if(bytes == 0ULL)
	{
		// Compiling a string 0b
		strcat(result,"0B");
		return(result);
	}

	Byte byte = tobyte(bytes);

	if(format == MAJOR_VIEW)
	{
		if(byte.exbibytes > 0ULL)
		{
			catbyte(result,byte.exbibytes,"EiB");
		} else if(byte.pebibytes > 0ULL){
			catbyte(result,byte.pebibytes,"PiB");
		} else if(byte.tebibytes > 0ULL){
			catbyte(result,byte.tebibytes,"TiB");
		} else if(byte.gibibytes > 0ULL){
			catbyte(result,byte.gibibytes,"GiB");
		} else if(byte.mebibytes > 0ULL){
			catbyte(result,byte.mebibytes,"MiB");
		} else if(byte.kibibytes > 0ULL){
			catbyte(result,byte.kibibytes,"KiB");
		} else {
			catbyte(result,byte.bytes,"B");
		}

	} else {
		catbyte(result,byte.exbibytes,"EiB");
		catbyte(result,byte.pebibytes,"PiB");
		catbyte(result,byte.tebibytes,"TiB");
		catbyte(result,byte.gibibytes,"GiB");
		catbyte(result,byte.mebibytes,"MiB");
		catbyte(result,byte.kibibytes,"KiB");
		catbyte(result,byte.bytes,"B");
	}

	// Remove space at the end of a line
	result[strlen(result) - 1ULL] = '\0';

	return(result);
}
#if 0
/// Test
/// 4617322122555958282 = ((1024*1024*1024*1024*1024*1024)*4)+((1024*1024*1024*1024*1024)*5)+((1024*1024*1024*1024)*6)+((1024*1024*1024)*7)+((1024*1024)*8)+((1024)*9)+10
/// Should be 4EiB 5PiB 6TiB 7GiB 8MiB 9KiB 10B
int main(void)
{
	const size_t bytes = 4617322122555958282ULL;
	printf("%s\n",bkbmbgbtbpbeb(bytes,FULL_VIEW));
	return 0;
}
#endif
