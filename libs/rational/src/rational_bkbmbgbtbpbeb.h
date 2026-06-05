/**
 *
 * @file
 * @brief Functions and structs to convert a number of bytes into a human-readable string
 *
 */
#ifndef RATIONAL_BKBMBGBTBPBEB_H
#define RATIONAL_BKBMBGBTBPBEB_H

#include <stddef.h>

/// Structure to record the number of bytes.
typedef struct {
	size_t bytes;
	size_t kibibytes;
	size_t mebibytes;
	size_t gibibytes;
	size_t tebibytes;
	size_t pebibytes;
	size_t exbibytes;
} Byte;

/**
 * @brief Output style for byte formatting.
 */
typedef enum ByteFormat : unsigned int
{
	FULL_VIEW = 0u,
	MAJOR_VIEW = 1u
} ByteFormat;

/**
 * @brief Convert bytes to a human-readable string in caller-provided buffer.
 *
 * @param bytes Number of bytes to format.
 * @param format FULL_VIEW for full decomposition, MAJOR_VIEW for the largest unit only.
 * @param buffer Destination buffer.
 * @param buffer_size Destination buffer size in bytes.
 * @return @p buffer on success, NULL when @p buffer is NULL or @p buffer_size is zero.
 */
char *bkbmbgbtbpbeb_r(
	const size_t,
	const ByteFormat,
	char *,
	size_t);

/**
 * @brief Convert bytes to a human-readable string.
 *
 * @param bytes Number of bytes to format.
 * @param format FULL_VIEW for full decomposition, MAJOR_VIEW for the largest unit only.
 * @return Pointer to a static buffer with formatted text.
 */
char *bkbmbgbtbpbeb(
	const size_t,
	const ByteFormat);

#endif // RATIONAL_BKBMBGBTBPBEB_H
