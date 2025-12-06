#include "precizer.h"
#include <stdarg.h>
#include <stdio.h>

void log_sqlite_error(
	sqlite3    *db,
	int        rc,
	char       *err_msg,
	const char *fmt,
	...)
{
	char context[MAX_CHARACTERS];

	if(fmt != NULL)
	{
		va_list ap;
		va_start(ap,fmt);
		vsnprintf(context,sizeof(context),fmt,ap);
		va_end(ap);
		context[sizeof(context) - 1] = '\0';

	} else {
		snprintf(context,sizeof(context),"SQLite error");
	}

	const char *sqlite_msg = NULL;

	if(db != NULL)
	{
		sqlite_msg = sqlite3_errmsg(db);
	}

	if(sqlite_msg == NULL)
	{
		sqlite_msg = sqlite3_errstr(rc);
	}

	if(err_msg != NULL && err_msg[0] != '\0')
	{
		slog(ERROR,"%s (%d): %s; detail: %s\n",
			context,
			rc,
			sqlite_msg ? sqlite_msg : "unknown",
			err_msg);
		sqlite3_free(err_msg);

	} else {
		slog(ERROR,"%s (%d): %s\n",
			context,
			rc,
			sqlite_msg ? sqlite_msg : "unknown");
	}
}
