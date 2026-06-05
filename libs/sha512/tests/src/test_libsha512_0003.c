#include "test_libsha512_utils.h"

/* Largest byte-aligned bit count; relies on SHA512_MAX_BIT_COUNT == UINT64_MAX */
#define SHA512_TEST_MAX_BYTE_ALIGNED_BITS (UINT64_MAX & ~(uint64_t)7U)

/**
 * @brief Check that the public API rejects missing pointers
 * @details NULL arguments are caller mistakes. The SHA-512 functions should
 * report them as ordinary failures instead of touching memory through them
 *
 * @return SUCCESS when each NULL argument is rejected cleanly
 */
static Return test_libsha512_0003_1(void)
{
	INITTEST;

	static const unsigned char sample_bytes[] = {0x61};
	SHA512_Context context = {0};

	/* Build a minimal valid byte buffer and digest destination. The NULL checks
	   below should fail because of the tested argument, not because setup is bad */
	m_create(unsigned char,input);
	m_create(unsigned char,digest);

	ASSERT(SUCCESS == m_copy_buffer(input,sizeof(sample_bytes),sample_bytes));
	ASSERT(SUCCESS == m_resize(digest,SHA512_DIGEST_LENGTH,ZERO_NEW_MEMORY));

	/* Extract raw pointers only after libmem has prepared the descriptors */
	const unsigned char *input_data = m_data_ro(unsigned char,input);
	unsigned char *digest_data = m_data(unsigned char,digest);

	/* Every public SHA-512 entry point should reject NULL storage clearly and
	   leave the caller with a named error code */
	ASSERT(input_data != NULL);
	ASSERT(digest_data != NULL);
	ASSERT(CRYPT_INVALID_ARG == sha512_init(NULL));
	ASSERT(CRYPT_OK == sha512_init(&context));
	ASSERT(CRYPT_INVALID_ARG == sha512_update(NULL,input_data,input->length));
	ASSERT(CRYPT_INVALID_ARG == sha512_update(&context,NULL,input->length));
	ASSERT(CRYPT_INVALID_ARG == sha512_final(NULL,digest_data));
	ASSERT(CRYPT_INVALID_ARG == sha512_final(&context,NULL));

	/* Mandatory cleanup uses call() so descriptors are freed even after a failed
	   assertion inside this negative-path test */
	call(m_del(input));
	call(m_del(digest));

	RETURN_STATUS;
}

/**
 * @brief Check that a damaged buffered-byte count is rejected
 * @details A context with a full internal buffer should never be handed back to
 * update or finalization. Rejecting it keeps the API from looping forever or
 * writing past the valid SHA-512 block area
 *
 * @return SUCCESS when damaged buffer state is rejected
 */
static Return test_libsha512_0003_2(void)
{
	INITTEST;

	static const unsigned char sample_bytes[] = {0x61};
	SHA512_Context context = {0};

	/* Prepare ordinary input and output buffers first. The only damaged value in
	   this scenario should be the context's buffered-byte counter */
	m_create(unsigned char,input);
	m_create(unsigned char,digest);

	ASSERT(SUCCESS == m_copy_buffer(input,sizeof(sample_bytes),sample_bytes));
	ASSERT(SUCCESS == m_resize(digest,SHA512_DIGEST_LENGTH,ZERO_NEW_MEMORY));

	/* Convert libmem descriptors to raw pointers after their sizes are known */
	const unsigned char *input_data = m_data_ro(unsigned char,input);
	unsigned char *digest_data = m_data(unsigned char,digest);

	/* Start from a real initialized context, then simulate a corrupted caller
	   state that claims the internal buffer is already full */
	ASSERT(input_data != NULL);
	ASSERT(digest_data != NULL);
	ASSERT(CRYPT_OK == sha512_init(&context));

	context.curlen = sizeof(context.buf);
	ASSERT(CRYPT_INVALID_ARG == sha512_update(&context,input_data,input->length));
	ASSERT(CRYPT_INVALID_ARG == sha512_final(&context,digest_data));

	/* Free local descriptors regardless of which invalid-state assertion fails */
	call(m_del(input));
	call(m_del(digest));

	RETURN_STATUS;
}

/**
 * @brief Check that the message length counter cannot wrap
 * @details This implementation stores the SHA-512 message length in a 64-bit
 * bit counter. When a context is already at the largest byte-aligned value, one
 * more byte must be rejected before the counter can overflow
 *
 * @return SUCCESS when update and finalization both report CRYPT_HASH_OVERFLOW
 */
static Return test_libsha512_0003_3(void)
{
	INITTEST;

	static const unsigned char sample_bytes[] = {0x61};
	SHA512_Context context = {0};

	/* Build one extra byte of input and a digest buffer. Both are valid, so the
	   expected failure can only come from the length counter limit */
	m_create(unsigned char,input);
	m_create(unsigned char,digest);

	ASSERT(SUCCESS == m_copy_buffer(input,sizeof(sample_bytes),sample_bytes));
	ASSERT(SUCCESS == m_resize(digest,SHA512_DIGEST_LENGTH,ZERO_NEW_MEMORY));

	/* Work with raw pointers after libmem has finished allocating storage */
	const unsigned char *input_data = m_data_ro(unsigned char,input);
	unsigned char *digest_data = m_data(unsigned char,digest);

	ASSERT(input_data != NULL);
	ASSERT(digest_data != NULL);

	/* Updating a context already at the byte-aligned maximum must refuse the
	   next byte before the bit counter can wrap */
	ASSERT(CRYPT_OK == sha512_init(&context));
	context.length = SHA512_TEST_MAX_BYTE_ALIGNED_BITS;
	ASSERT(CRYPT_HASH_OVERFLOW == sha512_update(&context,input_data,input->length));

	/* Finalization also adds buffered bytes to the counter, so it must detect the
	   same overflow risk before writing a digest */
	ASSERT(CRYPT_OK == sha512_init(&context));
	context.length = SHA512_TEST_MAX_BYTE_ALIGNED_BITS;
	context.curlen = 1U;
	ASSERT(CRYPT_HASH_OVERFLOW == sha512_final(&context,digest_data));

	/* Cleanup is intentionally unconditional from the test's point of view */
	call(m_del(input));
	call(m_del(digest));

	RETURN_STATUS;
}

/**
 * @brief Check that libsha512 fails safely on invalid API use
 * @details These cases cover practical misuse and corrupted state: missing
 * pointers, impossible buffered-byte counts, and messages too long for the
 * library's 64-bit length counter
 *
 * @return SUCCESS when every invalid case is reported as an error
 */
Return test_libsha512_0003(void)
{
	INITTEST;

	/* Keep invalid-use checks split by user story: missing storage, damaged
	   context state, and input that is too large to represent safely */
	TEST(test_libsha512_0003_1,"Missing SHA-512 pointers return CRYPT_INVALID_ARG");
	TEST(test_libsha512_0003_2,"A damaged SHA-512 buffer length is rejected before hashing continues");
	TEST(test_libsha512_0003_3,"SHA-512 length overflow returns CRYPT_HASH_OVERFLOW before wraparound");

	RETURN_STATUS;
}
