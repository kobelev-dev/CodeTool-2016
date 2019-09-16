//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Processing/IdentifiersHelper.H"

// -----------------------------------------------------------------------------
//	=====================  TIdentifiersHelper  =======================
// -----------------------------------------------------------------------------

TIdentifierStyle TIdentifiersHelper::GetBasicIdentStyle(TStrPtrInfo &name, bool allow_prefix, int &detected_prefix_len)
{
	detected_prefix_len = 0;
	int body_beg = 0;

	if (allow_prefix == TRUE)
	{
		// Search for the first underscore in the name.
		for (int i1=0; i1<name.m_len; ++i1)
		{
			if (name.m_body[i1] == L'_')
			{
				detected_prefix_len = i1+1;
				body_beg = i1+1;
				break;
			}
		}
	}

	if (body_beg >= name.m_len)
	{
		// The passed name has no body.
		return(ids_none);
	}

	// Find out what sort of chars are present in the passed name.
	bool underscore_present = FALSE;
	bool lower_case_present = FALSE;
	bool upper_case_present = FALSE;

	for (int i2=body_beg; i2<name.m_len; ++i2)
	{
		wchar_t ch = name.m_body[i2];
		if (ch == L'_')
			underscore_present = TRUE;
		else if (IsLowerCase(ch) == TRUE)
			lower_case_present = TRUE;
		else if (IsUpperCase(ch) == TRUE)
			upper_case_present = TRUE;
	}

	if (underscore_present == FALSE)
	{
		wchar_t ch = name.m_body[body_beg];
		if (IsLowerCase(ch) == TRUE)
			return(ids_wbeg_caps_exc_first);
		else if (IsUpperCase(ch) == TRUE)
			return(ids_word_beg_caps);
	}
	else
	{
		if (lower_case_present == TRUE && upper_case_present == FALSE)
			return(ids_lo_underscore);
		else if (lower_case_present == FALSE && upper_case_present == TRUE)
			return(ids_caps_underscore);
	}

	// Passed name does not belong to any of the categories.
	return(ids_none);
}

bool TIdentifiersHelper::ConvertCapsToUnderscoredLows(TNameBuffer &buffer, TStrPtrInfo &name)
{
	buffer.ClearBuffer();

	for (int inx=0; inx<name.m_len; ++inx)
	{
		wchar_t ch = name.m_body[inx];
		if (IsUpperCase(ch) == TRUE)
		{
			// Current char is the upper case character.
			if (inx > 0)
				buffer.Append(L'_');

			buffer.Append(ch-L'A'+L'a');
		}
		else
		{
			// Simply place this char into the output buffer.
			buffer.Append(ch);
		}
	}

	return(buffer.GetXpndError() == FALSE);
}

bool TIdentifiersHelper::ConvertUnderscoredLowsToCaps(TNameBuffer &buffer, TStrPtrInfo &name)
{
	buffer.ClearBuffer();

	for (int inx=0; inx<name.m_len; ++inx)
	{
		wchar_t ch = name.m_body[inx];
		if (inx == 0 && IsLowerCase(ch) == TRUE)
		{
			// Passed name starts from the lowercase character.
			buffer.Append(ch-L'a'+L'A');
		}
		else if (ch == L'_' && inx<name.m_len-1 && IsLowerCase(name.m_body[inx+1]) == TRUE)
		{
			// Current char is underscore that is followed by the lower case char.
			wchar_t nch = name.m_body[inx+1];
			buffer.Append(nch-L'a'+L'A');
			inx++;
		}
		else
		{
			// Simply place this char into the output buffer.
			buffer.Append(ch);
		}
	}

	return(buffer.GetXpndError() == FALSE);
}


