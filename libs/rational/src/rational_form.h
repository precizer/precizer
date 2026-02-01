/**
 *
 * @file
 * @brief Prototypes of functions for formatting numbers before printing
 *
 */
#include <stdint.h>
#include <stddef.h>

const char *form_real(long double);
const char *form_intmax(intmax_t);
const char *form_uintmax(uintmax_t);

#define form(x) _Generic((x), \
	long double: form_real, \
	double:      form_real, \
	float:       form_real, \
	signed char: form_intmax, \
	short:       form_intmax, \
	int:         form_intmax, \
	long:        form_intmax, \
	long long:   form_intmax, \
	char:        form_intmax, \
	unsigned char:      form_uintmax, \
	unsigned short:     form_uintmax, \
	unsigned int:       form_uintmax, \
	unsigned long:      form_uintmax, \
	unsigned long long: form_uintmax, \
	_Bool:              form_uintmax, \
	default:            form_real \
)(x)
