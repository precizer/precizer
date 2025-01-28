#include "testitall.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @brief Generates a random number within the specified [start..end] range (inclusive).
 *
 * Reads 8 bytes from /dev/urandom to obtain a 64-bit unsigned random value.
 * The value is then mapped to the given range and stored in the variable pointed to by random_number.
 *
 * @param random_number Pointer to an unsigned 64-bit integer where the result will be stored
 * @param start Beginning of the range (inclusive)
 * @param end End of the range (inclusive)
 */
Return random_number_generator(
	uint64_t *random_number,
	uint64_t start,
	uint64_t end
){
	Return status = SUCCESS;

	FILE*fp=fopen("/dev/urandom","rb");
	if(!fp)
	{
		echo(STDERR,"Can't open /dev/urandom\n");
		status = FAILURE;
	}

	uint64_t random_value;

	if(SUCCESS == status)
	{
		if(fread(&random_value,sizeof(random_value),1,fp)!=1)
		{
			echo(STDERR,"Failed to read from /dev/urandom\n");
			status = FAILURE;
		}
	}

	fclose(fp);

	if(SUCCESS == status)
	{
		if(end<start)
		{
			echo(STDERR,"Invalid range: end < start\n");
			status = FAILURE;
		}

		uint64_t range=(end-start)+1;
		*random_number=(random_value%range)+start;
	}

	#if 0
	printf("random:%zu\n",*random_number);
	#endif

	return(status);
}

