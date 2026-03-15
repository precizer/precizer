/**
 *
 * @file
 * @brief Prototypes of functions for formatting numbers before printing
 *
 */
#include <stdint.h>
#include <stddef.h>

/*
 * Reentrant formatter buffer size used by both real and integer formatters.
 *
 * Contract:
 * - `form_real_r()` formats with up to 9 fractional digits, but will silently reduce
 *   fractional precision (down to 0 digits) when the formatted value does not fit
 *   into the provided buffer. This happens most often on large magnitudes because
 *   the integer part and thousands separators consume most of the buffer.
 */
#define FORM_OUTPUT_BUFFER_SIZE 32U

/**
 * @brief Format a real number using ',' as thousands separator.
 *
 * @details
 * - Rounds to at most 9 fractional digits and strips trailing fractional zeros.
 * - Uses '.' as decimal separator.
 * - Treats very small magnitudes as zero.
 * - If the formatted output does not fit into `result_size`, fractional precision
 *   is reduced until it fits (silently). If it still does not fit, writes an empty
 *   string.
 *
 * @return `result` (or "" if `result` is NULL or `result_size` is 0).
 */
const char *form_real_r(
	long double,
	char *,
	size_t);

const char *form_intmax_r(
	intmax_t,
	char *,
	size_t);

const char *form_uintmax_r(
	uintmax_t,
	char *,
	size_t);

#define form(x,result,result_size) _Generic((x), \
	long double: form_real_r, \
	double:      form_real_r, \
	float:       form_real_r, \
	signed char: form_intmax_r, \
	short:       form_intmax_r, \
	int:         form_intmax_r, \
	long:        form_intmax_r, \
	long long:   form_intmax_r, \
	char:        form_intmax_r, \
	unsigned char:      form_uintmax_r, \
	unsigned short:     form_uintmax_r, \
	unsigned int:       form_uintmax_r, \
	unsigned long:      form_uintmax_r, \
	unsigned long long: form_uintmax_r, \
	_Bool:              form_uintmax_r, \
	default:            form_real_r \
)(x,result,result_size)
