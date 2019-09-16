//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Common_MiscStuffs_H
#define	Common_MiscStuffs_H

class TCommandLineSupp
{
public:

	static bool IsKeyPresent(int argc, wchar_t *argv[], const wchar_t *full_key_body)
	{
		for (int iprm=0; iprm<argc; ++iprm)
		{
			if (argv[iprm] != NULL && wcscmp(argv[iprm], full_key_body) == 0)
				return(TRUE);
		}

		// The requested key is not present on the command line.
		return(FALSE);
	}

	static wchar_t *GetPosParam(int argc, wchar_t *argv[], int one_based_pos_param_index)
	{
		int cnt_params = 0;
		for (int iprm=1; iprm<argc; ++iprm)
		{
			wchar_t *curr_argv = argv[iprm];
			if (curr_argv == NULL || curr_argv[0] == L'-')
				continue;

			cnt_params++;
			if (cnt_params == one_based_pos_param_index)
				return(curr_argv);
		}

		// Pos param with the requested index is not present on the command line.
		return(NULL);
	}
};

class MiscSupp
{
public:

	static void DumpListOfExtensions(const wchar_t **list_of_fname_exts, bool want_extra_delim = TRUE)
	{
		wprintf(L" Extensions:");

		for (int iext=0; list_of_fname_exts[iext] != NULL; ++iext)
		{
			if (*list_of_fname_exts[iext] != 0)
				wprintf(L" %s", list_of_fname_exts[iext]);
			else wprintf(L" <fnames_without_exts>");
		}

		wprintf(L"\r\n");
		if (want_extra_delim == TRUE)
			wprintf(L"\r\n");
	}

	static void DumpArguments(int argc, wchar_t *argv[], bool want_extra_delim = TRUE)
	{
		wprintf(L"Arguments:\r\n");
		for (int iprm=1; iprm<argc; ++iprm)
		{
			wprintf(L"  [%d]: %s\r\n", iprm, argv[iprm]);
		}

		if (want_extra_delim == TRUE)
			wprintf(L"\r\n");
	}
};

#endif	// Common_MiscStuffs_H


