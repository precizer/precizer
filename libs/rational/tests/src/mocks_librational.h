#pragma once

#include <stddef.h>

void mocks_librational_snprintf_fail_next(size_t);

void mocks_librational_snprintf_truncate_next(size_t);

void mocks_librational_write_fail_next(size_t,int);

void mocks_librational_disable(void);
