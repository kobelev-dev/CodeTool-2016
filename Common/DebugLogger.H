//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Basic debug logger. The log file is opened/closed for each logged message.
//

#ifndef	Common_DebugLogger_H
#define	Common_DebugLogger_H

void		LogOpenLogging(const wchar_t *shortFileName, bool addDateTime);
			// Logger writes data to the file in ASCII. The log file is always placed into the directory of EXE file.
			// The call to this function is not openning any file. It only prepares the name of the file.

void		LogCloseLogging();
			// Future calls to any of the LogXyzMessage() functions will not take any action.

const wchar_t		*LogGetLogFileName();
					// Function returns the address of the full file name or NULL if the logging is not prepared.

void		LogAsciiMessage(const char *message);
void		LogUnicodeMessage(const wchar_t *message);
void		LogMessageFmt(const wchar_t *format, ...);
			// Passed messages should be plain strings without CRLF at the end.

#endif	// Common_DebugLogger_H


