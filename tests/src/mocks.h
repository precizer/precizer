#pragma once

#include <stdbool.h>
#include <stddef.h>

void mocks_fread_set_target_suffix(const char *suffix);
void mocks_fread_enable(bool enabled);
void mocks_fread_set_errno(int err);
void mocks_fread_reset(void);
size_t mocks_fread_call_count(void);
