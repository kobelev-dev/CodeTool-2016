//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Processing_IdentifiersHelper_H
#define	Processing_IdentifiersHelper_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   Common_StringPointer_H
#include  "Common/StringPointer.H"
#endif

enum TIdentifierStyle
{
	ids_none,
	ids_lo_underscore,				// Something like:  abc_defg_next_word_etc.
	ids_caps_underscore,			// Something like:  ABC_DEFG_NEXT_WORD_ETC.
	ids_word_beg_caps,				// Something like:  AbcDefgNextWordEtc.
	ids_wbeg_caps_exc_first,		// Something like:  abcDefgNextWordEtc.
};

class TIdentifiersHelper
{
public:

	static TIdentifierStyle	GetBasicIdentStyle(TStrPtrInfo &name, bool allow_prefix, int &detected_prefix_len);

	static bool	ConvertCapsToUnderscoredLows(TNameBuffer &dest_buffer, TStrPtrInfo &src_name);
	static bool	ConvertUnderscoredLowsToCaps(TNameBuffer &dest_buffer, TStrPtrInfo &src_name);

protected:

	static bool	IsLowerCase(wchar_t ch) { return(ch >= L'a' && ch <= L'z'); }
	static bool	IsUpperCase(wchar_t ch) { return(ch >= L'A' && ch <= L'Z'); }
};

#endif	// Processing_IdentifiersHelper_H


