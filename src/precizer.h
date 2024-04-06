/**
 *
 * @file precizer.h
 * @brief Main header file of the project
 *
 */

#ifndef _PRECIZER_H
#define _PRECIZER_H

/// Included libraries from "libs" subdir
#include "rational.h"
#include "sha512.h"
#include "sqlite3.h"


/*
 *
 * A common set of shared libraties for all components
 * of the project
 *
 */

#include <sys/stat.h>

#define SHA512_DIGEST_LENGTH 64

/*
 *
 * Initialization of enumerations
 *
 */

/// Function exit status
/// Formatted as an enumeration
typedef enum
{
	/// Successfully
	SUCCESS, // The actual value is 0

	/// Unsuccessful
	FAILURE, // The actual value is 1

	// Warning
	WARNING, // The actual value is 2

	// Do nothing
	DONOTHING // The actual value is 3

} Return;

// PCRE2 return codes
typedef enum
{
	NOT_MATCH,   // The actual value is 0
	MATCH,       // The actual value is 1
	REGEXP_ERROR // The actual value is 2

} REGEXP;

// Return codes for Ignore function
typedef enum
{
	DO_NOT_IGNORE,     // The actual value is 0
	IGNORE,            // The actual value is 1
	FAIL_REGEXP_IGNORE // The actual value is 2

} Ignore;

// Return codes for Include function
typedef enum
{
	DO_NOT_INCLUDE,     // The actual value is 0
	INCLUDE,            // The actual value is 1
	FAIL_REGEXP_INCLUDE // The actual value is 2

} Include;


/*
 * Modification bits
 *
 */
typedef enum
{
    NOT_EQUAL                 = -1,
    IDENTICAL                 = 0,
    SIZE_CHANGED              = 1,
    CREATION_TIME_CHANGED     = 2,
    MODIFICATION_TIME_CHANGED = 4,

} Changed;

/*
 *
 * Declaration of structures
 *
 */

/* DB row content */
typedef struct {

	/* True if the relative path saved against DB */
	bool relative_path_already_in_db;

	/* Offset of a file (man 3 fopen) */
	sqlite3_int64 saved_offset;

	/* DB row ID */
	sqlite3_int64 ID;

	/* Metadata of a file (man 2 stat) */
	struct stat saved_stat;

	/* SHA512 metadata */
	SHA512_Context saved_mdContext;

} DBrow;

// The main Configuration
typedef struct {

	/// Max available size of a path
	long int running_dir_size;

	/// The pointer to the main database
	sqlite3 *db;

	/// Total size of all scanned files
	size_t total_size_in_bytes;

	/// Absolute path name of the working directory
	/// A directory where the program was executed
	char *running_dir;

	/// Show progress bar
	bool progress;

	/// Forse update of the database
	bool force;

	/// Additional output for debugging
	bool verbose;

	/// Force update of the database with new,
	/// changed or deleted files. This is special
	/// protection against accidental deletion of
	/// information from the database.
	bool update;

	/// Parameter to compare database
	bool compare;

	/// An array of paths to traverse
	char **paths;

	/// The path of DB file
	char *db_file_path;

	/// The name of DB file
	char *db_file_name;

	/// The flag meens that the DB already exists
	bool db_already_exists;

	/// Flag that reflects the presence of any changes
	/// since the last research
	bool something_has_been_changed;

	/// Pointers to the array with database paths
	char **db_file_paths;

	/// Pointers to the array with database file names
	char **db_file_names;

	/// Don't produce any output
	bool silent;

	/// Recursion depth limit. The depth of the traversal,
	/// numbered from 0 to N, where a file could be found.
	/// Representing the maximum of the starting
	/// point (from root) of the traversal.
	/// The root itself is numbered 0
	short maxdepth;

	/// Ignore those relative paths
	/// The string array of PCRE2 regular expressions
	char **ignore;

	/// Include those relative paths even if
	/// they were excluded via the --ignore option
	/// The string array of PCRE2 regular expressions
	char **include;

	/// Must be specifyed additionally in order
	/// to remove from the database mention of
	/// files that matches the regular expression
	/// passed through the ignore option(s)
	/// This is special protection against accidental
	/// deletion of information from the database.
	bool db_clean_ignored;

	/// Perform a trial run with no changes made
	bool dry_run;

} Config;

/*
 *
 * Prototypes of internal functions
 *
 */

Return file_list(
	bool
);

Return sha512sum(
	const char*,
	const short unsigned int*,
	unsigned char*,
	sqlite3_int64*,
	SHA512_Context*
);

void add_string_to_array(
	char ***,
	char *
);

void remove_trailing_slash(char*);

size_t correction(char*) __attribute__ ((pure));

void notify_quit_handler(int);

Return determine_running_dir(void);

void init_config(void);

Return init_signals(void);

void free_config(void);

Return db_delete_missing_files_from(void);

Return db_delete_the_file_by_id
(
	sqlite_int64*,
	bool*,
	const bool*,
	const char*
);

Return db_init(void);

Return db_vacuum(void);

Return db_read_file_data_from(
	DBrow*,
	const char*
);

Return db_update_the_record(
	const sqlite3_int64*,
	const sqlite3_int64*,
	const unsigned char*,
	const struct stat*,
	const SHA512_Context*
);

Return db_insert_the_record(
	const char*,
	const sqlite3_int64*,
	const unsigned char*,
	const struct stat*,
	const SHA512_Context*
);

Return db_file_name(void);

Return db_save_prefixes_into(void);

Return db_compare(void);

Return db_check_up_paths(void);

Return db_already_exists(void);

Return db_test(
	const char*
);

#if 0 // Old multiPATH solution
Return db_get_path_prefix_index
(
	const char*,
	sqlite3_int64*
);
#endif

int compare_file_metadata_equivalence(
	const struct stat*,
	const struct stat*
) __attribute__ ((pure));

Return parse_arguments(
	const int,
	char **
);

void show_relative_path
(
	const char*,
	const int*,
	const DBrow*,
	bool*,
	bool*,
	bool*,
	const bool*,
	bool*
);

void status_of_changes(void);

Return detect_a_path(const char*);

Return detect_paths(void);

Ignore ignore(
	const char*,
	bool*
);

Include include(
	const char*,
	bool*
);

REGEXP regexp_match
(
	const char*,
	const char*,
	bool*
);

int exit_status(
	Return,
	char**
);

void logger
(
	const bool,
	const char*,
	size_t,
	const char*,
	const char*,
	...
);

#define slog(x,...) logger(x,__FILE__, __LINE__, __func__, __VA_ARGS__ )

// Declaration of time function
char *logger_show_time(void);

extern _Atomic bool global_interrupt_flag;

extern Config _config;
extern Config *config;

// Application name and current code version
#include "version.h"

#endif /* _PRECIZER_H */
