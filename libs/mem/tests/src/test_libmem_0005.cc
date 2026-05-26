for(int i = 0; i < CYCLES; i++)
{
	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];
	unsigned char source_bytes[1000U * sizeof(TYPE)];
	uint64_t random = 0;

	m_create(TYPE,test);

	for(int k = 0; k < CYCLES; k++)
	{
		/* Choose a random descriptor length within the fixed test range */
		ASSERT(SUCCESS == random_number_generator_urandom(&random,1,1000));

		size_t array_length = (size_t)random;
		size_t array_size = array_length * sizeof(TYPE);

		/* Fill a bounded stack buffer with random bytes. This keeps the test
		   focused on libmem descriptors instead of raw heap allocation */
		for(size_t j = 0; j < array_size; j++)
		{
			ASSERT(SUCCESS == random_number_generator_urandom(&random,0,255));
			source_bytes[j] = (unsigned char)random;
		}

		/* Hash the original bytes before they enter the libmem descriptor */
		SHA512_Context source_context;
		ASSERT(CRYPT_OK == sha512_init(&source_context));
		ASSERT(CRYPT_OK == sha512_update(&source_context,source_bytes,array_size));
		ASSERT(CRYPT_OK == sha512_final(&source_context,hash_1));

		#if SHOW_TEST
		/* Print the source buffer summary and digest when verbose diagnostics
		   are enabled for this stress-style test */
		echo(STDERR,"Test %d:%d array size: %zu bytes, array_length=%zu, sizeof(TYPE)=%zu bytes\n",i,k,array_size,array_length,sizeof(TYPE));
		echo(STDERR,"Test %d:%d SHA-512 hash: ",i,k);
		print_hash(hash_1);
		#endif

		/* Import the bounded byte range into libmem-managed typed storage */
		ASSERT(SUCCESS == m_copy_buffer(test,array_size,source_bytes));
		ASSERT(test->length == array_length);

		/* Hash the descriptor view and compare it with the original bytes. The
		   two digests must match exactly after the bounded-buffer import */
		SHA512_Context descriptor_context;
		ASSERT(CRYPT_OK == sha512_init(&descriptor_context));

		const TYPE *test_view = m_data_ro(TYPE,test);
		ASSERT(test_view != NULL);

		IF(test_view != NULL)
		{
			ASSERT(CRYPT_OK == sha512_update(
				&descriptor_context,
				(const unsigned char *)test_view,
				test->length * sizeof(TYPE)));
		}

		ASSERT(CRYPT_OK == sha512_final(&descriptor_context,hash_2));

		#if SHOW_TEST
		/* Print the descriptor summary and digest when verbose diagnostics are
		   enabled for this stress-style test */
		echo(STDERR,"Test %d:%d array size: %zu bytes\n",i,k,test->length * sizeof(TYPE));
		echo(STDERR,"Test %d:%d SHA-512 hash: ",i,k);
		print_hash(hash_2);
		#endif

		ASSERT(0 == memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH));
	}

	/* Release the descriptor even when an earlier iteration failed */
	call(m_del(test));
}
