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

	/// Extra output
	bool verbose;

	/// Force update of the database with new, changed or deleted files
	bool update;

	/// Parameter to compare database
	bool compare;

	/// An array of paths to scan
	char **paths;

	/// Name of DB file
	char *db_file_name;

	/// The flag meens that the DB already exists
	bool db_already_exists;

	/// Flag that reflects the presence of any changes
	/// since the last research
	bool something_has_been_changed;

	/// Pointers to the array with database paths
	char **databases_to_compare;

	/// Pointers to the array with database file names
	char **filenames;

	/// Don't produce any output
	bool silent;

} Config;

/*
 *
 * Prototypes of internal functions
 *
 */

Return file_list(
	Config*,
	bool
);

Return sha512sum(
	const Config*,
	const char*,
	const short unsigned int*,
	unsigned char*,
	sqlite3_int64*,
	SHA512_Context*
);

void remove_trailing_slash(char*);

size_t correction(char*) __attribute__ ((pure));

void notify_quit_handler(int);

Return determine_running_dir(Config*);

Return init_config(Config**);

Return init_signals(const Config*);

void free_config(Config*);

Return db_delete_missing_files_from(
	Config*
);

Return db_delete_the_file_by_id
(
	Config*,
	sqlite_int64*,
	bool*,
	const char*
);

Return db_init(Config*);

Return db_vacuum(const Config*);

Return db_read_file_data_from(
	const Config*,
	DBrow*,
	const char*
);

Return db_update_the_record(
	const Config*,
	const sqlite3_int64*,
	const sqlite3_int64*,
	const unsigned char*,
	const struct stat*,
	const SHA512_Context*
);

Return db_insert_the_record(
	const Config*,
	const char*,
	const sqlite3_int64*,
	const unsigned char*,
	const struct stat*,
	const SHA512_Context*
);

Return db_file_name(
	Config*
);

Return db_save_paths_into(
	const Config*
);

Return db_compare(
	const Config*
);

Return db_check_up_paths(
	const Config*
);

Return db_already_exists(
	Config*
);

int compare_file_metadata_equivalence(
	const struct stat*,
	const struct stat*
) __attribute__ ((pure));

Return parse_arguments(
	Config*,
	const int,
	char **
);

void show_relative_path
(
	const Config*,
	const char*,
	const int*,
	const DBrow*,
	bool*,
	bool*,
	bool*
);

void changes_status(const Config*);

int exit_status(
	const Config*,
	Return,
	char**
);

void logger
(
	const Config*,
	const bool,
	const char*,
	size_t,
	const char*,
	const char*,
	...
);

#define slog(x,y,...) logger(x,y,__FILE__, __LINE__, __func__, __VA_ARGS__ )

// Declaration of time function
char *logger_show_time(void);

extern _Atomic bool global_interrupt_flag;

#endif /* _PRECIZER_H */
