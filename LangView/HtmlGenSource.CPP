//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Common/FormatNumber.H"
#include  "LangView/HtmlGenSource.H"
#include  "LangView/SpecTabHelpers.H"

// ---------------------------------------------------------------------------
//	===============  THtmlSourceCodeStyle  ========================
// ---------------------------------------------------------------------------

static const wchar_t *g_HTGS_HtmlTableStyle =
{
	L"src_code_table"
};

static const wchar_t *g_HTGS_WholeFileStyles[3*wfsi_num_types] =
{
	L"src_line_num",					L"src_marks_area",				L"src_line_body",					// wfsi_whole_file
	L"src_out_of_scope_line_num",		L"src_out_of_scope_marks_area",	L"src_out_of_scope_line_body",		// wfsi_out_of_scope
	L"src_aborted_scan_line_num",		L"src_aborted_scan_marks_area",	L"src_aborted_scan_line_body",		// wfsi_aborted_scan
	L"src_unscanned_line_num",			L"src_unscanned_marks_area",		L"src_unscanned_line_body",			// wfsi_unscanned
};

static const  wchar_t *g_HTGS_ConditionalParsingStatusStyles[3*cexr_num_types] =
{
	L"cps_inactive_nline",			L"cps_inactive_marks",				L"cps_inactive",			// cexr_hard_false
	NULL,						NULL,							NULL,					// cexr_hard_true
	L"cps_soft_inact_nline",			L"cps_soft_inact_marks",			L"cps_soft_inact",			// cexr_soft_false
	L"cps_soft_active_nline",		L"cps_soft_active_marks",			L"cps_soft_active",			// cexr_soft_true
	L"cps_undefined_nline",			L"cps_undefined_marks",			L"cps_undefined",			// cexr_undefined
};

static const wchar_t *g_HTGS_CmdlLexemaStyles[rcmsi_num_types] =
{
	L"cmdl_comment",				// rcmsi_comment
	L"cmdl_key_key",				// rcmsi_key_key
	L"cmdl_key_val",				// rcmsi_key_val
	L"cmdl_pos_param",			// rcmsi_pos_param
	L"cmdl_pos_param_inx",		// rcmsi_pos_param_inx
	L"cmdl_raw_scan_error",		// rcmsi_raw_scan_error
};

static const wchar_t *g_HTGS_CLangLexemaStyles[rclsi_num_types] =
{
	L"lct_c_style",					// rclsi_cmnt_c_style
	L"lct_c_style_big",				// rclsi_cmnt_c_style_big
	L"lct_cpp_endofline",			// rclsi_cmnt_cpp_eol
	L"lct_cpp_wholeline",			// rclsi_cmnt_cpp_whl
	L"lct_csh_trislash",				// rclsi_cmnt_csh_trs

	L"ltx_number",				// rclsi_number

	L"ltx_charconst",				// rclsi_chconst_a
	L"ltx_charconst",				// rclsi_chconst_w
	L"ltx_string",					// rclsi_string_a
	L"ltx_string",					// rclsi_string_w
	L"ltx_string_inc_fn",			// rclsi_string_inc_fn
	L"ltx_string_spec",				// rclsi_string_spec

	L"tkn_separ",					// rclsi_tkn_separ
	L"tkn_arithm",					// rclsi_tkn_arithm
	L"tkn_preproc",				// rclsi_tkn_preproc
	L"tkn_keyword",				// rclsi_tkn_clang
	L"tkn_keyword",				// rclsi_tkn_cpp
	L"tkn_keyword",				// rclsi_tkn_csh

	L"ltx_name",					// rclsi_name
	L"ltx_raw_scan_error",			// rclsi_raw_scan_error
};

static const wchar_t *g_HTGS_MidLevObjectBkgrStyles[mlbsi_num_types] =
{
	NULL,						// mlbsi_mpr_stmt_bkgr
	L"mlev_paste_buff_area",		// mlbsi_paste_buff_area
	L"mlev_macro_call_area",		// mlbsi_macro_call_area
	L"mlev_warning_bkgr",			// mlbsi_warning_bkgr
	L"mlev_error_bkgr",			// mlbsi_error_bkgr
};

static const wchar_t *g_HTGS_MidLevObjectFrgrStyles[mlfsi_num_types] =
{
	L"mlev_mpr_stmt_token",		// mlfsi_mpr_stmt_token

	L"mlev_include_fname",			// mlfsi_include_fname
	L"mlev_mcall_macro_name",		// mlfsi_mcall_macro_name
	L"mlev_mcall_mprm_name",		// mlfsi_mcall_mparam_name

	L"mlev_macro_def_name",		// mlfsi_macro_def_name

	L"mlev_cmdl_app_name",		// mlfsi_cmdl_app_name
	L"mlev_cmdl_mj_src_file",		// mlfsi_cmdl_mj_src_file

	L"mlev_mprep_blocked_stmt",	// mlfsi_mprep_blocked_stmt,
	L"mlev_mprep_failed_inc_fn",	// mlfsi_mprep_failed_inc_fn,
	L"mlev_mprep_other_fields",		// mlfsi_mprep_other_fields,

	L"mlev_cond_param_use",		// mlfsi_cond_param_use

	NULL,						// mlfsi_grammar_section
	NULL,						// mlfsi_grammar_rule_nterm
	NULL,						// mlfsi_grammar_xpct_name

	L"mlev_mdef_token",			// mlfsi_mdef_token
	L"mlev_mdef_name",			// mlfsi_mdef_name
	L"mlev_mdef_delim",			// mlfsi_mdef_delim
	L"mlev_mdef_param",			// mlfsi_mdef_param
	L"mlev_mdef_other",			// mlfsi_mdef_other
};

static const wchar_t *g_HTGS_CppObjectFrgrStyles[cxa_num_types] =
{
	NULL,						// cxa_none

	L"cxa_bright_keyword",			// cxa_bright_keyword
	L"cxa_namespace",			// cxa_namespace

	L"cxa_enum_fwd_decl",			// cxa_enum_fwd_decl
	L"cxa_struct_fwd_decl",			// cxa_struct_fwd_decl
	L"cxa_union_fwd_decl",			// cxa_union_fwd_decl
	L"cxa_class_fwd_decl",			// cxa_class_fwd_decl

	L"cxa_enum_defn",				// cxa_enum_defn
	L"cxa_struct_defn",				// cxa_struct_defn
	L"cxa_union_defn",				// cxa_union_defn
	L"cxa_class_defn",				// cxa_class_defn

	L"cxa_enum_member",			// cxa_enum_member
	L"cxa_data_field_xtrn",			// cxa_data_field_xtrn
	L"cxa_data_field_defn",			// cxa_data_field_defn
	L"cxa_function_proto",			// cxa_function_proto
	L"cxa_function_header",			// cxa_function_header

	L"cxa_nameless_bft",			// cxa_nameless_bft
	L"cxa_nameless_cvpr",			// cxa_nameless_cvpr
	L"cxa_nameless_array",			// cxa_nameless_array
	L"cxa_nameless_fntp",			// cxa_nameless_fntp
};

static const wchar_t *g_HTGS_SpacelessIconNames[spcdi_num_types] =
{
	NULL,						// spcdi_none
	L"DriveSubstEvent.GIF",			// spcdi_drive_subst_event_mark
	L"TempFileRegMark.GIF",		// spcdi_temp_file_reg_mark
	L"PasteBufferingBeg.GIF",		// spcdi_paste_buff_beg_mark
	L"PasteBufferingEnd.GIF",		// spcdi_paste_buff_end_mark

	NULL,						// spcdi_pbuff_at_lex_src_beg
	NULL,						// spcdi_pbuff_at_lex_src_end
	NULL,						// spcdi_pbuff_ends_in_call
	NULL,						// spcdi_pbuff_starts_after_call
	NULL,						// spcdi_pbuff_src_start_into_call
	NULL,						// spcdi_pbuff_out_of_call_src_end

	L"AntEnvCall.GIF",				// spcdi_ant_env_call_mark
	L"BuiltInDefnsCall.GIF",			// spcdi_bltin_defs_call_mark
	L"MjSrcCall.GIF",				// spcdi_mj_src_call_mark
	L"ScgmCall.GIF",				// spcdi_scgm_call_mark
	L"PasteBuffCall.GIF",			// spcdi_paste_buff_call_mark
	L"BogusSpacelessCall.GIF",		// spcdi_bad_spaceless_call_mark

	L"WarningInlineMark.GIF",		// spcdi_warning_area_end_mark
	L"ErrorInlineMark.GIF",			// spcdi_error_area_end_mark
	L"WarningEventIcon.GIF",		// spcdi_warning_whole_line_mark
	L"ErrorEventIcon.GIF",			// spcdi_error_whole_line_mark

	NULL,						// spcdi_curr_layer_scan_pos
	NULL,						// spcdi_lower_layer_scan_pos
	NULL,						// spcdi_spec_loc_marker
	NULL,						// spcdi_spaceless_object_slct
};

const wchar_t * THtmlSourceCodeStyle::GetHtmlTableCssStyle()
{
	assert(m_src_code_style_prepared == TRUE);
	return(g_HTGS_HtmlTableStyle);
}

void THtmlSourceCodeStyle::GetWholeFileBkgrStyles(TSrcViewWholeFileStyleInx style_inx, wchar_t *&line_lums_style, wchar_t *&marks_area_style, wchar_t *&line_body_style)
{
	assert(m_src_code_style_prepared == TRUE);
	assert(style_inx >= 0 && style_inx < wfsi_num_types);
	line_lums_style = (wchar_t*)g_HTGS_WholeFileStyles[3*style_inx];
	marks_area_style = (wchar_t*)g_HTGS_WholeFileStyles[3*style_inx+1];
	line_body_style = (wchar_t*)g_HTGS_WholeFileStyles[3*style_inx+2];
}

void THtmlSourceCodeStyle::GetCodeStatusBkgrStyles(TConditionalStatus code_status, wchar_t *&line_lums_style, wchar_t *&marks_area_style, wchar_t *&line_body_style)
{
	assert(m_src_code_style_prepared == TRUE);
	assert(code_status >= 0 && code_status < cexr_num_types);
	line_lums_style = (wchar_t*)g_HTGS_ConditionalParsingStatusStyles[3*code_status];
	marks_area_style = (wchar_t*)g_HTGS_ConditionalParsingStatusStyles[3*code_status+1];
	line_body_style = (wchar_t*)g_HTGS_ConditionalParsingStatusStyles[3*code_status+2];
}

const wchar_t *THtmlSourceCodeStyle::GetCmdlLexemaFrgrStyle(TSrcViewCmdlLexemaStyleInx style_inx)
{
	assert(m_src_code_style_prepared == TRUE);
	assert(style_inx >= 0 && style_inx < rcmsi_num_types);
	return(g_HTGS_CmdlLexemaStyles[style_inx]);
}

const wchar_t *THtmlSourceCodeStyle::GetCLangLexemaFrgrStyle(TSrcViewCLangLexemaStyleInx style_inx)
{
	assert(m_src_code_style_prepared == TRUE);
	assert(style_inx >= 0 && style_inx < rclsi_num_types);
	return(g_HTGS_CLangLexemaStyles[style_inx]);
}

const wchar_t *THtmlSourceCodeStyle::GetMidLevObjectBkgrStyle(TSrcViewMidLevBkgrStyleInx style_inx)
{
	assert(m_src_code_style_prepared == TRUE);
	assert(style_inx >= 0 && style_inx < mlbsi_num_types);
	return(g_HTGS_MidLevObjectBkgrStyles[style_inx]);
}

const wchar_t *THtmlSourceCodeStyle::GetMidLevObjectFrgrStyle(TSrcViewMidLevFrgrStyleInx style_inx)
{
	assert(m_src_code_style_prepared == TRUE);
	assert(style_inx >= 0 && style_inx < mlfsi_num_types);
	return(g_HTGS_MidLevObjectFrgrStyles[style_inx]);
}

const wchar_t *THtmlSourceCodeStyle::GetCppObjectBkgrStyle()
{
	assert(m_src_code_style_prepared == TRUE);

	// For now the bkgr of all Cpp objects is not highlighted.
	return(NULL);
}

const wchar_t *THtmlSourceCodeStyle::GetCppObjectFrgrStyle(TCppSourceAreaType area_type)
{
	assert(m_src_code_style_prepared == TRUE);
	assert(area_type >= 0 && area_type < cxa_num_types);
	return(g_HTGS_CppObjectFrgrStyles[area_type]);
}

void THtmlSourceCodeStyle::GetSpacelessDecorationProps(TSrcViewSpacelessDecorInx style_inx, const wchar_t *&image_src_file, const wchar_t *&css_style, wchar_t *buff_80)
{
	assert(m_src_code_style_prepared == TRUE);
	assert(style_inx >= 0 && style_inx < spcdi_num_types);

	// Pick up the short icon name.
	image_src_file = g_HTGS_SpacelessIconNames[style_inx];

	if (image_src_file != NULL && m_src_icons_directory != NULL)
	{
		// It is necessary to attach the name of the directory to the name of the icon.
		swprintf(buff_80, 80, L"%s/%s", m_src_icons_directory, image_src_file);
		image_src_file = buff_80;
	}

	// The ends of the warning and error areas marks should have bigger shift.
	if (style_inx == spcdi_warning_area_end_mark || style_inx == spcdi_error_area_end_mark)
		css_style = L"vertical-align:-6px;";
	else css_style = L"vertical-align:-3px;";
}

// ---------------------------------------------------------------------------------
//	============  THtmlSourceCodeFile::THtmlSourceCodeBuilder  ================
// ---------------------------------------------------------------------------------

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyFileViewStyleToUpperArea(TSrcViewWholeFileStyleInx view_style_inx, long upper_area_end, bool whole_line_area_style)
{
	// Pick up the CSS class names.
	wchar_t *line_lums_style = NULL, *marks_area_style = NULL, *line_body_style = NULL;
	m_code_style.GetWholeFileBkgrStyles(view_style_inx, line_lums_style, marks_area_style, line_body_style);
	if (line_lums_style == NULL && marks_area_style == NULL && line_body_style == NULL)
		return;

	// Pick up the line numbers that are involved in this code area.
	long lb = m_owner_file.m_file.GetLineNumber((long)0);
	long le = m_owner_file.m_file.GetLineNumber(upper_area_end);
	assert(lb > 0 && le > 0);

	// Process only the simple case for now.
	if (whole_line_area_style == TRUE)
	{
		if (le >= lb)
			m_owner_file.SetWholeLineBkgrDecoration(lb, le-lb+1, line_lums_style, marks_area_style, line_body_style);

		m_owner_file.SetTopBottomAreaDecoration(TRUE, FALSE, line_lums_style, marks_area_style, line_body_style);
	}
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyFileViewStyleToLowerArea(TSrcViewWholeFileStyleInx view_style_inx, long lower_area_beg, bool whole_line_area_style)
{
	// Pick up the CSS class names.
	wchar_t *line_lums_style = NULL, *marks_area_style = NULL, *line_body_style = NULL;
	m_code_style.GetWholeFileBkgrStyles(view_style_inx, line_lums_style, marks_area_style, line_body_style);
	if (line_lums_style == NULL && marks_area_style == NULL && line_body_style == NULL)
		return;

	// Pick up the line numbers that are involved in this code area.
	long lb = m_owner_file.m_file.GetLineNumber(lower_area_beg);
	long le = m_owner_file.m_file.GetLineNumber(m_owner_file.m_file.info.file_len);
	assert(lb > 0 && le > 0);

	// Process only the simple case for now.
	if (whole_line_area_style == TRUE)
	{
		if (le >= lb)
			m_owner_file.SetWholeLineBkgrDecoration(lb, le-lb+1, line_lums_style, marks_area_style, line_body_style);

		m_owner_file.SetTopBottomAreaDecoration(FALSE, TRUE, line_lums_style, marks_area_style, line_body_style);
	}
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplySpecialNumsMarksBackgrounds(long src_pars_beg_offs, long src_pars_end_offs, bool whole_line_area_style)
{
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyMidLevBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevBkgrStyleInx style_inx, TSourceAreaInfo &src_area)
{
	// This method creates only INLINE background decorations.
	assert(style_inx >= mlbsi_paste_buff_area && style_inx < mlbsi_num_types);

	// Pick up the CSS class.
	const wchar_t *css_class = m_code_style.GetMidLevObjectBkgrStyle(style_inx);
	if (css_class == NULL)
		return;

	// Create and cache the HTML owner.
	TRawHtmlElement *low_lev_owner = m_owner_file.m_low_level_decor_owners.CreateAppDelegateProxy(decor_owner, css_class);
	if (low_lev_owner == NULL)
		return;

	// Apply the decoration.
	if (m_owner_file.SetBkgrDecoration(low_lev_owner, zorder, src_area.area_beg, src_area.area_len) == FALSE)
		m_owner_file.m_decor_creation_errors++;
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceAreaInfo &src_area)
{
	// Pick up the CSS class.
	const wchar_t *css_class = m_code_style.GetMidLevObjectFrgrStyle(style_inx);
	if (css_class == NULL)
		return;

	// Create and cache the HTML owner.
	TRawHtmlElement *low_lev_owner = m_owner_file.m_low_level_decor_owners.CreateAppDelegateProxy(decor_owner, css_class);
	if (low_lev_owner == NULL)
		return;

	// Apply the decoration.
	if (m_owner_file.SetFrgrDecoration(low_lev_owner, zorder, src_area.area_beg, src_area.area_len) == FALSE)
		m_owner_file.m_decor_creation_errors++;
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyCppObjectFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TCppSourceAreaType area_type, TSourceAreaInfo &src_area)
{
	// Pick up the CSS class.
	const wchar_t *css_class = m_code_style.GetCppObjectFrgrStyle(area_type);
	if (css_class == NULL)
		return;

	// Create and cache the HTML owner.
	TRawHtmlElement *low_lev_owner = m_owner_file.m_low_level_decor_owners.CreateAppDelegateProxy(decor_owner, css_class);
	if (low_lev_owner == NULL)
		return;

	// Apply the decoration.
	if (m_owner_file.SetFrgrDecoration(low_lev_owner, zorder, src_area.area_beg, src_area.area_len) == FALSE)
		m_owner_file.m_decor_creation_errors++;
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyCodeStatusDecors(TSourceViewDecorOwner *owner, bool whole_line_beg_style, bool whole_line_end_style)
{
	TConditionalStatus code_status = owner->m_event_data.cond_pars.new_code_stt;
	wchar_t *line_lums_style = NULL, *marks_area_style = NULL, *line_body_style = NULL;
	m_code_style.GetCodeStatusBkgrStyles(code_status, line_lums_style, marks_area_style, line_body_style);
	if (line_lums_style == NULL && marks_area_style == NULL && line_body_style == NULL)
		return;

	// Pick up the lines that are involved in this code area.
	long area_beg = owner->m_src_area.AreaBeg();
	long area_end = owner->m_src_area.AreaEnd();
	long lb = m_owner_file.m_file.GetLineNumber(area_beg);
	long le = m_owner_file.m_file.GetLineNumber(area_end);
	assert(lb > 0 && le > 0);

	if (whole_line_beg_style == TRUE && whole_line_end_style == TRUE)
	{
		// Both sides have the whole line style. The situation is simple.
		m_owner_file.SetWholeLineBkgrDecoration(lb, le-lb+1, line_lums_style, marks_area_style, line_body_style);
	}
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyCmdlRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCmdlLexemaStyleInx style_inx, TSourceArea &area)
{
	// Pick up the CSS class.
	const wchar_t *css_class = m_code_style.GetCmdlLexemaFrgrStyle(style_inx);
	if (css_class == NULL)
		return;

	// Pick up or create the HTML owner.
	TRawHtmlElement *low_lev_owner = m_owner_file.m_low_level_decor_owners.FindOrCreateSimpleOwner(css_class);
	if (low_lev_owner == NULL)
		return;

	// Apply the decoration.
	if (m_owner_file.SetFrgrDecoration(low_lev_owner, zorder, area.area_beg, area.area_len) == FALSE)
		m_owner_file.m_decor_creation_errors++;
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyCLangRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCLangLexemaStyleInx style_inx, TSourceArea &area)
{
	// Pick up the CSS class.
	const wchar_t *css_class = m_code_style.GetCLangLexemaFrgrStyle(style_inx);
	if (css_class == NULL)
		return;

	// Pick up or create the HTML owner.
	TRawHtmlElement *low_lev_owner = m_owner_file.m_low_level_decor_owners.FindOrCreateSimpleOwner(css_class);
	if (low_lev_owner == NULL)
		return;

	// Apply the decoration.
	if (m_owner_file.SetFrgrDecoration(low_lev_owner, zorder, area.area_beg, area.area_len) == FALSE)
		m_owner_file.m_decor_creation_errors++;
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyMacroHdrFrgrDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceArea &area)
{
	// Pick up the CSS class.
	const wchar_t *css_class = m_code_style.GetMidLevObjectFrgrStyle(style_inx);
	if (css_class == NULL)
		return;

	// Create and cache the HTML owner.
	TRawHtmlElement *low_lev_owner = m_owner_file.m_low_level_decor_owners.CreateAppDelegateProxy(&m_owner_file.m_syntax_level_decor_owners.m_macro_header_owner, css_class);
	if (low_lev_owner == NULL)
		return;

	// Apply the decoration.
	if (m_owner_file.SetFrgrDecoration(low_lev_owner, zorder, area.area_beg, area.area_len) == FALSE)
		m_owner_file.m_decor_creation_errors++;
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyInlineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder,
																		TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long src_area_offs)
{
	// Create and cache the HTML owner.
	TRawHtmlElement *low_lev_owner = FindOrCreateSpacelessDecorOwner(decor_owner, decor_type_inx);
	if (low_lev_owner == NULL)
		return;

	// Apply the decoration.
	if (m_owner_file.SetSpacelessDecoration(low_lev_owner, horz_order, src_area_offs) == FALSE)
		m_owner_file.m_decor_creation_errors++;
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ApplyWholeLineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder,
																			TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long line_number)
{
	// Create and cache the HTML owner.
	TRawHtmlElement *low_lev_owner = FindOrCreateSpacelessDecorOwner(decor_owner, decor_type_inx);
	if (low_lev_owner == NULL)
		return;

	// Apply the decoration.
	if (m_owner_file.SetMarksAreaDecoration(line_number, horz_order, low_lev_owner) == FALSE)
		m_owner_file.m_decor_creation_errors++;
}

bool THtmlSourceCodeFile::THtmlSourceCodeBuilder::GetVisibleArea(long &visible_area_beg, long &visible_area_len)
{
	visible_area_beg = 0;
	visible_area_len = m_owner_file.m_file.info.file_len;
	return(m_owner_file.GetIntersectionWithVisibleFileArea(visible_area_beg, visible_area_len, TRUE));
}

void THtmlSourceCodeFile::THtmlSourceCodeBuilder::ReportProcessingProblem(const wchar_t *message)
{
	THtmlSourceCodeFile::ShowConsoleError(m_gen_console, message);
}

TRawHtmlElement *THtmlSourceCodeFile::THtmlSourceCodeBuilder::FindOrCreateSpacelessDecorOwner(TSourceViewDecorOwner *decor_owner, TSrcViewSpacelessDecorInx decor_type_inx)
{
	// Pick up the props of the image from the style object.
	wchar_t buffer[80];
	const wchar_t *image_src_file = NULL, *css_style = NULL;
	m_code_style.GetSpacelessDecorationProps(decor_type_inx, image_src_file, css_style, buffer);
	if (image_src_file == NULL)
		return(NULL);

	// Create and cache the HTML owner.
	TRawHtmlElement *low_level_owner = (decor_owner != NULL) ?
			m_owner_file.m_low_level_decor_owners.CreateAppDelegateProxy(decor_owner, NULL, css_style, image_src_file) :
			m_owner_file.m_low_level_decor_owners.FindOrCreateSimpleOwner(NULL, css_style, image_src_file);
	return(low_level_owner);
}

// ---------------------------------------------------------------------------
//	================  THtmlSourceCodeFile  ========================
// ---------------------------------------------------------------------------

THtmlSourceCodeFile::THtmlSourceCodeFile(const wchar_t *css_class, const wchar_t *css_style, THtmlCrLfMode crlf_mode)
		: THtmlDecoratedFile(css_class, css_style, crlf_mode), m_low_level_decor_owners(m_decor_creation_errors),
		m_syntax_level_decor_owners(m_decor_owner_creation_errors)
{
	m_file.Clear();
	m_file_data_owned = FALSE;

	m_decor_creation_errors = 0;
	m_decor_owner_creation_errors = 0;
}

void THtmlSourceCodeFile::ResetSourceFile()
{
	// Reset the base class.
	ResetDecoratedFile();

	if (m_file_data_owned == TRUE)
	{
		m_file.ReleaseBasicFileInfo();
		m_file.ReleaseLinesInfo();
	}

	m_file.Clear();
	m_file_data_owned = FALSE;

	m_low_level_decor_owners.ResetContext();
	m_syntax_level_decor_owners.ResetContext();

	m_decor_creation_errors = 0;
	m_decor_owner_creation_errors = 0;
}

bool THtmlSourceCodeFile::SetupSourceFile(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, const wchar_t *line_number_id_prefix,
											TFileInfo &file, bool bypass_file_info_data, int line_from, int num_lines)
{
	// Kill the previous contents of the object if any.
	ResetSourceFile();

	if (file.info.file_body != NULL && file.info.file_len < 0)
		file.info.file_len = (long)wcslen(file.info.file_body);

	// Do the low level setup.
	if (AcceptFileData(cons, code_style, line_number_id_prefix, file, bypass_file_info_data, line_from, num_lines) == FALSE)
		return(FALSE);

	// Do the syntax highlight.
	THtmlSourceCodeBuilder builder(cons, code_style, this);
	TSyntaxHighlighter hlpr(builder, m_syntax_level_decor_owners);
	hlpr.DecorateStandAloneFile(m_file);
	ApplyDefnRangesLayout();

	// Give a chance to a derived class to update the view.
	SetupAdditionalDecors(hlpr);

	// Show the count of errors if any and return the overall result.
	return(FinalizeFileSetup(cons));
}

bool THtmlSourceCodeFile::SetupSourceFile(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, TMidLevStoreReader *db, TMidLevScanner *active_scanner,
											ID call_pars_id, ID call_event_id, int line_from, int num_lines)
{
	// The database reader is essential for this type of setup.
	assert(db != NULL);

	// Pick up the lex src enclosure event from the database.
	if (db->GetLexSourceEnterEvent(call_pars_id, call_event_id) == FALSE)
	{
		ResetSourceFile();
		return(FALSE);
	}

	// Call the real processing.
	return(SetupSourceFile(cons, code_style, db, active_scanner, db->EventData().src_enter, line_from, num_lines));
}

bool THtmlSourceCodeFile::SetupSourceFile(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, TMidLevStoreReader *db, TMidLevScanner *active_scanner,
											CtvLexSourceEnterEvent &call_event, int line_from, int num_lines)
{
	// The database reader is essential for this type of setup.
	assert(db != NULL);

	// Kill the previous contents of the object if any.
	ResetSourceFile();

	// Allocate syntax highlight objects. They will guide the process.
	THtmlSourceCodeBuilder doc_builder(cons, code_style, this);
	TSyntaxHighlighter hlpr(doc_builder, m_syntax_level_decor_owners);

	// Load the file into the syntax highlight helper.
	if (hlpr.LoadLexScrEnclosureFile(call_event, db, active_scanner) == FALSE)
	{
		// The error is already reported.
		return(FALSE);
	}

	if (line_from <= 0)
	{
		// Caller is asking to select the lines range based on the active area of the call event.
		int num_upper_rows, num_lower_rows;
		code_style.GetPartialSrcEncloseFraming(num_upper_rows, num_lower_rows);
		hlpr.ResolveLinesRangeToDisplay(num_upper_rows, num_lower_rows, line_from, num_lines);
	}

	// Accept the file data.
	wchar_t line_number_id_prefix[40];
	TFileInfo::PrepareFileIdShort(hlpr.m_src_file.pars_id, hlpr.m_src_file.file_id, line_number_id_prefix, 40);
	if (AcceptFileData(cons, code_style, line_number_id_prefix, hlpr.m_src_file, hlpr.m_src_file_data_owned, line_from, num_lines) == FALSE)
		return(FALSE);

	// Ownership on the file data, if it belonged to the highlighter, now belongs to the current object.
	hlpr.m_src_file_data_owned = FALSE;

	// Do the syntax highlight.
	hlpr.DecorateLexSrcEnclosure();
	SetupAdditionalDecors(hlpr);

	// Show the count of errors if any and return the overall result.
	return(FinalizeFileSetup(cons));
}

bool THtmlSourceCodeFile::AcceptFileData(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, const wchar_t *line_numbers_id_prefix,
										TFileInfo &file, bool bypass_file_info_data, int line_from, int num_lines)
{
	// Do simple checks on the passed file.
	if (file.info.file_name == NULL)
	{
		ShowConsoleError(cons, L"THtmlSourceCodeFile: Passed file does not have a file name.");
		return(FALSE);
	}
	else if (file.info.file_body == NULL)
	{
		ShowConsoleError(cons, L"THtmlSourceCodeFile: Passed file \"%s\" does not have a file body.", file.info.file_name);
		return(FALSE);
	}

	// Construct the decorated file props.
	THtmlDecoratedFileProps props;
	code_style.GetWholeFileBkgrStyles(wfsi_whole_file, props.m_def_nums_css_style, props.m_def_marks_css_style, props.m_def_line_data_css_style);
	code_style.GetWholeFileTopBottomExtents(props.m_top_area_height, props.m_bottom_area_height);
	props.m_file_data_emit_mode = rdem_escape_tab_space;

	// Do the base class setup. Note that the ownership on the file data is not bypassed.
	if (Setup(props, line_numbers_id_prefix, file.info.file_body, file.info.file_len, 0, 1, file.lines_info, file.lines_info_len, line_from, num_lines) == FALSE)
	{
		ShowConsoleError(cons, L"THtmlSourceCodeFile: Error setting up the file \"%s\" into the base class.", file.info.file_name);
		return(FALSE);
	}

	// Prepare and set the HTML id of the whole object and set the CSS class of the tables of the object.
	SetupElementId(line_numbers_id_prefix);
	SetupCssClassForTable(code_style.GetHtmlTableCssStyle());

	// Copy in the data fields.
	m_file = file;
	m_file_data_owned = bypass_file_info_data;
	return(TRUE);
}

bool THtmlSourceCodeFile::FinalizeFileSetup(TGenericConsole &cons)
{
	bool overall_success = TRUE;

	// Show info about the owner allocation errors.
	if (m_decor_owner_creation_errors > 0)
	{
		ShowConsoleError(cons, L"THtmlSourceCodeFile: Decoration owner creation encountered %d problem%s.", m_decor_owner_creation_errors, PluralSuffix(m_decor_owner_creation_errors));
		overall_success = FALSE;
	}

	// Show info about the decoration creation errors.
	if (m_decor_creation_errors > 0)
	{
		ShowConsoleError(cons, L"THtmlSourceCodeFile: Decoration creation encountered %d problem%s.", m_decor_creation_errors, PluralSuffix(m_decor_creation_errors));
		overall_success = FALSE;
	}

	return(overall_success);
}

void THtmlSourceCodeFile::ApplyDefnRangesLayout()
{
	if (RAW_SCAN_CLANG_STYLE(m_file.rscn_type) == TRUE)
	{
		int cnt_decor_errors = 0, cnt_temp_data_alloc_errors = 0;
		TSpecTabDefinitionRangesProcessor defn_ranges_proc(*this, cnt_decor_errors, cnt_temp_data_alloc_errors);

		// Iterate lexemas in the file.
		TStringsDict dict;
		TRawScanner scanner(&m_file, &dict);

		TLexema lex;
		while (lex.IsEof() == FALSE)
		{
			scanner.GetLex(lex);
			defn_ranges_proc.ProcessLexema(lex);
		}
	}
}

void THtmlSourceCodeFile::ShowConsoleError(TGenericConsole &cons, const wchar_t *format, ...)
{
	// Do not show the message if the user has already cancelled.
	if (cons.GetAbortFlag() == TRUE)
		return;

	// Format the error message.
	va_list vargs;
	va_start(vargs, format);
	wchar_t proc_err_buff[2048];
	vswprintf(proc_err_buff, 2048, format, vargs);
	va_end(vargs);

	// Show the error message.
	if (cons.ShowRunTimeError(proc_err_buff, MB_YESNO) == IDNO)
		cons.SetAbortFlag(TRUE);
}

// ----------------------------------------------------------------------------
//	==============  THtmlCppDatabaseReportStyle  =====================
// ----------------------------------------------------------------------------

void THtmlCppDatabaseReportStyle::GetItemTypeBitmapProps(TCppItemBase &item, const wchar_t *&image_src_file, const wchar_t *&css_style, wchar_t *buff_80) const
{
	image_src_file = NULL;
	css_style = L"vertical-align:-3px;";

	// Pick up the short name of the icon first.
	switch (item.ItemType())
	{
		case cxx_built_in_type:			image_src_file = L"Cxx_BuiltInType.gif";
									break;
		case cxx_cv_ptr_ref_type:
		case cxx_array_type:
		case cxx_data_type_alias:		image_src_file = L"Cxx_DataType.gif";
									break;

		case cxx_enum_type:			image_src_file = L"Cxx_EnumType.gif";
									break;
		case cxx_struct_type:
				{
					TCppStructTypeDefn &struct_type = (TCppStructTypeDefn&)item;
					switch (struct_type.DefnElabType())
					{
						case elab_struct:		image_src_file = L"Cxx_StructType_Struct.gif";
											break;
						case elab_union:		image_src_file = L"Cxx_StructType_Union.gif";
											break;
						case elab_class:		image_src_file = L"Cxx_StructType_Class.gif";
											break;

						default:		// The elaboration type of the object is bogus. Show icon with a small cross in the center.
									image_src_file = L"Cxx_StructType.gif";
									break;
					}
				}
				break;

		case cxx_data_field:
				{
					TCppDataFieldDefn &data_field = (TCppDataFieldDefn&)item;
					switch (data_field.m_strg_class)
					{
						case strgc_param:					image_src_file = L"Cxx_DataFieldParam.gif";
														break;
						case strgc_template_param:		image_src_file = L"Cxx_DataFieldTemplateParam.gif";
														break;
						case strgc_catch_block_param:		image_src_file = L"Cxx_DataFieldCatchBlockParam.gif";
														break;

						default:		image_src_file = L"Cxx_DataField.gif";
									break;
					}
				}
				break;

		case cxx_code_entry_point:		image_src_file = L"Cxx_CodeEntryPoint.gif";
									break;
		case cxx_namespace:			image_src_file = L"Cxx_NameSpace.gif";
									break;
		case cxx_class_template:
				{
					TCppClassTemplateDefn &class_template = (TCppClassTemplateDefn&)item;
					switch (class_template.DefnElabType())
					{
						case elab_struct:		image_src_file = L"Cxx_ClassTemplate_Struct.gif";
											break;
						case elab_union:		image_src_file = L"Cxx_ClassTemplate_Union.gif";
											break;
						case elab_class:		image_src_file = L"Cxx_ClassTemplate_Class.gif";
											break;

						default:		image_src_file = L"Cxx_ClassTemplate.gif";
									break;
					}
				}
				break;

		case cxx_func_template:		image_src_file = L"Cxx_FunctionTemplate.gif";
									break;

		case cxx_unresolved_name_info:
		case cxx_unresolved_data_type:
		case cxx_unresolved_struct_type:
		case cxx_unresolved_data_field:
		case cxx_unresolved_code_entry_point:
		case cxx_unresolved_class_template:
		case cxx_unresolved_func_template:
				{
					// For now show the same icon for all unresolved types.
					image_src_file = L"Cxx_UnresolvedObject.gif";
					break;
				}
	}

	if (image_src_file != NULL && m_cpp_list_view_icons_dir != NULL)
	{
		// It is necessary to attach the name of the directory to the name of the icon.
		swprintf(buff_80, 80, L"%s/%s", m_cpp_list_view_icons_dir, image_src_file);
		image_src_file = buff_80;
	}
}

const wchar_t	*THtmlCppDatabaseReportStyle::GetItemNameCssStyle(TCppItemBase &item) const
{
	if (item.IsDefinition() == TRUE && (item.ItemType() < cxx_block || item.ItemType() > cxx_light_block))
		return(L"crp_defn_name");
	else return(L"crp_other_name");
}

const wchar_t *THtmlCppDatabaseReportStyle::GetItemDetailsCssStyle(TCppItemBase &item) const
{
	switch (item.ItemType())
	{
		case cxx_namespace:
		case cxx_global_namespace:
				return(L"crp_dtls_dtls");

		case cxx_enum_type:
		case cxx_enum_member:
		case cxx_struct_type:
		case cxx_data_field:
				return(L"crp_dtls_dtls");

		case cxx_code_entry_point:
		case cxx_class_template:
				return(L"crp_dtls_params");
	}

	// Other item types do not have details.
	return(NULL);
}

// ----------------------------------------------------------------------------
//	================  THtmlCppDatabaseReport  =======================
// ----------------------------------------------------------------------------

THtmlCppDatabaseReport::THtmlCppDatabaseReport(const wchar_t *css_class, const wchar_t *css_style, THtmlCrLfMode crlf_mode)
		: TAdvancedHtmlElement(L"table", NULL, 0, css_class, crlf_mode)
{
	SetCssStyle(css_style);

	m_rprt_console = NULL;
	m_report_style = NULL;
	m_cdb = NULL;

	m_cnt_app_errors = 0;
}

void THtmlCppDatabaseReport::EmitElementBody(THtmlDocument &doc)
{
	assert(m_rprt_console != NULL && m_report_style != NULL && m_cdb != NULL);
	m_cnt_app_errors = 0;

	if (m_report_style->WantTopRow() > 0)
		EmitPaddingRow(doc, m_report_style->WantTopRow());

	// Generate the body of the report.
	TCppItemBase &rprt_root = (m_report_root != NULL) ? *m_report_root : m_cdb->GetGlobalNamespace();
	EmitItemsHier(doc, rprt_root, 0);

	if (m_report_style->WantBottomRow() > 0)
		EmitPaddingRow(doc, m_report_style->WantBottomRow());

	if (m_cnt_app_errors != 0)
	{
		// Show the error message.
		wchar_t error_buff[512];
		swprintf(error_buff, 512, L"THtmlCppDatabaseReport: %d app level error%s detected.", m_cnt_app_errors, PluralSuffix(m_cnt_app_errors));
		m_rprt_console->ShowRunTimeError(error_buff);
	}
}

void THtmlCppDatabaseReport::SetupReport(TGenericConsole &cons, THtmlCppDatabaseReportStyle &report_style, TCppDatabase &cdb,
										TCppItemBase *report_root, bool want_all_database_items, bool want_item_details, bool want_item_ids_clmn)
{
	if (m_css_class == NULL)
		m_css_class = (wchar_t*)report_style.GetReportTableCssStyle();

	m_rprt_console = &cons;
	m_report_style = &report_style;
	m_cdb = &cdb;

	m_report_root = report_root;
	m_show_all_items = want_all_database_items;
	m_show_item_details = want_item_details;
	m_show_item_ids_clmn = want_item_ids_clmn;
}

void THtmlCppDatabaseReport::EmitItemsHier(THtmlDocument &doc, TCppItemBase &items_parent, int child_items_nest_level)
{
	// Process items of the current layer.
	for (TCppDirectChildrenIter iter1(items_parent); iter1; ++iter1)
	{
		TCppItemBase &item = iter1.CurrItem();
		if (m_show_all_items == TRUE || CheckIncludeItem(item) == TRUE)
		{
			EmitSingleItem(doc, item, child_items_nest_level);
			if (item.CanHaveChildren() == TRUE)
				EmitItemsHier(doc, item, child_items_nest_level+1);
		}
	}
}

void THtmlCppDatabaseReport::EmitSingleItem(THtmlDocument &doc, TCppItemBase &item, int item_nest_level)
{
	TSimpleHtmlElement table_row(L"tr", NULL, 0, NULL, crlm_outline_object_frames);
	doc.EmitElementBeg(table_row);

	if (item_nest_level > 0 && item_nest_level < NUM_HIER_COLUMNS)
	{
		// Generate the padding cells.
		TSimpleHtmlElement padding_cell(L"td", NULL, 0, m_report_style->GetIndentCellCssStyle(), crlm_none);
		for (int ic=0; ic<item_nest_level; ++ic)
			doc.EmitElement(padding_cell);
	}

	wchar_t colspan_buff[40];
	TAdvancedHtmlElement item_body_cell(L"td", NULL, 0, m_report_style->GetBodyCellCssStyle(), crlm_before_whole_obj);
	if (item_nest_level < NUM_HIER_COLUMNS-1)
		item_body_cell.SetFirstAppAttr(L"colspan", _itow(NUM_HIER_COLUMNS-item_nest_level, colspan_buff, 10));

	doc.EmitElementBeg(item_body_cell);

	// Check for the icon of the item.
	wchar_t buffer[80];
	const wchar_t *image_src_file, *css_style;
	m_report_style->GetItemTypeBitmapProps(item, image_src_file, css_style, buffer);
	if (image_src_file != NULL)
	{
		// Item needs an item type icon.
		TImageHtmlElement item_type_icon(image_src_file, NULL, css_style);
		doc.EmitElement(item_type_icon);
	}

	// Generate the name of item.
	TSimpleHtmlElement item_name(L"span", GetItemName(item), -1, m_report_style->GetItemNameCssStyle(item));
	doc.EmitElement(item_name);

	if (m_show_item_details == TRUE)
	{
		// Options require item details.
		TMessageBuffer buffer;
		if (GenerateItemDetails(buffer, item) == TRUE)
		{
			TSimpleHtmlElement item_details(L"span", buffer.DataPtr(), buffer.NumItems(), m_report_style->GetItemDetailsCssStyle(item));
			doc.EmitElement(item_details);
		}
	}

	doc.EmitElementEnd(item_body_cell);

	if (m_show_item_ids_clmn == TRUE)
	{
		// Show the id of the item.
		wchar_t item_id_buff[40];
		TSimpleHtmlElement item_id_cell(L"td", _itow(item.ItemId(), item_id_buff, 10), -1, m_report_style->GetItemIdCssStyle(), crlm_none);
		doc.EmitElement(item_id_cell);
	}

	doc.EmitElementEnd(table_row);
}

void THtmlCppDatabaseReport::EmitPaddingRow(THtmlDocument &doc, int row_height)
{
	wchar_t buff_height[40];
	swprintf(buff_height, 40, L"height:%dpx", row_height);
	TAdvancedHtmlElement ext_row(L"tr");
	ext_row.SetCssStyle(buff_height);

	wchar_t colspan_buff[40];
	TAdvancedHtmlElement ext_row_cell(L"td", NULL, 0, NULL, crlm_none);
	ext_row_cell.SetFirstAppAttr(L"colspan", _itow((m_show_item_ids_clmn == TRUE) ? NUM_HIER_COLUMNS+1 : NUM_HIER_COLUMNS, colspan_buff, 10));

	doc.EmitElementBeg(ext_row);
	doc.EmitElement(ext_row_cell);
	doc.EmitElementEnd(ext_row);
}

bool THtmlCppDatabaseReport::CheckIncludeItem(TCppItemBase &item)
{
	switch (item.ItemType())
	{
		case cxx_enum_type:
		case cxx_struct_type:
				return(TRUE);

		case cxx_data_field:
				{
					TCppDataFieldDefn &data_field = (TCppDataFieldDefn&)item;
					return(data_field.m_strg_class < strgc_param);
				}

		case cxx_code_entry_point:
		case cxx_namespace:
		case cxx_global_namespace:
		case cxx_class_template:
		case cxx_func_template:
				return(TRUE);
	}

	// Other items should not be included.
	return(FALSE);
}

bool THtmlCppDatabaseReport::GenerateItemDetails(TMessageBuffer &buffer, TCppItemBase &item)
{
	buffer.ClearBuffer();

	switch (item.ItemType())
	{
		case cxx_enum_type:
				buffer.Append(L"/enum/");
				break;

		case cxx_enum_member:
				{
					TCppEnumMemberDefn &enum_member = (TCppEnumMemberDefn&)item;
					wchar_t loc_buff[80];
					swprintf(loc_buff, 80, L"/%I64d, 0x%I64X/", enum_member.m_value, enum_member.m_value);
					buffer.Append(loc_buff);
				}
				break;

		case cxx_struct_type:
				{
					TCppStructTypeDefn &struct_type = (TCppStructTypeDefn&)item;
					buffer.Append(L"/");
					buffer.Append(struct_type.GetElabTypeName());
					buffer.Append(L"/");
				}
				break;

		case cxx_data_field:
				{
					TCppDataFieldDefn &data_field = (TCppDataFieldDefn&)item;
					if (data_field.m_field_type != NULL)
					{
						// The type of the data field is available.
						buffer.Append(L"/");
						buffer.Append(data_field.m_field_type->DefnName());
						buffer.Append(L"/");
					}
				}
				break;

		case cxx_code_entry_point:
				{
					TCppCodeEntryPointDefn &code_entry_point = (TCppCodeEntryPointDefn&)item;
					buffer.Append(L"(");

					bool first_param = TRUE;
					TCppDataFieldDefn *param = code_entry_point.GetFirstParam();
					while (param != NULL)
					{
						if (first_param == FALSE)
							buffer.Append(L", ");

						// Show the type of the parameter.
						if (param->m_field_type != NULL)
							buffer.Append(param->m_field_type->DefnName());
						else buffer.Append(L"null-type");

						param = code_entry_point.GetNextParam(param);
						first_param = FALSE;
					}

					buffer.Append(L")");
				}
				break;

		case cxx_namespace:
				buffer.Append(L"/namespace/");
				break;

		case cxx_global_namespace:
				buffer.Append(L"/global-namespace/");
				break;

		case cxx_class_template:
				{
					TCppClassTemplateDefn &class_template = (TCppClassTemplateDefn&)item;
					buffer.Append(L"<");

					bool first_param = TRUE;
					TCppDefnBase *curr_param = class_template.GetFirstTemplateParam();
					while (curr_param != NULL)
					{
						if (first_param == FALSE)
							buffer.Append(L", ");

						if (curr_param->ItemType() == cxx_template_template_param)
						{
							// Do not show the params of this template for now.
							buffer.Append(L"template");
						}
						else if (curr_param->ItemType() == cxx_template_type_param)
						{
							// Type param does not have any props.
							buffer.Append(L"class");
						}
						else if (curr_param->ItemType() == cxx_data_field)
						{
							TCppDataFieldDefn *data_param = (TCppDataFieldDefn*)curr_param;
							if (data_param->m_field_type != NULL)
								buffer.Append(data_param->m_field_type->DefnName());
							else buffer.Append(L"null-data-param-type");
						}
						else
						{
							// This is something bogus. Show the type of the object.
							buffer.Append(curr_param->ItemTypeEnumName());
						}

						curr_param = class_template.GetNextTemplateParam(curr_param);
						first_param = FALSE;
					}

					buffer.Append(L">");
				}
				break;

		default:
			{
				// Other items do not have details for now.
				return(FALSE);
			}
	}

	if (buffer.GetXpndError() == TRUE)
		m_cnt_app_errors++;

	// Something is placed into the buffer.
	return(TRUE);
}

// ----------------------------------------------------------------------------
//	================  THtmlSourceGenHelper  ========================
// ----------------------------------------------------------------------------

bool THtmlSourceGenHelper::DumpParsingIndependentFile(THtmlDocument &rprt, THtmlSourceCodeStyle &code_style, const wchar_t *line_number_id_prefix, TFileInfo &file, int line_from, int num_lines)
{
	assert(rprt.IsDocFragmentMode() == FALSE);

	// Emit the beg of the HTML page.
	rprt.OpenDocumentHead();
	rprt.AddCssFileReferences(m_css_dir_or_url, m_css_file_names);
	rprt.WriteLine();

	rprt.AddPageTitle(file.info.file_name);

	rprt.OpenDocumentBody();
	rprt.SetupIndentDelta(-1);
	rprt.WriteLine();

	// Instantiate the source file object and emit it.
	THtmlSourceCodeFile src_file;
	bool setup_res = src_file.SetupSourceFile(m_src_gen_cons, code_style, line_number_id_prefix, file, FALSE, line_from, num_lines);
	rprt.EmitElement(src_file);
	rprt.WriteLine();

	// Emit an end of the HTML page.
	rprt.SetupIndentDelta(0);
	rprt.CloseDocument();
	rprt.Close();
	return(setup_res == TRUE && rprt.GetErrorFlag() == FALSE);
}

bool THtmlSourceGenHelper::DumpLexSourceEnclosure(THtmlDocument &rprt, THtmlSourceCodeStyle &code_style, TMidLevStoreReader *db, TMidLevScanner *active_scanner, ID call_pars_id, ID call_event_id, int line_from, int num_lines)
{
	assert(rprt.IsDocFragmentMode() == FALSE);

	// Instantiate the source file object and run the setup.
	THtmlSourceCodeFile src_file;
	bool setup_res = src_file.SetupSourceFile(m_src_gen_cons, code_style, db, active_scanner, call_pars_id, call_event_id, line_from, num_lines);

	// Emit the beg of the HTML page.
	rprt.OpenDocumentHead();
	rprt.AddCssFileReferences(m_css_dir_or_url, m_css_file_names);
	rprt.WriteLine();

	if (setup_res == TRUE)
		rprt.AddPageTitle(src_file.SourceFileName());

	rprt.OpenDocumentBody();
	rprt.SetupIndentDelta(-1);
	rprt.WriteLine();

	// Emit the main element.
	rprt.EmitElement(src_file);
	rprt.WriteLine();

	// Emit an end of the HTML page.
	rprt.SetupIndentDelta(0);
	rprt.CloseDocument();
	rprt.Close();
	return(setup_res == TRUE && rprt.GetErrorFlag() == FALSE);
}


