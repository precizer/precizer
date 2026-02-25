/**
 * @file db_upgrade.h
 * @brief Shared structures for database upgrade steps.
 */

#ifndef _DB_UPGRADE_H
#define _DB_UPGRADE_H

#include <sys/types.h>
#include <time.h>

/**
 * @brief Compact stat layout used in DB versions 1..3.
 *
 * This is the legacy stat blob format stored before DB version 4.
 *
 * This legacy can be removed in 2036 (10-year Long-Term Support)
 */
typedef struct {
	off_t st_size;
	time_t mtim_tv_sec;
	long mtim_tv_nsec;
	time_t ctim_tv_sec;
	long ctim_tv_nsec;
} CmpctStat_v1;
#endif // _DB_UPGRADE_H
