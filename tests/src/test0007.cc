for(int i = 0; i < CYCLES;i++)
{
	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];

	// Generates a random number within a specified range (0-1000)
	size_t array_length = (size_t)(1L + (rand() % (1000L + 1L)));

	size_t array_size = array_length * sizeof(TYPE);
	unsigned char *ARRAY_TYPE = (unsigned char *)realloc(ARRAY_TYPE,array_size);

	// Seed random number generator
	srand((unsigned int)time(NULL));

	// Fill array with random bytes
	for(size_t j = 0; j < array_size; j++)
	{
		ARRAY_TYPE[j] = (unsigned char)(rand() % 256);
	}

	// Calculate SHA-512 hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,ARRAY_TYPE,array_size);
	sha512_final(&ctx,hash_1);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test array size: %zu bytes, array_length=%zu, sizeof(TYPE)=%zu bytes\n",array_size,array_length,sizeof(TYPE));
	echo(STDERR,"Test SHA-512 hash: ");
	print_hash(hash_1);
	#endif

	// Allocate memory for the structure TYPE
	MSTRUCT(MEM_TYPE,test);

	// Create an TYPE memory with randomly real reallocation or not
	bool true_reduce = (bool)(rand() % 2);
	ASSERT(SUCCESS == REALLOC_TYPE(test,array_length,true_reduce));

	// Test memeory edges
	memcpy(test->mem,ARRAY_TYPE,test->length * sizeof(test->mem[0]));

	// Calculate SHA-512 hash
	sha512_init(&ctx);
	sha512_update(&ctx,(const unsigned char *)test->mem,test->length * sizeof(test->mem[0]));
	sha512_final(&ctx,hash_2);

	#if SHOW_TEST
	// Print array summary and hash
	echo(STDERR,"Test array size: %zu bytes\n",test->length * sizeof(test->mem[0]));
	echo(STDERR,"Test SHA-512 hash: ");
	print_hash(hash_2);
	#endif

	if(0 != memcmp(hash_1,hash_2,(size_t)SHA512_DIGEST_LENGTH))
	{
		echo(STDERR,"Test fail\n");
		status = FAILURE;
	}

	// free an empty TYPE array
	DEL_TYPE(&test);
	reset(&ARRAY_TYPE);
}

