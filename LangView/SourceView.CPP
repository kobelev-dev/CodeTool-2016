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
#include  "LangView/SourceView.H"
#include  "LangView/LangViewResources.H"

// -----------------------------------------------------------------------------
//   ==================  TLangViewSourceFileStyle  =====================
// -----------------------------------------------------------------------------

static const TIconsGridStyleProps g_LangViewSourceCodeIconsProps =
{
	NULL,					// No specific CSS style.
	IDBMP_LANG_SOURCE,		// Resource ID.
	TRUE,					// Transparent pixels are present.
	TRUE,					// Bitmap has individual boundaries for each icon.
	75,						// Resolution.
	19, 12,					// Left-top position of the cell with the index 100.
	26, 24,					// Horz and vert steps between the cells.
	23, 21,					// The size of the cell.
};

// - - - - - - - -  Whole file styles group  - - - - - - - - -

static const TDecoratedFileViewStyleProps g_MLFV_WholeFileStyleProps[wfsi_num_types] =
{
	// wfsi_whole_file
	{
		L"src_code_table",
		56, 15,													// Nums width, marks width.
		5, 30, 7, 9,												// Text offsets: left, right, top, bottom.
		L"%d:", align_right, 2,										// Line numbers painting info.
		{ 0, 0 },													// Text area may have any size.
		{ 400, 0 },												// Min object size.

		RGB(254, 253, 222), RGB(234, 234, 234), RGB(255, 255, 255),		// Backgrounds (nums, marks, text).
		L"src_line_num", L"src_marks_area", L"src_line_body",			// CSS names of the default backgrounds.

		0, RGB(128, 128, 128),										// Shadow information.
		RGB(80, 0, 0), RGB(160, 200, 160),							// Mouse selection colors.
		2,														// Mouse selection horz exts.
	},

	// wfsi_out_of_scope
	{
		NULL,
		56, 15,													// Nums width, marks width.
		5, 30, 7, 9,												// Text offsets: left, right, top, bottom.
		L"%d:", align_right, 2,										// Line numbers painting info.
		{ 0, 0 },													// Text area may have any size.
		{ 400, 0 },												// Min object size.

		RGB(240, 240, 240), RGB(227, 227, 227), RGB(240, 240, 240),		// Backgrounds (nums, marks, text).
		L"src_out_of_scope_line_num",
		L"src_out_of_scope_marks_area",
		L"src_out_of_scope_line_body",

		0, RGB(128, 128, 128),										// Shadow information.
		RGB(80, 0, 0), RGB(160, 200, 160),							// Mouse selection colors.
		2,														// Mouse selection horz exts.
	},

	// wfsi_aborted_scan
	{
		NULL,
		56, 15,													// Nums width, marks width.
		5, 30, 7, 9,												// Text offsets: left, right, top, bottom.
		L"%d:", align_right, 2,										// Line numbers painting info.
		{ 0, 0 },													// Text area may have any size.
		{ 400, 0 },												// Min object size.

		RGB(254, 253, 222), RGB(234, 234, 234), RGB(255, 255, 255),		// Backgrounds (nums, marks, text).
		L"src_aborted_scan_line_num",
		L"src_aborted_scan_marks_area",
		L"src_aborted_scan_line_body",

		0, RGB(128, 128, 128),										// Shadow information.
		RGB(80, 0, 0), RGB(160, 200, 160),							// Mouse selection colors.
		2,														// Mouse selection horz exts.
	},

	// wfsi_unscanned
	{
		NULL,
		56, 15,													// Nums width, marks width.
		5, 30, 7, 9,												// Text offsets: left, right, top, bottom.
		L"%d:", align_right, 2,										// Line numbers painting info.
		{ 0, 0 },													// Text area may have any size.
		{ 400, 0 },												// Min object size (nums, marks, text).

		RGB(220, 220, 220), RGB(204, 204, 204), RGB(220, 220, 220),		// Backgrounds.
		L"src_unscanned_line_num",
		L"src_unscanned_marks_area",
		L"src_unscanned_line_body",

		0, RGB(128, 128, 128),										// Shadow information.
		RGB(80, 0, 0), RGB(160, 200, 160),							// Mouse selection colors.
		2,														// Mouse selection horz exts.
	},
};

static const TDecoratedFileViewTextProps g_MLFV_WholeFileTextProps[wfsi_num_types] =
{
	// Text style props for wfsi_whole_file.
	{
		{  NULL,	COURIER_NEW,	 8,	FALSE,	RGB(100, 100, 100),	STD_TXT },		// Line numbers props.
		{  NULL,	COURIER_NEW,	 9,	FALSE,	RGB(0, 0, 0),			STD_TXT },		// Default text props.
		{  NULL,	ARIAL,			 9,	TRUE,	RGB(60, 0, 0),			STD_TXT },		// Line overflow msg props.
	},

	// Text style props for wfsi_out_of_scope.
	{
		{  NULL,	COURIER_NEW,	 7,	FALSE,	RGB(180, 180, 180),	cs_transparent,	STD_SIMPLE,  0, 1 },	// Line numbers props.
		{  NULL,	COURIER_NEW,	 7,	FALSE,	RGB(180, 180, 180),	STD_TXT },		// Default text props.
		{  NULL,	ARIAL,			 8,	TRUE,	RGB(160, 160, 160),	STD_TXT },		// Line overflow msg props.
	},

	// Text style props for wfsi_aborted_scan.
	{
		{  NULL,	COURIER_NEW,	 7,	FALSE,	RGB(120, 120, 120),	cs_transparent,	STD_SIMPLE,  0, 1 },	// Line numbers props.
		{  NULL,	COURIER_NEW,	 7,	FALSE,	RGB(200, 200, 200),	STD_TXT },		// Default text props.
		{  NULL,	ARIAL,			 8,	TRUE,	RGB(160, 160, 160),	STD_TXT },		// Line overflow msg props.
	},

	// Text style props for wfsi_unscanned.
	{
		{  NULL,	COURIER_NEW,	 8,	FALSE,	RGB(100, 100, 100),	STD_TXT },		// Line numbers props.
		{  NULL,	COURIER_NEW,	 9,	FALSE,	RGB(80, 80, 80),		STD_TXT },		// Default text props.
		{  NULL,	ARIAL,			 8,	TRUE,	RGB(160, 160, 160),	STD_TXT },		// Line overflow msg props.
	},
};

static const TNonAsciiSubstsInfo  g_MLFV_NonAsciiSubsts[] =
{
	{ 0x0C, 0x0C,		101,		102	},
	{ 0x00, 0x1F,		102,		102	},
	{ 0, 0, 0, 0 },		// End of the list.
};

static TTabsSpacesViewMode g_MLFV_TabsSpacesViewMode =
{
	FALSE,				// Visible tabs.
	FALSE,				// Visible spaces.
	TRUE,				// Implicit tabs mode ON.
	4,					// Tabs spacing.
	75, 120, 75,			// Widths of the tab symbol and char place that are used in the tab pos calculation.
						// These values are percents of the width of the basic char in the default text style.
};

struct TLangViewSourceViewCodeStatusBkgrDecorProps
{
	wchar_t		*decor_css_style;
	TColor		decor_color;
};

static const TLangViewSourceViewCodeStatusBkgrDecorProps g_MLFV_CodeStatusDecorsInfoLineNumbers[cexr_num_types] =
{
	L"cps_inactive_nline",			RGB(220, 220, 220),		// cexr_hard_false
	NULL,						cs_transparent,			// cexr_hard_true
	L"cps_soft_inact_nline",			RGB(236, 220, 204),		// cexr_soft_false
	L"cps_soft_active_nline",		RGB(211, 232, 255),		// cexr_soft_true
	L"cps_undefined_nline",			RGB(206, 203, 255),		// cexr_undefined
};

static const TLangViewSourceViewCodeStatusBkgrDecorProps g_MLFV_CodeStatusDecorsInfoMarksArea[cexr_num_types] =
{
	L"cps_inactive_marks",			RGB(200, 200, 200),		// cexr_hard_false
	NULL,						cs_transparent,			// cexr_hard_true
	L"cps_soft_inact_marks",		RGB(225, 200, 174),		// cexr_soft_false
	L"cps_soft_active_marks",		RGB(166, 208, 255),		// cexr_soft_true
	L"cps_undefined_marks",		RGB(181, 176, 255),		// cexr_undefined
};

static const TLangViewSourceViewCodeStatusBkgrDecorProps g_MLFV_CodeStatusDecorsInfoLineBody[cexr_num_types] =
{
	L"cps_inactive",				RGB(220, 220, 220),		// cexr_hard_false
	NULL,						cs_transparent,			// cexr_hard_true
	L"cps_soft_inact",				RGB(236, 220, 204),		// cexr_soft_false
	L"cps_soft_active",				RGB(211, 232, 255),		// cexr_soft_true
	L"cps_undefined",				RGB(206, 203, 255),		// cexr_undefined
};


// - - - - - - - -  Raw and MidLev object styles group  - - - - - - - - -

static const TColor g_MLFV_IndexingKeyUseColors[rctx_num_types] =
{
	RGB(220, 30, 180),				// rctx_normal
	RGB(220, 30, 180),				// rctx_string
	RGB(220, 30, 180),				// rctx_comment
};

static const TTextStyleProps g_MLFV_CmdlLexemaFrgrStyleProps[rcmsi_num_types] =
{
	{  L"cmdl_comment",		COURIER_NEW,	 8,	FALSE,	RGB(200, 100, 100),	cs_transparent,		STD_SIMPLE,		0, 0 },		// rcmsi_comment
	{  L"cmdl_key_key",		TAHOMA,			10,	TRUE,	RGB(1, 92, 58),		cs_transparent,		STD_SIMPLE,		1, 1 },		// rcmsi_key_key
	{  L"cmdl_key_val",		MS_SANS_SERIF,	 9,	TRUE,	RGB(117, 0, 53),		cs_transparent,		STD_SIMPLE,		2, 3 },		// rcmsi_key_val
	{  L"cmdl_pos_param",		VERDANA,		10,	FALSE,	RGB(15, 28, 200),		RGB(255, 216, 157),	STD_SIMPLE,		2, 2 },		// rcmsi_pos_param
	{  L"cmdl_pos_param_inx",	SMALL_FONTS,	6,	FALSE,	RGB(15, 28, 200),		RGB(255, 216, 157),	STD_SIMPLE,		2, 2 },		// rcmsi_pos_param_inx
	{  L"cmdl_raw_scan_error",	TAHOMA,			10,	TRUE,	RGB(1, 22, 28),		RGB(255, 65, 75),		STD_SIMPLE,		1, 1 },		// rcmsi_raw_scan_error
};

static const TTextStyleProps g_MLFV_CLangLexemaFrgrStyleProps[rclsi_num_types] =
{
	{  L"lct_c_style",			COURIER_NEW,	 8,	FALSE,	RGB(200, 100, 100),	STD_TXT },			// rclsi_cmnt_c_style
	{  L"lct_c_style_big",		COURIER_NEW,	10,	FALSE,	RGB(200, 100, 100),	STD_TXT },			// rclsi_cmnt_c_style_big
	{  L"lct_cpp_endofline",		COURIER_NEW,	 8,	FALSE,	RGB(200, 100, 100),	STD_TXT },			// rclsi_cmnt_cpp_eol
	{  L"lct_cpp_wholeline",		COURIER_NEW,	 8,	FALSE,	RGB(200, 100, 100),	STD_TXT },			// rclsi_cmnt_cpp_whl
	{  L"lct_csh_trislash",		COURIER_NEW,	 8,	FALSE,	RGB(200, 100, 100),	STD_TXT },			// rclsi_cmnt_csh_trs

	{  L"ltx_number",			MS_SANS_SERIF,	 9,	FALSE,	RGB(255, 0, 0),		cs_transparent,		STD_NO_FRSH,   1, 0, 1,   0, 1 },		// rclsi_number
	{  L"ltx_charconst",			COURIER_NEW,	 8,	FALSE,	RGB(20, 37, 186),		RGB(251, 237, 136),	STD_NO_FRSH,   0, 0, 0,   1, 1 },		// rclsi_chconst_a
	{  L"ltx_charconst",			COURIER_NEW,	 8,	FALSE,	RGB(20, 37, 186),		RGB(251, 237, 136),	STD_NO_FRSH,   0, 0, 0,   3, 1 },		// rclsi_chconst_w
	{  L"ltx_string",			COURIER_NEW,	 8,	FALSE,	RGB(105, 31, 27),		RGB(255, 237, 111),	STD_NO_FRSH,   0, 0, 0,   2, 2 },		// rclsi_string_a
	{  L"ltx_string",			COURIER_NEW,	 8,	FALSE,	RGB(105, 31, 27),		RGB(255, 237, 111),	STD_NO_FRSH,   0, 0, 0,   3, 2 },		// rclsi_string_w

	{  L"ltx_string_inc_fn",		COURIER_NEW,	 8,	FALSE,	RGB(48, 48, 80),		RGB(226, 240, 255),	STD_NO_FRSH,   0, 0, 0,   3, 3 },		// rclsi_string_inc_fn
	{  L"ltx_string_spec",		COURIER_NEW,	 8,	FALSE,	RGB(48, 48, 80),		STD_TXT },			// rclsi_string_spec

	{  L"tkn_separ",			TAHOMA,			 9,	FALSE,	RGB(100, 100, 100),	STD_TXT },			// rclsi_tkn_separ
	{  L"tkn_arithm",			TAHOMA,			10,	FALSE,	RGB(100, 100, 160),	STD_TXT },			// rclsi_tkn_arithm
	{  L"tkn_preproc",			COURIER_NEW,	 8,	FALSE,	RGB(60, 101, 134),		STD_TXT },			// rclsi_tkn_preproc
	{  L"tkn_keyword",			VERDANA,		 8,	TRUE,	RGB(100, 100, 100),	STD_TXT },			// rclsi_tkn_clang
	{  L"tkn_keyword",			VERDANA,		 8,	TRUE,	RGB(100, 110, 100),	STD_TXT },			// rclsi_tkn_cpp
	{  L"tkn_keyword",			VERDANA,		 8,	TRUE,	RGB(100, 100, 100),	STD_TXT },			// rclsi_tkn_csh

	{  L"ltx_name",			VERDANA,		 8,	FALSE,	RGB(80, 80, 80),		STD_TXT },			// rclsi_name
	{  L"ltx_raw_scan_error",	TAHOMA,			10,	TRUE,	RGB(1, 28, 22),		RGB(255, 70, 70),		STD_NO_FRSH,   0, 0, 0,   1, 1 },		// rclsi_raw_scan_error
};

// Adjustments for the rclsi_tkn_preproc.
static const TTextStyleSymbolAdjustInfo g_MLFV_PreprocKeywordAdjustments[] =
{
	{ L'#', 0, 0, 1 }, { 0, 0, 0, 0 },
};

struct TLangViewSourceViewBkgrDecorProps
{
	wchar_t		*decor_css_style;
	TColor		decor_color;

	short		bkgr_inner_exts;
	short		bkgr_outer_exts;
	short		bkgr_up_ext;
	short		bkgr_down_ext;
};

static const TLangViewSourceViewBkgrDecorProps g_MLFV_MidLevObjectBkgrDecorsInfo[mlbsi_num_types] =
{
	{ NULL,						cs_transparent,			0,	0,	0,  0 },		// mlbsi_mpr_stmt_bkgr
	{ L"mlev_paste_buff_area",		RGB(197, 109, 199),	       3,	1,	2,  2 },		// mlbsi_paste_buff_area
	{ L"mlev_macro_call_area",		RGB(248, 203, 216),		2,	1,	0,  1 },		// mlbsi_macro_call_area
	{ L"mlev_warning_bkgr",		RGB(137, 179, 199),		2,	1,	1,  1 },		// mlbsi_warning_bkgr
	{ L"mlev_error_bkgr",			RGB(255, 89, 89),			2,	1,	1,  1 },		// mlbsi_error_bkgr
};

static const TTextStyleProps g_MLFV_MidLevObjectFrgrStyleProps[mlfsi_num_types] =
{
	{  L"mlev_mpr_stmt_token",		VERDANA,		 8,	TRUE,	RGB(60, 101, 134),		STD_TXT },			// mlfsi_mpr_stmt_token

	{  L"mlev_include_fname",		COURIER_NEW,	 8,	FALSE,	RGB(48, 48, 80),		RGB(234, 248, 255),	STD_NO_FRSH,    0, 0, 0,	3, 3 },	// mlfsi_include_fname
	{  L"mlev_mcall_macro_name",	VERDANA,		 8,	TRUE,	RGB(96, 96, 164),		STD_TXT },			// mlfsi_mcall_macro_name
	{  L"mlev_mcall_mprm_name",	VERDANA,		 8,	FALSE,	RGB(100, 100, 200),	STD_TXT },			// mlfsi_mcall_mparam_name

	{  L"mlev_macro_def_name",	ARIAL,			9,	TRUE,	RGB(23, 143, 101),		STD_TXT },			// mlfsi_macro_def_name

	{  L"mlev_cmdl_app_name",		L"Consolas",		13,	TRUE,	RGB(180, 100, 50),		RGB(255, 205, 130),	cs_transparent, fcs_none,	RGB(80, 128, 128), 1,	 0, 0, 0,	3, 3 },	// mlfsi_cmdl_app_name
	{  L"mlev_cmdl_mj_src_file",		VERDANA,		9,	TRUE,	RGB(15, 28, 200),		RGB(255, 216, 157),	STD_NO_FRSH,    0, 0, 0,	2, 2 },	// mlfsi_cmdl_mj_src_file

	{  L"mlev_mprep_blocked_stmt",	VERDANA,		 8,	FALSE,	RGB(100, 0, 0),		STD_TXT },			// mlfsi_mprep_blocked_stmt,
	{  L"mlev_mprep_failed_inc_fn",	VERDANA,		 8,	FALSE,	RGB(80, 80, 80),		STD_TXT },			// mlfsi_mprep_failed_inc_fn,
	{  L"mlev_mprep_other_fields",	COURIER_NEW,	 8,	FALSE,	RGB(100, 220, 100),	RGB(234, 255, 245),	STD_NO_FRSH,    0, 0, 1,	3, 3 },	// mlfsi_mprep_other_fields,

	{  L"mlev_cond_param_use",	VERDANA,		10,	TRUE,	RGB(112, 73, 46),		STD_TXT },			// mlfsi_cond_param_use

	{  NULL,						TAHOMA,			 8,	TRUE,	RGB(171, 24, 3),		cs_transparent,		cs_transparent, fcs_none,	RGB(255, 233, 233), 1,	 0, 0, 0,	0, 0 },	// mlfsi_grammar_section
	{  NULL,						VERDANA,		 9,	TRUE,	RGB(60, 70, 210),		cs_transparent,		cs_transparent, fcs_none,	RGB(220, 220, 250), 1,	 0, 0, 0,	0, 0 },	// mlfsi_grammar_rule_nterm
	{  NULL,						VERDANA,		10,	TRUE,	RGB(48, 56, 210),		cs_transparent,		cs_transparent, fcs_none,	RGB(180, 180, 250), 1,	 0, 0, 0,	0, 0 },	// mlfsi_grammar_xpct_name

	{  L"mlev_mdef_token",			VERDANA,		10,	TRUE,	RGB(60, 60, 255),		STD_TXT },			// mlfsi_mdef_token
	{  L"mlev_mdef_name",			ARIAL,			11,	TRUE,	RGB(23, 101, 101),		cs_transparent,		cs_transparent, fcs_none,	RGB(160, 160, 180), 1,	 0, 0, 0,	0, 0 },	// mlfsi_mdef_name
	{  L"mlev_mdef_delim",			TAHOMA,			11,	TRUE,	RGB(100, 140, 120),	STD_TXT },			// mlfsi_mdef_delim
	{  L"mlev_mdef_param",		TAHOMA,			10,	TRUE,	RGB(0, 100, 100),		STD_TXT },			// mlfsi_mdef_param
	{  L"mlev_mdef_other",			TAHOMA,			 7,	TRUE,	RGB(180, 180, 180),	STD_TXT },			// mlfsi_mdef_other
};

static const TTextStyleProps g_MLFV_CppObjectFrgrStyleProps[cxa_num_types] =
{
	//														FrgrColor			   BkgrColor			FrameProps				Shadow			    VertAdjust	   InnerExts
	{  NULL,						VERDANA,	13,	TRUE,	RGB(220, 220, 228),	RGB(8, 55, 38),	cs_transparent, fcs_none,	cs_transparent, 0,			0, 0, 0,	      3, 3		},	// cxa_none

	{  L"cxa_bright_keyword",		VERDANA,	 8,	TRUE,	RGB(255, 0, 0),		cs_transparent,	cs_transparent, fcs_none,	RGB(212, 212, 212), 1,		},	// cxa_bright_keyword
	{  L"cxa_namespace",			VERDANA,	15,	TRUE,	RGB(10, 10, 155),		STD_TXT },		// cxa_namespace

	{  L"cxa_enum_fwd_decl",		VERDANA,	8,	FALSE,	RGB(10, 155, 10),		STD_TXT },		// cxa_enum_fwd_decl
	{  L"cxa_struct_fwd_decl",		VERDANA,	8,	FALSE,	RGB(10, 155, 10),		STD_TXT },		// cxa_struct_fwd_decl
	{  L"cxa_union_fwd_decl",		VERDANA,	8,	FALSE,	RGB(10, 155, 10),		STD_TXT },		// cxa_union_fwd_decl
	{  L"cxa_class_fwd_decl",		VERDANA,	8,	FALSE,	RGB(10, 155, 10),		STD_TXT },		// cxa_class_fwd_decl

	{  L"cxa_enum_defn",			VERDANA,	10,	TRUE,	RGB(0, 0, 128),		cs_transparent,	cs_transparent, fcs_none,	RGB(192, 192, 192), 1,		},	// cxa_enum_defn
	{  L"cxa_struct_defn",			VERDANA,	10,	TRUE,	RGB(0, 0, 128),		cs_transparent,	cs_transparent, fcs_none,	RGB(192, 192, 192), 1,		},	// cxa_struct_defn
	{  L"cxa_union_defn",			VERDANA,	10,	TRUE,	RGB(0, 0, 128),		cs_transparent,	cs_transparent, fcs_none,	RGB(192, 192, 192), 1,		},	// cxa_union_defn
	{  L"cxa_class_defn",			VERDANA,	10,	TRUE,	RGB(0, 0, 128),		cs_transparent,	cs_transparent, fcs_none,	RGB(192, 192, 192), 1,		},	// cxa_class_defn

	{  L"cxa_enum_member",		VERDANA,	 8,	TRUE,	RGB(0, 0, 128),		STD_TXT },		// cxa_enum_member
	{  L"cxa_data_field_xtrn",		VERDANA,	 8,	FALSE,	RGB(0, 0, 155),		STD_TXT },		// cxa_data_field_xtrn
	{  L"cxa_data_field_defn",		VERDANA,	 8,	TRUE,	RGB(0, 0, 155),		STD_TXT },		// cxa_data_field_defn
	{  L"cxa_function_proto",		VERDANA,	 8,	TRUE,	RGB(0, 0, 170),		STD_TXT },		// cxa_function_proto
	{  L"cxa_function_header",		VERDANA,	11,	TRUE,	RGB(0, 0, 170),		cs_transparent,	cs_transparent, fcs_none,	RGB(192, 192, 192), 1,		},	// cxa_function_header

	{  L"cxa_nameless_bft",			VERDANA,	15,	TRUE,	RGB(10, 10, 155),		STD_TXT },		// cxa_nameless_bft
	{  L"cxa_nameless_cvpr",		VERDANA,	10,	TRUE,	RGB(150, 0, 128),		STD_TXT },		// cxa_nameless_cvpr
	{  L"cxa_nameless_array",		VERDANA,	15,	TRUE,	RGB(10, 10, 155),		STD_TXT },		// cxa_nameless_array
	{  L"cxa_nameless_fntp",		VERDANA,	10,	TRUE,	RGB(150, 0, 128),		STD_TXT },		// cxa_nameless_fntp
};

struct TLangViewSourceViewSpacelessDecorProps
{
	short		icon_inx;				// Index of the icon in the grid of icons.
	short		off_x;					// Width of the extra inline spacing or the horz shift for the whole line icon.
	short		off_y;					// Vertical shift of the icon.
	short		extra_spc_below;		// Height of the extra spacing below the line with decoration.
};

//
//  Note that placing zero in the "icon_inx" field silently blocks displaying the icon.
//
static const TLangViewSourceViewSpacelessDecorProps g_MLFV_SpacelessDecorsInfo[spcdi_num_types] =
{
	{ 0, 0, 0, },							// spcdi_none

	{ 300,			 6,	 6,	 12	},		// spcdi_drive_subst_event_mark
	{ 301,			 6,	 6,	 12	},		// spcdi_temp_file_reg_mark
	{ 302,			 6,	-6,	  0	},		// spcdi_paste_buff_beg_mark
	{ 303,			 6,	-6,	  0	},		// spcdi_paste_buff_end_mark

	{ 0,				 6,	-6,	  0	},		// spcdi_pbuff_at_lex_src_beg
	{ 0,				 6,   -6,	  0	},		// spcdi_pbuff_at_lex_src_end
	{ 0,				 6,   -6,	  0	},		// spcdi_pbuff_ends_in_call
	{ 0,				 6,   -6,	  0	},		// spcdi_pbuff_starts_after_call
	{ 0,				 6,   -6,	  0	},		// spcdi_pbuff_src_start_into_call
	{ 0,				 6,   -6,	  0	},		// spcdi_pbuff_out_of_call_src_end

	{ 400,			 8,	-2,	  0	},		// spcdi_ant_env_call_mark
	{ 401,			 8,	-2,	  0	},		// spcdi_bltin_defs_call_mark
	{ 402,			10,	-2,	  0	},		// spcdi_mj_src_call_mark
	{ 403,			 7,	 3,	  9	},		// spcdi_scgm_call_mark
	{ 404,			 7,	 3,	  9	},		// spcdi_paste_buff_call_mark
	{ 405,			 6,	 2,	  8	},		// spcdi_bad_spaceless_call_mark

	{ 601,			 0,	 2,	  5	},		// spcdi_warning_area_end_mark
	{ 501,			 0,	 2,	  5	},		// spcdi_error_area_end_mark
	{ 600,			-8,	-4,	  0	},		// spcdi_warning_whole_line_mark		<WHOLE_LINE_ICON>
	{ 500,			-8,	-4,	  0	},		// spcdi_error_whole_line_mark			<WHOLE_LINE_ICON>

	{ 200,			 9,	-5,	  0	},		// spcdi_curr_layer_scan_pos
	{ 201,			 8,	-5,	  0	},		// spcdi_lower_layer_scan_pos
	{ 204,			 6,	-3,	  0	},		// spcdi_spec_loc_marker
	{ 203,			 6,	-5,	  0	},		// spcdi_spaceless_object_slct

};

// - - - - - - - -  Ext Params group  - - - - - - - - -

static const TColor g_MLFV_ExtParamBkgrColors[ebpi_num_types] =
{
	RGB(240, 240, 220),		// ebpi_code_area_slct_bkgr

	RGB(255, 253, 190),		// ebpi_active_fragm_nums_bkgr1
	RGB(218, 218, 218),		// ebpi_active_fragm_marks_bkgr1
	RGB(255, 253, 176),		// ebpi_active_fragm_nums_bkgr2
	RGB(212, 212, 212),		// ebpi_active_fragm_marks_bkgr2

	cs_transparent,			// ebpi_cpp_object_bkgr
};

static const TTextStyleProps g_MLFV_ExtParamFrgrStyleProps[efpi_num_types] =
{
	// This is the line numbers font for the selected code area (efpi_code_area_slct_lnums).
	{  NULL,	 ARIAL,		11, TRUE,	RGB(23, 101, 101), RGB(240, 240, 220),	cs_transparent, fcs_none,	RGB(180, 180, 180), 1,      1, 0, -1,   0, 1 },
};

static const TTextStyleSymbolAdjustInfo g_MLFV_CodeAreaSlctLineNumsAdjustments[] =
{
	{ L'0', 0, 1, 1 },	{ L'1', 0, 1, 1 },	{ L'2', 0, 1, 1 },	{ L'3', 0, 1, 1 },
	{ L'4', 0, 1, 1 },	{ L'5', 0, 1, 1 },	{ L'6', 0, 1, 1 },	{ L'7', 0, 1, 1 },
	{ L'8', 0, 1, 1 },	{ L'9', 0, 1, 1 },	{ L':', 0, 1, 1 },	{ 0, 0, 0, 0 },
};

static TLangViewSourceFileFramingProps g_MLFV_DefaultFramingProps =
{
	4, 6,							// frp_lines_above, frp_lines_below.
	12, 16,						// frp_slct_above,  frp_slct_below.
};

// - - - - - - - - - - -  Methods  - - - - - - - - - - - - -

TLangViewSourceFileStyle::TLangViewSourceFileStyle(bool want_vert_text_offs, bool want_frame_shadow)
{
	m_vert_text_offs = want_vert_text_offs;
	m_frame_shadow = want_frame_shadow;
	m_stls_prepared = FALSE;
}

bool TLangViewSourceFileStyle::SetupDefaultSrcProps()
{
	assert(m_stls_prepared == FALSE);
	bool fsucc = TRUE;

	// Whole file styles.
	fsucc &= m_non_ascii_picts.Setup(NULL, g_LangViewSourceCodeIconsProps);
	for (int wfsi=0; wfsi<wfsi_num_types; ++wfsi)
	{
		// Prepare text styles that will be used in the decorated file screen item style.
		m_line_nums_styles[wfsi].Setup(g_MLFV_WholeFileTextProps[wfsi].nums_props);
		m_default_text_styles[wfsi].Setup(g_MLFV_WholeFileTextProps[wfsi].text_props);
		m_line_overflow_styles[wfsi].Setup(g_MLFV_WholeFileTextProps[wfsi].ovfl_props);

		// Prepare the whole file style.
		TDecoratedFileViewStyleProps wf_stlyle = g_MLFV_WholeFileStyleProps[wfsi];
		if (m_vert_text_offs == FALSE)
			wf_stlyle.text_top_offs = wf_stlyle.text_bottom_offs = 0;
		if (m_frame_shadow == TRUE)
			wf_stlyle.shadow_down_width = 2;

		// Prepare style for the decorated file screen item itself.
		m_file_view_styles[wfsi].Setup(wf_stlyle, g_MLFV_TabsSpacesViewMode, FALSE,
								&m_line_nums_styles[wfsi],
								&m_default_text_styles[wfsi],
								&m_line_overflow_styles[wfsi],
								&m_non_ascii_picts, g_MLFV_NonAsciiSubsts);

		// Text bkgr should fill the horz spacing gaps.
		m_file_view_styles[wfsi].m_text_area_bkgr_style.m_props.inner_margin_left = -1;
		m_file_view_styles[wfsi].m_text_area_bkgr_style.m_props.inner_margin_right = -1;
	}

	// Init the framing props.
	m_framing_props = g_MLFV_DefaultFramingProps;

	// Prepare the code status background styles.
	for (int cstt=0; cstt<cexr_num_types; ++cstt)
	{
		m_code_status_line_nums_style[cstt].Setup(g_MLFV_CodeStatusDecorsInfoLineNumbers[cstt].decor_color);
		m_code_status_line_nums_style[cstt].m_props.css_style_name = g_MLFV_CodeStatusDecorsInfoLineNumbers[cstt].decor_css_style;
		m_code_status_marks_area_style[cstt].Setup(g_MLFV_CodeStatusDecorsInfoMarksArea[cstt].decor_color);
		m_code_status_marks_area_style[cstt].m_props.css_style_name = g_MLFV_CodeStatusDecorsInfoMarksArea[cstt].decor_css_style;
		m_code_status_line_body_style[cstt].Setup(g_MLFV_CodeStatusDecorsInfoLineBody[cstt].decor_color);
		m_code_status_line_body_style[cstt].m_props.css_style_name = g_MLFV_CodeStatusDecorsInfoLineBody[cstt].decor_css_style;
		m_code_status_line_body_style[cstt].m_props.inner_margin_left = -1;
		m_code_status_line_body_style[cstt].m_props.inner_margin_right = -1;
	}

	// Indexing key use styles.
	for (int rctx=0; rctx<rctx_num_types; ++rctx)
	{
		m_indexing_key_use_style[rctx].Setup(g_MLFV_IndexingKeyUseColors[rctx]);
	}

	// Cmdl lexema styles.
	for (int rcmsi=0; rcmsi<rcmsi_num_types; ++rcmsi)
	{
		m_cmdl_lexema_frgr_style[rcmsi].Setup(g_MLFV_CmdlLexemaFrgrStyleProps[rcmsi]);
	}

	// CLang lexema styles.
	for (int rclsi=0; rclsi<rclsi_num_types; ++rclsi)
	{
		const TTextStyleSymbolAdjustInfo *adj_data = (rclsi == rclsi_tkn_preproc) ? g_MLFV_PreprocKeywordAdjustments : NULL;
		m_clang_lexema_frgr_style[rclsi].Setup(g_MLFV_CLangLexemaFrgrStyleProps[rclsi], adj_data);
	}

	// Prepare the MidLev object background styles.
	for (int mlbk=0; mlbk<mlbsi_num_types; ++mlbk)
	{
		TBasicStyleProps mlbk_props;
		mlbk_props.Init(g_MLFV_MidLevObjectBkgrDecorsInfo[mlbk].decor_color);
		mlbk_props.css_style_name = g_MLFV_MidLevObjectBkgrDecorsInfo[mlbk].decor_css_style;
		mlbk_props.inner_margin_left = mlbk_props.inner_margin_right = g_MLFV_MidLevObjectBkgrDecorsInfo[mlbk].bkgr_inner_exts;
		mlbk_props.outer_padding_left = mlbk_props.outer_padding_right = g_MLFV_MidLevObjectBkgrDecorsInfo[mlbk].bkgr_outer_exts;
		m_mid_lev_obj_bkgr_style[mlbk].Setup(mlbk_props);
	}

	// MidLev object foreground styles.
	for (int mlfr=0; mlfr<mlfsi_num_types; ++mlfr)
	{
		m_mid_lev_obj_frgr_style[mlfr].Setup(g_MLFV_MidLevObjectFrgrStyleProps[mlfr]);
	}

	// Cpp item foreground styles.
	for (int cxar=0; cxar<cxa_num_types; ++cxar)
	{
		m_cpp_object_frgr_style[cxar].Setup(g_MLFV_CppObjectFrgrStyleProps[cxar]);
	}

	// Bitmap based decors.
	fsucc &= m_spaceless_decors_style.Setup(NULL, g_LangViewSourceCodeIconsProps);

	// Misc bkgr styles.
	for (int ebpi=0; ebpi<ebpi_num_types; ++ebpi)
	{
		m_ext_bkgr_style[ebpi].Setup(g_MLFV_ExtParamBkgrColors[ebpi]);
	}

	// Misc text styles.
	for (int efpi=0; efpi<efpi_num_types; ++efpi)
	{
		const TTextStyleSymbolAdjustInfo *adj_data = (efpi == efpi_code_area_slct_lnums) ? g_MLFV_CodeAreaSlctLineNumsAdjustments : NULL;
		m_ext_frgr_style[efpi].Setup(g_MLFV_ExtParamFrgrStyleProps[efpi], adj_data);
	}

	m_stls_prepared = TRUE;
	return(fsucc);
}

bool TLangViewSourceFileStyle::SetupForDeviceContext(HDC hDC)
{
	assert(m_stls_prepared == TRUE);
	bool fsucc = TScreenItemStyle::SetupForDeviceContext(hDC);

	// Whole file styles.
	fsucc &= m_non_ascii_picts.SetupForDeviceContext(hDC);
	for (int wfsi=0; wfsi<wfsi_num_types; ++wfsi)
		fsucc &= m_file_view_styles[wfsi].SetupForDeviceContext(hDC);

	// Cmdl lexema styles.
	for (int rcmsi=0; rcmsi<rcmsi_num_types; ++rcmsi)
		fsucc &= m_cmdl_lexema_frgr_style[rcmsi].SetupForDeviceContext(hDC);

	// CLang lexema styles.
	for (int rclsi=0; rclsi<rclsi_num_types; ++rclsi)
		fsucc &= m_clang_lexema_frgr_style[rclsi].SetupForDeviceContext(hDC);

	// MidLev object foreground styles.
	for (int mlfr=0; mlfr<mlfsi_num_types; ++mlfr)
		fsucc &= m_mid_lev_obj_frgr_style[mlfr].SetupForDeviceContext(hDC);

	// Cpp item foreground styles.
	for (int cxar=0; cxar<cxa_num_types; ++cxar)
		fsucc &= m_cpp_object_frgr_style[cxar].SetupForDeviceContext(hDC);

	// Bitmap based decors.
	fsucc &= m_spaceless_decors_style.SetupForDeviceContext(hDC);

	// Misc text styles.
	for (int efpi=0; efpi<efpi_num_types; ++efpi)
		fsucc &= m_ext_frgr_style[efpi].SetupForDeviceContext(hDC);

	// Return the overall result.
	return(fsucc);
}

void TLangViewSourceFileStyle::SetTabsSpacesVisibMode(bool visib_tabs, bool visib_spaces)
{
	for (int wfsi=0; wfsi<wfsi_num_types; ++wfsi)
		m_file_view_styles[wfsi].SetTabsSpacesVisibMode(visib_tabs, visib_spaces);
}

// ----- Group 1 -----

const TDecoratedFileViewStyle *TLangViewSourceFileStyle::GetWholeFileStyle(TSrcViewWholeFileStyleInx inx) const
{
	assert(m_stls_prepared == TRUE);
	assert(inx >= 0 && inx < wfsi_num_types);
	return(m_file_view_styles+inx);
}

void TLangViewSourceFileStyle::GetCodeStatusBkgrStyle(TConditionalStatus code_status, TBasicStyle *&line_nums, TBasicStyle *&marks_area, TBasicStyle *&line_body)
{
	assert(m_stls_prepared == TRUE);
	assert(code_status >= 0 && code_status < cexr_num_types);

	line_nums = m_code_status_line_nums_style+code_status;
	marks_area = m_code_status_marks_area_style+code_status;
	line_body = m_code_status_line_body_style+code_status;
}

// ----- Group 2 -----

TBasicStyle *TLangViewSourceFileStyle::GetIndexingKeyUseStyle(TKeyType ftik, TRawContextType ctxt, bool active_code)
{
	assert(m_stls_prepared == TRUE);
	return(m_indexing_key_use_style+ctxt);
}

TTextStyle *TLangViewSourceFileStyle::GetCmdlLexemaFrgrStyle(TSrcViewCmdlLexemaStyleInx inx, bool active_code)
{
	assert(m_stls_prepared == TRUE);
	return(m_cmdl_lexema_frgr_style+inx);
}

TTextStyle *TLangViewSourceFileStyle::GetCLangLexemaFrgrStyle(TSrcViewCLangLexemaStyleInx inx, bool active_code)
{
	assert(m_stls_prepared == TRUE);
	return(m_clang_lexema_frgr_style+inx);
}

TBasicStyle *TLangViewSourceFileStyle::GetMidLevObjectBkgrStyle(TSrcViewMidLevBkgrStyleInx inx, short &vert_ext_up, short &vert_ext_down, bool active_code)
{
	assert(m_stls_prepared == TRUE);
	vert_ext_up = g_MLFV_MidLevObjectBkgrDecorsInfo[inx].bkgr_up_ext;
	vert_ext_down = g_MLFV_MidLevObjectBkgrDecorsInfo[inx].bkgr_down_ext;
	return(m_mid_lev_obj_bkgr_style+inx);
}

TTextStyle *TLangViewSourceFileStyle::GetMidLevObjectFrgrStyle(TSrcViewMidLevFrgrStyleInx inx, bool active_code)
{
	assert(m_stls_prepared == TRUE);
	return(m_mid_lev_obj_frgr_style+inx);
}

TBasicStyle *TLangViewSourceFileStyle::GetCppObjectBkgrStyle(short &vert_exts)
{
	assert(m_stls_prepared == TRUE);

	vert_exts = 0;
	return(m_ext_bkgr_style+ebpi_cpp_object_bkgr);
}

TTextStyle *TLangViewSourceFileStyle::GetCppObjectFrgrStyle(TCppSourceAreaType inx)
{
	assert(m_stls_prepared == TRUE);

	// Value of the passed index may come from the database record. Make a better check.
	if (inx <= cxa_none || inx >= cxa_num_types)
		return(m_cpp_object_frgr_style+cxa_none);

	return(m_cpp_object_frgr_style+inx);
}

TScreenItem *TLangViewSourceFileStyle::GetSpacelessDecorScreenItem(TSrcViewSpacelessDecorInx obj_type, short &horz_offset, short &vert_offset,
																	short &vert_spacing_below, bool active_code, int &cnt_decor_creation_errors)
{
	assert(m_stls_prepared == TRUE);
	if (obj_type <= spcdi_none || obj_type >= spcdi_num_types)
	{
		assert(FALSE);
		cnt_decor_creation_errors++;
		return(NULL);
	}

	// The passed index of the decor is valid.
	const TLangViewSourceViewSpacelessDecorProps &info = g_MLFV_SpacelessDecorsInfo[obj_type];
	if (info.icon_inx == 0)
		return(NULL);

	TBitmapItem *bmp_item = new TBitmapItem(0, 0, info.icon_inx);
	if (bmp_item == NULL)
	{
		cnt_decor_creation_errors++;
		return(NULL);
	}

	bmp_item->SetStyle(&m_spaceless_decors_style);
	horz_offset = info.off_x;
	vert_offset = info.off_y;
	vert_spacing_below = info.extra_spc_below;
	return(bmp_item);
}

// ----- Group 3 -----

TBasicStyle *TLangViewSourceFileStyle::GetBkgrParam(TLangViewExtBkgrParamInx inx)
{
	assert(m_stls_prepared == TRUE);
	assert(inx >= 0 && inx < ebpi_num_types);
	return(m_ext_bkgr_style+inx);
}

TTextStyle *TLangViewSourceFileStyle::GetFrgrParam(TLangViewExtFrgrParamInx inx)
{
	assert(m_stls_prepared == TRUE);
	assert(inx >= 0 && inx < efpi_num_types);
	return(m_ext_frgr_style+inx);
}

// --------------------------------------------------------------------------------
//   =================  TLangViewSourceFileScreenItem  =====================
// --------------------------------------------------------------------------------

static const wchar_t *g_MLFV_CodeStatusNames[cexr_num_types] =
{
	L"Passive", L"Active", L"PassiveWithDoubt", L"ActiveWithDoubt", L"Conditional",
};

static const wchar_t *g_MLFV_CondExprResNames[cexr_num_types] =
{
	L"False", L"True", L"FalseWithDoubt", L"TrueWithDoubt", L"Conditional",
};

static const short g_MLFV_CtxMenuErrWarnWeight[lerrc_num_errorclass_types] =
{
	2, 0, 1, 0, 1, 0, 1, 0, 1, 0,
};

TLangViewSourceFileScreenItem::TDecorOwnerWeightBase TLangViewSourceFileScreenItem::g_BuiltInDecorOwnerTypeWeights[svdo_app_decor_types_base] =
{
	TLangViewSourceFileScreenItem::fvdw_file_area_status,	TLangViewSourceFileScreenItem::fvdw_file_area_status,	TLangViewSourceFileScreenItem::fvdw_file_area_status,
	TLangViewSourceFileScreenItem::fvdw_file_area_status,	TLangViewSourceFileScreenItem::fvdw_paste_buff_area,	TLangViewSourceFileScreenItem::fvdw_mid_lev_object,

	TLangViewSourceFileScreenItem::fvdw_raw_lex_object,	TLangViewSourceFileScreenItem::fvdw_raw_lex_object,
	TLangViewSourceFileScreenItem::fvdw_raw_lex_object,	TLangViewSourceFileScreenItem::fvdw_mid_lev_object,
};

TLangViewSourceFileScreenItem::TLangViewSourceFileScreenItem()
			: m_decor_owners(m_decor_owner_creation_errors)
{
	ResetSourceFile();
}

void TLangViewSourceFileScreenItem::ResetSourceFile()
{
	// Reset the state of the base class.
	Reset();

	m_file.Clear();
	m_fname_buffer[0] = 0;

	m_src_enter_event.Clear();
	m_active_area.Clear();
	m_code_stts_area.Clear();

	m_decor_owners.ResetContext();

	m_decor_creation_errors = 0;
	m_decor_owner_creation_errors = 0;

	m_local_strings_dict.ResetDict();
}

bool TLangViewSourceFileScreenItem::SetupSourceFile(TBasicDialog *dlg, TScreenItemsViewObject &scr_view, TFileInfo &file, bool bypass_file_info_data, int line_from, int num_lines)
{
	assert(m_style != NULL);
	ResetSourceFile();

	// Accept the file data.
	if (AcceptFileData(dlg, scr_view, file, bypass_file_info_data, line_from, num_lines) == FALSE)
	{
		return(FALSE);
	}

	// Low level setup in the decorated file view class has succeeded. Set the active area to the whole file.
	m_active_area.doc_id = m_file.doc_id;
	m_active_area.area_len = m_file.info.file_len;
	m_code_stts_area = m_active_area;

	// While highlighting the syntax error messages may pop up.
	AllowUnupdatedPainting(TRUE);

	// Do the syntax highlight.
	TSourceViewScreenItemBuilder builder(Style(), this);
	TSyntaxHighlighter hlpr(builder, m_decor_owners);
	hlpr.DecorateStandAloneFile(m_file);

	AllowUnupdatedPainting(FALSE);
	return(FinalizeSourceFileSetup(dlg));
}

bool TLangViewSourceFileScreenItem::SetupSourceFile(TBasicDialog *dlg, TScreenItemsViewObject &scr_view, TMidLevStoreReader *db, TMidLevScanner *active_scanner, ID call_pars_id, ID call_event_id, int line_from, int num_lines)
{
	// The owning dialog and the database reader are essential for this type of setup.
	assert(dlg != NULL && db != NULL);

	// Pick up the lex src enclosure event from the db.
	if (db->GetLexSourceEnterEvent(call_pars_id, call_event_id) == FALSE)
	{
		ResetSourceFile();
		return(FALSE);
	}

	// Call the real processing.
	return(SetupSourceFile(dlg, scr_view, db, active_scanner, db->EventData().src_enter, line_from, num_lines));
}

bool TLangViewSourceFileScreenItem::SetupSourceFile(TBasicDialog *dlg, TScreenItemsViewObject &scr_view, TMidLevStoreReader *db, TMidLevScanner *active_scanner, CtvLexSourceEnterEvent &encl_enter_event, int line_from, int num_lines)
{
	// The owning dialog and the database reader are essential for this type of setup.
	assert(dlg != NULL && db != NULL);
	assert(m_style != NULL);

	ResetSourceFile();

	// Instantiate the syntax highlight objects.
	TSourceViewScreenItemBuilder builder(Style(), this);
	TSyntaxHighlighter hlpr(builder, m_decor_owners);

	// Load the file into the syntax highlight helper.
	if (hlpr.LoadLexScrEnclosureFile(encl_enter_event, db, active_scanner) == FALSE)
	{
		// The error is already reported.
		return(FALSE);
	}

	if (line_from <= 0)
	{
		// Caller is asking to select the lines range based on the active area of the call event.
		hlpr.ResolveLinesRangeToDisplay(Style()->FramingProps().frp_lines_above, Style()->FramingProps().frp_lines_below, line_from, num_lines);
	}

	// Accept the file data.
	if (AcceptFileData(dlg, scr_view, hlpr.m_src_file, hlpr.m_src_file_data_owned, line_from, num_lines) == FALSE)
	{
		// Failure setting up the file info into the base class. The error is reported.
		return(FALSE);
	}

	// Ownership on the file data, if it belonged to the highlighter, now belongs to the current object.
	hlpr.m_src_file_data_owned = FALSE;

	// Prepare for potentially long lasting processing.
	HCURSOR hPrevCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
	AllowUnupdatedPainting(TRUE);

	// Make the major call.
	hlpr.DecorateLexSrcEnclosure();

	// Release the mouse pointer.
	AllowUnupdatedPainting(FALSE);
	if (hPrevCursor != NULL)
		::SetCursor(hPrevCursor);

	// Accept the call event and the areas.
	m_src_enter_event = hlpr.m_lex_src_call;
	m_active_area = hlpr.m_encl_active_area;
	m_code_stts_area = hlpr.m_encl_code_stts_area;

	// Display the setup errors if any.
	return(FinalizeSourceFileSetup(dlg));
}

bool TLangViewSourceFileScreenItem::PrepareClickEventInfo(TMidLevStoreReader *db, TMidLevScanner *active_scanner, TLangViewClickEvent &buffer, TScreenItemsViewMouseNotify *mntfi, wchar_t **const_err_info)
{
	if (IsInited() == FALSE)
		return(FALSE);

	//
	//  This method expects that some non NULL screen item was clicked and that this screen item has a non NULL owner.
	//  Note that all decor owners in this viewer are instances of the TSourceViewDecorOwner class.
	//

	// Pick up the app level data from the clicked screen item. The non active area owners should be already filtered out.
	TSourceViewDecorOwner *owner = (TSourceViewDecorOwner*)(mntfi->object->GetOwner());
	DWORD owner_data = mntfi->object->GetOwnerData();
	assert(owner != NULL && owner->m_type >= svdo_code_area && owner->m_type < svdo_app_decor_types_base && owner->m_type != svdo_curr_scan_pos);

	// Clear the output variables.
	buffer.Clear();
	*const_err_info = NULL;

	switch (owner->m_type)
	{
		case svdo_code_area:
				{
					// User clicked some place in the code. Nothing specific was clicked.
					buffer.type = clet_code_area;
					buffer.code_area.Clear();
					buffer.code_area.parsing_id = m_src_enter_event.hdr.parsing_id;
					buffer.code_area.source_ref = m_src_enter_event.hdr.event_id;
					buffer.code_area.src_area.doc_id = m_file.doc_id;

					// Retrieve the clicked location of the file.
					POINT app_pt = { mntfi->app_x, mntfi->app_y };
					if (GetFileAreaFromPoint(app_pt, buffer.code_area.src_area.area_beg, buffer.code_area.src_area.area_len) == FALSE)
					{
						*const_err_info = L"Error getting file offset from the screen point";
						return(FALSE);
					}

					// Pick up the linear position of this place.
					buffer.code_area.lin_area.linear_beg = GetLinearOffset(buffer.code_area.src_area.area_beg, (buffer.code_area.src_area.area_len > 0) ? TRUE : FALSE);
					buffer.code_area.lin_area.linear_len = buffer.code_area.src_area.area_len;
				}
				break;

		case svdo_cmdl_lexema:
				{
					// User has clicked a comand line lexema.
					buffer.type = clet_cmdl_lexema;

					// Reconstruct the clicked lexema.
					TCmdlLexema cmdl_lexema;
					if (ReconstructCmdlLexema(cmdl_lexema, owner_data, const_err_info) == FALSE)
						return(FALSE);

					// Convert lexema into the lexema info.
					if (buffer.cmdl_lexema.Init(m_src_enter_event.hdr.parsing_id, cmdl_lexema, &m_local_strings_dict) == FALSE)
					{
						*const_err_info = L"Memory propblems with svdo_cmdl_lexema object";
						return(FALSE);
					}
				}
				break;

		case svdo_clang_lexema:
				{
					// User has clicked a C/C++ lexema.
					buffer.type = clet_clang_lexema;

					// Reconstruct the clicked lexema.
					TLexema lexema;
					if (ReconstructCLangLexema(lexema, owner_data, const_err_info) == FALSE)
						return(FALSE);

					// Convert lexema into the lexema info.
					if (buffer.clang_lexema.Init(m_src_enter_event.hdr.parsing_id, lexema, &m_local_strings_dict) == FALSE)
					{
						*const_err_info = L"Memory propblems with svdo_clang_lexema object";
						return(FALSE);
					}
				}
				break;

		case svdo_buffering_area:
		case svdo_parsing_event:
		case svdo_macro_hdr_object:
				{
					// Click event is the parsing event that is stored in the owner.
					buffer.SetMidLevEvent(owner->m_event_data);
				}
				break;

		default:
			{
				assert(FALSE);
				*const_err_info = L"Unknown or unappropriate owner type";
				return(FALSE);
			}
	}

	// Success.
	return(TRUE);
}

int TLangViewSourceFileScreenItem::GetDecorsListFromPoint(long x, long y, TScreenItem **buffer, int &len_buff, TScreenItem *worker_item)
{
	if (IsInited() == FALSE)
		return(0);

	// Ask generic object to do the major part of the work. It will create a list of the screen items that are under the passed point.
	int cnt = m_view->GetScreenItemsFromPoint(x, y, buffer, len_buff);
	int len_in_buff = __min(cnt, len_buff);

	if (cnt > 0)
	{
		// Check for duplications of decors with the same app layer props. Do this in the most simple and straightforward way.
		for (int ib=0; ib<len_in_buff-1; ++ib)
		{
			TSourceViewDecorOwner *owner = (TSourceViewDecorOwner*)buffer[ib]->GetOwner();
			DWORD owner_data = buffer[ib]->GetOwnerData();

			// Check lower decor layers for decors with the same props.
			for (int ik = ib+1; ik<len_in_buff; ++ik)
			{
				if (buffer[ik]->GetOwner() == owner && buffer[ik]->GetOwnerData() == owner_data)
				{
					// Duplication found. Remove it.
					for (int ir=ik+1; ir<len_in_buff; ++ir)
						buffer[ir-1] = buffer[ir];

					ik--; len_in_buff--; cnt--;
				}
			}
		}
	}

	if (m_src_enter_event.hdr.parsing_id != 0)
	{
		// Check for the code status object.
		bool code_status_item_present = FALSE;
		for (int i=0; i<len_in_buff; ++i)
		{
			TSourceViewDecorOwner *owner = (TSourceViewDecorOwner*)buffer[i]->GetOwner();

			// All owner types that are smaller than svdo_code_area describe the status of the file area.
			if (owner->m_type <= svdo_code_area)
			{
				code_status_item_present = TRUE;
				break;
			}
		}

		// If the code the status item is missing, append it.
		if (code_status_item_present == FALSE)
		{
			POINT app_pt = { x, y };
			long file_offs, area_len;
			if (GetFileAreaFromPoint(app_pt, file_offs, area_len) == TRUE)
			{
				// Clicked place belongs to the file.
				if (m_code_stts_area.ContainsStrict(file_offs) == TRUE || m_code_stts_area.AreaEnd() >= m_file.info.file_len && file_offs >= m_file.info.file_len)
				{
					// Clicked place should be covered with the code status objects.
					if (cnt < len_buff)
					{
						// In the normal situation the code status areas are separated with CR/LF charactes. Nevertheless sometimes
						// there may be no gap between them. Include the back border of the area into the code area only when there
						// is no other area that starts right behind the current area without any gap.
						TSourceViewDecorOwner *code_area_owner = NULL;

						for (TListIter<TSourceViewDecorOwner> iter(m_decor_owners.m_decor_owners); iter; ++iter)
						{
							TSourceViewDecorOwner *owner = iter.CurrItemPtr();
							if (owner->m_type == svdo_code_area && owner->m_src_area.Contains(file_offs) == TRUE)
							{
								if (code_area_owner != NULL)
									break;
								code_area_owner = owner;
							}
						}

						if (code_area_owner == NULL)
						{
							// The situation is not nice. Most likely the view is corrupted. Use the owner object
							// that is passed in the work_item. It should be not NULL.
							code_area_owner = (TSourceViewDecorOwner*)worker_item->GetOwner();
							assert(code_area_owner != NULL);

							// Prepare the fake code area with an empty intro event.
							code_area_owner->ClearEventData();
							code_area_owner->m_src_area.doc_id = m_file.doc_id;
							code_area_owner->m_src_area.area_beg = file_offs;
							code_area_owner->m_src_area.area_len = area_len;
						}

						worker_item->SetOwner(code_area_owner);
						buffer[cnt] = worker_item;
						len_in_buff++;
					}

					cnt++;
				}
			}
		}
	}

	// Sort decoration objects using the slow algorithm.
	bool wrong_order;
	do
	{
		wrong_order = FALSE;
		for (int is=0; is<len_in_buff-1; ++is)
		{
			TSourceViewDecorOwner *o1 = (TSourceViewDecorOwner*)buffer[is]->GetOwner();
			TSourceViewDecorOwner *o2 = (TSourceViewDecorOwner*)buffer[is+1]->GetOwner();

			bool swap_items = FALSE;
			if (GetCtxMenuWeight(o2) < GetCtxMenuWeight(o1))
			{
				swap_items = TRUE;
			}
			else if (o1->m_type == svdo_parsing_event && o2->m_type == svdo_parsing_event)
			{
				// Both items are the mid lev objects.
				if (o2->m_event_data.hdr.event_type == pset_error_warning_event)
				{
					if (o1->m_event_data.hdr.event_type != pset_error_warning_event)
					{
						swap_items = TRUE;
					}
					else
					{
						// Both items are the error_warning events.
						if (g_MLFV_CtxMenuErrWarnWeight[o2->m_event_data.err_warn.err_class] < g_MLFV_CtxMenuErrWarnWeight[o1->m_event_data.err_warn.err_class])
							swap_items = TRUE;
					}
				}
			}

			if (swap_items == TRUE)
			{
				TScreenItem *xt = buffer[is];
				buffer[is] = buffer[is+1];
				buffer[is+1] = xt;
				wrong_order = TRUE;
			}
		}
	}
	while (wrong_order == TRUE);

	// Give out the results.
	len_buff = len_in_buff;
	return(cnt);
}

void TLangViewSourceFileScreenItem::GetCtxMenuObjectDescription(TSourceViewDecorOwner *owner, DWORD owner_data, wchar_t *buffer, long buff_len)
{
	assert(buff_len >= 120);

	if (IsInited() == FALSE)
	{
		wcscpy(buffer, L"TLangViewSourceFileScreenItem is not inited.");
		return;
	}

	switch (owner->m_type)
	{
		case svdo_out_of_scope_code:
				{
					wcscpy(buffer, L"(C) Out of scope code (this area was not intended to be parsed).");
				}
				break;

		case svdo_aborted_scan_code:
				{
					wcscpy(buffer, L"(C) Unscanned code (the parsing was aborted).");
				}
				break;

		case svdo_unscanned_code:
				{
					wcscpy(buffer, L"(C) Unscanned code (the parsing has not reached this place yet).");
				}
				break;

		case svdo_code_area:
				{
					if (owner->m_event_data.hdr.parsing_id != 0)
					{
						// This is regular case. Even the root area that does not have any intro event has the parsing id set.
						wchar_t cond_ref_buff[80];
						if (owner->m_event_data.hdr.event_id != 0)
							swprintf(cond_ref_buff, 80, L", CondRef=%lu.%lu", owner->m_event_data.hdr.parsing_id, owner->m_event_data.hdr.event_id);
						else cond_ref_buff[0] = 0;

						swprintf(buffer, buff_len, L"(C) Source code area. CodeStatus=\"%s\", SourceRef=%lu.%lu%s.",
								g_MLFV_CodeStatusNames[owner->m_event_data.cond_pars.new_code_stt],
								m_src_enter_event.hdr.parsing_id, m_src_enter_event.hdr.event_id, cond_ref_buff);
					}
					else
					{
						// This is special case when some part of the file does not belong to any code area for unclear reason.
						swprintf(buffer, buff_len, L"(C) Source code area status is missing. SrcAreaOffs=%ld, SrcAreaLen=%ld.",
								owner->m_src_area.area_beg, owner->m_src_area.area_len);
					}
				}
				break;

		case svdo_buffering_area:
				{
					swprintf(buffer, buff_len, L"(C) Preprocessor paste buffering area. BuffLayer=%hd, EvtEnter=%lu.%lu.",
							owner->m_event_data.pbuff_chg.new_buff_level, owner->m_event_data.hdr.parsing_id, owner->m_event_data.hdr.event_id);
				}
				break;

		case svdo_cmdl_lexema:
				{
					TCmdlLexema cmdl_lexema;
					wchar_t buff_pos_prm[80], *msg = L"lexema reconstruction error";
					if (ReconstructCmdlLexema(cmdl_lexema, owner_data, &msg) == TRUE)
					{
						// Reconstruction succeeded.
						if (cmdl_lexema.type == cmdl_pos_param)
						{
							if (cmdl_lexema.num_value == 0)
								wcscpy(buff_pos_prm, L"application name");
							else swprintf(buff_pos_prm, 80, L"pos param #%d", cmdl_lexema.num_value);
							msg = buff_pos_prm;
						}
						else
						{
							msg = (wchar_t*)TCmdlLexema::GetLexTypeName(cmdl_lexema.type, FALSE);
						}
					}

					swprintf(buffer, buff_len, L"(L) Raw command line lexema (%s).", msg);
				}
				break;

		case svdo_clang_lexema:
				{
					TLexema lex;
					wchar_t *msg = L"lexema reconstruction error";
					if (ReconstructCLangLexema(lex, owner_data, &msg) == TRUE)
					{
						// Pick the name of lexema type with the lowercase first letter.
						if (lex.type == ltx_keyword)
							msg = (wchar_t*)TLexema::GetKeywordTypeName(lex.keyword_type, FALSE);
						else msg = (wchar_t*)TLexema::GetLexTypeName(lex.type, FALSE);
					}

					swprintf(buffer, buff_len, L"(L) Raw parsing lexema (%s).", msg);
				}
				break;

		case svdo_curr_scan_pos:
				{
					wcscpy(buffer, L"(C) Current scanning position icon.");
				}
				break;

		case svdo_parsing_event:
				{
					// Call the protected function that will handle the most complex case.
					GetCtxMenuMidLevEventDescr(owner->m_event_data, buffer, buff_len);
				}
				break;

		case svdo_macro_hdr_object:
				{
					// Prepare the name of the clicked part of the macrodef header.
					wchar_t stmt_part[120];
					GetCtxMenuMacroDefHeaderOwnerDataDescr(owner_data, stmt_part, 120);
					swprintf(buffer, buff_len, L"(H) Macro def header element (%s). DefEvent=%lu.%lu.",
							stmt_part, owner->m_event_data.hdr.parsing_id, owner->m_event_data.hdr.event_id);
				}
				break;

		default:
			{
				swprintf(buffer, buff_len, L"Bogus decor owner type value: %d.", (int)owner->m_type);
			}
			break;
	}
}

long TLangViewSourceFileScreenItem::GetScrollPosForSourceArea(TSourceAreaInfo &area, long curr_scroll, long window_height, bool show_margins)
{
	if (IsInited() == FALSE)
		return(0);

	// Resolve the borders of the area into the line numbers.
	assert(area.doc_id == m_file.doc_id);
	long line_beg = m_file.GetLineNumber(area.AreaBeg());
	long line_end = m_file.GetLineNumber(area.AreaEnd());
	if (line_beg < 0 || line_end < 0)
		return(curr_scroll);

	// Figure out how many pixels to request above and below.
	long pix_above = 0, pix_below = 0;
	if (show_margins == TRUE)
	{
		// It is ok to request bigger number of pixels than the file has.
		long baseline, hi_ext, lo_ext;
		if (GetLineHeightInfo(line_beg, baseline, hi_ext, lo_ext) == TRUE)
			pix_above = (hi_ext+lo_ext)*Style()->FramingProps().frp_slct_above;
		if (GetLineHeightInfo(line_end, baseline, hi_ext, lo_ext) == TRUE)
			pix_below = (hi_ext+lo_ext)*Style()->FramingProps().frp_slct_below;
	}

	// Call the decorated file view object method to get the proper scroll pos.
	return(GetScrollPosForLinesRange(line_beg, line_end, curr_scroll, window_height, pix_above, pix_below, FALSE));
}

void TLangViewSourceFileScreenItem::ScrollToSourceArea(TSourceAreaInfo &src_area, bool want_horz_scroll, bool show_margins)
{
	// Get the appropriate vert scroll position for this area.
	TScreenItemsViewSpaceInfo spc_info;
	m_view->GetSpaceInfo(spc_info);
	long new_scroll_y = GetScrollPosForSourceArea(src_area, spc_info.scroll_pos.y, spc_info.client_size.cy, show_margins);

	// Get the appropriate horz scroll position for this area.
	long new_scroll_x = 0;
	if (want_horz_scroll == TRUE)
	{
		TDecoratedFileViewStyleProps &props = m_dfv_style->Props();
		long horz_left_margin = (show_margins == TRUE) ? props.line_nums_width+props.marks_area_width : 10;
		long horz_right_margin = (show_margins == TRUE) ? props.marks_area_width : 10;
		new_scroll_x = GetHorzScrollForFirstLineOfFileArea(src_area.area_beg, src_area.area_len,
									spc_info.scroll_pos.x, spc_info.client_size.cx, horz_left_margin, horz_right_margin);
	}

	// Apply the new scroll pos.
	if (new_scroll_x != spc_info.scroll_pos.x || new_scroll_y != spc_info.scroll_pos.y)
		m_view->SetScrollPos(new_scroll_x, new_scroll_y);
}

bool TLangViewSourceFileScreenItem::SelectClickEventArea(TBasicDialog *dlg, TMidLevStoreReader *db, TMidLevScanner *active_scanner, TLangViewClickEvent &event, bool show_margins)
{
	if (IsInited() == FALSE)
		return(FALSE);

	ResetClickEventAreaSelection();

	//
	//  Step 1. Apply the visible selection if any.
	//

	// Decors should be set up as a first step because they can modify the height of the line.
	TSourceLocation evt_loc;
	if (event.type == clet_mid_lev_event)
	{
		// Retrieve source location of this event from the database.
		if (db->GetSourceLocationInfo(evt_loc, event.mldb_event_data, active_scanner) == FALSE)
			return(FALSE);
	}
	else if (event.GetSrcLocation(evt_loc) == FALSE)
	{
		// Passed event does not have any source location.
		return(FALSE);
	}

	// Source location is present. Check if it belongs to the current enclosure.
	if (evt_loc.src_area.doc_id != m_file.doc_id)
	{
		// Passed event describes location in some other file.
		return(FALSE);
	}
	else if (m_src_enter_event.hdr.parsing_id != 0)
	{
		// Source location belongs to the parsed file.
		if (evt_loc.parsing_id != m_src_enter_event.hdr.parsing_id || evt_loc.source_ref != m_src_enter_event.hdr.event_id)
		{
			// This is some different enclosure. There is nothing to highlight.
			return(FALSE);
		}
	}

	// Enclosure is correct. Pick up the line number.
	long line_num = m_file.GetLineNumber(evt_loc.src_area.area_beg);
	if (line_num <= 0)
		return(FALSE);

	bool full_succ = TRUE;
	if (event.type == clet_code_area)
	{
		// Setup the bkgr color in the line number area.
		TBasicStyle *nums_bkgr = Style()->GetBkgrParam(ebpi_code_area_slct_bkgr);
		if (nums_bkgr != NULL)
		{
			full_succ &= AddWholeLineBkgrDecoration(NULL, 0, svzo_click_event_slct, line_num, 1, nums_bkgr, wlbk_nums);
		}

		// Setup the line number decor.
		TTextStyle *nums_style = Style()->GetFrgrParam(efpi_code_area_slct_lnums);
		if (nums_style != NULL)
		{
			full_succ &= AddLineNumFrgrDecoration(NULL, 0, svzo_click_event_slct, line_num, 1, nums_style);
		}
	}
	else if (evt_loc.src_area.area_len > 0)
	{
		// Highlight this non empty area.
		full_succ &= AddSelectionDecoration(NULL, 0, svzo_click_event_slct, evt_loc.src_area.area_beg, evt_loc.src_area.area_len,
										Style()->GetObjectSelectionBkgrColor(), Style()->GetObjectSelectionFrgrColor(), 0);
	}

	if (full_succ == FALSE)
	{
		ResetClickEventAreaSelection();
		return(FALSE);
	}

	// Finalize applying decorations.
	UpdateControl();

	// Step 2. Scroll the file to this place.
	ScrollToSourceArea(evt_loc.src_area, ((event.type != clet_code_area) ? TRUE : FALSE), show_margins);

	// Step 3. Display mboxes if the selection is not visible.
	if (evt_loc.src_area.area_len <= 0 && event.type != clet_code_area)
	{
		// There is nothing visible on the file. This is either something bogus or this is an error with an empty body.
		if (event.type == clet_mid_lev_event && event.mldb_event_data.hdr.event_type == pset_error_warning_event)
		{
			dlg->MsgBoxFmt(MB_OK, L"Error/warning event has an empty body.\r\n\r\n"
							L"  ParsingId: \t%lu\r\n  EventId: \t%lu\r\n  LineNum: \t%ld\r\n",
							event.mldb_event_data.hdr.parsing_id, event.mldb_event_data.hdr.event_id, line_num);
		}
	}

	// Success.
	return(TRUE);
}

void TLangViewSourceFileScreenItem::ResetClickEventAreaSelection()
{
	if (IsInited() == FALSE)
		return;

	// Remove the selection decorations.
	RemoveDecoration(NULL, svzo_click_event_slct);
	UpdateControl();
}

void TLangViewSourceFileScreenItem::SetupSpecialLocationMarker(long src_offs)
{
	if (IsInited() == FALSE)
		return;

	RemoveDecoration(m_decor_owners.GetCurrScanPosOwner(), ANY_DECOR_ZORDER);

	if (src_offs >= 0 && src_offs <= m_file.info.file_len)
	{
		// Setup linear area equal to the source area. The difference between them will be passed as zero.
		ApplyMidLevImposedDecor(m_decor_owners.GetCurrScanPosOwner(), spcdi_spec_loc_marker, src_offs);
		UpdateControl();

		// Prepare the source area using the current file id.
		TSourceAreaInfo src_area = { { m_file.pars_id, m_file.file_id }, src_offs, 0 };
		ScrollToSourceArea(src_area, TRUE, TRUE);
	}
	else
	{
		// Only the mark was removed. Display the changes.
		UpdateControl();
	}
}

bool TLangViewSourceFileScreenItem::AcceptFileData(TBasicDialog *dlg, TScreenItemsViewObject &scr_view, TFileInfo &file, bool bypass_file_info_data, int line_from, int num_lines)
{
	// Bypassed file struct should contain the file name, the file body and the lines info.
	assert(file.IsNull() == FALSE);
	if (file.info.file_name == NULL)
	{
		dlg->MsgBox(L"TLangViewSourceFileScreenItem: Passed file does not have the file name.");
		return(FALSE);
	}
	else if (file.info.file_body == NULL)
	{
		dlg->MsgBoxFmt(MB_OK, L"TLangViewSourceFileScreenItem: Passed file \"%s\" does not have the file body.", file.info.file_name);
		return(FALSE);
	}
	else if (file.lines_info == NULL || file.lines_info_len <= 0)
	{
		dlg->MsgBoxFmt(MB_OK, L"TLangViewSourceFileScreenItem: Passed file \"%s\" does not have the lines info.", file.info.file_name);
		return(FALSE);
	}

	// Copy in the file. Note that it is ok to copy the pointer to the file body and the lines info. The body
	// will be used as it was passed by the caller regardless if the ownership rights are passed or not.
	m_file = file;
	if (m_file.info.file_len < 0)
		m_file.info.file_len = (long)wcslen(m_file.info.file_body);

	// Pick up the main file view style and ensure that it is available.
	const TDecoratedFileViewStyle *primary_file_style = Style()->GetWholeFileStyle(wfsi_whole_file);
	assert(primary_file_style != NULL);

	// Setup the file data into the base class. Note that the base class displays the body of the file
	// and it does not know the name of that file.
	bool r1 = Setup(&scr_view, (TDecoratedFileViewStyle*)primary_file_style,		// The view object and the style.
					m_file.info.file_body, m_file.info.file_len, (bypass_file_info_data == TRUE) ? dbm_take_ownsh : dbm_use_only,		// File data.
					0, 1,													// File data src offs and its first line number.
					m_file.lines_info, m_file.lines_info_len, (bypass_file_info_data == TRUE) ? dbm_take_ownsh : dbm_use_only,		// Lines info.
					line_from, num_lines);

	if (r1 == FALSE)
	{
		dlg->MsgBoxFmt(MB_OK, L"TLangViewSourceFileScreenItem: Error displaying \"%s\".", file.info.file_name);
		m_file.Clear();
		return(FALSE);
	}

	// Copy in the file name.
	wcsncpy(m_fname_buffer, m_file.info.file_name, sizeof(m_fname_buffer)/sizeof(wchar_t));
	m_fname_buffer[sizeof(m_fname_buffer)/sizeof(wchar_t)-1] = 0;
	m_file.info.file_name = NULL;

	if (bypass_file_info_data == TRUE)
	{
		// The name of the file was not passed to the base class, while the caller wants to pass all data.
		free(file.info.file_name);
		file.Clear();
	}

	// Success.
	return(TRUE);
}

bool TLangViewSourceFileScreenItem::FinalizeSourceFileSetup(TBasicDialog *dlg)
{
	assert(IsInited() == TRUE);
	bool overall_success = TRUE;

	// Finalize the setup procedure inside the base object.
	if (UpdateControl() == FALSE)
	{
		dlg->MsgBoxFmt(MB_OK, L"TLangViewSourceFileScreenItem: Error preparing screen items for \"%s\".", FileNameForDisplay());
		overall_success = FALSE;
	}

	// Show info about the owner allocation errors.
	if (m_decor_owner_creation_errors > 0)
	{
		dlg->MsgBoxFmt(MB_OK, L"Decoration owner creation encountered %d problem%s.", m_decor_owner_creation_errors, PluralSuffix(m_decor_owner_creation_errors));
		overall_success = FALSE;
	}

	// Show info about the decoration creation errors.
	if (m_decor_creation_errors > 0)
	{
		dlg->MsgBoxFmt(MB_OK, L"Decoration creation encountered %d problem%s.", m_decor_creation_errors, PluralSuffix(m_decor_creation_errors));
		overall_success = FALSE;
	}

	return(overall_success);
}

bool TLangViewSourceFileScreenItem::ReconstructCmdlLexema(TCmdlLexema &lexema, DWORD owner_data, wchar_t **const_err_info)
{
	// Instantiate and setup the scanner.
	TCmdlRawScanner scanner(NULL, NULL, &m_file, &m_local_strings_dict);

	// Find out the key start chars and the keys dict for the current file type.
	TParseEnvInfo *pars_env_info = TParseEnvInfo::FindParseEnvForRawScanningType(m_file.rscn_type);

	// Setup the scanner.
	if (pars_env_info != NULL)
	{
		scanner.SetKeyParamsInfo(pars_env_info->cmdl_ks_chars, pars_env_info->cmdl_keys_dict);
	}

	long offs = owner_data & cmod_scan_pos_mask;
	if (scanner.Seek(offs, -1) == FALSE)
	{
		*const_err_info = L"Bogus file offset value in the command line lexema object.";
		return(FALSE);
	}

	// Read the lexema.
	scanner.GetLex(lexema);
	if (lexema.type == cmdl_pos_param)
		lexema.num_value = (owner_data & cmod_pos_prm_val_mask) >> cmod_pos_prm_val_shift;

	// Success. Fill in the missing parts of the lexema origin.
	lexema.key_name_origin.source_ref = m_src_enter_event.hdr.event_id;
	lexema.key_name_origin.cond_ref = 0xFFFFFFFF;
	lexema.key_name_origin.PrepareLinAreaFromLinOffs(GetLinearOffset(lexema.key_name_origin.src_area.area_beg, TRUE));
	lexema.key_name_origin.partial_src_area = lexema.key_name_origin.mixed_cond_pars = FALSE;
	lexema.value_origin.source_ref = m_src_enter_event.hdr.event_id;
	lexema.value_origin.cond_ref = 0xFFFFFFFF;
	lexema.value_origin.PrepareLinAreaFromLinOffs(GetLinearOffset(lexema.value_origin.src_area.area_beg, TRUE));
	lexema.value_origin.partial_src_area = lexema.value_origin.mixed_cond_pars = FALSE;
	return(TRUE);
}

bool TLangViewSourceFileScreenItem::ReconstructCLangLexema(TLexema &lexema, DWORD owner_data, wchar_t **const_err_info)
{
	// Instantiate the scanner.
	TRawScanner scanner(&m_file, &m_local_strings_dict);

	// Seek to the beg of the lexema.
	long offs = owner_data & clod_scan_pos_mask;
	bool new_ln_flag = ((owner_data & clod_new_line_stt) != 0) ? TRUE : FALSE;
	bool inc_fn_flag = ((owner_data & clod_inc_fname_stt) != 0) ? TRUE : FALSE;
	if (scanner.Seek(offs, -1, new_ln_flag, inc_fn_flag) == FALSE)
	{
		*const_err_info = L"Bogus file offset value in the C/C++ lexema object.";
		return(FALSE);
	}

	// Read the lexema.
	scanner.GetLex(lexema);

	// Fill in the rest of the origin fields. The cond ref is not needed but still fill in something there.
	lexema.origin.source_ref = m_src_enter_event.hdr.event_id;
	lexema.origin.cond_ref = 0xFFFFFFFF;
	lexema.origin.PrepareLinAreaFromLinOffs(GetLinearOffset(lexema.origin.src_area.area_beg, TRUE));
	lexema.origin.partial_src_area = lexema.origin.mixed_cond_pars = FALSE;
	return(TRUE);
}

long TLangViewSourceFileScreenItem::GetLinearOffset(long src_file_offset, bool non_empty_area)
{
	if (m_src_enter_event.hdr.parsing_id == 0 || m_src_enter_event.hdr.event_id == 0)
		return(0);

	// The parsing context is available.
	assert(m_src_enter_event.LinearPosAfterEntering() > 0);
	if (m_active_area.Contains(src_file_offset) == FALSE)
		return(0);

	// Passed src offset belongs to the active area.
	long lin_src_delta = m_src_enter_event.LinearPosAfterEntering()-m_active_area.area_beg;
	for (TListIter<TSourceViewDecorOwner> iter(m_decor_owners.m_decor_owners); iter; ++iter)
	{
		TSourceViewDecorOwner *owner = &iter.CurrItem();
		if (owner->m_type == svdo_parsing_event && owner->m_event_data.hdr.event_type == pset_lex_source_enter)
		{
			if (src_file_offset < owner->m_src_area.AreaEnd() || (src_file_offset == owner->m_src_area.AreaBeg() && non_empty_area == FALSE))
			{
				// Passed offset belongs to the current segment of consequtive linear addresses.
				return(src_file_offset+lin_src_delta);
			}

			if (owner->m_event_data.src_enter.evt_exit_id == 0)
			{
				// The lex source call was not exited. It is not possible to establish relation between
				// the passed offset and the linear space of the parsing process.
				return(0);
			}

			// Update the linear offset.
			lin_src_delta = owner->m_event_data.src_enter.evt_exit_linear_after_ret-owner->m_src_area.AreaEnd();
		}
	}

	// Passed offest stays after the last lex src call stmt or there were no call statements at all.
	return(src_file_offset+lin_src_delta);
}

//----- Group 2. Single MidLev objects. -----

void TLangViewSourceFileScreenItem::ApplyMidLevImposedDecor(TSourceViewDecorOwner *decor_owner, TSrcViewSpacelessDecorInx inx, long src_offs)
{
	// Create the decoration screen item.
	short horz_offs, vert_offs, vert_spacing_below;
	TScreenItem *scr_item = Style()->GetSpacelessDecorScreenItem(inx, horz_offs, vert_offs, vert_spacing_below, TRUE, m_decor_creation_errors);
	if (scr_item == NULL)
		return;

	// Paint an inline mark at the end of the event area.
	CreateImposedDecoration(decor_owner, 0, svzo_error_warning, src_offs, scr_item, 0, vert_offs);
	if (horz_offs > 0)
		CreateHorzSpacingDecoration(decor_owner, 0, svzo_error_warning, src_offs, horz_offs, horz_offs);

	if (vert_spacing_below > 0)
	{
		// Enlarge the line height to avoid overlapping with the next line.
		long line_end = m_file.GetLineNumber(src_offs);
		assert(line_end > 0);

		CreateWholeLineSpacingDecoration(decor_owner, 0, svzo_error_warning, line_end, FALSE, vert_spacing_below, TRUE);
	}
}

// ----------- File area support methods --------------------

void TLangViewSourceFileScreenItem::DecorateLineBeg(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder,
													TSimpleLineInfo *linfo, long offs_end, long extra_spacing_value,
													const TBasicStyle *nums_style, const TBasicStyle *marks_style, const TBasicStyle *text_bkgr_style,
													const TTextStyle *line_nums_frgr_style, const TTextStyle *text_frgr_style)
{
	// Passed offset should belong to the line. Among all styles only the text background cannot be NULL.
	assert(offs_end >= linfo->LineBeg() && offs_end <= linfo->LineEnd());
	assert(text_bkgr_style != NULL);

	// Decorate the line numbers area and the marks area if needed.
	DecorateLinesArea(owner, owner_data, zorder, linfo, linfo, nums_style, marks_style, NULL, line_nums_frgr_style, NULL);

	if (text_bkgr_style != NULL)
		CreateWholeLineBkgrDecoration(owner, owner_data, zorder, m_file.GetLineNumber(linfo), 1, text_bkgr_style,  wlbk_ltext);

	if (offs_end > linfo->LineBeg())
	{
		// At least part of the line body is involved.
		long len_fragm = offs_end-linfo->LineBeg();

		//  Highlight the body of the line.
		if (text_bkgr_style != NULL)
			CreateBkgrDecoration(owner, owner_data, zorder, linfo->LineBeg(), len_fragm, text_bkgr_style, -1, -1);
		if (text_frgr_style != NULL)
			CreateFrgrDecoration(owner, owner_data, zorder, linfo->LineBeg(), len_fragm, text_frgr_style);
	}

	if (extra_spacing_value > 0)
		CreateHorzSpacingDecoration(owner, owner_data, zorder, offs_end, extra_spacing_value, extra_spacing_value);
}

void TLangViewSourceFileScreenItem::DecorateLineMiddle(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder,
													long area_beg, long area_end, long extra_spacing_front, long extra_spacing_back,
													const TBasicStyle *nums_style, const TBasicStyle *marks_style, const TBasicStyle *text_bkgr_style,
													const TTextStyle *line_nums_frgr_style, const TTextStyle *text_frgr_style)
{
	TSimpleLineInfo *linfo = m_file.GetLineInfoFromFileOffset(area_beg);
	assert(linfo != NULL);

	// Ensure that the passed area is located inside the line.
	assert(area_end >= area_beg && area_beg >= linfo->LineBeg() && area_end <= linfo->LineEnd());

	// Decorate the line numbers area and the marks area if needed.
	DecorateLinesArea(owner, owner_data, zorder, linfo, linfo, nums_style, marks_style, NULL, line_nums_frgr_style, NULL);

	if (linfo->line_len > 0)
	{
		// Create spacing at the beg of the area.
		if (extra_spacing_front > 0)
			CreateHorzSpacingDecoration(owner, owner_data, zorder, area_beg, extra_spacing_front, extra_spacing_back);

		if (area_end > area_beg)
		{
			// Highlight the body of the line.
			if (text_bkgr_style != NULL)
				CreateBkgrDecoration(owner, owner_data, zorder, area_beg, area_end-area_beg, text_bkgr_style, -1, -1);
			if (text_frgr_style != NULL)
				CreateFrgrDecoration(owner, owner_data, zorder, area_beg, area_end-area_beg, text_frgr_style);
		}

		// Create spacing at the end of the area.
		if (extra_spacing_back > 0)
			CreateHorzSpacingDecoration(owner, owner_data, zorder, area_end, extra_spacing_back, extra_spacing_back);
	}
}

void TLangViewSourceFileScreenItem::DecorateLineEnd(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder,
													TSimpleLineInfo *linfo, long offs_beg, long extra_spacing_value,
													const TBasicStyle *nums_style, const TBasicStyle *marks_style, const TBasicStyle *text_bkgr_style,
													const TTextStyle *line_nums_frgr_style, const TTextStyle *text_frgr_style)
{
	// Passed offset should belong to the line. Among all styles only the text background cannot be NULL.
	assert(offs_beg >= linfo->LineBeg() && offs_beg <= linfo->LineEnd());
	assert(text_bkgr_style != NULL);

	// Decorate the line numbers area and the marks area if needed.
	DecorateLinesArea(owner, owner_data, zorder, linfo, linfo, nums_style, marks_style, NULL, line_nums_frgr_style, NULL);

	if (extra_spacing_value > 0)
		CreateHorzSpacingDecoration(owner, owner_data, zorder, offs_beg, extra_spacing_value, extra_spacing_value);

	if (offs_beg < linfo->LineEnd())
	{
		// Highlight the body of the line.
		long len_fragm = linfo->LineEnd()-offs_beg;

		if (text_bkgr_style != NULL)
			CreateBkgrDecoration(owner, owner_data, zorder, offs_beg, len_fragm, text_bkgr_style, -1, -1);
		if (text_frgr_style != NULL)
			CreateFrgrDecoration(owner, owner_data, zorder, offs_beg, len_fragm, text_frgr_style);
	}

	if (text_bkgr_style != NULL)
		CreateWholeLineBkgrDecoration(owner, owner_data, zorder, m_file.GetLineNumber(linfo), 1, text_bkgr_style,  wlbk_rtext);
}

void TLangViewSourceFileScreenItem::DecorateLinesArea(TSourceViewDecorOwner *owner, DWORD owner_data, TSrcViewDecorZorder zorder,
													TSimpleLineInfo *linfo_beg, TSimpleLineInfo *linfo_end,
													const TBasicStyle *nums_style, const TBasicStyle *marks_style, const TBasicStyle *text_bkgr_style,
													const TTextStyle *line_nums_frgr_style, const TTextStyle *text_frgr_style)
{
	assert(linfo_beg != NULL && linfo_end != NULL && linfo_beg <= linfo_end);
	long line_beg  = m_file.GetLineNumber(linfo_beg);
	long num_lines = (long)(linfo_end-linfo_beg)+1;

	// Decorate the line numbers area.
	if (nums_style != NULL)
		CreateWholeLineBkgrDecoration(owner, owner_data, zorder, line_beg, num_lines, nums_style,  wlbk_nums);
	if (line_nums_frgr_style != NULL)
		CreateLineNumFrgrDecoration(owner, owner_data, zorder, line_beg, num_lines, line_nums_frgr_style);

	// Decorate the marks area.
	if (marks_style != NULL)
		CreateWholeLineBkgrDecoration(owner, owner_data, zorder, line_beg, num_lines, marks_style, wlbk_marks);

	// Decorate the text area.
	if (text_bkgr_style != NULL)
		CreateWholeLineBkgrDecoration(owner, owner_data, zorder, line_beg, num_lines, text_bkgr_style,  wlbk_text);
	if (text_frgr_style != NULL)
		CreateFrgrDecoration(owner, owner_data, zorder, linfo_beg->LineBeg(), linfo_end->LineEnd()-linfo_beg->LineBeg(), text_frgr_style);
}

short TLangViewSourceFileScreenItem::GetCtxMenuWeight(TSourceViewDecorOwner *owner)
{
	assert(owner->m_type >= 0 && owner->m_type < svdo_app_decor_types_base);
	return(g_BuiltInDecorOwnerTypeWeights[owner->m_type]);
}

void TLangViewSourceFileScreenItem::GetCtxMenuMidLevEventDescr(CtvParsingEventInfo &data, wchar_t *buffer, long buff_len)
{
	assert(buff_len >= 120);
	CtvEventHeader &hdr = data.hdr;

	// Prepare the event details. Only some of the event types have details.
	wchar_t stmt_dtls[80], fid_buff[40];
	stmt_dtls[0] = 0;
	switch (hdr.event_type)
	{
		case pset_register_temp_file:
				{
					swprintf(stmt_dtls, 80, L"FileId=%s, ", data.temp_file.temp_file_info.PrepareFileId(fid_buff, 40));
				}
				break;

		case pset_paste_buffering_change:
				{
					swprintf(stmt_dtls, 80, L"NewBuffLev=%d, ", data.pbuff_chg.new_buff_level);
				}
				break;

		case pset_lex_source_enter:
				{
					if (LEX_SRC_GEN_PBUFF(data.src_enter.src_type) == TRUE)
					{
						swprintf(stmt_dtls, 80, L"PasteBuffFid=%s%s, ",
								TFileInfo::PrepareFileId(data.hdr.parsing_id, data.src_enter.paste_buff_fid, fid_buff, 40),
								(data.src_enter.append_tail == TRUE) ? L", AppendTail=TRUE" : L"");
					}
					else
					{
						switch (data.src_enter.src_type)
						{
							case srct_cmdl:
							case srct_file:
									swprintf(stmt_dtls, 80, L"PathId=%lu, FileId=%s, ", data.src_enter.inc_path_id, data.src_enter.inc_file_info.PrepareFileId(fid_buff, 40));
									break;

							case srct_macro:
									swprintf(stmt_dtls, 80, L"DefId=%lu, ", data.src_enter.mcall_def_id);
									break;

							case srct_mparam:
									swprintf(stmt_dtls, 80, L"Param=%lu, ", data.src_enter.mcall_param_inx+1);
									break;

							default:
								break;
						}
					}
				}
				break;

		case pset_macro_library_change:
				{
					swprintf(stmt_dtls, 80, L"PrevDefId=%lu, ", data.mdef_chg.prev_def_id);
				}
				break;

		case pset_cond_parsing_change:
				{
					if (data.cond_pars.stmt_type <= mpr_elif)
						swprintf(stmt_dtls, 80, L"ExprResult=\"%s\", ", g_MLFV_CondExprResNames[data.cond_pars.expr_result]);
				}
				break;

		case pset_misc_preproc_stmt:
				{
					if (data.misc_stmt.mis_event_type == exmis_failed_include)
						swprintf(stmt_dtls, 80, L"NonCallReason=%s, ", PrMiscPreprocStmtEvent::GetNonCallReasonName((THiddenIncudeStmtDtls)data.misc_stmt.evt_details));
				}
				break;

		case pset_error_warning_event:
				{
					swprintf(stmt_dtls, 80, L"ErrorCode=%lu, ", data.err_warn.err_code);
				}
				break;

		case pset_cond_param_use:
				{
					swprintf(stmt_dtls, 80, L"ParamId=%lu, ", data.cond_param.cond_param_id);
				}
				break;


		case pset_cpp_item_ref:
				{
					swprintf(stmt_dtls, 80, L"CppItemId=%lu, CppItemType=%s, ", data.cpp_item.pars_sess_cpp_item_id, TCppItemHelper::GetItemTypeDisplayName(data.cpp_item.cpp_item_type));
				}
				break;
	}

	// Combine everything together.
	swprintf(buffer, buff_len, L"%s. %sEvent=%lu.%lu.", data.GetEventTypeName(), stmt_dtls, hdr.parsing_id, hdr.event_id);
}

void TLangViewSourceFileScreenItem::GetCtxMenuMacroDefHeaderOwnerDataDescr(DWORD owner_data, wchar_t *buff, int len_buff)
{
	assert(len_buff >= 40);
	switch (owner_data & mhod_func_part_mask)
	{
		case mhod_mdef_token:	wcscpy(buff, L"macrodef token");	break;
		case mhod_mdef_name:	wcscpy(buff, L"name of the macro");	break;
		case mhod_mdef_delim:	wcscpy(buff, L"delimiter");			break;
		case mhod_mdef_other:	wcscpy(buff, L"other lexema");		break;

		default:
			swprintf(buff, len_buff, L"param #%d", ((owner_data >> mhod_param_inx_shift) & 0xFF)+1);
			break;
	}
}

// ----------- Worker Object --------------------

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyFileViewStyleToUpperArea(TSrcViewWholeFileStyleInx view_style_inx, long area_end, bool whole_line_area_style)
{
	TSourceViewDecorOwner *owner = m_owner_screen_item.m_decor_owners.GetInactiveAreaDecorOwner(view_style_inx);
	const TDecoratedFileViewStyle *dfv_style = m_src_file_style.GetWholeFileStyle(view_style_inx);
	if (dfv_style == NULL)
		return;

	// Once this method is called, the background of the area above all text lines should be decorated.
	m_owner_screen_item.CreateTopBottomAreaBkgrDecoration(owner, TRUE, FALSE, dfv_style->LineNumsBkgr(), wlbk_nums);
	m_owner_screen_item.CreateTopBottomAreaBkgrDecoration(owner, TRUE, FALSE, dfv_style->MarksAreaBkgr(), wlbk_marks);
	m_owner_screen_item.CreateTopBottomAreaBkgrDecoration(owner, TRUE, FALSE, dfv_style->TextAreaBkgr(), wlbk_text);

	TSimpleLineInfo *lb = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(0);
	TSimpleLineInfo *le = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(area_end);
	assert(lb != NULL && le != NULL);

	if (whole_line_area_style == FALSE)
	{
		// Process the end of the upper area first. This will effectively paint the beg of the line.
		m_owner_screen_item.DecorateLineBeg(owner, 0, svzo_inactive_area, le, area_end, m_src_file_style.GetScopeChangeExtraSpace(),
											NULL, NULL, dfv_style->TextAreaBkgr(), NULL, dfv_style->DefaultTextFrgr());
		le--;
	}

	if (le >= lb)
	{
		// The area that consists of one or several completely inactive lines is not empty.
		m_owner_screen_item.DecorateLinesArea(owner, 0, svzo_inactive_area, lb, le, dfv_style->LineNumsBkgr(), dfv_style->MarksAreaBkgr(), dfv_style->TextAreaBkgr(),
												dfv_style->LineNumsFrgr(), dfv_style->DefaultTextFrgr());
	}
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyFileViewStyleToLowerArea(TSrcViewWholeFileStyleInx view_style_inx, long area_beg, bool whole_line_area_style)
{
	TSourceViewDecorOwner *owner = m_owner_screen_item.m_decor_owners.GetInactiveAreaDecorOwner(view_style_inx);
	const TDecoratedFileViewStyle *dfv_style = m_src_file_style.GetWholeFileStyle(view_style_inx);
	if (dfv_style == NULL)
		return;

	TSimpleLineInfo *lb = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(area_beg);
	TSimpleLineInfo *le = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(m_owner_screen_item.m_file.info.file_len);
	assert(lb != NULL && le != NULL);

	if (whole_line_area_style == FALSE)
	{
		// Process the beg of the lower area first. This will paint the end of the line.
		m_owner_screen_item.DecorateLineEnd(owner, 0, svzo_inactive_area, lb, area_beg, m_src_file_style.GetScopeChangeExtraSpace(),
										NULL, NULL, dfv_style->TextAreaBkgr(), NULL, dfv_style->DefaultTextFrgr());
		lb++;
	}

	if (le >= lb)
	{
		// The area that consists of one or several completely inactive lines is not empty.
		m_owner_screen_item.DecorateLinesArea(owner, 0, svzo_inactive_area, lb, le, dfv_style->LineNumsBkgr(), dfv_style->MarksAreaBkgr(), dfv_style->TextAreaBkgr(),
												dfv_style->LineNumsFrgr(), dfv_style->DefaultTextFrgr());
	}

	// Once this method is called, the background of the area below all text lines should be decorated.
	m_owner_screen_item.CreateTopBottomAreaBkgrDecoration(owner, FALSE, TRUE, dfv_style->LineNumsBkgr(), wlbk_nums);
	m_owner_screen_item.CreateTopBottomAreaBkgrDecoration(owner, FALSE, TRUE, dfv_style->MarksAreaBkgr(), wlbk_marks);
	m_owner_screen_item.CreateTopBottomAreaBkgrDecoration(owner, FALSE, TRUE, dfv_style->TextAreaBkgr(), wlbk_text);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyUnscannedAreaStyle(TSrcViewWholeFileStyleInx view_style_inx, long area_beg, long area_end, bool area_end_whole_line_style)
{
	// This is an area with the finite beginning and the finite end.
	assert(area_beg >= 0 && area_end >= 0 && area_end > area_beg);

	TSourceViewDecorOwner *owner = m_owner_screen_item.m_decor_owners.GetInactiveAreaDecorOwner(view_style_inx);
	const TDecoratedFileViewStyle *dfv_style = m_src_file_style.GetWholeFileStyle(view_style_inx);
	if (dfv_style == NULL)
		return;

	TSimpleLineInfo *lb = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(area_beg);
	TSimpleLineInfo *le = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(area_end);
	assert(lb != NULL && le != NULL);

	long ext_horz_spc = m_src_file_style.GetScopeChangeExtraSpace();
	if (lb != le)
	{
		// Process the beginning of the miltiline area. Beginning of the area has always an inline style.
		m_owner_screen_item.DecorateLineEnd(owner, 0, svzo_inactive_area, lb, area_beg, ext_horz_spc,
										NULL, NULL, dfv_style->TextAreaBkgr(), NULL, dfv_style->DefaultTextFrgr());
		lb++;

		if (area_end_whole_line_style == FALSE)
		{
			// Decorate the end of the area.
			m_owner_screen_item.DecorateLineBeg(owner, 0, svzo_inactive_area, le, area_end, ext_horz_spc,
											dfv_style->LineNumsBkgr(), dfv_style->MarksAreaBkgr(), dfv_style->TextAreaBkgr(),
											dfv_style->LineNumsFrgr(), dfv_style->DefaultTextFrgr());
			le--;
		}

		if (le >= lb)
		{
			// Area with one or several completely inactive lines is not empty.
			m_owner_screen_item.DecorateLinesArea(owner, 0, svzo_inactive_area, lb, le,
											dfv_style->LineNumsBkgr(), dfv_style->MarksAreaBkgr(), dfv_style->TextAreaBkgr(),
											dfv_style->LineNumsFrgr(), dfv_style->DefaultTextFrgr());
		}
	}
	else
	{
		// Just one line is involved.
		if (area_end_whole_line_style == TRUE)
		{
			m_owner_screen_item.DecorateLineEnd(owner, 0, svzo_inactive_area, lb, area_beg, ext_horz_spc,
											NULL, NULL, dfv_style->TextAreaBkgr(), NULL, dfv_style->DefaultTextFrgr());
		}
		else
		{
			m_owner_screen_item.DecorateLineMiddle(owner, 0, svzo_inactive_area, area_beg, area_end, ext_horz_spc, ext_horz_spc,
											NULL, NULL, dfv_style->TextAreaBkgr(), NULL, dfv_style->DefaultTextFrgr());
		}
	}
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplySpecialNumsMarksBackgrounds(long pars_beg, long pars_end, bool whole_line_area_style)
{
	TSimpleLineInfo *lb = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(pars_beg);
	TSimpleLineInfo *le = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(pars_end);
	assert(lb != NULL && le != NULL);

	TBasicStyle *nums_bkgr = NULL;
	TBasicStyle *marks_bkgr = NULL;
	if (whole_line_area_style == TRUE)
	{
		if (lb == le)
		{
			// This is single line active area in the "whole line" style.
			nums_bkgr = m_src_file_style.GetBkgrParam(ebpi_active_fragm_nums_bkgr1);
			marks_bkgr = m_src_file_style.GetBkgrParam(ebpi_active_fragm_marks_bkgr1);
		}
	}
	else
	{
		// This is an active area in the "inline" style.
		nums_bkgr = m_src_file_style.GetBkgrParam((lb == le) ? ebpi_active_fragm_nums_bkgr2 : ebpi_active_fragm_nums_bkgr1);
		marks_bkgr = m_src_file_style.GetBkgrParam((lb == le) ? ebpi_active_fragm_marks_bkgr2 : ebpi_active_fragm_marks_bkgr1);
	}

	// Apply the retrieved styles if any.
	m_owner_screen_item.DecorateLinesArea(NULL, 0, svzo_default, lb, le, nums_bkgr, marks_bkgr);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::SetupUnscannedAreaEntranceIcon(long area_entrance_offs, bool lower_layer)
{

	// Create the decoration screen item.
	short spc_for_icon, vert_offs, ext_spacing_below;
	TSrcViewSpacelessDecorInx dcr_inx = (lower_layer == FALSE) ? spcdi_curr_layer_scan_pos : spcdi_lower_layer_scan_pos;
	TScreenItem *scr_item = m_src_file_style.GetSpacelessDecorScreenItem(dcr_inx, spc_for_icon, vert_offs, ext_spacing_below, TRUE, m_owner_screen_item.m_decor_creation_errors);
	if (scr_item == NULL)
		return;

	long horz_offs = 0;
	TSimpleLineInfo *linfo = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(area_entrance_offs);
	if (linfo != NULL && area_entrance_offs == linfo->LineBeg())
	{
		horz_offs = 2;
		spc_for_icon += 2;
	}

	// Creation of the screen item succeeded.
	m_owner_screen_item.CreateImposedDecoration(m_owner_screen_item.m_decor_owners.GetCurrScanPosOwner(), 0, svzo_mid_lev_frgr, area_entrance_offs, scr_item, horz_offs, vert_offs);
	m_owner_screen_item.CreateHorzSpacingDecoration(m_owner_screen_item.m_decor_owners.GetCurrScanPosOwner(), 0, svzo_mid_lev_frgr, area_entrance_offs, spc_for_icon, spc_for_icon);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::SetupUnscannedLineEndArea(long real_src_active_end)
{
	const TDecoratedFileViewStyle *dfv_style = m_src_file_style.GetWholeFileStyle(wfsi_unscanned);
	if (dfv_style != NULL)
		m_owner_screen_item.CreateWholeLineBkgrDecoration(m_owner_screen_item.m_decor_owners.GetInactiveAreaDecorOwner(wfsi_unscanned), 0, svzo_inactive_area, m_owner_screen_item.m_file.GetLineNumber(real_src_active_end), 1, dfv_style->TextAreaBkgr(),  wlbk_rtext);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyMidLevBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevBkgrStyleInx style_inx, TSourceAreaInfo &src_area)
{
	short vert_ext_up, vert_ext_down;
	TBasicStyle *decor_style = m_src_file_style.GetMidLevObjectBkgrStyle(style_inx, vert_ext_up, vert_ext_down, TRUE);
	if (decor_style == NULL)
	{
		// Style object does not want to highlight this part of the mid lev stmt.
		return;
	}

	// Create the main decoration.
	m_owner_screen_item.CreateBkgrDecoration(decor_owner, 0, zorder, src_area.area_beg, src_area.area_len, decor_style, vert_ext_up, vert_ext_down);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceAreaInfo &src_area)
{
	TTextStyle *decor_style = m_src_file_style.GetMidLevObjectFrgrStyle(style_inx, TRUE);
	if (decor_style == NULL)
	{
		// Style object does not want to highlight this part of the mid lev stmt.
		return;
	}

	// Create the frgr decoration.
	m_owner_screen_item.CreateFrgrDecoration(decor_owner, 0, zorder, src_area.area_beg, src_area.area_len, decor_style);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyMidLevWlbkDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, long line_beg, long num_lines)
{
	short vert_ext_up, vert_ext_down;
	TBasicStyle *decor_style = m_src_file_style.GetMidLevObjectBkgrStyle(mlbsi_mpr_stmt_bkgr, vert_ext_up, vert_ext_down, TRUE);
	if (decor_style == NULL)
	{
		// Style object does not want to highlight this part of the mid lev statement.
		return;
	}

	// Add the decoration.
	m_owner_screen_item.CreateWholeLineBkgrDecoration(decor_owner, 0, svzo_mpr_stmt_bkgr, line_beg, num_lines, decor_style, wlbk_text);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyCppObjectBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSourceAreaInfo &src_area)
{
	short vert_exts;
	TBasicStyle *decor_style = m_src_file_style.GetCppObjectBkgrStyle(vert_exts);
	if (decor_style == NULL)
	{
		// Style object does not want to highlight this part of the mid lev statement.
		return;
	}

	// Create the decoration.
	m_owner_screen_item.CreateBkgrDecoration(decor_owner, 0, zorder, src_area.area_beg, src_area.area_len, decor_style, vert_exts, vert_exts);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyCppObjectFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TCppSourceAreaType area_type, TSourceAreaInfo &src_area)
{
	// Convert the type of the area into the style index.
	TTextStyle *decor_style = m_src_file_style.GetCppObjectFrgrStyle(area_type);
	if (decor_style == NULL)
	{
		// Style object does not want to highlight this part of the cpp object.
		return;
	}

	// Add the decoration.
	m_owner_screen_item.CreateFrgrDecoration(decor_owner, 0, svzo_cpp_item_frgr, src_area.area_beg, src_area.area_len, decor_style);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyCodeStatusDecors(TSourceViewDecorOwner *owner, bool whole_line_beg, bool whole_line_end)
{
	TConditionalStatus code_status = owner->m_event_data.cond_pars.new_code_stt;
	TBasicStyle *style_line_numbers = NULL, *style_marks_area = NULL, *style_line_body = NULL;
	m_src_file_style.GetCodeStatusBkgrStyle(code_status, style_line_numbers, style_marks_area, style_line_body);
	if (style_line_numbers == NULL && style_marks_area == NULL && style_line_body == NULL)
	{
		// Style object instructs not to highlight this type of the code status.
		return;
	}

	// Pick up the lines that are involved in this code area.
	long area_beg = owner->m_src_area.AreaBeg();
	long area_end = owner->m_src_area.AreaEnd();
	TSimpleLineInfo *lb = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(area_beg);
	TSimpleLineInfo *le = m_owner_screen_item.m_file.GetLineInfoFromFileOffset(area_end);
	assert(lb != NULL && le != NULL);

	if (whole_line_beg == TRUE && whole_line_end == TRUE)
	{
		// Both sides have the whole line style. The situation is simple.
		m_owner_screen_item.DecorateLinesArea(owner, 0, svzo_code_status, lb, le, style_line_numbers, style_marks_area, style_line_body);
	}
	else
	{
		// At least one of the sides have inline nature.
		long ext_horz_spc = m_src_file_style.GetScopeChangeExtraSpace();

		if (lb != le)
		{
			if (whole_line_beg == FALSE)
			{
				m_owner_screen_item.DecorateLineEnd(owner, 0, svzo_code_status, lb, area_beg, ext_horz_spc, style_line_numbers, style_marks_area, style_line_body);
				lb++;
			}

			if (whole_line_end == FALSE)
			{
				m_owner_screen_item.DecorateLineBeg(owner, 0, svzo_code_status, le, area_end, ext_horz_spc, style_line_numbers, style_marks_area, style_line_body);
				le--;
			}

			if (lb <= le)
			{
				// There is some number of completely covered lines.
				m_owner_screen_item.DecorateLinesArea(owner, 0, svzo_code_status, lb, le, style_line_numbers, style_marks_area, style_line_body);
			}
		}
		else
		{
			// Just one line is involved.
			if (whole_line_beg == TRUE)
			{
				m_owner_screen_item.DecorateLineBeg(owner, 0, svzo_code_status, le, area_end, ext_horz_spc, style_line_numbers, style_marks_area, style_line_body);
			}
			else if (whole_line_end == TRUE)
			{
				m_owner_screen_item.DecorateLineEnd(owner, 0, svzo_code_status, lb, area_beg, ext_horz_spc, style_line_numbers, style_marks_area, style_line_body);
			}
			else
			{
				m_owner_screen_item.DecorateLineMiddle(owner, 0, svzo_code_status, area_beg, area_end, ext_horz_spc, ext_horz_spc,
													style_line_numbers, style_marks_area, style_line_body);
			}
		}
	}
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyCmdlRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCmdlLexemaStyleInx style_inx, TSourceArea &area)
{
	TTextStyle *decor_style = m_src_file_style.GetCmdlLexemaFrgrStyle(style_inx, TRUE);
	if (decor_style == NULL)
	{
		// Style object instructs not to highlight this part of the marco definition header.
		return;
	}

	// Add the decoration.
	m_owner_screen_item.CreateFrgrDecoration(&m_owner_screen_item.m_decor_owners.m_cmdl_lexemas_owner, owner_data, zorder, area.area_beg, area.area_len, decor_style);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyCLangRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCLangLexemaStyleInx style_inx, TSourceArea &area)
{
	TTextStyle *decor_style = m_src_file_style.GetCLangLexemaFrgrStyle(style_inx, TRUE);
	if (decor_style == NULL)
	{
		// Style object instructs not to highlight this part of the marco definition header.
		return;
	}

	// Add the decoration.
	m_owner_screen_item.CreateFrgrDecoration(&m_owner_screen_item.m_decor_owners.m_clang_lexemas_owner, owner_data, zorder, area.area_beg, area.area_len, decor_style);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyMacroHdrFrgrDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceArea &area)
{
	TTextStyle *decor_style = m_src_file_style.GetMidLevObjectFrgrStyle(style_inx, TRUE);
	if (decor_style == NULL)
	{
		// Style object instructs not to highlight this part of the marco definition header.
		return;
	}

	// Add the decoration.
	m_owner_screen_item.CreateFrgrDecoration(&m_owner_screen_item.m_decor_owners.m_macro_header_owner, owner_data, zorder, area.area_beg, area.area_len, decor_style);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyInlineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long src_area_offs)
{
	m_owner_screen_item.ApplyMidLevImposedDecor(decor_owner, decor_type_inx, src_area_offs);
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ApplyWholeLineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long line_number)
{
	// Create the decoration screen item.
	short horz_offs, vert_offs, vert_spacing_below;
	TScreenItem *scr_item = m_src_file_style.GetSpacelessDecorScreenItem(decor_type_inx, horz_offs, vert_offs, vert_spacing_below, TRUE, m_owner_screen_item.m_decor_creation_errors);
	if (scr_item == NULL)
		return;

	// Place decoration in the marks area of the line.
	m_owner_screen_item.CreateWholeLineImposedDecoration(decor_owner, 0, zorder, line_number, scr_item, horz_offs, vert_offs);
}

bool TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::GetVisibleArea(long &visible_area_beg, long &visible_area_len)
{
	visible_area_beg = 0;
	visible_area_len = m_owner_screen_item.m_file.info.file_len;
	return(m_owner_screen_item.IntersectWithVisibleFileArea(visible_area_beg, visible_area_len, TRUE));
}

void TLangViewSourceFileScreenItem::TSourceViewScreenItemBuilder::ReportProcessingProblem(const wchar_t *message)
{
}

// ----------- Different types of decorations --------------------

void TLangViewSourceFileScreenItem::CreateBkgrDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
														long area_beg, long area_len, const TBasicStyle *style, short upper_ext, short lower_ext)
{
	// Check for intersection with the visible area.
	if (IntersectWithVisibleFileArea(area_beg, area_len, FALSE) == FALSE)
		return;

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddBkgrDecoration(owner, data, zo, area_beg, area_len, (TBasicStyle*)style, upper_ext, lower_ext) == FALSE)
		m_decor_creation_errors++;
}

void TLangViewSourceFileScreenItem::CreateFrgrDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
														long area_beg, long area_len, const TTextStyle *style)
{
	// Check for intersection with the visible area.
	if (IntersectWithVisibleFileArea(area_beg, area_len, FALSE) == FALSE)
		return;

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddFrgrDecoration(owner, data, zo, area_beg, area_len, (TTextStyle*)style) == FALSE)
		m_decor_creation_errors++;
}

void TLangViewSourceFileScreenItem::CreateInsertedDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
														long area_offs, TScreenItem *scr_item, short decor_horz_order, long baseline_vert_offs)
{
	// Check for intersection with the visible area.
	long area_len = 0;
	if (IntersectWithVisibleFileArea(area_offs, area_len, TRUE) == FALSE)
	{
		delete scr_item;
		return;
	}

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddInsertedDecoration(owner, data, zo, area_offs, scr_item, decor_horz_order, baseline_vert_offs) == FALSE)
	{
		delete scr_item;
		m_decor_creation_errors++;
	}
}

void TLangViewSourceFileScreenItem::CreateImposedDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
														long area_offs, TScreenItem *scr_item, long horz_offs, long baseline_vert_offs)
{
	// Check for intersection with the visible area.
	long area_len = 0;
	if (IntersectWithVisibleFileArea(area_offs, area_len, TRUE) == FALSE)
	{
		delete scr_item;
		return;
	}

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddImposedDecoration(owner, data, zo, area_offs, scr_item, horz_offs, baseline_vert_offs) == FALSE)
	{
		delete scr_item;
		m_decor_creation_errors++;
	}
}

void TLangViewSourceFileScreenItem::CreateHorzSpacingDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
																long area_offs, long spc_val_left, long spc_val_right)
{
	if (spc_val_left <= 0 && spc_val_right <= 0)
		return;

	// Check for intersection with the visible area.
	long area_len = 0;
	if (IntersectWithVisibleFileArea(area_offs, area_len, TRUE) == FALSE)
		return;

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddHorzSpacingDecoration(owner, data, zo, area_offs, spc_val_left, spc_val_right) == FALSE)
		m_decor_creation_errors++;
}

void TLangViewSourceFileScreenItem::CreateSpecTabDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
																long area_offs, short decor_horz_order)
{
	// Check for intersection with the visible area.
	long area_len = 0;
	if (IntersectWithVisibleFileArea(area_offs, area_len, TRUE) == FALSE)
		return;

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddSpecTabDecoration(owner, data, zo, area_offs, decor_horz_order) == FALSE)
		m_decor_creation_errors++;
}

void TLangViewSourceFileScreenItem::CreateWholeLineBkgrDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
																long line_beg, long num_lines, const TBasicStyle *style, WORD cover_mask)
{
	// Check for intersection with the visible lines range.
	if (IntersectWithVisibleLinesRange(line_beg, num_lines) == FALSE)
		return;

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddWholeLineBkgrDecoration(owner, data, zo, line_beg, num_lines, (TBasicStyle*)style, cover_mask) == FALSE)
		m_decor_creation_errors++;
}

void TLangViewSourceFileScreenItem::CreateLineNumFrgrDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
																long line_beg, long num_lines, const TTextStyle *style)
{
	// Check for intersection with the visible lines range.
	if (IntersectWithVisibleLinesRange(line_beg, num_lines) == FALSE)
		return;

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddLineNumFrgrDecoration(owner, data, zo, line_beg, num_lines, (TTextStyle*)style) == FALSE)
		m_decor_creation_errors++;
}

void TLangViewSourceFileScreenItem::CreateSpecTabsAreaDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo,
																long line_beg, long num_lines)
{
	// Check for intersection with the visible lines range.
	if (IntersectWithVisibleLinesRange(line_beg, num_lines) == FALSE)
		return;

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddSpecTabsAreaDecoration(owner, data, zo, line_beg, num_lines) == FALSE)
		m_decor_creation_errors++;
}

void TLangViewSourceFileScreenItem::CreateWholeLineImposedDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo, long line_num,
																TScreenItem *scr_item, long horz_offs, long baseline_vert_offs)
{
	// Check for intersection with the visible area.
	long num_lines = 1;
	if (IntersectWithVisibleLinesRange(line_num, num_lines) == FALSE)
	{
		delete scr_item;
		return;
	}

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddWholeLineImposedDecoration(owner, data, zo, line_num, scr_item, horz_offs, baseline_vert_offs) == FALSE)
	{
		delete scr_item;
		m_decor_creation_errors++;
	}
}

void TLangViewSourceFileScreenItem::CreateWholeLineSpacingDecoration(TSourceViewDecorOwner *owner, DWORD data, TSrcViewDecorZorder zo, long line_num,
																bool above_spc, short spc_value, bool accept_wl_bkgr)
{
	// Check for intersection with the visible area.
	long num_lines = 1;
	if (IntersectWithVisibleLinesRange(line_num, num_lines) == FALSE)
		return;

	// Add the low level decor.
	if (TDecoratedFileViewScreenItem::AddWholeLineSpacingDecoration(owner, data, zo, line_num, above_spc, spc_value, accept_wl_bkgr) == FALSE)
		m_decor_creation_errors++;
}

void TLangViewSourceFileScreenItem::CreateTopBottomAreaBkgrDecoration(TSourceViewDecorOwner *owner, bool cover_top_area, bool cover_bottom_area, const TBasicStyle *style, WORD cover_mask)
{
	if (style == NULL)
		return;

	// This type of decoration cannot be invisible.
	if (TDecoratedFileViewScreenItem::AddTopBottomAreaBkgrDecoration(owner, 0, svzo_inactive_area, cover_top_area, cover_bottom_area, (TBasicStyle*)style, cover_mask) == FALSE)
		m_decor_creation_errors++;
}

// -----------------------------------------------------------------------------
//    =================  TLangViewSourceFileDialog  =====================
// -----------------------------------------------------------------------------

static const TControlPosInfo g_SourceFileViewDialog_ControlPositioningInfo[] =
{
	IDC_LVSSV_FNTITLE,			cpm_fix_left_top,
	IDC_LVSSV_FILENAME,			cpm_fix_top_side,
	IDC_LVSSV_ENCLOSURE,		cpm_fix_right_top,
	IDC_LVSSV_FILEVIEW,			cpm_fix_all_corners,
	IDCANCEL,					cpm_fix_right_top,			// This control may not be present in all dialog templates.
	0, cpm_none,
};

static TMenuItemInfo g_SourceFileViewDialog_RightBottomCornerContextMenu[] =
{
	{ 0, 100, L"Show tabs and spaces"		   },
	{ 0, 101, L"Hide tabs and spaces"			   },
	{ 0, 102,	L"Copy to clipboard as Html Page"   },
	{ 0, 103,	L"Copy to clipboard as Bitmap"	   },
	{ 0, 0, NULL },
};

const wchar_t *g_SourceFileViewDialog_CssStylesDir =
{
	L"C:/Ballerup/Calvados/Grammars/CSS"
};

const wchar_t *g_SourceFileViewDialog_CssStyleFiles[] =
{
	L"Wbs_CppSourceSyntax.CSS",
	L"Wbs_RawSyntaxHighlight.CSS",
	NULL
};

TLangViewSourceFileDialog::TLangViewSourceFileDialog(TMidLevStoreReader *db_link, TLangViewChildBrowserType type, short template_id, TColor bkgr_color, TColor bkgr_color_no_file)
			: TLangViewChildBrowser(db_link, type, (template_id != 0) ? template_id : IDD_LANGV_SVF_SOURCE_FILE_VIEW)
{
	m_active_scanner = NULL;
	m_items_view_bkgr_color = bkgr_color;
	m_items_view_bkgr_color_no_file = bkgr_color_no_file;
	m_show_hide_scrollers_horz = m_show_hide_scrollers_vert = FALSE;
	m_send_reset_slct_on_click = m_send_encl_loc_on_dblclk = FALSE;
}

LRESULT TLangViewSourceFileDialog::OnInitDialog(HWND hDefaultFocusControl)
{
	// Prepare resizing.
	RECT dlg_rect, flab_ctrl_rect, encl_ctrl_rect;
	::GetClientRect(m_hDlg, &dlg_rect);
	GetControlRect(IDC_LVSSV_FNTITLE, flab_ctrl_rect);
	GetControlRect(IDC_LVSSV_ENCLOSURE, encl_ctrl_rect);
	SetControlPos(IDC_LVSSV_ENCLOSURE, dlg_rect.right-flab_ctrl_rect.left-(encl_ctrl_rect.right-encl_ctrl_rect.left)-1, encl_ctrl_rect.top);
	SetMinTrackingSize(encl_ctrl_rect.right-encl_ctrl_rect.left+2*(dlg_rect.right-encl_ctrl_rect.right), 0);
	AdjustControlRightSide(IDC_LVSSV_FILEVIEW);
	SetupControlsListPositioning(g_SourceFileViewDialog_ControlPositioningInfo);

	// Assign control styles.
	SetControlFont(IDCANCEL, SMALL_FONTS, 6);
	SetControlFont(IDC_LVSSV_FILENAME, TAHOMA, 8);

	// Prepare the screen items control. In the ccurrent design when the dialog is opened, it does not have any file.
	// This means that when the show/hide props are TRUE, scrollers should not be visible. When value these flags
	// are FASLSE, the scrollers should be present.
	m_view_helper.SetWindow(::GetDlgItem(m_hDlg, IDC_LVSSV_FILEVIEW));
	RefresMainViewerProps();

	// Prepare the file view style with the default props.
	if (m_view_style.CheckStylePrepared() == FALSE)
		m_view_style.SetupDefaultSrcProps();
	if (m_view_style.SetupForDeviceContextEx(m_hDlg) == FALSE)
		return(-1);

	// Assign the stype to the only one item that will be used in the view.
	m_file_item.SetStyle(&m_view_style);

	// As it was mentioned above, the initial state of the dialog is empty.
	SetControlText(IDC_LVSSV_FILENAME, NULL);
	SetControlText(IDC_LVSSV_ENCLOSURE, NULL);

	SetFocusControl(IDC_LVSSV_FILEVIEW);
	return(FALSE);
}

void TLangViewSourceFileDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_LVSSV_FILEVIEW:
				{
					// This is notification from the major ScreenItemsView object.
					TScreenItemsViewNtfCode ntf_code = (TScreenItemsViewNtfCode)notificationCode;
					if (TScreenItemsViewObject::IsUserActivityEvent(ntf_code) == TRUE)
						ResetUserCancel();

					if (TScreenItemsViewObject::IsMouseNotifyEvent(ntf_code) == TRUE)
					{
						// This type of event requires that object should be inited.
						if (m_file_item.IsInited() == FALSE)
							break;

						// This event carries the TScreenItemsViewMouseNotify structure.
						TScreenItemsViewMouseNotify *ev_info = (TScreenItemsViewMouseNotify*)hWndControl;

						POINT app_pt;
						m_view_helper.GetAppSpacePos(ev_info->click_place, app_pt);
						TDecoratedFileWlbkAreaFlags horz_area = m_file_item.GetHorzAreaTypeFromPoint(app_pt);

						switch (ntf_code)
						{
							case xiv_left_btn_down:
							case xiv_left_btn_dblclk:
									{
										// Color picker support.
										m_view_helper.SaveClickedPointColor(ev_info);

										// Check/process special click modes.
										if (ntf_code == xiv_left_btn_down && m_send_reset_slct_on_click == TRUE)
										{
											if (m_owner != NULL)
												m_owner->RemoveSelection(this);
											break;
										}
										else if (ntf_code == xiv_left_btn_dblclk && m_send_encl_loc_on_dblclk == TRUE)
										{
											if (m_owner != NULL && m_file_item.SrcEnterEvent().hdr.event_type == pset_lex_source_enter)
											{
												// Prepare file location that will describe the whole contents of the current enclosure.
												TLangViewClickEvent click_info;
												click_info.type = clet_code_area;
												click_info.code_area.Clear();
												click_info.code_area.parsing_id = m_file_item.SrcEnterEvent().hdr.parsing_id;
												click_info.code_area.source_ref = m_file_item.SrcEnterEvent().hdr.event_id;
												SendForProcessingFromScreenItemsView(ntf_code, click_info);
											}

											break;
										}

										// Click on the line numbers area and nowhere should remove the selection.
										if (horz_area == wlbk_none || horz_area == wlbk_nums || ev_info->object == NULL)
										{
											if (m_owner != NULL)
												m_owner->RemoveSelection(this);
											break;
										}

										// Something was clicked.
										ProcessClickEvent(ntf_code, ev_info);
									}
									break;

							case xiv_right_btn_down:
									{
										// Clicks outside of the screen item should be ignored.
										if (horz_area == wlbk_none)
											break;

										ProcessRightMouseClick(ev_info);
									}
									break;
						}
					}
					else if (ntf_code == xiv_corner_clicked)
					{
						// User has clicked the right-bottom corner.
						TScreenItemsViewNotify *nti = (TScreenItemsViewNotify*)hWndControl;
						if (nti->right_btn == TRUE)
						{
							DWORD res = m_view_helper.ShowAndTrackPopupMenu(g_SourceFileViewDialog_RightBottomCornerContextMenu, nti->pos_x, nti->pos_y);
							if (res >= 100 && res <= 101)
							{
								bool show_tabs_spaces = (res == 101) ? FALSE : TRUE;
								if (m_file_item.IsInited() == TRUE)
									m_file_item.SetupTabsSpacesViewMode(show_tabs_spaces, show_tabs_spaces);
								else m_view_style.SetTabsSpacesVisibMode(show_tabs_spaces, show_tabs_spaces);
							}
							else if (res == 102)
							{
								// Generate HTML page out of the whole file.
								THtmlDocument rprt(L"source_code");
								rprt.PrepareInMemoryStream();
								bool gen_res = m_file_item.GenerateBasicHtmlPage(rprt, m_file_item.FileName(), g_SourceFileViewDialog_CssStylesDir, g_SourceFileViewDialog_CssStyleFiles);
								rprt.Close();

								if (gen_res == FALSE || rprt.GetErrorFlag() == TRUE)
								{
									MsgBox(L"Error generating the HTML representation.");
								}
								else
								{
									// The generated data is fine.
									DWORD err_code = TControlHelper::PutStringToClipboard(rprt.GetBodyPtr());
									if (err_code != ERROR_SUCCESS)
										MsgBoxFmt(MB_OK, L"Error putting HTML data into the clipboard (Win32 error: %d).", err_code);
								}
							}
							else if (res == 103)
							{
								// Generate screen shot without margins.
								PutScreenShotToClipboard(m_view_helper, NULL);
							}
						}
					}
				}
				break;
	}
}

void TLangViewSourceFileDialog::OnWmSize()
{
	if (m_file_item.IsInited() == TRUE)
	{
		// Resetup the min size of file view screen item. Make it identical to the current width of the screen items view.
		RefreshFileItemMinWidth();
	}
}

void TLangViewSourceFileDialog::SetShowHideScrollersMode(bool horz_scroll, bool vert_scroll)
{
	if (horz_scroll != m_show_hide_scrollers_horz || vert_scroll != m_show_hide_scrollers_vert)
	{
		// Take the data in and update the control.
		m_show_hide_scrollers_horz = horz_scroll;
		m_show_hide_scrollers_vert = vert_scroll;
		if (m_hDlg != NULL)
		{
			RefresMainViewerProps();
		}
	}
}

bool TLangViewSourceFileDialog::LoadContents(TFileInfo &unparsed_src_file, bool bypass_file_info_data, long scroll_pos)
{
	assert(m_view_style.CheckStylePrepared() == TRUE);
	if (m_file_item.SetupSourceFile(this, m_view_helper, unparsed_src_file, bypass_file_info_data) == FALSE)
		return(FALSE);
	return(LoadContentsInternal(NULL, scroll_pos));
}

bool TLangViewSourceFileDialog::LoadContents(TMidLevScanner *active_scanner, ID pars_id, ID src_enter_evid, long scroll_pos)
{
	assert(m_view_style.CheckStylePrepared() == TRUE);
	if (m_file_item.SetupSourceFile(this, m_view_helper, m_db, active_scanner, pars_id, src_enter_evid) == FALSE)
		return(FALSE);
	return(LoadContentsInternal(active_scanner, scroll_pos));
}

bool TLangViewSourceFileDialog::LoadContents(TMidLevScanner *active_scanner, CtvLexSourceEnterEvent &call_event, long scroll_pos)
{
	assert(m_view_style.CheckStylePrepared() == TRUE);
	if (m_file_item.SetupSourceFile(this, m_view_helper, m_db, active_scanner, call_event) == FALSE)
		return(FALSE);
	return(LoadContentsInternal(active_scanner, scroll_pos));
}

void TLangViewSourceFileDialog::ResetContents()
{
	// Clear prev contents of the screen items control if any.
	m_view_helper.ClearInternalDataSet();
	m_file_item.ResetSourceFile();
	m_active_scanner = NULL;

	RefresMainViewerProps();
	m_view_helper.RepaintControl();

	SetControlText(IDC_LVSSV_FILENAME,   NULL);
	SetControlText(IDC_LVSSV_ENCLOSURE,  NULL);
}

bool TLangViewSourceFileDialog::LoadContentsInternal(TMidLevScanner *active_scanner, long scroll_pos)
{
	assert(m_hDlg != NULL);

	// Show data on the additional controls.
	wchar_t fid_buff[40], fid_suffix[50];
	if (m_file_item.FileInfo().file_id != 0)
		swprintf(fid_suffix, 50, L"  (%s)", m_file_item.FileInfo().PrepareFileIdShort(fid_buff, 40));
	else fid_suffix[0] = 0;

	wchar_t fname_buff[2*MAX_PATH+80];
	swprintf(fname_buff, 2*MAX_PATH+80, L"%s%s", m_file_item.FileNameForDisplay(), fid_suffix);
	SetControlText(IDC_LVSSV_FILENAME, fname_buff);

	CtvLexSourceEnterEvent &call_event = m_file_item.SrcEnterEvent();
	if (call_event.hdr.parsing_id != 0)
		SetControlTextFmt(IDC_LVSSV_ENCLOSURE, L"%lu.%lu", call_event.hdr.parsing_id, call_event.hdr.event_id);
	else SetControlText(IDC_LVSSV_ENCLOSURE, NULL);

	// Prepare the major control.
	RefreshFileItemMinWidth();
	RefresMainViewerProps();
	m_view_helper.ClearInternalDataSet();
	m_view_helper.AddToInternalDataSet(m_file_item);

	m_view_helper.ShowInternalDataSet(FALSE);
	m_view_helper.SetScrollPos(0, scroll_pos);

	// Success.
	m_active_scanner = active_scanner;
	return(TRUE);
}

void TLangViewSourceFileDialog::RefreshFileItemMinWidth()
{
	TScreenItemsViewSpaceInfo space_info;
	m_view_helper.GetSpaceInfo(space_info);
	m_file_item.SetupMinBoundingRectSize(space_info.client_size.cx, -1);
	m_view_helper.ShowInternalDataSet();
}

void TLangViewSourceFileDialog::RefresMainViewerProps()
{
	bool doc_avail = m_file_item.IsInited();
	bool horz_visib = (m_show_hide_scrollers_horz == TRUE) ? doc_avail : TRUE;
	bool vert_visib = (m_show_hide_scrollers_vert == TRUE) ? doc_avail : TRUE;
	m_view_helper.SetStndDocStyleProps(horz_visib, vert_visib, ((doc_avail == TRUE) ? m_items_view_bkgr_color : m_items_view_bkgr_color_no_file), TRUE);
}

void TLangViewSourceFileDialog::ProcessClickEvent(TScreenItemsViewNtfCode ntf_code, TScreenItemsViewMouseNotify *ev_info)
{
	// This method expects that some screen item was clicked.
	assert(ev_info->object != NULL);
	TSourceViewDecorOwner *decor_owner = (TSourceViewDecorOwner*)(ev_info->object->GetOwner());
	assert(decor_owner != NULL);

	if (m_owner == NULL)
	{
		// There is no destination to send the click to. It makes sense to make this check here and not in
		// the WM_COMMNAD handler. This allows displaying the context menus while keeping the code
		// reasonably simple.
		return;
	}

	// Check for areas that should not be sent to the browser frame for processing.
	if (decor_owner->m_type == svdo_out_of_scope_code)
	{
		m_owner->RemoveSelection(this);
		MsgBox(L"The clicked code area is out of the parsing scope.\r\n");
		return;
	}
	else if (decor_owner->m_type == svdo_aborted_scan_code)
	{
		m_owner->RemoveSelection(this);
		MsgBox(L"Clicked code was not parsed because the parsing was aborted.\r\n");
		return;
	}
	else if (decor_owner->m_type == svdo_unscanned_code)
	{
		m_owner->RemoveSelection(this);
		MsgBox(L"Clicked code was not parsed because the parsing has not reached this place yet.\r\n");
		return;
	}
	else if (decor_owner->m_type == svdo_curr_scan_pos)
	{
		m_owner->RemoveSelection(this);
		MsgBox(L"This is the current scanning position.\r\n");
		return;
	}

	// User has clicked something that has description. Prepare details about this object.
	TLangViewClickEvent click_info;
	wchar_t *err_message = NULL;
	if (m_file_item.PrepareClickEventInfo(m_db, m_active_scanner, click_info, ev_info, &err_message) == FALSE)
	{
		assert(err_message != NULL);
		MsgBoxFmt(MB_OK, L"Error preparing MouseClickEvent info: %s.", err_message);
		return;
	}

	if (click_info.type != clet_none)
	{
		// Send event for processing to the parent.
		ResetSelection();
		SendForProcessingFromScreenItemsView(ntf_code, click_info);
	}
}

void TLangViewSourceFileDialog::ProcessRightMouseClick(TScreenItemsViewMouseNotify *ev_info)
{
	// This is a temp object, it is needed for returning the invisible conditional parsing status or message that
	// the code structure decorations are broken.
	TRectItem worker_scr_item;
	TSourceViewDecorOwner worker_scr_item_owner(svdo_code_area);
	worker_scr_item.SetOwner(&worker_scr_item_owner);

	// Get summary of the screen items under the clicked point. Note that if current file belongs to some parsing,
	// then result should always contain at least the code status area item even if the code status is not visible.
	#define lvsfd_max_ctx_menu_items 24
	TScreenItem *scr_items[lvsfd_max_ctx_menu_items];
	int num_objects = lvsfd_max_ctx_menu_items;
	int cnt = m_file_item.GetDecorsListFromPoint(ev_info->click_place.x, ev_info->click_place.y, scr_items, num_objects, &worker_scr_item);
	if (cnt <= 0)
	{
		// There are no decorations under the clicked point.
		return;
	}

	// Prepare context menu that describes these clicked objects.
	assert(num_objects > 0 && num_objects <= lvsfd_max_ctx_menu_items);
	TMenuItemInfo info[lvsfd_max_ctx_menu_items+2];
	wchar_t menu_text[(lvsfd_max_ctx_menu_items+2)*121], *pBuff = menu_text;
	for (int inx=0; inx<num_objects; ++inx)
	{
		TSourceViewDecorOwner *owner = (TSourceViewDecorOwner*)scr_items[inx]->GetOwner();
		m_file_item.GetCtxMenuObjectDescription(owner, scr_items[inx]->GetOwnerData(), pBuff, 120);
		info[inx].Setup(inx+100, pBuff);
		pBuff += wcslen(pBuff)+1;
	}

	// Check for the menu overflow case.
	if (cnt > num_objects)
	{
		// There are some items that did not fit into the buffer.
		swprintf(pBuff, 120, L"......... Menu overflow. %d decoration%s in total.", cnt, PluralSuffix(cnt));
		info[num_objects++].Setup(200, pBuff);
	}

	// Add the termination record.
	info[num_objects].Setup(0, NULL);

	// Display and track this context menu.
	DWORD res = m_view_helper.ShowAndTrackPopupMenu(info, ev_info->click_place.x, ev_info->click_place.y);
	if (res >= 100 && res < 100+(DWORD)num_objects)
	{
		// Replace the screen item in the event description and send the click event for processing.
		ev_info->object = scr_items[res-100];
		ProcessClickEvent(xiv_left_btn_down, ev_info);
	}
}


