/**
 * @file realloc.d
 * @brief Template for heap memory resizing
 * @details Handles memory reallocation with optimization for reducing actual
 * reallocations and maintains telemetry data
 */
{
	if (!structure) {
		return(FAILURE); // Add null pointer check
	}

	// Skip if no change in length
	if(length == structure->length)
	{
		telemetry_how_many_times_realloc_do_nothing();
		// Do nothing
		return(SUCCESS);
	}

	const size_t oldlength = structure->length;

	const size_t newbytes = length * sizeof(TYPE);

	const size_t oldbytes = oldlength * sizeof(TYPE);

	size_t new_aligned_bytes = get_aligned_bytes(newbytes);

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

	#if 0
	printf("new_aligned_bytes=%ld,old_aligned_bytes=%ld\n",new_aligned_bytes,old_aligned_bytes);
	#endif

	// Check if memory should be decreased
	if(new_aligned_bytes < old_aligned_bytes)
	{
		if(true_reduce == true)
		{
			decrease_memory = true;
		}
		#if 0
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

			fprintf(stderr, "ERROR: Memory reallocation failed for %s bytes: %s (errno: %d)\n",
				bkbmbgbtbpbeb(new_aligned_bytes), strerror(errno), errno);

			return(FAILURE);
		}
		structure->mem = temp;

		// Update telemetry
		telemetry_heap_true_reallocations_counter();
		if(decrease_memory == true)
		{
			telemetry_heap_reduce(old_aligned_bytes - new_aligned_bytes);

		} else if(growth_memory == true)
		{
			telemetry_heap_add(new_aligned_bytes - old_aligned_bytes);

		}

		if(oldlength == 0)
		{
			telemetry_heap_allocations_counter();
		}
	} else {
		// Update telemetry
		telemetry_heap_trick_reallocations_counter();
	}

	// Initialize new memory for calloc operations
	#ifdef CALLOC
	// Filling a new array with zeros
	if(oldlength == 0 && length > 0)
	{
		memset(structure->mem, CALLOC, length * sizeof(TYPE));
	}
	#endif

	// Update telemetry for effective memory usage
	if(structure->length > length)
	{
		telemetry_effective_reduce(oldbytes - newbytes);

	} else if (length > structure->length)
	{
		telemetry_effective_add(newbytes - oldbytes);
	}

	// Update structure metadata
	structure->length = length;
	structure->allocated = new_aligned_bytes;

	return(SUCCESS);
}
