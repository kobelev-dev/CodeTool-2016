//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "TextScan/Globals.H"

//--------------------------------------------------------------------------
//  ================  Command lilne keys  =========================
//--------------------------------------------------------------------------

TCmdlKeysDictEntry g_cl_app_cmdl_keys[] =
{
	//     key_ident			key_name		delim_prop	value_prop
	{ cmdk_include_path,		DefLN(L"-I"),		cmkp_opt,	cmkp_req	},
	{ cmdk_include_path,		DefLN(L"/I"),		cmkp_opt,	cmkp_req	},
	{ cmdk_predefined_cond,	DefLN(L"-D"),	cmkp_opt,	cmkp_req	},
	{ cmdk_predefined_cond,	DefLN(L"/D"),	cmkp_opt,	cmkp_req	},
	{ cmdk_nested_src_disk,	DefLN(L"-FI"),	cmkp_none,	cmkp_req	},
	{ cmdk_nested_src_disk,	DefLN(L"/FI"),	cmkp_none,	cmkp_req	},
	{ cmdk_analysis_tool_key,	DefLN(L"-ant:"),	cmkp_none,	cmkp_req	},
	{ cmdk_nested_cmdl_disk,	DefLN(L"@@"),	cmkp_none,	cmkp_req	},
	{ cmdk_nested_cmdl_temp,	DefLN(L"@"),		cmkp_none,	cmkp_req	},
	{ cmdk_ignore_key,		DefLN(L"-"),		cmkp_none,	cmkp_req	},
	{ cmdk_ignore_key,		DefLN(L"/"),		cmkp_none,	cmkp_req	},
	{ 0, 0, NULL, cmkp_none, cmkp_none, },
};

TParseEnvInfo g_cl_app_info =
{
	L"cl",					// app_name
	rscn_cmdl_ms_cl,			// cmdl_rscn_type
	L"-/@",					// cmdl_ks_chars
	g_cl_app_cmdl_keys,		// cmdl_keys_dict
	strg_disk,				// src_file_strg
	rscn_c_cpp,				// src_file_rscn
};

TCmdlKeysDictEntry g_mc_app_cmdl_keys[] =
{
	//     key_ident			key_name		delim_prop	value_prop
	{ cmdk_ignore_key,		DefLN(L"-h"),		cmkp_req,	cmkp_req	},
	{ cmdk_ignore_key,		DefLN(L"-r"),		cmkp_req,	cmkp_req	},
	{ cmdk_ignore_key,		DefLN(L"-"),		cmkp_none,	cmkp_req	},
	{ 0, 0, NULL, cmkp_none, cmkp_none, },
};

TParseEnvInfo g_mc_app_info =
{
	L"mc",					// app_name
	rscn_cmdl_ms_mc,			// cmdl_file_type
	L"-",						// cmdl_ks_chars
	g_mc_app_cmdl_keys,	// cmdl_keys_dict
	strg_disk,				// src_file_strg
	rscn_c_cpp,				// src_file_rscn
};

// -----------------------------------------------------------
//  ================  TParseEnvInfo  =================
// -----------------------------------------------------------

static TParseEnvInfo *g_ParseEnvInfos[] =
{
	&g_cl_app_info,			// cl
	&g_mc_app_info,			// mc
	NULL,
};

TParseEnvInfo *TParseEnvInfo::FindParseEnvForAppName(const wchar_t *app_name_from_cmd_line, long app_name_len)
{
	TParseEnvInfo **pInfos = g_ParseEnvInfos;
	while (*pInfos != NULL)
	{
		if (wcsncmp(app_name_from_cmd_line, (*pInfos)->app_name, app_name_len) == 0)
			return(*pInfos);

		pInfos++;
	}

	// The passed app name is unknown.
	return(NULL);
}

TParseEnvInfo *TParseEnvInfo::FindParseEnvForRawScanningType(TRawScanningType rscn)
{
	TParseEnvInfo **pInfos = g_ParseEnvInfos;
	while (*pInfos != NULL)
	{
		if ((*pInfos)->cmdl_rscn_type == rscn)
			return(*pInfos);

		pInfos++;
	}

	// The passed raw scanning type is unknown.
	return(NULL);
}

// --------------------------------------------------------
//  ===============  Built in files  ==================
// --------------------------------------------------------

static wchar_t g_BuiltInMacro12[] =
{
	L"#define __FILE__\r\n"
	L"#define __LINE__\r\n"
	L"#define __FUNCTION__\r\n"
	L"#define __COUNTER__\r\n"
	L"\r\n"
	L"#define  NULL\t   0\r\n"
	L"#define  __cplusplus\r\n"
	L"\r\n"
	L"#define  __cdecl\r\n"
	L"\r\n"
	L"// Microsoft extensions.\r\n"
	L"#define __inline\t inline\r\n"
	L"#define __declspec(x)\r\n"
};

static TBuiltInFileInfo g_BuiltInFile12 =
{
	12,						// file_id.
	L"@BuiltInFile12.inc",		// file name.
	g_BuiltInMacro12,		// file body.
	rscn_c_cpp,				// raw scanning type.
};

TBuiltInFileInfo *TBuiltInFileInfo::GetBuiltInMacroFileInfo(ID file_id)
{
	if (file_id == 0)
	{
		// Return the latest version of the built in macros.
		return(&g_BuiltInFile12);
	}
	else if (file_id == g_BuiltInFile12.fid)
	{
		// Return the file by its explicit id.
		return(&g_BuiltInFile12);
	}

	// The file id is unknown.
	return(NULL);
}


