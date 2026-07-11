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
#include "mem.h"
#include "sqlite3.h"
// Application name and current code version
#include "version.h"

/*
 *
 * A common set of shared libraries for all components
 * of the project
 *
 */
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>

/* Determine hostname */
#include <sys/utsname.h>

/* String operations */
#include <string.h>

/* Terminal window width and
   other terminal operations */
#include <sys/ioctl.h>
#include <termios.h>

/* System Signals */
#include <sys/types.h>
#include <signal.h>

/* Atomic operations */
#include <stdatomic.h>

/* PCRE2 Library */
#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

/* File traversal library */
#include <fts.h>

/* Parse arguments with argp library */
#include <argp.h>

#define SQL_DRY_RUN_MODE ((int)-1)

#define POSIX_STAT_BLOCK_BYTES 512ULL
/*
 * st_blocks value used in migrated v4 records when legacy DB versions
 * (v1..v3) did not store allocated block count.
 *
 * This legacy can be removed in 2036 (10-year Long-Term Support)
 */
#define BLKCNT_UNKNOWN ((blkcnt_t)-1)

/*
 * Evil Empire OS uses `st_*timespec` fields instead of `st_*tim`.
 * Map the member names so we can keep the Linux-oriented copy code.
 */
#ifdef EVIL_EMPIRE_OS
#define st_mtim st_mtimespec
#define st_ctim st_ctimespec
#endif

/*
 * All the macros are here
 */
#define slog_show(level,respect_quiet,first_iteration,summary,...) \
	slog_show_impl(__FILE__,__func__,__LINE__,(level),(respect_quiet),(first_iteration),(summary),__VA_ARGS__)

/**
 * @brief Get a pointer to a field in the global @ref Config instance.
 *
 * Convenience wrapper for `&(config->field)`.
 * Primarily used with libmem helpers that accept `memory *`.
 *
 * @param field Name of a @ref Config field without the `config->` prefix.
 * @return Pointer to the selected field.
 */
#define conf(field) (&(config->field))

/**
 * @brief Get a C-string view of a string field in the global @ref Config instance.
 *
 * Convenience wrapper for `m_text(&(config->field))`.
 * The target field is expected to be of type @ref memory and to store string data.
 *
 * @param field Name of a @ref Config field without the `config->` prefix.
 * @return `const char *` returned by @ref m_text.
 */
#define confstr(field) m_text(&(config->field))

// PCRE2 return codes
typedef enum REGEXP : unsigned int
{
	NOT_MATCH = 0u,
	MATCH = 1u,
	REGEXP_ERROR = 2u

} REGEXP;

// Return codes for Ignore function
typedef enum Ignore : unsigned int
{
	DO_NOT_IGNORE = 0u,
	IGNORE = 1u,
	FAIL_REGEXP_IGNORE = 2u

} Ignore;

// Return codes for Include function
typedef enum Include : unsigned int
{
	DO_NOT_INCLUDE = 0u,     // The actual value is 0
	INCLUDE = 1u,            // The actual value is 1
	FAIL_REGEXP_INCLUDE = 2u // The actual value is 2

} Include;

// Return codes for Lock Checksum function
typedef enum LockChecksum : unsigned int
{
	DO_NOT_LOCK_CHECKSUM = 0u,     // The actual value is 0
	LOCK_CHECKSUM = 1u,            // The actual value is 1
	FAIL_REGEXP_LOCK_CHECKSUM = 2u // The actual value is 2

} LockChecksum;

/*
 * A file or a directory
 *
 */
typedef enum FILEDIR : unsigned int
{
	SHOULD_BE_A_FILE = 1u,
	SHOULD_BE_A_DIRECTORY = 2u

} FILEDIR;

/**
 * Database validation level
 *
 */
typedef enum DB_CHECK_LEVEL : unsigned int
{
	QUICK = 1u,
	FULL = 2u

} DB_CHECK_LEVEL;

/**
 * @brief Bitmask that controls which result categories are shown by `--compare`
 *
 * Stores the output categories selected through `--compare-filter`
 * Each non-zero bit enables one report category in `db_compare()`
 * A zero value means that no filter was specified explicitly, so compare mode
 * uses its default behavior and reports all categories
 */
typedef enum CompareFilter : unsigned int
{
	CF_NONE_SPECIFIED = 0x00u, /**< No explicit `--compare-filter` options were provided */
	CF_CHECKSUM_MISMATCH = 0x01u, /**< Report paths present in both databases whose checksums differ */
	CF_FIRST_SOURCE = 0x02u, /**< Report paths that exist in the first compared database but not in the second */
	CF_SECOND_SOURCE = 0x04u /**< Report paths that exist in the second compared database but not in the first */

} CompareFilter;

/**
 * Whether the file exists or not
 *
 */
typedef enum FileAvailability : unsigned int
{
	NOT_FOUND = 0u,
	EXISTS = 1u

} FileAvailability;

/*
 *
 * Declaration of structures
 *
 */

/**
 * @brief Named change-flag descriptor for human-readable output.
 *
 * Maps a change bitmask value from @ref Changed to its printable name.
 */
typedef struct Flags {
	Changed flag_value;
	const char *flag_name;
} Flags;

/**
 * @brief Compact Stat file metadata structure
 *
 * Contains essential file metadata including logical size, allocated blocks,
 * and timestamps.
 * Provides high precision timing using separate second and nanosecond fields.
 *
 * @warning This struct is persisted to SQLite as a raw binary blob via
 * `sqlite3_bind_blob(..., sizeof(CmpctStat), ...)`. The on-disk layout is
 * therefore ABI-specific: field sizes (`off_t`, `blkcnt_t`, `dev_t`, `ino_t`,
 * `time_t`) and compiler-inserted padding vary between platforms and ABIs.
 * A database file created on a 64-bit x86 Linux system is not readable on
 * a 32-bit or big-endian platform. No cross-ABI migration path exists;
 * existing migrations only handle schema changes within the same ABI
 */
typedef struct {

	/** File size in bytes */
	off_t st_size;

	/** Number of 512-byte blocks allocated to the file */
	blkcnt_t st_blocks;

	/** Filesystem device number */
	dev_t st_dev;

	/** Inode number on the filesystem */
	ino_t st_ino;

	/**
	 * Last file content modification time - seconds portion
	 * Updated when file contents are modified (write, truncate, etc.)
	 */
	time_t mtim_tv_sec;

	/**
	 * Last file content modification time - nanoseconds portion
	 * Provides nanosecond precision for modification timestamp
	 * Valid range: 0 to 999999999
	 */
	long mtim_tv_nsec;

	/**
	 * Last status change time - seconds portion
	 * Updated when file metadata changes (permissions, ownership)
	 * or when contents are modified
	 */
	time_t ctim_tv_sec;

	/**
	 * Last status change time - nanoseconds portion
	 * Provides nanosecond precision for status change timestamp
	 * Valid range: 0 to 999999999
	 */
	long ctim_tv_nsec;

} CmpctStat; // Compact 'stat' structure

/* DB row content */
typedef struct {

	/* True if the relative path already existed in DB before current file processing */
	bool relative_path_was_in_db_before_processing;

	/* Offset of a file (man 3 fopen) */
	sqlite3_int64 saved_offset;

	/* DB row ID */
	sqlite3_int64 ID;

	/* Compact Stat
	   Metadata of a file (man 2 stat) */
	CmpctStat saved_stat;

	/* SHA512 metadata */
	SHA512_Context saved_mdContext;

	/* SHA512 summ */
	unsigned char sha512[SHA512_DIGEST_LENGTH];

} DBrow;

// The main Configuration
typedef struct {

	/// Application start timestamp in monotonic nanoseconds.
	/// Used for reporting total process runtime.
	long long int app_start_time_ns;

	/// Show progress bar
	bool progress;

	/// When set to true, all messages logged with the REMEMBER flag
	/// are reprinted as a summary block before the program exits.
	/// This ensures critical warnings (e.g. checksum-locked violations)
	/// remain visible even after lengthy output.
	bool show_remembered_messages_at_exit;

	/// Force update of the database
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

	/// Bitmask of enabled --compare output filters
	unsigned int compare_filter;

	/// Managed array of root paths to traverse
	memory roots;

	/// The pointer to the primary database
	sqlite3 *db;

	/// The path of DB file stored as a managed byte string.
	memory db_primary_file_path;

	/// True when primary DB path is the in-memory SQLite marker (":memory:").
	bool db_primary_path_is_memory;

	/// The name of DB file stored as a managed byte string.
	memory db_file_name;

	/// Managed array of database file paths
	memory db_file_paths;

	/// Pointers to the array with database file names
	char **db_file_names;

	/// Allow or disallow database table
	/// initialization
	bool db_initialize_tables;

	/// Flag indicating whether the primary database file exists
	/// This flag is set to true if the primary database FILE exists and
	/// is accessible, false otherwise. The value is updated
	/// by db_primary_file_validate_existence()
	bool db_primary_file_exists;

	/// The structure contains all database file metadata
	/// to compare within status_of_changes();
	struct stat db_file_stat;

	/**
	 * @brief Flag indicating if the database contains data from previous runs
	 * @details This flag is set to true if the database file exists and contains
	 *    valid data from previous program executions. This differs from
	 *    db_primary_file_exists in that it not only checks for file existence,
	 *    but also verifies that the database has been previously
	 *    populated with data.
	 *
	 *    States:
	 *    - true:  Database exists and contains previously saved data
	 *    - false: Database is either empty or has not been initialized
	 *             with data from previous program runs
	 *
	 * @see db_primary_file_exists
	 */
	bool db_contains_data;

	/// The flags parameter to sqlite3_open_v2()
	/// must include, at a minimum, one of the
	/// following flag combinations:
	///   - SQLITE_OPEN_READONLY
	///   - SQLITE_OPEN_READWRITE
	///   - SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
	///   - SQL_DRY_RUN_MODE
	int sqlite_open_flag;

	/// Must be specified additionally in order
	/// to remove from the database mention of
	/// files that matches the regular expression
	/// passed through the ignore option(s)
	/// This is special protection against accidental
	/// deletion of information from the database.
	bool db_drop_ignored;

	/// Allow dropping database records for files that are inaccessible
	/// (permission denied). Disabled by default to avoid accidental loss
	/// when access rights temporarily change.
	bool db_drop_inaccessible;

	/// Select database validation level: 'quick' for basic
	/// structure check, 'full' for comprehensive
	/// integrity verification
	char db_check_level;

	/// True when the primary database changed during the current scan
	bool db_primary_file_modified;

	/// Recursion depth limit. The depth of the traversal,
	/// numbered from 0 to N, where a file could be found.
	/// Representing the maximum of the starting
	/// point (from root) of the traversal.
	/// The root itself is numbered 0
	/// The variable is assigned a default value of -1
	short maxdepth;

	/// Ignore those relative paths
	/// The string array of PCRE2 regular expressions
	char **ignore;

	/// Suppress per-file log output for paths matched by --ignore
	bool quiet_ignored;

	/// Include those relative paths even if
	/// they were excluded via the --ignore option
	/// The string array of PCRE2 regular expressions
	char **include;

	/// True when at least one --include pattern has been specified.
	/// Used to adjust traversal behavior (e.g., avoid subtree skipping).
	bool include_specified;

	/// Relative paths whose checksums must never be recalculated
	/// after the initial write. PCRE2 regular expressions.
	char **lock_checksum;

	/// Force full checksum verification for entries protected by
	/// --lock-checksum during an update run
	bool rehash_locked;

	/// Dry Run Mode Specification
	/// When operating in Dry Run mode, the system performs validation
	/// and simulates execution without making any actual changes
	/// to data. This includes:
	/// - No modifications to any database records
	/// - No updates to any data structures
	/// - No persistence of state changes
	/// This mode allows for testing and verification of logic while ensuring
	/// data integrity remains untouched.
	bool dry_run;

	/// Enable SHA512 calculation while staying in Dry Run mode.
	/// Activated by: --dry-run=with-checksums
	bool dry_run_with_checksums;

	/// Consider file metadata changes (creation and modification timestamps)
	/// in addition to file size when detecting changes. By default, only
	/// file size changes trigger rescanning. When this option is enabled,
	/// any changes to file timestamps or size will cause the file to be
	/// rescanned and its checksum updated in the database.
	bool watch_timestamps;

	/// This option prevents directory traversal from descending into
	/// directories that have a different device number than the file
	/// from  which the descent began
	bool start_device_only;

	/// Pre-compiled PCRE2 patterns for --ignore (parallel to ignore[])
	/// Compiled once by compile_patterns() after argument parsing
	/// NULL when no --ignore patterns were provided
	pcre2_code **ignore_pcre_compiled;

	/// Pre-compiled PCRE2 patterns for --include (parallel to include[])
	/// Compiled once by compile_patterns() after argument parsing
	/// NULL when no --include patterns were provided
	pcre2_code **include_pcre_compiled;

	/// Pre-compiled PCRE2 patterns for --lock-checksum (parallel to lock_checksum[])
	/// Compiled once by compile_patterns() after argument parsing
	/// NULL when no --lock-checksum patterns were provided
	pcre2_code **lock_checksum_pcre_compiled;

} Config;

/**
 * @brief Summary of a traversal pass for delayed reporting.
 */
typedef struct {
	/// True for the stats-only pass in main().
	/// When enabled, the traversal counts and allocated size are collected
	/// without hashing, database updates, or per-file output.
	bool stats_only_pass;

	/// Total directories encountered during this traversal.
	size_t count_dirs;

	/// Total regular files encountered during this traversal.
	size_t count_files;

	/// Total symlinks encountered during this traversal.
	size_t count_symlnks;

	/// Total allocated size in bytes for encountered files.
	/// Computed from st_blocks using POSIX_STAT_BLOCK_BYTES.
	size_t total_allocated_bytes;

	/// Total bytes actually hashed by SHA512 during this traversal.
	/// This counts the data passed to sha512_update, not allocated size.
	size_t total_hashed_bytes;

	/**
	 * @brief True when current pass emitted at least one visible path-level line.
	 * Set by slog_show()-based output and reset at start of file_list().
	 * Used by file_list() to decide whether to print "File traversal complete".
	 * This is an output marker, not a counter of file changes.
	 */
	bool at_least_one_file_was_shown;

	/// Root path currently being traversed.
	/// Used by slog_show() to include root context in traversal-start output
	const memory *root;

	/// Total elapsed hashing time in nanoseconds for this traversal pass.
	/// Accumulated per-file in sha512sum() from read-loop start to finish.
	long long int total_hashing_elapsed_ns;

} TraversalSummary;

/**
 * @brief Per-file processing state for one FTS_F iteration
 *
 * Carries the database row loaded for the current relative path together with
 * the transient scan, hashing, and reporting state accumulated while the file
 * is being processed
 */
typedef struct {

	/// Database row loaded for this path before processing begins.
	/// Points to a stack-allocated row prepared by the caller and attached to File.
	/// The row starts zeroed, so relative_path_was_in_db_before_processing
	/// is false when the path is not yet present in the database
	DBrow *db;

	/// Set when a file changed while its initial hash was not yet complete,
	/// requiring a full rehash from the beginning
	bool rehashing_from_the_beginning;

	/// True when this path remains excluded after applying --ignore and --include
	bool ignore;

	/// True when the path was explicitly selected or restored by --include
	bool include;

	/// Flag that marks files matched by the checksum lock pattern
	bool locked_checksum_file;

	/// Locked checksum files must not diverge once sealed
	bool lock_checksum_violation;

	/// Detects corruption when rehashing locked files
	bool locked_checksum_mismatch;

	/// Set when SHA512 hashing was gracefully interrupted (e.g. Ctrl+C)
	/// with a non-zero offset saved for later resumption
	bool hash_interrupted;

	/// Decision whether to rehash the file contents using the SHA512 algorithm.
	/// Defaults to true
	bool rehash;

	/// Read access flag for non-ignored paths
	bool is_readable;

	/// Marks zero-length files to avoid unnecessary hashing
	bool zero_size_file;

	/// True when a new database record was successfully inserted for this file
	bool new_db_record_inserted;

	/// True when an existing database record was successfully updated for this file
	bool db_record_updated;

	/// Set by sha512sum() when reading this file fails
	bool read_error;

	/// Bitmask of metadata differences between the saved DB record and the current file.
	/// Default value is NOT_EQUAL
	Changed db_record_vs_file_metadata_changes;

	/// Current byte offset into the file for incremental SHA512 hashing.
	/// Zero means the hash was completed or not yet started
	sqlite3_int64 checksum_offset;

	/// errno snapshot captured by sha512sum() when read_error is true
	int read_errno;

	/// Indicates files that cannot be read or seeked, such as sysfs special files.
	/// When this flag is set, metadata is stored but the checksum is written as NULL
	bool wrong_file_type;

	/// SHA512 digest computed for this file.
	/// All-zero when not yet computed, file is empty, or wrong_file_type is set
	unsigned char sha512[SHA512_DIGEST_LENGTH];

	/// SHA512 incremental hashing context, used for resumable hashing
	SHA512_Context mdContext;

	/// Current filesystem metadata copied from fts_statp for comparisons and DB writes
	CmpctStat stat;

} File;

/*
 *
 * Prototypes of internal functions
 *
 */

#ifdef TESTITALL
/*
 * All static functions for unit testing purposes are declared here
 * Prototypes of functions
 *
 */
#if 0
void remove_leading_dots(char *);
void remove_trailing_dots(char *);
#endif
#endif

/*
 *
 * Prototypes of functions
 *
 */

Return file_list(TraversalSummary *);

void show_statistics(const TraversalSummary *);

void show_elapsed(const TraversalSummary *);

Return sha512sum(
	const int,
	const memory *,
	memory *,
	TraversalSummary *,
	File *,
	bool *);

size_t file_buffer_memory(void);

void free_string_array(char ***);

void free_compiled_array(pcre2_code ***);

Return add_string_to_array(
	char ***,
	const char *);

Return remove_trailing_slash(memory *);

Return path_build_relative(
	memory *,
	const FTSENT *);

LockChecksum match_checksum_lock_pattern(const memory *);

Return path_check_locked_checksum(const memory *);

/**
 * @brief Result of checking accessibility for a given path
 *
 * FILE_ACCESS_ALLOWED    — requested access to the path is permitted
 * FILE_ACCESS_DENIED     — requested access to the path is denied
 * FILE_NOT_FOUND         — path or one of its components does not exist
 * FILE_ACCESS_ERROR      — access checking failed for another reason
 */
typedef enum FileAccessStatus : unsigned int
{
	FILE_ACCESS_ALLOWED = 0u,
	FILE_ACCESS_DENIED = 1u,
	FILE_NOT_FOUND = 2u,
	FILE_ACCESS_ERROR = 3u

} FileAccessStatus;

FileAccessStatus file_access_status(const int);

FileAccessStatus directory_open(
	const memory *,
	int *);

FileAccessStatus directory_open_root(
	const memory *,
	int *);

FileAccessStatus file_check_access(
	const int,
	const memory *,
	const int);

Return show_locked_checksum_unavailable_violation(
	const memory *,
	const FileAccessStatus,
	bool *,
	TraversalSummary *);

void signal_notify_quit_handler(int);

void init_config(void);

Return init_signals(void);

void log_sqlite_error(
	sqlite3 *,
	int,
	char *,
	const char *,
	...);

Return db_close(
	sqlite3 *,
	const bool *);

void db_primary_sync(void);

void free_config(void);

Return db_delete_missing_metadata(void);

Return db_retrieve_root_path(memory *);

Return db_delete_the_record_by_id(const sqlite_int64 *);

Return db_init(void);

Return db_vacuum(const char *);

Return db_primary_consider_vacuum(void);

Return db_read_file_data_from(
	File *,
	const memory *);

Return db_update_the_record_by_id(const File *);

Return db_insert_the_record(
	const memory *,
	const File *);

Return db_save_file_record(
	const memory *,
	File *,
	bool *,
	const bool);

Return db_determine_name(void);

Return db_determine_mode(void);

Return db_save_prefixes(void);

Return db_compare(void);

Return db_validate_paths(void);

Return db_contains_data(void);

Return db_primary_file_validate_existence(void);

Return db_integrity_check(const char *);

Return db_retrieve_version(
	int *,
	const char *);

Return db_check_version(
	const char *,
	const char *);

Return db_upgrade(
	int *,
	const char *,
	const char *);

/* This legacy can be removed in 2034 (10-year Long-Term Support) */
Return db_migrate_from_0_to_1(const char *);

/* This legacy can be removed in 2036 (10-year Long-Term Support) */
Return db_migrate_to_version_4(const char *);

Return db_specify_version(
	const char *,
	int);

Return db_primary_file_test(void);

#if 0 // Disabled multi-root path index implementation
Return db_get_path_prefix_index(
	const char *,
	sqlite3_int64 *);
#endif

Return stat_copy(
	const struct stat *,
	CmpctStat *);

size_t blocks_to_bytes(blkcnt_t);

Changed file_compare_metadata_equivalence(
	const CmpctStat *,
	const CmpctStat *) __attribute__ ((pure));

Return parse_arguments(
	const int,
	char **);

void about(void);

void slog_show_impl(
	const char *,
	const char *,
	int,
	const unsigned int,
	const bool,
	bool *,
	TraversalSummary *,
	const char *,
	...);

void show_file(
	const memory *,
	bool *,
	TraversalSummary *,
	const File *);

void directory_show(
	const memory *,
	bool *,
	TraversalSummary *,
	const bool,
	const bool);

void show_metadata(
	LOGMODES,
	Changed,
	const CmpctStat *,
	const CmpctStat *);

Return show_difference(
	Changed,
	const CmpctStat *,
	const CmpctStat *);

void show_checksum_gracefully_interrupted(
	const char *,
	const sqlite3_int64 *);

Return status_of_changes(void);

Return show_remembered_messages(void);

Return directory_access_verify(
	FTS *,
	FTSENT *,
	const int,
	const memory *,
	bool *,
	TraversalSummary *);

Return db_check_changes(void);

FileAvailability file_availability(
	const char *,
	struct stat *,
	const unsigned char);

Return paths_detect(void);

Ignore match_ignore_pattern(const memory *);

Include match_include_pattern(const memory *);

Return match_include_ignore(
	const memory *,
	bool *,
	bool *);

REGEXP match_regexp(
	pcre2_code *,
	const memory *);

Return compile_patterns(void);

int exit_status(
	Return,
	char * const *);

extern _Atomic bool global_interrupt_flag;

extern Config _config;

extern Config *config;

#ifdef TESTITALL_TEST_HOOKS
#include "testitall_test_hooks.h"
#endif

#endif /* _PRECIZER_H */
