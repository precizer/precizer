#include "test_libsha512_utils.h"

#include "monocypher-ed25519.h"

/**
 * @brief Hash one complete message with libsha512
 * @details This helper gives tests a small, readable way to ask libsha512 for
 * the digest of a message. The digest is stored in a libmem descriptor so the
 * tests exercise the same memory-management style used by the other libraries
 *
 * @param message Message bytes that should be hashed
 * @param message_size Number of bytes from @p message to include in the hash
 * @param digest Output descriptor that receives the 64-byte SHA-512 digest
 * @return SUCCESS when the digest was written, otherwise FAILURE
 */
Return calculate_sha512_digest(
	const unsigned char *message,
	size_t              message_size,
	memory              *digest)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	SHA512_Context context = {0};
	unsigned char *digest_data = NULL;

	/* Validate the public helper inputs before allocating the digest buffer */
	ASSERT(message != NULL);
	ASSERT(digest != NULL);
	ASSERT(SUCCESS == m_resize(digest,SHA512_DIGEST_LENGTH,ZERO_NEW_MEMORY));

	/* Convert the digest descriptor to writable bytes after it has the required
	   SHA-512 digest length */
	IF(digest != NULL)
	{
		digest_data = m_data(unsigned char,digest);
	}

	/* Drive the normal one-shot SHA-512 flow: initialize, feed bytes, finalize */
	ASSERT(digest_data != NULL);
	ASSERT(CRYPT_OK == sha512_init(&context));
	ASSERT(CRYPT_OK == sha512_update(&context,message,message_size));
	ASSERT(CRYPT_OK == sha512_final(&context,digest_data));

	provide(status);
}

/**
 * @brief Hash one complete message with Monocypher SHA512
 * @details The tests use this helper as an independent reference implementation
 * for libsha512. It keeps Monocypher setup and length validation in one place
 * while the individual tests stay focused on the input shape they exercise
 *
 * @param message Message bytes that should be hashed
 * @param message_size Number of bytes from @p message to include in the hash
 * @param digest Output descriptor that receives the 64-byte SHA-512 digest
 * @return SUCCESS when Monocypher produced one SHA-512 digest, otherwise FAILURE
 */
Return calculate_sha512_digest_monocypher(
	const unsigned char *message,
	size_t              message_size,
	memory              *digest)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	unsigned char *digest_data = NULL;

	ASSERT(message != NULL);
	ASSERT(digest != NULL);
	ASSERT(SUCCESS == m_resize(digest,SHA512_DIGEST_LENGTH,ZERO_NEW_MEMORY));

	IF(digest != NULL)
	{
		digest_data = m_data(unsigned char,digest);
	}

	ASSERT(digest_data != NULL);

	if(SUCCESS == status)
	{
		crypto_sha512(digest_data,message,message_size);
	}

	provide(status);
}

/**
 * @brief Hash one message by sending it to libsha512 in smaller pieces
 * @details SHA-512 callers may provide input gradually instead of all at once.
 * This helper checks that pattern by applying each requested chunk size to a
 * single hash context and writing the final digest into a libmem descriptor
 *
 * @param message Full message bytes that should be hashed
 * @param message_size Number of bytes available in @p message
 * @param chunk_sizes Descriptor containing the piece sizes to feed in order
 * @param digest Output descriptor that receives the 64-byte SHA-512 digest
 * @return SUCCESS when every chunk was accepted and the digest was written
 */
Return calculate_sha512_digest_in_chunks(
	const unsigned char *message,
	size_t              message_size,
	const memory        *chunk_sizes,
	memory              *digest)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	SHA512_Context context = {0};
	const size_t *chunks = NULL;
	unsigned char *digest_data = NULL;
	size_t offset = 0U;

	/* Validate the message, chunk plan, and output descriptor before hashing */
	ASSERT(message != NULL);
	ASSERT(chunk_sizes != NULL);
	ASSERT(digest != NULL);
	ASSERT(chunk_sizes->length > 0U);
	ASSERT(SUCCESS == m_resize(digest,SHA512_DIGEST_LENGTH,ZERO_NEW_MEMORY));

	/* Read chunk sizes from libmem and prepare writable storage for the digest */
	IF(chunk_sizes != NULL)
	{
		chunks = m_data_ro(size_t,chunk_sizes);
	}

	IF(digest != NULL)
	{
		digest_data = m_data(unsigned char,digest);
	}

	ASSERT(chunks != NULL);
	ASSERT(digest_data != NULL);
	ASSERT(CRYPT_OK == sha512_init(&context));

	/* Feed each requested chunk in order. The bounds checks keep the chunk plan
	   honest, so the final digest represents exactly the message under test */
	for(size_t chunk_index = 0U; SUCCESS == status && chunk_index < chunk_sizes->length; chunk_index++)
	{
		ASSERT(offset <= message_size);
		ASSERT(chunks[chunk_index] <= message_size - offset);

		if(SUCCESS == status)
		{
			ASSERT(CRYPT_OK == sha512_update(&context,message + offset,chunks[chunk_index]));
			offset += chunks[chunk_index];
		}
	}

	/* Confirm that the chunk plan consumed the full message, then write the
	   digest from the completed SHA-512 context */
	ASSERT(offset == message_size);
	ASSERT(CRYPT_OK == sha512_final(&context,digest_data));

	provide(status);
}

/**
 * @brief Compare two SHA-512 digest descriptors
 * @details The check validates descriptor shape before comparing bytes, so a
 * failing test points either to malformed test data or to a digest mismatch
 *
 * @param digest Digest produced by the code under test
 * @param expected_digest Digest that the test expects
 * @return SUCCESS when both descriptors contain the same 64 digest bytes
 */
Return assert_sha512_digest_matches(
	const memory *digest,
	const memory *expected_digest)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	const unsigned char *digest_data = NULL;
	const unsigned char *expected_data = NULL;
	size_t digest_size = 0U;
	size_t expected_size = 0U;

	/* First verify descriptor shape. A length failure is easier to understand
	   than a raw byte comparison against malformed data */
	ASSERT(digest != NULL);
	ASSERT(expected_digest != NULL);
	ASSERT(digest->length == SHA512_DIGEST_LENGTH);
	ASSERT(expected_digest->length == SHA512_DIGEST_LENGTH);
	ASSERT(SUCCESS == m_guarded_byte_size(digest,digest->length,&digest_size));
	ASSERT(SUCCESS == m_guarded_byte_size(expected_digest,expected_digest->length,&expected_size));
	ASSERT(digest_size == SHA512_DIGEST_LENGTH);
	ASSERT(expected_size == SHA512_DIGEST_LENGTH);

	/* Pull out read-only byte pointers only after descriptor validation passes */
	IF(digest != NULL)
	{
		digest_data = m_data_ro(unsigned char,digest);
	}

	IF(expected_digest != NULL)
	{
		expected_data = m_data_ro(unsigned char,expected_digest);
	}

	ASSERT(digest_data != NULL);
	ASSERT(expected_data != NULL);

	/* Finally compare the digest bytes themselves */
	IF(digest_data != NULL && expected_data != NULL)
	{
		ASSERT(0 == memcmp(digest_data,expected_data,(size_t)SHA512_DIGEST_LENGTH));
	}

	provide(status);
}

/**
 * @brief Run one human-readable SHA-512 reference case
 * @details The vector describes a message and the digest that public SHA-512
 * references say it must produce. This helper prepares the message through
 * libmem, hashes it, and compares the result with the expected bytes
 *
 * @param vector Message text and expected digest bytes for one reference case
 * @return SUCCESS when libsha512 returns the expected digest
 */
Return check_sha512_vector(const sha512_digest_vector *vector)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	size_t message_length = 0U;

	m_create(char,message,MEMORY_STRING);
	m_create(unsigned char,digest);
	m_create(unsigned char,expected_digest);
	m_create(unsigned char,monocypher_digest);

	/* Validate the vector and copy its text into libmem-managed storage. The
	   string length check catches accidental vector size drift */
	ASSERT(vector != NULL);
	ASSERT(vector->message_text != NULL);
	ASSERT(vector->expected_digest != NULL);
	ASSERT(SUCCESS == m_copy_fixed_string(message,vector->message_size + 1U,vector->message_text));
	ASSERT(SUCCESS == m_string_length(message,&message_length));
	ASSERT(message_length == vector->message_size);

	/* Hash the prepared message and copy the expected digest into the same
	   descriptor style before comparing both byte arrays */
	ASSERT(SUCCESS == calculate_sha512_digest(
		(const unsigned char *)m_text(message),
		message_length,
		digest));
	ASSERT(SUCCESS == calculate_sha512_digest_monocypher(
		(const unsigned char *)m_text(message),
		message_length,
		monocypher_digest));
	ASSERT(SUCCESS == m_copy_buffer(
		expected_digest,
		SHA512_DIGEST_LENGTH,
		vector->expected_digest));
	ASSERT(SUCCESS == assert_sha512_digest_matches(expected_digest,monocypher_digest));
	ASSERT(SUCCESS == assert_sha512_digest_matches(digest,monocypher_digest));

	/* Clean up every descriptor even when a vector assertion fails */
	call(m_del(message));
	call(m_del(digest));
	call(m_del(expected_digest));
	call(m_del(monocypher_digest));

	provide(status);
}
