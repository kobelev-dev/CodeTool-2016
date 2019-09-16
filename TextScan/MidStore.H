//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Programming interface to the results of the mid lev parsing session.
//

#ifndef	TextScan_MidStore_H
#define	TextScan_MidStore_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   Common_LookAsideTable_H
#include  "Common/LookAsideTable.H"
#endif
#ifndef   TextScan_MidScan_H
#include  "TextScan/MidScan.H"
#endif

// Version of the abs() function that works fine with the 64 bit values.
#define  ___abs(x) ((x) >= 0 ? (x) : -(x))

// --- --- --- ---  Top level structures  --- --- --- --- ---

struct CtvCompilationInfo
{
	ID						batch_id;						// Batch Id can be zero.
	ID						parsing_id;						// The parsing Id cannot be zero.
	THighLevelLangType		lang_type;						// This field can also be used to differentiate between the raw indexing
															// session and the language level session.
	DWORD					grammar_vers_code;

	TSourceAreaInfo			cmd_line_origin;
	TLongId					cmd_line_doc_id;				// When this field is not empty this means that the preparation phase
															// was used in the parsing session.
	wchar_t					*cmd_line_file_name;

	short					major_src_pos_param;
	TIdStruct					major_src_doc_id;
	wchar_t					*major_src_file_name;

	ID						major_src_scan_evt;				// Value of this field is not zero only when entering the mj_src
															// is delayed till the end of cmdl processing.
	ID						major_src_enter_evt;

	ID						major_src_prev_pars_id;			// An Id of the earlier parsing session that was scanning the major src file
															// of the current session. Note that in some cases it makes sense to store
															// several sessions for the same file for example when these sessions
															// were using different cond prasing params.
	TLongId					obj_file_doc_id;
	wchar_t					*obj_file_name;

	TDateTime				parsing_date;

	TMidLevPrepOptions		prep_opts;
	TMidLevScannerOptions		scan_opts;
								// The options fields are set twice. For the first time this happens when the corresponding phase
								// starts (if it starts) and for the second time this field is assigned when the parsing session is being
								// closed. The values of the options might be different during the second assignment.

	wchar_t					*base_dir;
								// The base directory cannot be NULL and it can be empty only for the sessions that failed to start.
								// In normal sessions this is always an absolute path with back slashes in it.

	bool						allow_intermediate_paths;
	bool						allow_unsubsted_paths;

	union
	{
		TRawIndexingSessSummary		raw_summary;
		TMidLevParsingSummary		summary;
	};

	int						parsing_status;
								//
								// This field can store the following values:
								//
								//		 -1						   --  Parsing session is still active.
								//		  0						   --  Parsing is complete and all lexemas were retrieved up to the final EOF.
								//	mscn_err_misc_bad_fsys_setup	   --  This is rare case when the parsing has failed to start.
								//	mscn_err_phsw_nest_src_unex	   --  Application has closed the parsing session without retrieving all lexemas.
								//	mscn_err_misc_aborted_by_user	   --  User pressed abort button on the scanning console.
								//	mscn_err_ostm_error_stmt		   --  #error statement aborted the parsing session.
								//		value					   --  AbortParsing(value) was called by the app layer code.
								//

	inline void Clear() { memset(this, 0, sizeof(CtvCompilationInfo)); }
};

// --- --- --- Compilation level structures --- --- ---

struct CtvDriveSubstRegEvent				// .drv_subst - [1]
{
	CtvEventHeader			hdr;
	TDriveSubstInfo			subst_info;
};

struct CtvTempFileRegEvent				// .temp_file - [2]
{
	CtvEventHeader			hdr;

	TFileInfo					temp_file_info;
};

struct CtvPasteBufferingChgEvent			// .paste_buff - [3]
{
	CtvEventHeader			hdr;								// The length of this linear area in the header is always zero.

	ID						parent_buff_ref;
	ID						evt_enter_cn;					// This field allows to distinguish between the buffering enter and buffering
															// exit events. The value of the field is zero for the enter events.
	short					new_buff_level;

	TPreprocToken			buff_type;						// The value of this field can be only one of the inline preprocessor tokens:
															//	  mpr_prm_cnct, mpr_prm_str, mpr_prm_chr.

	ID						buffer_fid;						// Note that all paste buffers are actually an intermediate files. This means that
															// this field contains an id of the file reg event in the current parsing session.
	ID						evt_exit_linear;					// This field is valid only for the buffing enter events. It contains the linear offset
															// of the corresponding buffering exit event.
	long						real_linear_end;					// This field is valid only for exits from the suffix based bufferings.
															// When the paste buffering exit event is processed, this field is filled with zero.
															// This field gets real value when exit from the buffer lex source is processed.
};

struct CtvLexSourceEnterEvent			// .src_enter - [4]
{
	CtvEventHeader			hdr;

	ID						source_ref;
	ID						cond_ref;
	ID						paste_buffering_ref;				// State of the paste buffering when this lex src was entered. This field has
															// nothing to do with entering the paste buffers themselves.

	TLexSourceType			src_type;

	bool						on_doubt;
	bool						inc_fn_mode;					// When this mode is set at the the lex source enter, it is guaranteed to remain
															// set till the exit from this lex scr enclosure.
	short					new_src_level;					// Source layers are numbered starting from 1. The topmost entered layer is
															// considered to be the first layer.

	ID						evt_exit_id;						// This field is useful when it is necessary to convert linear area into the source
															// area. Plus it is used as event ID restriction when events are iterated.
	ID						evt_exit_cond_ref;				// This field is needed when the source code viewer is processing lex scr calls
															// on the territory of some lex scr enclosure. This field simplifies the check
	ID						evt_exit_pbuff_ref;				// State of the paste buffering when the lex src was exited. This field has nothing
															// to do with exiting from the paste buffers themselves.
															// if the lex src call has changed the current cond pars status of the code or not.
	long						evt_exit_linear_pos;				// Linear offset of the lex source exit event.
	long						evt_exit_linear_after_ret;		// This is the copy of the linear_pos_after_return field of the lex src exit event.

	long						src_offs_after_exit;				// Source file offset of the area after the call event area. This field shows relation
															// between the linear pos of the event that is stored in the header and the location
															// of this call in the source file. This relation can be figured out without this field
															// by processing all call events in the session. But this field makes sense because
															// it is used for getting the source location from the linear offs that is much faster.
		//
		// Event exit fields are filled with non zero values when the lex src exit event is processed. Note that the linear
		// offset in the exit event may not be identical to the end of the area that was intended to be scanned.
		// The offset of the exit shows what was really scanned. Looking at the offset in the exit event it is not possible
		// to tell why the scanner decided to stop at this point. Exit event can be missing if the parsing session is still
		// running or when the session was not closed for some reason. Aborted (but properly closed) parsing sessions
		// always have all lex source exit events in place.
		//

	// Validity of the members of the union below depends on the src_type field.
	union
	{
		// Possible src types: srct_cmdl, srct_ant_env, srct_file, srct_bltin_defs, srct_mj_src.
		// This is the same to LEX_SRC_WHOLE_FILE(src_type).
		struct
		{
			ID					inc_path_id;
			TFileInfo				inc_file_info;
			TLinearArea			inc_token_area;
			TLinearArea			inc_fname_area;
			short				pos_params_cnt;			// Count of the pos params in the command line that were already scanned
															// before entering the current lex source. This field is used for correct displaying
															// the pos params in the include files with the command line syntax.
			short				evt_exit_pos_prms_cnt;		// Value of the same counter at the point of exit.
		};

		// Possible src types: srct_scgm.
		// There is no special macro for detecting this case.
		struct
		{
			TSourceAreaInfo		scgm_scan_area;			// This area describes the body of the self changing macro. This field can be
															// considered as sort of the macro definition.
		};

		// Possible src types: srct_macro, srct_mparam.
		// This is the same to LEX_SRC_MACRO_OR_MPARAM(src_type).
		struct
		{
			ID					mcall_def_id;
			wchar_t				*mcall_def_name;
			TSourceAreaInfo		mcall_def_area;				// MacroCall:
															//     Location of the body of the macro definition, that is used in the call.
															// MacroParamCall:
															//     Location of the parameter definition. This field is important because
															// this is the only one place in the database that contains information about
															// the contents of the macro parameter.
			bool					mcall_xpnd_macros;
			bool					mcall_paste_prefix;
			bool					mcall_paste_suffix;
			short				mcall_param_inx;			// For MacroCalls the value of this field is 0.
															// For mPrmCalls the value of this field is zero based inx.
			TPreprocToken		mcall_param_decor;
			TLinearArea			mcall_name_area;			// Only for macro calls. This field is empty in the macro param calls.
		};

		// Possible src types: srct_pbuff_macro, srct_pbuff_mparam.
		// This is the same to LEX_SRC_GEN_PBUFF(src_type).
		struct
		{
			bool					append_tail;
			ID					paste_buff_fid;
			long					paste_buff_len;				// This field is set when the lex src exit event is processed. Before this event
															// this field contains zero. While processing the exit event this field it is set to
															// the length of the paste buffer field.
				//
				// Entering the paste buffer has a lot in common with entering the whole file source. The difference is that
				// this call is always spaceless and it does not have several other whole file props. The paste_buff_len field
				// is equivalent of the length of the file that is stored in the inc_file_info field.
				//
		};
	};

	inline void Clear() { memset(this, 0, sizeof(CtvLexSourceEnterEvent)); }

	TLongId		GetEnclosureDocId() const;
	long			GetEnclosureSrcStart() const;
	long			GetEnclosureSrcEnd() const;
					// These methods return offsets in the source file being entered. Note that both offsets are known
					// when the source is entered. If the file was never scanned before, it is always scanned up to its
					// end and the length of the file is known. If the file is scanned partially, this means that this is some
					// sort of macro, and boundaries of the macro body are always known at the point of the macro call.
					// Note that in case of unexited paste buffer its src end is reported as zero.

	long			LinearPosAfterEntering() const { return(hdr.area.AreaEnd()+1); }

	bool			WholeLineLexSrcStyle() const { return(src_type != srct_scgm && src_type != srct_mparam); }
};

struct CtvLexSourceExitEvent				// .scr_exit - [5]
{
	CtvEventHeader			hdr;

	ID						source_ref;
	ID						cond_ref;
	ID						paste_buff_ref;					// State of the paste buffering when the lex source was exited.

	TLexSourceType			src_type;

	long						linear_pos_after_return;			// When the lex src exit event is stored, the value of this field is filled
															// into the evt_exit_linear_after_ret of the lex source enter event.
};

struct CtvMacroLibraryChangeEvent		// .mdef_chg - [6]
{
	CtvEventHeader			hdr;
	ID						prev_def_id;

	TMacroDefStatus			mdef_status;
	TMacroDefOrigin			mdef_origin_type;

	ID						mdef_key_id;					// Indexing key id.
	wchar_t					*mdef_name;

	short					num_params;
	bool						va_args_tail;

	TLinearArea				token_area;
	TLinearArea				name_area;
	TLinearArea				body_area;
};

struct CtvCondParsingChangeEvent		// .cond_pars - [7]
{
	CtvEventHeader			hdr;
	ID						source_ref;
	ID						cond_ref;

	ID						prev_stmt_id;
	ID						next_stmt_id;

	TPreprocToken			stmt_type;
	TCondExprSmplResult		expr_smpl_res;
	TConditionalStatus			expr_result;

	TConditionalStatus			stmt_code_stt;
	TConditionalStatus			new_code_stt;
	short					stmt_code_level;
	short					new_code_level;

	ID						scan_expr_error_id;
	TCondExprScanningDetails	scan_expr_details;
	int						num_expr_nodes;

	TLinearArea				stmt_token_area;
	TLinearArea				cond_expr_area;

	void Clear() { memset(this, 0, sizeof(CtvCondParsingChangeEvent)); }
};

//
//  Fields validity for different misc statement types:
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
// Note that when the mis_event_type is equal to the exmis_mjsrc_area, the callback implementation should
// save the event ID of the passed event in the major_src_scan_evt field of the parsing header.
//
// Note that when the #include statements are disabled in the scanner props, these statements do not generate
// any parsing events at all.
//
struct CtvMiscPreprocStmtEvent			// .misc_stmt - [8]
{
	CtvEventHeader			hdr;

	TMiscEventEx				mis_event_type;
	TLinearArea				token_area;						// The field is not empty when the token is present in the statement.
	TConditionalStatus			code_stt;

	DWORD					evt_details;
	wchar_t					*evt_str_data;

	long						line_num;
	TLinearArea				line_num_area;

	TLongId					doc_ident;
	wchar_t					*file_name;
	TLinearArea				fname_area;
};

struct CtvNewIncPathEvent				// .inc_path - [9]
{
	CtvEventHeader			hdr;								// Area in the header covers the whole command line parameter. It covers the the name
															// of the command line key and the value of the cmdl key param.
	wchar_t					*inc_path;						// Path, exactly as it was present in the code.
	wchar_t					*inc_directory;					// This is an absolute include dir as it should be used before doing the drive/directory
															// substitutions. Directory name has no backslash at the end.
};

struct CtvErrorWarningEvent				// .err_warn - [10]
{
	CtvEventHeader			hdr;
	ID						parent_event;

	TLexErrorClass			err_class;
	int						err_code;
	wchar_t					*err_msg;						// The error message may be not constant. It may include names of the variables, classes,
															// names of the files, etc.
	TLinearArea				ext_context;
};

struct CtvCondParamUseEvent				// .cond_param - [11]
{
	CtvEventHeader			hdr;
	ID						cond_stmt_id;
	TConditionalStatus			cond_stmt_code_stt;

	ID						cond_param_id;					// Indexing key id of the param.
	TCondExprTreeItemType	cond_param_type;				// Actually this is a subset of this enum.
	wchar_t					*cond_param_name;

	ID						decor_mdef_id;
	TMacroDefStatus			decor_mdef_status;
};

struct CtvGrammarDefnSection				// .grm_sect - [12]
{
	CtvEventHeader			hdr;
	TGrammarSectionType		section_type;
};

struct CtvGrammarDefnObject				// .grm_object - [13]
{
	CtvEventHeader			hdr;
	WORD					object_sym;				//
													// The value of the field may look like:
													//
													//	  TerminalSymbolsBase + offs	<--	This type of events is fired once for every terminal symbol of the source
													//									grammar at the location of the definition of this terminal symbol.
													//	  NonTerminalsBase + offs		<--	This event is fired once for every non terminal of the source grammar
													//									at the location of the first definition of this non terminal symbol.
													//	  RuleObjectsBase + offs		<--	This type of events is fired for every grammar rule in the source grammar
													//									at the location of the rule intro symbol.
													//	  RuleNonTermObject + offs		<--	This type of events is fired at every location of the rule non terminal in
													//									the source grammar.
													//	  XpctConflictNameObject		<--	This type of events is fired for every name of the expected conflict definition.
													//									Note that this type of event does not carry the index of the xpct conflict.
													//									Several different events may have this symbol value.
													//
													// Note that all offsets above are offsets in the tables of the SOURCE grammar. These events are fired
													// when the source grammar is scanned. At this point the target grammars are not yet existing.
													//

	WORD					object_app_id;			// This id is assigned by the app level before parsing of the grammar is started. It is used
													// in the callback handlers of the grammar based parser.
	ID						extra_context_id;		// This field is valid only for events that describe the rule intro symbols. This field is not used
													// for other grammar objects. This id references event for the non terminal for this rule.
};

struct CtvCppItemRefEvent					// .cpp_item - [14]
{
	CtvEventHeader			hdr;								// The linear area in the header covers the whole oject. This area may have zero length
															// but it should always have a non zero offset. This area is painted using the transparent
															// background in the source viewer.
	TCppItemType			cpp_item_type;
	short					cpp_item_nesting;

	ID						pars_sess_cpp_item_id;
	ID						project_level_cpp_item_id;		// The project level item id is assigned during the linking phase.

	TCppSourceArea			ext_src_area;					// The extension area can be empty. This should be checked by checking the type
															// of the area.
	ID						extra_areas_temp_file_id;		// This is an id of the parsing session specific binary file that contains an array of
															// additional TCppSourceArea objects.
};

union CtvParsingEventInfo
{
	CtvEventHeader	hdr;

	// Preprocessor related events.
	CtvDriveSubstRegEvent			drv_subst;
	CtvTempFileRegEvent			temp_file;
	CtvPasteBufferingChgEvent		pbuff_chg;
	CtvLexSourceEnterEvent		src_enter;
	CtvLexSourceExitEvent			src_exit;
	CtvMacroLibraryChangeEvent	mdef_chg;
	CtvCondParsingChangeEvent	cond_pars;
	CtvMiscPreprocStmtEvent		misc_stmt;
	CtvNewIncPathEvent			inc_path;
	CtvErrorWarningEvent			err_warn;
	CtvCondParamUseEvent		cond_param;

	// Grammar definition parsing.
	CtvGrammarDefnSection		grm_sect;
	CtvGrammarDefnObject			grm_object;

	// Language level events.
	CtvCppItemRefEvent			cpp_item;

public:

	void		Clear() { memset(this, 0, sizeof(CtvParsingEventInfo)); }
	long		GetSize();

	const wchar_t		*GetEventTypeName();
	const wchar_t		*GetEventTypeNameForListBox();
						// Both methods return only static strings.

	const wchar_t		*GetEventDescriptionForListBox(wchar_t *buffer_at_least_120_chars_long, long buff_len);
						// When the return value from this method is not NULL, this means that it is address of a const string.
						// Otherwise method places event description into the passed buffer or clears it.
public:

		//
		//	Public event comparison methods:
		//

	static int __cdecl	ComparePidEvidForListBox(const CtvParsingEventInfo *p1, const CtvParsingEventInfo *p2);
	static int __cdecl	CompareTypeDescrForListBox(const CtvParsingEventInfo *p1, const CtvParsingEventInfo *p2);
						//
						//  Ret val:	p1 >  p2		==>  1
						//			p1 == p2		==>  0			<--	This situation is possible and it is expected to happen.
						//			p1 <  p2		==> -1
						//
private:

	int				CompareEventTypeNamesForListBoxInternal(const CtvParsingEventInfo &other_inst) const;
	int				CompareEventDescriptionsForListBoxInternal(const CtvParsingEventInfo &other_inst) const;
						// These methods should be kept in sync with methods that return the text values.

	const wchar_t		*GetDescriptionMeasureForListBoxInternal(DWORD &dval) const;
};

class CtvParsingEventPtrsArray : public THeapBasedArray<CtvParsingEventInfo*, 0x8000>
{
public:

	typedef CtvParsingEventInfo *CtvParsingEventInfoPtr;
				// Unfortunately MS compiler is not able to match parameters of the comparison function
				// when they are defined as "const CtvParsingEventInfo**".

	static int __cdecl	ComparePidEvid(const CtvParsingEventInfoPtr *pp1, const CtvParsingEventInfoPtr *pp2) { return(CtvParsingEventInfo::ComparePidEvidForListBox(*pp1, *pp2)); }
	static int __cdecl	CompareTypeDescr(const CtvParsingEventInfoPtr *pp1, const CtvParsingEventInfoPtr *pp2) { return(CtvParsingEventInfo::CompareTypeDescrForListBox(*pp1, *pp2)); }
						// Static functions that compare double pointers.
};

//
// Item of this type is associated with every indexing key. It describes how the particular key is used in the document.
//
struct CtvKeyFileRefInfo
{
	ID			key_id;
	TLongId		doc_id;
	ULONG		use_cnt;
	DWORD		use_flags;

	inline void	Clear() { memset(this, 0, sizeof(CtvKeyFileRefInfo)); }
};

typedef THeapBasedArray<CtvKeyFileRefInfo, 256> CtvKeyFileRefsArray;

//
// Item of the type is associated with every indexing key. It describes how the particular key was used in the parsing session.
//
struct CtvKeyParsRefInfo
{
	ID			key_id;
	ID			pars_id;
	ULONG		use_cnt;
	DWORD		use_flags;

	inline void	Clear() { memset(this, 0, sizeof(CtvKeyParsRefInfo)); }
};

typedef THeapBasedArray<CtvKeyParsRefInfo, 128> CtvKeyParsRefsArray;

// -----------  File cache infrastructure  ------------------

//
//  The file name, file body and lines info are always stored in the heap allocations. These allocations
//  are owned by the instance of the structure.
//
class TSourceFilesTableItem : public TListItem
{
public:

	TSourceFilesTableItem(ID pars_id, ID file_id);
	TSourceFilesTableItem(TFileInfo &info);
	~TSourceFilesTableItem() { data.ReleaseLinesInfo(); data.ReleaseBasicFileInfo(); }

	bool		CheckDateLengthCrc();
				// Method compares local fields and returns TRUE if all fields are the same.

	virtual DWORD	GetHashValue() { return(data.file_id ^ data.pars_id); }
	virtual bool		IsEqualTo(const TSourceFilesTableItem &item) { return(data.file_id == item.data.file_id && data.pars_id == item.data.pars_id); }
						//
						// These functions are required for the hash table to work.
						//
public:

	TFileInfo			data;					// The length of the file body and date, that are sored in this field,
											// correspond to the file that was loaded from the disk.

	long				original_file_len;
	TDateTime		original_file_date;
	DWORD			original_file_crc;
						// These fields keep information about the file in the database (i.e. about the file that was used
						// in the parsing session) or they contain zeroes if this info is not known.
};

//
// This cache keeps a mixture of disk and temporary files. The search is always based on the compound doc_id.
//
class TSourceFilesTable : public THashTable<TSourceFilesTable, TSourceFilesTableItem, 0x100> { };

//  --------- Database reader object (abstract class)  ---------------

struct TSourceAreaBodyInfo
{
	TSourceArea			area;

	long					first_line_num;				// The line number where the source area starts.
	long					first_line_charpos;

	wchar_t				*fline_body_ptr;
	long					fline_body_len;				// Length of the part of the area that belongs to its first line or the length
													// of the whole area if the area does not span over several lines.

	long					last_line_num;				// Line number where the source area ends.

	inline void Clear() { memset(this, 0, sizeof(TSourceAreaBodyInfo)); }
};

//
//	Mid level database READER object.
//
class TMidLevStoreReader
{
public:

			TMidLevStoreReader(bool allow_other_disk_file_revisions = TRUE, bool recheck_cached_disk_file_revisions = TRUE);
	virtual	~TMidLevStoreReader();

	//
	//  Part 1a. Top level objects direct access.
	//

	virtual  bool	GetCompilationInfo			(ID parsing_id, bool silent_mode = FALSE) = 0;
	virtual  bool	GetLatestCompilationInfo		(bool silent_mode = FALSE) = 0;
					// It is possible to get info about the latest compilation by specifying the parsing_id equal to 0.

	virtual  bool	GetFileInformation				(ID pars_id, ID file_id, bool want_fbody_and_linfo = FALSE, bool *file_version_mismatch_flag = NULL, bool silent_mode = FALSE) = 0;
					// If the file body and the lines info are requested, then the disk file is placed into the cache of the reader.
					// Temp file may be placed, and may not be placed, depending on the MidStore implementation.
					// Method FileDataPtr() always returns a persistent pointer. It is not NULL only when the file body and
					// lines info were requested. The name of the file is cached in any case.

	virtual  bool	CheckFileInfoPresence		(ID pars_id, ID file_id) = 0;
					// This method returns TRUE if the info about the requested file is present in the database. Method is not
					// showing messages in the case of error.

	virtual  int	GetNumDiskFiles				(bool want_files_array_len = FALSE) const = 0;

	virtual  bool	GetIndexingKeyInfo			(ID key_id) = 0;

	virtual  bool	DeleteParsingSession		(ID parsing_id) = 0;

	//
	//  Part 1b. Iterating top level objects.
	//

	virtual  bool	StartBatchesIteration			() = 0;
					// Current implementation will iterate only the mid lev batches, not the raw batches.

	virtual  bool	StartCompilationsIteration	(ID batch_id = 0) = 0;
	virtual  bool	StartFilesIteration				(bool show_disk_files, bool show_temp_files, ID pars_id_restr = 0,
												bool want_file_body_and_lines_info = FALSE, bool only_with_raw_indexing_info = FALSE) = 0;
	virtual  bool	StartIndexingKeysIteration	() = 0;
					// Iteration gives out complete contents of the tables except for compilations.
					// This table allows to iterate only compilations that belong to certain batch.

	virtual  bool	NextBatchId					(ID &id) = 0;
	virtual  bool	NextCompilationRecord		() = 0;
	virtual  bool	NextFileRecord				() = 0;
	virtual  bool	NextIndexingKeyRecord		() = 0;

	virtual  bool	CloseBatchesIteration			() = 0;
	virtual  bool	CloseCompilationsIteration	() = 0;
	virtual  bool	CloseFilesIteration				() = 0;
	virtual  bool	CloseIndexingKeysIteration	() = 0;

	CtvCompilationInfo		&CompilationData()	{ return(m_ci); }
	TFileInfo				&FileData()				{ return(m_fi); }
	TFileInfo				*FileDataPtr()			{ return(m_pfi); }
	TKeyInfo				&IndexingKeyData()	{ return(m_ki); }

	//
	//  Part 2a. Parsing events direct access.
	//
	//  Iteration of all types of events in one iteration is not available in SQL implementation.
	//
	//	   Note:	Picking up info about the temp file does not allow to retrieve persistent pointer to the info on this file.
	//			The same applies to parsing events iteration where temp file registration event is part of the iteration.
	//			This can be achieved only by using the GetFileInformation()/StartFilesIteration() API.
	//

	virtual  bool	CheckEventPresence				(ID parsing_id, ID event_id) = 0;
					// This method shows errors only if the parsing id is bogus or if passed event id is out of range.
					// If params are correct but event is missing, then this method silently returns FALSE.

	virtual  bool	GetParsingEvent					(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetDriveSubstEvent				(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetTempFileInfo					(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetPasteBufferingChgEvent		(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetLexSourceEnterEvent		(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetLexSourceExitEvent			(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetMacroLibraryChangeEvent	(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetCondParsingChangeEvent	(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetMiscPreprocStmtEvent		(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetIncludePathRegEvent		(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetErrorWarningEvent			(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetCondParamUseEvent			(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetGrammarSectionEvent		(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetGrammarObjectEvent		(ID parsing_id, ID event_id) = 0;
	virtual  bool	GetCppItemRefEvent				(ID parsing_id, ID event_id) = 0;
					// Both params in all methods of the group above should be non zero.

	bool		GetLexSrcEnterEventAllowNullEventId		(ID parsing_id, ID event_id);
	bool		GetCondParsChgEventAllowNullEventId	(ID parsing_id, ID event_id);
				// Both methods allow calls with parsing_id != 0 and event_id == 0.
				// In this case they do not call the backend but generate the data internally.

	//
	//  Part 2b. Looking for parent events.
	//

			bool		GetSourceRefEvent		(ID parsing_id, ID event_id);
	virtual	bool		GetSourceRefEvent		(ID parsing_id, long linear_offs) = 0;
			bool		GetSourceRefEvent		(CtvParsingEventInfo &event);
			bool		GetCondRefEvent		(ID parsing_id, ID event_id);
	virtual	bool		GetCondRefEvent		(ID parsing_id, long linear_offs) = 0;
			bool		GetCondRefEvent		(CtvParsingEventInfo &event);
						// If the parameters are correct and the corresponding event sits on top of the hierarchy,
						// return value is TRUE and &EventData() contains empty event.
public:

	//
	//  Part 2c. Events misc.
	//

	virtual	bool		GetParsingEventsArray(CtvParsingEventPtrsArray &buffer,
											ID parsing_id, PSET_TYPES_MASK evt_types_mask,
											ID min_event_id = 0, ID max_event_id = 0) = 0;
						// This method may not be supported by some implementation of the interface.
						// Old contents of the buffer are discarded. All pointers are owned by the database
						// object and they will be released automatically when the database will be destroyed.

	//
	//  Part 3a. Iterating parsing events.
	//
	//  The source_ref param can be zero. This means that there is no source_ref restriction. In a similar
	//  way the min_event_id and max_event_id restirct the range of events that should be returned.
	//  If they are ommited, this means that there is no restriction on the value of the event ids.
	//
	//    NB:	Once the max_evid is passed, then object with this id will be included into the iteration.
	//		This is not the first id that stays out of the requested range.
	//
	//    NB:	Success of the iteration start means that connection to the database is ok and that state
	//		of the db object is fine. This tells nothing if the requested data set is empty or not.
	//

	virtual  bool  StartParsingEventsIteration				(ID parsing_id, PSET_TYPES_MASK evt_types_mask, ID source_ref = 0, ID min_evid = 0, ID max_evid = 0) = 0;

	virtual  bool  StartDriveSubstsIteration					(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartTempFileRegsIteration				(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartPasteBufferingChgsIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartLexSourceEntersIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartLexSourceExitsIteration				(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;

	virtual  bool  StartMacroLibraryChangesIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartCondParsingChangesIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartMiscPreprocStmtsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartIncludePathRegsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartErrorWarningEventsIteration		(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartCondParamsUseIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;

	virtual  bool  StartGrammarSectionsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartGrammarObjectsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;
	virtual  bool  StartCppItemRefEventsIteration			(ID parsing_id, ID source_ref = 0, ID min_event_id = 0, ID max_event_id = 0) = 0;

	virtual  bool  StartCondParsChgCondRefsIteration	(ID parsing_id, ID cond_ref, ID min_event_id = 0, ID max_event_id = 0, bool ascending_iter_order = TRUE) = 0;
					// This method is similar to the StartCondParsingChangesIteration(...) in the sense that it returns only
					// the cond pars change events. Although it differs from that method in the possible types of restrictions.
					// Note that cond ref param equal to zero is still a restriction that instructs to iterate events only in
					// the root area. This method does not allow iteration without cond ref restriction.

	virtual  bool  NextParsingEvent							() = 0;

	virtual  bool  NextDriveSubstEvent						() = 0;
	virtual  bool  NextTempFileRegsEvent					() = 0;
	virtual  bool  NextPasteBufferingChgsEvent			() = 0;
	virtual  bool  NextLexSourceEntersEvent				() = 0;
	virtual  bool  NextLexSourceExitsEvent				() = 0;
	virtual  bool  NextMacroLibraryChangesEvent			() = 0;
	virtual  bool  NextCondParsingChangesEvent			() = 0;
	virtual  bool  NextMiscPreprocStmtsEvent				() = 0;
	virtual  bool  NextIncludePathRegEvent				() = 0;
	virtual  bool  NextErrorWarningEventsEvent			() = 0;
	virtual  bool  NextCondParamsUseEvent				() = 0;

	virtual  bool  NextGrammarSectionEvent				() = 0;
	virtual  bool  NextGrammarObjectEvent				() = 0;
	virtual  bool  NextCppItemRefEvent						() = 0;

	virtual  bool  NextCondParsChgCondRefEvent			() = 0;

	virtual  bool  CloseParsingEventsIteration				() = 0;

	virtual  bool  CloseDriveSubstsIteration				() = 0;
	virtual  bool  CloseTempFileRegsIteration				() = 0;
	virtual  bool  ClosePasteBufferingChgsIteration		() = 0;
	virtual  bool  CloseLexSourceEntersIteration			() = 0;
	virtual  bool  CloseLexSourceExitsIteration			() = 0;
	virtual  bool  CloseMacroLibraryChangesIteration		() = 0;
	virtual  bool  CloseCondParsingChangesIteration		() = 0;
	virtual  bool  CloseMiscPreprocStmtsIteration			() = 0;
	virtual  bool  CloseIncludePathRegsIteration			() = 0;
	virtual  bool  CloseErrorWarningEventsIteration		() = 0;
	virtual  bool  CloseCondParamsUseIteration			() = 0;

	virtual  bool  CloseGrammarSectionsIteration			() = 0;
	virtual  bool  CloseGrammarObjectsIteration			() = 0;
	virtual  bool  CloseCppItemRefEventsIteration			() = 0;

	virtual  bool  CloseCondParsChgCondRefsIteration	() = 0;

	//
	//  Part 3b. Iterating file/pars references and key uses in raw files.
	//

	virtual  bool  StartFileParsRefsIteration					(ID pars_id, ID file_id) = 0;
	virtual  bool  StartFileIndexingSessionsIteration		(ID pars_id, ID file_id, ID key_id = 0) = 0;
	virtual  bool  StartKeyFileRefsIteration					(ID key_id) = 0;
	virtual  bool  StartKeyParsRefsIteration				(ID key_id) = 0;

	virtual  bool  NextFileParsRefsParsingId				(ID &id) = 0;
	virtual  bool  NextFileIndexingSessionRecord			(CtvRawIndexingKeyUseInfo &info) = 0;
	virtual  bool  NextKeyFileRefRecord					() = 0;
	virtual  bool  NextKeyParsRefRecord					() = 0;

	virtual  bool  CloseFileParsRefsIteration				() = 0;
	virtual  bool  CloseFileIndexingSessionsIteration		() = 0;
	virtual  bool  CloseKeyFileRefsIteration					() = 0;
	virtual  bool  CloseKeyParsRefsIteration				() = 0;

	CtvParsingEventInfo	&EventData() { return(m_event); }
	CtvKeyFileRefInfo		&KeyFileRefData() { return(m_kf_ref); }
	CtvKeyParsRefInfo		&KeyParsRefData() { return(m_kp_ref); }

	//
	//  Part 4a. Misc and support methods.
	//

	TMidLevStoreReader			*CreateSecondaryStore();
									// If this method returns non NULL object, this object inherits the data source and the error callback from
									// the primary store. When the primary store is released, all allocated secondary stores are automatically
									// destroyed. Secondary store cannot live longer than its primary store. This method may return a cached
									// store.

	void							ReleaseSecondaryStore(TMidLevStoreReader *qp) { RemoveFromRegistrationList(qp, TRUE); }
									// Method returns secondary store to the cache or releases it.

	virtual TMidLevStoreReader	*AllocateNewSecondaryStore() = 0;
									// This internal worker method should create object, do the app level initialization and it should not place
									// this object into any list. This method is expected to be called from inside of CreateSecondaryStore().
									// Note that data store reader should allow deletion using simple operator delete.

	void			SetupErrorCallback(TGenericConsole *cons) { m_reader_console = cons; }

	bool			CheckUserCancel() { assert(m_reader_console != NULL); return(m_reader_console->GetAbortFlag()); }
					// The return value is TRUE if user has decided to cancel the operation.

	void			SetUserCancel(bool new_val) { assert(m_reader_console != NULL); m_reader_console->SetAbortFlag(new_val); }
	int			ShowError(const wchar_t *message, int buttons = TGenericConsole::CONS_BTN_OK, bool inc_errors_count = TRUE)
											{ assert(m_reader_console != NULL); return(m_reader_console->ShowRunTimeError(message, buttons, inc_errors_count)); }

	//
	//  Part 4b. Source location support.
	//

	bool			GetSourceLocationInfo(TSourceLocation &buffer, ID parsing_id, ID event_id, TMidLevScanner *active_sess_scanner = NULL);
	bool			GetSourceLocationInfo(TSourceLocation &buffer, ID parsing_id, TLinearArea &lin_area, TMidLevScanner *active_sess_scanner = NULL);
	inline bool	GetSourceLocationInfo(TSourceLocation &buffer, CtvParsingEventInfo &event, TMidLevScanner *active_sess_scanner = NULL) { return(GetSourceLocationInfo(buffer, event.hdr.parsing_id, event.hdr.area, active_sess_scanner)); }
					//
					// If the parsing session is currently active and the proper scanner is not passed, then an error message is displayed
					// and the return value is FALSE. Besides that, an error message is displayed when the passed linear area sticks outside
					// of the valid range of the linear adresses. Return value in this case is also FALSE. These methods require that
					// beginning of the linear area should belong to some source area. If this is not so, return value is FALSE even
					// if some part of the linear area does have a source code location. Methods return data only about the first
					// segment of the passed linear areas in case if the linear areas includes several source areas.
					//
					// In case of the partial source area, the linear area, that is returned in the buffer is shorter than the orignal lin area
					// that was passed in as a parameter (directly or indirectly).
					//

	bool			GetSourceAreaBodyInfo(TSourceAreaBodyInfo &buffer, TSourceAreaInfo &area, bool *file_version_mismatch_flag = NULL, bool silent_mode = FALSE);
					// Method checks the cache, looks in the database, loads the body of the file and finds the line that corresponds to
					// the beginning of the area. In case of success all fields of the second param are filled in with valid non NULL data.
					// Param diff_file_version_flag shows if correct version of the file was retrieved or not.

	long			GetSourceAreaLineBeg(TSourceAreaInfo area);
					// Method returns the line number of the beginning of the passed source area. When the input params are bogus or
					// in the case of failure, the return value is -1.

	//
	//  Part 4c. Direct cache handling.
	//

	wchar_t		*RegisterString(const wchar_t *stmt_name, short param_inx, const wchar_t *str_data);
					// First two params are used only for OOM reporting.

	//
	//	Part 4d. Test and integrity checks.
	//

	bool			CheckParsingSessionConsistency(ID parsing_id);
					// When the passed parsing Id is equal to zero, this means that the latest parsing session should be checked.

protected:

	virtual bool	GetSourceLocationInfoInternal(TSourceLocation &buffer, ID parsing_id, TLinearArea &lin_area, long max_linear_offs) = 0;

	void			PrepareEmptyLexSrcEnterEvent(ID parsing_id);
	void			PrepareEmptyCondParsChangeEvent(ID parsing_id);

	TFileInfo		*LookupCachedFile(ID parsing_id, ID file_ident, bool *file_version_mismatch, bool &cache_item_present);
	TFileInfo		*CacheDiskFile(TFileInfo &info, bool *file_version_mismatch);
	TFileInfo		*CacheTempFile(TFileInfo &info);
	TFileInfo		*AddFileItemToCache(TSourceFilesTableItem *fi);

	int			ShowFileRevisionQuestion(TSourceFilesTableItem *fi, bool item_from_cache);
					// Possible return values: IDYES, IDNO, IDRETRY, CANCEL.

	void			ShowFileLoadingError(TFileInfo &info, TLoadSaveResult load_res);
	void			ShowDataContentsError(const wchar_t *message);

	void			RemoveFromRegistrationList(TMidLevStoreReader *qp, bool add_to_look_aside_list);

protected:

	typedef TStructsArray<TMidLevStoreReader*, 16, 32> TMidLevSecStorePtrsArray;
	typedef TLookAsideTable<TMidLevStoreReader, 12>  TMidLevSecStoresLookAside;

	TMidLevStoreReader			*m_primary_store;
	TMidLevSecStorePtrsArray		m_allocated_secondary_stores;
	TMidLevSecStoresLookAside		m_secondary_stores_look_aside;
									// Current model is not allowing the trees of the child stores. Primary store has its
									// m_primary_store member equal to NULL. All secondary stores should be direct
									// discendants from the primary store.

	CtvCompilationInfo				m_ci;
	TFileInfo						m_fi;
	TFileInfo						*m_pfi;				// This field contains a persistent pointer if it is not NULL.
	TKeyInfo						m_ki;

	CtvParsingEventInfo			m_event;
	CtvKeyFileRefInfo				m_kf_ref;
	CtvKeyParsRefInfo				m_kp_ref;

	TSourceFilesTable				m_files_cache;
									// This cache contains info about all disk files ever queried. Temp files are stored in this cache
									// only if derived class is not providing a persistent ptrs for temp files.

	TStringsDict					*m_pStringsDict;
									// Derived class should provide storage for this dictionary.

	bool							m_allow_other_revisions;
									// When this field is FALSE, incorrect revisions cannot be placed into the cache. If this field
									// is TRUE, they are placed only after confirmation from the user.

	bool							m_recheck_cached_files;
									// This option instructs instance of the class to reverify date/len/crc of the cached file each
									// time before returning it to the upper layer.
private:

	TGenericConsole				*m_reader_console;
};

#endif	// TextScan_MidStore_H


