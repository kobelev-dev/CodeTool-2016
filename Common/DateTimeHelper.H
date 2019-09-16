//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Basic date time handling support.
//

#ifndef	Common_DateTimeHelper_H
#define	Common_DateTimeHelper_H

#define   ___min(x, y)		(((x)<(y)) ? (x) : (y))
#define   ___max(x, y)	(((x)>(y)) ? (x) : (y))

typedef __int64 TDateTime;			// This is actually an alias to NT FILETIME.

#define   MILLI_SEC		((__int64)10000)
#define   ONE_SECOND	((__int64)10000000)
#define   ONE_MINUTE	(((__int64)10000000)*60)
#define   ONE_HOUR		(((__int64)10000000)*3600)
#define   ONE_DAY		(ONE_HOUR*24)
#define   ONE_YEAR		(ONE_DAY*365)

inline	TDateTime	CurrDateTime() { TDateTime param; ::GetSystemTimeAsFileTime((FILETIME*)&param); return(param); }
inline	TDateTime	MakeDateTime(const FILETIME &param)  { TDateTime value = *((TDateTime*)&param); return(value); }


wchar_t		*FormatDateTime(TDateTime dateTime, wchar_t *pBuffer, int buffLen, bool wantDoubleSpace = TRUE);
wchar_t		*FormatUnixTime(DWORD srcTime, wchar_t *pBuffer, int buffLen, bool wantDoubleSpace = TRUE);

wchar_t		*FormatDuration(TDateTime dataTimeDiff, wchar_t *pBuffer, int buffLen, bool showDaysSeparately = FALSE, bool want_wide_space = FALSE);
					//
					//  Formats:	L"%d day%s %02d:%02d:%02d"		(showDaysSeparately == TRUE && numDurationHours >= 24);
					//			L"%02d:%02d:%02d"				(otherwise);
					//

#endif	// Common_DateTimeHelper_H


