//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_SyntaxHighlight_H
#define	LangView_SyntaxHighlight_H

#ifndef   Xml_HtmlGenDecors_H
#include  "Xml/HtmlGenDecors.H"
#endif
#ifndef   TextScan_MidStore_H
#include  "TextScan/MidStore.H"
#endif

//
// Index for picking up the whole file display styles. Only the style that corresponds to the first enum member
// (wfsi_whole_file) is used as a whole. Other styles are used in parts for creating different types of decorations.
//
enum TSrcViewWholeFileStyleInx
{
	wfsi_whole_file,
	wfsi_out_of_scope,
	wfsi_aborted_scan,
	wfsi_unscanned,							// Unscanned area in the scenario with an active scanner.
	wfsi_num_types,
};

// These style indexes describe the text styles.
enum TSrcViewCmdlLexemaStyleInx
{
	rcmsi_comment,
	rcmsi_key_key,
	rcmsi_key_val,
	rcmsi_pos_param,
	rcmsi_pos_param_inx,					// Small number that shows the index of the pos parameter.
	rcmsi_raw_scan_error,					// Source viewer highlights the raw scanning errors only when it displays the parsing
											// indepenedent files. During the parsing session all raw scanning errors are always
											// reported to the callback. This means that they will be displayed as mid lev events.
	rcmsi_num_types,
};

// These style indexes describe the text styles.
enum TSrcViewCLangLexemaStyleInx
{
	rclsi_cmnt_c_style,
	rclsi_cmnt_c_style_big,
	rclsi_cmnt_cpp_eol,
	rclsi_cmnt_cpp_whl,
	rclsi_cmnt_csh_trs,

	rclsi_number,							// Integer and floating point constants.

	rclsi_chconst_a,
	rclsi_chconst_w,
	rclsi_string_a,
	rclsi_string_w,
	rclsi_string_inc_fn,						// String in the context of the include file statement.
	rclsi_string_spec,						// Neutral form of the string with the transparent bacground.

	rclsi_tkn_separ,
	rclsi_tkn_arithm,
	rclsi_tkn_preproc,
	rclsi_tkn_clang,
	rclsi_tkn_cpp,
	rclsi_tkn_csh,

	rclsi_name,

	rclsi_raw_scan_error,					// Source viewer highlights the raw scanning errors only when it displays the parsing
											// indepenedent files. During the parsing session all raw scanning errors are always
											// reported to the callback. This means that they will be displayed as mid lev events.
	rclsi_num_types,
};

//
// These indexes describe either an inline fragment or the whole line bkgr styles.
//
// Note that background decors (both inline and whole line) can be invisible but still clickable.
//
enum TSrcViewMidLevBkgrStyleInx
{
	mlbsi_mpr_stmt_bkgr,					// Transparent background.

	mlbsi_paste_buff_area,
	mlbsi_macro_call_area,

	mlbsi_warning_bkgr,
	mlbsi_error_bkgr,

	mlbsi_num_types,
};

//
// These indexes describe the text styles.
//
enum TSrcViewMidLevFrgrStyleInx
{
	mlfsi_mpr_stmt_token,

	mlfsi_include_fname,					// File name in the #include statement.
	mlfsi_mcall_macro_name,				// Name of the macro in the macro definition call.
	mlfsi_mcall_mparam_name,				// Area of the mparam call. This may happen only inside the macro.

	mlfsi_macro_def_name,					// Name of the macro in the macro def/undef statement.

	mlfsi_cmdl_app_name,
	mlfsi_cmdl_mj_src_file,

	mlfsi_mprep_blocked_stmt,
	mlfsi_mprep_failed_inc_fn,
	mlfsi_mprep_other_fields,

	mlfsi_cond_param_use,					// Conditional parsing statement parameter.

	mlfsi_grammar_section,					// Name of the section in the grammar definition file.
	mlfsi_grammar_rule_nterm,				// Non terminal in the left hand side of the grammar rule.
	mlfsi_grammar_xpct_name,				// Name of the expected conflict.

	mlfsi_mdef_token,						// Macro header element inside the mcall enclosure.
	mlfsi_mdef_name,						// Macro header element inside the mcall enclosure.
	mlfsi_mdef_delim,						// Macro header element inside the mcall enclosure.
	mlfsi_mdef_param,						// Macro header element inside the mcall enclosure.
	mlfsi_mdef_other,						// Macro header element inside the mcall enclosure.

	mlfsi_num_types,
};

//
// Note:	Decors with indexes spcdi_error_whole_line_mark and spcdi_warning_whole_line_mark
//		are the whole line imposed decors. All other decors are the inline imposed decors.
//
enum TSrcViewSpacelessDecorInx
{
	spcdi_none,

	spcdi_drive_subst_event_mark,
	spcdi_temp_file_reg_mark,

	spcdi_paste_buff_beg_mark,
	spcdi_paste_buff_end_mark,

	spcdi_pbuff_at_lex_src_beg,				// Lex source starts with an active paste buffering.
	spcdi_pbuff_at_lex_src_end,				// Lex source ends with an active paste buffering.
	spcdi_pbuff_ends_in_call,				// Paste buffering ends inside the local lex src call.
	spcdi_pbuff_starts_after_call,			// Paste buffering starts after the local lex src call.
	spcdi_pbuff_src_start_into_call,			// Lex source starts with active buffering and immediately sinks into a local call.
	spcdi_pbuff_out_of_call_src_end,			// Paste buffering starts after the local call and immediately exits the lex source.

	spcdi_ant_env_call_mark,				// Entering the analysis tool profile.
	spcdi_bltin_defs_call_mark,				// Entering the file with built in definitions.
	spcdi_mj_src_call_mark,					// Entering the major source file of the parsing session.
	spcdi_scgm_call_mark,					// Self schanging macro call mark.
	spcdi_paste_buff_call_mark,				// Entering the paste buffering buffer.
	spcdi_bad_spaceless_call_mark,			// This mark is used when a regular lex src call has zero length of the call area.

	spcdi_warning_area_end_mark,
	spcdi_error_area_end_mark,
	spcdi_warning_whole_line_mark,			// <-- Whole line decoration.
	spcdi_error_whole_line_mark,			// <-- Whole line decoration.

	spcdi_curr_layer_scan_pos,				// This is an inactive area decoration icon.
	spcdi_lower_layer_scan_pos,				// This is an inactive area decoration icon.
	spcdi_spec_loc_marker,					// Current scanning position in the visual parser.
	spcdi_spaceless_object_slct,

	spcdi_num_types,
};

//
// Types of decorations that may be present on the source file view.
//
enum TSrcViewDecorOwnerType
{
	svdo_out_of_scope_code,			// The code that does not belong to the current parsing scope. In other words this an area of
										// the file that was not intended to be scanned when the lex source enclosure was entered.
	svdo_aborted_scan_code,			// The code that belongs to the parsing scope but was not scanned because the parsing process
										// was aborted before scanning has reached this area of the file.
	svdo_unscanned_code,				// The code that belongs to the parsing scope but was not scanned because the parsing session
										// is currently active and the scanning has not reached this area of the file yet.

	svdo_code_area,					// Every cond parsing code area including the initial one, is described with this decoration.
										// Note that the code area is not the same to the cond parsing area defined with the cond pars
										// change event. Code area defines consequtive part of one file.
	svdo_buffering_area,				// Part of code that belongs to the preproc buffering area. Note this is area where data for
										// buffering was collected, not the area where results of the buffering are scanned.
										// For now the decor owner object contains only the BUFFERING ENTER event.
	svdo_parsing_event,					// Every parsing event that belongs to the current lex src enclosure is described with an object
										// of this type. The high level language objects are described indirectly using the proxy mid lev
										// events.

	svdo_cmdl_lexema,					// Raw lexema from the file with the command line syntax. All raw objects share the same
										// decoration owner object. Info on the particular lexema is stored in the owner data field
										// of the decorating screen item.
	svdo_clang_lexema,					// Raw lexema from the file with the C/C++ syntax. All raw objects share the same decoration
										// owner object. Info on the particular lexema is stored in the owner data field of the screen
										// item.

	svdo_curr_scan_pos,					// This owner type is used for displaying the current scanning position in the visual parser and
										// in other scenarios that include active scanner.
	svdo_macro_hdr_object,				// Member of the macro definition header. This object can be present only in the macro defn
										// invocation enclosure.

	svdo_app_decor_types_base,
};

//
// Contents of the "owner_data" field of the command line raw objects.
//
enum TSrcViewCmdlLexemaOwnerData
{
	cmod_pos_prm_val_shift		=               20,
	cmod_pos_prm_val_mask	= 0xFFF00000,
	cmod_scan_pos_mask		= 0x000FFFFF,
};

//
// Contents of the "owner_data" field of the C/C++ raw objects. The lexema is reconstructed by scanning
// from the saved offset. The length of the area is not specified. The code expects that the RawScanner will
// return the same lexema as it was returned during the decoration process.
//
enum TSrcViewCLangLexemaOwnerData
{
	clod_new_line_stt			= 0x80000000,		// This field shows the state of the new line flag in the raw scanner
													// when this lexema was scanned.
	clod_inc_fname_stt			= 0x40000000,		// This field shows the state of the inc fname flag in the raw scanner
													// when this lexema was scanned.
	clod_scan_pos_mask			= 0x0FFFFFFF,
};

//
// Contents of the "owner_data" field of the MacroHdr objects. The data consists of 2 fields. Both fields
// are always present. Lower 3 bytes contain the src offset field. Higher byte contains the purpose field.
//
enum TSrcViewMacroHdrObjectOwnerData
{
	mhod_mdef_token			= 0xFF000000,
	mhod_mdef_name			= 0xFE000000,
	mhod_mdef_delim			= 0xFD000000,
	mhod_mdef_other			= 0xFC000000,		// Lexema of the header that is not a token, name, param and it is not a delim.
	mhod_param_inx_shift		=               24,
	mhod_func_part_mask		= 0xFF000000,
	mhod_scan_pos_mask		= 0x00FFFFFF,
};

class TSourceViewDecorOwner : public THtmlGenAppDelegate
{
public:

		TSourceViewDecorOwner(TSrcViewDecorOwnerType otp);
		TSourceViewDecorOwner(TSrcViewDecorOwnerType otp, TSourceAreaInfo &area, CtvParsingEventInfo &event);

		inline void	ClearEventData() { m_src_area.Clear(); m_event_data.Clear(); }

		virtual void	EmitSpecificAttrs(THtmlDocument &doc);

protected:

		//
		//  Meaning of the data fields for the different types of the owner objects.
		//
		//			type					   src_area			    event_data			 owner_data
		//  ------------------------------------------------------------------------------------------
		//
		//		svdo_out_of_scope_code	     zeroes				zeroes				n/a
		//		svdo_aborted_scan_code	     zeroes				zeroes				n/a
		//		svdo_unscanned_code		     zeroes				zeroes				n/a
		//
		//		svdo_code_area			  contig_area(*)	    cond_area_intro_event(**)	n/a
		//		svdo_buffering_area		  contig_area		     start_buffering_event		n/a
		//		svdo_parsing_event		whole_event_area		  mid_lev_event			n/a
		//
		//		svdo_cmdl_lexema			     zeroes				zeroes			TSrcViewCmdlLexemaOwnerData
		//		svdo_clang_lexema		     zeroes				zeroes			TSrcViewCLangLexemaOwnerData
		//
		//		svdo_curr_scan_pos		     zeroes				zeroes				n/a
		//		svdo_macro_hdr_object		whole_mdef_area(***)	mdef_intro_event		TSrcViewMacroHdrObjectOwnerData
		//
		//  (*)	Note that the src area in the code area decoration describes the area in the current file. This area has nothing to do
		//		with the area where the intro cond pars statementis located.
		//
		//		There are gaps betweeen the src areas of the adjacent code areas. These gaps contain the CR LF characters that
		//		are present between the lines that belong to different areas.
		//
		// (**)	Code area intro event can be empty (only the parsing_id field is filled and a few other fields) when the code area
		//		belongs to the root of the cond parsing structure.
		//
		// (***)	Source area of the whole macrodef statement.
		//

	TSrcViewDecorOwnerType		m_type;
	TSourceAreaInfo				m_src_area;
	CtvParsingEventInfo			m_event_data;

	friend class TSourceViewDecorOwnersContext;
	friend class TLangViewSourceFileScreenItem;
	friend class TLangViewSourceFileDialog;
	friend class THtmlSourceCodeFile;
	friend class TSyntaxHighlighter;
};

class TSourceViewDecorOwnersContext
{
public:

	TSourceViewDecorOwnersContext(int &cnt_owner_creation_errors);
	~TSourceViewDecorOwnersContext() { ResetContext(); }

	void		ResetContext();

	void		SetupMacroHeaderInfo(TSourceAreaInfo &area, CtvMacroLibraryChangeEvent &mdef_info) { m_macro_header_owner.m_src_area = area; m_macro_header_owner.m_event_data.mdef_chg = mdef_info; }

	TSourceViewDecorOwner		*CreateDecorOwner(TSrcViewDecorOwnerType owner_type, TSourceAreaInfo &area, CtvParsingEventInfo &event);

	TSourceViewDecorOwner		*GetInactiveAreaDecorOwner(TSrcViewWholeFileStyleInx style_inx);
	TSourceViewDecorOwner		*GetCurrScanPosOwner() { return(&m_curr_scan_pos_owner); }

protected:

	static int	CompareDecorOwners(const TSourceViewDecorOwner *owner1, const TSourceViewDecorOwner *owner2);
				// This method should be called only for objects that allow multiple instantiation.

protected:

	int									&m_owner_creation_errors;

	TList									m_decor_owners;
											//
											// This list keeps the TSourceViewDecorOwner objects. They can have only the following types:
											//
											//		svdo_code_area, svdo_buffering_area, svdo_parsing_event.
											//

	TSourceViewDecorOwner				*m_latest_decor_owner;
											//
											// The list of owners is ordered by the source offsets of the event areas. The pointer to the latest
											// owner is used for speeding up the insertion of the new owner objects into the list. This is needed
											// because events are iterated in the order of increasing of their event ids, not their src position.
											//

	TSourceViewDecorOwner				m_out_of_scope_code_owner;
	TSourceViewDecorOwner				m_aborted_scan_code_owner;
	TSourceViewDecorOwner				m_unscanned_code_owner;

	TSourceViewDecorOwner				m_cmdl_lexemas_owner;
	TSourceViewDecorOwner				m_clang_lexemas_owner;
	TSourceViewDecorOwner				m_curr_scan_pos_owner;

	TSourceViewDecorOwner				m_macro_header_owner;
											// This field is used only in the macro definition enclosures. This object also keeps info about
											// the macro definition event.
	friend class TSyntaxHighlighter;
	friend class TLangViewSourceFileScreenItem;
	friend class THtmlSourceCodeFile;
};

enum TSrcViewDecorZorder
{
	svzo_default,					//  0
	svzo_inactive_area,				//  1
	svzo_code_status,				//  2

	svzo_mpr_stmt_bkgr,			//  3
	svzo_cpp_stmt_bkgr,			//  4
	svzo_paste_buff_bkgr,			//  5
	svzo_mcall_stmt_bkgr,			//  6

	svzo_error_warning,				//  7
	svzo_cpp_bkgrs_base,			//  8
	svzo_cpp_bkgrs_end = 31999,

	svzo_raw_object,				//  32000
	svzo_mid_lev_frgr,				//  32001
	svzo_cpp_item_frgr,				//  32002

	svzo_click_event_slct,			//  32003
};

enum TSrcViewSpacelessHorzOrder
{
	svho_err_warn_mark,

	svho_icon_ids_base,
	svho_icon_ids_end = svho_icon_ids_base+spcdi_num_types,

	svho_default,
};

class TSyntaxHighlightDocumentBuilder
{
public:

	//----- Decors group 1. Code areas that were not scanned. -----

	virtual void	ApplyFileViewStyleToUpperArea(TSrcViewWholeFileStyleInx view_style_inx, long upper_area_end, bool whole_line_area_style) { }
	virtual void	ApplyFileViewStyleToLowerArea(TSrcViewWholeFileStyleInx view_style_inx, long lower_area_beg, bool whole_line_area_style) { }
	virtual void	ApplyUnscannedAreaStyle(TSrcViewWholeFileStyleInx view_style_inx, long area_beg, long area_end, bool area_end_whole_line_style) { }
	virtual void	ApplySpecialNumsMarksBackgrounds(long src_pars_beg_offs, long src_pars_end_offs, bool whole_line_area_style) { }
	virtual void	SetupUnscannedAreaEntranceIcon(long area_entrance_offs, bool lower_layer_icon) { }
	virtual void	SetupUnscannedLineEndArea(long real_src_active_end) { }

	//----- Decors group 2. MidLev database, paste buffering areas and Cpp database objects. -----

	virtual void	ApplyMidLevBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevBkgrStyleInx style_inx, TSourceAreaInfo &src_area) { }
	virtual void	ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceAreaInfo &src_area) { }
	virtual void	ApplyMidLevWlbkDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, long line_beg, long num_lines) { }

	virtual void	ApplyCppObjectBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSourceAreaInfo &src_area) { }
	virtual void	ApplyCppObjectFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TCppSourceAreaType area_type, TSourceAreaInfo &src_area) { }

	//----- Decors group 3. Conditional parsing areas. -----

	virtual void	ApplyCodeStatusDecors(TSourceViewDecorOwner *owner, bool whole_line_beg_style, bool whole_line_end_style) { }
					// The owner param contains information about the area of the file that should be highlighted.

	//----- Decor groups 4. Raw lexemas and macrodef lex src enclosure header. -----

	virtual void	ApplyCmdlRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCmdlLexemaStyleInx style_inx, TSourceArea &area) { }
	virtual void	ApplyCLangRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCLangLexemaStyleInx style_inx, TSourceArea &area) { }
	virtual void	ApplyMacroHdrFrgrDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceArea &area) { }
					// Implementation should not expect that the "file_info" field in the area param contains valid information.

	//----- Support group. -----

	virtual void	ApplyInlineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder,
												TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long src_area_offs) { }
	virtual void	ApplyWholeLineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder,
												TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long line_number) { }

	virtual bool	GetVisibleArea(long &visible_area_beg, long &visible_area_len) = 0;
	virtual void	ReportProcessingProblem(const wchar_t *message) = 0;
};

class TSyntaxHighlighter
{
public:			//
				//	This is a transient helper object that should be created on the stack. It should be instantiated,
				//	used for decorating the file and immediately destroyed after that. This object is not expected
				//	to be reused.
				//
				//	NB:	The document builder is also used for reporting problems. This is important.
				//

		TSyntaxHighlighter(TSyntaxHighlightDocumentBuilder &doc_builder, TSourceViewDecorOwnersContext &decor_owners);
		~TSyntaxHighlighter();

	void		DecorateStandAloneFile(TFileInfo &file_info);
				// This method should be used for parsing independent files.

	bool		LoadLexScrEnclosureFile(CtvLexSourceEnterEvent &lex_src_call, TMidLevStoreReader *mldb_reader, TMidLevScanner *active_scanner = NULL);
				// This method setups the object. It should be called right after the ctor. Note that when the file is retrieved
				// from the database, it continues to be owned by the database but when the file is retrieved from the scanner,
				// it is duplicated and this copy is owned by the object.

	void		ExtractFileData(TFileInfo &file_info, bool &file_data_owned);
	void		ResolveLinesRangeToDisplay(int num_upper_rows, int num_lower_rows, int &line_from, int &num_lines);

	void		DecorateLexSrcEnclosure();
				// This method is the main execution method. Note that this method is void. Nevertheless it can report errors
				// plus the doc builder and the decors context object may accumulate their own errors.

	TMidLevStoreReader	&MldbReader() { return(*m_mldb_reader); }

public:

	void		SetupNonActiveAreaDecors();

	void		SetupMidLevEventDecors(ID pars_sess, ID src_encl_id, PSET_TYPES_MASK events_mask = PSET_ALL_TYPES);
				// In the normal situation the values of the passed parameters describe the current enclosure.
				// Nevertheless in custom scenarios sometimes it is needed to add decors from other parsing
				// sessions. It is up to the application to ensure that passed enclosure scans the current file.

	void		SetupPasteBuffAreaDecors();
	void		SetupCodeStatusDecors();

	void		SetupRawSyntaxDecors();
	void		SetupMacroHeaderDecors();

protected:

	//----- Decors group 1. Non active areas. -----

	long		GetRealActiveAreaEnd(bool &sess_running, bool &partial_active_area);

	//----- Decors group 2. Mid lev events and cpp object references. -----

	void		DecorateMidLevEvent(TSourceViewDecorOwner *decor_owner);
				// Note that the passed decor owner contains the source area of the parsing event.

	void		ApplyMidLevWholeLineBkgr(TSourceViewDecorOwner *decor_owner, TSourceAreaInfo &src_area);
	void		ApplyMidLevBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewMidLevBkgrStyleInx inx, TSourceAreaInfo &src_area);

	void		ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewMidLevFrgrStyleInx inx, TSourceAreaInfo &src_area);
	void		ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewMidLevFrgrStyleInx inx, TLinearArea &lin_area, long lin_src_delta);
	void		ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewMidLevFrgrStyleInx inx, TLinearArea &lin_area);

	void		ApplyCppObjectFrgrDecor(TSourceViewDecorOwner *decor_owner, TCppSourceArea cpp_area);

	//----- Decors group 3. Paste buffering areas. -----

	TSourceViewDecorOwner		*StartPasteBufferingArea(long src_area_beg, ID paste_buff_area_beg_event);
	TSourceViewDecorOwner		*StartPasteBufferingArea(long src_area_beg, CtvParsingEventInfo &area_beg_event);

	void		FinalizePasteBufferingArea(TSourceViewDecorOwner *area_owner, long src_area_end);

	// ----------- Group 4. Code status decors. --------------------

	void		ApplyMultiAreaCodeStatusProc(CtvCondParsingChangeEvent &initial_stt_event);
	void		ApplySingleAreaCodeStatusProc(CtvCondParsingChangeEvent &initial_stt_event);
	void		CreateCodeStatusArea(CtvCondParsingChangeEvent &intro_event, long area_beg, long area_end);

	void		SetupCmdlRawSyntaxDecors(TSourceAreaInfo &active_area, int next_pos_prm_index, TParseEnvInfo *pars_env_info, bool show_raw_errors);
	void		SetupClangRawSyntaxDecors(TSourceAreaInfo &active_area, bool stand_alone_file_mode, bool initial_inc_fn_mode = FALSE);
				// These methods are expected to be called after setting up the mid lev decorations because this process
				// creates the list of the decoration owners. This list of owners is needed both for the cmld and cpp cases.

	void		DecorateCLangLexema(TLexema &lex, bool stand_alone_file_mode, bool inc_fn_mode);
	void		DecorateMacroHdrLexema(DWORD owner_data_high_byte, TSrcViewMidLevFrgrStyleInx inx, long src_area_beg, long src_area_len);

	void		ApplyMarksAreaDecor(TSourceViewDecorOwner *decor_owner, TSrcViewSpacelessDecorInx inx, short horz_order, TSourceAreaInfo &src_area);
	void		ApplySpacelessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewSpacelessDecorInx inx, short horz_order, TSourceAreaInfo &src_area);

	bool		GetLocalSrcAreaInfo(TSourceAreaInfo &buffer, TLinearArea &lin_area, const wchar_t *area_descr, bool allow_partial_area = FALSE);
				// Method picks up the source area that corresponds to the linear area, ensures that it belongs to the current
				// file and that it does not stick out of the current file.

	TSourceViewDecorOwner		*GetFirstLocalLexSrcCall(bool want_paste_buffering_gap_call);
	TSourceViewDecorOwner		*GetNextLocalLexSrcCall(TSourceViewDecorOwner *owner, bool want_paste_buffering_gap_call);
									// These helper methods iterate available decor owners.

	void		ReportProblem(const wchar_t *format, ...);

protected:

	TSyntaxHighlightDocumentBuilder			&m_doc_builder;
	TSourceViewDecorOwnersContext		&m_decor_owners;

	TMidLevStoreReader					*m_mldb_reader;
	TMidLevScanner						*m_active_scanner;

	CtvLexSourceEnterEvent				m_lex_src_call;

	TFileInfo								m_src_file;
	bool									m_src_file_data_owned;

	TSourceAreaInfo						m_encl_active_area;
	TSourceAreaInfo						m_encl_code_stts_area;
	bool									m_encl_whole_line_active_area_end;
											// When the source file is picked up from the database, these fields are set up according
											// to the info in the database record. Later on, when the non active decorations are applied,
											// code stts area is extended for macro source enclosures and in case if the unscanned area
											// is present, both areas are reduced and the wl status of the end of the areas is set to FALSE.
private:

	bool									m_non_empty_lex_present;			// This field is needed for handling files that start with C-style comments.
																			// These comments are painted using a bigger font.
	friend class THtmlSourceCodeFile;
	friend class TLangViewSourceFileScreenItem;
};

#endif	// LangView_SyntaxHighlight_H


