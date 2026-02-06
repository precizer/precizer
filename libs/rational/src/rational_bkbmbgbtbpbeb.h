/**
 *
 * @file
 * @brief Functions and structs to convert a number of bytes into a human-readable string
 *
 */

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
typedef enum
{
	FULL_VIEW = 0,
	MAJOR_VIEW = 1
} ByteFormat;

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
