for(int i = 0; (SKIP & status) == 0 && i < CYCLES; i++)
{
	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];
	uint64_t random = 0;

	m_create(TYPE,test);

	for(int k = 0; (SKIP & status) == 0 && k < CYCLES; k++)
	{
		// Generates a random number within a specified range (1-1000)
		ASSERT(SUCCESS == random_number_generator_urandom(&random,1,1000));

		unsigned char *array = NULL;

		if(SUCCESS == status)
		{
			size_t array_length = (size_t)random;
			size_t array_size = array_length * sizeof(TYPE);
			unsigned char *temp = (unsigned char *)realloc(array,array_size);

			ASSERT(temp != NULL);

			if(SUCCESS == status)
			{
				array = temp;

				// Fill array with random bytes
				for(size_t j = 0; (SKIP & status) == 0 && j < array_size; j++)
				{
					ASSERT(SUCCESS == random_number_generator_urandom(&random,0,255));
					array[j] = (unsigned char)random;
				}

				if(SUCCESS == status)
				{
					// Calculate SHA-512 hash
					SHA512_Context ctx;
					sha512_init(&ctx);
					sha512_update(&ctx,array,array_size);
					sha512_final(&ctx,hash_1);

					#if SHOW_TEST
					// Print array summary and hash
					echo(STDERR,"Test %d:%d array size: %zu bytes, array_length=%zu, sizeof(TYPE)=%zu bytes\n",i,k,array_size,array_length,sizeof(TYPE));
					echo(STDERR,"Test %d:%d SHA-512 hash: ",i,k);
					print_hash(hash_1);
					#endif

					// Create a TYPE memory with randomly real reallocation or not
					ASSERT(SUCCESS == random_number_generator_urandom(&random,0,1));
					bool true_reduce = (bool)random;
					(void)true_reduce; /* resizing always keeps spare capacity */
					ASSERT(SUCCESS == m_resize(test,array_length));
				}

				if(SUCCESS == status)
				{
					TYPE *test_data = m_data(TYPE,test);
					ASSERT(test_data != NULL);

					if(test_data != NULL)
					{
						memcpy(test_data,array,test->length * sizeof(TYPE));
					}
				}

				if(SUCCESS == status)
				{
					// Calculate SHA-512 hash
					SHA512_Context ctx;
					sha512_init(&ctx);
					const TYPE *test_view = m_data_ro(TYPE,test);
					ASSERT(test_view != NULL);

					if(test_view != NULL)
					{
						sha512_update(&ctx,(const unsigned char *)test_view,test->length * sizeof(TYPE));
					}
					sha512_final(&ctx,hash_2);

					#if SHOW_TEST
					// Print array summary and hash
					echo(STDERR,"Test %d:%d array size: %zu bytes\n",i,k,test->length * sizeof(TYPE));
					echo(STDERR,"Test %d:%d SHA-512 hash: ",i,k);
					print_hash(hash_2);
					#endif

					ASSERT(0 == memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH));
				}
			}
		}

		m_reset(&array);
	}

	// free an empty TYPE array
	call(m_del(test));
}
