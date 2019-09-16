//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Processing_FixGrammarSources_H
#define	Processing_FixGrammarSources_H

#ifndef   Processing_RawProcessing_H
#include  "Processing/RawProcessing.H"
#endif
#ifndef   Processing_RefactoringEngine_H
#include  "Processing/RefactoringEngine.H"
#endif

void		DoGrammarSourcesConversion(const wchar_t *grammars_root_abs_dir, const wchar_t *grm_to_process_sub_dir, const wchar_t *dest_subdir = NULL);
			//
			// This function allows 2 different types of calls:
			//
			//	DoGrammarSourcesConversion(L"c:/Grammars", L"LanguageB", L"NewGrmSubdir");
			//
			//		The grammar sources are taken from	L"c:/Grammars/LanguageB";
			//		Converted sources are placed into	L"c:/Grammars/NewGrmSubdir";
			//		The backup is not taking place.
			//
			//	DoGrammarSourcesConversion(L"c:/Grammars", L"LanguageB");
			//
			//		The grammar sources are taken from	L"c:/Grammars/LanguageB";
			//		Old state of the sources is backed into	L"c:/Grammars/Backup/LanguageB";
			//		Converted sources are placed into	L"c:/Grammars/LanguageB";
			//

#endif	// Processing_FixGrammarSources_H


