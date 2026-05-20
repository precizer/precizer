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
 * @param result_size Size of the output buffer.
 * @param used_len Current used length of the output buffer.
 * @param bytes Unit value to append when it is non-zero.
 * @param suffix Unit suffix (B, KiB, MiB, GiB, TiB, PiB, EiB).
 */
static void catbyte_r(
	char *const       result,
	const size_t      result_size,
	size_t *const     used_len,
	const size_t      bytes,
	const char *const suffix)
{
	if(bytes == 0ULL || *used_len >= result_size)
	{
		return;
	}

	const int written = snprintf(result + *used_len,result_size - *used_len,"%zu%s ",bytes,suffix);

	if(written < 0)
	{
		return;
	}

	const size_t write_size = (size_t)written;

	if(write_size >= result_size - *used_len)
	{
		/*
		 * snprintf() has already written the largest prefix that fits.
		 * Mark the buffer as full so later units do not try to append text
		 */
		*used_len = result_size;
		result[result_size - 1ULL] = '\0';

		/*
		 * If the visible prefix ends right after a unit separator, hide that
		 * separator so callers get a clean partial value
		 */
		if(result_size > 1ULL && result[result_size - 2ULL] == ' ')
		{
			result[result_size - 2ULL] = '\0';
		}
	} else {
		*used_len += write_size;
	}
}

/**
 * @brief Convert bytes to a human-readable size string in caller-provided buffer.
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
 * @param buffer Destination buffer.
 * @param buffer_size Destination buffer size in bytes.
 * @return @p buffer on success, NULL when @p buffer is NULL or @p buffer_size is zero.
 */
char *bkbmbgbtbpbeb_r(
	const size_t     bytes,
	const ByteFormat format,
	char             *buffer,
	const size_t     buffer_size)
{
	if(buffer == NULL || buffer_size == 0ULL)
	{
		return(NULL);
	}

	buffer[0] = '\0';
	size_t used_len = 0ULL;

	// If the number passed is 0 Bytes
	if(bytes == 0ULL)
	{
		(void)snprintf(buffer,buffer_size,"0B");
		return(buffer);
	}

	Byte byte = tobyte(bytes);

	if(format == MAJOR_VIEW)
	{
		if(byte.exbibytes > 0ULL)
		{
			catbyte_r(buffer,buffer_size,&used_len,byte.exbibytes,"EiB");
		} else if(byte.pebibytes > 0ULL){
			catbyte_r(buffer,buffer_size,&used_len,byte.pebibytes,"PiB");
		} else if(byte.tebibytes > 0ULL){
			catbyte_r(buffer,buffer_size,&used_len,byte.tebibytes,"TiB");
		} else if(byte.gibibytes > 0ULL){
			catbyte_r(buffer,buffer_size,&used_len,byte.gibibytes,"GiB");
		} else if(byte.mebibytes > 0ULL){
			catbyte_r(buffer,buffer_size,&used_len,byte.mebibytes,"MiB");
		} else if(byte.kibibytes > 0ULL){
			catbyte_r(buffer,buffer_size,&used_len,byte.kibibytes,"KiB");
		} else {
			catbyte_r(buffer,buffer_size,&used_len,byte.bytes,"B");
		}

	} else {
		catbyte_r(buffer,buffer_size,&used_len,byte.exbibytes,"EiB");
		catbyte_r(buffer,buffer_size,&used_len,byte.pebibytes,"PiB");
		catbyte_r(buffer,buffer_size,&used_len,byte.tebibytes,"TiB");
		catbyte_r(buffer,buffer_size,&used_len,byte.gibibytes,"GiB");
		catbyte_r(buffer,buffer_size,&used_len,byte.mebibytes,"MiB");
		catbyte_r(buffer,buffer_size,&used_len,byte.kibibytes,"KiB");
		catbyte_r(buffer,buffer_size,&used_len,byte.bytes,"B");
	}

	if(used_len > 0ULL && buffer[used_len - 1ULL] == ' ')
	{
		buffer[used_len - 1ULL] = '\0';
	}

	return(buffer);
}

/**
 * @brief Convert bytes to a human-readable size string.
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

	(void)bkbmbgbtbpbeb_r(bytes,format,result,sizeof(result));

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
