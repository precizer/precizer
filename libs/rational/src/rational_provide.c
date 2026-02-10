#include "rational.h"
#include <sysexits.h>

/// Converts Return enum to a string
const char *show_status(const Return status)
{
	switch(status)
	{
		case COMPLETED: return "COMPLETED";
		case SUCCESS:   return "SUCCESS";
		case FAILURE:   return "FAILURE";
		case WARNING:   return "WARNING";
		case DONOTHING: return "DONOTHING";
		case HALTED:    return "HALTED";
		default:        return "UNKNOWN";
	}
}
