#ifndef _TESTITALL_TEST_HOOKS_H
#define _TESTITALL_TEST_HOOKS_H

#ifdef TESTITALL_TEST_HOOKS
bool testitall_file_access_status_override(
	const char *,
	FileAccessStatus *);

Return testitall_db_bump_timestamps(void);

Return testitall_db_resync_stat_baseline(const struct stat *);

bool testitall_is_huge_interruption_target(const char *);

uint64_t testitall_random_stop_byte(const uint64_t);

bool testitall_hash_checkpoint_at_random_byte_enabled(void);

bool testitall_exit_after_hash_checkpoint_enabled(void);

#if defined(__GNUC__)
__attribute__((noreturn))
#endif
void testitall_exit_after_hash_checkpoint(void);

void signal_wait_at_point(unsigned int);
#endif

#endif /* _TESTITALL_TEST_HOOKS_H */
