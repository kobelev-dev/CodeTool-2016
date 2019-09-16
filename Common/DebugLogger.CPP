//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Basic debug logger.
//

#define    STRICT
#include  <stdio.h>
#include  <assert.h>
#include  <windows.h>

#include  "Common/DateTimeHelper.H"
#include  "Common/DebugLogger.H"

//---------------------------------------------------------------------
//   ====================  Functions  =======================
//---------------------------------------------------------------------

#define CONVERT_TO_ASCII_IN_PLACE(buff) { wchar_t *src = (buff); char *dest = (char*)(buff); while (*src != 0) *dest++ = (char)*src++; *dest = 0; }

wchar_t		g_DebugLogger_LogFileName[2*MAX_PATH];
bool			g_DebugLogger_AddDateTime;

void LogOpenLogging(const wchar_t *shortFileName, bool addDateTime)
{
	assert(shortFileName != NULL && shortFileName[0] != 0);

	// Prepare the name of the file.
	::GetModuleFileNameW(NULL, g_DebugLogger_LogFileName, sizeof(g_DebugLogger_LogFileName)/sizeof(wchar_t));
	wchar_t *dirEnd = wcsrchr(g_DebugLogger_LogFileName, L'\\');
	if (dirEnd == NULL)
	{
		// Weird situation. Do at least something.
		assert(FALSE);
		dirEnd = g_DebugLogger_LogFileName;
	}
	else
	{
		// Shift pointer to the first char of the exe file name.
		dirEnd++;
	}

	wcscpy(dirEnd, shortFileName);
	g_DebugLogger_AddDateTime = addDateTime;
	::DeleteFileW(g_DebugLogger_LogFileName);
}

void LogCloseLogging()
{
	g_DebugLogger_LogFileName[0] = 0;
}

const wchar_t *LogGetLogFileName()
{
	if (g_DebugLogger_LogFileName[0] == 0)
		return(NULL);

	// The logging is preprared.
	return(g_DebugLogger_LogFileName);
}

static void LogMessageInternal(FILE *debugLog, char *message)
{
	if (g_DebugLogger_AddDateTime == TRUE)
	{
		TDateTime datetime = CurrDateTime();
		wchar_t dateTimeBuffer[80];
		FormatDateTime(datetime, dateTimeBuffer, 80, FALSE);
		CONVERT_TO_ASCII_IN_PLACE(dateTimeBuffer);

		fprintf(debugLog, "%s %s\r\n", (char*)dateTimeBuffer, message);
	}
	else
	{
		fprintf(debugLog, "%s\r\n", message);
	}
}

void LogAsciiMessage(const char *message)
{
	FILE *debugLog = (g_DebugLogger_LogFileName[0] != 0) ? _wfopen(g_DebugLogger_LogFileName, L"ab") : NULL;
	if (debugLog == NULL)
		return;

	// Local buffer is needed because input string is splitted into lines by replacing EOL with NULL char.
	char msgBuffer[4096];
	strncpy(msgBuffer, message, sizeof(msgBuffer));
	msgBuffer[sizeof(msgBuffer)-1] = 0;

	char *pntBeg = (char*)msgBuffer;
	while (*pntBeg != 0)
	{
		// Look for the end of the line.
		char *pntEnd = pntBeg;
		while (*pntEnd != 0)
		{
			if (*pntEnd == '\r' || *pntEnd == '\n')
				break;
			pntEnd++;
		}

		char *newPntBeg = pntEnd;
		if (*newPntBeg != 0)
			newPntBeg++;

		if (pntEnd != pntBeg)
		{
			*pntEnd = 0;
			LogMessageInternal(debugLog, pntBeg);
		}

		pntBeg = newPntBeg;
	}

	fclose(debugLog);
}

void LogUnicodeMessage(const wchar_t *message)
{
	FILE *debugLog = (g_DebugLogger_LogFileName[0] != 0) ? _wfopen(g_DebugLogger_LogFileName, L"ab") : NULL;
	if (debugLog == NULL)
		return;

	wchar_t msgBuffer[2048];
	wcsncpy(msgBuffer, message, 2048);
	msgBuffer[2047] = 0;
	CONVERT_TO_ASCII_IN_PLACE(msgBuffer);

	LogMessageInternal(debugLog, (char*)msgBuffer);
	fclose(debugLog);
}

void LogMessageFmt(const wchar_t *format, ...)
{
	FILE *debugLog = (g_DebugLogger_LogFileName[0] != 0) ? _wfopen(g_DebugLogger_LogFileName, L"ab") : NULL;
	if (debugLog == NULL)
		return;

	wchar_t msgBuffer[2048];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(msgBuffer, 2048, format, vargs);
	va_end(vargs);

	CONVERT_TO_ASCII_IN_PLACE(msgBuffer);

	LogMessageInternal(debugLog, (char*)msgBuffer);
	fclose(debugLog);
}


