/**
 *
 * @file
 * @brief Functions and structs to convert a number of bytes into a human-readable string
 *
 */

/// Structure to record the number of bytes.
typedef struct
{
	size_t bytes;
	size_t kilobytes;
	size_t megabytes;
	size_t gigabytes;
	size_t terabytes;
	size_t petabytes;
	size_t exabytes;

} Byte;

char *bkbmbgbtbpbeb(
	const size_t
);
