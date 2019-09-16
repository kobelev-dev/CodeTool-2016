//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Formatting INT64 with possible groupping of digits.
//

#ifndef	Common_FormatNumber_H
#define	Common_FormatNumber_H

//
//  Integers in the range 1..16 can be typecasted to this enum. This allows generating
//  hex numbers with any required minimal number of hex digits.
//
enum FormatNumberStyle
{
	fnms_dec_unsigned	= -1,
	fnms_dec_signed		=  0,
	fnms_hex			=  1,
	fnms_hex8			=  2,
	fnms_hex16			=  4,
	fnms_hex32			=  8,
	fnms_hex64			= 16,
};

wchar_t	*FormatInt64(__int64 value, wchar_t *pBuffer, int lenBuff,
						FormatNumberStyle format_style = fnms_dec_signed,
						wchar_t thousands_separator_char = 0,
						const wchar_t *value_suffix = NULL);
			//
			//  Decimal digits are always groupped by 3 and the hex digits are always groupped by 4.
			//  When the param "thousands_separator_char" is ZERO, there will be no digits groupping.
			//
			//  The string is silently truncated when the size of the buffer is not big enough.
			//  After return the buffer is always zero reminated.
			//

wchar_t	*FormatGuid(GUID &value, wchar_t *pBuffer, int lenBuff);
			//
			//  Used format: L"%08lx-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x".
			//

wchar_t	*PluralSuffix(__int64 value);
wchar_t	*PluralSuffixUnsigned(unsigned __int64 value);
			// The return value is either L"" or L"s".

wchar_t	*GetPresentTenseVerbForNumber(__int64 value);
			// The return value is either L"is" or L"are".

//
//  The result of assembling is returned as a signed type value. Although it can contain an unsigned value.
//
enum AssembleNumberExpectedValueType
{
	anxvt_signed,
	anxvt_unsigned,
};

enum AssembleNumberInputStringStyle
{
	anstrs_none,					// The type of the input string is not detected yet.
	anstrs_positive_dec,
	anstrs_negative_dec,
	anstrs_hex_value,
};

bool		AssembleInt64(wchar_t *buffer, __int64 &result, AssembleNumberExpectedValueType xpct_value_type = anxvt_signed, wchar_t *chars_to_ignore = NULL,
							wchar_t **pntSuff = NULL, AssembleNumberInputStringStyle *input_str_style = NULL);
			//
			//  The return value is TRUE when the passed string contains a number. Empty string is not considered
			//  as a number. When the param "pntSuff" is not NULL, the number can be followed by the suffix, that is
			//  returned in this param. Otherwise the stirng should contain only the number and nothing else.
			//

#endif	// Common_FormatNumber_H


