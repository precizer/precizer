#include "precizer.h"
#include <sys/ioctl.h>

/**
 * @brief Gets the terminal width (number of columns)
 * @return Number of columns in the terminal or default value 80 columns
 */
size_t terminal_width(void){

	struct winsize w;

	if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&w) == -1)
	{
		/* Unable to retrieve terminal window width.
		   Defaulting to 80 columns. */
		return(80UL);

	} else {
		return((size_t)w.ws_col);
	}
}
