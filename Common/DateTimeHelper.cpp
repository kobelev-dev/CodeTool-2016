//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Basic date time handling support.
//

#define    STRICT
#include  <string.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <time.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/DateTimeHelper.H"

//---------------------------------------------------------------------
//   ====================  Functions  =======================
//---------------------------------------------------------------------

static void FormatSystemTimeStruct(SYSTEMTIME &systemTime, wchar_t *pBuffer, int buffLen, bool wantWideSpace)
{
	// Convert passed date into the text representation.
	::GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemTime, NULL, pBuffer, buffLen);

	pBuffer[buffLen-1] = 0;
	int lenMsg = (int)wcslen(pBuffer);
	pBuffer += lenMsg;
	buffLen -= lenMsg;
	if (lenMsg > 0 && buffLen > 2)
	{
		wcscpy(pBuffer, (wantWideSpace == TRUE) ? L"  " : L" ");
		pBuffer   += (wantWideSpace == TRUE) ? 2 : 1;
		buffLen -= (wantWideSpace == TRUE) ? 2 : 1;
	}

	// Convert passed time into the text representation.
	::GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, &systemTime, NULL, pBuffer, buffLen);
	pBuffer[buffLen-1] = 0;
}

wchar_t *FormatDateTime(TDateTime dateTime, wchar_t *pBuffer, int buffLen, bool wantWideSpace)
{
	assert(buffLen >= 40);
	pBuffer[0] = 0;

	if (dateTime == 0)
		return(pBuffer);

	FILETIME localTime;
	::FileTimeToLocalFileTime((FILETIME*)&dateTime, &localTime);

	SYSTEMTIME systemTime;
	::FileTimeToSystemTime(&localTime, &systemTime);

	FormatSystemTimeStruct(systemTime, pBuffer, buffLen, wantWideSpace);
	return(pBuffer);
}

wchar_t *FormatUnixTime(DWORD unixTime, wchar_t *pBuffer, int buffLen, bool wantWideSpace)
{
	assert(buffLen >= 40);
	pBuffer[0] = 0;

	if (unixTime == 0)
		return(pBuffer);

	struct tm *structTmTime = localtime((time_t*)&unixTime);

	SYSTEMTIME systemTime;
	memset(&systemTime, 0, sizeof(systemTime));
	systemTime.wYear		= (WORD)structTmTime->tm_year+1900;
	systemTime.wMonth	= (WORD)structTmTime->tm_mon+1;
	systemTime.wDay		= (WORD)structTmTime->tm_mday;
	systemTime.wHour		= (WORD)structTmTime->tm_hour;
	systemTime.wMinute	= (WORD)structTmTime->tm_min;
	systemTime.wSecond	= (WORD)structTmTime->tm_sec;

	FormatSystemTimeStruct(systemTime, pBuffer, buffLen, wantWideSpace);
	return(pBuffer);
}

wchar_t *FormatDuration(TDateTime value, wchar_t *pBuffer, int buffLen, bool showDaysSeparately, bool want_wide_space)
{
	assert(buffLen >= 40);

	int numSecs = (int)(value/ONE_SECOND);
	int numMins = numSecs/60;
	int numHours = numMins/60;
	int retVal = 0;

	if (showDaysSeparately == TRUE && numHours >= 24)
	{
		// Caller asks to show the number of days separately.
		int numDays = numHours/24;
		swprintf(pBuffer, buffLen, L"%d day%s%s %02d:%02d:%02d",
				numDays, (((numDays % 10 == 1) && (numDays % 100 != 11)) ? "" : "s"),
				((want_wide_space == TRUE) ? L" " : L""),
				numHours % 24, numMins % 60, numSecs % 60);
	}
	else
	{
		// Use the default style for formatting the passed duration.
		swprintf(pBuffer, buffLen, L"%02d:%02d:%02d",
				numHours, numMins % 60, numSecs % 60);
	}

	return(pBuffer);
}


