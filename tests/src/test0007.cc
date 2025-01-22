for(int i = 0; i < CYCLES;i++)
{
	unsigned char hash_1[SHA512_DIGEST_LENGTH];
	unsigned char hash_2[SHA512_DIGEST_LENGTH];

	// Seed random number generator
	srand((unsigned int)time(NULL));

	// Generates a random number within a specified range (0-1000)
	// result = min + (rand() % (max - min + 1));
	size_t array_length = (size_t)(1L + (rand() % 1000L));

	size_t array_size = array_length * sizeof(TYPE);
	unsigned char *array = NULL;
	unsigned char *temp = (unsigned char *)realloc(array,array_size);
	if (temp == NULL)
	{
		echo(STDERR,"Can't allocate memory %zu bytes\n",array_size);
	}
	array = temp;

	// Fill array with random bytes
	for(size_t j = 0; j < array_size; j++)
	{
		array[j] = (unsigned char)(rand() % 256);
	}

	// Calculate SHA-512 hash
	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,array,array_size);
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
	memcpy(test->mem,array,test->length * sizeof(test->mem[0]));

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
	reset(&array);
}