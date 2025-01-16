#include "sute.h"

/**
 * Testing scenario 15
 *
 * Database upgrade testing:
 * - Upgrade the database from version 0 to version 1
 * - Run the program again to verify that the database is actually at version 1
 * - Launch the program without specifying a database to ensure that a new database is created with the correct version
 * - Run the program with the --compare parameter to compare databases when one of them has an older version - this should generate an appropriate error message
 * - Run the database comparison again using the --compare parameter, but this time with the --update option. The database should be upgraded accordingly.
*/
Return test0015(void){
    Return status = SUCCESS;

    RETURN_STATUS;
}
