//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <string.h>
#include  <stdlib.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/StringPointer.H"

//----------------------------------------------------------------
//   ================  TStringPtr  =======================
//----------------------------------------------------------------

int TStringPtr::Compare(const TStringPtr &ptrStr1, const TStringPtr &ptrStr2)
{
	int inx = 0;
	wchar_t *pStr1 = ptrStr1.m_body;
	wchar_t *pStr2 = ptrStr2.m_body;

	// Compare the bodies of the strings char by char.
	while (inx < ptrStr1.m_len && inx < ptrStr2.m_len)
	{
		if (*pStr1 > *pStr2)
			return(1);
		else if (*pStr1 < *pStr2)
			return(-1);

		inx++;
		pStr1++;
		pStr2++;
	}

	// One of the strings has terminated or they are identical.
	if (ptrStr1.m_len > ptrStr2.m_len)
		return(1);
	else if (ptrStr1.m_len < ptrStr2.m_len)
		return(-1);

	// The strings are identical.
	return(0);
}

wchar_t *TStringPtr::CopyWithPossibleTruncationTo(wchar_t *buff, int buffLen) const
{
	assert(buffLen >= 7);

	if (m_body == NULL)
	{
		wcscpy(buff, L"<NULL>");
	}
	else if (m_len < buffLen)
	{
		wcsncpy(buff, m_body, m_len);
		buff[m_len] = 0;
	}
	else
	{
		int lenToCopy = buffLen-4;
		wcsncpy(buff, m_body, lenToCopy);
		wcscpy(buff+lenToCopy, L"...");
	}

	return(buff);
}

wchar_t	*TStringPtr::CopyToVerifiedBuffer(wchar_t *long_enough_buffer) const
{
	wcsncpy(long_enough_buffer, m_body, m_len);
	long_enough_buffer[m_len] = 0;
	return(long_enough_buffer);
}

//----------------------------------------------------------------
//   ================  TStrPtrInfo  ======================
//----------------------------------------------------------------

wchar_t *TStrPtrInfo::CopyWithTruncationTo(wchar_t *buff, int buffLen) const
{
	assert(buffLen >= 7);

	if (m_body == NULL)
	{
		wcscpy(buff, L"<NULL>");
	}
	else if (m_len < buffLen)
	{
		wcsncpy(buff, m_body, m_len);
		buff[m_len] = 0;
	}
	else
	{
		int lenToCopy = buffLen-4;
		wcsncpy(buff, m_body, lenToCopy);
		wcscpy(buff+lenToCopy, L"...");
	}

	return(buff);
}

wchar_t	*TStrPtrInfo::CopyToVerifiedBuffer(wchar_t *long_enough_buffer) const
{
	wcsncpy(long_enough_buffer, m_body, m_len);
	long_enough_buffer[m_len] = 0;
	return(long_enough_buffer);
}


