//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Common/FormatNumber.H"
#include  "LangView/LexItems.H"
#include  "LangView/LexItemsXml.H"
#include  "LangView/LangViewResources.H"

#define  WANT_IDENTIFIER_NAME		// Type of the main label and its position for ltx_name lexemas.

// ---------------------------------------------------------------------
//   =================  LexItem style props  ====================
// ---------------------------------------------------------------------

static const wchar_t *g_LexItemBasicBuiltInStyleNames[gti_bkgr_num_styles] =
{
	L"gti_bkgr_lexema",
	L"gti_bkgr_lexema_chlds",
	L"gti_bkgr_mid_lev",
	L"gti_bkgr_mid_lev_chlds",
	L"gti_bkgr_non_terminal",
	L"gti_bkgr_non_term_chlds",
	L"gti_bkgr_transp_non_term_chlds",
	L"gti_bkgr_opaq_non_term_chlds",

	L"gti_bkgr_mlev_aux_call",
	L"gti_bkgr_mlev_aux_call_chlds",
	L"gti_bkgr_mlev_file_call",
	L"gti_bkgr_mlev_file_call_chlds",
	L"gti_bkgr_mlev_mj_src_call",
	L"gti_bkgr_mlev_mj_src_call_chlds",
	L"gti_bkgr_mlev_error_stmt",
	L"gti_bkgr_mlev_passive_stmt",
	L"gti_bkgr_mlev_error_event",

	L"gti_bkgr_spec_lex_filler",
	L"gti_bkgr_ambig_conflicts",
	L"gti_bkgr_grammar_inst",
	L"gti_bkgr_rule_label",
	L"gti_bkgr_rule_pos",
	L"gti_bkgr_xpct_inst",
	L"gti_bkgr_conflict_inst",
	L"gti_bkgr_cfcting_action",
	L"gti_bkgr_derivation_path",
	L"gti_bkgr_center_button",
	L"gti_bkgr_missing_link",
	L"gti_bkgr_code_location",

	L"gti_bkgr_program",
	L"gti_bkgr_program_chlds",
	L"gti_bkgr_parser_obj",
	L"gti_bkgr_parser_obj_chlds",
	L"gti_bkgr_null_item",
	L"gti_bkgr_null_item_chlds",

	L"gti_bkgr_cpp_misc",
	L"gti_bkgr_cpp_defns",
	L"gti_bkgr_cpp_unresolved",
	L"gti_bkgr_cpp_stmts",
	L"gti_bkgr_cpp_labels",
	L"gti_bkgr_cpp_exprs",
	L"gti_bkgr_cpp_oprnds",
	L"gti_bkgr_cpp_temarg_chlds",
	L"gti_bkgr_cpp_dim_obj",

	L"gti_bkgr_slct_light_roze",
	L"gti_bkgr_slct_bold_blue",
	L"gti_bkgr_slct_foreign_nterm",
};

static const wchar_t *g_LexItemTextBuiltInStyleNames[gti_text_num_styles] =
{
	L"gti_text_lexema",
	L"gti_text_token_operator",
	L"gti_text_token_keyword",
	L"gti_text_lexema_symbol",
	L"gti_text_non_terminal",
	L"gti_text_non_terminal_symbol",
	L"gti_text_mlev_small_mlab"

	L"gti_text_act_sym_normal",
	L"gti_text_act_sym_operator",
	L"gti_text_act_sym_eof",
	L"gti_text_cinp_sym_normal",
	L"gti_text_cinp_sym_operator",
	L"gti_text_cinp_sym_eof",
	L"gti_text_non_term_label",

	L"gti_text_message",
	L"gti_text_details",
	L"gti_text_small_title",
	L"gti_text_grammar_object",
	L"gti_text_rule_mlab",
	L"gti_text_rule_symbol",

	L"gti_text_program",
	L"gti_text_parser_obj_index",
	L"gti_text_null_item",

	L"gti_text_cpp_misc",
	L"gti_text_cpp_defn",
	L"gti_text_cpp_dim_obj",
	L"gti_text_cpp_item_symbol",
};

static TIconsGridStyleProps g_LexItemIconsGridProps =
{
	NULL,								// No specific CSS style.
	IDBMP_LEX_ITEMS, TRUE, TRUE, 75,
	20, 13,								// Left-top position of the cell 100.
	22, 22, 19, 19,						// Step between cells and size of the cell.
};

#define  DARK_BORDER  RGB(40, 40, 40), 1

static TBasicStyleProps g_LexItemBkgrStyleProps[gti_bkgr_num_styles] =
{
	// gti_bkgr_lexema
	{
		NULL,
		RGB(254, 254, 81),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(52, 66, 6), 1,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_lexema_chlds
	{
		NULL,
		RGB(173, 111, 5),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(52, 66, 6), 1,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mid_lev
	{
		NULL,
		RGB(239, 216, 171),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(139, 67, 1), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mid_lev_chlds
	{
		NULL,
		RGB(208, 152, 125),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(139, 67, 1), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_non_terminal
	{
		NULL,
		RGB(167, 228, 174),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(52, 66, 6), 1,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_non_term_chlds
	{
		NULL,
		RGB(80, 106, 0),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(52, 66, 6), 1,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_transp_non_term_chlds
	{
		NULL,
		RGB(226, 222, 175),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(52, 66, 6), 1,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_opaq_non_term_chlds
	{
		NULL,
		RGB(99, 167, 207),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(52, 66, 6), 1,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_aux_call
	{
		NULL,
		RGB(204, 227, 168),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(27, 67, 7), 1,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_aux_call_chlds
	{
		NULL,
		RGB(118, 168, 62),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(27, 67, 7), 1,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_file_call
	{
		NULL,
		RGB(163, 211, 241),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(14, 49, 69), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_file_call_chlds
	{
		NULL,
		RGB(71, 154, 199),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(14, 49, 69), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_mj_src_call
	{
		NULL,
		RGB(244, 200, 140),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(107, 96, 88), 2,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_mj_src_call_chlds
	{
		NULL,
		RGB(192, 131, 3),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(107, 96, 88), 2,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_error_stmt
	{
		NULL,
		RGB(255, 12, 12),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(90, 60, 60), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_passive_stmt
	{
		NULL,
		RGB(196, 196, 196),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(60, 60, 60), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_mlev_error_event
	{
		NULL,
		RGB(254, 188, 188),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(90, 30, 60), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_spec_lex_filler
	{
		NULL,
		RGB(228, 228, 228),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(143, 194, 180), 4,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_ambig_conflicts
	{
		NULL,
		RGB(236, 189, 197),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(143, 194, 180), 4,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_grammar_inst
	{
		NULL,
		RGB(228, 228, 228),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(103, 107, 137), 3,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_rule_label
	{
		NULL,
		RGB(225, 225, 240),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(106, 106, 181), 2,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_rule_pos
	{
		NULL,
		RGB(228, 228, 228),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(162, 165, 191), 4,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_xpct_inst
	{
		NULL,
		RGB(215, 220, 209),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(173, 175, 182), 4,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_conflict_inst
	{
		NULL,
		RGB(254, 233, 205),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(221, 156, 85), 4,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cfcting_action
	{
		NULL,
		RGB(255, 247, 240),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(214, 133, 114), 4,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_derivation_path
	{
		NULL,
		RGB(213, 240, 249),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(148, 219, 239), 4,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_center_button
	{
		NULL,
		RGB(239, 216, 171),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(139, 67, 1), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_missing_link
	{
		NULL,
		RGB(255, 64, 64),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(96, 40, 40), 3,				// Frame (color, width).
		-1, PS_SOLID,					// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_code_location
	{
		NULL,
		RGB(39, 216, 171),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(139, 67, 201), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_program  (root of the fully parsed program).
	{
		NULL,
		RGB(225, 140, 225),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(129, 44, 131), 2,				// Frame (color, width).
		-1, PS_SOLID,					// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_program_chlds
	{
		NULL,
		RGB(163, 84, 167),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(129, 44, 131), 2,				// Frame (color, width).
		-1, PS_SOLID,					// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_parser_obj (this is parser stack slot or inp queue item).
	{
		NULL,
		RGB(104, 134, 215),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(0, 0, 160), 2,					// Frame (color, width).
		-1, PS_SOLID,					// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_parser_obj_chlds (this is parser stack slot or inp queue item).
	{
		NULL,
		RGB(41, 69, 152),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(0, 0, 160), 2,					// Frame (color, width).
		-1, PS_SOLID,					// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_null_item
	{
		NULL,
		RGB(255, 32, 32),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		DARK_BORDER,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_null_item_chlds
	{
		NULL,
		RGB(176, 0, 0),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		DARK_BORDER,					// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_misc
	{
		NULL,
		RGB(239, 216, 171),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(139, 67, 1), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_defns
	{
		NULL,
		RGB(133, 224, 169),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(22, 80, 46), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_unresolved
	{
		NULL,
		RGB(192, 128, 122),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(62, 30, 30), 3,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_stmts
	{
		NULL,
		RGB(248, 129, 129),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(140, 53, 53), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_labels
	{
		NULL,
		RGB(164, 206, 240),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(23, 89, 142), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_exprs
	{
		NULL,
		RGB(250, 224, 122),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(142, 115, 6), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_oprnds
	{
		NULL,
		RGB(251, 186, 128),					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(152, 72, 5), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_temarg_chlds
	{
		NULL,
		RGB(166, 122, 115),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(152, 72, 5), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_cpp_dim_obj
	{
		NULL,
		RGB(206, 206, 206),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(96, 96, 96), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_slct_light_roze
	{
		NULL,
		RGB(255, 232, 232),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(140, 140, 140), 1,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_slct_bold_blue
	{
		NULL,
		RGB(181, 220, 249),				// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(52, 101, 190), 2,				// Frame (color, width).
		0, PS_SOLID,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},

	// gti_bkgr_slct_foreign_nterm
	{
		NULL,
		cs_transparent,					// Background (color).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Shadow (up, down).
		RGB(28, 80, 78), 2,				// Frame (color, width).
		0, PS_DOT,						// Frame (offset, pen style).
		RGB(0, 0, 0), 0, RGB(0, 0, 0), 0,		// Frame_shadow (up, down).
	},
};

#define  TKNOPR_FONT  L"Copper Black"

static TTextStyleProps g_LexItemTextStyleProps[gti_text_num_styles] =
{
	// Lexemas and non terminals group.
	{  NULL,	 ARIAL,				8,	FALSE,	RGB(0, 0, 0),			STD_TXT  },		// gti_text_lexema
	{  NULL,	 TKNOPR_FONT,		8,	TRUE,	RGB(0, 0, 0),			STD_TXT  },		// gti_text_token_operator
	{  NULL,	 COURIER_NEW,		8,	TRUE,	RGB(0, 0, 0),			STD_TXT  },		// gti_text_token_keyword
	{  NULL,	 SMALL_FONTS,		7,	FALSE,	RGB(64, 64, 64),		STD_TXT  },		// gti_text_lexema_symbol
	{  NULL,	 TAHOMA,			8,	FALSE,	RGB(0, 0, 0),			STD_TXT  },		// gti_text_non_terminal
	{  NULL,	 SMALL_FONTS,		7,	FALSE,	RGB(64, 64, 64),		STD_TXT  },		// gti_text_non_terminal_symbol
	{  NULL,	 SMALL_FONTS,		6,	FALSE,	RGB(32, 32, 32),		STD_TXT  },		// gti_text_mlev_small_mlab

	// gti_text_act_sym_normal
	{
		NULL,
		ARIAL, 7, FALSE,						// Font params.
		RGB(80, 80, 80), RGB(255, 255, 144),		// Foreground and bkgr.
		RGB(128, 128, 108), fcs_1pix_excl_tip,
		cs_transparent, 0,						// No shadow.
		1, 0, 1,								// Vertical adjustments.
		5, 4									// Horz extensions.
	},

	// gti_text_act_sym_operator
	{
		NULL,
		TKNOPR_FONT, 8, TRUE,				// Font params.
		RGB(80, 80, 80), RGB(255, 255, 144),		// Foreground and bkgr.
		RGB(128, 128, 108), fcs_1pix_excl_tip,
		cs_transparent, 0,						// No shadow.
		1, 0, 1,								// Vertical adjustments.
		5, 4									// Horz extensions.
	},

	// gti_text_act_sym_eof
	{
		NULL,
		L"Microsoft Sans Serif", 7, FALSE,			// Font params.
		RGB(0, 0, 0), RGB(255, 255, 144),		// Foreground and bkgr.
		RGB(128, 128, 108), fcs_1pix_excl_tip,
		cs_transparent, 0,						// No shadow.
		2, 0, 2,								// Vertical adjustments.
		5, 4									// Horz extensions.
	},

	// gti_text_cinp_sym_normal
	{
		NULL,
		ARIAL, 7, FALSE,						// Font params.
		RGB(155, 155, 136), RGB(255, 255, 196),	// Foreground and bkgr.
		RGB(188, 188, 175), fcs_1pix_excl_tip,
		cs_transparent, 0,						// No shadow.
		1, 0, 1,								// Vertical adjustments.
		5, 4									// Horz extensions.
	},

	// gti_text_cinp_sym_operator
	{
		NULL,
		TKNOPR_FONT, 8, TRUE,				// Font params.
		RGB(155, 155, 136), RGB(255, 255, 196),	// Foreground and bkgr.
		RGB(188, 188, 175), fcs_1pix_excl_tip,
		cs_transparent, 0,						// No shadow.
		1, 0, 1,								// Vertical adjustments.
		5, 4									// Horz extensions.
	},

	// gti_text_cinp_sym_eof
	{
		NULL,
		L"Microsoft Sans Serif", 7, FALSE,			// Font params.
		RGB(80, 80, 80), RGB(255, 255, 196),		// Foreground and bkgr.
		RGB(188, 188, 175), fcs_1pix_excl_tip,
		cs_transparent, 0,						// No shadow.
		2, 0, 2,								// Vertical adjustments.
		5, 4									// Horz extensions.
	},

	// gti_text_non_term_label
	{
		NULL,
		TAHOMA, 8, FALSE,						// Font params.
		RGB(0, 0, 0), RGB(191, 223, 189),		// Foreground and bkgr.
		RGB(80, 80, 80), fcs_1pix_simple,
		cs_transparent, 0,						// No shadow.
		1, 0, 2,								// Vertical adjustments.
		7, 6									// Horz extensions.
	},

	// Special lex items group.
	{  NULL,	 ARIAL,				8,	FALSE,	RGB(0, 0, 0),			STD_BKGR,	  0, 0, 0,    0, 0  },		// gti_text_message
	{  NULL,	 SMALL_FONTS,		7,	FALSE,	RGB(0, 0, 0),			STD_TXT },						// gti_text_details
	{  NULL,	 ARIAL,				7,	FALSE,	RGB(90, 90, 90),		STD_TXT },						// gti_text_small_title
	{  NULL,	 VERDANA,			9,	TRUE,	RGB(0, 0, 0),			STD_TXT },						// gti_text_grammar_object
	{  NULL,	 SMALL_FONTS,		4,	FALSE,	RGB(120, 120, 120),	STD_BKGR,	  1, 0, 0,    0, 0  },		// gti_text_rule_mlab
	{  NULL,	 SMALL_FONTS,		6,	FALSE,	RGB(64, 64, 64),		STD_TXT },						// gti_text_rule_symbol

	// Visual parser related items.
	{  NULL,	 VERDANA,			11,	TRUE,	RGB(255, 255, 255),	cs_transparent, cs_transparent, fcs_none,
											RGB(128, 128, 128),	1,			  0, 0, 0,    1, 0  },		// gti_text_program
	{  NULL,	 VERDANA,			13,	TRUE,	RGB(248, 248, 248),	STD_BKGR,	  1, 0, 3,    0, 0  },		// gti_text_parser_obj_index
	{  NULL,	 TAHOMA,			8,	FALSE,	RGB(0, 0, 0),			STD_BKGR,	  0, 0, 0,    2, 0  },		// gti_text_null_item

	// Cpp database related items.
	{  NULL,	 ARIAL,				8,	FALSE,	RGB(0, 0, 0),			STD_BKGR,	  0, 0, 0,    0, 0  },		// gti_text_cpp_misc
	{  NULL,	 VERDANA,			9,	TRUE,	RGB(0, 0, 0),			STD_TXT },						// gti_text_cpp_defn
	{  NULL,	 ARIAL,				8,	FALSE,	RGB(80, 80, 80),		STD_BKGR,	  0, 0, 0,    0, 0  },		// gti_text_cpp_dim_obj
	{  NULL,	 SMALL_FONTS,		7,	FALSE,	RGB(64, 64, 64),		STD_TXT },						// gti_text_cpp_item_symbol
};

static const TTextStyleSymbolAdjustInfo g_LexItem_TokenOperator_CharacterAdjustInfo[] =
{
	{ L'!', 0, 0, 1 },	{ L'-', -1, 0, 0 },	{ L'*', 2, 0, 1 },	{ L'/', 0, 0, 1 },	{ L'^', 1, 0, 0 },	{ 0, 0, 0, 0 },
};

static const TTextStyleSymbolAdjustInfo g_LexItem_ActSymOperator_CharacterAdjustInfo[] =
{
	{ L'(', -1, 1, 2 },	{ L')', -1, 1, 2 },	{ L'[', -1, 1, 2 },	{ L']', -1, 1, 2 },	{ L'{', -1, 0, 0 },	{ L'}', -1, 0, 0 },
	{ L'-', -1, 0, 0 },	{ L'*', 2, 1, 2 },	{ L':', -1, 0, 0 },	{ L';', -1, 1, 2 },	{ 0, 0, 0, 0 },
};

static const TTextStyleSymbolAdjustInfo g_LexItem_GrammarObject_CharacterAdjustInfo[] =
{
	{ L' ', 0, 0, -2 },	{ 0, 0, 0, 0 },
};

static const TTextStyleSymbolAdjustInfo g_LexItem_ParserObjIndex_CharacterAdjustInfo[] =
{
	{ L'[', -1, 0, 0 },	{ L']', -1, 0, 0 },	{ 0, 0, 0, 0 },
};

static const TTextStyleSymbolAdjustInfo g_LexItem_SmallTitle_CharacterAdjustInfo[] =
{
	{ L'a', 0, 0, 1 },	{ L':', 0, -1, 0 },	{ 0, 0, 0, 0 },
};

static TLineStyleProps g_LexItemLineStyleProps[gti_line_num_styles] =
{
	// gti_line_upper_link_default
	{
		RGB(48, 48, 48), 0,				// Color and width.
		lcs_sharp, lrs_straight,				// Corner and rounding.
		{ les_none, 0, 0, 0, 0, TRUE },		// Line beg style.
		{ les_none, 0, 0, 0, 0, TRUE },		// Line end style.
	},

	// gti_line_upper_link_selected
	{
		RGB(48, 48, 48), 3,				// Color and width.
		lcs_sharp, lrs_straight,				// Corner and rounding.
		{ les_none, 0, 0, 0, 0, TRUE },		// Line beg style.
		{ les_none, 0, 0, 0, 0, TRUE },		// Line end style.
	},

	// gti_line_right_link_default
	{
		RGB(222, 217, 225), 0,				// Color and width.
		lcs_sharp, lrs_straight,				// Corner and rounding.
		{ les_none, 0, 0, 0, 0, TRUE },		// Line beg style.
		{ les_none, 0, 0, 0, 0, TRUE },		// Line end style.
	},

	// gti_line_right_link_selected
	{
		RGB(48, 48, 48), 3,				// Color and width.
		lcs_sharp, lrs_straight,				// Corner and rounding.
		{ les_none, 0, 0, 0, 0, TRUE },		// Line beg style.
		{ les_none, 0, 0, 0, 0, TRUE },		// Line end style.
	},
};

// --------------------------------------------------------------------
//   ===============  TLexItemStyleInfoProps  ===================
// --------------------------------------------------------------------

//
// ----  Lexema items (non keywords)  ----------------------------
//
static TLexItemStyleInfoProps g_LexItemLexemaTypesStyleSetup[] =
{
	// Default lexema style.
	{
		lex_props_any_index,						// Id.
		0, 0,										// Concentric and Rect lines app icons.
		gti_bkgr_lexema, gti_bkgr_lexema_chlds,		// Bkgr styles.
		gti_text_lexema, gti_text_lexema_symbol,		// Text styles.
		33,										// Circle diameter.
		1, 3,										// Main label offsets.
		0, 3,										// Symbol label offsets.
	},

	// EOF.
	{
		ltx_eof,									// Id.
		0, 0,										// Concentric and Rect lines app icons.
		gti_bkgr_lexema, gti_bkgr_lexema_chlds,		// Bkgr styles.
		gti_text_lexema, gti_text_lexema_symbol,		// Text styles.
		33,										// Circle diameter.
		1, 4,										// Main label offsets.
		0, 3,										// Symbol label offsets.
	},

	// Error.
	{
		ltx_error,									// Id.
		0, 0,										// Concentric and Rect lines app icons.
		gti_bkgr_lexema, gti_bkgr_lexema_chlds,		// Bkgr styles.
		gti_text_lexema, gti_text_lexema_symbol,		// Text styles.
		33,										// Circle diameter.
		1, 4,										// Main label offsets.
		0, 3,										// Symbol label offsets.
	},

	{  lex_props_end_mark  }
};

static TLexItemMainLabelProps g_LexItemLexemaTypesMainLabelSetup[] =
{
	// All valid lexema types should have preferred main labels.
	{  ltx_empty,			L"empty"		},
	{  ltx_comment,		L"//.."		},
	{  ltx_number,		L"num"		},
	{  ltx_floating_point,	L"fp-val"		},
	{  ltx_charconst,		L"\'ch\'"		},
	{  ltx_string,			L"\"str\""		},
	{  ltx_keyword,		L"keyw"		},

#ifdef WANT_IDENTIFIER_NAME
	{  ltx_name,			L"identifier"	},
#else
	{  ltx_name,			L"name"		},
#endif

	{  ltx_eol,			L"eol"		},
	{  ltx_eof,			L"EOF"		},
	{  ltx_error,			L"Error"		},

	{  lex_props_end_mark  }
};

//
// ----  Keyword lexema items  ----------------------------
//

#define LEX_TOKEN_OPERATOR_PROP_INFO(sh_x, sh_y)				\
																\
	0, 0,							/* App icon ids.				*/	\
	gti_bkgr_lexema,				/* Main bkgr style.				*/	\
	gti_bkgr_lexema_chlds,			/* Rect lines mode chld area.	*/	\
	gti_text_token_operator,		/* Main label text style.			*/	\
	gti_text_lexema_symbol,		/* Symbol label text style.		*/	\
	27,							/* Circle diameter.				*/	\
	sh_x, sh_y+2,				/* Main label offsets.			*/	\
	1, 1,							/* Symbol label offsets.			*/	\

#define LEX_TOKEN_KEYWORD_PROP_INFO(sh_x, sh_y)				\
																\
	0, 0,							/* App icon ids.				*/	\
	gti_bkgr_lexema,				/* Main bkgr style.				*/	\
	gti_bkgr_lexema_chlds,			/* Rect lines mode chld area.	*/	\
	gti_text_token_keyword,		/* Main label text style.			*/	\
	gti_text_lexema_symbol,		/* Symbol label text style.		*/	\
	34,							/* Circle diameter.				*/	\
	sh_x, sh_y+4,				/* Main label offsets.			*/	\
	0, 3,							/* Symbol label offsets.			*/	\

static TLexItemStyleInfoProps g_LexItemKeywordTypesStyleSetup[] =
{
	// Default style for keyword lexemas.
	{
		lex_props_any_index,					// Id.
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_lexema, gti_bkgr_lexema_chlds,	// Bkgr styles.
		gti_text_token_keyword,				// Text styles.
		gti_text_lexema_symbol,
		34,									// Circle diameter.
		0, 4,									// Main label offsets.
		0, 3,									// Symbol label offsets.
	},

	//
	// Separators and arithmetic opeartions with smaller size and special adjustments.
	//

	{  spr_lcurvbr,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  spr_rcurvbr,		LEX_TOKEN_OPERATOR_PROP_INFO(1, 0)	},
	{  spr_semicol,		LEX_TOKEN_OPERATOR_PROP_INFO(1, -1)	},
	{  spr_ellipsis,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 0)	},

	{  opr_comma,		LEX_TOKEN_OPERATOR_PROP_INFO(1, -1)	},
	{  opr_lpar,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 0)	},
	{  opr_rpar,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_bang,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_tilda,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_unplus,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_unminus,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_deref_ptr,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_get_addr,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_arr_index,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_funcall,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},

	{  opr_mul,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_div,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_rmnd,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_plus,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_minus,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_bitlsh,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_bitrsh,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_bitand,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_bitor,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 0)	},
	{  opr_bitxor,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_lt,				LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_le,				LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_gt,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_ge,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_eq,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_ne,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_logand,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 2)	},
	{  opr_logor,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 0)	},
	{  opr_question,		LEX_TOKEN_OPERATOR_PROP_INFO(1, 0)	},
	{  opr_colon,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 0)	},

	{  opr_dblcolon,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_dot,			LEX_TOKEN_OPERATOR_PROP_INFO(1, 0)	},
	{  opr_arrow,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_dotstar,		LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_arrowstar,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 2)	},
	{  opr_lbracket,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_rbracket,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 0)	},
	{  opr_plusplus,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_minusminus,	LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_plpl_postfix,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_mnmn_postfix,	LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},

	{  opr_assign,			LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_mul_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_div_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_rmnd_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_plus_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_minus_asgn,	LEX_TOKEN_OPERATOR_PROP_INFO(1, 1)	},
	{  opr_lsh_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(1, 2)	},
	{  opr_rsh_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 2)	},
	{  opr_btand_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(2, 1)	},
	{  opr_btor_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(0, 1)	},
	{  opr_btxor_asgn,		LEX_TOKEN_OPERATOR_PROP_INFO(1, 2)	},

	{  clg_int,			LEX_TOKEN_KEYWORD_PROP_INFO(1, 0)	},

	{  lex_props_end_mark  }
};

static TLexItemMainLabelProps g_LexItemKeywordTypesMainLabelsSetup[] =
{
	// TSeparToken
	{  spr_lcurvbr,		L" { "			},		// Spaces around are needed because of the problems with painting in Windows.
	{  spr_rcurvbr,		L" } "			},
	{  spr_semicol,		L";"				},
	{  spr_ellipsis,			L"..."			},

	// TOperatorToken
	{  opr_comma,		L","				},
	{  opr_lpar,			L"("				},   {  opr_rpar,			L")"				},
	{  opr_bang,			L"!"				},   {  opr_tilda,			L"~"				},
	{  opr_defined,		L"DEF"			},
	{  opr_unplus,			L"u+"			},   {  opr_unminus,		L"u-"			},
	{  opr_deref_ptr,		L"*"				},   {  opr_get_addr,		L"&"				},
	{  opr_arr_index,		L"[ ]"			},   {  opr_funcall,			L"call"			},

	{  opr_mul,			L"*"				},   {  opr_div,			L"/"				},
	{  opr_rmnd,			L"%"			},
	{  opr_plus,			L"+"				},   {  opr_minus,			L"-"				},
	{  opr_bitlsh,			L"<<"			},   {  opr_bitrsh,			L">>"			},
	{  opr_bitand,			L"&"				},   {  opr_bitor,			L"|"				},
	{  opr_bitxor,			L"^"				},
	{  opr_lt,				L"<"				},   {  opr_le,				L"<="			},
	{  opr_gt,			L">"				},   {  opr_ge,				L">="			},
	{  opr_eq,			L"=="			},   {  opr_ne,				L"!="			},
	{  opr_logand,			L"&&"			},   {  opr_logor,			L"||"			},
	{  opr_question,		L"?"				},   {  opr_colon,			L":"				},

	{  opr_dblcolon,		L"::"				},
	{  opr_dot,			L"."				},   {  opr_arrow,			L"->"			},
	{  opr_dotstar,		L".*"				},   {  opr_arrowstar,		L"->*"			},
	{  opr_lbracket,		L"["				},   {  opr_rbracket,		L"]"				},
	{  opr_plusplus,		L"++"			},   {  opr_minusminus,		L"--"			},
	{  opr_plpl_postfix,		L"++"			},   {  opr_mnmn_postfix,	L"--"			},

	{  opr_sizeof,			L"sizeof"			},   {  opr_new,			L"new"			},
	{  opr_delete,			L"delete"			},   {  opr_new_array,		L"new[]"			},
	{  opr_delete_array,	L"delete[]"		},   {  opr_throw,			L"throw"			},
	{  opr_typeid,			L"typeid"			},   {  opr_cast,			L"(cast)"			},
	{  opr_const_cast,		L"const_cast"		},   {  opr_dyna_cast,		L"dyna_cast"		},
	{  opr_rein_cast,		L"rein_cast"		},   {  opr_static_cast,		L"static_cast"		},

	{  opr_assign,			L"="				},   {  opr_mul_asgn,		L"*="			},
	{  opr_div_asgn,		L"/="			},   {  opr_rmnd_asgn,		L"%="			},
	{  opr_plus_asgn,		L"+="			},   {  opr_minus_asgn,		L"-="			},
	{  opr_lsh_asgn,		L"<<="			},   {  opr_rsh_asgn,		L">>="			},
	{  opr_btand_asgn,		L"&="			},   {  opr_btor_asgn,		L"|="			},
	{  opr_btxor_asgn,		L"^="			},

	// TPreprocToken
	{  mpr_prm_chr,		L"#chr"			},   {  mpr_prm_str,		L"#str"			},
	{  mpr_prm_cnct,		L"##"			},   {  mpr_vargs,			L"#..."			},

	{  mpr_define,		L"#define"		},   {  mpr_undef,			L"#undef"		},
	{  mpr_include,		L"#include"		},   {  mpr_ifdef,			L"#ifdef"			},
	{  mpr_ifndef,			L"#ifndef"		},   {  mpr_if,				L"#if"			},
	{  mpr_elif,			L"#elif"			},   {  mpr_else,			L"#else"			},
	{  mpr_endif,			L"#endif"			},   {  mpr_pragma,		L"#pragma"		},
	{  mpr_error,			L"#error"			},   {  mpr_import,			L"#import"		},
	{  mpr_line,			L"#line"			},   {  mpr_using,			L"#using"			},

	{  mpr_warning,		L"#warning"		},   {  mpr_region,			L"#region"		},
	{  mpr_endregion,		L"#endregion"		},

	// TCLangToken
	{  clg_asm,			L"asm"			},  {  clg_auto,			L"auto"			},
	{  clg_break,			L"break"			},  {  clg_case,			L"case"			},
	{  clg_catch,			L"catch"			},  {  clg_char,			L"char"			},
	{  clg_const,			L"const"			},  {  clg_continue,			L"continue"		},
	{  clg_default,			L"default"			},  {  clg_do,				L"do"			},
	{  clg_double,			L"double"			},  {  clg_else,			L"else"			},
	{  clg_enum,			L"enum"			},  {  clg_extern,			L"extern"			},
	{  clg_float,			L"float"			},  {  clg_for,				L"for"			},
	{  clg_goto,			L"goto"			},  {  clg_if,				L"if"				},
	{  clg_int,			L"int"			},  {  clg_long,			L"long"			},
	{  clg_register,		L"register"		},  {  clg_return,			L"return"			},
	{  clg_short,			L"short"			},  {  clg_signed,			L"signed"			},
	{  clg_static,			L"static"			},  {  clg_struct,			L"struct"			},
	{  clg_switch,			L"switch"			},  {  clg_try,				L"try"			},
	{  clg_typedef,		L"typedef"		},  {  clg_union,			L"union"			},
	{  clg_unsigned,		L"unsigned"		},  {  clg_void,			L"void"			},
	{  clg_volatile,			L"volatile"		},  {  clg_wchar_t,			L"wchar_t"		},
	{  clg_while,			L"while"			},

	// TCppToken
	{  cpp_bool,			L"bool"			},  {  cpp_class,			L"class"			},
	{  cpp_explicit,		L"explicit"		},  {  cpp_export,			L"export"			},
	{  cpp_false,			L"false"			},  {  cpp_friend,			L"friend"			},
	{  cpp_inline,			L"inline"			},  {  cpp_mutable,			L"mutable"		},
	{  cpp_namespace,		L"namespace"		},  {  cpp_operator,		L"operator"		},
	{  cpp_private,		L"private"		},  {  cpp_protected,		L"protected"		},
	{  cpp_public,			L"public"			},  {  cpp_template,		L"template"		},
	{  cpp_this,			L"this"			},  {  cpp_true,			L"true"			},
	{  cpp_typename,		L"typename"		},  {  cpp_using,			L"using"			},
	{  cpp_virtual,			L"virtual"			},

	{  lex_props_end_mark  }
};

#define	MLEV_OBJ_SINDEX_ITEM_SUBT(lex_item_subt)			(lex_item_subt)
#define	MLEV_OBJ_SINDEX_EVENT_TYPE(evt_type)				(mlit_num_types+(evt_type))
#define	MLEV_OBJ_SINDEX_LEX_SRC_ENTER(srct_type)			(mlit_num_types+pset_num_types+(srct_type))
#define	MLEV_OBJ_SINDEX_MISC_PREPROC(exmis_type)		(mlit_num_types+pset_num_types+srct_num_types+(exmis_type))
#define	MLEV_OBJ_SINDEX_HIDDEN_INC(non_load_reason)		(mlit_num_types+pset_num_types+srct_num_types+exmis_num_types+(non_load_reason))
#define	MLEV_OBJ_SINDEX_ERROR_WARN_EVT(error_class)		(mlit_num_types+pset_num_types+srct_num_types+exmis_num_types+pinc_num_types+(error_class))

//
// ----  Mid Lev database objects  ----------------------------
//
static TLexItemStyleInfoProps g_LexItemMidLevEventsStyleSetup[] =
{
	{
		lex_props_any_index,					// All indexes.
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mid_lev,						// Main bkgr style.
		gti_bkgr_mid_lev_chlds,					// Rect lines chld area.
		gti_text_non_terminal,					// Text styles.
		gti_text_lexema_symbol,
		36,									// Circle diameter.
		1, 2,									// Main label offsets.
		0, 2,									// Symbol label offsets.
	},

	{
		MLEV_OBJ_SINDEX_LEX_SRC_ENTER(srct_cmdl),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_aux_call,				// Main bkgr style.
		gti_bkgr_mlev_aux_call_chlds,			// Rect lines chld area.
		gti_text_non_terminal,					// Text styles.
		gti_text_lexema_symbol,
		36,									// Circle diameter.
		1, 2,									// Main label offsets.
		0, 2,									// Symbol label offsets.
	},

	{
		MLEV_OBJ_SINDEX_LEX_SRC_ENTER(srct_file),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_file_call,					// Main bkgr style.
		gti_bkgr_mlev_file_call_chlds,			// Rect lines chld area.
		gti_text_non_terminal,					// Text styles.
		gti_text_lexema_symbol,
		36,									// Circle diameter.
		1, 2,									// Main label offsets.
		0, 2,									// Symbol label offsets.
	},

	{
		MLEV_OBJ_SINDEX_LEX_SRC_ENTER(srct_bltin_defs),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_aux_call,				// Main bkgr style.
		gti_bkgr_mlev_aux_call_chlds,			// Rect lines chld area.
		gti_text_non_terminal,					// Text styles.
		gti_text_lexema_symbol,
		36,									// Circle diameter.
		1, 2,									// Main label offsets.
		0, 2,									// Symbol label offsets.
	},

	{
		MLEV_OBJ_SINDEX_LEX_SRC_ENTER(srct_mj_src),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_mj_src_call,				// Main bkgr style.
		gti_bkgr_mlev_mj_src_call_chlds,			// Rect lines chld area.
		gti_text_cpp_defn,						// Text styles.
		gti_text_lexema_symbol,
		42,									// Circle diameter.
		1, 3,									// Main label offsets.
		0, 4,									// Symbol label offsets.
	},

	{
		MLEV_OBJ_SINDEX_MISC_PREPROC(exmis_error),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_error_stmt,				// Main bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_defn,						// Text styles.
		gti_text_lexema_symbol,
		39,									// Circle diameter.
		0, 2,									// Main label offsets.
		0, 2,									// Symbol label offsets.
	},

	{
		MLEV_OBJ_SINDEX_HIDDEN_INC(pinc_pasive_code),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_passive_stmt,			// Main bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_mlev_small_mlab,				// Text styles.
		gti_text_lexema_symbol,
		30,									// Circle diameter.
		1, 1,									// Main label offsets.
		0, 1,									// Symbol label offsets.
	},

	{
		MLEV_OBJ_SINDEX_HIDDEN_INC(pinc_pragma_once),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_passive_stmt,			// Main bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_mlev_small_mlab,				// Text styles.
		gti_text_lexema_symbol,
		30,									// Circle diameter.
		1, 1,									// Main label offsets.
		0, 1,									// Symbol label offsets.
	},

	{
		MLEV_OBJ_SINDEX_ERROR_WARN_EVT(lerrc_raw_scanner),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_error_event,				// Main bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_mlev_small_mlab,				// Text styles.
		gti_text_lexema_symbol,
		30,									// Circle diameter.
		0, 6,									// Main label offsets.
	},

	{
		MLEV_OBJ_SINDEX_ERROR_WARN_EVT(lerrc_preproc_err),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_error_event,				// Main bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_mlev_small_mlab,				// Text styles.
		gti_text_lexema_symbol,
		30,									// Circle diameter.
		0, 6,									// Main label offsets.
	},

	{
		MLEV_OBJ_SINDEX_ERROR_WARN_EVT(lerrc_syntax_err),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_error_event,				// Main bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_mlev_small_mlab,				// Text styles.
		gti_text_lexema_symbol,
		30,									// Circle diameter.
		0, 6,									// Main label offsets.
	},

	{
		MLEV_OBJ_SINDEX_ERROR_WARN_EVT(lerrc_analysis_err),
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_mlev_error_event,				// Main bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_mlev_small_mlab,				// Text styles.
		gti_text_lexema_symbol,
		30,									// Circle diameter.
		0, 6,									// Main label offsets.
	},

	{  lex_props_end_mark  }
};

//
// ----  Non terminals  ----------------------------
//
static TLexItemStyleInfoProps g_LexItemNonTerminalsStyleSetup[] =
{
	// First element of this array should contain lex_props_any_index in the symbol field.
	// When grammar is not set up, first slot is used to present any non terminal.
	{
		lex_props_any_index,					// All indexes.
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_non_terminal,					// Main bkgr style.
		gti_bkgr_non_term_chlds,				// Rect lines chld area.
		gti_text_non_terminal,					// Text styles.
		gti_text_non_terminal_symbol,
		36,									// Circle diameter.
		1, 3,									// Main label offsets.
		0, 3,									// Symbol label offsets.
	},

	{  lex_props_end_mark  }
};

//
// ----  Special items  --------------------------------
//
static TLexItemStyleInfoProps g_LexItemSpecialLexItemsStyleSetup[] =
{
	// Generic filler. In reality it will be used for painting various object categories.
	{
		lex_props_any_index,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_spec_lex_filler, 0xFF,			// Bkgr styles. The lower pcb rect is invisible.
		gti_text_rule_mlab,						// Text styles.
		gti_text_details,
		53,									// Circle diameter.
		0, -1,								// Main label offsets.
		0, 4,
	},

	// Ambiguous grammar conflicts (objects category).
	{
		slit_ambig_grm_conflicts,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_ambig_conflicts, 0xFF,			// Bkgr styles. The lower pcb rect is invisible.
		gti_text_rule_mlab,						// Text styles.
		gti_text_details,
		53,									// Circle diameter.
		0, -1,								// Main label offsets.
		0, 4,
	},

	// Grammar.
	{
		slit_grammar,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_grammar_inst, 0xFF,			// Bkgr styles. The lower pcb rect is invisible.
		gti_text_grammar_object,				// Text styles.
		gti_text_grammar_object,
		35,									// Circle diameter.
		0, 2,									// Main label offsets.
		0, 0,
	},

	// Rule label.
	{
		slit_rule_label,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_rule_label, 0xFF,				// Bkgr styles. The lower pcb rect is invisible.
		gti_text_rule_mlab,						// Text styles.
		gti_text_rule_symbol,
		23,									// Circle diameter.
		0, -1,								// Main label offsets.
		0, -1,
	},

	// Rule position.
	{
		slit_rule_position,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_rule_pos, 0xFF,				// Bkgr styles. The lower pcb rect is invisible.
		gti_text_message, gti_text_details,		// Text styles.
		37,									// Circle diameter.
		1, 3,									// Main label offsets.
		0, 1,									// "rpos" message.
	},

	// Expected conflict (instance).
	{
		slit_xpct_conflict,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_xpct_inst, 0xFF,				// Bkgr styles. The lower pcb rect is invisible.
		gti_text_message, gti_text_details,		// Text styles.
		44,									// Circle diameter.
		1, 7,									// Main label offsets.
	},

	// Grammar conflict (instance).
	{
		slit_grammar_conflict,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_conflict_inst, 0xFF,				// Bkgr styles. The lower pcb rect is invisible.
		gti_text_message, gti_text_details,		// Text styles.
		65,									// Circle diameter.
		1, 7,									// Main label offsets.
	},

	// Conflicting action.
	{
		slit_conflicting_action,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cfcting_action, 0xFF,			// Bkgr styles. The lower pcb rect is invisible.
		gti_text_message, gti_text_details,		// Text styles.
		42,									// Circle diameter.
		1, 3,									// Main label offsets.
		0, 3,									// Symbol value offsets.
	},

	// Derivation path as AST.
	{
		slit_derivation_path,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_derivation_path, 0xFF,			// Bkgr styles. The lower pcb rect is invisible.
		gti_text_small_title,					// Text styles.
		gti_text_non_terminal_symbol,
		35,									// Circle diameter.
		1, 3,									// Main label offsets.
		0, 2,									// Symbol value offset.
	},

	// Center button.
	{
		slit_center_button,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_center_button, 0xFF,			// Bkgr styles. The lower pcb rect is invisible.
		gti_text_small_title,					// Text styles.
		gti_text_details,
		29,									// Circle diameter.
		0, -1,								// Main label offsets.
		0, 2,
	},

	// Missing link.
	{
		slit_missing_link,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_missing_link, 0xFF,				// Bkgr styles. The lower pcb rect is invisible.
		gti_text_message,						// Text styles.
		gti_text_details,
		44,									// Circle diameter.
		1, 7,									// Main label offsets.
	},

	// Empty symbol.
	{
		slit_empty_symbol,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_lexema,						// Bkgr styles.
		gti_bkgr_lexema_chlds,
		gti_text_small_title,					// Text styles.
		gti_text_non_terminal_symbol,
		17,									// Circle diameter.
		1, 7,									// Main label offsets.
	},

	// Code location.
	{
		slit_code_location,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_code_location,					// Bkgr styles.
		gti_bkgr_code_location,
		gti_text_message, gti_text_details,		// Text styles.
		39,									// Circle diameter.
		1, 4,									// Main label offsets.
		0, 4,
	},

	// End of array marker.
	{  lex_props_end_mark  }
};

//
// ----  Visual parser items  --------------------------------
//
//  This array does not have default record because all members of the destination array are inited directly.
//
static TLexItemStyleInfoProps g_LexItemVisualParserLexItemsStyleSetup[vplit_num_types+1] =
{
	// Root of the parsed program.
	{
		vplit_program,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_program,						// Bkgr styles.
		gti_bkgr_program_chlds,
		gti_text_program,						// Text styles.
		gti_text_program,
		83,									// Circle diameter.
		1, 11,								// Main label offsets.
	},

	// Parser stack slot.
	{
		vplit_parser_stk_slot,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_parser_obj,					// Bkgr styles.
		gti_bkgr_parser_obj_chlds,
		gti_text_parser_obj_index,				// Text styles.
		gti_text_parser_obj_index,
		53,									// Circle diameter.
		1, 12,								// Slot index offsets.
	},

	// Input queue symbol.
	{
		vplit_parser_inp_sym,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_parser_obj,					// Bkgr styles.
		gti_bkgr_parser_obj_chlds,
		gti_text_parser_obj_index,				// Text styles.
		gti_text_parser_obj_index,
		53,									// Circle diameter.
		1, 12,								// Slot index text offsets.
	},

	// Out of memory marker.
	{
		vplit_null_item,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_null_item,						// Bkgr styles.
		gti_bkgr_null_item_chlds,
		gti_text_null_item, gti_text_null_item,		// Text styles.
		31,									// Circle diameter.
		1, 7,									// Main label offsets.
	},

	// End of array marker.
	{  lex_props_end_mark  }
};

//
// ----  Cpp database items  --------------------------------
//
static TLexItemStyleInfoProps g_LexItemCppDatabaseItemsStyleSetup[] =
{
	// Generic filler.
	{
		lex_props_any_index,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_misc,						// Bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_misc,						// Text styles.
		gti_text_cpp_item_symbol,
		44,									// Circle diameter.
		0, 3,									// Main label offsets.
		0, 5,									// Symbol label offsets.
	},

	// cpp_item_style_defn
	{
		TLangViewLexItemStyle::cpp_item_style_defn,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_defns,					// Bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_defn,						// Text styles.
		gti_text_cpp_item_symbol,
		56,									// Circle diameter.
		0, 4,									// Main label offsets.
		0, 9,									// Symbol label offsets.
	},

	// cpp_item_style_unresolved
	{
		TLangViewLexItemStyle::cpp_item_style_unresolved,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_unresolved,				// Bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_defn,						// Text styles.
		gti_text_cpp_item_symbol,
		56,									// Circle diameter.
		0, 4,									// Main label offsets.
		0, 9,									// Symbol label offsets.
	},

	// cpp_item_style_stmt
	{
		TLangViewLexItemStyle::cpp_item_style_stmt,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_stmts,					// Bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_misc,						// Text styles.
		gti_text_cpp_item_symbol,
		42,									// Circle diameter.
		0, 3,									// Main label offsets.
		0, 4,									// Symbol label offsets.
	},

	// cpp_item_style_label
	{
		TLangViewLexItemStyle::cpp_item_style_label,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_labels,					// Bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_misc,						// Text styles.
		gti_text_cpp_item_symbol,
		38,									// Circle diameter.
		0, 3,									// Main label offsets.
		0, 4,									// Symbol label offsets.
	},

	// cpp_item_style_expr
	{
		TLangViewLexItemStyle::cpp_item_style_expr,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_exprs,					// Bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_misc,						// Text styles.
		gti_text_cpp_item_symbol,
		34,									// Circle diameter.
		1, 2,									// Main label offsets.
		0, 3,									// Symbol label offsets.
	},

	// cpp_item_style_operand
	{
		TLangViewLexItemStyle::cpp_item_style_operand,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_oprnds,					// Bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_misc,						// Text styles.
		gti_text_cpp_item_symbol,
		34,									// Circle diameter.
		1, 2,									// Main label offsets.
		0, 3,									// Symbol label offsets.
	},

	// cpp_item_style_template_arg
	{
		TLangViewLexItemStyle::cpp_item_style_template_arg,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_oprnds,					// Bkgr style.
		gti_bkgr_cpp_temarg_chlds,				// Explicit color index.
		gti_text_cpp_misc,						// Text styles.
		gti_text_cpp_item_symbol,
		38,									// Circle diameter.
		1, 2,									// Main label offsets.
		0, 3,									// Symbol label offsets.
	},

	// cpp_item_style_dim_object
	{
		TLangViewLexItemStyle::cpp_item_style_dim_object,
		0, 0,									// Concentric and Rect lines app icons.
		gti_bkgr_cpp_dim_obj,					// Bkgr style.
		0xF8,								// Rect lines chld area has derived color.
		gti_text_cpp_dim_obj,					// Text styles.
		gti_text_cpp_item_symbol,
		42,									// Circle diameter.
		0, 3,									// Main label offsets.
		0, 4,									// Symbol label offsets.
	},

	// End of array marker.
	{  lex_props_end_mark  }
};

static double g_LexItemStyleSaturationGrades[TLangViewLexItemStyle::NUM_DIM_GRADES] =
{
	1.0, 0.60, 0.50, 0.40,
};
static double g_LexItemStyleLuminosityGrades[TLangViewLexItemStyle::NUM_DIM_GRADES] =
{
	1.0, 0.60, 0.30, 0.15,
};

static const wchar_t *g_LexItemSpecItemNames[slit_num_types] =
{
	L"slit_grammar_rules",
	L"slit_parsing_states",
	L"slit_xpct_conflicts",
	L"slit_all_grammar_conflicts",
	L"slit_ambig_grm_conflicts",
	L"slit_assigned_grm_conflicts",
	L"slit_unexp_grm_conflicts",

	L"slit_grammar",
	L"slit_rule_label",
	L"slit_rule_position",
	L"slit_xpct_conflict",
	L"slit_grammar_conflict",
	L"slit_conflicting_action",
	L"slit_derivation_path",

	L"slit_center_button",
	L"slit_empty_symbol",
	L"slit_missing_link",
	L"slit_code_location",
};

static const wchar_t *g_LexItemVisualParserItemNames[vplit_num_types] =
{
	L"vplit_program",
	L"vplit_parser_stk_slot",
	L"vplit_parser_inp_sym",
	L"vplit_null_item",
};

static const wchar_t *g_LexItemCppDatabaseItemNames[TLangViewLexItemStyle::cpp_item_num_styles] =
{
	L"cpp_item_style_default",
	L"cpp_item_style_defn",
	L"cpp_item_style_unresolved",
	L"cpp_item_style_stmt",
	L"cpp_item_style_label",
	L"cpp_item_style_expr",
	L"cpp_item_style_operand",
	L"cpp_item_style_template_arg",
	L"cpp_item_style_dim_object",
};

// ----------------------------------------------------------------------
//    ===============  TLangViewLexItemStyle  ====================
// ----------------------------------------------------------------------

wchar_t		*TLangViewLexItemStyle::g_DefaultProfileName = L"LexItemStyles.XML";
				// This is short file name. Profile is picked up from the directory of the executable.

TDateTime	TLangViewLexItemStyle::g_LatestScannedProfileDate = -1;
				// When the value of this field is -1, this corresponds to the "file is missing" case. This also means
				// that when the profile is not present at all, the error messages will not be emitted.

TConcentricModePlacingProps  TLangViewLexItemStyle::g_DefaultConcentricPlacingProps =
{
	FALSE,					// Do not balance the tree.
	FALSE,					// The direction of placing is counterclockwise.
	NULL,					// Info on the min radius steps.
	45,						// Arc step between the nodes in pixels.
	-90, 180,					// The tree grows down. The width of the angle is 180.
};

TRectLinesModePlacingProps  TLangViewLexItemStyle::g_DefaultRectLinesPlacingProps =
{
	10, 24,					// Horz steps inside and between families.
	12, 10,					// Top and bottom outer margins of the main rect.
	4,						// Vert lines step.
	FALSE,					// Regular orientation with root the tree up.
};

TLangViewLexItemStyle::TLangViewLexItemStyle()
{
	m_grammar = NULL;

	m_non_terminal_styles = NULL;
	m_non_terminal_styles_len = 0;

	m_show_terminals_grammar_names = FALSE;
}

void TLangViewLexItemStyle::Reset()
{
	m_grammar = NULL;

	if (m_non_terminal_styles != NULL)
	{
		free(m_non_terminal_styles);
		m_non_terminal_styles = NULL;
	}

	m_non_terminal_styles_len = 0;

	m_show_terminals_grammar_names = FALSE;

	m_dynamic_bkgr_styles.DeleteAll();
	m_dynamic_text_styles.DeleteAll();
	m_dynamic_line_styles.DeleteAll();
}

bool TLangViewLexItemStyle::SetupLexItemsProps(TGrammar *grammar, int dim_grade, bool show_terminals_grm_names, bool highlight_non_terms_transp,
												const wchar_t *short_fname, TGenericConsole *cons)
{
	Reset();
	m_grammar = grammar;
	m_show_terminals_grammar_names = show_terminals_grm_names;

	// Prepare the app icons directory.
	m_app_icons_style.Setup(NULL, g_LexItemIconsGridProps);

	// Setup and prepare the low level bkgr styles.
	for (int ibk=0; ibk<gti_bkgr_num_styles; ++ibk)
	{
		// Process the passed dim grade.
		TBasicStyleProps props = g_LexItemBkgrStyleProps[ibk];
		ApplyDimGrade(props, dim_grade);

		TBasicStyleProps lo_rect_props = props;
		THueSatLumColorHelper lo_rect_color_helper(lo_rect_props.background_color);
		lo_rect_color_helper.AdjustLuminosity(0.0, 0.70);
		lo_rect_props.background_color = lo_rect_color_helper.GetRgb();

		// Apply resulting props.
		m_bkgr_styles[ibk].Setup(props);
		m_bkgr_styles_dark_lo_rect[ibk].Setup(lo_rect_props);
	}

	// Setup and prepare the low level text styles.
	for (int itx=0; itx<gti_text_num_styles; ++itx)
	{
		// Process the passed dim grade.
		TTextStyleProps props = g_LexItemTextStyleProps[itx];
		ApplyDimGrade(props, dim_grade);

		// Apply resulting props.
		m_text_styles[itx].Setup(props);
		if (itx == gti_text_token_operator)
		{
			// Shift minus up.
			m_text_styles[itx].SetupSymbolsAdjustData(g_LexItem_TokenOperator_CharacterAdjustInfo);
		}
		else if (itx == gti_text_act_sym_operator || itx == gti_text_cinp_sym_operator)
		{
			m_text_styles[itx].SetupSymbolsAdjustData(g_LexItem_ActSymOperator_CharacterAdjustInfo);
		}
		else if (itx == gti_text_grammar_object)
		{
			// Narrow space.
			m_text_styles[itx].SetupSymbolsAdjustData(g_LexItem_GrammarObject_CharacterAdjustInfo);
		}
		else if (itx == gti_text_parser_obj_index)
		{
			// Shift brackets up.
			m_text_styles[itx].SetupSymbolsAdjustData(g_LexItem_ParserObjIndex_CharacterAdjustInfo);
		}
		else if (itx == gti_text_small_title)
		{
			// Shift brackets up.
			m_text_styles[itx].SetupSymbolsAdjustData(g_LexItem_SmallTitle_CharacterAdjustInfo);
		}
	}

	// Setup and prepare low level line styles.
	for (int iln=0; iln<gti_line_num_styles; ++iln)
	{
		// Process the passed dim grade.
		TLineStyleProps props = g_LexItemLineStyleProps[iln];
		ApplyDimGrade(props.foreground_color, dim_grade);

		// Apply resulting props.
		m_line_styles[iln].Setup(props);
	}

	// Setup the text label style pointers.
	m_lexema_label_styles[glxi_label_normal] = m_text_styles+gti_text_act_sym_normal;
	m_lexema_label_styles[glxi_label_operator] = m_text_styles+gti_text_act_sym_operator;
	m_lexema_label_styles[glxi_label_eof] = m_text_styles+gti_text_act_sym_eof;
	m_cfct_inp_sym_label_styles[glxi_label_normal] = m_text_styles+gti_text_cinp_sym_normal;
	m_cfct_inp_sym_label_styles[glxi_label_operator] = m_text_styles+gti_text_cinp_sym_operator;
	m_cfct_inp_sym_label_styles[glxi_label_eof] = m_text_styles+gti_text_cinp_sym_eof;
	m_non_term_label_style = m_text_styles+gti_text_non_term_label;

	// Process the style setup scripts.
	PrepareStyleInfosArray(litp_lexema,				m_lexema_type_styles,			ltx_num_lexema_types,			g_LexItemLexemaTypesStyleSetup);
	PrepareStyleInfosArray(litp_lexema,				m_keyword_styles,				ltkn_num_keyword_vals,		g_LexItemKeywordTypesStyleSetup);
	PrepareStyleInfosArray(litp_mid_lev_object,		m_mid_lev_event_styles,		mid_lev_object_num_styles,		g_LexItemMidLevEventsStyleSetup);
	PrepareStyleInfosArray(litp_special_lex_item,		m_spec_item_styles,			slit_num_types,				g_LexItemSpecialLexItemsStyleSetup);
	PrepareStyleInfosArray(litp_visual_parser_item,		m_vpars_item_styles,			vplit_num_types,				g_LexItemVisualParserLexItemsStyleSetup);
	PrepareStyleInfosArray(litp_cpp_database_item,		m_cpp_item_styles,			cpp_item_num_styles,			g_LexItemCppDatabaseItemsStyleSetup);

	// Process the main label scripts.
	PreparePreferredLabels(m_lexema_type_styles,		ltx_num_lexema_types,			g_LexItemLexemaTypesMainLabelSetup);
	PreparePreferredLabels(m_keyword_styles,			ltkn_num_keyword_vals,		g_LexItemKeywordTypesMainLabelsSetup);

	if (grammar != NULL && grammar->non_terminals.NumItems() != 0)
	{
		// Allocate space for non terminal symbols info.
		m_non_terminal_styles = (TLexItemStyleInfo*)malloc(grammar->NumNonTerminals()*sizeof(TLexItemStyleInfo));
		if (m_non_terminal_styles == NULL)
		{
			// Prepare default style to lower the risk of crash.
			PrepareStyleInfo(litp_non_terminal, m_def_non_terminal_style, &m_app_icons_style, g_LexItemNonTerminalsStyleSetup[0]);
			return(FALSE);
		}

		// Do grammar independent non terminal styles setup.
		m_non_terminal_styles_len = grammar->NumNonTerminals();
		PrepareStyleInfosArray(litp_non_terminal, m_non_terminal_styles, m_non_terminal_styles_len, g_LexItemNonTerminalsStyleSetup);

		if (highlight_non_terms_transp == TRUE)
		{
			// Default filler has neutral color for non terminals chld area. Fill in the "transparent" non terminals
			// with different pcb child area color.
			for (int ik=0; ik<m_grammar->NumNonTerminals(); ++ik)
			{
				if (m_grammar->non_terminals[ik].solid_symbol == FALSE)
				{
					m_non_terminal_styles[ik].basic_props.pcb_lower_bkgr_style = m_bkgr_styles+gti_bkgr_transp_non_term_chlds;
				}
				else
				{
					m_non_terminal_styles[ik].basic_props.pcb_lower_bkgr_style = m_bkgr_styles+gti_bkgr_opaq_non_term_chlds;
				}
			}
		}
	}
	else
	{
		// Prepare default styles that will be used for non terminals with any symbol value.
		PrepareStyleInfo(litp_non_terminal, m_def_non_terminal_style, &m_app_icons_style, g_LexItemNonTerminalsStyleSetup[0]);
	}

	// Setup links and bkgr selection style pointers.
	m_upper_link_styles[0] = m_line_styles+gti_line_upper_link_default;
	m_upper_link_styles[1] = m_line_styles+gti_line_upper_link_selected;
	m_right_link_styles[0] = m_line_styles+gti_line_right_link_default;
	m_right_link_styles[1] = m_line_styles+gti_line_right_link_selected;
	m_bkgr_selection_style[list_light_roze] = m_bkgr_styles+gti_bkgr_slct_light_roze;
	m_bkgr_selection_style[list_bold_blue] = m_bkgr_styles+gti_bkgr_slct_bold_blue;
	m_bkgr_selection_style[list_frin_nterm] = m_bkgr_styles+gti_bkgr_slct_foreign_nterm;

	if (short_fname != NULL)
	{
		// Load and process the styles profile.
		TLangViewLexItemStylePropsXmlHandler handler(this, dim_grade, highlight_non_terms_transp);
		if (handler.LoadFromExeDirectory(short_fname, cons, &g_LatestScannedProfileDate) == FALSE)
			return(FALSE);

		// Profile was loaded without errors. Pick up styles that have fixed names.
		bool overall_success = TRUE;
		overall_success &= handler.ResolveFixedTextStyleName(m_lexema_label_styles[glxi_label_normal], L"gti_text_act_sym_normal");
		overall_success &= handler.ResolveFixedTextStyleName(m_lexema_label_styles[glxi_label_operator], L"gti_text_act_sym_operator");
		overall_success &= handler.ResolveFixedTextStyleName(m_lexema_label_styles[glxi_label_eof], L"gti_text_act_sym_eof");
		overall_success &= handler.ResolveFixedTextStyleName(m_cfct_inp_sym_label_styles[glxi_label_normal], L"gti_text_cinp_sym_normal");
		overall_success &= handler.ResolveFixedTextStyleName(m_cfct_inp_sym_label_styles[glxi_label_operator], L"gti_text_cinp_sym_operator");
		overall_success &= handler.ResolveFixedTextStyleName(m_cfct_inp_sym_label_styles[glxi_label_eof], L"gti_text_cinp_sym_eof");
		overall_success &= handler.ResolveFixedTextStyleName(m_non_term_label_style, L"gti_text_non_term_label");
		overall_success &= handler.ResolveFixedLineStyleName(m_upper_link_styles[0], L"gti_line_upper_link_default");
		overall_success &= handler.ResolveFixedLineStyleName(m_upper_link_styles[1], L"gti_line_upper_link_selected");
		overall_success &= handler.ResolveFixedLineStyleName(m_right_link_styles[0], L"gti_line_right_link_default");
		overall_success &= handler.ResolveFixedLineStyleName(m_right_link_styles[1], L"gti_line_right_link_selected");
		overall_success &= handler.ResolveFixedBasicStyleName(m_bkgr_selection_style[list_light_roze], L"gti_bkgr_slct_light_roze");
		overall_success &= handler.ResolveFixedBasicStyleName(m_bkgr_selection_style[list_bold_blue], L"gti_bkgr_slct_bold_blue");
		overall_success &= handler.ResolveFixedBasicStyleName(m_bkgr_selection_style[list_frin_nterm], L"gti_bkgr_slct_foreign_nterm");

		if (overall_success == FALSE)
		{
			// One or more styles were not allocated.
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewLexItemStyle::SetupForDeviceContext(HDC hDC)
{
	if (TScreenItemStyle::SetupForDeviceContext(hDC) == FALSE)
		return(FALSE);

	if (m_app_icons_style.SetupForDeviceContext(hDC) == FALSE)
		return(FALSE);

	// Setup and prepare the bkgr styles.
	for (int ibk=0; ibk<gti_bkgr_num_styles; ++ibk)
	{
		if (m_bkgr_styles[ibk].SetupForDeviceContext(hDC) == FALSE)
			return(FALSE);

		if (m_bkgr_styles_dark_lo_rect[ibk].SetupForDeviceContext(hDC) == FALSE)
			return(FALSE);
	}

	// Setup and prepare the text styles.
	for (int itx=0; itx<gti_text_num_styles; ++itx)
	{
		if (m_text_styles[itx].SetupForDeviceContext(hDC) == FALSE)
			return(FALSE);
	}

	// Setup and prepare the line styles.
	for (int iln=0; iln<gti_line_num_styles; ++iln)
	{
		if (m_line_styles[iln].SetupForDeviceContext(hDC) == FALSE)
			return(FALSE);
	}

	// Setup and prepare dynamic basic styles if any.
	for (TListIter<TBasicStyle> iter1(m_dynamic_bkgr_styles); iter1; ++iter1)
	{
		if (iter1.CurrItem().SetupForDeviceContext(hDC) == FALSE)
			return(FALSE);
	}

	// Setup and prepare dynamic text styles if any.
	for (TListIter<TTextStyle> iter2(m_dynamic_text_styles); iter2; ++iter2)
	{
		if (iter2.CurrItem().SetupForDeviceContext(hDC) == FALSE)
			return(FALSE);
	}

	// Setup and prepare dynamic line styles if any.
	for (TListIter<TLineStyle> iter3(m_dynamic_line_styles); iter3; ++iter3)
	{
		if (iter3.CurrItem().SetupForDeviceContext(hDC) == FALSE)
			return(FALSE);
	}

	// Success.
	return(TRUE);
}

wchar_t *TLangViewLexItemStyle::GetLexemaTypeLabel(TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, wchar_t *buff_40_chars)
{
	if (lex_type == ltx_keyword && check_subt == TRUE)
	{
		// Check the subtype value.
		wchar_t *preferred_label = NULL;
		if (lex_subt.subtype >= 0 && lex_subt.subtype < ltkn_num_keyword_vals)
			preferred_label = m_keyword_styles[lex_subt.subtype].preferred_main_label;
		if (preferred_label != NULL)
			return(preferred_label);

		// Keyword id is bogus or the label is not set up.
		swprintf(buff_40_chars, 40, L"KeywId=%d", lex_subt.subtype);
		return(buff_40_chars);
	}
	else if (lex_type >= 0 && lex_type < ltx_num_lexema_types)
	{
		// This is a valid lexema type.
		wchar_t *mlab = m_lexema_type_styles[lex_type].preferred_main_label;
		assert(mlab != NULL);
		return(mlab);
	}
	else
	{
		// Lexema type is bogus.
		swprintf(buff_40_chars, 40, L"Ltx=%d", lex_type);
		return(buff_40_chars);
	}
}

TTextStyle *TLangViewLexItemStyle::GetLexemaTypeStyle(TLexemaType lex_type, bool check_subt, TLexSubtype lex_subt, bool bright_symbol)
{
	if (lex_type == ltx_keyword && check_subt == TRUE)
	{
		// Check the subtype value.
		if (lex_subt.subtype >= 0 && lex_subt.subtype < ltkn_num_keyword_vals && m_keyword_styles[lex_subt.subtype].basic_props.cnct_circle_diameter == 27)
		{
			// Return special operator style.
			return((bright_symbol == TRUE) ? m_lexema_label_styles[glxi_label_operator] : m_cfct_inp_sym_label_styles[glxi_label_operator]);
		}
		else
		{
			// Return regular lexema style.
			return((bright_symbol == TRUE) ? m_lexema_label_styles[glxi_label_normal] : m_cfct_inp_sym_label_styles[glxi_label_normal]);
		}
	}
	else if (lex_type == ltx_eof)
	{
		// Return special EOF style.
		return((bright_symbol == TRUE) ? m_lexema_label_styles[glxi_label_eof] : m_cfct_inp_sym_label_styles[glxi_label_eof]);
	}
	else
	{
		// Return regular lexema style.
		return((bright_symbol == TRUE) ? m_lexema_label_styles[glxi_label_normal] : m_cfct_inp_sym_label_styles[glxi_label_normal]);
	}
}

TLexItemStyleInfo &TLangViewLexItemStyle::GetLexemaStyleInfo(TLexemaType ltx, short keyword_id)
{
	if (ltx == ltx_keyword && keyword_id > 0)
	{
		// This is a keyword and its id is given explicitly.
		assert(keyword_id < ltkn_num_keyword_vals);

		// The style is based on the keyword id value.
		return(m_keyword_styles[keyword_id]);
	}
	else
	{
		// This is either a non keyword lexema or it is a "generic" keyword.
		assert(ltx >= ltx_empty && ltx <= ltx_error);

		// The style is based on the lexema type.
		return(m_lexema_type_styles[ltx]);
	}
}

TLexItemStyleInfo &TLangViewLexItemStyle::GetMidLevObjectStyleInfo(TMidLevObjectLexItemType mlev_item_subt)
{
	assert(mlev_item_subt >= 0 && mlev_item_subt < mlit_num_types);
	return(m_mid_lev_event_styles[MLEV_OBJ_SINDEX_ITEM_SUBT(mlev_item_subt)]);
}

TLexItemStyleInfo &TLangViewLexItemStyle::GetMidLevEventStyleInfo(CtvParsingEventInfo &evt_info)
{
	// Convert the props of the object into the linear index.
	int index = -1;
	if (evt_info.hdr.event_type == pset_lex_source_enter)
	{
		TLexSourceType srct = evt_info.src_enter.src_type;
		assert(srct >= 0 && srct < srct_num_types);
		index = MLEV_OBJ_SINDEX_LEX_SRC_ENTER(srct);
	}
	else if (evt_info.hdr.event_type == pset_misc_preproc_stmt)
	{
		TMiscEventEx exmis = evt_info.misc_stmt.mis_event_type;
		if (exmis == exmis_failed_include)
		{
			THiddenIncudeStmtDtls inc_dtls = (THiddenIncudeStmtDtls)evt_info.misc_stmt.evt_details;
			assert(inc_dtls >= 0 && inc_dtls < pinc_num_types);
			index = MLEV_OBJ_SINDEX_HIDDEN_INC(inc_dtls);
		}
		else
		{
			// Use the subtype of event as an index.
			assert(exmis >= 0 && exmis < exmis_num_types);
			index = MLEV_OBJ_SINDEX_MISC_PREPROC(exmis);
		}
	}
	else if (evt_info.hdr.event_type == pset_error_warning_event)
	{
		// Use the class of the error as an index.
		TLexErrorClass err_class = evt_info.err_warn.err_class;
		assert(err_class >= 0 && err_class < lerrc_num_errorclass_types);
		index = MLEV_OBJ_SINDEX_ERROR_WARN_EVT(err_class);
	}
	else
	{
		// Use the type of event as an index.
		assert(evt_info.hdr.event_type >= 0 && evt_info.hdr.event_type < pset_num_types);
		index = MLEV_OBJ_SINDEX_EVENT_TYPE(evt_info.hdr.event_type);
	}

	// Style is based on the composite index.
	return(m_mid_lev_event_styles[index]);
}

TLexItemStyleInfo &TLangViewLexItemStyle::GetNonTerminalStyleInfo(WORD symbol_value)
{
	if (m_non_terminal_styles_len > 0)
	{
		// Grammar info is present.
		assert(symbol_value >= NonTerminalsBase && symbol_value < NonTerminalsBase+m_non_terminal_styles_len);

		// Style is based on the symbol value.
		return(m_non_terminal_styles[symbol_value-NonTerminalsBase]);
	}
	else
	{
		// Return the grammar neutral default style.
		return(m_def_non_terminal_style);
	}
}

TLexItemStyleInfo &TLangViewLexItemStyle::GetSpecialItemStyleInfo(TSpecialLexItemType spec_item_type)
{
	assert(spec_item_type >= 0 && spec_item_type < slit_num_types);

	// Style is based on the type of the special item.
	return(m_spec_item_styles[spec_item_type]);
}

TLexItemStyleInfo &TLangViewLexItemStyle::GetVisualParserItemStyleInfo(TVisualParserLexItemType vpars_item_type)
{
	assert(vpars_item_type >= 0 && vpars_item_type < vplit_num_types);

	// Style is based on the type of the visual parser item.
	return(m_vpars_item_styles[vpars_item_type]);
}

TLexItemStyleInfo &TLangViewLexItemStyle::GetCppDatabaseItemStyleInfo(TCppItemBase *item)
{
	// Convert the props of the object into the linear index.
	int index = cpp_item_style_default;
	if (item != NULL)
	{
		// Check the contents of the cpp item.
		TCppItemType cit = item->ItemType();
		switch (cit)
		{
			case cxx_none:
					{
						index = cpp_item_style_dim_object;
					}
					break;

			// Defns 1.
			case cxx_data_type_alias:
			case cxx_enum_type:
			case cxx_enum_member:
			case cxx_struct_type:
			case cxx_data_field:

			// Defns 2.
			case cxx_function_type:
			case cxx_function_type_alias:
			case cxx_code_entry_point:
			case cxx_namespace:
			case cxx_global_namespace:

			// Templates.
			case cxx_class_template:
			case cxx_class_template_spec:
			case cxx_func_template:
			case cxx_func_template_spec:
			case cxx_template_type_param:
			case cxx_template_template_param:
					{
						index = cpp_item_style_defn;
					}
					break;

			case cxx_label:
			case cxx_case_label:
					{
						index = cpp_item_style_label;
					}
					break;

			case cxx_operand_expr:
					{
						index = cpp_item_style_operand;
					}
					break;

			case cxx_template_type_argument:
			case cxx_template_template_argument:
					{
						index = cpp_item_style_template_arg;
					}
					break;

			case cxx_unresolved_name_info:
			case cxx_unresolved_data_type:
			case cxx_unresolved_struct_type:
			case cxx_unresolved_data_field:
			case cxx_unresolved_code_entry_point:
			case cxx_unresolved_class_template:
			case cxx_unresolved_func_template:
					{
						index = cpp_item_style_unresolved;
					}
					break;

			default:
				{
					if (cit >= cxx_empty_stmt && cit <= cxx_light_block)
					{
						index = cpp_item_style_stmt;
					}
					else if (cit >= cxx_unary_expr && cit <= cxx_typeid_expr)
					{
						index = cpp_item_style_expr;
					}
				}
				break;
		}
	}

	// The style is based on the composite index.
	return(m_cpp_item_styles[index]);
}

void TLangViewLexItemStyle::ApplyDimGrade(TColor &color, int dim_grade)
{
	if (dim_grade >= MIN_DIM_GRADE)
	{
		if (dim_grade >= MAX_DIM_GRADE)
			dim_grade = MAX_DIM_GRADE;

		THueSatLumColorHelper colorHelper(color);
		colorHelper.sat_comp *= g_LexItemStyleSaturationGrades[dim_grade];
		colorHelper.AdjustLuminosity(0.35, g_LexItemStyleLuminosityGrades[dim_grade]);
		color = colorHelper.GetRgb();
	}
}

void TLangViewLexItemStyle::ApplyDimGrade(TBasicStyleProps &props, int dim_grade)
{
	ApplyDimGrade(props.background_color, dim_grade);
	ApplyDimGrade(props.shadow_up_color, dim_grade);
	ApplyDimGrade(props.shadow_down_color, dim_grade);
	ApplyDimGrade(props.frame_color, dim_grade);
	ApplyDimGrade(props.frame_shadow_up_color, dim_grade);
	ApplyDimGrade(props.frame_shadow_down_color, dim_grade);
}

void TLangViewLexItemStyle::ApplyDimGrade(TTextStyleProps &props, int dim_grade)
{
	ApplyDimGrade(props.foreground_color, dim_grade);
	ApplyDimGrade(props.background_color, dim_grade);
	ApplyDimGrade(props.frame_color, dim_grade);
	ApplyDimGrade(props.shadow_down_color, dim_grade);
}

void TLangViewLexItemStyle::ApplyDimGrade(TLineStyleProps &props, int dim_grade)
{
	ApplyDimGrade(props.foreground_color, dim_grade);
}

int TLangViewLexItemStyle::ResolveLexemaTypeName(const wchar_t *type_name)
{
	TLexemaType ltx = TLexema::GetLexTypeFromEnumName(type_name);
	if (ltx == ltx_num_lexema_types)
		return(-1);

	// Passed string corresponds to some lexema type.
	return(ltx);
}

int TLangViewLexItemStyle::ResolveKeywordName(const wchar_t *type_name)
{
	short value = TLexema::GetLexKeywordIdFromEnumName(type_name);
	if (value == 0)
		return(-1);

	// Passed string corresponds to some name of the keyword.
	return(value);
}

int TLangViewLexItemStyle::ResolveMidLevEventTypeName(const wchar_t *type_name)
{
	return(-1);
}

int TLangViewLexItemStyle::ResolveNonTerminalName(const wchar_t *type_name)
{
	if (m_non_terminal_styles_len <= 0)
		return(-1);

	// Grammar is existing and it has non terminals.
	WORD value = m_grammar->FindSymbol(type_name);
	if (value < NonTerminalsBase)
		return(-1);

	// Passed string corresponds to the name of the non terminal.
	return(value-NonTerminalsBase);
}

int TLangViewLexItemStyle::ResolveSpecItemName(const wchar_t *type_name)
{
	for (int inx=0; inx<slit_num_types; ++inx)
	{
		if (wcscmp(g_LexItemSpecItemNames[inx], type_name) == 0)
			return(inx);
	}

	// The passed string is bogus.
	return(-1);
}

int TLangViewLexItemStyle::ResolveVisualParserItemName(const wchar_t *type_name)
{
	for (int inx=0; inx<vplit_num_types; ++inx)
	{
		if (wcscmp(g_LexItemVisualParserItemNames[inx], type_name) == 0)
			return(inx);
	}

	// The passed string is bogus.
	return(-1);
}

int TLangViewLexItemStyle::ResolveCppDatabaseItemName(const wchar_t *type_name)
{
	for (int inx=0; inx<cpp_item_num_styles; ++inx)
	{
		if (wcscmp(g_LexItemCppDatabaseItemNames[inx], type_name) == 0)
			return(inx);
	}

	// The passed string is bogus.
	return(-1);
}

bool TLangViewLexItemStyle::ResolveBuiltInBasicStyleName(TBasicStyle *&style, const wchar_t *style_name)
{
	for (int inx=0; inx<gti_bkgr_num_styles; ++inx)
	{
		if (wcscmp(g_LexItemBasicBuiltInStyleNames[inx], style_name) == 0)
		{
			// The name is known.
			style = m_bkgr_styles+inx;
			return(TRUE);
		}
	}

	style = NULL;
	return(FALSE);
}

bool TLangViewLexItemStyle::ResolveBuiltInTextStyleName(TTextStyle *&style, const wchar_t *style_name)
{
	for (int inx=0; inx<gti_text_num_styles; ++inx)
	{
		if (wcscmp(g_LexItemTextBuiltInStyleNames[inx], style_name) == 0)
		{
			// The name is known.
			style = m_text_styles+inx;
			return(TRUE);
		}
	}

	style = NULL;
	return(FALSE);
}

bool TLangViewLexItemStyle::FindOrAllocateDynamicBasicStyle(TBasicStyle *&style, TNamedScreenItemStyleProps *sp_item, int dim_grade)
{
	// Look for style with the passed props among already known dynamic styles.
	for (TListIter<TBasicStyle> iter(m_dynamic_bkgr_styles); iter; ++iter)
	{
		if (iter.CurrItem().m_app_ptr == sp_item)
		{
			style = &(iter.CurrItem());
			return(TRUE);
		}
	}

	// These style props are new.
	TBasicStyle *dyna_style = new TBasicStyle();
	if (dyna_style == NULL)
	{
		// No luck. Return failure and preserve value of the style pointer.
		return(FALSE);
	}

	// Prepare props of the new style structure.
	TBasicStyleProps props = sp_item->m_basic_props;
	ApplyDimGrade(props, dim_grade);
	dyna_style->Setup(props);

	// Mark and store the new style structure.
	dyna_style->m_app_ptr = sp_item;
	m_dynamic_bkgr_styles.AppendItem(dyna_style);

	// Give the new style out.
	style = dyna_style;
	return(TRUE);
}

bool TLangViewLexItemStyle::FindOrAllocateDynamicTextStyle(TTextStyle *&style, TNamedScreenItemStyleProps *sp_item, int dim_grade)
{
	// Look for style with the passed props among already known dynamic styles.
	for (TListIter<TTextStyle> iter(m_dynamic_text_styles); iter; ++iter)
	{
		if (iter.CurrItem().m_app_ptr == sp_item)
		{
			style = &(iter.CurrItem());
			return(TRUE);
		}
	}

	// These style props are new.
	TTextStyle *dyna_style = new TTextStyle();
	if (dyna_style == NULL)
	{
		// No luck. Return failure and preserve value of the style pointer.
		return(FALSE);
	}

	// Prepare props of the new style structure.
	TTextStyleProps props = sp_item->m_text_props;
	ApplyDimGrade(props, dim_grade);
	dyna_style->Setup(props);

	// Mark and store the new style structure.
	dyna_style->m_app_ptr = sp_item;
	m_dynamic_text_styles.AppendItem(dyna_style);

	// Give the new style out.
	style = dyna_style;
	return(TRUE);
}

bool TLangViewLexItemStyle::FindOrAllocateDynamicLineStyle(TLineStyle *&style, TNamedScreenItemStyleProps *sp_item, int dim_grade)
{
	// Look for style with the passed props among already known dynamic styles.
	for (TListIter<TLineStyle> iter(m_dynamic_line_styles); iter; ++iter)
	{
		if (iter.CurrItem().m_app_ptr == sp_item)
		{
			style = &(iter.CurrItem());
			return(TRUE);
		}
	}

	// These style props are new.
	TLineStyle *dyna_style = new TLineStyle();
	if (dyna_style == NULL)
	{
		// No luck. Return failure and preserve value of the style pointer.
		return(FALSE);
	}

	// Prepare props of the new style structure.
	TLineStyleProps props = sp_item->m_line_props;
	ApplyDimGrade(props, dim_grade);
	dyna_style->Setup(props);

	// Mark and store the new style structure.
	dyna_style->m_app_ptr = sp_item;
	m_dynamic_line_styles.AppendItem(dyna_style);

	// Give the new style out.
	style = dyna_style;
	return(TRUE);
}

void TLangViewLexItemStyle::PrepareStyleInfosArray(TLexItemType itp, TLexItemStyleInfo *slots, int num_slots, TLexItemStyleInfoProps *props_script)
{
	// All property scripts are defined in the source code. Assume that EOF record is always present there.
	for(;;)
	{
		if (props_script->sym_index == lex_props_end_mark)
		{
			// End of the script.
			break;
		}
		else if (props_script->sym_index == lex_props_any_index)
		{
			// Fill in all slots of the array with the same info.
			for (int inx=0; inx<num_slots; ++inx)
				PrepareStyleInfo(itp, slots[inx], &m_app_icons_style, *props_script);
		}
		else
		{
			// Ensure that current record contains valid symbol index.
			assert(props_script->sym_index >= 0 && props_script->sym_index < num_slots);

			// Fill in the requested slot.
			TLexItemStyleInfo &slot = slots[props_script->sym_index];
			PrepareStyleInfo(itp, slot, &m_app_icons_style, *props_script);
		}

		// Shift to the next script record.
		props_script++;
	}
}

void TLangViewLexItemStyle::PreparePreferredLabels(TLexItemStyleInfo *slots, int num_slots, TLexItemMainLabelProps *props_script)
{
	// All property scripts are defined in the source code. Assume that EOF record is always present there.
	while (props_script->sym_index != lex_props_end_mark)
	{
		assert(props_script->sym_index >= 0 && props_script->sym_index < num_slots);

		slots[props_script->sym_index].preferred_main_label = props_script->main_label;
		props_script++;
	}
}

void TLangViewLexItemStyle::PrepareStyleInfo(TLexItemType itp, TLexItemStyleInfo &slot, TBitmapStyle *app_icons_style, TLexItemStyleInfoProps &props)
{
	slot.basic_props.app_icons_style = app_icons_style;
	slot.basic_props.app_cnct_icon = props.circ_icon;
	slot.basic_props.app_pcb_icon = props.pcb_icon;

	// Use array of regular backgrounds.
	slot.basic_props.main_bkgr_style = (props.main_bkgr_style_inx != 0xFF) ? m_bkgr_styles+props.main_bkgr_style_inx : NULL;
	if (props.pcb_lower_bkgr_style_inx == 0xFF)
	{
		// There should be no style at all.
		slot.basic_props.pcb_lower_bkgr_style = NULL;
	}
	else if (props.pcb_lower_bkgr_style_inx == 0xF8)
	{
		// Use style that is derived from the main bkgr style.
		assert(props.main_bkgr_style_inx < gti_bkgr_num_styles);
		slot.basic_props.pcb_lower_bkgr_style = m_bkgr_styles_dark_lo_rect+props.main_bkgr_style_inx;
	}
	else
	{
		// Use explicitly specified style index.
		slot.basic_props.pcb_lower_bkgr_style = m_bkgr_styles+props.pcb_lower_bkgr_style_inx;
	}

	slot.basic_props.main_label_style = m_text_styles + props.main_label_style_inx;
	slot.basic_props.symbol_label_style = m_text_styles + props.symbol_label_style_inx;

	// Concentric mode specific props.
	slot.basic_props.cnct_circle_diameter = props.circle_diameter;
	slot.basic_props.cnct_main_label_shift_x = props.main_label_shift_x;
	slot.basic_props.cnct_main_label_shift_y = props.main_label_shift_y;
	slot.basic_props.cnct_symbol_label_shift_x = props.symbol_label_shift_x;
	slot.basic_props.cnct_symbol_label_shift_y = props.symbol_label_shift_y;

	// Rect lines placing mode specific props.
	slot.basic_props.pcb_max_rect_width = 160;
	slot.basic_props.pcb_main_label_margins = 8;
	slot.basic_props.pcb_main_label_down_shift = 2;
	slot.basic_props.pcb_main_label_lo_space = 2;
	slot.basic_props.pcb_symlab_left_margin = 12;			// This value should correspond to the rcln_upper_link_offset value.
	slot.basic_props.pcb_symlab_right_margin = 4;
	slot.basic_props.pcb_lower_rect_height = 4;

	// Do adjustments for some of the slots.
	if (itp == litp_mid_lev_object)
	{
		if (props.sym_index == MLEV_OBJ_SINDEX_LEX_SRC_ENTER(srct_mj_src))
		{
			slot.basic_props.pcb_max_rect_width = 300;
			slot.basic_props.pcb_main_label_margins = 14;
			slot.basic_props.pcb_main_label_down_shift = 6;
			slot.basic_props.pcb_main_label_lo_space = 5;
			slot.basic_props.pcb_lower_rect_height = 6;
		}
		else if (props.sym_index == MLEV_OBJ_SINDEX_MISC_PREPROC(exmis_error))
		{
			slot.basic_props.pcb_main_label_down_shift = 3;
			slot.basic_props.pcb_main_label_lo_space = 3;
		}
		else if (props.sym_index == MLEV_OBJ_SINDEX_HIDDEN_INC(pinc_pasive_code))
		{
			slot.basic_props.pcb_main_label_margins = 5;
			slot.basic_props.pcb_main_label_lo_space = 1;
		}
		else if (props.sym_index == MLEV_OBJ_SINDEX_ERROR_WARN_EVT(lerrc_raw_scanner) || props.sym_index == MLEV_OBJ_SINDEX_ERROR_WARN_EVT(lerrc_preproc_err) || props.sym_index == MLEV_OBJ_SINDEX_ERROR_WARN_EVT(lerrc_syntax_err) || props.sym_index == MLEV_OBJ_SINDEX_ERROR_WARN_EVT(lerrc_analysis_err))
		{
			slot.basic_props.pcb_main_label_margins = 4;
			slot.basic_props.pcb_main_label_down_shift = 3;
		}
	}
	else if (itp == litp_special_lex_item)
	{
		if (props.sym_index == lex_props_any_index || props.sym_index == slit_ambig_grm_conflicts)
		{
			slot.basic_props.pcb_main_label_margins = 10;
			slot.basic_props.pcb_main_label_down_shift = 8;
			slot.basic_props.pcb_main_label_lo_space = 5;
		}
		else if (props.sym_index == slit_grammar)
		{
			slot.basic_props.pcb_main_label_margins = 10;
			slot.basic_props.pcb_main_label_down_shift = 7;
			slot.basic_props.pcb_main_label_lo_space = 4;
		}
		else if (props.sym_index == slit_rule_label)
		{
			slot.basic_props.pcb_main_label_down_shift = 5;
		}
		else if (props.sym_index == slit_rule_position)
		{
			slot.basic_props.pcb_main_label_margins = 10;
			slot.basic_props.pcb_main_label_down_shift = 7;
			slot.basic_props.pcb_main_label_lo_space = 2;
		}
		else if (props.sym_index >= slit_xpct_conflict && props.sym_index <= slit_conflicting_action)
		{
			slot.basic_props.pcb_main_label_margins = 13;
			slot.basic_props.pcb_main_label_down_shift = 8;
			slot.basic_props.pcb_main_label_lo_space = 4;
		}
		else if (props.sym_index == slit_derivation_path)
		{
			slot.basic_props.pcb_main_label_margins = 11;
			slot.basic_props.pcb_main_label_down_shift = 8;
			slot.basic_props.pcb_main_label_lo_space = 3;
		}
		else if (props.sym_index == slit_missing_link)
		{
			slot.basic_props.pcb_main_label_margins = 10;
			slot.basic_props.pcb_main_label_down_shift = 8;
			slot.basic_props.pcb_main_label_lo_space = 4;
		}
	}
	else if (itp == litp_visual_parser_item)
	{
		if (props.sym_index == vplit_program)
		{
			slot.basic_props.pcb_main_label_margins += 14;
			slot.basic_props.pcb_main_label_down_shift += 10;
			slot.basic_props.pcb_main_label_lo_space += 11;
			slot.basic_props.pcb_lower_rect_height += 2;
		}
	}
	else if (itp == litp_cpp_database_item)
	{
		slot.basic_props.pcb_main_label_margins = 10;
		slot.basic_props.pcb_main_label_down_shift = 3;
		slot.basic_props.pcb_main_label_lo_space = 2;

		if (props.sym_index == cpp_item_style_defn)
		{
			slot.basic_props.pcb_main_label_down_shift = 4;
			slot.basic_props.pcb_main_label_lo_space = 3;
		}
		else if (props.sym_index == cpp_item_style_unresolved)
		{
			slot.basic_props.pcb_main_label_margins = 14;
			slot.basic_props.pcb_main_label_down_shift = 6;
			slot.basic_props.pcb_main_label_lo_space = 4;
			slot.basic_props.pcb_lower_rect_height = 8;
		}
	}

	// There is no preferred label by default.
	slot.preferred_main_label = NULL;
}

// --------------------------------------------------------------------------
//    ==================  TLangViewLexItem  =======================
// --------------------------------------------------------------------------

// Ids of the icons on the LexItems app icons bitmap.
static const short g_LangViewSymbolSpecialOriginIconIds[sporg_num_types] =
{
	0,				// No conflict was resolved and the app callback was not updating the parsing action.
	200,				// The syntax error was fixed or the parsing action was updated.
	301,				// Conflict, resolved by the app callback (RED).
	302,				// Conflict, resolved by the nested grammar (BLUE).
	303,				// Nested processing result, resolved by the app callback (VIOLET-RED).
};

static const short g_LangViewLexItemsSlct_DiameterDelta[list_num_types] = { 10, 10, 6 };
static const short g_LangViewLexItemsSlct_HorzExpand[list_num_types] = { 6, 5, 2 };
static const short g_LangViewLexItemsSlct_VertExpand[list_num_types] = { 8, 5, 2 };

TLangViewLexItem::TLangViewLexItem(TLexItemType item_type, TLangViewLexItemStyle *style)
		: TBasicSymbolItem(style)
{
	m_item_type = item_type;
}

void TLangViewLexItem::PrepareForConcentricMode(HDC hDC, long pos_x, long pos_y)
{
	// Ask the derived class for the appropriate style info.
	TLexItemStyleInfo &style_info = GetStyleInfo();

	// Pick up the messages for the object.
	short extra_vert_offs = 0;
	wchar_t buff_40_chars1[40], buff_40_chars2[40];
	wchar_t *main_label = GetMainLabel(style_info, buff_40_chars1, plc_mode_concentric);
	wchar_t *symbol_label = GetSymbolLabel(style_info, buff_40_chars2, &extra_vert_offs, plc_mode_concentric);

	// Call the worker function of the base class.
	style_info.basic_props.cnct_symbol_label_shift_y += extra_vert_offs;
	PrepareConcentricModeItems(hDC, style_info.basic_props, pos_x, pos_y, main_label, -1, symbol_label, -1, GetAppIconId(plc_mode_concentric));
	style_info.basic_props.cnct_symbol_label_shift_y -= extra_vert_offs;
}

void TLangViewLexItem::PrepareForRectLinesMode(HDC hDC, long pos_x, long pos_y, int num_children, bool root_down_placing)
{
	// Ask the derived class for the appropriate style info.
	TLexItemStyleInfo &style_info = GetStyleInfo();

	// Pick up the messages for the object.
	wchar_t buff_40_chars1[40], buff_40_chars2[40];
	wchar_t *main_label = GetMainLabel(style_info, buff_40_chars1, plc_mode_rect_lines);
	wchar_t *symbol_label = GetSymbolLabel(style_info, buff_40_chars2, NULL, plc_mode_rect_lines);

	// Evaluate the min width.
	long min_width = GetMinWidthForPlacingChldLinks(hDC, style_info, num_children, (root_down_placing == TRUE) ? symbol_label : NULL);

	// Call the worker function of the base class.
	PrepareRectLinesModeItems(hDC, style_info.basic_props, pos_x, pos_y, min_width, main_label, -1, symbol_label, -1, GetAppIconId(plc_mode_rect_lines));
}

TScreenItem *TLangViewLexItem::CreateSelectionScreenItem(TLangViewLexItemSelectionType selt)
{
	assert(selt >= 0 && selt < list_num_types);

	if (m_def_circle.GetDiameter() > 0)
	{
		// The object is in the concentric mode.
		TCircleItem *circle_bkgr = new TCircleItem(Style()->GetBkgrSelectionStyle(selt));
		if (circle_bkgr == NULL)
			return(NULL);

		// Place the new object.
		long px, py;
		m_def_circle.GetHotSpotPos(px, py);
		circle_bkgr->SetPosition(px, py, m_def_circle.GetDiameter()+g_LangViewLexItemsSlct_DiameterDelta[selt]);
		return(circle_bkgr);
	}
	else
	{
		// The object is in the rect lines mode.
		TRectItem *rect_bkgr = new TRectItem(Style()->GetBkgrSelectionStyle(selt));
		if (rect_bkgr == NULL)
			return(NULL);

		RECT base_rect;
		m_main_rect.GetBaseRect(base_rect);

		// Place the new object.
		long expand_val_horz = g_LangViewLexItemsSlct_HorzExpand[selt];
		long expand_val_vert = g_LangViewLexItemsSlct_VertExpand[selt];
		base_rect.left -= expand_val_horz;
		base_rect.top -= expand_val_vert;
		base_rect.right += expand_val_horz;
		base_rect.bottom += expand_val_vert;
		rect_bkgr->SetBaseRect(base_rect);
		return(rect_bkgr);
	}
}

int TLangViewLexItem::GetConcentricModeParam(HDC hDC, TConcentricPlacingModeParamIndex inx)
{
	if (inx == cnct_node_diameter)
	{
		// Diameter is stored in the style info of the item.
		return(GetStyleInfo().basic_props.cnct_circle_diameter);
	}
	else
	{
		// Bogus or unimplemented index.
		assert(FALSE);
		return(0);
	}
}

int TLangViewLexItem::GetRectLinesModeParam(HDC hDC, TRectLinesPlacingModeParamIndex inx, TRectLinesModePlacingProps &props, int num_children)
{
	int retValue = 0;
	switch (inx)
	{
		case rcln_none:
				{
					retValue = 0;
				}
				break;

		case rcln_size_x:
				{
					// Pick up the style and messages for the object.
					TLexItemStyleInfo &style_info = GetStyleInfo();
					wchar_t buff_40_chars1[40], buff_40_chars2[40];
					wchar_t *main_label = GetMainLabel(style_info, buff_40_chars1, plc_mode_rect_lines);
					wchar_t *symbol_label = GetSymbolLabel(style_info, buff_40_chars2, NULL, plc_mode_rect_lines);

					// Evaluate the min width.
					long min_width = GetMinWidthForPlacingChldLinks(hDC, style_info, num_children, (props.up_side_down_placing == TRUE) ? symbol_label : NULL);

					// Method of the base class will calculate the actual width.
					retValue = GetRectLinesModeRectWidth(hDC, style_info.basic_props, min_width, main_label, -1, symbol_label, -1);
				}
				break;

		case rcln_size_y:
				{
					TLexItemStyleInfo &style_info = GetStyleInfo();
					retValue = GetRectLinesModeRectHeight(hDC, style_info.basic_props);
				}
				break;

		case rcln_upper_link_offset:
				{
					retValue = RCLN_UPPER_LINK_OFFSET;			// This value should correspond to TBasicSymbolStyleInfo.pcb_symlab_left_margin.
				}
				break;

		case rcln_chld_links_offset:
				{
					retValue = RCLN_CHLD_LINKS_OFFSET;
					if (m_item_type == litp_special_lex_item)
					{
						// Center the child links with some minor left shift.
						int szx = GetRectLinesModeParam(hDC, rcln_size_x, props, num_children);
						retValue = (szx-RCLN_CHLD_LINKS_STEP*(num_children-1))/3;
					}
				}
				break;

		case rcln_chld_links_step:
				{
					retValue = RCLN_CHLD_LINKS_STEP;
				}
				break;

		default:
			{
				// Bogus or unimplemented index.
				assert(FALSE);
				return(0);
			}
	}

	return(retValue);
}

long TLangViewLexItem::GetMinWidthForPlacingChldLinks(HDC hDC, TLexItemStyleInfo &style_info, int num_children, wchar_t *symbol_label)
{
	// Evaluate the min width for normal placing and vertically reversed placing when item has no children.
	long min_width = RCLN_CHLD_LINKS_OFFSET+2*style_info.basic_props.pcb_symlab_right_margin;
	if (num_children > 1)
		min_width += RCLN_CHLD_LINKS_STEP*(num_children-1);

	if (symbol_label != NULL)
	{
		// This param tells that the tree is displayed with its root at the bottom.
		if (num_children > 0)
		{
			// Use alternative strategy to calc the min width.
			min_width = 2*RCLN_CHLD_LINKS_OFFSET+RCLN_CHLD_LINKS_STEP*(num_children-1);
			min_width += style_info.basic_props.symbol_label_style->GetStringWidth(hDC, symbol_label)+style_info.basic_props.pcb_symlab_right_margin;
		}
	}

	return(min_width);
}

wchar_t *TLangViewLexItem::FormatSymbolValue(WORD symbol, wchar_t *buff_40_chars, short *extra_vert_offs)
{
	if (extra_vert_offs != NULL)
		*extra_vert_offs = 0;

	if (symbol == 0xFFFF)
	{
		// Lexema was not mapped to any symbol of the grammar.
		return(L"-Unk-");
	}
	else if (symbol >= IgnoreLexTypeBase && symbol < ErrorLexTypeBase)
	{
		// Do not show exact symbol value due to the lack of space.
		return(L"-Ign-");
	}
	else if (symbol >= ErrorLexTypeBase && symbol < NonTerminalsBase)
	{
		// Do not show exact symbol value due to the lack of space.
		return(L"-Err-");
	}
	else if (symbol == 0)
	{
		// This is the EOF symbol.
		if (extra_vert_offs != NULL)
			*extra_vert_offs = -2;
		return(L"eof");
	}

	// Show just the symbol value without any comments.
	swprintf(buff_40_chars, 40, L"%hd", symbol);
	return(buff_40_chars);
}

TLangViewSymbolSpecialOrigin TLangViewLexItem::GetSpecOriginFromSpecOriginInfo(WORD final_action, TParsingActionOriginInfo &info)
{
	if (info.dw_action_origin == 0xFFFFFFFF)
		return(sporg_none);

	assert(final_action < actb_conflict);
	TLangViewSymbolSpecialOrigin spec_org = sporg_none;

	if (info.conflict_action == 0)
	{
		// Initial parsing action was picked up from the analysis table.
		if (info.initial_action != final_action)
		{
			if (info.initial_action >= actb_conflict && info.initial_action < actb_nested_result)
			{
				// Initial action was conflict and nested processing was not involved. This means that
				// app callback handler has resolved this conflict.
				spec_org = sporg_resolved_conflict;
			}
			else
			{
				// App callback has changed the parsing action for some unclear reason.
				spec_org = sporg_cooked;
			}
		}
	}
	else
	{
		// Initial parsing action came from the resolution vector after returning from the nested grammar.
		if (info.initial_action == final_action)
		{
			// This is plain conflict resolution.
			spec_org = sporg_nested_parsing;
		}
		else if (info.initial_action >= actb_nested_result)
		{
			// Nested parsing resulted in nested proc res that was resolved into the parsing action by the callback handler.
			spec_org = sporg_resolved_nres;
		}
		else
		{
			// App callback has changed the parsing action for some unclear reason.
			spec_org = sporg_cooked;
		}
	}

	return(spec_org);
}

// -----------------------------------------------------------------------------
//    ==================  TLangViewLexemaLexItem  ====================
// -----------------------------------------------------------------------------

// Ctor for creating terminal symbols as part of the grammar definition.
TLangViewLexemaLexItem::TLangViewLexemaLexItem(WORD symbol, TLexemaType lex_type, bool check_subt, DWORD lex_subt)
		: TLangViewLexItem(litp_lexema)
{
	m_symbol = symbol;
	m_grammar_object = TRUE;

	m_lex_info.Clear();
	m_lex_info.type = lex_type;

	if (check_subt == TRUE)
	{
		if (lex_type == ltx_keyword)
		{
			// Reconstruct subtype from the keyword id.
			TLexKeywordType keyw_subt = TLexema::GetKeywordTypeFromTokenId(lex_subt);
			assert(keyw_subt != ltkn_num_keyword_types);

			m_lex_info.subtype = keyw_subt;
			m_lex_info.num_value = lex_subt;
		}
		else
		{
			// Simply store the passed subtype.
			m_lex_info.subtype = lex_subt;
		}
	}

	m_check_subt = check_subt;
	m_shift_dest_state = 0xFFFF;
	m_spec_origin.dw_action_origin = 0xFFFFFFFF;
}

// Ctor for creating terminal symbols during the parsing process.
TLangViewLexemaLexItem::TLangViewLexemaLexItem(WORD symbol, TLexemaInfo &lex, WORD dest_state, TParsingActionOriginInfo spec_origin)
		: TLangViewLexItem(litp_lexema)
{
	m_symbol = symbol;
	m_grammar_object = FALSE;

	m_lex_info = lex;
	m_check_subt = TRUE;
	m_shift_dest_state = dest_state;
	m_spec_origin = spec_origin;
}

TLexItemStyleInfo &TLangViewLexemaLexItem::GetStyleInfo()
{
	if (m_lex_info.type != ltx_keyword)
	{
		// Style of all non keyword lexemas depends only on the type of lexema.
		return(Style()->GetLexemaStyleInfo(m_lex_info.type, 0));
	}

	// Style of the keyword lexemas depends only on the keyword id if checking the subtype is active.
	return(Style()->GetLexemaStyleInfo(ltx_keyword, (m_check_subt == TRUE) ? (short)m_lex_info.num_value : 0));
}

short TLangViewLexemaLexItem::GetAppIconId(TTreeItemsPlacingMode mode)
{
	if (m_grammar_object == TRUE)
		return(0);

	return(g_LangViewSymbolSpecialOriginIconIds[GetSpecOriginFromSpecOriginInfo(m_shift_dest_state, m_spec_origin)]);
}

wchar_t *TLangViewLexemaLexItem::GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode)
{
	if (Style()->GetGrammar() != NULL && Style()->GetShowLexemasGrammarNameMode() == TRUE)
	{
		// Show the name of the lexema symbol as it is defined in the grammar.
		return(Style()->GetGrammar()->GetSymbolName(m_symbol, buff_40_chars));
	}

	// Filter out lexema types that need special processing.
	if (m_lex_info.type == ltx_number && m_grammar_object == FALSE)
	{
		// Show the value of this integer constant.
		return(FormatInt64(m_lex_info.num_value, buff_40_chars, 40, ((m_lex_info.subtype & 1) != 0) ? fnms_dec_unsigned : fnms_dec_signed));
	}
	if (m_lex_info.type == ltx_floating_point && m_grammar_object == FALSE)
	{
		// Show the value of this floating point constant.
		swprintf(buff_40_chars, 40, L"%lg", m_lex_info.GetDoubleValue());
		return(buff_40_chars);
	}
	else if (m_lex_info.type == ltx_keyword)
	{
		// In the regular case the keywords are displayed in the grammar and in the src code in the same way.
		// Check for the preferred name in the style info. Picking it up might be a rather complex procedure.
		if (style_info.preferred_main_label != NULL)
			return(style_info.preferred_main_label);

		// There is no preferred name. Display the token enum name.
		return((wchar_t*)TLexema::GetKeywordIdEnumName((int)m_lex_info.num_value));
	}
	else if (m_lex_info.type == ltx_name)
	{
		if (m_grammar_object == FALSE)
		{
			// Display the name itself.
			return(m_lex_info.str_value);
		}
		else
		{
			// This is a name in the grammar definition. Show a const string disregarding the name subtype.
			#ifdef WANT_IDENTIFIER_NAME
				return(L"ident");
			#else
				return(L"name");
			#endif
		}
	}

	// All other lexema types use the preferred label. Ensure that it is present.
	assert(style_info.preferred_main_label != NULL);
	return(style_info.preferred_main_label);
}

// ---------------------------------------------------------------------------
//    ================  TLangViewMidLevObjectLexItem  ==================
// ---------------------------------------------------------------------------

TLangViewMidLevObjectLexItem::TLangViewMidLevObjectLexItem(CtvCompilationInfo &pars_info)
		: TLangViewLexItem(litp_mid_lev_object)
{
	m_mlev_obj_type = mlit_compilation_info;
	m_pars_info = pars_info;
}

TLangViewMidLevObjectLexItem::TLangViewMidLevObjectLexItem(TFileInfo &file_info)
		: TLangViewLexItem(litp_mid_lev_object)
{
	m_mlev_obj_type = mlit_file_info;
	m_file_info = file_info;
}

TLangViewMidLevObjectLexItem::TLangViewMidLevObjectLexItem(TKeyInfo &key_info)
		: TLangViewLexItem(litp_mid_lev_object)
{
	m_mlev_obj_type = mlit_key_info;
	m_key_info = key_info;
}

TLangViewMidLevObjectLexItem::TLangViewMidLevObjectLexItem(CtvParsingEventInfo &event_info)
		: TLangViewLexItem(litp_mid_lev_object)
{
	m_mlev_obj_type = mlit_parsing_event;
	m_event = event_info;
}

bool TLangViewMidLevObjectLexItem::PrepareClickEvent(TLangViewClickEvent &info)
{
	switch (m_mlev_obj_type)
	{
		case mlit_compilation_info:
				info.type = clet_pars_session;
				info.pars_sess_data = m_pars_info;
				break;

		case mlit_file_info:
				// For now this can be only the file object, not the directory object.
				info.type = clet_file_object;
				info.file_data = m_file_info;
				break;

		case mlit_key_info:
				info.type = clet_indexing_key;
				info.key_data = m_key_info;
				break;

		case mlit_parsing_event:
				info.type = clet_mid_lev_event;
				info.mldb_event_data = m_event;
				break;

		default:
			// Most likely the object is corrupted.
			assert(FALSE);
			info.Clear();
			return(FALSE);
	}

	return(TRUE);
}

TLexItemStyleInfo	&TLangViewMidLevObjectLexItem::GetStyleInfo()
{
	if (m_mlev_obj_type != mlit_parsing_event)
	{
		return(Style()->GetMidLevObjectStyleInfo(m_mlev_obj_type));
	}
	else
	{
		return(Style()->GetMidLevEventStyleInfo(m_event));
	}
}

wchar_t *TLangViewMidLevObjectLexItem::GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode)
{
	switch (m_mlev_obj_type)
	{
		case mlit_compilation_info:	return(L"CompilationInfo");
		case mlit_file_info:		return(L"FileInfo");
		case mlit_key_info:		return(L"IndxKeyInfo");

		case mlit_parsing_event:
				{
					switch (m_event.hdr.event_type)
					{
						case pset_lex_source_enter:
								{
									switch (m_event.src_enter.src_type)
									{
										case srct_file:
										case srct_mj_src:
												{
													return((wchar_t*)TPathHelper::GetShortNamePtr(m_event.src_enter.inc_file_info.info.file_name));
												}

										default:	return((wchar_t*)TLexemasSource::GetLexSourceTypeEnumName(m_event.src_enter.src_type));
									}
								}

						case pset_misc_preproc_stmt:
								{
									if (m_event.misc_stmt.mis_event_type == exmis_failed_include)
									{
										if (m_event.misc_stmt.evt_details == pinc_pasive_code || m_event.misc_stmt.evt_details == pinc_pragma_once)
										{
											// Show the name of the file.
											return((wchar_t*)TPathHelper::GetShortNamePtr(m_event.misc_stmt.file_name));
										}
										else
										{
											// Show the reason why the #include file was not entered.
											swprintf(buff_40_chars, 40, L"#inc: %s", PrMiscPreprocStmtEvent::GetNonCallReasonName((THiddenIncudeStmtDtls)m_event.misc_stmt.evt_details));
											return(buff_40_chars);
										}
									}
									else if (m_event.misc_stmt.mis_event_type == exmis_pragma)
									{
										return(L"#pragma");
									}
									else if (m_event.misc_stmt.mis_event_type == exmis_error)
									{
										return(L"#error");
									}

									return((wchar_t*)PrMiscPreprocStmtEvent::GetMiscEventExEnumName(m_event.misc_stmt.mis_event_type));
								}

						case pset_error_warning_event:
								{
									// Show the error code as the main label.
									wchar_t *etype_prefix = L"";
									if (LEX_ERROR_CLASS_WARNING(m_event.err_warn.err_class) == TRUE)
										etype_prefix = L"W";
									else if (LEX_ERROR_CLASS_ERROR(m_event.err_warn.err_class) == TRUE)
										etype_prefix = L"E";

									swprintf(buff_40_chars, 40, L"%s%d", etype_prefix, m_event.err_warn.err_code);
									return(buff_40_chars);
								}
					}

					return(L"MLDB");
				}
	}

	return(L"Bogus-MLDB-subtype");
}

wchar_t *TLangViewMidLevObjectLexItem::GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode)
{
	switch (m_mlev_obj_type)
	{
		case mlit_compilation_info:
				{
					swprintf(buff_40_chars, 40, L"%lu", m_pars_info.parsing_id);
					return(buff_40_chars);
				}

		case mlit_file_info:
				{
					swprintf(buff_40_chars, 40, L"%lu", m_file_info.file_id);
					return(buff_40_chars);
				}

		case mlit_key_info:
				{
					swprintf(buff_40_chars, 40, L"%lu", m_key_info.key_id);
					return(buff_40_chars);
				}

		case mlit_parsing_event:
				{
					// Do not show ids for the parsing event to keep size of this object small.
					if (m_event.hdr.event_type != pset_error_warning_event)
					{
						swprintf(buff_40_chars, 40, L"%ld.%ld", m_event.hdr.parsing_id, m_event.hdr.event_id);
						return(buff_40_chars);
					}
				}
	}

	return(NULL);
}

// ---------------------------------------------------------------------------
//    ================  TLangViewNonTerminalLexItem  ==================
// ---------------------------------------------------------------------------

TLangViewNonTerminalLexItem::TLangViewNonTerminalLexItem(WORD symbol)
		: TLangViewLexItem(litp_non_terminal)
{
	m_symbol = symbol;
	m_grammar_object = TRUE;

	m_rule_index = 0xFFFF;
	m_origin.Clear();
	m_spec_origin.dw_action_origin = 0xFFFFFFFF;
}

TLangViewNonTerminalLexItem::TLangViewNonTerminalLexItem(WORD symbol, WORD rule_index, TSourceLocation &origin, TParsingActionOriginInfo spec_origin)
		: TLangViewLexItem(litp_non_terminal)
{
	m_symbol = symbol;
	m_grammar_object = FALSE;

	m_rule_index = rule_index;
	m_origin = origin;
	m_spec_origin = spec_origin;
}

short TLangViewNonTerminalLexItem::GetAppIconId(TTreeItemsPlacingMode mode)
{
	if (m_grammar_object == TRUE)
		return(0);

	return(g_LangViewSymbolSpecialOriginIconIds[GetSpecOriginFromSpecOriginInfo(actb_reduce+m_rule_index, m_spec_origin)]);
}

wchar_t *TLangViewNonTerminalLexItem::GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode)
{
	// Non terminal object.
	if (style_info.preferred_main_label != NULL)
	{
		// This non terminal has special label (non typical case).
		return(style_info.preferred_main_label);
	}

	// There is no preferred label for this non terminal.
	if (Style()->m_grammar != NULL)
	{
		// Return name of this non terminal in the grammar.
		int sym_inx = m_symbol-NonTerminalsBase;
		assert(sym_inx >= 0 && sym_inx < Style()->m_non_terminal_styles_len);
		return(Style()->m_grammar->non_terminals[sym_inx].symbol_name);
	}

	// Grammar is not set up. Return dummy name.
	return(L"NonTerm");
}

// ----------------------------------------------------------------------------
//    =================  TLangViewSpecialLexItem  =====================
// ----------------------------------------------------------------------------

TLangViewSpecialLexItem::TLangViewSpecialLexItem(TSpecialLexItemType type)
		: TLangViewLexItem(litp_special_lex_item)
{
	m_special_item_type = type;
	ClearAppLevelFields();
}

void TLangViewSpecialLexItem::SetGrammarCategoryProps(TSpecialLexItemType category_type, WORD igrammar, int num_instances)
{
	assert(category_type >= slit_grammar_rules && category_type <= slit_unexp_grm_conflicts);

	m_special_item_type = category_type;
	ClearAppLevelFields();

	m_igrammar = igrammar;
	m_num_instances = num_instances;
}

void TLangViewSpecialLexItem::SetGrammarProps(WORD igrammar)
{
	m_special_item_type = slit_grammar;
	ClearAppLevelFields();

	m_igrammar = igrammar;
}

void TLangViewSpecialLexItem::SetRuleLabelProps(WORD igrammar, WORD irule)
{
	m_special_item_type = slit_rule_label;
	ClearAppLevelFields();

	m_igrammar = igrammar;
	m_irule = irule;
}

void TLangViewSpecialLexItem::SetRulePositionProps(WORD igrammar, WORD irule, WORD isym, WORD rule_action_sym)
{
	m_special_item_type = slit_rule_position;
	ClearAppLevelFields();

	// In fact rule symbol is stored in TFullRulePosition structure in the BYTE field.
	if (isym >= 0xFF)
		isym = 0xFFFF;

	m_igrammar = igrammar;
	m_irule = irule;
	m_isym = isym;
	m_action_value = rule_action_sym;
}

void TLangViewSpecialLexItem::SetXpctConflictProps(WORD igrammar, WORD ixpct)
{
	m_special_item_type = slit_xpct_conflict;
	ClearAppLevelFields();

	m_igrammar = igrammar;
	m_ixpct = ixpct;
}

void TLangViewSpecialLexItem::SetGrammarConflictProps(WORD igrammar, WORD iconflict)
{
	m_special_item_type = slit_grammar_conflict;
	ClearAppLevelFields();

	m_igrammar = igrammar;
	m_iconflict = iconflict;
}

void TLangViewSpecialLexItem::SetConflictingActionProps(WORD igrammar, WORD iconflict, WORD iaction, WORD parsing_action)
{
	m_special_item_type = slit_conflicting_action;
	ClearAppLevelFields();

	m_igrammar = igrammar;
	m_iconflict = iconflict;
	m_iaction = iaction;
	m_action_value = parsing_action;
}

void TLangViewSpecialLexItem::SetDerivationPathProps(WORD igrammar, WORD iconflict, WORD iaction, int ipath)
{
	m_special_item_type = slit_derivation_path;
	ClearAppLevelFields();

	m_igrammar = igrammar;
	m_iconflict = iconflict;
	m_iaction = iaction;
	m_ipath = ipath;
}

void TLangViewSpecialLexItem::SetSimpleObjectProps(TSpecialLexItemType simple_object_type)
{
	assert(simple_object_type == slit_center_button);

	m_special_item_type = simple_object_type;
	ClearAppLevelFields();
}

void TLangViewSpecialLexItem::SetEmptySymbolProps(TLexemaInfo &empty_lex)
{
	m_special_item_type = slit_empty_symbol;
	ClearAppLevelFields();

	assert(empty_lex.type == ltx_empty);
	m_empty_sym_empty_lex = empty_lex;
}

void TLangViewSpecialLexItem::SetMissingLinkProps(const wchar_t *link_name)
{
	m_special_item_type = slit_missing_link;
	ClearAppLevelFields();

	m_missing_link_name = (wchar_t*)link_name;
}

void TLangViewSpecialLexItem::SetCodeLocationProps(TSourceLocation &loc_info, TFileInfo &file_info)
{
	m_special_item_type = slit_code_location;
	ClearAppLevelFields();

	m_cloc_pos_info = loc_info;
	m_cloc_file_info = file_info;
}

wchar_t *TLangViewSpecialLexItem::GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode)
{
	if (m_special_item_type == slit_grammar_rules)
	{
		return(L"Rules");
	}
	else if (m_special_item_type == slit_parsing_states)
	{
		return(L"States");
	}
	else if (m_special_item_type == slit_xpct_conflicts)
	{
		return(L"XpctConflicts");
	}
	else if (m_special_item_type == slit_all_grammar_conflicts)
	{
		return(L"GrammarConflicts");
	}
	else if (m_special_item_type == slit_ambig_grm_conflicts)
	{
		return(L"AmbigConflicts");
	}
	else if (m_special_item_type == slit_assigned_grm_conflicts)
	{
		return(L"AssignedConflicts");
	}
	else if (m_special_item_type == slit_unexp_grm_conflicts)
	{
		return(L"UnexpectedConflicts");
	}
	else if (m_special_item_type == slit_grammar)
	{
		if (plc_mode == plc_mode_rect_lines)
		{
			swprintf(buff_40_chars, 40, L"Grammar %hd", m_igrammar);
			return(buff_40_chars);
		}

		return(L"Grm");
	}
	else if (m_special_item_type == slit_rule_label)
	{
		if (plc_mode == plc_mode_rect_lines)
		{
			swprintf(buff_40_chars, 40, L"R%hd", m_irule);
			return(buff_40_chars);
		}

		return(L"R");
	}
	else if (m_special_item_type == slit_rule_position)
	{
		if (m_irule == 0xFFFF && m_isym == 0xFFFF)
		{
			// This is special case of the axioma action symbol of the nested grammar.
			swprintf(buff_40_chars, 40, L"#%hd ", m_action_value);
		}
		else
		{
			// Regular case.
			swprintf(buff_40_chars, 40, L"%hd.%hd", m_irule, m_isym);
		}

		return(buff_40_chars);
	}
	else if (m_special_item_type == slit_xpct_conflict)
	{
		swprintf(buff_40_chars, 40, L"Xpct:%hd", m_ixpct);
		return(buff_40_chars);
	}
	else if (m_special_item_type == slit_grammar_conflict)
	{
		swprintf(buff_40_chars, 40, L"Conflict:%hd", m_iconflict);
		return(buff_40_chars);
	}
	else if (m_special_item_type == slit_conflicting_action)
	{
		swprintf(buff_40_chars, 40, L"Act:%hd", m_iaction);
		return(buff_40_chars);
	}
	else if (m_special_item_type == slit_derivation_path)
	{
		if (plc_mode == plc_mode_rect_lines)
		{
			swprintf(buff_40_chars, 40, L"Path:%d", m_ipath);
			return(buff_40_chars);
		}

		return(L"Path");
	}
	else if (m_special_item_type == slit_center_button)
	{
		return(L" ");
	}
	else if (m_special_item_type == slit_empty_symbol)
	{
		return(L"e");
	}
	else if (m_special_item_type == slit_missing_link)
	{
		return(m_missing_link_name);
	}
	else if (m_special_item_type == slit_code_location)
	{
		return(L"cLoc");
	}
	else
	{
		// Special item type is bogus.
		assert(FALSE);
		return(L"BadItemType");
	}
}

wchar_t *TLangViewSpecialLexItem::GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode)
{
	if (m_special_item_type >= slit_grammar_rules && m_special_item_type <= slit_unexp_grm_conflicts)
	{
		swprintf(buff_40_chars, 40, L"%d", m_num_instances);
		return(buff_40_chars);
	}
	else if (m_special_item_type == slit_grammar && plc_mode == plc_mode_concentric)
	{
		swprintf(buff_40_chars, 40, L"%hd", m_igrammar);
		return(buff_40_chars);
	}
	else if (m_special_item_type == slit_rule_label && plc_mode == plc_mode_concentric)
	{
		swprintf(buff_40_chars, 40, L"%hd", m_irule);
		return(buff_40_chars);
	}
	if (m_special_item_type == slit_rule_position)
	{
		if (m_irule == 0xFFFF && m_isym == 0xFFFF)
		{
			// This is special case of the axioma action symbol of the nested grammar.
			return(L"asym");
		}

		// Rule position (regular case).
		return(L"rpos");
	}
	else if (m_special_item_type == slit_conflicting_action)
	{
		return(TGrammar::GetShortParsingActionName(m_action_value, buff_40_chars));
	}
	else if (m_special_item_type == slit_derivation_path && plc_mode == plc_mode_concentric)
	{
		swprintf(buff_40_chars, 40, L"%d", m_ipath);
		return(buff_40_chars);
	}
	else if (m_special_item_type == slit_code_location)
	{
		return((wchar_t*)m_cloc_file_info.PrepareFileIdShort(buff_40_chars, 40));
	}
	else
	{
		// All other special item types either do not have anything in their symbol label
		// or this label should be empty in the current placing mode.
		return(NULL);
	}
}

void TLangViewSpecialLexItem::ClearAppLevelFields()
{
	m_igrammar = m_irule = m_isym = 0;
	m_ixpct = m_iconflict = m_iaction = 0;

	m_ipath = 0;
	m_num_instances = 0;
	m_action_value = 0xFFFF;

	m_empty_sym_empty_lex.Clear();
	m_missing_link_name = NULL;
	m_cloc_pos_info.Clear();
	m_cloc_file_info.Clear();
}

void TLangViewSpecialLexItem::CloneAppLevelFields(TLangViewSpecialLexItem &cloned_fields_recipient)
{
	cloned_fields_recipient.m_igrammar = m_igrammar;
	cloned_fields_recipient.m_irule = m_irule;
	cloned_fields_recipient.m_isym = m_isym;

	cloned_fields_recipient.m_ixpct = m_ixpct;
	cloned_fields_recipient.m_iconflict = m_iconflict;
	cloned_fields_recipient.m_iaction = m_iaction;
	cloned_fields_recipient.m_ipath = m_ipath;

	cloned_fields_recipient.m_num_instances = m_num_instances;
	cloned_fields_recipient.m_action_value = m_action_value;

	cloned_fields_recipient.m_empty_sym_empty_lex = m_empty_sym_empty_lex;
	cloned_fields_recipient.m_missing_link_name = m_missing_link_name;
	cloned_fields_recipient.m_cloc_pos_info = m_cloc_pos_info;
	cloned_fields_recipient.m_cloc_file_info = m_cloc_file_info;
}

// -------------------------------------------------------------------------------
//    =================  TLangViewVisualParserLexItem  ====================
// -------------------------------------------------------------------------------

wchar_t *TLangViewVisualParserLexItem::GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode)
{
	if (m_parser_item_type == vplit_program)
	{
		return(L"Program");
	}
	else if (m_parser_item_type == vplit_parser_stk_slot || m_parser_item_type == vplit_parser_inp_sym)
	{
		swprintf(buff_40_chars, 40, L"[%hd]", m_slot_index);
		return(buff_40_chars);
	}
	else if (m_parser_item_type == vplit_null_item)
	{
		return(L"NULL");
	}
	else
	{
		// Visual parser item type is bogus.
		assert(FALSE);
		return(L"BadVisualParserItemType");
	}
}

// --------------------------------------------------------------------------------
//    =================  TLangViewCppItemLexItem  ====================
// --------------------------------------------------------------------------------

bool TLangViewCppItemLexItem::PrepareClickEvent(TLangViewClickEvent &info)
{
	info.type = clet_cpp_object;
	info.cpp_item = m_cpp_item_ptr;
	return(TRUE);
}

wchar_t *TLangViewCppItemLexItem::GetMainLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, TTreeItemsPlacingMode plc_mode)
{
	if (m_cpp_item_ptr == NULL)
		return(L"null_cpp_item");

	switch (m_cpp_item_ptr->ItemType())
	{
		// Defns 1.
		case cxx_built_in_type:
		case cxx_bit_field_type:
		case cxx_cv_ptr_ref_type:
		case cxx_array_type:
		case cxx_data_type_alias:
		case cxx_enum_type:
		case cxx_enum_member:
		case cxx_struct_type:
		case cxx_data_field:

		// Defns 2.
		case cxx_function_type:
		case cxx_function_type_alias:
		case cxx_code_entry_point:
		case cxx_namespace:
		case cxx_namespace_alias:

		// Templates.
		case cxx_class_template:
		case cxx_class_template_spec:
		case cxx_class_template_inst:
		case cxx_func_template:
		case cxx_func_template_spec:
		case cxx_func_template_inst:
		case cxx_template_type_param:
		case cxx_template_template_param:

		// Unresolved objects.
		case cxx_unresolved_name_info:
		case cxx_unresolved_data_type:
		case cxx_unresolved_struct_type:
		case cxx_unresolved_data_field:
		case cxx_unresolved_code_entry_point:
		case cxx_unresolved_func_template:
		case cxx_unresolved_class_template:

		// Statements.
		case cxx_label:
		case cxx_case_label:
				{
					// Show the name of the object.
					assert(m_cpp_item_ptr->IsDefinition() == TRUE);
					TCppDefnBase *defn = (TCppDefnBase*)m_cpp_item_ptr;

					if (defn->DefnName() == NULL)
						return(L"<null_name>");
					else if (defn->DefnName()[0] == 0)
						return(L"<empty_name>");

					// The name is present.
					return((wchar_t*)defn->DefnName());
				}

		case cxx_while_stmt:
				{
					// Add extra spaces around the regular name. They are needed to avoid nasty Windows clipping.
					swprintf(buff_40_chars, 40, L" %s ", TCppItemHelper::GetItemTypeDisplayName(cxx_while_stmt));
					return(buff_40_chars);
				}

		case cxx_operand_expr:
				{
					// Pick up description for the most common types of the operands.
					TCppOperandExpr *oprnd = (TCppOperandExpr*)m_cpp_item_ptr;
					if (oprnd->m_oprnd_type == eopr_this)
						return(L"this");

					const wchar_t *description = oprnd->GetTextRepresentation(buff_40_chars, 40, TRUE);
					if (description != NULL)
						return((wchar_t*)description);

					// Show the name of the operand subtype.
					return((wchar_t*)TCppOperandExpr::GetOperandTypeEnumName(oprnd->m_oprnd_type));
				}

		case cxx_unary_expr:
		case cxx_binary_expr:
		case cxx_multiop_expr:
		case cxx_assignment_expr:
				{
					// Show the text representation of the operation code.
					TCppExpressionBase *expr = (TCppExpressionBase*)m_cpp_item_ptr;
					return((wchar_t*)TLexema::GetArithmOperationText(expr->m_operation));
				}
	}

	// Show the type of the object.
	return((wchar_t*)TCppItemHelper::GetItemTypeDisplayName(m_cpp_item_ptr->ItemType()));
}

wchar_t *TLangViewCppItemLexItem::GetSymbolLabel(TLexItemStyleInfo &style_info, wchar_t *buff_40_chars, short *extra_vert_offs, TTreeItemsPlacingMode plc_mode)
{
	if (m_cpp_item_ptr == NULL)
		return(L"n/a");
	else if (m_cpp_item_ptr->ItemType() == cxx_global_namespace)
		return(NULL);

	// The item is present. Show the id of the item as the symbol label.
	if (m_cpp_item_ptr->ItemId() < bltin_ids_base)
		swprintf(buff_40_chars, 40, L"%lu", m_cpp_item_ptr->ItemId());
	else swprintf(buff_40_chars, 40, L"Std+%lu", m_cpp_item_ptr->ItemId()-bltin_ids_base);

	return(buff_40_chars);
}

// --------------------------------------------------------------------------------
//    =====================  TLangViewTreeNode  ========================
// --------------------------------------------------------------------------------

TLangViewTreeNode::TLangViewTreeNode(TGenericTreeNode *owner, BYTE poly_inx)
		: TGenericTreeNode(owner, poly_inx)
{
	m_unique_node_id = 0;
	m_missing_chld = FALSE;

	// By default the upper link is visible and it is not selected.
	m_upper_link_visib = TRUE;
	m_upper_link_slct = FALSE;
	m_right_link_visib = FALSE;
	m_right_link_slct = FALSE;
}

void TLangViewTreeNode::ResetScreenItems(TList &screen_items_data_set,  bool reset_links)
{
	LangViewScreenItem().ResetScreenItems(screen_items_data_set);

	if (reset_links == TRUE)
	{
		if (m_tree_link_item.IsInList() == TRUE)
			screen_items_data_set.RemoveItem(m_tree_link_item);
		if (m_special_link.IsInList() == TRUE)
			screen_items_data_set.RemoveItem(m_special_link);
	}
}

void TLangViewTreeNode::SetSelectedStateEx(bool upper_link_visib, bool upper_link_slct, bool right_link_visib, bool right_link_slct)
{
	// Simply save all passed values.
	m_upper_link_visib = upper_link_visib;
	m_upper_link_slct  = upper_link_slct;
	m_right_link_visib = right_link_visib;
	m_right_link_slct  = right_link_slct;
}

void TLangViewTreeNode::SetSelectedStateEx2(int ulv, int uls, int rlv, int rls)
{
	m_upper_link_visib = (ulv < 0) ? m_upper_link_visib : ((ulv > 0) ? TRUE : FALSE);
	m_upper_link_slct = (uls < 0) ? m_upper_link_slct : ((uls > 0) ? TRUE : FALSE);
	m_right_link_visib = (rlv < 0) ? m_right_link_visib : ((rlv > 0) ? TRUE : FALSE);
	m_right_link_slct = (rls < 0) ? m_right_link_slct : ((rls > 0) ? TRUE : FALSE);
}

void TLangViewTreeNode::CloneGenericFields(TLangViewTreeNode *cloned_fields_recipient)
{
	if (LangViewScreenItem().GetOwner() == &LangViewScreenItem())
	{
		// Current instance is owned by itself. Propagate this to the recipient.
		cloned_fields_recipient->SetLexItemClickOwner(LangViewScreenItem().GetOwnerData());
	}

	cloned_fields_recipient->m_unique_node_id = m_unique_node_id;
	cloned_fields_recipient->m_missing_chld = m_missing_chld;
}

void TLangViewTreeNode::CloneChildren(TLangViewTreeNode *cloned_children_recipient)
{
	// Iterate children and clone them.
	for (TListIter<TLangViewTreeNode> iter(m_children); iter; ++iter)
	{
		TLangViewTreeNode *cloned_chld = iter.CurrItem().Clone(TRUE);
		if (cloned_chld != NULL)
		{
			cloned_children_recipient->AdoptChild(cloned_chld);
		}
		else
		{
			cloned_children_recipient->m_missing_chld = TRUE;
		}
	}
}

void TLangViewTreeNode::CheckPrepareConcentricRightLink(TLangViewLexItem &item)
{
	if (m_right_link_visib == TRUE)
	{
		// Prepare additional link to the next sibling.
		long px, py;
		GetPosition(px, py);
		TGenericTreeNode *next_sibl = GetNextSibling(FALSE);
		assert(next_sibl != NULL);

		long nsib_px, nsib_py;
		next_sibl->GetPosition(nsib_px, nsib_py);

		// Right link should connect the centers of the objects.
		m_special_link.SetStyle(item.Style()->GetRightLinkStyle(m_right_link_slct));
		m_special_link.SetJctPos(0, px, py);
		m_special_link.SetJctPos(1, nsib_px, nsib_py, TRUE);
	}
}

void TLangViewTreeNode::CheckPrepareRectLinesRightLink(TLangViewLexItem &item)
{
	if (m_right_link_visib == TRUE)
	{
		// Prepare additional link to the next sibling.
		long link_y = item.GetBoundingRectY()+item.GetHeight()/2+5;
		TGenericTreeNode *next_sibl = GetNextSibling(FALSE);
		assert(next_sibl != NULL);

		long nsib_px, nsib_py;
		next_sibl->GetPosition(nsib_px, nsib_py);
		TLineStyle *link_style = item.Style()->GetRightLinkStyle(m_right_link_slct);
		m_special_link.SetStyle(link_style);

		// Right link should connect the sides of the objects.
		long ext_offs = (link_style->m_props.line_width > 1) ? 1 : 0;
		m_special_link.SetJctPos(0, item.GetBoundingRect()->right-ext_offs-2, link_y);
		m_special_link.SetJctPos(1, nsib_px+1+ext_offs, link_y, TRUE);
	}
}

// -------------------------------------------------------------------------------
//    ==================  TLangViewLexemaTreeNode  =====================
// -------------------------------------------------------------------------------

TLangViewTreeNode *TLangViewLexemaTreeNode::Clone(bool clone_children)
{
	TLangViewLexemaTreeNode *node = NULL;
	TLangViewLexemaLexItem &lc_item = m_lang_view_screen_item;
	if (m_lang_view_screen_item.m_grammar_object == TRUE)
	{
		// Create lexema object as part of the grammar structure view.
		TLexemaInfo &lex_info = m_lang_view_screen_item.m_lex_info;
		TLexSubtype lex_subt = { (lex_info.type == ltx_keyword) ? (DWORD)lex_info.num_value : lex_info.subtype };
		node = new TLangViewLexemaTreeNode(NULL, m_poly_inx, lc_item.m_symbol, lex_info.type, lc_item.m_check_subt, lex_subt.subtype);
	}
	else
	{
		// Create lexema object as part of the source code parsing.
		node = new TLangViewLexemaTreeNode(NULL, m_poly_inx, lc_item.m_symbol, lc_item.m_lex_info, lc_item.m_shift_dest_state, lc_item.m_spec_origin);
	}

	// Check allocation results.
	if (node == NULL)
		return(NULL);

	// Duplicate generic fields and take care of the children if any.
	CloneGenericFields(node);
	if (clone_children == TRUE)
		CloneChildren(node);

	// Success.
	return(node);
}

// ------------------------------------------------------------------------------
//    ===============  TLangViewMidLevObjectTreeNode  ====================
// ------------------------------------------------------------------------------

TLangViewTreeNode *TLangViewMidLevObjectTreeNode::Clone(bool clone_children)
{
	// Create the frame. This will also take care of the app level fields.
	TLangViewMidLevObjectTreeNode *node = NULL;
	switch (m_lang_view_screen_item.m_mlev_obj_type)
	{
		case mlit_compilation_info:
				node = new TLangViewMidLevObjectTreeNode(NULL, m_poly_inx, m_lang_view_screen_item.m_pars_info);
				break;

		case mlit_file_info:
				node = new TLangViewMidLevObjectTreeNode(NULL, m_poly_inx, m_lang_view_screen_item.m_file_info);
				break;

		case mlit_parsing_event:
				node = new TLangViewMidLevObjectTreeNode(NULL, m_poly_inx, m_lang_view_screen_item.m_event);
				break;

		default:
			assert(FALSE);
			break;
	}

	// Check allocation results.
	if (node == NULL)
		return(NULL);

	// Duplicate generic fields and take care of the children if any.
	CloneGenericFields(node);
	if (clone_children == TRUE)
		CloneChildren(node);

	// Success.
	return(node);
}

// --------------------------------------------------------------------------------
//    =================  TLangViewNonTerminalTreeNode  ====================
// --------------------------------------------------------------------------------

TLangViewTreeNode *TLangViewNonTerminalTreeNode::Clone(bool clone_children)
{
	TLangViewNonTerminalTreeNode *node = NULL;
	if (m_lang_view_screen_item.m_grammar_object == TRUE)
	{
		// Create non term object as part of the grammar structure view.
		node = new TLangViewNonTerminalTreeNode(NULL, m_poly_inx, m_lang_view_screen_item.m_symbol);
	}
	else
	{
		// Create non term object as part of the source code parsing.
		TLangViewNonTerminalLexItem &item = m_lang_view_screen_item;
		node = new TLangViewNonTerminalTreeNode(NULL, m_poly_inx, item.m_symbol, item.m_rule_index, item.m_origin, item.m_spec_origin);
	}

	// Check allocation results.
	if (node == NULL)
		return(NULL);

	// Duplicate generic fields and take care of the children if any.
	CloneGenericFields(node);
	if (clone_children == TRUE)
		CloneChildren(node);

	// Success.
	return(node);
}

// ------------------------------------------------------------------------------
//    =================  TLangViewSpecialTreeNode  ======================
// ------------------------------------------------------------------------------

TLangViewTreeNode *TLangViewSpecialTreeNode::Clone(bool clone_children)
{
	// Create the frame.
	TLangViewSpecialTreeNode *node = new TLangViewSpecialTreeNode(NULL, m_poly_inx, m_lang_view_screen_item.m_special_item_type);
	if (node == NULL)
		return(NULL);

	// Duplicate generic and app level fields.
	CloneGenericFields(node);
	m_lang_view_screen_item.CloneAppLevelFields(node->m_lang_view_screen_item);

	// Take care of the children if any.
	if (clone_children == TRUE)
		CloneChildren(node);

	// Success.
	return(node);
}

// --------------------------------------------------------------------------------
//    =================  TLangViewVisualParserTreeNode  ====================
// --------------------------------------------------------------------------------

TLangViewTreeNode *TLangViewVisualParserTreeNode::Clone(bool clone_children)
{
	// Create the frame.
	TLangViewVisualParserTreeNode *node = new TLangViewVisualParserTreeNode(NULL, m_poly_inx);
	if (node == NULL)
		return(NULL);

	// Duplicate generic fields.
	CloneGenericFields(node);

	// Duplicate the app level fields.
	node->SetParserNodeType(GetParserNodeType());
	node->SetParserSlotIndex(GetParserSlotIndex());

	// Take care of the children if any.
	if (clone_children == TRUE)
		CloneChildren(node);

	// Success.
	return(node);
}

// ------------------------------------------------------------------------------
//    ==============  TLangViewCppItemTreeNode  ==================
// ------------------------------------------------------------------------------

TLangViewTreeNode *TLangViewCppItemTreeNode::Clone(bool clone_children)
{
	// Create the frame.
	TLangViewCppItemTreeNode *node = new TLangViewCppItemTreeNode(NULL, m_poly_inx, m_lang_view_screen_item.m_cpp_item_ptr);
	if (node == NULL)
		return(NULL);

	// Duplicate generic fields.
	CloneGenericFields(node);

	// Take care of the children if any.
	if (clone_children == TRUE)
		CloneChildren(node);

	// Success.
	return(node);
}

TLangViewCppItemTreeNode *TLangViewCppItemTreeNode::FindNodeByIdHier(ID cpp_item_id)
{
	if (GetCppItem() != NULL && GetCppItem()->ItemId() == cpp_item_id)
		return(this);

	for (TGenericTreeNodeDirectChildrenIter<TLangViewTreeNode> iter(this); iter; ++iter)
	{
		if (iter.CurrItem().GetItemType() == litp_cpp_database_item)
		{
			TLangViewCppItemTreeNode *node = ((TLangViewCppItemTreeNode&)iter.CurrItem()).FindNodeByIdHier(cpp_item_id);
			if (node != NULL)
				return(node);
		}
	}

	return(NULL);
}


