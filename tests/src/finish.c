#include "sute.h"

Return finish(void){

	printf(CYAN "\nTelemetry\n");
	printf(WHITE);
	telemetry_show();
	printf(RESET);

	return(SUCCESS);
}
