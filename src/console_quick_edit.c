#include "precizer.h"

#ifdef __MSYS__
// Exclude the GDI ERROR macro, which conflicts with the logger level
#define NOGDI
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// A saved handle means QuickEdit was enabled and was successfully disabled
static HANDLE quick_edit_console = NULL;

/**
 * @brief Disable console QuickEdit while files are being processed
 *
 * Only an enabled QuickEdit setting is changed. The console input handle is
 * saved for explicit restoration after processing
 */
void disable_console_quick_edit(void)
{
	if(quick_edit_console == NULL)
	{
		HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
		DWORD console_mode;

		if(GetConsoleMode(console,&console_mode) != 0
		        && (console_mode & ENABLE_QUICK_EDIT_MODE) != 0)
		{
			if(SetConsoleMode(console,
				(console_mode | ENABLE_EXTENDED_FLAGS) & ~(DWORD)ENABLE_QUICK_EDIT_MODE) != 0)
			{
				quick_edit_console = console;
			} else {
				slog(ERROR,"Could not disable console QuickEdit mode (Windows error %lu)\n",
					(unsigned long)GetLastError());
			}
		}
	}
}

/**
 * @brief Restore QuickEdit if it was disabled for the current run
 *
 * Read the current console mode so changes made while restoring terminal
 * input are preserved. The console handle belongs to stdin and is not closed
 */
void restore_console_quick_edit(void)
{
	/* Restore only the QuickEdit setting changed during initialization */

	if(quick_edit_console != NULL)
	{
		DWORD console_mode;

		if(GetConsoleMode(quick_edit_console,&console_mode) == 0
		        || SetConsoleMode(quick_edit_console,
			console_mode | ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS) == 0)
		{
			slog(ERROR,"Could not restore console QuickEdit mode (Windows error %lu)\n",(unsigned long)GetLastError());
		}

		quick_edit_console = NULL;
	}
}
#endif
