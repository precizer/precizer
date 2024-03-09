/**
 *
 * @file
 * @brief Functions and structs to convert a number of bytes into a human-readable string
 *
 */

/// Structure to record the number of bytes.
typedef struct
{
	ui64 bytes;
	ui64 kilobytes;
	ui64 megabytes;
	ui64 gigabytes;
	ui64 terabytes;
	ui64 petabytes;
	ui64 exabytes;

} Byte;

char *bkbmbgbtbpbeb(
	const ui64
);
