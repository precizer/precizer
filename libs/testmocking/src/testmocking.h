#pragma once

#include <stddef.h>

void testmocking_malloc_fail_next(size_t);

void testmocking_malloc_disable(void);

void testmocking_realloc_fail_next(size_t);

void testmocking_realloc_disable(void);

void testmocking_snprintf_fail_next(size_t);

void testmocking_snprintf_truncate_next(size_t);

void testmocking_snprintf_disable(void);

void testmocking_gettimeofday_fail_next(size_t);

void testmocking_gettimeofday_disable(void);

void testmocking_localtime_r_fail_next(size_t);

void testmocking_localtime_r_disable(void);

void testmocking_vsnprintf_fail_next(size_t);

void testmocking_vsnprintf_disable(void);

void testmocking_write_fail_next(size_t,int);

void testmocking_write_disable(void);
