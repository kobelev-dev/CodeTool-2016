//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	TextScan_Globals_H
#define	TextScan_Globals_H

#ifndef   TextScan_CoreTypes_H
#include  "TextScan/CoreTypes.H"
#endif
#ifndef   TextScan_CmdlScan_H
#include  "TextScan/CmdlScan.H"
#endif

//
// This structure establishes a link between the name of the application, that is typically picked up from
// the beginning of the cmd line, and various props that are needed to start the parsing process.
//
// This structure is also used when it is necessary to find the key params start chars string and the cmdl
// keys dictionary for certain raw scanning type of the command line file.
//
struct TParseEnvInfo
{
	wchar_t				*app_name;

	TRawScanningType		cmdl_rscn_type;

	wchar_t				*cmdl_ks_chars;			// Characters that start the command line key params.
	TCmdlKeysDictEntry	*cmdl_keys_dict;		// The dictionary of the known command line keys.

	TFileStorageType		src_file_strg;
	TRawScanningType		src_file_rscn;

public:

	static TParseEnvInfo	*FindParseEnvForAppName(const wchar_t *app_name_from_cmd_line, long app_name_len);
	static TParseEnvInfo	*FindParseEnvForRawScanningType(TRawScanningType rscn);
};

extern TParseEnvInfo	g_cl_app_info;
extern TParseEnvInfo	g_mc_app_info;

//
// Information about the built in file. The built in files have less params than the regular disk files.
// For example they do not have dates.
//
struct TBuiltInFileInfo
{
	ID					fid;
	wchar_t				*fname;
	wchar_t				*fbody;
	TRawScanningType		rscn;

public:

	static TBuiltInFileInfo	*GetBuiltInMacroFileInfo(ID file_id);
							// This API can be used only for picking up the files that contain definitions of the built in macros.
							// Passing the fileId == 0 means a request for the most recent file (typical case).
};

#endif	// TextScan_Globals_H


