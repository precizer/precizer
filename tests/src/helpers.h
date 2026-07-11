/**
 *
 * @file helpers.h
 * @brief Shared helper function declarations for tests
 *
 */

#ifndef _TEST_HELPERS_H
#define _TEST_HELPERS_H

#include "precizer.h"
#include <stdio.h>

#define REQUIRE_SOURCE_EXISTS ((unsigned int)0U)
#define ALLOW_MISSING_SOURCE  ((unsigned int)1U)
#define FILE_WRITE_APPEND  ((unsigned int)1U)
#define FILE_WRITE_REPLACE ((unsigned int)2U)

Return open_db_from_tmpdir(
	const char *db_filename,
	const int  open_flags,
	sqlite3    **db_out);

/**
 * @brief Verify that files.relative_path values in DB match expected list
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] expected_paths Sorted expected relative_path values
 * @param[in] expected_count Number of expected paths
 *
 * @return Return status code
 */
Return db_paths_match(
	const char        *db_filename,
	const char *const *expected_paths,
	const int         expected_count);

/**
 * @brief Check whether one files-table row exists by relative path
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 * @param[out] exists_out Output existence flag
 *
 * @return Return status code
 */
Return db_relative_path_exists(
	const char *db_filename,
	const char *relative_path,
	bool       *exists_out);

Return db_read_file_id(
	const char    *db_filename,
	const char    *relative_path,
	sqlite3_int64 *id_out);

Return db_read_first_row_id(
	const char    *db_filename,
	sqlite3_int64 *row_id_out);

Return db_overwrite_stat_blob_by_row_id(
	const char          *db_filename,
	const sqlite3_int64 row_id,
	const void          *blob,
	const int           blob_size);

Return db_corrupt_first_row_stat_blob(
	const char    *db_filename,
	sqlite3_int64 *row_id_out);

/**
 * @brief Read resume-related state for one file from files table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 * @param[out] offset_out Output offset value from files table
 * @param[out] md_context_bytes_out Output mdContext blob size in bytes
 *
 * @return Return status code
 */
Return read_resume_state_from_db(
	const char    *db_filename,
	const char    *relative_path,
	sqlite3_int64 *offset_out,
	int           *md_context_bytes_out);

Return db_resume_state_is_empty(
	const char *db_filename,
	const char *relative_path);

/**
 * @brief Read number of rows from files table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[out] count_out Output row count
 *
 * @return Return status code
 */
Return db_read_files_count(
	const char *db_filename,
	int        *count_out);

Return db_read_files_count_with_blob_size(
	const char *db_filename,
	const int  blob_size,
	int        *count_out);

/**
 * @brief Read db_version value from metadata table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[out] db_version_out Output database version
 *
 * @return Return status code
 */
Return read_db_version_from_metadata(
	const char *db_filename,
	int        *db_version_out);

Return set_db_version_in_metadata(
	const char *db_filename,
	const int  db_version);

Return db_read_stat_blob_by_row_id(
	const char          *db_filename,
	const sqlite3_int64 row_id,
	unsigned char       *blob_out,
	const size_t        blob_out_size,
	int                 *blob_size_out);

Return db_read_cmpctstat_by_row_id(
	const char          *db_filename,
	const sqlite3_int64 row_id,
	CmpctStat           *stat_out);

Return db_read_cmpctstat_by_relative_path(
	const char *db_filename,
	const char *relative_path,
	CmpctStat  *stat_out);

bool cmpctstat_matches_stat_timestamps(
	const CmpctStat   *db_stat,
	const struct stat *file_stat);

/**
 * @brief Read final offset and SHA512 checksum for one file from files table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 * @param[out] offset_out Output offset value, 0 when SQL value is NULL
 * @param[out] sha512_out Output SHA512 bytes with SHA512_DIGEST_LENGTH size
 *
 * @return Return status code
 */
Return read_final_sha512_from_db(
	const char    *db_filename,
	const char    *relative_path,
	sqlite3_int64 *offset_out,
	unsigned char *sha512_out);

Return db_final_sha512_matches_file(
	const char *db_filename,
	const char *relative_path,
	const char *file_path);

/**
 * @brief Set files.sha512 to NULL for one row in the database
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 *
 * @return Return status code
 */
Return db_set_sha512_to_null(
	const char *db_filename,
	const char *relative_path);

Return db_tamper_sha512(
	const char *db_filename,
	const char *relative_path);

/**
 * @brief Add a new file by truncating the named file to zero size
 *
 * @param[in] relative_path_to_tmpdir File path relative to TMPDIR
 *
 * @return Return status code
 */
Return truncate_file_to_zero_size(const char *relative_path_to_tmpdir);

/**
 * @brief Create symbolic link by path relative to TMPDIR
 *
 * The target string is stored in the link exactly as provided
 *
 * @param[in] link_target Literal target string for the symbolic link
 * @param[in] relative_link_path_to_tmpdir Symbolic link path relative to TMPDIR
 *
 * @return Return status code
 */
Return create_symlink(
	const char *link_target,
	const char *relative_link_path_to_tmpdir);

/**
 * @brief Change file mode by path relative to TMPDIR
 *
 * The provided mode is applied exactly with native `chmod()`
 *
 * @param[in] relative_path_to_tmpdir File or directory path relative to TMPDIR
 * @param[in] new_mode Exact mode value to apply
 *
 * @return Return status code
 */
Return change_mode(
	const char   *relative_path_to_tmpdir,
	unsigned int new_mode);

/**
 * @brief Create directory tree relative to TMPDIR using native filesystem calls
 *
 * Existing directories are accepted when every path component resolves to a
 * directory, including symlinks to directories
 *
 * @param[in] relative_path_to_tmpdir Directory path relative to TMPDIR
 *
 * @return Return status code
 */
Return create_directory(const char *relative_path_to_tmpdir);

/**
 * @brief Remove file or directory tree by path relative to TMPDIR
 *
 * When relative_path_to_tmpdir is an empty string, the function targets TMPDIR itself
 * Missing paths are treated as a hard failure to keep test cleanup strict and deterministic
 *
 * @param[in] relative_path_to_tmpdir File or directory path relative to TMPDIR
 *
 * @return Return status code
 */
Return delete_path(const char *relative_path_to_tmpdir);

Return delete_path_if_present(const char *relative_path_to_tmpdir);

Return prepare_huge_fixture(
	memory      *huge_file_path,
	struct stat *huge_file_stat_out);

/**
 * @brief Copy file or directory tree by path relative to TMPDIR
 *
 * Empty source or destination path resolves to TMPDIR root
 * The destination path is treated as exact target path and must not be an existing directory
 *
 * @param[in] relative_source_path Source file or directory path relative to TMPDIR
 * @param[in] relative_destination_path Destination file or directory path relative to TMPDIR
 *
 * @return Return status code
 */
Return copy_path(
	const char *relative_source_path,
	const char *relative_destination_path);

/**
 * @brief Copy file or directory tree from ORIGIN_DIR into TMPDIR
 *
 * Empty source or destination path resolves to the corresponding environment root
 * Missing sources fail by default with @ref REQUIRE_SOURCE_EXISTS
 * Optional missing sources can be ignored with @ref ALLOW_MISSING_SOURCE
 *
 * @param[in] relative_source_path_to_origin_dir Source path relative to ORIGIN_DIR
 * @param[in] relative_destination_path_to_tmpdir Destination path relative to TMPDIR
 * @param[in] flags Behavior flags such as @ref REQUIRE_SOURCE_EXISTS or @ref ALLOW_MISSING_SOURCE
 *
 * @return Return status code
 */
Return copy_from_origin(
	const char   *relative_source_path_to_origin_dir,
	const char   *relative_destination_path_to_tmpdir,
	unsigned int flags);

/**
 * @brief Move file or directory by path relative to TMPDIR using native rename
 *
 * Empty source or destination path resolves to TMPDIR root
 * This operation does not fallback to copy and delete on cross-device moves
 *
 * @param[in] relative_source_path Source file or directory path relative to TMPDIR
 * @param[in] relative_destination_path Destination file or directory path relative to TMPDIR
 *
 * @return Return status code
 */
Return move_path(
	const char *relative_source_path,
	const char *relative_destination_path);

/**
 * @brief Prepare a mutable working copy for a fixture path relative to TMPDIR
 *
 * The pristine backup is stored under the hidden `.fixture_backups` root
 *
 * @param[in] fixture_path Fixture path relative to TMPDIR
 *
 * @return Return status code
 */
Return prepare_mutable_fixture(const char *fixture_path);

/**
 * @brief Restore a fixture path from the hidden mutable-fixture backup
 *
 * The working copy is deleted before the pristine backup is moved back
 *
 * @param[in] fixture_path Fixture path relative to TMPDIR
 *
 * @return Return status code
 */
Return restore_mutable_fixture(const char *fixture_path);

/**
 * @brief Create sparse growth via hole punch and explicit final size
 *
 * @param[in] relative_path_to_tmpdir File path relative to TMPDIR
 * @param[out] new_size_out Output resulting file size in bytes
 * @param[out] blocks_after_change_out Output allocated block count after change
 *
 * @return Return status code
 */
Return make_sparse_size_change_without_allocated_block_growth(
	const char *relative_path_to_tmpdir,
	off_t      *new_size_out,
	blkcnt_t   *blocks_after_change_out);

/**
 * @brief Rewrite file densely while preserving target size constraints
 *
 * @param[in] relative_path_to_tmpdir File path relative to TMPDIR
 * @param[in] target_size Required final file size
 * @param[in] blocks_before_rewrite Expected allocated block count before the rewrite
 *
 * @return Return status code
 */
Return rewrite_file_dense_with_same_size(
	const char     *relative_path_to_tmpdir,
	const off_t    target_size,
	const blkcnt_t blocks_before_rewrite);

/**
 * @brief Calculate SHA512 digest of a file using Monocypher
 *
 * @param[in] file_path File path passed directly to `fopen()`
 * @param[out] sha512_out Output SHA512 bytes with SHA512_DIGEST_LENGTH size
 *
 * @return Return status code
 */
Return compute_file_sha512_monocypher(
	const char    *file_path,
	unsigned char *sha512_out);

/**
 * @brief Append one byte to a file
 *
 * @param[in] file_path_buffer Managed path string passed directly to `open_file_stream()`
 * @param[in] byte Byte value to append
 *
 * @return Return status code
 */
Return append_byte_to_file(
	const memory  *file_path_buffer,
	unsigned char byte);

/**
 * @brief Open a writable file stream with explicit create mode 0600
 *
 * Supports only `"ab"` and `"wb"` modes
 *
 * @param[in] file_path Managed path string passed directly to `open()`
 * @param[in] stream_open_mode Mode string for `fdopen()`
 * @param[out] opened_file_stream_out Output writable stream
 *
 * @return Return status code
 */
Return open_file_stream(
	const memory *file_path,
	const char   *stream_open_mode,
	FILE         **opened_file_stream_out);

/**
 * @brief Verify access to a file path relative to a test root under TMPDIR
 *
 * The root path is resolved from TMPDIR, opened with directory_open(), and the
 * relative path is checked with file_check_access()
 *
 * @param[in] root_path_to_tmpdir Root directory path relative to TMPDIR
 * @param[in] relative_path Path to check relative to the opened root
 * @param[in] mode Access mode passed to file_check_access()
 * @param[in] expected_status Required access-check result
 *
 * @return Return status code
 */
Return expect_file_access_from_root(
	const char       *root_path_to_tmpdir,
	const char       *relative_path,
	const int        mode,
	FileAccessStatus expected_status);

/**
 * @brief Write string to file with explicit append or replace mode
 *
 * The function writes bytes exactly as provided without adding a trailing newline
 *
 * @param[in] file_content String payload to write
 * @param[in] file_path File path relative to TMPDIR
 * @param[in] write_flags One of FILE_WRITE_APPEND or FILE_WRITE_REPLACE
 *
 * @return Return status code
 */
Return write_string_to_file(
	const char         *file_content,
	const char         *file_path,
	const unsigned int write_flags);

/**
 * @brief Append string bytes to file without newline
 *
 * @param[in] file_content String payload to append
 * @param[in] file_path File path relative to TMPDIR
 *
 * @return Return status code
 */
Return add_string_to(
	const char *file_content,
	const char *file_path);

/**
 * @brief Replace file content with string bytes without newline
 *
 * @param[in] file_content String payload to write
 * @param[in] file_path File path relative to TMPDIR
 *
 * @return Return status code
 */
Return replase_to_string(
	const char *file_content,
	const char *file_path);

/**
 * @brief Set target file mtime based on source mtime plus nanosecond delta
 *
 * @param[in] relative_source_path Source path relative to TMPDIR or NULL to use target mtime as reference
 * @param[in] relative_target_path Target path relative to TMPDIR
 * @param[in] mtime_delta_nanoseconds Signed nanosecond delta applied to reference mtime
 *
 * @return Return status code
 */
Return touch_file_mtime_with_reference_delta_ns(
	const char *relative_source_path,
	const char *relative_target_path,
	int64_t    mtime_delta_nanoseconds);

/**
 * @brief Modify first two bytes in a locked fixture file without changing size
 *
 * @param[in] relative_path File path relative to TMPDIR
 *
 * @return Return status code
 */
Return tamper_locked_file_bytes(const char *relative_path);

#endif
