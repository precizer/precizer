/**
 *
 * @file helpers.h
 * @brief Shared helper function declarations for tests
 *
 */

#ifndef _TEST_HELPERS_H
#define _TEST_HELPERS_H

#include "precizer.h"

Return db_paths_match(
	const char        *db_filename,
	const char *const *expected_paths,
	const int         expected_count);

Return read_resume_state_from_db(
	const char     *db_filename,
	const char     *relative_path,
	sqlite3_int64  *offset_out,
	int            *md_context_bytes_out);

Return truncate_file_to_zero_size(
	const char *relative_path_to_tmpdir);

Return make_sparse_size_change_without_allocated_block_growth(
	const char *relative_path_to_tmpdir,
	off_t      *new_size_out,
	blkcnt_t   *blocks_after_change_out);

Return rewrite_file_dense_with_same_size(
	const char   *relative_path_to_tmpdir,
	const off_t  target_size,
	const blkcnt_t blocks_before_rewrite);

Return compute_file_sha512(
	const char    *file_path,
	unsigned char *sha512_out);

Return append_byte_to_file(
	const char   *file_path,
	unsigned char byte);

Return touch_file_mtime_with_reference_delta_ns(
	const char *relative_source_path,
	const char *relative_target_path,
	int64_t    mtime_delta_nanoseconds);

Return tamper_locked_file_bytes(
	const char *relative_path);

#endif
