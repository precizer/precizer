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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// Request POSIX.1-2008 interfaces (pathconf, clock_gettime, snprintf, etc.)
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

// Enable full libc surface on macOS even when POSIX macros are set
#ifdef EVIL_EMPIRE_OS
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif
#define st_atim st_atimespec
#define st_mtim st_mtimespec
#define st_ctim st_ctimespec
#endif

// 64bit File Systems
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64 1
#endif

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

/**
 *
 * Prototypes of functions for report an error without relying on dynamic memory
 *
 */
#include "rational_report.h"

/**
 *
 * Common usage structures and enumerations
 *
 */
#include "rational_strenum.h"

/**
 *
 * Prototypes of functions and macros for logging
 *
 */
#include "rational_logger.h"

/**
 *
 * Prototypes of the function to convert an integer
 * to a string representation
 *
 */
#include "rational_itoa.h"

/**
 *
 * Terminal decoration
 *
 *
 */
#include "rational_decoration.h"

/**
 *
 *
 *
 *
 */
#include "rational_provide.h"
