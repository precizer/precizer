#pragma once

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Target the fread mock at files whose path ends with the given suffix
 *
 * @param[in] suffix Path suffix to match
 */
void mocks_fread_set_target_suffix(const char *suffix);

/**
 * @brief Enable or disable the fread mock failure path
 *
 * @param[in] enabled New enabled state
 */
void mocks_fread_enable(bool enabled);

/**
 * @brief Set errno reported by the mocked fread failure
 *
 * @param[in] err Errno value to expose through the mock
 */
void mocks_fread_set_errno(int err);

/**
 * @brief Reset all fread mock state to defaults
 */
void mocks_fread_reset(void);

/**
 * @brief Return how many times the mocked fread failure path was triggered
 *
 * @return Number of intercepted calls
 */
size_t mocks_fread_call_count(void);

/**
 * @brief Target the remove mock at files whose path ends with the given suffix
 *
 * @param[in] suffix Path suffix to match
 */
void mocks_remove_set_target_suffix(const char *suffix);

/**
 * @brief Enable or disable the remove mock failure path
 *
 * @param[in] enabled New enabled state
 */
void mocks_remove_enable(bool enabled);

/**
 * @brief Set errno reported by the mocked remove failure
 *
 * @param[in] err Errno value to expose through the mock
 */
void mocks_remove_set_errno(int err);

/**
 * @brief Reset all remove mock state to defaults
 */
void mocks_remove_reset(void);

/**
 * @brief Return how many times the mocked remove failure path was triggered
 *
 * @return Number of intercepted calls
 */
size_t mocks_remove_call_count(void);
