/**
 * @file realloc.d
 * @brief Template for heap memory resizing
 * @details Handles memory reallocation with optimization for reducing actual
 * reallocations and maintains telemetry data
 */
{
	// Skip if no change in length
	if(newlength == structure->length)
	{
		telemetry_realloc_noop_counter();
		// Do nothing
		return(SUCCESS);
	}

	const size_t old_length = structure->length;

	const size_t new_bytes = newlength * sizeof(TYPE);

	const size_t old_used_bytes = old_length * sizeof(TYPE);

	size_t new_aligned_bytes = get_aligned_bytes(new_bytes);

	size_t old_aligned_bytes = structure->allocated;

	/* Memory growth control flags */

	// Do not do real growth of allocated memory size by default
	bool growth_memory = false;

	// Do not do real decrease of allocated memory size by default
	bool decrease_memory = false;

	// Check if actual memory growth is needed
	if(new_aligned_bytes > old_aligned_bytes)
	{
		growth_memory = true;
	}

	#if SHOW
	printf("new_aligned_bytes=%ld,old_aligned_bytes=%ld\n",new_aligned_bytes,old_aligned_bytes);
	#endif

	// Check if memory should be decreased
	if(new_aligned_bytes < old_aligned_bytes)
	{
		if(true_reduce == true)
		{
			decrease_memory = true;
		}
		#if SHOW
		else {
			printf("do not reduce memory\n");
		}
		#endif
	}

	// Perform actual reallocation if needed
	if(growth_memory == true || decrease_memory == true)
	{
		TYPE *temp = (TYPE*)realloc(structure->mem, new_aligned_bytes);
		if(temp == NULL){
			free(structure->mem);
			report("Memory allocation failed, requested size: %zu bytes", new_aligned_bytes);
			return(FAILURE);
		}
		structure->mem = temp;
		#if SHOW
		printf("aligned realloc() to bytes=%ld\n",new_aligned_bytes);
		#endif

		// Update structure metadata
		structure->allocated = new_aligned_bytes;

		// Update telemetry
		if(old_length == 0)
		{
			telemetry_allocations_counter();
		} else {
			telemetry_aligned_reallocations_counter();
		}

		if(decrease_memory == true)
		{
			telemetry_reduce(old_aligned_bytes - new_aligned_bytes);

		} else if(growth_memory == true)
		{
			telemetry_add(new_aligned_bytes - old_aligned_bytes);
		}

	} else {
		// Update telemetry
		telemetry_realloc_optimized_counter();
	}

	// Initialize new memory for calloc operations
	#ifdef CALLOC
	// Filling a new array with zeros
	if(old_length == 0 && newlength > 0)
	{
		memset(structure->mem, CALLOC, structure->allocated);
	}
	#endif

	// Update telemetry for effective memory usage
	if(structure->length > newlength)
	{
		telemetry_effective_reduce(old_used_bytes - new_bytes);

	} else if (newlength > structure->length)
	{
		telemetry_effective_add(new_bytes - old_used_bytes);
	}

	// Update structure metadata
	structure->length = newlength;

	#if SHOW
	printf("new length=%ld,new aligned bytes=%ld\n",newlength,new_aligned_bytes);
	#endif

	return(SUCCESS);
}
