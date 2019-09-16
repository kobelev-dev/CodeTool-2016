//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   C/C++ macroprocessor level.
//

#ifndef	TextScan_MidScan_H
#define	TextScan_MidScan_H

#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif
#ifndef   Common_FileDataHelper_H
#include  "Common/FileDataHelper.H"
#endif
#ifndef   Common_HashTable_H
#include  "Common/HashTable.H"
#endif
#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif

#ifndef   TextScan_CmdlScan_H
#include  "TextScan/CmdlScan.H"
#endif
#ifndef   TextScan_RawScan_H
#include  "TextScan/RawScan.H"
#endif
#ifndef   TextScan_Globals_H
#include  "TextScan/Globals.H"
#endif

//
// The configuration params below should be defined before calling the high level include files.
//
#define    MAX_MACRO_PARAMS_NUM		12
#define    MAX_INCLUDE_STACK_LEN		200
#define    MAX_PASTE_BUFF_STACK_LEN	50
#define    MAX_COND_EXPR_STACK_LEN		400
#define    MAX_IFDEF_STACK_LEN			800

#ifndef   TextScan_MacroDefinition_H
#include  "TextScan/MacroDefinition.H"
#endif
#ifndef   TextScan_CondExpr_H
#include  "TextScan/CondExpr.H"
#endif

class TMidLevStoreReader;

// -----------------------------------------------------------------
//   ==(1)=====  Parsing phase, Options and Summary  ==============
// -----------------------------------------------------------------

enum TMidLevParsingPhase
{
	mlph_none,				// The parsing is inactive. The scanner is just created or it was reset. To do the work it is
							// necessary to start the parsing session.
	mlph_indexing,			// This is raw indexing of disk or temp files. The parsing Id is not available. This state can
							// be changed only to mlph_none.
	mlph_filereg,			// This state is close to mlph_none. Application can only to register files and write errors.
							// The parsing Id is already available.
	mlph_preparing,			// Ths scanner is processing the command line and/or the forced include files. Theparsing
							// options can be changed, but they shoud not be used. The app level lexemas should not
							// be given out, although there is an option that allows this.
	mlph_scanning,			// The major source of lexemas is set and this source is in the process of scanning.
							// The parsing options cannot be changed during this phase.
	mlph_major_eof,		// An end in the major lexemas source file was reached. Most likely the scanner is standing
							// at the end of the command line. No significant activity is expected. The parsing session
							// is waiting to be closed.
	mlph_num_items,
};

struct TMidLevScannerOptions
{
	bool		want_analysis_env;						// When this flag is set to TRUE, the parser will attempt to load the config
													// file with cmdl syntax from directory where the executable is located.
	bool		raw_scanning_mode;						// This is a major switch. Once this flag is set, all other options are
													// not important.
	ID		built_in_macros_fid;						// Id of the file with the built in macros to load. Special values:
													//     0 - use latest file; -1 - do not load any built in conditions at all.

	bool		disab_define_stmts;						// Disable #define and #undef stmts.
	bool		disab_include_stmts;						// Disable #include statements.
	bool		disab_cond_pars_stmts;					// Disable #ifdef, #ifndef, #if, #elif, #else, #endif stmts.
	bool		disab_misc_prepr_stmts;					// Disable #pragma, #error, #import, #line, #using stmts.
													// Note that the disabled statements are not generating parsing events.
	bool		disab_macro_calls;

	TCondExprSimplificationProps	expr_mode;			// Mode of processing expressions in #ifdef, #ifndef, #if, #elif statements.
													// Note, this option is not used during the cond expressions scanning.
													// It controls only the expressions simpification.

	bool		ignore_error_stmt;						// When the value of this flag is TRUE and the parser reaches the #error
													// statement, it writes the parsing error and continues the parsing.
	//
	//  Enable warning options.
	//

	bool		report_macro_redef_to_same_val;
	bool		report_macro_redef_to_diff_val;

	bool		report_includes_from_same_dir;

	bool		report_include_file_duplications;			// This option turns on warnings that show if include file can be found in other
													// directory using less important path. Also it shows cases, where short file
													// name from the statement allows to load the file using some include path.
	bool		report_def_opers_with_unkn_names;		// This warning is emitted while #ifdef or #ifndef statements are processed or
													// while processing defined(x) operation in #if or #elif statements.
													// Note: When unknown names are used as operands in #if or #elif stmts,
													// they are always treated as errors.

	void Clear() { memset(this, 0, sizeof(TMidLevScannerOptions)); }

	void Init(bool noisy_mode = TRUE)
	{
		Clear();
		expr_mode.InitForOrdinaryParsing();
		report_macro_redef_to_same_val = report_macro_redef_to_diff_val = TRUE;
		report_include_file_duplications = report_def_opers_with_unkn_names = TRUE;
	}
};

struct TMidLevPrepOptions
{
	bool			disab_inc_path_keys;
	bool			disab_define_keys;
	bool			disab_src_inc_keys;				// Only the -FI<fn> will be disabled. The @file params will still work.

	bool			immed_mj_src_load;				// When value of this flag is TRUE, the major source file is loaded immediately.
												// Otherwise (default) it is loaded at the end of processing the command line.
	bool			allow_mid_lev_lexemas;			// This flag allows C/C++ lexemas to come out not only from the major source
												// file, but also from the forced includes and/or from the built in definitions file.
	TMidLevScannerOptions	scan_opts;				// This field primarily controls processing of the -FI<fname> files.

	void Clear() { memset(this, 0, sizeof(TMidLevPrepOptions)); }
};

struct TRawIndexingSessSummary
{
	ULONG		files_indexed;					// This counter shows how many files were sucessfully indexed.

	ULONG		num_parsing_msgs;
	ULONG		num_warnings;
	ULONG		num_errors;

	void Clear() { memset(this, 0, sizeof(TRawIndexingSessSummary)); }
};

struct TMidLevParsingSummary
{
	//
	//  Cmld counters do not include the compiler_name lexema, cmdl errors and cmdl EOF lexema.
	//

	ULONG		cmdl_key_params;
	ULONG		cmdl_pos_params;				// The number of pos params that were already scanned. This field is also used
												// as a counter to check if is it high time to call the major source file or not.
												// The name of the application on the cmdl is treated as pos param[0].
	ULONG		file_src_lexemas;				// Lexemas from C/C++ source files and clang lexemas from mdefs in cmdls.
	ULONG		macro_src_lexemas;				// Lexemas from macro and macro params.
	ULONG		pbuff_src_lexemas;				// Lexemas from paste buffers.
	ULONG		mid_lev_lexemas;				// Number of lexemas that were given by the mid lev scanner to higher levels.
												// Final EOFs are not included.
	ULONG		drive_substs_num;				// Number of drive or directory substitutions.
	ULONG		inc_paths_num;					// Number of different include paths.
	ULONG		built_in_conds;					// Number of built in conditions.
	ULONG		predefined_conds;				// Number of external predefined conditions. These are definitions that were
												// added before entering the major source file.
	ULONG		files_cache_size;				// Number of objects in the known_files field. These are: disk, built_in, proto,
	ULONG		temp_files_num;					// These are only strg_temp files.
	ULONG		interm_files_num;				// Number of strg_interm buffers. Note that several nested paste areas can
												// share the same intermediate file.
	ULONG		file_source_loads;
	ULONG		different_file_srcs;				// Disk files only.
	ULONG		diff_file_srcs_len;				// Total length of all different disk file sources that were loaded during parsing.

	ULONG		macro_dict_size;				// Size of the macro dictionary at the end of parsing.
	ULONG		different_macro_used;
	ULONG		macro_calls_num;
	ULONG		mparam_calls_num;

	ULONG		paste_buff_areas;				// Number of times the paste buffering is started.
	ULONG		paste_buff_sources;				// This counter is incremented when paste buff is entered as a lex source.

	ULONG		cond_parsing_areas;				// Number of conditional parsing statements not including #endif statements in all
												// code paths (active and passive) plus one.
	ULONG		cond_params_num;

	ULONG		num_parsing_msgs;				// Parsing message is any text comment that scanner or parser wants to add.
	ULONG		num_warnings;
	ULONG		num_errors;

	ULONG		mid_lev_events;
	ULONG		final_linear_offs;
	ID			final_cn;

	void Clear() { memset(this, 0, sizeof(TMidLevParsingSummary)); }
};

//
//  NB: High byte of the trace mask can be used by the high level parsers.
//
enum TMidLevScannerTraceFlags
{
	//
	//  General group.
	//

	// Mixed model.
	sctr_major_actions	= 0x00000001,
	sctr_main_phase	= 0x00000040,
			#define TPRF_BEG_PARS_SESS		L"========> BeginParsingSession ---"		// - 1 enter + 2 failures + 1 success.
			#define TPRF_BEG_RAW_SESS		L"========> BeginRawIndxSession ---"	// - 1 enter + 2 failures + 1 success.
			#define TPRF_PREP_PHASE			L"========> BeginPrepPhase --------"	// - 1 enter + 3 failures + 1 success.
			#define TPRF_MAIN_PHASE			L"========> BeginMainPhase --------"	// - 1 enter + 3 failures + 1 success.
			#define TPRF_CL_PARS_SESS		L"========> CloseParsingSession ---"		// - 1 enter + 1 exit.
			#define TPRF_CL_RAW_SESS		L"========> CloseRawIndxSession ---"	// - 1 enter + 1 exit.
			#define TPRF_MLEV_RESET			L"====>MidLevReset"						// - 1 place (some_phase --> mlph_none).
			#define TPRF_SCAN_PHASE			L"=========>Scan()"					// - 1 place (setting phase to major_eof).
			#define TPRF_ABORT_PARS			L"===>AbortParsing"						// - 1 place.
			#define TPRF_EXIT_PHASE			L"=====>LexSrcExit"						// - 1 place (setting phase to major_eof).
			#define TPRF_DONT_WANT_CE		L"LoadAnalysisEnvrm"						// - 1 place.
			#define TPRF_LOAD_ANT_ENV		L"LoadAnalysisEnvrm"						// - 1 enter + 1 reject + 1 failures + 1 success/failure (LoadAnalysisToolProfile).
			#define TPRF_LOAD_BUILTIN		L"LoadBuiltIn"								// - 1 enter + 1 reject + 3 failures + 1 success/failure (LoadBuiltInMacrosFile).

	// Entry/Exit model.
	sctr_scan_enter		= 0x00000002,
	sctr_scan_exit		= 0x00000004,
			#define TPRF_SCAN				L"++Scan()--"			// - 1 enter + 1 exit.

	// Mixed model.
	sctr_mid_scan		= 0x00000008,
			#define TPRF_MIDSCAN				L"+MidScan--"			// - 1 enter + 1 exit.
			#define TPRF_MCALL_PRMS			L"  MCallPrm"				// - 1 enter + 1 exit.
			#define TPRF_MIDSCAN_P1			L" MidScan----P1--"		// - 1 place.
			#define TPRF_MIDSCAN_P4			L" MidScan----P4--"		// - 1 place.
			#define TPRF_MIDSCAN_P6			L" MidScan----P6--"		// - 1 place.

	// Single ntf model.
	sctr_file_retr		= 0x00000010,
			#define TPRF_RAW_LEX				L"     ---  RawLex"			// - 2 places.
			#define TPRF_RAW_UNSCAN			L"    -- RawUnscan"		// - 4 places.

	// Single ntf model.
	sctr_cmdl_retr		= 0x00000020,
			#define TPRF_CMDL_RAW_LEX		L"  --- CmdlRawLex"		// - 1 place.
			#define TPRF_CMDL_CLG_LEX		L" -- CmdlClangLex"		// - 1 place.

	// Entry/Exit model.
	sctr_new_path		= 0x00000100,
			#define TPRF_ADD_INC_PATH		L"  AddIPath"				// - 1 enter +  4 failures + 1 success.

	// Single ntf model.
	sctr_new_file		= 0x00000200,
			#define TPRF_INCLUDE_HLPR		L"  IncFileLodHlpr"			// - 1 enter + 1 success + 1 error.
			#define TPRF_CREATE_FINFO		L"  CreateFileInfo"			// - 1 success + 6 failures.
			#define TPRF_REG_NEW_FILE		L"  RegistrNewFile"			// - 5 failures + 1 success.

	// Entry/Exit model.
	sctr_new_macro		= 0x00000400,
			#define TPRF_UPDATE_MLIB			L"  UpdtMlib"				// - 1 enter + 2 failures + 1 success.

	// Single ntf model.
	sctr_new_key		= 0x00000800,
			#define TPRF_NEW_KEY				L"  LookupOrRegKey"		// - 2 failures + 2 successes.

	// Single ntf model.
	sctr_err_warn		= 0x00001000,
			#define TPRF_PARS_MSG			L"****PARS_MSG****"		// - 1 place for all 3.
			#define TPRF_WARNING			L"****WARNING*****"
			#define TPRF_SYNT_ERR			L"****SYNT_ERR****"

	// Single ntf model.
	sctr_lex_src_enter	= 0x00002000,
			#define TPRF_ENTER_FILE			L"  LexEnter(file)"			// - 1 place (2 types of failures or 1 success).
			#define TPRF_ENTER_MDEF			L"  LexEnter(mdef)"		// - 1 place (1 failure or 1 success).
			#define TPRF_ENTER_MPRM			L"  LexEnter(mprm)"		// - 1 place (1 failure or 1 success).
			#define TPRF_ENTER_PBUF			L"  LexEnter(pbuf)"			// - 1 place (1 failure or 1 success).

	// Single ntf model.
	sctr_lex_src_exit	= 0x00004000,
			#define TPRF_LSRC_EXIT			L"  LexSrcExit(xx)"			// - 1 place (ExitFromLexSource).

	// Single ntf model.
	sctr_paste_buff		= 0x00008000,
			#define TPRF_PBUFF_ENTER			L"  PbuffLayerEntr"			// - 2 failures + 1 success (EnterPasteBuffLayer).
			#define TPRF_PBUFF_EXIT			L"  PbuffLayerExit"			// - 1 success  (ExitPasteBuffLayer).
			#define TPRF_PBUFF_CONTS			L"  PbuffContsInfo"			// - 1 place    (ExitFromLexSource: truncation and saving interm file).

	//
	//  Statements group. All tags in the group follow the entry/exit pattern.
	//

	sctr_stmt_def_undef	= 0x00010000,
			#define TPRF_STMT_DEFINE			L"  #definex"				// - 1 EmptyStmt + 1 enter + 2 failures + 1 success/failure.
			#define TPRF_STMT_UNDEF			L"  #undefxx"				// - 1 EmptyStmt + 1 enter + 2 failures + 1 success/failure.

	sctr_stmt_include	= 0x00020000,
			#define TPRF_STMT_INCLUDE		L"  #include"				// - 1 EmptyStmt + 1 enter + 1 misc_mid_lev_evt + 1 success/failure.

	sctr_stmt_cond_pars	= 0x00040000,
			#define TPRF_STMT_CONDPR		L"  #CondPar"				// - 1 EmptyStmt + 1 enter + 2 failures + 1 success.

	sctr_stmt_pragma_error	= 0x00080000,
			#define TPRF_STMT_PRAGMA		L"  #pragmax"			//  1 EmptyStmt + 1 enter + 1 exit.
			#define TPRF_STMT_ERROR			L"  #errorxx"				//  1 EmptyStmt + 1 enter + 1 exit.

	sctr_stmt_misc_mid_lev	= 0x00100000,
			#define TPRF_STMT_IMPORT			L"  #importx"				// - 1 EmptyStmt + 1 enter + 1 exit.
			#define TPRF_STMT_LINE			L"  #linexxx"				// - 1 EmptyStmt + 1 enter + 1 exit.
			#define TPRF_STMT_USING			L"  #usingxx"				// - 1 EmptyStmt + 1 enter + 1 exit.
			#define TPRF_STMT_BOGUS			L"  #!Bogus!"				// - 1 enter + 1 exit.

	//
	//  Cmdl group.
	//

	// Mixed model.
	sctr_cmdl_pos_param	= 0x00200000,
			#define TPRF_CMDL_POS			L"  >>CmdlPosParam"		// - 2 places.
			#define TPRF_CMDL_MJSRC			L">>MajorSrc"			// - 1 entry + 1 failue + 1 success + 1 succes/failure.

	// Mixed model.
	sctr_cmdl_key_param	= 0x00400000,
			#define TPRF_CMDL_PATH			L">>CmdlPath"			// - 1 enter + 1 exit.
			#define TPRF_CMDL_MDEF			L">>CmdlMdef"			// - 1 enter + 2 failures + 1 success/failure (all in CmdlProcessDefinitionKey).
			#define TPRF_CMDL_NEST			L">>CmdlNest"			// - 1 enter + 2 failures + 1 success/failure (all in CmdlNestLexSource).
			#define TPRF_CMDL_COMPIL		L"  >>CompilerName"		// - 1 place.
			#define TPRF_CMDL_IGNORE		L"  >>>>CmdlIgnore"		// - 4 places.
			#define TPRF_CMDL_EOF			L"  >>>>>>>CmdlEof"		// - 1 place.
			#define TPRF_CMDL_KEY			L"  >>>>>>>CmdlKey"		// - 1 place (Key param was handled by cmdl_cbk).

	// Single ntf model.
	sctr_cmdl_ant_key		= 0x00800000,
			#define TPRF_CMDL_ANTKEY		L"  >>>>>AntEnvKey"		// - 3 places (all in CmdlProcessAnalysisToolKey).
};

//
// Members of this enum are specializers for the mscn_err_misc_out_of_memory error code.
//
enum TMidLevOutOfMemLocation
{
	mloom_loc_none,

	mloom_incl_hlp_loc1,
	mloom_incl_hlp_loc2,
	mloom_incl_hlp_loc3,
	mloom_incl_hlp_loc4,
	mloom_incl_hlp_loc5,

	mloom_create_finfo_loc1,
	mloom_create_finfo_loc2,

	mloom_reg_inx_key_loc1,

	mloom_serve_scgm_loc0,
	mloom_serve_scgm_loc1,
	mloom_serve_scgm_loc2,
	mloom_serve_scgm_loc3,

	mloom_ld_an_tool_profile1,
	mloom_ld_builtin_macro_loc1,

	mloom_create_dsubst_loc1,
	mloom_accept_base_dir,
	mloom_create_abs_loc1,
	mloom_create_abs_loc2,
	mloom_drv_subst_loc1,
	mloom_drv_subst_loc2,

	mloom_convert_to_text,
};

// ------------------------------------------------------------------
//   ==(2)=====  Internal list items and hash tables  =================
// ------------------------------------------------------------------

// Drive substitutions are kept in a simple list. They are not hashed in any way.
class TDriveSubstItem : public TListItem
{
protected:

	TDriveSubstItem(const wchar_t *intrnl_prefix, const wchar_t *extrnl_pref) : fname_prefix((wchar_t*)intrnl_prefix), extern_subst((wchar_t*)extrnl_pref)
								{ prefix_len = (int)wcslen(fname_prefix); subst_len = (int)wcslen(extern_subst); cnt_use = 0; }

	wchar_t		*fname_prefix;
	wchar_t		*extern_subst;
					// The prefix and subst are stored exactly as they were passed by the application.
					// No verification is done on them except for checking that they are not empty.

	int			prefix_len;
	int			subst_len;

	ID			drive_subst_id;		// Id of the event that registered this substitution in the mid_lev database.
	ULONG		cnt_use;

	friend class TMidLevScanner;
};

// Include paths are kept in a simple list. They are not hashed in any way.
class TIncludePathItem : public TListItem
{
protected:

	TIncludePathItem(wchar_t *prm_inc_path, wchar_t *prm_inc_dir) : inc_path(prm_inc_path), inc_directory(prm_inc_dir) { inc_path_id = 0; cnt_use = 0; }

	wchar_t		*inc_path;			// This is the path exactly as it was added by the upper layer.
	wchar_t		*inc_directory;		// An absolute path. It is known that inc path has successfully passed conversion into
									// the absolute path according to the current options and value of the base directory.
									// This is not the substed path. Directory name has no backslash at the end.
	ID			inc_path_id;
	ULONG		cnt_use;

	friend class TMidLevIncludeHelper;
	friend class TMidLevScanner;
};

class TIntermFileBody;

//
// Information about the disk or temp file in the cache of the mid lev parser. Note that this object owns all its resources.
//
class TFileInfoItem : public THashTableItem<TFileInfoItem>
{
protected:

	TFileInfoItem(TFileInfo &info) { data = info; loads_count = 0; pragma_once_noticed = FALSE; interm_file_body = NULL; }
			// Object takes ownership over the file name and body of the file. Application should take this
			// into account.

	TFileInfoItem(const wchar_t *fname) { data.info.file_name = (wchar_t*)fname; data.info.file_body = NULL; data.lines_info = NULL; interm_file_body = NULL; }
			// Ctor for creating the dummy search objects. This object is not functional. Fields are prepared
			// just to satisfy the dector. After using the object app should clear the data.info.file_name field.

	virtual ~TFileInfoItem();
			// Note that file name and body of the file either belong to data.info or, if the file is strg_interm,
			// they belong to the interm_file_body.
public:

		//
		// These functions are required for the hash table to work.
		//
	virtual  DWORD	GetHashValue() { return(CalcStrHash(data.info.file_name)); }
	virtual  bool		IsEqualTo(const TFileInfoItem &otherInst) { return(wcscmp(data.info.file_name, otherInst.data.info.file_name) == 0); }

public:

	TFileInfo			data;
	int				loads_count;				// This field can be used to determine if the file was used during some parsing or not.
	bool				pragma_once_noticed;

	TIntermFileBody	*interm_file_body;
						// All strg_interm files have this pointer not equal to NULL. Files of all other storage types
						// have NULL in this field. It is not good to use derived class to store this info because app
						// can use its own derived class to store info about the files.

	friend class TMidLevScanCbkHandler;
	friend class TFileInfoItemsList;
	friend class TMidLevScanner;
	friend class TParsingDriver;
};

//
//  This is table of the source files. Its elements are hashed by the name of the file. This also means
//  that this table cannot contain two different files with the same full name.
//
class TFileInfoItemsList : public THashTable<TFileInfoItemsList, TFileInfoItem, 0x100>
{
public:

	TFileInfoItem *LookupFileInfoItem(const wchar_t *fname)
	{
		TFileInfoItem searchItem(fname);
		TFileInfoItem *item = FindRecord(&searchItem);
		searchItem.data.info.file_name = NULL;
		return(item);
	}

	TFileInfoItem *LookupFileInfoItem(ID file_id)
	{
		for (StartIteration(); CheckIteration(); StepIteration())
		{
			if (CurrIteratedItem()->data.file_id == file_id)
				return(CurrIteratedItem());
		}

		return(NULL);
	}

	void ResetSessionStts()
	{
		for (StartIteration(); CheckIteration(); StepIteration())
		{
			CurrIteratedItem()->loads_count = 0;
			CurrIteratedItem()->pragma_once_noticed = FALSE;
		}
	}
};

class TIntermFileBody
{
public:

	TIntermFileBody(TFileInfoItem *owner, PrEventHeader *creation_ctx);
			// Note that this object has no destructor because all its data members have their own destructors.

	void  RefreshFileIdent();
	void  SetLineBegStt(bool val) { line_beg_stt = val;  }
	void  SetWriteComplete() { wrt_complete = TRUE; }

	PrEventHeader	&GetEventHeader() { UpdateFileBody(); return(event_hdr); }
	long				GetPureAreaBeg() { return(pure_area_beg); }
	long				DataLength() { return(data.NumItems()); }

	bool AddToBuffer(const wchar_t *data, long data_len)
	{
		return(AddToBufferInternal((wchar_t*)data, NULL, 0, data_len, FALSE));
	}

	bool AddToBuffer(TFileInfoItem *esc_src, TFileInfo *data_src, PrEventHeader &area)
	{
		bool *esc_ptr = (esc_src != NULL && esc_src->data.strg_type == strg_interm) ? esc_src->interm_file_body->GetEscapesMask() : NULL;
		return(AddToBufferInternal(data_src->info.file_body, esc_ptr, area.src_area.area_beg, area.src_area.area_len, FALSE));
	}

	bool AddToBuffer(TFileInfoItem *esc_src, TFileInfo *data_src, long data_offs, long data_len, bool orig_data)
	{
		bool *esc_ptr = (esc_src != NULL && esc_src->data.strg_type == strg_interm) ? esc_src->interm_file_body->GetEscapesMask() : NULL;
		return(AddToBufferInternal(data_src->info.file_body, esc_ptr, data_offs, data_len, orig_data));
	}

	bool		EscapeQuotes(long off_beg, long area_len);
				// Return value is FALSE in the OOM condition.

	void		Truncate(long new_data_len, bool set_rdn_complete = FALSE);
				// These 2 methods also change the state of the object that allow external tracking
				// of the processing stage.

	void		RevertLatestTruncation();
	bool		*GetEscapesMask() { return(escaped_chr.DataPtr()); }

protected:

	void		UpdateFileBody();
	bool		AddToBufferInternal(wchar_t *data_ptr, bool *data_esc_ptr, long data_offs, long data_len, bool orig_data);

	typedef TStructsArray<bool, 256, 1024>	TEscapedCharsArray;

protected:

	TFileInfoItem			*fd;
	wchar_t				fname[24];

	TTextBuffer80			data;
	long					pure_area_beg;			// Body of the file consists of 2 parts. First part belongs to the file "permanently",
												// while the second part belongs only "temporarily". Later on Truncate() method
												// will set the real length of the body somewhere in the middle of its temp part.
	bool					line_beg_stt;			// This field shows whether the beginning of the intermediate file corresponds
												// to the beginning of the line or not.
	TEscapedCharsArray	escaped_chr;

	bool					wrt_complete;
	bool					rdn_complete;

	PrEventHeader		event_hdr;				// This is parsing position that was taking place when the body builder was created.
												// It is passed into the ctor and it will be used when the temp file will be registered
												// with the callback.
	ID					latest_pbuff_exit;		// This field contains the latest event id of the event that has closed the buffering.

private:

	int					saved_body_len;		// This data field is used for reverting the latest truncation.

	friend class TMidLevScanner;
};

class TSelfChangeMacroGenHistItem : public THashTableItem<TSelfChangeMacroGenHistItem>
{
public:

	TSelfChangeMacroGenHistItem(TFileInfo &fi) { ident_area.file_info = scan_area.file_info = &fi; ident_area.area_beg = scan_area.area_beg = 0; }
	TSelfChangeMacroGenHistItem(TSourceArea &ia, TSourceArea &sa) { ident_area = ia; scan_area = sa; }

		//
		// These functions are required for the hash table to work.
		//

	virtual DWORD GetHashValue()
	{
		wchar_t *data = ident_area.file_info->info.file_body+ident_area.area_beg;
		return(CalcStrHash(data, ident_area.area_len));
	}

	virtual bool IsEqualTo(const TSelfChangeMacroGenHistItem &otherInst)
	{
		if (ident_area.area_len != otherInst.ident_area.area_len)
			return(FALSE);

		return(wcsncmp(ident_area.AreaBegPtr(), otherInst.ident_area.AreaBegPtr(), ident_area.area_len) == 0);
	}

	TSourceArea			ident_area;				// This area is used to compare the hash table items.
	TSourceArea			scan_area;
};

//
// Hash table of infos that were genererated during the calls to self changing macros.
//
class TSelfChangeMacroGenHist : public THashTable<TSelfChangeMacroGenHist, TSelfChangeMacroGenHistItem, 0x20>
{
public:

	TSelfChangeMacroGenHist() { file_info = NULL; dwData = 0; }

	TFileInfoItem			*file_info;				// Instance of the object is not owning its file.
	DWORD				dwData;					// This field is used only in the __COUNTER__ macro.
};

// Members of this enum are used as flags in the use_flags field of the TKeyInfoItem struct.
enum TMidLevIndexingKeyUses
{
	mlku_define			= 0x01,
	mlku_undef			= 0x02,
	mlku_mcall			= 0x04,

	mlku_cprm_const	= 0x10,
	mlku_cprm_value	= 0x20,
	mlku_cprm_defstt	= 0x40,

	mlku_cprm_any		= 0x70,
};

// Information about the indexing key in the cache of the parser.
class TKeyInfoItem : public THashTableItem<TKeyInfoItem>
{
protected:

	TKeyInfoItem(TKeyType type, const wchar_t *name) { assert(name != NULL && name[0] != 0); data.Clear(); data.key_type = type; data.key_name = (wchar_t*)name; key_name_len = (int)wcslen(name); use_cnt = 0; use_flags = 0; }
			// The key name is expected to be NULL terminated. Object is NOT taking ownership over the key name.

	TKeyInfoItem(TStringPtr &name) { data.key_name = name.GetBodyPtr(); key_name_len = name.GetLength(); }
			// This is special ctor for creating dummy lookup items. These temporary instances contain non NULL
			// terminated names.
public:

		//
		// These functions are required for the hash table to work. Items are compared by their key_name fields.
		//
	virtual DWORD	GetHashValue() { return(CalcStrHash(data.key_name, key_name_len)); }
	virtual bool		IsEqualTo(const TKeyInfoItem &otherInst) { return(key_name_len == otherInst.key_name_len && memcmp(data.key_name, otherInst.data.key_name, key_name_len*sizeof(wchar_t)) == 0); }

	void  BumpUsage(DWORD flags) { use_cnt++; use_flags |= flags; }

public:

	TKeyInfo			data;
	int				key_name_len;				// The length is expressed in symbols, not in bytes.

	ULONG			use_cnt;
	DWORD			use_flags;
						// These fields refer to the current mid lev parsing session or to the raw indexing session.
						// Meaning of individual bits in the flag field depends on the session type. For mid lev sessions
						// these are the members of the TMidLevIndexingKeyUses enum.

	friend class TIndexingKeyInfoItemsList;
	friend class TMidLevScanCbkHandler;
	friend class TMidLevScanner;
};

class TIndexingKeyInfoItemsList : public THashTable<TIndexingKeyInfoItemsList, TKeyInfoItem, 0x200>
{
public:

	void ResetKeysUseInfo()
	{
		for (StartIteration(); CheckIteration(); StepIteration())
		{
			CurrIteratedItem()->use_cnt = 0;
			CurrIteratedItem()->use_flags = 0;
		}
	}

	int GetNumDifferentCondParams()
	{
		int cnt_cprms = 0;
		for (StartIteration(); CheckIteration(); StepIteration())
		{
			if ((CurrIteratedItem()->use_flags & mlku_cprm_any) != 0)
				cnt_cprms++;
		}

		return(cnt_cprms);
	}
};

// -----------------------------------------------------------------
//   ==(3)========= Structural Callbacks ======================
// -----------------------------------------------------------------

class TMidLevScanner;

enum TLexSourceType : BYTE
{
	srct_none			= 0x00,

	srct_cmdl			= 0x01,		// The source file is the command line. This is either a root command line that
									// is called from nowhere or this is a nested command line file.
	srct_ant_env		= 0x02,		// Analysis tool profile. On exit from this lex source the parser with try
									// to enter the file with the built in conditions if the parsing options are asking
									// for this. Analysis tool profile has the command line syntax.
	srct_file				= 0x04,		// This type of sorce is used in the #include statements and in the forced
									// include files that are loaded from the cmdl. It is not used in any other cases.
	srct_bltin_defs		= 0x05,		// File with the built in macro definitions. This file can be called only from
									// the root command line. This file has the C/C++ type syntax.
	srct_mj_src			= 0x06,		// Major source file. Major source file is called either from nowhere or
									// from the file with the command line syntax.

	srct_scgm			= 0x07,		// Body of the self changing macro. Such body is always located in the temp file.

	srct_macro			= 0x08,		// The lex source is the macro definition.
	srct_mparam		= 0x09,			// The lex source is the macro parameter.

	srct_pbuff_macro		= 0x0A,		// Macro definition in the paste buff mode.
	srct_pbuff_mparam	= 0x0B,		// Macrodef parameter in the paste buff mode.

	srct_num_types		= 0x0C,
};

#define   LEX_SRC_CMDL(x)						(((x) <= srct_ant_env) ? TRUE : FALSE)
#define   LEX_SRC_WHOLE_FILE(x)					(((x) <= srct_mj_src) ? TRUE : FALSE)
#define   LEX_SRC_GEN_FILE(x)					(((x) <= srct_scgm) ? TRUE : FALSE)
#define   LEX_SRC_GEN_MACRO(x)					(((x) >= srct_macro) ? TRUE : FALSE)
#define   LEX_SRC_GEN_PBUFF(x)					(((x) >= srct_pbuff_macro) ? TRUE : FALSE)
#define   LEX_SRC_MACRO_OR_SCGM(x)				(((x) >= srct_scgm) ? TRUE : FALSE)
#define   LEX_SRC_MACRO_OR_PBUFF_MACRO(x)		((((x) & srct_mparam) == srct_macro) ? TRUE : FALSE)
#define   LEX_SRC_MACRO_OR_MPARAM(x)			((((x) & srct_pbuff_macro) == srct_macro) ? TRUE : FALSE)
#define   LEX_SRC_PBUFF_FLAG					2

//  -0-
struct PrInitParsingInfo1
{
	ID						batch_id;
	TDateTime				parsing_date;
	wchar_t					*base_dir;					// This pointer cannot be null, but it can be an empty string.
	bool						allow_intermediate_paths;
	bool						allow_unsubsted_paths;
};

//  -0-
struct PrInitParsingInfo2
{
	TSourceAreaInfo			cmdl_origin;					// This field defines location of the command line in the bigger file
														// like the build log. This field can be empty.
	TFileInfoItem				*cmdl_file;					// Regardless of the value in the field above the cmd line is always
														// registered as a separate file.
	short					mj_src_pos_prm;

	TMidLevPrepOptions		*prep_phase_opts;
};

//  -0-
struct PrInitParsingInfo3
{
	THighLevelLangType		lang_type;
	TFileInfoItem				*major_src_file;
	TFileInfoItem				*obj_proto_file;
	TMidLevScannerOptions		*options;
};

//  -1-
struct PrDriveSubstRegistration
{
	PrEventHeader			hdr;
	TDriveSubstInfo			subst_info;
};

//  -2-
struct PrTempFileRegistration
{
	PrEventHeader			hdr;
	TFileInfo					file_info;
};

//  -3-
struct PrPasteBufferingChange
{
	PrEventHeader			hdr;							// Event area always has zero length.

	ID						parent_cn;					// The value of this field is non zero only for the nested paste buffers.
	ID						cn_enter;					// This field is valid for exit events only. It is zero for enter events.
	short					new_buff_level;

	TPreprocToken			buff_type;					// Only the inline preprocessor tokens are allowed in this field.

	TFileInfoItem				*buffer;
};

//  -4-
struct PrLexSourceEnter
{
	PrEventHeader			hdr;

	ID						pbuff_enter_ref;				// ID of the paste buffering enter event if the paste buffering is active
														// when lex source is being entered or zero otherwise.
	TLexSourceType			src_type;
	bool						on_doubt;
	bool						inc_fn_mode;				// This flag can be applied to the macro and paste buff sources only.
														// Scanner guarantees that if this flag is set on enter it will not be reset
														// till the lex source will be exited.
	short					new_src_level;

	union
	{
		struct					// Entering the source file or the command line.
		{
			ID					inc_path_id;
			TFileInfoItem			*include_file;			// The file that was just entered.
			PrEventHeader		mpr_token;
			PrEventHeader		stmt_fname;				// Note that area of this header may belong not to the current lex src enclosure
														// if the name of the include file is passed using a macro.
			short				pos_params_cnt;		// Count of the pos params in the command line that were already scanned
														// before entering the current lex source.
		};

		struct					// Entering the body of the self changing macro.
		{
			TFileInfoItem			*scgm_file_item;
			TSourceArea			scgm_scan_area;
		};

		struct					// Entering the macro, the macro param or the paste buff source.
		{
			TMacroDefinition		*macro;
			bool					xpnd_macros;
			bool					paste_prefix;
			bool					paste_suffix;
			short				param_inx;				// Only for macro param invocations.
			TPreprocToken		param_decor;
			PrEventHeader		name_area;				// Area with the name of the macro or an area with the mparam definition.
														// For mparam this is very important field that tells location of the passed data.

			bool					append_tail;				// The field is valid only for the paste buff lex sources.
			TFileInfoItem			*paste_buff;				// The field is valid only for the paste buff lex sources.
		};
	};
};

//  -5-
struct PrLexSourceExit
{
	PrEventHeader			hdr;

	ID						pbuff_exit_ref;				// ID of the paste buffering enter event if the paste buffering is active or zero
														// if there is no active paste buffering.
	long						linear_pos_after_return;		// This field is very important for exiting from the suffix based paste buffers.
														// In all other cases the value of this field is equal to the linear offset of
														// the event itself plus one. When exiting from paste buffers, this offset shows
														// how many characters were consumed from the upper lex source.
	long						src_offs_after_return;

	TLexSourceType			src_type;
	short					new_src_level;

	union
	{
		// This group of fields is relevant for all whole file types of the src enclosure.
		struct
		{
			short			pos_params_count;			// Count of the pos params that were already scanned before exiting
														// the current lex source.
		};

		// This group of fields is relevant for exits from the paste buffers.
		struct
		{
			ID				pbuff_origin_exit_evid;		// This field contains an id of the paste buffering exit event that generated
														// the current lex source. This field allows updating the paste buffering change
														// events with the length of the scanned area that became clear only now.
			long				pbuff_final_length;			// This field shows the length of the paste buffer when it was exited. This value
														// is identical to the length of the intermediate file when it will be stored.
		};
	};
};

//  -6-
struct PrMacroLibraryChange
{
	PrEventHeader			hdr;
	TMacroDefinition			*macro;
	PrEventHeader			mpr_token;
	PrEventHeader			name_area;
};

//  -7-
struct PrCondParsingSttChange
{
	PrEventHeader			hdr;

	ID						prev_stmt_id;

	TPreprocToken			stmt_type;
	TCondExprSmplResult		expr_smpl_res;
	TConditionalStatus			expr_res;

	TConditionalStatus			stmt_code_stt;
	TConditionalStatus			new_code_stt;
	short					stmt_code_level;
	short					new_code_level;

	ID						scan_expr_error;			// Id of the first error event during the conditional expression parsing.
	TCondExprScanningDetails	scan_expr_details;
	TCondExprTreeItem		*expr_code;					// This is an unsimplified expression. Application layer can take this
														// object and put NULL into the field if it wants to use this expression.
	PrEventHeader			mpr_token;
	PrEventHeader			expr_area;
};

enum TMiscEventEx
{
	exmis_none,

	exmis_app_name,				// This event is generated by the command line parser when it processes the app name.
									// Parsing event does not contain any details besides the name of the app itself.
	exmis_mjsrc_area,				// Command line scanner found the name of the major src file but its entering was delayed.
									// When the mj src is entered immediately, this event is not generated.
	exmis_blocked_stmt,				// This is a whole line preprocessor statement that was blocked by the parsing
									// options. The stamenent was ignored.
	exmis_failed_include,			// All #include stmts that have not resulted in the lex src enter event. Note that #includes that
									// are disabled with the parsing options are not leaving any trace. In all other cases, like syntax
									// error in the statement, passive code, file not found, etc, either a lex source enter event or
									// a misc preproc event with this subtype is generated. Once again. This event is generated
									// from the failed #includes, not from any other type of failed lex src call.
	exmis_pragma,
	exmis_error,
	exmis_import,
	exmis_line,
	exmis_using,
		// Events with these subtypes are generated by enabled statements (disab_misc_prepr_stmts)
		// that stay in the active code. Othervise parsing events are not generated.

	exmis_num_types,
};

//
// Note: When the #include statements are disabled in the scanning options, the scanner is not generating anything
// while processing them. This is why the "statement disabled" option is not present in the enum below that describes
// various non call reasons.
//
enum THiddenIncudeStmtDtls
{
	pinc_none,

	pinc_syntax_error		= 1,		// It was not possible to scan the file name.
	pinc_pasive_code		= 2,
	pinc_file_not_found		= 3,		// This enum member may also mean a fatal problem while building the path.
									// In this case the search for the file is not even started.
	pinc_file_loading_err	= 4,
	pinc_pragma_once		= 5,		// The file_id field is valid.
	pinc_nesting_deep		= 6,		// The file_id field is valid.

	pinc_num_types			= 7,
};

enum TPragmaType
{
	pragma_synt_error,				// Syntax error in the #pragma stmt.
	pragma_type_other,
	pragma_type_note,
	pragma_type_once,
};

enum TErrorStmtDtls
{
	error_stmt_cont			= 0,		// The scanning was continued because the option ignore_error_stmt required continuation
									// of the parsing. Note that the #error statements, that were disabled in the parsing options
									// (disab_misc_prepr_stmts) and statements from the passive code are not leaving any trace.
	error_stmt_abort		= 1,		// The scanning was aborted.

	error_stmt_num_types,
};

enum TMiscStmtDtls
{
	mis_dtls_synt_error,
	mis_dtls_ok,
};

//
//  -8-
//
//  Fields validity for different misc stmt types:
//
//    exmis_app_name:		code_stt,				   evt_str_data;
//    exmis_mjsrc_area:	code_stt,								file_info;
//	exmis_blocked_stmt:	code_stt;
//
//    exmis_failed_include:	code_stt,		evt_dtls,					file_info;
//
//    exmis_pragma:		code_stt,		evt_dtls,	   evt_str_data;
//    exmis_error:			code_stt,		evt_dtls,	   evt_str_data;
//    exmis_import:		code_stt,		evt_dtls,					file_info;
//    exmis_line:			code_stt,		evt_dtls,		  line_info,	file_info;
//    exmis_using:			code_stt,		evt_dtls,					file_info;
//
struct PrMiscPreprocStmtEvent
{
	PrEventHeader			hdr;

	TMiscEventEx				ex_event_type;
	PrEventHeader			mpr_token;
	TConditionalStatus			code_stt;

	DWORD					evt_details;
	wchar_t					*evt_str_data;

	long						line_num;
	PrEventHeader			line_area;

	TLongId					doc_ident;					// This can be only a disk file except for the exmis_failed_include
														// and the exmis_mjsrc_area. The value of the field can also be zero.
	TStringPtr				fname_data;
	PrEventHeader			fname_area;

	void Prepare(ID cn, TMiscEventEx stmt_type, TConditionalStatus stmt_code_stt, PrEventHeader &token_origin, PrEventHeader &last_lex_origin);
			// This method fills ALL fields from hdr to evt_details including it. Field starting from evt_str_data are zeroed.

	void PrepareFileNameSection(TLexema &file_name_lex, TMidLevIncludeHelper &hlpr, TFileInfoItem *file_item = NULL);
	void PrepareFileNameSection(TFileInfoItem *file_item, PrEventHeader &fname_lex_origin);
			// These methods fill in the last three fileds.

	static const wchar_t	*GetMiscEventExEnumName(TMiscEventEx val);

	static const wchar_t	*GetNonCallReasonName(THiddenIncudeStmtDtls val);
								// This method returns name of the reason why the #include file was not entered.
};

//  -9-
struct PrNewIncPathEvent
{
	PrEventHeader			hdr;

	wchar_t					*inc_path;					// This field can be used only from inside the notification function.
	wchar_t					*inc_directory;				// This field can be used only from inside the notification function.
};

//
//  -10-	Syntax error or warning or message info.
//
struct PrErrorWarningEvent
{
	PrEventHeader			hdr;

	ID						parent_event;				// Id of the parent event is not zero in dependent messages. For example
														// the main message is saying that the name is ambiguous while dependent
														// messages list all encountered ambiguities.
	TLexErrorClass			err_class;
	int						err_code;
	wchar_t					*err_msg;					// Callback handler should not expect validity of this pointer
														// after exiting from the notification function.
	PrEventHeader			ext_context;
								// Error can be recognized in one context while the cause of the error can be in other context.
								// If these contexts are identical, location of the error is simply placed into the event header and
								// ext_context is filled with zeroes. If error is recognized in a different context, header is filled
								// with the current parsing position (zero length area) and ext context contains location of
								// the cause of this error.
};

//
//  -11-	Information about the conditional compilation parameter. Code item can have any type
//		except for the "value" and "operation".
//
struct PrCondParamUseEvent
{
	PrEventHeader			hdr;

	ID						cond_stmt_cn;

	short					lex_src_nest;				// Lex source nesting level of the cond parsing statement.
	short					cond_pars_nest;				// Cond pars nesting of the statement. Counting starts from 0.
	TConditionalStatus			code_stt;

	TCondExprTreeItem		*param;						// Application should call Clone() if it wants to keep the code tree.
};

enum TGrammarSectionType
{
	grsc_none,
	grsc_descr,
	grsc_revision,
	grsc_langtype,
	grsc_symbols,
	grsc_ignore,				// Lex types to ignore.
	grsc_errors,				// Prohibited lexema types.
	grsc_axioma,
	grsc_rules,
	grsc_conflicts,
	grsc_locations,			// Nested grammars only.
	grsc_resolution,			// Nested grammars only.
	grsc_num_types,
};

const wchar_t			*GetGrammarDefnSectionTypeName(TGrammarSectionType val);
TGrammarSectionType	GetGrammarDefnSectionType(TStrPtrInfo &str_ptr);
						// If the passed string does not match any section name, the return value is grsc_none.

//
// -12-
// This event can happen only when the grammar definition is parsed.
//
struct PrGrammarDefnSection
{
	PrEventHeader			hdr;

	TGrammarSectionType		section_type;
};

//
//  -13-	This event can happen only when the grammar definition is parsed.
//		Look for the meaning of the fields in the comments for CtvGrammarDefnObject structure.
//
struct PrGrammarDefnObject
{
	PrEventHeader			hdr;

	WORD					object_sym;
	WORD					object_app_id;

	PrEventHeader			extra_context_area;
								// This field is valid only for events that describe the rule intro symbols. It is not used
								// for other grammar objects. This header contains information about the non terminal
								// of this grammar rule.
};

struct TCppSourceAreaHeader
{
	TCppSourceAreaType			m_atype;
	PrEventHeader				m_header;

};

typedef TStructsArray<TCppSourceAreaHeader, 8, 16> TCppSourceAreaHeadersArray;
			// Areas in this array may nest and overlap. This is an expected behavior.

//
//  -14-	Cpp language object reference. This structure does not contain a pointer to the cpp database
//		object to allow using the mid lev scanner in environments that do not want to include definitions
//		of the cpp parser infrastructure.
//
struct PrCppItemReferenceEvent
{
	PrEventHeader				hdr;						// Area in the header describes the area of the whole cpp object. This area does
														// not have an area type and it is typically painted using the transparent bkgr.
	TCppItemType				cpp_item_type;
	short						cpp_item_nesting;
	ID							cpp_item_id;			// Id of the cpp object in the database of the current parsing session.

	TCppSourceAreaHeadersArray	event_related_areas;
};

struct PrRawIndexingFileInfo
{
	TIdStruct					doc_id;
	TDateTime				parsing_date;

	ULONG					keys_normal;
	ULONG					keys_string;
	ULONG					keys_comment;
};

//
//  This enum defines controls on the scanner console. The ids of these controls should be strictly
//  consecutive according to the order of definitions in this enum.
//
enum TScannerConsoleLabelIndex
{
	sclb_parsing_id,

	sclb_major_file_name,
	sclb_mid_lev_lexemas,
	sclb_mid_lev_events,
	sclb_num_errors,
	sclb_num_warnings,
	sclb_elapsed_time,

	sclb_run_time_errors,

	sclb_batch_id,
	sclb_batch_files_processed,
	sclb_batch_dirs_processed,
	sclb_batch_num_errors,
	sclb_batch_num_warns,
	sclb_batch_elapsed_time,
};

class TMidLevScanCmdlCbkHandler
{
public:

	TMidLevScanCmdlCbkHandler() { m_scanner = NULL; }

	void		SetupScanner(TMidLevScanner *scanner);			// This method cannot be implemented in the class header.

	virtual void	OnStart() { }
	virtual bool	OnKeyParam(TCmdlLexema &key_lex) { return(FALSE); }
	virtual bool	OnPosParam(TCmdlLexema &pos_lex) { return(FALSE); }
					// Return values show if handler has processed the request or not. If the request was not processed,
					// the stnd processing will happen. When the callback handler encounters a critical error or it wants to
					// abort the parsing for any other reason, it should call AbortParsing() in the m_scanner object.
protected:

	TMidLevScanner		*m_scanner;

		//
		// The cmdl callback handler does not have direct access to the parsing console. In case it would
		// be needed, the derived class should access console using the m_scanner->Console() method.
		//

	friend class TMidLevScanner;
};

//
//  Note that all virtual methods in this base callback handler have default implementation.
//  This allows applications to overwrite only some of the methods on the "by need" basis.
//
class TMidLevScanCbkHandler
{
public:

	TMidLevScanCbkHandler(TGenericConsoleEx *console = NULL)
	{
		m_console = console;
		m_scanner = NULL;

		m_curr_batch_id = m_curr_parsing_id = 0;
		m_raw_indexing_sess = FALSE;

		m_pseudo_batch_ids_count = m_pseudo_parsing_ids_count = 0;
		m_pseudo_file_ids_count = TFileInfo::DiskFileIdsBase-1;
		m_pseudo_key_ids_count = 420;
	}

	virtual ~TMidLevScanCbkHandler() { }

	void		SetupConsole(TGenericConsoleEx *console) { m_console = console; }
	void		SetupScanner(TMidLevScanner *scanner);			// This method cannot be implemented in the class header.

	virtual bool	AllocateBatchId(ID &batch_id) { batch_id = ++m_pseudo_batch_ids_count; return(TRUE); }

	virtual TFileInfoItem	*CreateFileInfoItem(TFileInfo &finfo) { return new TFileInfoItem(finfo); }
	virtual TKeyInfoItem	*CreateKeyInfoItem(TKeyType type, const wchar_t *name) { return new TKeyInfoItem(type, name); }
	virtual TMacroDefinition	*CreateMacroDefinition(TStrPtrInfo &name, TMacroDefStatus stts, TMacroDefOrigin nature) { return new TMacroDefinition(name, stts, nature); }
							//
							//  These methods above are pure fabricas of classes. These methods should not check the uniqueness
							//  of the file name, key name, macro name or any other params.
							//
							//  Callbacks should not add the created objects to any internal lists and/or assign any IDs to them
							//
							//  Returned objects should be disposable using the delete operator. Caller may delete the returned object
							//  at any time without notifying the callback handler.
							//
							//  These callbacks allow creating objects that return application specific derived classes that may provide
							//  additional functionality or collect additional statistics. This is not a public API. Caller is responsibile of
							//  freeing the returned object. Callback implementations should not report memory errors themselves.
							//  These methods will be called only when parsing session or raw indexing session is active.
							//

	virtual bool	LookupOrRegisterDiskFileInfo(TFileInfo &info) { if (info.file_id == 0) info.file_id = ++m_pseudo_file_ids_count; return(TRUE); }
					//
					//  This callback is used when the new file comes into the scope. Callback handler has to assign the unique file_ident.
					//  Derived class should return FALSE in case of failure. In case of success method should not return 0 in the file_ident
					//  field. If database has file with the same name, but with different date/length, this file should be treated as different
					//  file. If current session is a midlev session, this call should automatically add Id of the current session id to the list
					//  of references of this file.
					//

	virtual bool	LookupOrRegisterIndexingKey(TKeyInfoItem &info) { info.data.key_id = ++m_pseudo_key_ids_count; return(TRUE); }
					//
					//  This call is not setting any reference or usage info. Such information will be reported to the database in a separate call.
					//  In case of success the callback MUST fill in existing Id or assign new Id to the passed key info item.
					//

	virtual bool	StartRawIndexingSession(PrInitParsingInfo1 &init_info)
								{ assert(m_curr_parsing_id == 0 && m_raw_indexing_sess == FALSE); m_curr_batch_id = init_info.batch_id; m_raw_indexing_sess = TRUE; return(TRUE); }
	virtual void	FinishRawIndexingSession(TRawIndexingSessSummary &sumry, TFileInfoItem *mj_src_file = NULL)
								{ assert(m_raw_indexing_sess == TRUE); m_curr_batch_id = 0; m_raw_indexing_sess = FALSE; }
					// It is allowed to index several disk/temp files in the same raw indexing session. The raw indexing session
					// cannot be aborted.

	virtual bool	StartParsing(PrInitParsingInfo1 &init_info) { assert(m_curr_parsing_id == 0 && m_raw_indexing_sess == FALSE); m_curr_batch_id = init_info.batch_id; m_curr_parsing_id = ++m_pseudo_parsing_ids_count; return(TRUE); }
	virtual bool	StartPrepPhase(PrInitParsingInfo2 &init_info) { return(TRUE); }
	virtual bool	StartMainPhase(PrInitParsingInfo3 &init_info) { return(TRUE); }
	virtual void	FinalizeParsing(TMidLevPrepOptions *prep_options, TMidLevScannerOptions *scan_options,
								TMidLevParsingSummary &summary, int final_pars_status) { assert(m_curr_parsing_id != 0); m_curr_batch_id = m_curr_parsing_id = 0; }
					// This group of methods should be called only by the MidLev scanner from inside its corresponding methods.
					// These methods are not public API. Options are sent to the closing method once again because they can be
					// changed by when the env file it is processed. StartPrepPhase() and StartMainPhase() can be called only
					// when the session is already opened with the StartParsing() method.

	virtual ID		GetParsingId() const { return(m_curr_parsing_id); }
	virtual bool	IsRawIndexingSessionActive() const { return(m_raw_indexing_sess); }
	virtual bool	IsIdle() const { return(m_curr_parsing_id == 0 && m_raw_indexing_sess == FALSE); }
					// Methods can be called at any time to determine the current state of the callback handler.

	virtual void	RegisterDriveSubstData(PrDriveSubstRegistration *info) { }
	virtual void	RegisterTempFileData(PrTempFileRegistration *info) { }
					// Ident of the temp file is assigned by the caller. This id is basically the pars_id.event_id of the registration call.

	virtual void	PasteBuffChangeNotification(PrPasteBufferingChange *info) { }
	virtual void	LexSourceEnterNotification(PrLexSourceEnter *info) { }
	virtual void	LexSourceExitNotification(PrLexSourceExit *info) { }
	virtual void	MacroLibraryChangeNotification(PrMacroLibraryChange *info) { }
	virtual void	CondParsSttChangeNotification(PrCondParsingSttChange *info) { }
	virtual void	MiscPreprocEventNotification(PrMiscPreprocStmtEvent *info) { }
	virtual void	NewIncludePathNotification(PrNewIncPathEvent *info) { }
	virtual void	ParsingErrorNotification(PrErrorWarningEvent *info) { }
	virtual void	CondParamUseNotification(PrCondParamUseEvent *info) { }

	virtual void	GrammarSectionNotification(PrGrammarDefnSection *info) { }
	virtual void	GrammarObjectNotification(PrGrammarDefnObject *info) { }
					// These events can be fired only when the grammar definition is scanned.

	virtual void	CppItemNotification(PrCppItemReferenceEvent *info) { }
					// This is a C/C++ parser specific notification. Purpose of this notication is to establish a link between
					// the location in the source code and the object in the cpp database. This also means that the type of
					// the Cpp object in the passed event struct cannot be emty and its item id cannot be zero.

	virtual void	MidLevLexemaNotification(TLexema *lex) { }
					// This method is not used in the raw scanning sessions and in the regular high level language sessions.
					// It is used in the custom analysises that use the generic parser.

	virtual bool	SetParsSessIndexingKeyUseStatus(TKeyInfoItem *info) { return(TRUE); }
	virtual bool	SetRawSessIndexingKeyUseStatus(TLongId doc_id, TKeyInfoItem *info) { return(TRUE); }
					// Methods can either set or remove the back reference between key_id and file or parsing session.
					// This behavior depends on use_cnt field of the TKeyInfoItem parameter.

	virtual bool	SetRawIndexingData(PrRawIndexingFileInfo &hdr_info, BYTE *inx_data, long inx_data_len) { return(TRUE); }
					// First param contains doc_id of the file that was indexed. When this method is called all back refs
					// to this file should be already successfully set.

	void			ShowProcessingError(const wchar_t *format, ...);

	virtual TMidLevStoreReader *AllocateReaderObject(wchar_t *buffer_4_max_path = NULL) { if (buffer_4_max_path != NULL) wcscpy(buffer_4_max_path, L"TMidLevScanCbkHandler does not support allocation of the reader objects."); return(NULL); }
	virtual void	DisposeReaderObject(TMidLevStoreReader *reader) { if (reader != NULL) { assert(FALSE); } }		// Non NULL readers shoud not be disposed in the base class.

protected:

	TGenericConsoleEx		*m_console;
	TMidLevScanner		*m_scanner;

	ID					m_curr_batch_id;			// This field simply stores value of the batch id. It does not indicate
													// anything.
	ID					m_curr_parsing_id;			// The id is not zero when the parsing session is active. This fiels can be used
													// to check if the parsing session is active or not.
	bool					m_raw_indexing_sess;		// The value is TRUE when the raw indexing session is active. This fiels can be
													// used to check the state of the raw indexing session.
							//
							// The parsing session and the raw indexing session cannot be active at the same time.
							//
private:

	ID				m_pseudo_batch_ids_count;
							// Base class keeps id of the batch that was recently given out here.

	ID				m_pseudo_parsing_ids_count;
							// Base class keeps Id  of the current parsing in this field. It can be also id of
							// the parsing that has just finished.

	ID				m_pseudo_file_ids_count;
							// Counter for assigning the source file ids when derived class is not using any
							// other advanced scheme for assigning the file ids.

	ID				m_pseudo_key_ids_count;
							// Indexing keys have parsing independent values. They are used both in mid lev
							// and in the raw parsing sessions.

	friend class TMidLevScanner;
	friend class TMidLevScanCbkHandlerProxy;
};

// ------------------------------------------------------------------
//   ==(4)========== MidLevScanner infrastructure =================
// ------------------------------------------------------------------

class TMidLevIncludeHelper
{
public:

	TMidLevIncludeHelper(TMidLevScanner *owner, bool load_if_found, bool rprt_cdir_loads, bool rprt_inc_dupls)
					: scanner(*owner) { inc_file_item = NULL; err_ctx = NULL; load_file_flag = load_if_found; report_curr_dir_loads = rprt_cdir_loads; report_dupls = rprt_inc_dupls; }

	TFileInfoItem  *LoadIncludeFile(TLexema &fname_lex);

	TLoadSaveResult	load_res;
						// If value of this field is "ldres_fname_missing", this means that file was not found at all.
						// If the file was found, then reagrdless of any other problem, the value will be different.
						// Return value from the main function can be NULL while load_res can be still "ldres_success"
						// either because the helper in the "locate_file_only" mode or due to the failure while
						// registering the file with the mid lev callback handler.

	TFileNameBuffer	main_location;
						// This is an absolute unsubsted name of the file at the place, where it was found first.
						// Value in the "load_res" data field corresponds to this location.

	ID				inc_path_ident;
						// The ID of the include path that was used to load the file. This id can be 0 if the statement contained
						// either an absolute or a semi relative path of if the file was loaded from the direcory of the current file.

protected:

	void CheckRelativeLocations(const wchar_t *fname_from_stmt, bool load_file_pass);
			// This method may emit:
			//    mscn_err_ihlp_make_fname2.

	void CheckAbsoluteLocation(const wchar_t *abs_file_name, TIncludePathItem *path_item, bool load_file_pass);
			// This method may emit:
			//    none, except for errors from scanner.CreateSubstedPath().

	void CheckFileName(const wchar_t *abs_file_name, TIncludePathItem *path_item, const wchar_t *substed_file_name);
			// This method may emit:
			//    mscn_err_ihlp_file_load_error.

	void CheckOtherPlace(const wchar_t *abs_file_name, const wchar_t *substed_file_name);
			// This method may emit:
			//    mscn_err_ihlp_other_path_loc.

	void CheckShortName(const wchar_t *abs_file_name, const wchar_t *substed_file_name);
			// This method may emit:
			//    mscn_err_ihlp_short_name_also1, mscn_err_ihlp_short_name_also2, mscn_err_ihlp_short_name_cload.

	bool IsLocationReported(const wchar_t *file_name);

protected:

	TFileInfoItem			*inc_file_item;				// This pointer points to some file in the files cache. The class is not owning
													// this pointer.
	TTextBuffer256		locations_list;				// List of places were the warnings were already reported. Buffer contains list
													// of strings that are separated from one another with the NULL char.
	TFileNameBuffer		buffer1, buffer2;				// Simple work buffers for accumulating file name.

	PrEventHeader		*err_ctx;					// This field was added to avoid passing error ctx to internal functions.
													// Using pointer is ok here because this field is used only from inside
													// the call to only one public method of this class.
	TMidLevScanner		&scanner;
	bool					load_file_flag;				// TRUE: Find and load the file;
													// FALSE: Only look for the file and generate errs/warns, but do not load.
													// This feature is used for processing #import and #using statements.
	bool					report_curr_dir_loads;
	bool					report_dupls;
};

struct TLexemasSource
{
public:

	static const wchar_t *GetLexSourceTypeEnumName(TLexSourceType val);

protected:

	TLexSourceType		src_type;
	TMacroDefOrigin		mdefs_nature;				// The nature of the macrodefs that reside in the current source.

	PrEventHeader		ltst_call_area;				// Area of the latest lex src call.
	ID					cn_enter;
	long					initial_linear;				// This value is calculated after entering the lex source. It is not changing
													// after that.
	long					curr_linear_beg;				// Linear offset of the beginning of the file. This field is set when the lex src is
													// entered and it is updated after return from every nested lex source.
	bool					source_on_doubt;

	TRawScanner			scanner;					// This scanner is used to scan file sources, macro sources and macro param srcs.
	TCmdlRawScanner		cmdl_scanner;				// Raw scanner for all files with the command line syntax.
	TFileInfoItem			*file_context;				// Not NULL only when the lex source is either a file or a pbuff.
	TMacroDefinition		*macro_context;				// This macro defines macro params that are applicable to the current
													// context. This field can be NULL.

	int					mprm_def_level;			// Nesting level that contains definitions of the macro params that are recognized
													// on the current level.
	TPreprocToken		mparam_decor;				// For all non macro param sources this field is equal to 0.
	bool					xpnd_macros;				// Typically this flag is TRUE. It is FALSE only in "literal" substs.
	bool					paste_suff;					// This flag shows if the lex source call stmt was followed by the ## or not.

	short				src_param_inx;
	PrEventHeader		src_name_area;
							// These two fields are never checked. They are needed only to send callback
							// notification when the paste buffer is being entered.

	const TFileInfo		*GetFileInfo() { return((LEX_SRC_CMDL(src_type) == TRUE) ? cmdl_scanner.GetFileInfo() : scanner.GetFileInfo()); }
	const wchar_t		*GetFileName() { return((LEX_SRC_CMDL(src_type) == TRUE) ? cmdl_scanner.GetFileName() : scanner.GetFileName()); }

	bool			GetFirstInLine() { return(src_type >= srct_file ? scanner.GetFirstInLineState() : FALSE); }
	bool			IsInEof() const { return((LEX_SRC_CMDL(src_type) == TRUE) ? cmdl_scanner.IsEof() : scanner.IsEof()); }

	friend class TMidLevIncludeHelper;
	friend class TCondExpressionScanner;
	friend class TMidLevScanner;
	friend class TGrammarDefinitionParser;
};

//
//  There are 2 different types of buffering:
//
//	--	Suffix based. This type of buffering lasts as long as ## operator is present. Note that this
//		type of buffering can enter/exit macro or mparam calls if the call is preceeded/followed by
//		the ## operator. Suffix based buffering cannot be initiated right at the beginning of the lex
//		source and it cannot exit the lex source if there is no ## operator outside.
//	--	Eof based. This type of buffering lasts until the macro param expansion is not finished.
//		The result of buffering is framed into either single or double quotes as requested.
//
struct TPasteBufferingSlot
{
	ID					cn_enter;					// Id of event that started this buffering.
	bool					suff_buffering;				// The type of buffering.
	wchar_t				frame_char;					// This char can be zero, single or double quote.
	int					lev_buff_level;				// Lex sources nesting level of the macro param.
	TFileInfoItem			*buffer;						// This buffer can be shared with other buffering layers.
	long					beg_offs;					// Offset where buffering for this level was started. It is not used
													// for making important decisions like starting/ending the buffering.
													// It is needed for using proper delimiters and for escaping quotes.

	wchar_t   GetDelimChar() { return((buffer->interm_file_body->DataLength() == beg_offs) ? frame_char : L' '); }
};

struct TCondCompLevel
{
	ID					area_cn;
							// Change number that introduced this conditional area.

	TConditionalStatus		code_stts;
							// Current status of the code under compilation. Almost all the times it is identical to the result
							// of the expression from the latest #ifxx or #elif statement.

	TConditionalStatus		elif_hist;
							// This variable reflects the type of result that was found some time earlier. Based on this result
							// the status of the code after possible #elif and #else stmts is set. Initial state of the history
							// for every level matches the value of the initial #ifxx statement.

	bool					else_flag;
							// The #else statement was already processed on this conditional parsing level.
};

struct TCommandLineTaskInfo
{
	TSourceAreaInfo		cmdl_origin;					// The cmdl origin is known only when it is part of the build log.
	wchar_t				*cmdl_ks_chars;
	TCmdlKeysDictEntry	*cmdl_keys_dict;
	TRawScanningType		cmdl_rscn_type;				// This raw scanning type is used for nesting cmdl from
													// the main command line.
	int					mj_src_pos_param;			// Positional param index. Application name has index 0.
	TFileStorageType		mj_src_strg_type;			// In reality this can be either strg_disk or strg_temp only.
	TRawScanningType		mj_src_rscn_type;			// The raw scannig type can be also rscn_none. In this case
													// scanner will not enter the major source.
	bool					src_unicode;				// Type of major src file encoding: ASCII or unicode.
	THighLevelLangType	lang_type;					// Type of the high level grammar. This field can be also used
													// for determining the type of the latest session.

	inline void Clear() { memset(this, 0, sizeof(TCommandLineTaskInfo)); }

	void   Init(TParseEnvInfo &env, int mj_src_prm, THighLevelLangType langt);
};

struct TMajorSrcFileInfo
{
	TIdStruct				cmd_line_ident;
	TFileInfoItem			*cmd_line_info;

	bool					mj_src_prm_found;			// When this field is TRUE, this means that some mj src param was requested
													// and it was found on the command line.
	TIdStruct				mj_src_ident;
	TFileInfoItem			*mj_src_info;
	bool					mj_src_entered;				// This field is needed to prevent infinite loop on entering the major src.

	TIdStruct				obj_file_ident;
	TFileInfoItem			*obj_file_info;

	inline void Clear() { memset(this, 0, sizeof(TMajorSrcFileInfo)); }
	inline void ResetPointers() { cmd_line_info = mj_src_info = obj_file_info = NULL; }
};

// -----------------------------------------------------------------------
//   ==(5)============ MidLevScanner ============================
// -----------------------------------------------------------------------

class TMidLevScanner
{
public:

		TMidLevScanner(TMidLevScanCbkHandler *mid_lev_cbk = NULL, TMidLevScanCmdlCbkHandler *cmdl_cbk = NULL);
		~TMidLevScanner();

	//
	// Setup/maintenance.
	//

	void       SetupCbkHandler(TMidLevScanCbkHandler *cbk) { cbk_handler = cbk; if (cbk != NULL) cbk->m_scanner = this; }
					// This setup is required if the callback is not passed in the ctor. If it is missing, the scanner will AV.

	void       SetupCmdlCbkHandler(TMidLevScanCmdlCbkHandler *cbk) { cmdl_cbk_handler = cbk; if (cbk != NULL) cbk->m_scanner = this; }
					// This setup is required only if the command line processing is used.

	void       SetupTraceMode(DWORD new_val, bool print_help_header = TRUE) { trace_mask = new_val; if (print_help_header == TRUE) TraceColumnTitles(); }
					// Tracing destination is a virtual method is the parsing console. The parsing console is retrieved
					// from the MidLev cbk handler.

	void       MidLevReset(bool full_reset, bool reset_disk_files_cache = FALSE);
					// This method competely destroyes the stack of lexema sources and the stack of conditional compilation
					// if they are not empty. It frees all macro definitions, include paths, etc. Partial reset is called at the end
					// of the parsing session. It is not destroying temp files, parsing propertiess and statistics. Full reset
					// is called when parsing session starts.

	// -----------------------------

	bool       BeginParsingSession(ID batch_id, const wchar_t *parsing_base_dir = NULL,
							TDriveSubstInfo *drive_substs_array = NULL,
							bool allow_interm_paths = FALSE,
							bool allow_non_substituted_paths = FALSE);
	bool       BeginRawIndexingSession(ID batch_id, const wchar_t *parsing_base_dir = NULL,
							TDriveSubstInfo *drive_substs_array = NULL,
							bool allow_interm_paths = FALSE,
							bool allow_non_substituted_paths = FALSE);
					//
					// If the base parsing directory is NULL, then the current directory of the process is used. If the base directory is
					// passed, then it should be an absolute directory. If the array of drive substitutions is not NULL, then the base
					// directory should be passed explicitly.
					//
					// Once these methods return TRUE, the parsing session should be closed either with the CloseParsingSession()
					// or with the CloseRawIndexingSession() method.
					//

	TFileInfoItem *CreateFileInfo(PrEventHeader *src_context,
							TFileStorageType strg, TRawScanningType rscn,
							const wchar_t *fname);
					//
					//  Create file info for a file, that resides on the disk.
					//
					// Method looks in the file cache or loads the file from the disk. If loading was successful, method registers
					// file with the mid lev callback handler and adds this file into the cache. If file with this name is already
					// present in the cache, method calls assert to ensure that the raw scanning type is identical.
					// Source context is passed only for error reporting and it can be NULL.
					//
					// Note that temp file still can be loaded from disk. In this case compiler believes that if body
					// of the file would not be saved in the database, the file would be lost.
					//

	TFileInfoItem *CreateFileInfo(PrEventHeader *src_context,
							TFileStorageType strg, TRawScanningType rscn,
							const wchar_t *fname, const wchar_t *fbody,
							long file_len_in_wchars = -1, TDateTime fdate = 0);
					//
					//  Create file info for the data set that resides in memory.
					//
					// Method duplicates file name and file body into heap, and registers passed memory file with mid lev callback
					// handler. Source context is passed only for error reporting and it can be NULL.
					//
					// When this method is used for creating file item for built in file, the file_id should be passed in the date field.
					//

	TFileInfoItem	*FindCachedFile(const wchar_t *fname);
	TFileInfoItem	*FindCachedFile(ID pars_id, ID file_id);

	bool		PurgeCachedFile(const wchar_t *fname) { return(PurgeCachedFile(FindCachedFile(fname))); }
	bool		PurgeCachedFile(ID pars_id, ID file_id) { return(PurgeCachedFile(FindCachedFile(pars_id, file_id))); }
				// Param pars_id shows the type of file that is searched: a disk file (pars_id == 0), or a temp file.
				// In this case the pars_id should be equal to the id of the current session.

	void		ExtractFilesCache(TList &disk_files_to, TList &temp_files_to);
				// Note that both lists contain the TFileInfoItem elements. After extracing the list it is responcibilty of the application
				// to release items of the lists with an ordinary operator delete or simply clear these lists with the DeleteAll() method.

	// -----------------------------

	bool		BeginPrepPhase(TFileInfoItem *cmdl, TCommandLineTaskInfo &cmdl_info,
						TMidLevPrepOptions *cmdl_opts = NULL,
						TMidLevScannerOptions *main_opts_proto = NULL);
				//
				// The first param can NOT be NULL. Its raw scanning type should be one of the cmdl types. This param
				// should be created using one of the CreateFileInfo(...) methods. Directory names should be full paths.
				//
				// This method can be called only after calling the BeginParsingSession() method.
				//

	bool		BeginMainPhase(THighLevelLangType lang_type, TFileInfoItem *major_src,
						TFileInfoItem *obj_proto = NULL,
						PrEventHeader **call_origin = NULL,		// There should be either no source info at all or all 3 source areas should be present.
						TMidLevScannerOptions *opts = NULL);
				//
				// The file info for the "major_src" should be already registered. This method actually enters the passed
				// major file source. "Enters" means here adding lex source to the sources stack, so the next GetLex() call
				// will pick up something from the passed major file source.
				//
				// This method should be used only with sessions that do not have the preparation phase. Otherwise one
				// of the calls to Scan() will switch the session phase itself when the mjsrc will be entered.
				//
				// This method can be called only after calling the BeginParsingSession() method.
				//

	TLexemaType		Scan(TLexema &lex);
						//
						//	This is the main method of the class.
						//
						// Method skips empty characters and fills lexema struct with the next lexema found. Function gives
						// out the type of retieved lexema. This method never gives out an empty lexema.
						// Note: If type of the returned lexema is ltx_error, this error is already reported to the error callback.
						//

	bool		ExecRawIndexing(TFileInfoItem *cached_file_info);
	bool		ExecRawIndexingForCachedFiles();

	void		CloseParsingSession(bool reset_disk_files_cache = FALSE);
	void		CloseRawIndexingSession(bool reset_disk_files_cache = FALSE, TFileInfoItem *major_sess_file = NULL);
				// These methods can be called at any time during the scanning process. These methods also save information
				// about the indexing keys use.

	void		GetParsingSummary(TMidLevParsingSummary &buffer);
	void		GetRawIndexingSummary(TRawIndexingSessSummary &buffer);
	int		GetErrorsCount() const { return(summary.num_errors); }

	//
	//  Service and support methods. /* 1 */
	//

	inline ID  GetNextCN() { return(++latest_cn); }

	void		DecrementNextCN() { latest_cn--; }
				// Event id is generated on the client side and mid lev callback is not used for this. Because of that events
				// can be submitted to the mid lev callback not in the order of issuing CNs.

	void		AddIncludePath(TStringPtr &inc_path, PrEventHeader *key_area, PrEventHeader *path_area);
				// Return value can be FALSE only in a low on memory case.

	bool		UpdateMacroLibrary(TMacroDefinition *macro, TLexema &name_lex, PrEventHeader &token_area);
				// Method modifies the current table of macrodefinitions. Passed definition is expected to be fully prepared.
				// Ownership on the definition is bypassed to this method.

	void		ShowRunTimeError(const wchar_t *format, ...);
				// Method displays passed message on the console only when the abort flag is not set. It adds one more line
				// to the message "Do you want to continue?" and buttons "Yes" and "No". It can raise the abort flag based
				// on the answer.

	ID		AddErrorWarning(PrEventHeader *source_context, ID parent_event, TLexErrorClass err_class, int err_num, const wchar_t *error_message);
				//
				// This is primary API for reporting warnings and errors. Method is doing the following actions:
				//
				//    -- fills in the event structure;
				//    -- makes tracing, if this is requested.
				//    -- calls mid lev callback or console;
				//    -- updates statistics (summary);
				//
				// This method is a mandatory wrapper for all error/warning calls to the mid lev callback handler.
				//

	void		AbortParsing(int err_code, TLexErrorClass err_class) { if (abort_err_code == 0) { abort_err_code = err_code; abort_err_class = err_class; status_callback_cnt = 0; } }
				// Public API for aborting parsing. This method will not set the abort flag in the console. App needs
				// to do this separately if this is necessary. Abort state can be reset only with a call to CloseParsing().

	bool		CheckAbortParsingState();
				// Method checks if AbortParsing() was called by the upper layers of the software or if user wants to abort
				// parsing by clicking the "Abort" button on the console (This will result in setting the abort flag in the console).
				// If this is so, method reflects this in the data fields of the object and does other related actions.
				// Return value: FALSE - continue parsing, TRUE - abort.

	void		GetCurrScanPos(PrEventHeader &hdr, ID cn = 0);
				// Method can be called only when the lex sources are available. The "cn" field is set into the header exactly
				// as it is passed.

	long		GetCurrentLinearOffset();
				// If the saved lex is not empty, method returns its beginning. Otherwise method returns current scanning
				// position from the raw scanners.

	inline DWORD	GetCurrTraceMode()				{ return(trace_mask); }
	inline int		GetCurrLexSrcNesting()			{ return(src_cnt);  }
	inline int		GetCurrCondParsingNesting()	{ return(cond_cnt); }
	inline ID		GetCurrSourceRef()				{ return((src_cnt > 0) ? src_stack[src_cnt-1].cn_enter : 0); }
	inline ID		GetCurrCondRef()				{ return(cond_area_cn); }

	inline TConditionalStatus		GetCodeSttForLevel(int lev)	{ return((lev > 0) ? cond_stk[lev-1].code_stts : cexr_hard_true); }
	inline ID						GetCondRefForLevel(int lev)	{ return((lev > 0) ? cond_stk[lev-1].area_cn : 0); }

	TGenericConsoleEx				&Console()				{ return(*(cbk_handler->m_console)); }
	TMidLevScanCbkHandler		&CbkHandler()			{ return(*cbk_handler); }
	TMidLevScanCmdlCbkHandler	&CmdlCbkHandler()	{ return(*cmdl_cbk_handler); }
	TMajorSrcFileInfo				&MajorSrcInfo()		{ return(major_src_info); }

	TMacroDefinition	*FindMacro(TStrPtrInfo &macro_name) { return(macrodefs_list.LookupMacro(macro_name)); }

	TMacroDefinition	*CloneMacro(TMacroDefinition *proto);
						// Clone macro is method in the scanner because it is using the mid_lev callback to create macro
						// object that might have an app specific class that is derived from TMacroDefinition.

	TKeyInfoItem		*LookupIndexingKey(TStringPtr &name) { TKeyInfoItem dummy(name); return(keys_list.FindRecord(&dummy)); }
						// This method is only checking the table. It is not creating anything there.

	TKeyInfoItem		*GetIndexingKey(PrEventHeader *err_ctx, TKeyType key_type, TStringPtr &name, ID &err_ident, bool &malloc_problem);
	TKeyInfoItem		*GetIndexingKeyFromLex(TLexema &lex);
	TKeyInfoItem		*GetIndexingKeyFromLexEx(TLexema &lex, ID &err_ident, bool &malloc_problem);
						// First method for getting key info from lexema should be called only with the charconst, number
						// and name lexemas. Otherwise the method will assert. Methods look up or create indexing key object
						// in the memory and in the database. Methods are not creating any back_refs. If return value is NULL,
						// this means the error has happened and this error is already reported to the database.

	wchar_t			*StroreIntermediateString(TStringPtr &str_ptr) { return(misc_strings_dict.RegisterStr(str_ptr.GetBodyPtr(), str_ptr.GetLength())); }
						// Passed string may have NULL chars inside. Returned pointer is valid till the end of parsing.

	void		ExtractMacroDefsList(TList &extract_to) { macrodefs_list.ConvertToList(extract_to); }
	void		ExtractIndexingKeysList(TList &extract_to) { keys_list.ConvertToList(extract_to); }

	// The ranges of the error code values are specified in the TParsingErrorCodeBases enum.
	enum TMidScanErrorCode
	{
		mscn_err_base = 1020,

		// Errors while accepting the file proc params.
		mscn_err_subst_no_base_dir				= 1021,
		mscn_err_subst_interm_paths			= 1022,
		mscn_err_subst_curr_dir_err1			= 1023,			// %1 - Win32 error code (dec).
		mscn_err_subst_curr_dir_err2			= 1024,			// %1 - length of the path (dec).
		mscn_err_subst_base_not_abs			= 1025,			// %1 - base dir.

		// Errors while parsing the command line and the cmdl keys.
		mscn_err_cmdl_no_app_name			= 1028,
		mscn_err_cmdl_no_major_src			= 1029,			// %1 - zero based mj_src_prm, %2 - number of pos params.
		mscn_err_cmdl_no_prep_phase			= 1030,			// %1 - name of the curr phase.
		mscn_err_cmdl_pcnd_no_name			= 1031,
		mscn_err_cmdl_oom_for_pcnd			= 1032,
		mscn_err_cmdl_pcnd_bad_delim			= 1033,
		mscn_err_cmdl_bad_atkey_src			= 1034,
		mscn_err_cmdl_unkn_atool_key			= 1035,

		// Errors while locating and loading the include files.
		mscn_err_ihlp_abs_sys_fname			= 1040,			// %1 - type of stmt.
		mscn_err_ihlp_abs_user_fname			= 1041,			// %1 - type of stmt.
		mscn_err_ihlp_path_semi_rel			= 1042,			// This can be either warning or error.
		mscn_err_ihlp_bad_intermp				= 1043,			// %1 - error code.
		mscn_err_ihlp_long_intermp				= 1044,			// %1 - path length.
		mscn_err_ihlp_upper_sys_ref			= 1045,
		mscn_err_ihlp_file_not_found			= 1046,
		mscn_err_ihlp_make_fname1				= 1047,			// %1 - dir_name.
		mscn_err_ihlp_make_fname2				= 1048,			// %1 - file_name.
		mscn_err_ihlp_curr_dir_load				= 1049,
		mscn_err_ihlp_file_load_error			= 1050,			// %1 - error text, %2 - file_name.
		mscn_err_ihlp_other_path_loc			= 1051,			// %1 - orig_loc, %2 - other_loc.
		mscn_err_ihlp_short_name_also1			= 1052,			// %1 - orig_loc
		mscn_err_ihlp_short_name_also2			= 1053,			// %1 - orig_loc, %2 - other_loc.
		mscn_err_ihlp_short_name_cload			= 1054,			// %1 - location.

		// Errors while switching between the parsing phases.
		mscn_err_phsw_nest_src_unex			= 1057,
		mscn_err_phsw_indx_key_updt			= 1058,			// %1 - indexing key name.

		// Errors while setting the base dir and the include paths.
		mscn_err_incp_oom_for_ipath			= 1061,			// %1 - inc_path.
		mscn_err_incp_semi_rel_dir				= 1062,			// %1 - inc_path.
		mscn_err_incp_semi_rel_file				= 1063,			// %1 - inc_path.
		mscn_err_incp_append_relat				= 1064,			// %1 - inc_path, %2 - root_path.
		mscn_err_incp_interm_path_conv		= 1065,			// %1 - fname, %2 - Win32 error.
		mscn_err_incp_bad_pars_phase			= 1066,
		mscn_err_incp_dir_missing1				= 1067,			// %1 - missing_dir.
		mscn_err_incp_dir_missing2				= 1068,			// %1 - missing_dir, %2 - inc_path.
		mscn_err_incp_dupl_inc_path			= 1069,			// %1 - dupl_path.
		mscn_err_incp_dupl_inc_dir				= 1070,			// %1 - dupl_dir, %2 - path1, %3 - path2.
		mscn_err_incp_subst_missing			= 1071,			// %1 - file_name that has no subst.

		// Core scanning plus nesting the new lex sources.
		mscn_err_srcst_file_not_found1			= 1074,			// %1 - file_name.
		mscn_err_srcst_file_not_found2			= 1075,			// %1 - file_name, %2 - substed_fname.
		mscn_err_srcst_file_load_error1			= 1076,			// %1 - error text, %2 - file_name.
		mscn_err_srcst_file_load_error2			= 1077,			// %1 - error text, %2 - file_name, %2 - substed_fname.
		mscn_err_srcst_noemp_condstk			= 1078,			// %1 - number of cond pars layers.
		mscn_err_srcst_oom_for_fitem			= 1079,
		mscn_err_srcst_oom_for_ifbody			= 1080,
		mscn_err_srcst_cant_reg_file			= 1081,			// %1 - file_name.
		mscn_err_srcst_date_len_mism			= 1082,			// %1 - file_name.
		mscn_err_srcst_nesting_deep1			= 1083,			// File src: cmdl or real source file.
		mscn_err_srcst_pr_once_in_mjsrc		= 1084,
		mscn_err_srcst_nesting_deep2			= 1085,			// Macro call.
		mscn_err_srcst_nesting_deep3			= 1086,			// Macro param.
		mscn_err_srcst_nesting_deep4			= 1087,			// Paste buffer.
		mscn_err_srcst_single_hash_sym			= 1088,
		mscn_err_srcst_inln_macro_token		= 1089,
		mscn_err_srcst_stmt_macrotkn			= 1090,
		mscn_err_srcst_varg_bad_ctx			= 1091,
		mscn_err_srcst_preproc_notlbeg			= 1092,			// Preproc stmt token stays after some non empty lexema on the line.
		mscn_err_srcst_preproc_other			= 1093,			// %1 - token num value.

		// Paste buffering related errors.
		mscn_err_pbuff_eof_suff_nob			= 1103,			// Eof with paste suffix and there is no active suff based buffering.
		mscn_err_pbuff_oom_for_pbuff			= 1104,
		mscn_err_pbuff_nesting_deep			= 1105,			// %1 - nesting limit.
		mscn_err_pbuff_quote_failure			= 1106,
		mscn_err_pbuff_delim_failure			= 1107,
		mscn_err_pbuff_lexema_failure			= 1108,
		mscn_err_pbuff_stail_failure				= 1109,			// Tail of the curr level source.
		mscn_err_pbuff_esc_failure				= 1110,			// Escaping inline quotes.
		mscn_err_pbuff_cnct_lex_first			= 1111,
		mscn_err_pbuff_cnct_lex_last			= 1112,
		mscn_err_pbuff_str_chr_param			= 1113,			// # or #@ and no param name after them.

		// Errors while updating the macro library.
		mscn_err_mlib_creat_builtin				= 1127,
		mscn_err_mlib_modif_builtin				= 1128,
		mscn_err_mlib_redef_to_same			= 1129,
		mscn_err_mlib_redef_to_diff				= 1130,
		mscn_err_mlib_undef_twice				= 1131,
		mscn_err_mlib_modif_predef				= 1132,			// Warning.

		// Errors in #define and #undef statements.
		mscn_err_mdef_name_token				= 1143,			// %1 - token_name, %2 - stmt name. Warning.
		mscn_err_mdef_no_macro_name			= 1144,
		mscn_err_mdef_oom_for_macro			= 1145,			// No mem for the Macrodef structure.
		mscn_err_mdef_no_mprm_lst_end		= 1146,
		mscn_err_mdef_no_mprm_name			= 1147,
		mscn_err_mdef_no_vargs_paren			= 1148,
		mscn_err_mdef_mprm_name_dupe		= 1149,
		mscn_err_mdef_too_many_mprms		= 1150,			// %1 - max number of macro params.
		mscn_err_mdef_no_mprm_separ			= 1151,
		mscn_err_mdef_mprm_name_token		= 1152,			// %1 - token_name.
		mscn_err_mdef_mprm_lst_error			= 1153,
		mscn_err_mdef_prstkn_in_mdef			= 1154,			// Preproc stmt token in mdef (warning).

		// Misc issues while scanning.
		mscn_err_misc_bad_fsys_setup			= 1166,			// This is rare case when it is not possible to start the parsing.
		mscn_err_misc_bltin_bad_fid				= 1167,			// %1 - fid of the built in file.
		mscn_err_misc_bltin_unkn_fid			= 1168,			// %1 - fid of the built in file.
		mscn_err_misc_aborted_by_user			= 1169,
		mscn_err_misc_prst_extra_lex			= 1170,
		mscn_err_misc_not_impl_yet				= 1171,
		mscn_err_misc_out_of_memory			= 1172,			// %1 - loc name, %2 - name context.
		mscn_err_misc_indexing_key_reg		= 1173,			// %1 - key name.
		mscn_err_misc_atool_env_miss			= 1174,			// %1 - file name.
		mscn_err_misc_lex_from_cmdl			= 1175,

		// Errors while scanning the macro call.
		mscn_err_mcall_eol_in_mcall			= 1186,
		mscn_err_mcall_eof_in_mcall			= 1187,
		mscn_err_mcall_mprm_not_foll			= 1188,
		mscn_err_mcall_varg_not_foll			= 1189,
		mscn_err_mcall_varg_bad_ctx			= 1190,
		mscn_err_mcall_err_adding				= 1191,
		mscn_err_mcall_more_actual				= 1192,			// %1 - prms in call, %2 - prms in macro.
		mscn_err_mcall_less_actual				= 1193,			// %1 - prms in call, %2 - prms in macro.

		// Errors in the cond parsing statements.
		mscn_err_cond_nesting_deep			= 1208,		// %1 - max number of cond parsing layers.
		mscn_err_cond_stack_underflow			= 1209,
		mscn_err_cond_elif_after_else			= 1210,
		mscn_err_cond_second_else				= 1211,
		mscn_err_cond_ifdef_no_name			= 1212,
		mscn_err_cond_oom_for_code			= 1213,
		mscn_err_cond_rdef_cond_used			= 1214,		// Redefined cond is used in defined(x).
		mscn_err_cond_unkn_name_used		= 1215,
		mscn_err_cond_oom_for_clone			= 1216,

		// Errors and messages from other preprocessor stmts.
		mscn_err_ostm_incl_no_fname			= 1230,
		mscn_err_ostm_pragma_no_type			= 1231,
		mscn_err_ostm_pragma_bad_once		= 1232,
		mscn_err_ostm_pragma_once_dup		= 1233,
		mscn_err_ostm_pragma_note			= 1234,
		mscn_err_ostm_error_stmt				= 1235,
		mscn_err_ostm_import_no_fname		= 1236,
		mscn_err_ostm_line_no_fname			= 1237,
		mscn_err_ostm_line_no_lnum			= 1238,
		mscn_err_ostm_using_no_fname			= 1239,

		// Errors while scanning the conditional expressions.
		mscn_err_cexpr_empty_lexema			= 1260,
		mscn_err_cexpr_64bit_negnum			= 1261,
		mscn_err_cexpr_128bit_number			= 1262,
		mscn_err_cexpr_64bit_charconst			= 1263,
		mscn_err_cexpr_foating_point_lexema	= 1264,
		mscn_err_cexpr_string_lexema			= 1265,
		mscn_err_cexpr_operation_missing		= 1266,
		mscn_err_cexpr_unary_op_error			= 1267,
		mscn_err_cexpr_lpar_no_operand1		= 1268,
		mscn_err_cexpr_no_oprnd_op			= 1269,
		mscn_err_cexpr_no_oprnd_bop			= 1270,
		mscn_err_cexpr_no_oprnd_rpar			= 1271,
		mscn_err_cexpr_rpars_than_lpars		= 1272,
		mscn_err_cexpr_unkn_operation			= 1273,
		mscn_err_cexpr_defnd_params			= 1274,
		mscn_err_cexpr_defnd_no_rpar			= 1275,
		mscn_err_cexpr_unkn_mparams			= 1276,
		mscn_err_cexpr_no_operation			= 1277,
		mscn_err_cexpr_expr_missing			= 1278,
		mscn_err_cexpr_lpar_no_operand2		= 1279,
		mscn_err_cexpr_unop_no_operand		= 1280,
		mscn_err_cexpr_biop_no_operand		= 1281,
		mscn_err_cexpr_no_operand				= 1282,
		mscn_err_cexpr_expr_uncomplete		= 1283,
		mscn_err_cexpr_out_of_memory			= 1284,
		mscn_err_cexpr_stack_overflow1			= 1285,
		mscn_err_cexpr_stack_overflow2			= 1286,
		mscn_err_cexpr_resv_unopr				= 1287,
		mscn_err_cexpr_resv_condopr1			= 1288,
		mscn_err_cexpr_resv_condopr2			= 1289,
		mscn_err_cexpr_resv_biop				= 1290,
		mscn_err_cexpr_resv_unkn_op			= 1291,
		mscn_err_cexpr_redef_predef_used		= 1292,
		mscn_err_cexpr_known_undef			= 1293,
		mscn_err_cexpr_unknown_name			= 1294,

		mscn_err_max = 1299,
	};

	static const wchar_t *GetErrorText(int err_code);
							// Method returns non empty addr of some static string even for unknown error codes.
							// Returned string may have "%" placeholders.

	bool		GetFileOffsInfo(TFileInfo *finfo, long loc_offs, long &line_num, long &char_pos, wchar_t **line_beg = NULL, long *line_len = NULL);
					// Method returns the line number and the char offset in the file. Method expects that this file is known
					// to the files chache. Tabs are treated by this method like spaces. It is also possible to get the pointer to
					// the beginning of the string and its length. Note that lines in the file cache are NOT null terminated.
					// When the return value is FALSE, the params are not modified.

	void		PrepareCallbackHeader(PrEventHeader &hdr, ID cn, PrEventHeader *ctx);
					// Method checks if the ctx is passed or not. If the ctx is passed, it uses it. Otherwise it picks up current
					// scan position. This method should be used in situations when the ctx can be omitted but once it is present,
					// it is correct.

	void		PrepareCallbackHeaderSpecial(PrEventHeader &hdr, ID cn, PrEventHeader *ctx);
					// Method uses passed ctx when it is not NULL and when it is not empty. Otherwise it fills the event header
					// either with the current scan pos or it clears the header.
protected:

	struct TMidLevConsoleDataStt
	{
		TDateTime			parsing_start_time;
		TDateTime			prev_update_time;
		long					prev_update_rots_cnt;			// Total number of rotations before updating console that was assigned
															// during the latest revision of the update delay.
		ULONG				mid_lev_lexemas;
		ULONG				mid_lev_events;
		ULONG				num_errors;
		ULONG				num_warnings;
		TDateTime			elapsed_time;

		bool					abort_msg_displayed;			// This flag is used to avoid displaying the abort message multiple times.
															// Other purpose of this flag is confirmation of processing the external
															// abort request inside the scanner.

		inline void Clear() { memset(this, 0, sizeof(TMidLevConsoleDataStt)); }
	};

protected:

	//
	//  Data fields of the parser.
	//

	// 1. General parser state and options.
	TMidLevParsingPhase			parsing_phase;
	TCommandLineTaskInfo			cmdl_task_info;
	TMajorSrcFileInfo				major_src_info;
	TMidLevPrepOptions			prep_opts;
	TMidLevScannerOptions			scan_opts_proto;
	TMidLevScannerOptions			options;

	int							abort_err_code;					// The value of the field is 0 if the abort is not requested.
	TLexErrorClass				abort_err_class;

	// 2. Include paths, known files.
	TFileNameBuffer				base_directory;					// Base direcory is passed as a param to the BeginParsingSession() or
																// to the BeginRawIndexingSession(). If this param is not NULL or empty,
																// then it should contain an absolute path and this is explicitly checked.
																// Otherwise the base dir is set to the current directory of the process.
																// Contents of this field has no backslash at its end.
	TList							drive_substs;
	bool							allow_intermediate_paths;		// Intermediate paths are paths that are not absolute and at the same time
																// they are not relative to the base dir, like "d:name.h" or "\dir\name.h".
	bool							allow_non_substed_paths;		// This field is checked only if the list of the drive_substs is not empty.

	TList							inc_paths;
	TFileInfoItemsList				known_files;						// These are primarily disk files. All files in this list have different names.
																// Plus they all have parsing_id equal to 0 (strg_disk, strg_builtin, strg_proto).
	TList							temp_files;						// Files in this list may have same names. Items have TFileInfoItem class and
																// are identified by pid, fid. All files have pid != 0 (strg_temp, strg_interm).

	TSelfChangeMacroGenHist		scgm_info[mbit_num_types];

	// 3. Lexema sources nesting state.
	TLexemasSource				src_stack[MAX_INCLUDE_STACK_LEN];
	int							src_cnt;							// Number of active slots in the stack of sources.
	bool							src_on_doubt;					// At least one of the lex sources in the stack is on doubt.
	int							src_cnt_min;					// When scanning comes to the end of the lex source at this level,
																// the <eof> lexema is given out instead of quitting the nested source.
	TPasteBufferingSlot			pbuff_stk[MAX_PASTE_BUFF_STACK_LEN];
	int							pbuff_cnt;						// Number of active slots in the stack of paste bufferings.

	// 4. Conditional compilation state.
	// When the parsing starts, the value of cond_cnt is zero and there is no info in the cond parsing stack.
	// First #ifxx statement increases the cond_cnt and fills in the first cell of the stack.
	TCondCompLevel				cond_stk[MAX_IFDEF_STACK_LEN];
	int							cond_cnt;
	bool							active_code;
	bool							conditional_area;
	TConditionalStatus				cond_pars_status;
	ID							cond_area_cn;

	// 5. Macrodefinitions and indexing keys.
	TMacroDefinitionsList			macrodefs_list;
	TIndexingKeyInfoItemsList		keys_list;
									// List of keys is used here primarily because in some cases the callback handler maintains
									// its own table of indexing keys, in other cases not. This list (hash table) contains instances
									// of TKeyInfoItem structures. Names of the keys in the stuctures are NULL terminated.
									// Bodies of the names are stored in the misc_strings_dict.
	//
	//  Misc and service state variables.
	//

	TMidLevScanCbkHandler		*cbk_handler;					// The primary callback handler should always be not NULL.
	TMidLevScanCmdlCbkHandler	*cmdl_cbk_handler;				// The command line cbk handler is optional. It can be NULL.
	long							max_rprt_linear;

	TLexema						saved_lex;
	bool							include_fname_mode;
	ID							latest_cn;						// This field contains the value that was already given out.

	TMidLevParsingSummary		summary;
	TRawIndexingSessSummary		raw_sess_summary;				// This stucture does not keep the number of msgs/warns/errors. These fields
																// are taken from the pars sess sumry when the raw sess sumry is given out.
	DWORD						trace_mask;
	long							next_trace_evtid;
	WORD						curr_trace_attr;					// Emitted messages will be displayed using this color.

	TMidLevConsoleDataStt			console_data_stt;
	long							status_callback_cnt;

	bool							soft_macro_expansion_mode;
									// This is special flag for scanning the conditional expressions. When it is set, macros without params
									// that have just one lexema in their body, are not expanded but they are returned as a name lexemas.

	bool							macro_call_happened;
									// The field is set whenever the macro is called. This allows to know if non trivial macros were used
									// in conditional expression or not.

	bool							block_paths_substing;
									// This flag is set during short and rare periods of time when it is necessary to load the file that
									// belongs to the tool itself and in this way it does not belong the filename space of the application.

	TStringsDict					misc_strings_dict;
									// Primary purpose of this dict is keeping names of the indexing keys and it is passed to the raw
									// scanners to store their persistent strings. Other misc strings are stored here also.

private:   /* 2 */

	inline void SetPhase(TMidLevParsingPhase phase) { parsing_phase = phase; }
				// This method is declared as private to simplify tracking changes of the parsing phase.

	void		SetupIntegralCondParsingState(int cond_pars_stack_len);

protected:

	inline void PostProsessMidLevCbkCall(PrEventHeader &hdr)
	{
		summary.mid_lev_events++;
		if (hdr.lin_area.linear_beg > max_rprt_linear)
			max_rprt_linear = hdr.lin_area.linear_beg;
	}

	inline void PostProsessMidLevCbkCallEx(PrEventHeader &hdr)
	{
		if (hdr.lin_area.linear_beg > max_rprt_linear)
			max_rprt_linear = hdr.lin_area.linear_beg;
	}

	void		AbortParsingInternal(PrEventHeader *abort_location, int err_code, TLexErrorClass err_class);
					// This method should be called when some part of the compiler, like the #error statement  procedure,
					// decides that further scanning should not take place. This method exits from all nested sources except
					// for the top level source and places the top level source into the eof state.

	ID		AddWarning(PrEventHeader *ctx, int err_num, ID parent_event = 0, const wchar_t *ext1 = NULL, const wchar_t *ext2 = NULL, const wchar_t *ext3 = NULL)
								{ return(AddErrWarnInternal(ctx, parent_event, lerrc_preproc_warn, err_num, ext1, ext2, ext3)); }
	ID		AddError(PrEventHeader *ctx, int err_num, ID parent_event = 0, const wchar_t *ext1 = NULL, const wchar_t *ext2 = NULL, const wchar_t *ext3 = NULL)
								{ return(AddErrWarnInternal(ctx, parent_event, lerrc_preproc_err, err_num, ext1, ext2, ext3)); }
	ID		AddOutOfMemError(PrEventHeader *ctx, TMidLevOutOfMemLocation loc, const wchar_t *ctx_name = L"n/a")
								{ wchar_t buff[20]; return(AddErrWarnInternal(ctx, 0, lerrc_preproc_err, mscn_err_misc_out_of_memory, _itow(loc, buff, 10), ctx_name, NULL)); }
	ID		AddErrWarnInternal(PrEventHeader *ctx, ID parent_event, TLexErrorClass err_cls, int err_num, const wchar_t *ext1, const wchar_t *ext2, const wchar_t *ext3);
					// All 3 methods above report errors/warnings from inside the mid level scanner and they are not intended
					// to be used in the high level parsers.

	TFileInfoItem	*RegisterNewFile(PrEventHeader *error_context, TFileInfo &finfo);
					// NB: Passed file should not be present in the cache. Error context should be NULL only if no lex source
					// is entered. Method creates TFileInfoItem and assigns file_id to the file. After that it places it to the cache
					// of known files or to the list of temp files. In case of success it returns pointer to the cached file descriptor
					// and takes ownership on resources that were passed in the finfo structure. In case of failure it returns NULL
					// and writes an error. On failure the file name and body still belong to the caller.

	TFileInfoItem	*CreateIntermFile(PrEventHeader *ctx, TRawScanningType rscn);
					// Name of the file is generated and assigned inside. Caller is responsible for calling SaveIntermFile() some
					// time later on.

	void			SaveIntermFile(TFileInfoItem *pmsf);
					// Context is not passed here because all functions that are called inside cannot generate error and the file reg
					// event is fired with the header that is stored inside the passed intermediate file. Call to this method also sets
					// the "write complete" flag on the file.

	bool		PurgeCachedFile(TFileInfoItem *info);
	bool		PurgeCachedIndexingKey(TKeyInfoItem *info);

	bool		EnterFileSource(ID inc_path_id, TFileInfoItem *file_item,
							TLexSourceType srct, TMacroDefOrigin defs_nature,
							long seek_pos, long length_to_scan,
							bool new_line_stt, bool source_on_doubt,
							PrEventHeader *stmt_area, PrEventHeader *token_area, PrEventHeader *fname_area);
					// The file, that is being entered, should already belong to the files cache. This method
					// makes trace, fills in the lex src stack frame and sends the structural callback.

	bool		EnterMacroSource(TMacroDefinition &mdef, PrEventHeader *stmt_area, PrEventHeader *name_area,
							bool xpnd_macros, bool paste_suff);
	bool		EnterMacroParamSource(PrEventHeader *stmt_area, int param_inx, TPreprocToken param_decor,
							bool xpnd_macros, bool paste_suff);

	bool		EnterPasteBuffSource(PrEventHeader *event_ctx, bool suff_buffer, bool append_tail,
							TFileInfoItem *pbuff_buff, TLexemasSource *lex_src_proto = NULL);
					// If the lex_src_proto is passed, it should be a context of the already exited src level.

	void		ExitFromLexSource(PrEventHeader *eof_area);
					// Method exits from one level of the lexema sources. When the eof_area param is NULL,
					// this exit is not traced and the structural callback is not sent. This feature is used when
					// the scanner is reset or destroyed.

	bool		EnterPasteBuffLayer(PrEventHeader *event_area, TPreprocToken buff_type, bool paste_suff, TFileInfoItem *forced_buff = NULL);
	void		ExitPasteBuffLayer(PrEventHeader *event_area, TFileInfoItem **ppbuff = NULL);

	bool		ServeSelfChangingMacroCall(TBuiltInMacroType mbit);

	void		ProcessMacroDirective(TLexema &directive_lexema);
	void		ProcessEmptyStmt(TLexema &directive_lexema, TMidLevScannerTraceFlags mask, const wchar_t *stmt_prefix);

	void		ProcessDefineStmt(TLexema &directive_lexema);
	void		ProcessUndefStmt(TLexema &directive_lexema);
	void		ProcessIncludeStmt(TLexema &directive_lexema);
	void		ProcessConditionalStmt(TLexema &directive_lexema);

	void		ProcessPragmaStmt(TLexema &directive_lexema);
	void		ProcessErrorStmt(TLexema &directive_lexema);
	void		ProcessImportStmt(TLexema &directive_lexema);
	void		ProcessLineStmt(TLexema &directive_lexema);
	void		ProcessUsingStmt(TLexema &directive_lexema);

	void		ScanCondExpression(ID stmt_cn, int stmt_cond_lev,
						ID &error_cn, TCondExprScanningDetails &scan_dtls,
						PrEventHeader &expr_area, PrEventHeader &stmt_end_area,
						TCondExprTreeItem **ppcode);
	void		ScanIfdefExpression(ID stmt_cn, int stmt_cond_lev, bool ifndef_stmt, TLexema &token_lex,
						ID &error_cn, TCondExprScanningDetails &scan_dtls,
						PrEventHeader &expr_area, PrEventHeader &stmt_end_area,
						TCondExprTreeItem **ppcode);
				// Both methods scan till the end of the line and return offset to the beginning of the eol lexema.

	TCondExprTreeItem *CreateNameTreeItem(ID stmt_cn, int stmt_src_lev,  int stmt_cond_lev,
						TKeyInfoItem *key_info, TLexema &name_lexema,
						TCondExprScanningDetails &dtls);
				// Method looks for the definition state of the passed name and creates either a decorated
				// value or a variab_def code item. Method returns NULL only in the of out of memory case.

	TConditionalStatus EvaluateCondExprTree(ID &err_evt_cn, TCondExprSmplResult &smpl_stt,
						PrEventHeader *err_ctx, TCondExprTreeItem *code_tree);
				// Method simplifies the expression and converts its remaining part to one of the expression
				// result enum values.

protected:   /* 3 */

	TLexemaType	MidScan(TLexema &lex);
					// Method can return lexemas from ltx_comment to ltx_error. Macro/macro param can be entered
					// from inside this method. Exit from the lex source can also happen. This method will not exit from
					// the level, that is set with SetLexSourceExitLimit().

	void			RewindMidScan(PrEventHeader *ctx, TLexemasSource &src, TRawScanSeekInfo &pos_info, const wchar_t *ext_msg = NULL);
					// Method sets up the beg of the passed lex as the new scanning pos and clears the saved_lex.
					// It should be called only from the inside of MidScan.

	inline bool	CanFilterOut(TLexema &lex) { return(lex.type == ltx_comment || lex.type == ltx_eol && (src_cnt > src_cnt_min || lex.endofline_type != leolt_normal)); }
					//
					// Behavior of this method significantly depends on the nesting layer of the curr context.
					//
					//   ltx_comment:		always ignore.
					//   ltx_eol:
					//			border nest level:		ignore only decorated eols.
					//			deeper nest level:		ignore all types of eols;
					//

	inline void	SetupMidLevOrigin(PrEventHeader &origin, TLexemasSource &src)
						{ origin.cn = latest_cn; origin.source_ref = src.cn_enter; origin.cond_ref = cond_area_cn; origin.PrepareLinearArea(src.curr_linear_beg); origin.partial_src_area = origin.mixed_cond_pars = FALSE; }

	TLexemaType	GetLoLex(TLexema &lex);
	TLexemaType	GetFilteredLoLex(TLexema &lex);
					// These methods can return any type of lexema including the ltx_empty. This means that a new lex
					// src was entered. In this case the upper layer should call one of these methods again. These methods
					// never exit from any lex source.

	inline TLexemaType GetNextPosAndFilteredLoLex(TLexemasSource &src, TRawScanSeekInfo &pos_info, TLexema &lex)
	{
		src.scanner.SkipSpaces();
		src.scanner.GetCurrPos(pos_info);
		return(GetFilteredLoLex(lex));
	}

	inline TLexemaType GetLoLexWithSavedLexCheck(TLexema &lex)
	{
		if (saved_lex.type != ltx_empty)
		{
			lex = saved_lex;
			saved_lex.SetEmpty();
		}
		else
		{
			GetLoLex(lex);
		}

		return(lex.type);
	}

	TLexemaType		GetPrStmtLoLex(TLexema &lex);
	TLexemaType		GetPrStmtMidLex(TLexema &lex);
						// These methods are relatively close to the GetFilteredLoLex(). They report raw errors inside
						// if they come across them.

	bool		IsNamedMacroParam(TStrPtrInfo &name, int &prm_inx);
	bool		IsMacroName(TStrPtrInfo &name, TMacroDefinition *&mdef);
				// Methods check the passed strings for being a macro param name or macro name in the current context.
				// Note that the macro param index is zero based.

	void		ScanMacroCallParams(TMacroDefinition &mdef, PrEventHeader &mcall_area_end);
				// Method fills macro definition with param values or with with empty params. It may report errors
				// from inside. Method puts the end of the macro call area into the second param.

	void		ScanTillEol(TLexema &latest_scanned, bool allow_lexemas, PrEventHeader &end_loc_hdr);
				// Method scans lexemas till the end of the line. Typical use: #include statement in the passive code.
				// Last param receives a ZERO sized header with the beginning of the eol or the eof lexema that has
				// stopped the scanning.

	void		SetIncFileNameMode(bool new_val);
				// Method sets this flag to the current source nesting level and ensures that all levels that will
				// be entered will also have this flag set.

	int		SetLexSrcExitLimit(int new_lim) { int old_lim = src_cnt_min; src_cnt_min = new_lim; return(old_lim); }
				// This method sets the high water mark for silent exits from the lex sources.

protected:   /* 4 */

	void		StdProcessKeyParam(TCmdlLexema &cmdl_lex);
	void		StdProcessPosParam(TCmdlLexema &cmdl_lex);

	void		FirstLexSourceEnterActions();
	bool		LoadAnalysisToolProfile(PrEventHeader *ctx);
	bool		LoadBuiltInMacrosFile(PrEventHeader *ctx);
	void		CmdlProcessAnalysisToolKey(TCmdlLexema &cmdl_lex);
	void		CmdlProcessDefinitionKey(TCmdlLexema &cmdl_lex);
	bool		CmdlNestLexSource(TCmdlLexema &cmdl_lex, TFileStorageType strg, TRawScanningType rscn, TLexSourceType srct);

	TLexemaType CmdlRawScanCmdlArea(TRawScanner &raw_scanner, TLexema &lex, PrEventHeader &origin_proto);

	wchar_t	*PrepareBaseDirectory(wchar_t *buffer_2_max_path_len, const wchar_t *parsing_base_dir, DWORD &resvd_dir_len, DWORD &resvd_dir_win32_error);
	bool		SetupFileHandlingFields(const wchar_t *parsing_base_dir, wchar_t *base_dir, DWORD resvd_len, DWORD resvd_error, TDriveSubstInfo *drive_substs_array, bool allow_interm_paths, bool allow_unsubsted_paths);

	bool		CreateAbsolutePath(PrEventHeader *ctx, TFileNameBuffer &buffer, const wchar_t *file_name, bool fname_is_file = TRUE);
				// Method expects that passed file_name is not NULL and not empty. When return value is TRUE, this means
				// that there was no OOM error and that buffer contains syntactically correct path. When method fails, all
				// errors are reported inside.

	bool		CreateSubstedPath(PrEventHeader *ctx, TFileNameBuffer &buffer, const wchar_t *file_name);
				// When method fails, all errors are reported inside. This method is sensitive to the block_paths_substing
				// flag that is set when it is necessary to load the file for the tool itself.

	bool		ConvertToTextBuffer(PrEventHeader *ctx, TTextBuffer80 &buffer, TStringPtr &string_ptr);

	void		TraceColumnTitles();
	void		TraceMsgInternal(PrEventHeader *context, const wchar_t *prefix, const wchar_t *action_label, const wchar_t *format, va_list vargs);
	void		TraceLexemaInternal(PrEventHeader *context, const wchar_t *prefix, const wchar_t *action_label, const wchar_t *lexema_label, TLexema &lex);

public:

	void TraceEvent(PrEventHeader *context, const wchar_t *prefix, const wchar_t *format = NULL, ...)
	{
		va_list vargs = NULL;
		if (format != NULL)
			va_start(vargs, format);
		TraceMsgInternal(context, prefix, NULL, format, vargs);
		va_end(vargs);
	}

	void TraceFuncEnter(PrEventHeader *context, const wchar_t *func_prefix, const wchar_t *format = NULL, ...)
	{
		va_list vargs = NULL;
		if (format != NULL)
			va_start(vargs, format);
		TraceMsgInternal(context, func_prefix, L"Entr", format, vargs);
		va_end(vargs);
	}

	void TraceFuncExit(PrEventHeader *context, const wchar_t *func_prefix, const wchar_t *format = NULL, ...)
	{
		va_list vargs = NULL;
		if (format != NULL)
			va_start(vargs, format);
		TraceMsgInternal(context, func_prefix, L"Exit", format, vargs);
		va_end(vargs);
	}

	void TraceEventLexema(PrEventHeader *context, const wchar_t *prefix, const wchar_t *lexema_label, TLexema &lex)
	{
		TraceLexemaInternal(context, prefix, NULL, lexema_label, lex);
	}

	void TraceFuncEnterLexema(PrEventHeader *context, const wchar_t *prefix, const wchar_t *lexema_label, TLexema &lex)
	{
		TraceLexemaInternal(context, prefix, L"Entr", lexema_label, lex);
	}

	void TraceFuncExitLexema(PrEventHeader *context, const wchar_t *prefix, const wchar_t *lexema_label, TLexema &lex)
	{
		TraceLexemaInternal(context, prefix, L"Exit", lexema_label, lex);
	}

protected:

	void		TraceRawUnscan(PrEventHeader *ctx, const wchar_t *pos_info_label, TRawScanSeekInfo &pos);
	void		TraceRawUnscan(PrEventHeader *ctx, const wchar_t *pos_info_label, TLexema &lex);

	void		InitConsoleInternal();

	long		UpdateConsoleInternal(bool force_update);
				// Return value is the number of internal rotations that should be skipped before update console
				// function should be called again.

	void		ShowConsoleValue(TScannerConsoleLabelIndex lab_index, ULONG value);

	friend class TMacroDefinition;
	friend class TCondExpressionScanner;
	friend class TMidLevIncludeHelper;
	friend class TMidLevScanCmdlCbkHandler;
	friend class TMidLevScanCbkHandler;
	friend class TParsingDriver;
	friend class TGrammarDefinitionParser;
	friend class TMidLevRamLogger;
	friend class TCppParser;
};

class TMidLevScanCbkHandlerProxy : public TMidLevScanCbkHandler
{
public:

		TMidLevScanCbkHandlerProxy(TMidLevScanCbkHandler *cbk_handler = NULL, TGenericConsoleEx *console = NULL)
				: TMidLevScanCbkHandler((console != NULL) ? console : cbk_handler->m_console) { m_handler = (cbk_handler != NULL) ? cbk_handler : &m_local_handler; }

	void		SetupConsole(TGenericConsoleEx *console) { m_handler->SetupConsole(console); }
	void		SetupScanner(TMidLevScanner *scanner) { m_handler->SetupScanner(scanner); }

	bool		AllocateBatchId(ID &batch_id) { return(m_handler->AllocateBatchId(batch_id)); }

	TFileInfoItem		*CreateFileInfoItem(TFileInfo &finfo) { return(m_handler->CreateFileInfoItem(finfo)); }
	TKeyInfoItem		*CreateKeyInfoItem(TKeyType type, const wchar_t *name) { return(m_handler->CreateKeyInfoItem(type, name)); }
	TMacroDefinition	*CreateMacroDefinition(TStrPtrInfo &name, TMacroDefStatus stts, TMacroDefOrigin nature) { return(m_handler->CreateMacroDefinition(name, stts, nature)); }

	bool		LookupOrRegisterDiskFileInfo(TFileInfo &info) { return(m_handler->LookupOrRegisterDiskFileInfo(info)); }
	bool		LookupOrRegisterIndexingKey(TKeyInfoItem &info) { return(m_handler->LookupOrRegisterIndexingKey(info)); }

	bool		StartRawIndexingSession(PrInitParsingInfo1 &init_info) { return(m_handler->StartRawIndexingSession(init_info)); }
	void		FinishRawIndexingSession(TRawIndexingSessSummary &sumry, TFileInfoItem *mj_src_file = NULL) { m_handler->FinishRawIndexingSession(sumry, mj_src_file); }

	bool		StartParsing(PrInitParsingInfo1 &init_info) { return(m_handler->StartParsing(init_info)); }
	bool		StartPrepPhase(PrInitParsingInfo2 &init_info) { return(m_handler->StartPrepPhase(init_info)); }
	bool		StartMainPhase(PrInitParsingInfo3 &init_info) { return(m_handler->StartMainPhase(init_info)); }
	void		FinalizeParsing(TMidLevPrepOptions *prep_options, TMidLevScannerOptions *scan_options, TMidLevParsingSummary &summary, int final_pars_status)
														{ m_handler->FinalizeParsing(prep_options, scan_options, summary, final_pars_status); }

	ID		GetParsingId() const { return(m_handler->GetParsingId()); }
	bool		IsRawIndexingSessionActive() const { return(m_handler->IsRawIndexingSessionActive()); }
	bool		IsIdle() const { return(m_handler->IsIdle()); }

	void		RegisterDriveSubstData(PrDriveSubstRegistration *info)			{ m_handler->RegisterDriveSubstData(info); }
	void		RegisterTempFileData(PrTempFileRegistration *info)				{ m_handler->RegisterTempFileData(info); }

	void		PasteBuffChangeNotification(PrPasteBufferingChange *info)		{ m_handler->PasteBuffChangeNotification(info); }
	void		LexSourceEnterNotification(PrLexSourceEnter *info)			{ m_handler->LexSourceEnterNotification(info); }
	void		LexSourceExitNotification(PrLexSourceExit *info)				{ m_handler->LexSourceExitNotification(info); }
	void		MacroLibraryChangeNotification(PrMacroLibraryChange *info)	{ m_handler->MacroLibraryChangeNotification(info); }
	void		CondParsSttChangeNotification(PrCondParsingSttChange *info)	{ m_handler->CondParsSttChangeNotification(info); }

	void		MiscPreprocEventNotification(PrMiscPreprocStmtEvent *info)		{ m_handler->MiscPreprocEventNotification(info); }
	void		NewIncludePathNotification(PrNewIncPathEvent *info)			{ m_handler->NewIncludePathNotification(info); }
	void		ParsingErrorNotification(PrErrorWarningEvent *info)				{ m_handler->ParsingErrorNotification(info); }
	void		CondParamUseNotification(PrCondParamUseEvent *info)			{ m_handler->CondParamUseNotification(info); }

	void		GrammarSectionNotification(PrGrammarDefnSection *info)		{ m_handler->GrammarSectionNotification(info); }
	void		GrammarObjectNotification(PrGrammarDefnObject *info)			{ m_handler->GrammarObjectNotification(info); }

	void		CppItemNotification(PrCppItemReferenceEvent *info)				{ m_handler->CppItemNotification(info); }
	void		MidLevLexemaNotification(TLexema *lex)						{ m_handler->MidLevLexemaNotification(lex); }

	bool		SetParsSessIndexingKeyUseStatus(TKeyInfoItem *info) { return(m_handler->SetParsSessIndexingKeyUseStatus(info)); }
	bool		SetRawSessIndexingKeyUseStatus(TLongId doc_id, TKeyInfoItem *info) { return(m_handler->SetRawSessIndexingKeyUseStatus(doc_id, info)); }
	bool		SetRawIndexingData(PrRawIndexingFileInfo &hdr_info, BYTE *inx_data, long inx_data_len) { return(m_handler->SetRawIndexingData(hdr_info, inx_data, inx_data_len)); }

	TMidLevStoreReader *AllocateReaderObject(wchar_t *buffer_4_max_path = NULL) { return(m_handler->AllocateReaderObject(buffer_4_max_path)); }
	void		DisposeReaderObject(TMidLevStoreReader *reader) { m_handler->DisposeReaderObject(reader); }

protected:

	TMidLevScanCbkHandler		*m_handler;

	TMidLevScanCbkHandler		m_local_handler;
									// When the destination handler is NULL, the current instance (this) cannot be used as destination
									// because an infinite recursion will happen. This is why the local handler is needed here.
};

#endif	// TextScan_MidScan_H


