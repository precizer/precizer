/**
 *
 * @file helpers.h
 * @brief Shared helper function declarations for tests
 *
 */

#ifndef _TEST_HELPERS_H
#define _TEST_HELPERS_H

#include "precizer.h"

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
	const char     *db_filename,
	const char     *relative_path,
	sqlite3_int64  *offset_out,
	int            *md_context_bytes_out);

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
	const char     *db_filename,
	const char     *relative_path,
	sqlite3_int64  *offset_out,
	unsigned char  *sha512_out);

/**
 * @brief Add a new file by truncating the named file to zero size
 *
 * @param[in] relative_path_to_tmpdir File path relative to TMPDIR
 *
 * @return Return status code
 */
Return truncate_file_to_zero_size(
	const char *relative_path_to_tmpdir);

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
 * @param[in] blocks_before_rewrite Expected allocated block count lower bound
 *
 * @return Return status code
 */
Return rewrite_file_dense_with_same_size(
	const char   *relative_path_to_tmpdir,
	const off_t  target_size,
	const blkcnt_t blocks_before_rewrite);

/**
 * @brief Calculate SHA512 digest of a file
 *
 * @param[in] file_path File path relative to TMPDIR or absolute path
 * @param[out] sha512_out Output SHA512 bytes with SHA512_DIGEST_LENGTH size
 *
 * @return Return status code
 */
Return compute_file_sha512(
	const char    *file_path,
	unsigned char *sha512_out);

/**
 * @brief Append one byte to a file
 *
 * @param[in] file_path File path relative to TMPDIR or absolute path
 * @param[in] byte Byte value to append
 *
 * @return Return status code
 */
Return append_byte_to_file(
	const char   *file_path,
	unsigned char byte);

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
Return tamper_locked_file_bytes(
	const char *relative_path);

#endif
