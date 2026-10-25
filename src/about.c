#include "precizer.h"

/**
 * @brief Print application name and version unless silent, then flush stdout
 *
 */
void about(void)
{
	slog(EVERY,APP_NAME " " APP_VERSION "\n");
	fflush(stdout);
}
