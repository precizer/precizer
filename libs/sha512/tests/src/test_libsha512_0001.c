#include "test_libsha512_utils.h"

/* Published SHA-512 digests for the reference messages used below. The empty,
   "abc" and 896-bit multi-block vectors come from FIPS-180-2 Appendix C; the
   "Hello World" vector is a widely cited non-FIPS check that lets a developer
   eyeball the library output against any common SHA-512 calculator */

/* FIPS-180-2 Appendix C.1 (empty input). Anchors the simplest SHA-512
   contract: zero input bytes must still produce the standard 64-byte digest
   with correct length-encoded padding */
static const unsigned char empty_digest[SHA512_DIGEST_LENGTH] = {
	0xcf,0x83,0xe1,0x35,0x7e,0xef,0xb8,0xbd,
	0xf1,0x54,0x28,0x50,0xd6,0x6d,0x80,0x07,
	0xd6,0x20,0xe4,0x05,0x0b,0x57,0x15,0xdc,
	0x83,0xf4,0xa9,0x21,0xd3,0x6c,0xe9,0xce,
	0x47,0xd0,0xd1,0x3c,0x5d,0x85,0xf2,0xb0,
	0xff,0x83,0x18,0xd2,0x87,0x7e,0xec,0x2f,
	0x63,0xb9,0x31,0xbd,0x47,0x41,0x7a,0x81,
	0xa5,0x38,0x32,0x7a,0xf9,0x27,0xda,0x3e
};

/* FIPS-180-2 Appendix C.1 (24-bit "abc" message). The canonical short
   single-block vector quoted by virtually every SHA-512 reference, so a
   failure here points directly at the core compression function */
static const unsigned char abc_digest[SHA512_DIGEST_LENGTH] = {
	0xdd,0xaf,0x35,0xa1,0x93,0x61,0x7a,0xba,
	0xcc,0x41,0x73,0x49,0xae,0x20,0x41,0x31,
	0x12,0xe6,0xfa,0x4e,0x89,0xa9,0x7e,0xa2,
	0x0a,0x9e,0xee,0xe6,0x4b,0x55,0xd3,0x9a,
	0x21,0x92,0x99,0x2a,0x27,0x4f,0xc1,0xa8,
	0x36,0xba,0x3c,0x23,0xa3,0xfe,0xeb,0xbd,
	0x45,0x4d,0x44,0x23,0x64,0x3c,0xe8,0x0e,
	0x2a,0x9a,0xc9,0x4f,0xa5,0x4c,0xa4,0x9f
};

/* Informal "Hello World" check. Not part of FIPS-180-2, but widely cited;
   convenient because any general-purpose SHA-512 tool will compute the same
   digest without project-specific context */
static const unsigned char hello_world_digest[SHA512_DIGEST_LENGTH] = {
	0x2c,0x74,0xfd,0x17,0xed,0xaf,0xd8,0x0e,
	0x84,0x47,0xb0,0xd4,0x67,0x41,0xee,0x24,
	0x3b,0x7e,0xb7,0x4d,0xd2,0x14,0x9a,0x0a,
	0xb1,0xb9,0x24,0x6f,0xb3,0x03,0x82,0xf2,
	0x7e,0x85,0x3d,0x85,0x85,0x71,0x9e,0x0e,
	0x67,0xcb,0xda,0x0d,0xaa,0x8f,0x51,0x67,
	0x10,0x64,0x61,0x5d,0x64,0x5a,0xe2,0x7a,
	0xcb,0x15,0xbf,0xb1,0x44,0x7f,0x45,0x9b
};

/* FIPS-180-2 Appendix C.2 (896-bit, 112-byte message). After SHA-512 padding
   this input straddles the 128-byte block boundary, so the same one-shot API
   call also exercises the multi-block compression path that the short
   single-block vectors above cannot reach */
static const unsigned char fips_multi_block_digest[SHA512_DIGEST_LENGTH] = {
	0x8e,0x95,0x9b,0x75,0xda,0xe3,0x13,0xda,
	0x8c,0xf4,0xf7,0x28,0x14,0xfc,0x14,0x3f,
	0x8f,0x77,0x79,0xc6,0xeb,0x9f,0x7f,0xa1,
	0x72,0x99,0xae,0xad,0xb6,0x88,0x90,0x18,
	0x50,0x1d,0x28,0x9e,0x49,0x00,0xf7,0xe4,
	0x33,0x1b,0x99,0xde,0xc4,0xb5,0x43,0x3a,
	0xc7,0xd3,0x29,0xee,0xb6,0xdd,0x26,0x54,
	0x5e,0x96,0xe5,0x5b,0x87,0x4b,0xe9,0x09
};

/* Use recognizable messages so the expected digests can be checked against
   common SHA-512 references without extra project context */
static const char empty_message[] = "";
static const char abc_message[] = "abc";
static const char hello_world_message[] = "Hello World";

/* FIPS-180-2 Appendix C.2 sequence: 112 ASCII bytes that, together with
   SHA-512 padding, span two 128-byte compression blocks */
static const char fips_multi_block_message[] =
	"abcdefghbcdefghicdefghijdefghijkefghijkl"
	"fghijklmghijklmnhijklmnoijklmnopjklmnopq"
	"klmnopqrlmnopqrsmnopqrstnopqrstu";

/**
 * @brief Check the published SHA-512 digest for empty input
 * @details Empty input is a useful user-facing baseline: callers can hash zero
 * bytes and still expect a stable, standard 64-byte SHA-512 digest
 *
 * @return SUCCESS when the empty message matches the FIPS reference digest
 */
static Return test_libsha512_0001_1(void)
{
	INITTEST;

	/* Keep the message and expected digest together so this subtest reads as one
	   small reference example */
	const sha512_digest_vector vector = {
		.message_text = empty_message,
		.message_size = sizeof(empty_message) - 1U,
		.expected_digest = empty_digest
	};

	/* Run the vector through the shared helper used by every reference case */
	run(check_sha512_vector(&vector));

	RETURN_STATUS;
}

/**
 * @brief Check the canonical SHA-512 digest for "abc"
 * @details The "abc" vector is the short SHA-512 example most developers know.
 * It proves that ordinary single-block text input reaches the expected digest
 *
 * @return SUCCESS when "abc" matches the FIPS reference digest
 */
static Return test_libsha512_0001_2(void)
{
	INITTEST;

	/* Keep this compact reference vector local to the subtest that reports it */
	const sha512_digest_vector vector = {
		.message_text = abc_message,
		.message_size = sizeof(abc_message) - 1U,
		.expected_digest = abc_digest
	};

	/* The common helper handles libmem setup, hashing, and byte comparison */
	run(check_sha512_vector(&vector));

	RETURN_STATUS;
}

/**
 * @brief Check the widely cited SHA-512 digest for "Hello World"
 * @details This vector is not a FIPS appendix case, but it is easy to reproduce
 * with common SHA-512 tools and helps people sanity-check the library quickly
 *
 * @return SUCCESS when "Hello World" matches the expected public digest
 */
static Return test_libsha512_0001_3(void)
{
	INITTEST;

	/* Use an informal but recognizable message that is convenient to verify
	   outside the project */
	const sha512_digest_vector vector = {
		.message_text = hello_world_message,
		.message_size = sizeof(hello_world_message) - 1U,
		.expected_digest = hello_world_digest
	};

	/* Hash and compare the vector through the same route as the FIPS examples */
	run(check_sha512_vector(&vector));

	RETURN_STATUS;
}

/**
 * @brief Check the FIPS multi-block SHA-512 example
 * @details The 896-bit FIPS vector forces SHA-512 finalization to process more
 * than one block, so it protects behavior that short examples cannot reach
 *
 * @return SUCCESS when the 896-bit message matches the FIPS reference digest
 */
static Return test_libsha512_0001_4(void)
{
	INITTEST;

	/* Keep the long FIPS vector in one descriptor so a failure names this exact
	   multi-block reference case in the test output */
	const sha512_digest_vector vector = {
		.message_text = fips_multi_block_message,
		.message_size = sizeof(fips_multi_block_message) - 1U,
		.expected_digest = fips_multi_block_digest
	};

	/* The shared vector helper keeps this check consistent with the short cases */
	run(check_sha512_vector(&vector));

	RETURN_STATUS;
}

/**
 * @brief Check SHA-512 answers that users can verify from public references
 * @details Three compact messages (the empty string, "abc" and "Hello World")
 * confirm that the library produces the standard SHA-512 output for ordinary
 * single-block callers. A fourth vector is the 896-bit two-block example from
 * FIPS-180-2 Appendix C.2, which also exercises the multi-block compression
 * path through the very same one-shot API
 *
 * @return SUCCESS when all reference messages produce their expected digests
 */
Return test_libsha512_0001(void)
{
	INITTEST;

	/* Each vector is its own SUTE test so the output names the exact reference
	   message that failed */
	TEST(test_libsha512_0001_1,"Empty input hashes to the FIPS SHA-512 digest...");
	TEST(test_libsha512_0001_2,"The canonical abc message hashes to the FIPS SHA-512 digest...");
	TEST(test_libsha512_0001_3,"Hello World hashes to the widely cited SHA-512 digest...");
	TEST(test_libsha512_0001_4,"The 896-bit FIPS message exercises multi-block SHA-512 hashing...");

	RETURN_STATUS;
}
