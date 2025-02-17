#include "rational.h"

/// Converts Return enum to a string
const char *return_status(const Return status)
{
    switch (status) {
        case SUCCESS:   return "SUCCESS";
        case FAILURE:   return "FAILURE";
        case WARNING:   return "WARNING";
        case DONOTHING: return "DONOTHING";
        default:        return "UNKNOWN";
    }
}
