/**
 * @file del.d
 * @brief Template for array deallocation based on type
 * @details Handles memory freeing and telemetry updates for dynamic arrays
 */
{
	if (!structure || !(*structure) || !(*structure)->mem) {
		return(FAILURE); // Add null pointer check
	}

	// Free array memory inside structure
	free((*structure)->mem);

	// Set pointer to NULL as recommended by best practices
	(*structure)->mem = NULL;

	// Update telemetry
	telemetry_heap_reduce((*structure)->allocated);
	telemetry_effective_reduce(sizeof(TYPE) * (*structure)->length);
	telemetry_heap_free_bytes((*structure)->allocated);
	telemetry_heap_free_counter();

	// Reset array metadata
	(*structure)->length = 0;
	(*structure)->allocated = 0;

	// Clear owning structure in stack
	memset((*structure), 0, sizeof(MEM_TYPE));

	return(SUCCESS);
}
