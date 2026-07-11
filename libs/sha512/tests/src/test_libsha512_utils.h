#ifndef LIBSHA512_TEST_LIBSHA512_UTILS_H
#define LIBSHA512_TEST_LIBSHA512_UTILS_H

#include "sute.h"

typedef struct sha512_digest_vector {
	const char *message_text;
	size_t message_size;
	const unsigned char *expected_digest;
} sha512_digest_vector;

Return calculate_sha512_digest(
	const unsigned char *,
	size_t,
	memory *);

Return calculate_sha512_digest_monocypher(
	const unsigned char *,
	size_t,
	memory *);

Return calculate_sha512_digest_in_chunks(
	const unsigned char *,
	size_t,
	const memory *,
	memory *);

Return assert_sha512_digest_matches(
	const memory *,
	const memory *);

Return check_sha512_vector(const sha512_digest_vector *);
#endif
