//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <string.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <windows.h>

#include  "Common/FormatNumber.H"

//-----------------------------------------------------------------
//   ================  Static Functions  ====================
//-----------------------------------------------------------------

static wchar_t *FormatDecimalNumberValue(wchar_t *ptrBuff, __int64 value, FormatNumberStyle format_style, wchar_t thousands_separator_char)
{
	// Generate signed or unsigned decimal representation.
	if (value == 0)
	{
		// The result contains just one digit.
		*(--ptrBuff) = L'0';
	}
	else
	{
		// Split the source into the sign flag and the positive value.
		bool minusFlag = FALSE;
		unsigned __int64 unsVal = (unsigned __int64)value;
		if (format_style == fnms_dec_signed && value < 0)
		{
			minusFlag = TRUE;
			if (value != 0x8000000000000000)
				unsVal = (unsigned __int64)(-value);
		}

		int numDigits = 0;
		while (unsVal != 0)
		{
			*(--ptrBuff) = L'0' + (unsigned short)(unsVal % 10);
			unsVal /= 10;

			if (thousands_separator_char != 0 && (++numDigits == 3) && unsVal != 0)
			{
				*(--ptrBuff) = thousands_separator_char;
				numDigits = 0;
			}
		}

		// Check for the minus sign.
		if (minusFlag == TRUE)
			*(--ptrBuff) = L'-';
	}

	return(ptrBuff);
}

static wchar_t *FormatHexNumberValue(wchar_t *ptrBuff, __int64 value, FormatNumberStyle formatStyle, wchar_t thousands_separator_char)
{
	// The hex number is needed.
	unsigned __int64 unsVal = (unsigned __int64)value;
	int minHexDigits = formatStyle;
	if (minHexDigits > 16)
		minHexDigits = 16;

	int numDigits = 0;
	while (unsVal != 0 || minHexDigits > 0)
	{
		int val = (int)(unsVal & 0xF);
		*(--ptrBuff) = (wchar_t)((val <= 9) ? L'0'+val : L'A'+val-10);
		unsVal >>= 4;

		if (thousands_separator_char != 0 && (++numDigits == 4))
		{
			*(--ptrBuff) = thousands_separator_char;
			numDigits = 0;
		}

		minHexDigits--;
	}

	// Add the "0x" prefix to the string.
	int lenPrefix = (thousands_separator_char == 0 || numDigits == 0) ? 2 : 3;
	ptrBuff -= lenPrefix;
	wcsncpy(ptrBuff, L"0x ", lenPrefix);

	return(ptrBuff);
}

wchar_t *FormatInt64(__int64 value, wchar_t *pBuffer, int lenBuff, FormatNumberStyle formatStyle, wchar_t thousands_separator_char, const wchar_t *valueSuffix)
{
	if (lenBuff <= 0)
		return(pBuffer);

	// Generate text representation of the number in the local buffer.
	wchar_t buff[80], *ptr = buff+80;
	*(--ptr) = 0;

	if (formatStyle <= 0)
	{
		ptr = FormatDecimalNumberValue(ptr, value, formatStyle, thousands_separator_char);
	}
	else
	{
		ptr = FormatHexNumberValue(ptr, value, formatStyle, thousands_separator_char);
	}

	wcsncpy(pBuffer, ptr, lenBuff);
	pBuffer[lenBuff-1] = 0;
	int lenValue = (int)wcslen(pBuffer);
	wchar_t *pFreeSpace = pBuffer + lenValue;
	lenBuff -= lenValue;

	if (valueSuffix != NULL && lenBuff > 1)
	{
		wcsncpy(pFreeSpace, valueSuffix, lenBuff);
		pFreeSpace[lenBuff-1] = 0;
	}

	return(pBuffer);
}

wchar_t *FormatGuid(GUID &value, wchar_t *pBuffer, int lenBuff)
{
	// Use the standard format that it is used in Windows.
	swprintf(pBuffer, lenBuff, L"%08lx-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
			value.Data1, value.Data2, value.Data3, value.Data4[0], value.Data4[1],
			value.Data4[2], value.Data4[3], value.Data4[4], value.Data4[5],
			value.Data4[6], value.Data4[7]);

	// Success.
	return(pBuffer);
}

wchar_t *PluralSuffix(__int64 value)
{
	if ((value % 10 == 1) && (value % 100 != 11))
		return(L"");

	return(L"s");
}

wchar_t *PluralSuffixUnsigned(unsigned __int64 value)
{
	if ((value % 10 == 1) && (value % 100 != 11))
		return(L"");

	return(L"s");
}

wchar_t *GetPresentTenseVerbForNumber(__int64 value)
{
	if ((value % 10 == 1) && (value % 100 != 11))
		return(L"is");

	return(L"are");
}

bool AssembleInt64(wchar_t *buffer, __int64 &result, AssembleNumberExpectedValueType xpct_value_type, wchar_t *chars_to_ignore, wchar_t **pntSuff, AssembleNumberInputStringStyle *input_str_style)
{
	result = 0;
	if (pntSuff != NULL)
		*pntSuff = NULL;
	if (input_str_style != NULL)
		*input_str_style = anstrs_none;

	bool minusPresent = FALSE;
	bool decDigitsPresent = FALSE;
	bool biggestNegativeValue = FALSE;
	bool hexMode = FALSE;
	short numHexDigits = 0;

	unsigned __int64 value = 0;
	for (wchar_t *pnt=buffer; *pnt; ++pnt)
	{
		if (*pnt == L' ' || *pnt == L'\t' || (chars_to_ignore != NULL && wcschr(chars_to_ignore, *pnt) != NULL) != NULL)
		{
			// This is a space char or the app layer has instructed to ignore this delimiter.
			continue;
		}

		if (*pnt == L'+' && value == 0)
		{
			// Ignore this leading plus.
			continue;
		}

		if (*pnt == L'-' && xpct_value_type == anxvt_signed && hexMode == FALSE && minusPresent == FALSE && value == 0)
		{
			// The string can be treated as a signed value.
			minusPresent = TRUE;
			continue;
		}

		if ((*pnt == L'x' || *pnt == L'X') && hexMode == FALSE && minusPresent == FALSE && value == 0)
		{
			// The string can be treated as a hex value.
			hexMode = TRUE;
			continue;
		}

		// Process the next digit.
		if (hexMode == FALSE)
		{
			if (*pnt >= L'0' && *pnt <= L'9')
			{
				decDigitsPresent = TRUE;

				if (biggestNegativeValue == TRUE)
				{
					// More digits when the magic value is already achieved.
					return(FALSE);
				}

				value = 10*value + (*pnt & 15);
				if (xpct_value_type == anxvt_signed)
				{
					if (value == 0x8000000000000000)
					{
						if (minusPresent == FALSE)
						{
							// This is signed int64 overflow.
							return(FALSE);
						}

						biggestNegativeValue = TRUE;
						continue;
					}
					else if (value > 0x8000000000000000)
					{
						// This is signed int64 overflow.
						return(FALSE);
					}
				}

				continue;
			}
		}
		else
		{
			if (*pnt >= L'0' && *pnt <= L'9' || *pnt >= L'a' && *pnt <= L'f' || *pnt >= L'A' && *pnt <= L'F')
			{
				if (++numHexDigits > 16)
					return(FALSE);

				if (*pnt <= L'9')
					value = (value << 4) + (*pnt & 15);
				else value = (value << 4) + (*pnt & 7) + 9;

				continue;
			}
		}

		// Buffer contains character that cannot be part of the number. This means that either
		// the input string is bogus or that the number contains the suffix.
		if (pntSuff == NULL)
		{
			// Application thinks that the suffix should not be present.
			return(FALSE);
		}
		else
		{
			// Return the suffix to the app layer.
			*pntSuff = pnt;
			break;
		}
	}

	// Check for the number presence.
	if (hexMode == FALSE)
	{
		if (decDigitsPresent == FALSE)
			return(FALSE);
	}
	else
	{
		if (numHexDigits == 0)
			return(FALSE);
	}

	if (minusPresent == TRUE && biggestNegativeValue == FALSE)
	{
		__int64 tempValue = value;
		result = -tempValue;
	}
	else
	{
		result = value;
	}

	if (input_str_style != NULL)
		*input_str_style = (hexMode == TRUE) ? anstrs_hex_value : ((minusPresent == TRUE) ? anstrs_negative_dec : anstrs_positive_dec);

	// Success.
	return(TRUE);
}


