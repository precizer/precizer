/**
 *
 * @file
 * @brief Main header file of the project
 *
 */

#pragma once

/*
 *
 *
 * Defining control macros for system libraries
 *
 *
 */

// Need for strdup(), clock_gettime()
// Have to be at the beginning of the file
#define _GNU_SOURCE

// Need for pathconf()
#define _POSIX_SOURCE

// 64bit File Systems
#define __USE_FILE_OFFSET64 1

/*
 *
 *
 * System libraries
 *
 *
 */
#include <stdio.h>

// String library
#include <string.h>

// Extended types
#include <inttypes.h>
#include <stdbool.h>

// Mathematical formulas
#include <math.h>

// Standard functions
#include <stdlib.h>

// String functions
#include <string.h>

/**
 *
 * Macros and preprocessor constants
 *
 */
#include "rational_constants.h"

/**
 *
 * Time Logging structures and functions prototypes
 *
 */
#include "rational_time.h"

/**
 *
 * Functions and structs to convert a number of bytes into a human-readable string
 *
 */
#include "rational_bkbmbgbtbpbeb.h"

/**
 *
 * Prototypes of functions to format numbers before printing
 *
 */
#include "rational_form.h"
