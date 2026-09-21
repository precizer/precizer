/**
 * @file
 * @brief Terminal color output policy
 */

#ifndef RATIONAL_COLOR_H
#define RATIONAL_COLOR_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum RATIONAL_COLOR_MODE : unsigned char
{
	COLOR_MODE_AUTO = 0,
	COLOR_MODE_ALWAYS,
	COLOR_MODE_NEVER
} RATIONAL_COLOR_MODE;

/**
 * @brief Process-wide mode for terminal colors and text styling
 *
 * Defaults to COLOR_MODE_AUTO. COLOR_MODE_ALWAYS forces styling, while
 * COLOR_MODE_NEVER disables it. In automatic mode, rational_color_is_enabled()
 * checks NO_COLOR, TERM, and terminal status separately for each output stream.
 * Atomic storage allows concurrent reads and updates of this setting
 */
extern _Atomic RATIONAL_COLOR_MODE rational_color_mode;

bool rational_color_is_enabled(FILE *);
#endif // RATIONAL_COLOR_H
