#include "testitall.h"

/**
 * @brief Sets an environment variable with error handling
 *
 * @param variable Name of the environment variable to set
 * @param value Value to assign to the environment variable
 *
 * @return Return Status code indicating operation result:
 *         - SUCCESS: Variable was set successfully
 *         - FAILURE: Failed to set variable (null parameters or setenv error)
 *
 * @details This function safely sets an environment variable with proper error checking
 *          and reporting. It validates input parameters and uses setenv() with
 *          overwrite enabled.
 */
Return set_environment_variable(
	const char *variable,
	const char *value
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Validate input parameters */
	if(!variable || !value)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Attempt to set environment variable
		 * Third parameter (1) allows overwriting existing values */
		if(setenv(variable,value,1) != 0)
		{
			/* Log error if setenv fails */
			echo(STDERR,"Failed to set environment variable %s",variable);
			status = FAILURE;
		}
	}

	/* Output error message on failure */
	if(SUCCESS != status)
	{
		echo(STDERR,"ERROR: Failed to set environment variable\n");
	}

	return(status);
}
