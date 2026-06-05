#pragma once

#include <stdbool.h>
#include <stddef.h>

void mocks_fread_set_target_suffix(const char *);

void mocks_fread_enable(bool);

void mocks_fread_set_errno(int);

void mocks_fread_reset(void);

size_t mocks_fread_call_count(void);

void mocks_remove_set_target_suffix(const char *);

void mocks_remove_enable(bool);

void mocks_remove_set_errno(int);

void mocks_remove_reset(void);

size_t mocks_remove_call_count(void);

void mocks_access_set_target_suffix(const char *);

void mocks_access_enable(bool);

void mocks_access_set_errno(int);

void mocks_access_reset(void);

size_t mocks_access_call_count(void);
