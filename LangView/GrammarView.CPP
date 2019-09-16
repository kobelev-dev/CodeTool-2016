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
#include  "ScreenItems/GeometryHelpers.H"
#include  "ScreenItems/ScreenItemsXml.H"
#include  "TextScan/GrammarAnalysisXml.H"
#include  "LangView/GrammarView.H"
#include  "LangView/ScanProcHelpers.H"
#include  "LangView/LoadGrammarHelpers.H"
#include  "LangView/GrammarAnalysisSupp.H"
#include  "LangView/SimpleBrowser.H"

// ----------------------------------------------------------------------
//	===============  TLangViewRulesGridDialog  ==================
// ----------------------------------------------------------------------

int	g_RulesView_DiffersTreeForConflictingAction_MinLayerSteps[] =
{
	58, 49, 0					// Zero terminated lists of min radius steps in pixels.
};

// ----------------------------------------------------------------------

#define	GRVIEW_NO_TEXT_FRAME	cs_transparent, fcs_none

static TTextStyleProps g_LangViewRulesGrid_RuleIndex =
{
	NULL,
	TAHOMA, 9, FALSE,						// Font params.
	RGB(0, 0, 0), cs_transparent,			// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
	1, -1, 0,								// Vertical adjustments.
	4, 3									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_KeyRuleIndex =
{
	NULL,
	TAHOMA, 9, FALSE,						// Font params.
	RGB(0, 0, 0), RGB(254, 224, 205),		// Foreground and bkgr.
	RGB(224, 192, 128), fcs_1pix_excl_tip,
	cs_transparent, 0,						// No shadow.
	7, -1, 6,								// Vertical adjustments.
	12, 11								// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_BoldIndex =
{
	NULL,
	TAHOMA, 9, TRUE,						// Font params.
	RGB(0, 0, 0), cs_transparent,			// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
	1, -1, 0,								// Vertical adjustments.
	4, 3									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_SectTitle =
{
	NULL,
	VERDANA, 9, FALSE,					// Font params.
	RGB(0, 0, 0), cs_transparent,			// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
	1, -1, 0,								// Vertical adjustments.
	4, 3									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_RowDescr =
{
	NULL,
	ARIAL, 8, FALSE,						// Font params.
	RGB(0, 0, 0), cs_transparent,			// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
	1, 0, 0,								// Vertical adjustments.
	4, 3									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_RowComment =
{
	NULL,
	ARIAL, 8, FALSE,						// Font params.
	RGB(96, 96, 96), RGB(255, 255, 255),		// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
	1, 0, 1,								// Vertical adjustments.
	4, 3									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_AnalysisWarning =
{
	NULL,
	L"Arial Black", 10, FALSE,				// Font params.
	RGB(212, 212, 212), cs_transparent,		// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	RGB(160, 160, 160), 1,					// Shadow is present.
	1, 0, 0,								// Vertical adjustments.
	3, 2									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_AnalysisError =
{
	NULL,
	L"Arial", 10, TRUE,						// Font params.
	RGB(228, 0, 0), cs_transparent,			// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	RGB(202, 202, 202), 1,					// Shadow.
	1, 0, 0,								// Vertical adjustments.
	3, 2									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_ActionMissing =
{
	NULL,
	SMALL_FONTS, 7, FALSE,				// Font params.
	RGB(180, 180, 180), cs_transparent,		// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
	1, -2, 0,								// Vertical adjustments.
	4, 6									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_ActionPresent =
{
	NULL,
	L"MS Serif", 7, FALSE,					// Font params.
	RGB(0, 0, 0), cs_transparent,			// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
	1, -1, 0,								// Vertical adjustments.
	4, 5									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_SmallComment =
{
	NULL,
	SMALL_FONTS, 7, FALSE,				// Font params.
	RGB(80, 80, 80), cs_transparent,			// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
	1, 0, 0,								// Vertical adjustments.
	4, 3									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_ParsSttLabel =
{
	NULL,
	VERDANA, 7, FALSE,					// Font params.
	RGB(0, 0, 0), RGB(254, 224, 205),		// Foreground and bkgr.
	RGB(224, 192, 128), fcs_1pix_excl_tip,
	cs_transparent, 0,						// No shadow.
	-1, 0, 0,								// Vertical adjustments.
	4, 4									// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_NoObjectsMessage =
{
	NULL,
	VERDANA, 12, TRUE,					// Font params.
	RGB(0, 0, 0), RGB(254, 224, 205),		// Foreground and bkgr.
	RGB(253, 203, 170), fcs_1pix_excl_tip,	// One pixel frame.
	RGB(253, 177, 128), 1,					// Shadow.
	10, 0, 11,								// Vertical adjustments.
	40, 36								// Horz extensions.
};

static TTextStyleProps g_LangViewRulesGrid_ClipboardImageTitle =
{
	NULL,
	L"Consolas", 14, TRUE,					// Font params.
	RGB(96, 96, 96), cs_transparent,			// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
};

static TTextStyleProps g_LangViewRulesGrid_ClipboardImageFooter =
{
	NULL,
	L"Consolas", 7, FALSE,					// Font params.
	RGB(225, 225, 225), cs_transparent,		// Foreground and bkgr.
	GRVIEW_NO_TEXT_FRAME,
	cs_transparent, 0,						// No shadow.
};

static TTextStyleSymbolAdjustInfo g_LangViewRulesGrid_AnalysisWarning_CharacterAdjustInfo[] =
{
	{ L'E', 0, 0, 1 }, { L'N', 0, 0, 1 }, { L'a', 0, 0, 1 }, { L'd', 0, 0, 1 }, { L'g', 0, 0, 1 }, { L'm', 0, -1, 0 }, { L'r', 0, 0, 1 }, { L't', 0, 0, 1 }, { 0, 0, 0, 0 },
};

enum TRulesViewCtxMenuIds
{
	rvcm_base = 400,

	rvcm_hd1_zi,		rvcm_hd1_zo,		rvcm_hd1_zob,		rvcm_hd1_rz,
	rvcm_hd2_cnct,		rvcm_hd2_rcp,

	rvcm_m1_igrc,
	rvcm_m1a_ssv,		rvcm_m1a_salp,
	rvcm_m1b_so1,		rvcm_m1b_so2,		rvcm_m1b_so3,		rvcm_m1b_so4,		rvcm_m1b_sstc,
	rvcm_m1c_sts,		rvcm_m1c_dyn1,	rvcm_m1c_dyn2,

	rvcm_m2,			rvcm_m2_stm2,		rvcm_m2_stm3,
	rvcm_m3a_sdr,		rvcm_m3a_ssu,		rvcm_m3a_hsm,		rvcm_m3a_dyn1,	rvcm_m3a_dyn2,
	rvcm_m3b_ipon,
	rvcm_m4_sxn,

	rvcm_m5_scah,		rvcm_m5_sdcr,
	rvcm_m5b_dinr,		rvcm_m5c_wan,

	rvcm_m6_skr,		rvcm_m6_mfd,		rvcm_m6_sps,		rvcm_m6_sfs,
	rvcm_m7_sdpp,		rvcm_m7_ssli,
	rvcm_m8_sdpd,		rvcm_m8_sspb,

	rvcm_ft1_pccu,
	rvcm_ft2_tsgn,		rvcm_ft2_htnt,
	rvcm_ft3_gobk,		rvcm_ft3_ctc,
};

#define	CTX_MENU_NAME(x)		g_RightBottomCornerContextMenu##x
#define	CTX_MLEN(x)			sizeof(CTX_MENU_NAME(x))/sizeof(TMenuItemInfo)

//
//  Stnd beginning for all dialog modes.
//
static TMenuItemInfo g_RightBottomContextMenuHeader1[4] =
{
	{ 0, rvcm_hd1_zi,		L"Zoom In"							},	// 0 (-4)
	{ 0, rvcm_hd1_zo,		L"Zoom Out"							},	// 1 (-3)
	{ 0, rvcm_hd1_zob,	L"Zoom Out (bigger step)"				},	// 2 (-2)
	{ 0, rvcm_hd1_rz,		L"Reset Zoom"						},	// 3 (-1)
};

//
//  Stnd beginning supp for tree based modes.
//
static TMenuItemInfo g_RightBottomContextMenuHeader2[3] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-3)
	{ 0, rvcm_hd2_cnct,	L"Set concentric placing mode"			},	// 1 (-2)
	{ 0, rvcm_hd2_rcp,	L"Set rect lines placing mode"			},	// 2 (-1)
};

//
//  grdm_grms_overview
//
static TMenuItemInfo g_RightBottomCornerContextMenu1[2] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-2)
	{ 0, rvcm_m1_igrc,	L"Show individual grammar conflicts"		},	// 1 (-1)
};

//
//  grdm_terminal_symbols
//
static TMenuItemInfo g_RightBottomCornerContextMenu1a[3] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-3)
	{ 0, rvcm_m1a_ssv,	L"Sort by symbol value"					},	// 1 (-2)
	{ 0, rvcm_m1a_salp,	L"Sort alphabetically"					},	// 2 (-1)
};

//
//  grdm_non_terminals
//
static TMenuItemInfo g_RightBottomCornerContextMenu1b[7] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-7)
	{ 0, rvcm_m1b_so1,	L"Sort by symbol value"					},	// 1 (-6)
	{ 0, rvcm_m1b_so2,	L"Sort alphabetically"					},	// 2 (-5)
	{ 0, rvcm_m1b_so3,	L"Sort alphabetically special"				},	// 3 (-4)
	{ 0, rvcm_m1b_so4,	L"Sort by rule sections"					},	// 4 (-3)
	{ MF_SEPARATOR, -1, NULL									},	// 5 (-2)
	{ 0, rvcm_m1b_sstc,	L"Show possible starter symbols"			},	// 6 (-1)
};

//
//  grdm_non_term_depends
//
static TMenuItemInfo g_RightBottomCornerContextMenu1c[4] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-4)
	{ 0, rvcm_m1c_sts,	L"Show terminal symbols"				},	// 1 (-3)
	{ 0, rvcm_m1c_dyn1,	L"== Non term counts (dynamic1) =="		},	// 2 (-2)
	{ 0, rvcm_m1c_dyn2,	L"== Non term counts (dynamic2) =="		},	// 3 (-1)
};

//
//  Rules view and states view in single parsing state mode.
//  grdm_grammar_rules || grdm_parsing_states && m_single_obj_index != -1
//
static TMenuItemInfo g_RightBottomCornerContextMenu2[4] =
{
	{ MF_SEPARATOR, -1, NULL															},	// 0 (-4)
	{ 0, rvcm_m2+pstt_trnv_none,				L"No parsing state trainsitions"				},	// 1 (-3)
	{ 0, rvcm_m2+pstt_trnv_simple,				L"Show parsing state trainsitions"				},	// 2 (-2)
	{ 0, rvcm_m2+pstt_trnv_show_act_syms,		L"Show transitions and rule action symbols"		},	// 3 (-1)
};

//
//  Symbol use report.
//  grdm_grammar_rules && (0 <= m_single_obj_index < actb_reduce)
//
static TMenuItemInfo g_RightBottomCornerContextMenu3a[6] =
{
	{ MF_SEPARATOR, -1, NULL											},	// 0 (-6)
	{ 0, rvcm_m3a_sdr,	L"Show definition rules"							},	// 1 (-5)
	{ 0, rvcm_m3a_ssu,	L"Show symbol uses"							},	// 2 (-4)
	{ 0, rvcm_m3a_hsm,	L"Highlight symbol"								},	// 3 (-3)
	{ 0, rvcm_m3a_dyn1,	L"== Number of definition rules (dynamic1) =="		},	// 4 (-2)
	{ 0, rvcm_m3a_dyn2,	L"== Num rules where sym is used (dynam2) =="	},	// 5 (-1)
};

//
//  grdm_parsing_states
//
static TMenuItemInfo g_RightBottomCornerContextMenu3b[2] =
{
	{ MF_SEPARATOR, -1, NULL											},	// 0 (-2)
	{ 0, rvcm_m3b_ipon,	L"Show only intermediate and final rule locations"	},	// 1 (-1)
};

//
//  grdm_xpct_conflicts || grdm_grammar_conflicts || grdm_key_rules_conflict ||grdm_dplist_symbols_xpct || grdm_dplist_symbols_conflict
//
static TMenuItemInfo g_RightBottomCornerContextMenu4[2] =
{
	{ MF_SEPARATOR, -1, NULL											},	// 0 (-2)
	{ 0, rvcm_m4_sxn,		L"Show expected conflict name"					},	// 1 (-1)
};

//
//  grdm_xpct_conflicts || grdm_grammar_conflicts
//
//  This menu controls showing/not showing action headers in the main xpct and grammar conflicts view only.
//  Action headers are also displayed in 2 other cases. They are displayed there all the times.
//
static TMenuItemInfo g_RightBottomCornerContextMenu5[3] =
{
	{ MF_SEPARATOR, -1, NULL											},	// 0 (-4)
	{ 0, rvcm_m5_scah,	L"Show conflicting action headers"				},	// 1 (-3)
	{ 0, rvcm_m5_sdcr,	L"Show directly conflicting rules"					},	// 2 (-2)
};

//
//  grdm_xpct_conflicts && m_single_obj_index < 0
//
static TMenuItemInfo g_RightBottomCornerContextMenu5b[1] =
{
	{ 0, rvcm_m5b_dinr,	L"== List of directly involved rules (dynamic) =="	},	// 0 (-1)
};

//
//  grdm_grammar_conflicts && m_single_obj_index < 0
//
static TMenuItemInfo g_RightBottomCornerContextMenu5c[2] =
{
	{ MF_SEPARATOR, -1, NULL											},	// 0 (-2)
	{ 0, rvcm_m5c_wan,	L"Show only those conflicts that have analysis"		},	// 1 (-1)
};

//
//  grdm_key_rules_xpct || grdm_key_rules_conflict
//
static TMenuItemInfo g_RightBottomCornerContextMenu6[5] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-5)
	{ 0, rvcm_m6_skr,		L"Show key conflicting rules"				},	// 1 (-4)
	{ 0, rvcm_m6_mfd,	L"Merge seqs from different key rules"		},	// 2 (-3)
	{ 0, rvcm_m6_sps,		L"Show predecessor sequences"			},	// 3 (-2)
	{ 0, rvcm_m6_sfs,		L"Show follower sequences"				},	// 4 (-1)
};

//
//  grdm_deriv_path_steps
//
static TMenuItemInfo g_RightBottomCornerContextMenu7[3] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-3)
	{ 0, rvcm_m7_sdpp,	L"Show derivation path properties"		},	// 1 (-2)
	{ 0, rvcm_m7_ssli,		L"Show path symbols as lex items"		},	// 2 (-1)
};

//
//  grdm_deriv_path_steps
//
static TMenuItemInfo g_RightBottomCornerContextMenu8[3] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-3)
	{ 0, rvcm_m8_sdpd,	L"Show derivation path details"			},	// 1 (-2)
	{ 0, rvcm_m8_sspb,	L"Start showing path from the bottom"		},	// 2 (-1)
};

//
//  General menu footer for viewers that can promote their contents up.
//
static TMenuItemInfo g_RightBottomContextMenuFooter1[2] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-2)
	{ 0, rvcm_ft1_pccu,	L"Promote current contents up"			},	// 1 (-1)
};

//
//  General menu footer for all dialog modes except terminal symbols list and non terminals list.
//
static TMenuItemInfo g_RightBottomContextMenuFooter2[3] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-3)
	{ 0, rvcm_ft2_tsgn,	L"Show terminal symbol grammar names"	},	// 1 (-2)
	{ 0, rvcm_ft2_htnt,		L"Highlight transparent non terminals"		},	// 2 (-1)
};

//
//  General menu footer for all dialog modes.
//
static TMenuItemInfo g_RightBottomContextMenuFooter3[4] =
{
	{ MF_SEPARATOR, -1, NULL									},	// 0 (-4)
	{ 0, rvcm_ft3_gobk,	L"Back to previous view"				},	// 1 (-3)
	{ 0, rvcm_ft3_ctc,		L"Copy to clipboard as Bitmap"			},	// 2 (-2)
	{ 0, 0, NULL },												// 3 (-1)
};

wchar_t		*TLangViewRulesGridDialog::g_StylesProfileName = L"RulesGridStyles.XML";
				// This is short file name. Profile is picked up from the directory of the executable.

TDateTime	TLangViewRulesGridDialog::g_LatestScannedProfileDate = -1;
				// When value of this field is -1, this corresponds to the "file is missing" case. This also means
				// that when profile is not present at all, the error messages will be never emitted.

bool			TLangViewRulesGridDialog::g_need_image_origin_footer = FALSE;
				// Clipboard image generation props.

TLangViewRulesGridDialog::TLangViewRulesGridDialog(TMidLevStoreReader *db_link)
				: TLangViewChildBrowser(db_link, cbrt_grbr_rules_grid, IDD_LANGV_GRMV_RULES_GRID),
				m_tree_items_root(NULL, 0, slit_num_types), m_aux_upper_tree_root(NULL, 0, slit_num_types),
				m_local_console(NULL, L" Grammar View ")
{
	// Layout related variables.
	m_initial_main_view_top = m_main_view_off_bottom = 0;

	// Common fields for all modes.
	m_show_dialog_mode = FALSE;
	m_show_grammar_name = FALSE;

	m_dialog_mode = grdm_none;
	StoreMajorTaskParams(grdm_none, NULL);
	m_step_back_proc_flag = FALSE;
	ResetSecondaryFlags();

	// Make main grid and tree items root objects clickable.
	m_rules_grid.GetGrid().SetOwner(&m_rules_grid);
	m_tree_items_root.SetLexItemClickOwner();
	m_aux_upper_tree_root.SetLexItemClickOwner();

	// Make symbols table row bkgrs clickable.
	m_symbols_table_grid.SetOwner(&m_symbols_table_grid);

	// Initially there should be no selection in the symbol lists.
	m_grm_symbols_list_frgr_slct = 0xFFFF;
	m_non_terms_list_bkgr_slct = NULL;

	// Misc fields.
	m_sym_rprt_defn_rules = 0;
	m_sym_rprt_use_rules = 0;
	m_grm_view_labels_owner = 0x0F0F0F0F;
	m_frgr_lex_items_owner = 0xF0F0F0F0;

	// Status panel related fields.
	m_local_console.SetDialogParams(this);
	m_local_console.SetShowRunTimeErrorsAsPopups(FALSE);
}

LRESULT TLangViewRulesGridDialog::OnInitDialog(HWND hDefFocusControl)
{
	RECT rc_client, rc_main_view;
	::GetClientRect(m_hDlg, &rc_client);
	AdjustControlRightSide(IDC_RULGRID_SCR_ITEMS_VIEW);
	GetControlRect(IDC_RULGRID_SCR_ITEMS_VIEW, rc_main_view);
	m_initial_main_view_top = rc_main_view.top;
	m_main_view_off_bottom = rc_client.bottom-rc_main_view.bottom;

	SetControlFont(IDC_RULGRID_CURR_VIEW_TITLE, VERDANA, 9, TRUE);
	SetControlColor(IDC_RULGRID_CURR_VIEW_TITLE, RGB(0, 0, 96));
	SetControlBkgrBrush(IDC_RULGRID_CURR_VIEW_TITLE, ::GetSysColorBrush(COLOR_3DFACE), FALSE);

	m_local_console.SetParentWindow(m_hDlg);
	SetUpdateControlsLayout();
	SetMinTrackingSize(50, 44);

	HDC hDC = ::GetDC(m_hDlg);
	if (hDC == NULL)
		return(-1);

	//
	// Setup bkgr colors.
	//

	m_dark_delim_style.Setup(RGB(0, 0, 0));
	m_semi_light_delim_style.Setup(RGB(212, 212, 212));
	m_light_delim_style.Setup(RGB(220, 220, 220));
	m_symbols_vert_delim_style.Setup(RGB(210, 210, 255));
	m_light_syms_vert_delim_style.Setup(RGB(252, 252, 255));

	m_grammar_rules_hdr_style.Setup(RGB(176, 211, 255));
	m_parsing_states_hdr_style.Setup(RGB(254, 242, 205));

	m_xpct_conflict_hdr_style.Setup(RGB(215, 220, 209));
	m_grammar_cfct_hdr_style.Setup(RGB(254, 224, 205));
	m_cfcting_action_hdr_style.Setup(RGB(255, 247, 240));
	m_xpct_actions_map_hdr_style.Setup(RGB(250, 242, 222));

	m_derivation_path_hdr_style.Setup(RGB(55, 221, 204));
	m_symbol_use_rprt_hdr_style.Setup(RGB(211, 176, 255));

	m_predec_flwr_seqs_bkgr_style.Setup(RGB(252, 252, 252));

	m_non_term_clmn_style.Setup(RGB(233, 248, 235));
	m_pars_stt_align_clmn_style.Setup(RGB(241, 171, 24));
	m_pars_stt_align_clmn_style.SetupFrameProps(RGB(192, 134, 12), 1);
	m_conflict_align_clmn_style.Setup(RGB(209, 156, 116));
	m_conflict_align_clmn_style.SetupFrameProps(RGB(172, 132, 73), 1);
	m_derv_path_align_clmn_style.Setup(RGB(55, 160, 154));
	m_derv_path_align_clmn_style.SetupFrameProps(RGB(28, 80, 78), 1);

	m_extern_non_term_slct_style.Setup(cs_transparent);
	m_extern_non_term_slct_style.SetupFrameProps(RGB(28, 80, 78), 2, PS_DOT);
	m_recurs_non_term_slct_style.Setup(RGB(181, 220, 249));
	m_recurs_non_term_slct_style.SetupFrameProps(RGB(52, 101, 190), 2);

	//
	// Setup text styles.
	//

	m_rule_index_text_style.Setup(g_LangViewRulesGrid_RuleIndex);
	m_key_rule_index_text_style.Setup(g_LangViewRulesGrid_KeyRuleIndex);
	m_bold_index_text_style.Setup(g_LangViewRulesGrid_BoldIndex);
	m_sect_title_text_style.Setup(g_LangViewRulesGrid_SectTitle);

	m_row_descr_text_style.Setup(g_LangViewRulesGrid_RowDescr);
	m_row_comment_text_style.Setup(g_LangViewRulesGrid_RowComment);
	m_analysis_warning_text_style.Setup(g_LangViewRulesGrid_AnalysisWarning, g_LangViewRulesGrid_AnalysisWarning_CharacterAdjustInfo);
	m_analysis_error_text_style.Setup(g_LangViewRulesGrid_AnalysisError);
	m_action_missing_text_style.Setup(g_LangViewRulesGrid_ActionMissing);
	m_action_present_text_style.Setup(g_LangViewRulesGrid_ActionPresent);
	m_small_comment_text_style.Setup(g_LangViewRulesGrid_SmallComment);

	m_pars_stt_label_text_style.Setup(g_LangViewRulesGrid_ParsSttLabel);
	m_no_objects_text_style.Setup(g_LangViewRulesGrid_NoObjectsMessage);

	m_clipboard_image_title_style.Setup(g_LangViewRulesGrid_ClipboardImageTitle);
	m_clipboard_image_footer_style.Setup(g_LangViewRulesGrid_ClipboardImageFooter);

	//
	// Setup line styles.
	//

	m_pars_stt_link_style.Setup(RGB(203, 196, 210));
	m_action_syms_link_style.Setup(RGB(230, 226, 233));

	// Try to load styles from the profile and finalize the styles.
	LoadStylesProfile(FALSE);
	PrepareStylesForDeviceContext(hDC);

	// Prepare the screen items viewer. All dialog modes depend on it.
	m_view_helper.SetWindow(::GetDlgItem(m_hDlg, IDC_RULGRID_SCR_ITEMS_VIEW));
	m_view_helper.SetStndScrollableProps();

	// Set focus and return.
	::ReleaseDC(m_hDlg, hDC);
	SetFocusControl(IDC_RULGRID_SCR_ITEMS_VIEW);
	return(FALSE);
}

void TLangViewRulesGridDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	if (controlId == IDC_RULGRID_SCR_ITEMS_VIEW)
	{
		// Notification came from the main screen items viewer.
		TScreenItemsViewMouseNotify *mouse_ntf = (TScreenItemsViewMouseNotify*)hWndControl;
		switch (notificationCode)
		{
			case xiv_left_btn_down:
					{
						// Save color of the clicked pixel in the color picker.
						m_view_helper.SaveClickedPointColor(mouse_ntf);

						// Button was pressed in the screen items area.
						if (mouse_ntf->object != NULL)
						{
							// Clicked object is present.
							ProcessMouseClick((TScreenItemsViewNtfCode)notificationCode, mouse_ntf);
						}
						else
						{
							// The background of the viewer was clicked. Propagate this event to the big browser.
							m_owner->RemoveSelection(this);
						}
					}
					break;

			case xiv_left_btn_dblclk:
					{
						//
						//  Note on double click implementation in Windows. If fact, when the mouse is clicked for the first time,
						//  Windows places event in the message queue without wating for anything. This results in immediate processing
						//  of this event on the app layer. After a while Windows detects second click and places second event into
						//  the queue. At this point Windows is not able to detect if the first event is already processed or not yet.
						//
						//  If processing of the second click requires considerable time and RotateMessageLoop() is called in the middle
						//  this WILL result in recursive call of the app layer function.
						//
						if (mouse_ntf->object != NULL)
						{
							// Clicked object is present.
							ProcessMouseClick((TScreenItemsViewNtfCode)notificationCode, mouse_ntf);
						}
						else if (m_owner->IsGrammarBrowser() == TRUE)
						{
							TLangViewGrammarBrowser *casted_frame = (TLangViewGrammarBrowser*)m_owner;
							if (casted_frame->CanPromoteChildViewer(this) == TRUE)
							{
								// There is no clicked object and contents of the current viewer can be promoted up.
								casted_frame->PromoteChildViewerContents(this, TRUE);
							}
						}
					}
					break;

			case xiv_right_btn_down:
					{
						// Button was pressed in the screen items area.
						if (mouse_ntf->object == NULL || m_dialog_mode != grdm_non_term_depends)
							break;

						// Current view mode is a tree of non terminal dependencies.
						TMenuItemInfo menu_info[7];
						wchar_t  buff_item_name[80], buff_sym_name[40];
						int cnt_items = 0;

						TLangViewLexItem *basic_item = (TLangViewLexItem*)mouse_ntf->object->GetOwner();
						WORD sym_value = basic_item->GetSymbolValue();
						if (sym_value != 0xFFFF)
						{
							// Clicked item is terminal or non terminal. Show its props as a menu item.
							swprintf(buff_item_name, 80, L"%hd: %s", sym_value, m_grammar->GetSymbolName(sym_value, buff_sym_name));
							menu_info[cnt_items++].Setup(100, buff_item_name);
						}

						if (cnt_items > 0)
						{
							// Non empty menu was prepared. Display this menu.
							menu_info[cnt_items].SetEndOfMenu();
							DWORD res = m_view_helper.ShowAndTrackPopupMenu(menu_info, mouse_ntf->click_place.x, mouse_ntf->click_place.y);
							if (res == 100)
							{
								// Remove the old selection if any and select all nodes, that represent the clicked symbol.
								ReleaseSymbolsTreeSelections(list_bold_blue);
								CreateSymbolsTreeSelections(m_tree_items_root, list_bold_blue, sym_value);
								CreateSymbolsTreeSelections(m_aux_upper_tree_root, list_bold_blue, sym_value);

								// Repaint the viewer.
								m_view_helper.UpdateControl();
							}
						}
					}
					break;

			case xiv_corner_clicked:
					{
						// Lower right corner was clicked. Prepare and process the context menu.
						TScreenItemsViewNotify *nti = (TScreenItemsViewNotify*)hWndControl;
						if (m_dialog_mode == grdm_none)
							ProcessRightBottomCornerEmptyViewerClick(nti->pos_x, nti->pos_y);
						else ProcessRightBottomCornerClick(nti->pos_x, nti->pos_y);
					}
					break;
		}
	}
}

void TLangViewRulesGridDialog::ProcessMouseClick(TScreenItemsViewNtfCode notificationCode, TScreenItemsViewMouseNotify *mouse_ntf)
{
	// Clicked object should be present.
	assert(mouse_ntf->object != NULL);
	assert(mouse_ntf->object->GetOwner() != NULL);

	if (mouse_ntf->object->GetOwner() == &m_rules_grid)
	{
		//
		//	Background of the main grid row was clicked.
		//
		int irow = m_rules_grid.GetGrid().GetRowFromPosY(mouse_ntf->app_y);
		assert(irow >= 0);
		ProcessMainGridRowBkgrClick(notificationCode, irow);
	}
	else if (mouse_ntf->object->GetOwner() == &m_symbols_table_grid)
	{
		//
		//	Background of the symbols table row was clicked.
		//
		int irow = m_symbols_table_grid.GetRowFromPosY(mouse_ntf->app_y);
		assert(irow >= 0);

		// Check the type of the clicked row.
		DWORD row_arr_data = m_symbols_table_grid.GetRowAppData(irow);
		switch (LOWORD(row_arr_data))
		{
			case SYMS_ROW_none:
					// This row is not clickable.
					break;

			case SYMS_ROW_grammar_symbol:
					{
						TLangViewClickEvent evt_info;
						evt_info.SetGrammarBrowserEvent(grbr_evt_symbol, (WORD)m_grammar->GetGrammarIndex(), HIWORD(row_arr_data));
						SendForProcessingFromScreenItemsView(notificationCode, evt_info);
					}
					break;

			default:
				// Uninited or unsupported row type.
				assert(FALSE);
				break;
		}
	}
	else if (mouse_ntf->object->GetOwner() == &m_grm_view_labels_owner)
	{
		//
		//	The grammar view label was clicked.
		//
		TGrammarViewLabelItem *grm_label = (TGrammarViewLabelItem*)(mouse_ntf->object);
		ProcessMainGridSpecLabelClick(notificationCode, grm_label);
	}
	else if (mouse_ntf->object->GetOwner() == &m_frgr_lex_items_owner)
	{
		//
		//	Foreground LexItemScreenItem object was clicked.
		//
		TLangViewLexItem *lex_item = (TLangViewLexItem*)(mouse_ntf->object);
		ProcessMainGridOrTreeLexItemClick(notificationCode, lex_item);
	}
	else
	{
		//
		//	This should be foreground LexItemTreeNode object. Unfortunately current design
		//	does not allow to check the value and type of the owner pointer.
		//
		TLangViewLexItem *lex_item = (TLangViewLexItem*)(mouse_ntf->object->GetOwner());
		ProcessMainGridOrTreeLexItemClick(notificationCode, lex_item);
	}
}

void TLangViewRulesGridDialog::ProcessMainGridRowBkgrClick(TScreenItemsViewNtfCode notificationCode, int irow)
{
	// Pick up the index of the current grammar.
	WORD igrammar = (WORD)m_grammar->GetGrammarIndex();

	TLangViewClickEvent evt_info;
	switch (m_rules_grid.GetRowMeaning(irow))
	{
		case ROW_none:
				// This row is not clickable.
				return;

		case ROW_grammar_rule_body:
				evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_rule, igrammar, m_rules_grid.GetIndex2(irow));
				break;

		case ROW_parsing_state_header:
				evt_info.SetGrammarBrowserEvent(grbr_evt_parsing_state, igrammar, m_rules_grid.GetIndex2(irow));
				break;

		case ROW_xpct_conflict_header:
				evt_info.SetGrammarBrowserEvent(grbr_evt_xpct_conflict, igrammar, m_rules_grid.GetIndex2(irow));
				break;

		case ROW_grammar_conflict_header:
				evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_conflict, igrammar, m_rules_grid.GetIndex2(irow));
				break;

		case ROW_conflicting_action_header:
				evt_info.SetGrammarBrowserEvent(grbr_evt_conflicting_action, igrammar, m_rules_grid.GetIndex2(irow), m_rules_grid.GetIndex3(irow));
				break;

		case ROW_derivation_path_body:
				evt_info.SetGrammarBrowserEvent(grbr_evt_derivation_path, igrammar, m_rules_grid.GetIndex2(irow), m_rules_grid.GetIndex3(irow), m_rules_grid.GetIndex4(irow));
				break;

		default:
			// Uninited or unsupported row type.
			assert(FALSE);
			return;
	}

	// Send prepared event to the owner for processing.
	SendForProcessingFromScreenItemsView(notificationCode, evt_info);
}

void TLangViewRulesGridDialog::ProcessMainGridOrTreeLexItemClick(TScreenItemsViewNtfCode notificationCode, TLangViewLexItem *lex_item)
{
	TLangViewClickEvent evt_info;
	switch (lex_item->GetItemType())
	{
		case litp_lexema:
		case litp_non_terminal:
				{
					// Mark event with the index of the current grammar.
					WORD igrammar = (WORD)m_grammar->GetGrammarIndex();

					// Prepare the symbol click event.
					evt_info.SetGrammarBrowserEvent(grbr_evt_symbol, igrammar, lex_item->GetSymbolValue());
				}
				break;

		case litp_special_lex_item:
				{
					TLangViewSpecialLexItem *spec_item = (TLangViewSpecialLexItem*)lex_item;

					//  Clicked object contains the index of the grammar.
					WORD igrammar = spec_item->GetGrammarIndex();

					switch (spec_item->GetSpecType())
					{
						case slit_grammar_rules:
								evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_rule, igrammar, -1);
								break;

						case slit_parsing_states:
								evt_info.SetGrammarBrowserEvent(grbr_evt_parsing_state, igrammar, -1);
								break;

						case slit_xpct_conflicts:
								evt_info.SetGrammarBrowserEvent(grbr_evt_xpct_conflict, igrammar, -1);
								break;

						case slit_all_grammar_conflicts:
								evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_conflict, igrammar, -4);
								break;

						case slit_ambig_grm_conflicts:
								evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_conflict, igrammar, -3);
								break;

						case slit_assigned_grm_conflicts:
								evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_conflict, igrammar, -2);
								break;

						case slit_unexp_grm_conflicts:
								evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_conflict, igrammar, -1);
								break;

						case slit_grammar:
								evt_info.SetGrammarBrowserEvent(grbr_evt_grammar, igrammar);
								break;

						case slit_rule_label:
								evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_rule, igrammar, spec_item->GetRuleIndex());
								break;

						case slit_rule_position:
								{
									// Check for special case where rule position describes the rule action symbol of the axioma
									// of the nested grammar. Such position is used as final step in some derivation paths although
									// it is not a real rule position.
									WORD irule = spec_item->GetRuleIndex();
									if (irule == 0xFFFF)
										return;

									// This is real position. Prepare the rule click event.
									evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_rule, igrammar, irule);
								}
								break;

						case slit_xpct_conflict:
								evt_info.SetGrammarBrowserEvent(grbr_evt_xpct_conflict, igrammar, spec_item->GetXpctConflictIndex());
								break;

						case slit_grammar_conflict:
								evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_conflict, igrammar, spec_item->GetGrammarConflictIndex());
								break;

						case slit_conflicting_action:
								evt_info.SetGrammarBrowserEvent(grbr_evt_conflicting_action, igrammar, spec_item->GetGrammarConflictIndex(), spec_item->GetActionIndex());
								break;

						case slit_derivation_path:
								evt_info.SetGrammarBrowserEvent(grbr_evt_derivation_path, igrammar, spec_item->GetGrammarConflictIndex(), spec_item->GetActionIndex(), spec_item->GetPathIndex());
								break;

						case slit_center_button:
						case slit_empty_symbol:
								// These objects are not clickable.
								return;

						default:
							// Uninited or unsupported spec lex item type.
							assert(FALSE);
							return;
					}
				}
				break;

		default:
			// Uninited or unsupported lex item type.
			assert(FALSE);
			return;
	}

	// Send prepared event to the owner for processing.
	SendForProcessingFromScreenItemsView(notificationCode, evt_info);
}

void TLangViewRulesGridDialog::ProcessMainGridSpecLabelClick(TScreenItemsViewNtfCode notificationCode, TGrammarViewLabelItem *label_item)
{
	TLangViewClickEvent evt_info;
	WORD igrammar = (WORD)m_grammar->GetGrammarIndex();
	switch (label_item->m_label_type)
	{
		case TGrammarViewLabelItem::gvlt_grammar_symbol:
		case TGrammarViewLabelItem::gvlt_cfct_inp_symbol:
				evt_info.SetGrammarBrowserEvent(grbr_evt_symbol, igrammar, label_item->m_app_value);
				break;

		case TGrammarViewLabelItem::gvlt_parsing_state:
				evt_info.SetGrammarBrowserEvent(grbr_evt_parsing_state, igrammar, label_item->m_app_value);
				break;

		case TGrammarViewLabelItem::gvlt_xpct_conflict:
				evt_info.SetGrammarBrowserEvent(grbr_evt_xpct_conflict, igrammar, label_item->m_app_value);
				break;

		case TGrammarViewLabelItem::gvlt_grammar_conflict:
				evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_conflict, igrammar, label_item->m_app_value);
				break;

		case TGrammarViewLabelItem::gvlt_parsing_action:
				{
					if (label_item->m_app_value < actb_reduce)
					{
						// This is shift action. Generate parsing state event.
						evt_info.SetGrammarBrowserEvent(grbr_evt_parsing_state, igrammar, label_item->m_app_value);
					}
					else if (label_item->m_app_value < actb_conflict)
					{
						// This is reduce action. Generate grammar rule event.
						evt_info.SetGrammarBrowserEvent(grbr_evt_grammar_rule, igrammar, label_item->m_app_value-actb_reduce);
					}
					else
					{
						// Bogus action value. conflicting action can be only either shift or reduce.
						assert(FALSE);
						return;
					}
				}
				break;

		default:
			// Uninited or unsupported label type.
			assert(FALSE);
			return;
	}

	// Send prepared event to the owner for processing.
	SendForProcessingFromScreenItemsView(notificationCode, evt_info);
}

void TLangViewRulesGridDialog::ProcessRightBottomCornerEmptyViewerClick(long mouse_x, long mouse_y)
{
	assert(m_dialog_mode == grdm_none);
	if (m_contents_history.GetNumItems() <= 0)
	{
		// The history is empty. Do not show anything.
		return;
	}

	// Show only the std footer 3. It contains the menu items list terminator.
	TMenuItemInfo menu_info[sizeof(g_RightBottomContextMenuFooter3)/sizeof(TMenuItemInfo)];
	memcpy(menu_info, g_RightBottomContextMenuFooter3, sizeof(g_RightBottomContextMenuFooter3));

	// Copy to clipboard should be disabled because there are no contents.
	menu_info[2].item_flags |= MF_GRAYED;

	// Track popup menu.
	DWORD res = m_view_helper.ShowAndTrackPopupMenu(menu_info+1, mouse_x, mouse_y);
	if (res != 0)
	{
		// Some menu item was selected.
		ExecPopupMenuCommand(res, NULL);
	}
}

void TLangViewRulesGridDialog::ProcessRightBottomCornerClick(long mouse_x, long mouse_y)
{
	assert(m_dialog_mode != grdm_none);

	wchar_t dynamic_item_buff1[80], dynamic_item_buff2[180];
	TMenuItemInfo menu_info[7+CTX_MLEN(1)+CTX_MLEN(1a)+CTX_MLEN(1b)+CTX_MLEN(1c)+CTX_MLEN(2)+CTX_MLEN(3a)+CTX_MLEN(3b)+CTX_MLEN(4)+CTX_MLEN(5)+CTX_MLEN(5b)+CTX_MLEN(5c)+CTX_MLEN(6)+CTX_MLEN(7)+CTX_MLEN(8)+8+10];
	int items_used = 0;

	// Show the stnd menu that should be present in all dialog modes.
	memcpy(menu_info, g_RightBottomContextMenuHeader1, sizeof(g_RightBottomContextMenuHeader1));
	items_used += sizeof(g_RightBottomContextMenuHeader1)/sizeof(TMenuItemInfo);

	// Extra menu for all tree based dialog modes.
	if (IsTreeBasedDialogMode() == TRUE)
	{
		memcpy(menu_info+items_used, g_RightBottomContextMenuHeader2, sizeof(g_RightBottomContextMenuHeader2));
		items_used += sizeof(g_RightBottomContextMenuHeader2)/sizeof(TMenuItemInfo);

		switch (m_tree_placing_mode[m_dialog_mode])
		{
			case plc_mode_concentric:
					menu_info[items_used-2].item_flags |= MF_CHECKED;
					break;

			case plc_mode_rect_lines:
					menu_info[items_used-1].item_flags |= MF_CHECKED;
					break;
		}
	}

	switch (m_dialog_mode)
	{
		case grdm_grms_overview:
				{
					// Menu 1. Show/hide individual grammar conflict instances.
					memcpy(menu_info+items_used, CTX_MENU_NAME(1), sizeof(CTX_MENU_NAME(1)));
					items_used += CTX_MLEN(1);

					if (m_show_individual_grm_conflicts == TRUE)
						menu_info[items_used-1].item_flags |= MF_CHECKED;
				}
				break;

		case grdm_terminal_symbols:
				{
					// Menu 1a. Select terminal symbols sort order.
					memcpy(menu_info+items_used, CTX_MENU_NAME(1a), sizeof(CTX_MENU_NAME(1a)));
					items_used += CTX_MLEN(1a);

					// Some sort order is selected all the times.
					menu_info[items_used-2+(int)m_terminals_sort_alpha_num].item_flags |= MF_CHECKED;
				}
				break;

		case grdm_non_terminals:
				{
					// Menu 1b. Select non terminals sort order.
					memcpy(menu_info+items_used, CTX_MENU_NAME(1b), sizeof(CTX_MENU_NAME(1b)));
					items_used += CTX_MLEN(1b);

					// Select the current sort order.
					menu_info[items_used-ntsm_num_types-2+m_non_terms_list_sort_mode].item_flags |= MF_CHECKED;

					if (m_show_non_term_starters == TRUE)
						menu_info[items_used-1].item_flags |= MF_CHECKED;
				}
				break;

		case grdm_non_term_depends:
				{
					// Menu 1c. Show/hide terminal symbols that can be directly present in the non terminal.
					memcpy(menu_info+items_used, CTX_MENU_NAME(1c), sizeof(CTX_MENU_NAME(1c)));
					items_used += CTX_MLEN(1c);

					if (m_non_term_deps_show_terms == TRUE)
						menu_info[items_used-3].item_flags |= MF_CHECKED;

					// Count number of different non terminals that are present in the tree.
					TSymbolsArray local_syms_list, extern_syms_list;
					TGenericTreeNodesIterator iter(&m_tree_items_root);
					for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
					{
						WORD sym_value = ((TLangViewTreeNode*)iter.GetCurrentItem())->GetSymbolValue();
						if (sym_value != 0xFFFF && sym_value >= NonTerminalsBase)
						{
							// Compare rules section of the clicked non term with rules section of the root non terminal.
							WORD sym_sect = m_grammar->non_terminals[sym_value-NonTerminalsBase].rules_sect;
							if (sym_sect == m_grammar->non_terminals[m_single_obj_index-NonTerminalsBase].rules_sect)
							{
								if (local_syms_list.IsSymInBuffer(sym_value) == FALSE && local_syms_list.AppendItem(sym_value) == FALSE)
								{
									MsgBox(L"Low on memory while counting number of local non terminals.");
									break;
								}
							}
							else
							{
								if (extern_syms_list.IsSymInBuffer(sym_value) == FALSE && extern_syms_list.AppendItem(sym_value) == FALSE)
								{
									MsgBox(L"Low on memory while counting number of extern non terminals.");
									break;
								}
							}
						}
					}

					swprintf(dynamic_item_buff1, 80, L"Different local non terms in the graph: %d", local_syms_list.NumItems());
					swprintf(dynamic_item_buff2, 80, L"Different extern non terms in the graph: %d", extern_syms_list.NumItems());
					menu_info[items_used-2].item_text = dynamic_item_buff1;
					menu_info[items_used-1].item_text = dynamic_item_buff2;
				}
				break;

		case grdm_grammar_rules:
				{
					// Menu 2. State transitions mode selection is applicable to all variants of the rules view.
					AddShowStateTransitionsModeMenu(menu_info, items_used);

					// Menu 3a.
					if (m_single_obj_index >= 0 && m_single_obj_index < actb_reduce)
					{
						// Add symbol use report menu items.
						memcpy(menu_info+items_used, CTX_MENU_NAME(3a), sizeof(CTX_MENU_NAME(3a)));
						items_used += CTX_MLEN(3a);

						if (m_sym_rprt_show_defn_rules == TRUE)
							menu_info[items_used-5].item_flags |= MF_CHECKED;
						if (m_sym_rprt_show_sym_uses == TRUE)
							menu_info[items_used-4].item_flags |= MF_CHECKED;
						if (m_sym_rprt_highlight_sym == TRUE)
							menu_info[items_used-3].item_flags |= MF_CHECKED;

						swprintf(dynamic_item_buff1, 80, L"Number of symbol defn rules: %d", m_sym_rprt_defn_rules);
						swprintf(dynamic_item_buff2, 80, L"Number of symbol use rules: %d", m_sym_rprt_use_rules);
						menu_info[items_used-2].item_text = dynamic_item_buff1;
						menu_info[items_used-1].item_text = dynamic_item_buff2;
					}
				}
				break;

		case grdm_parsing_states:
				{
					if (m_single_obj_index >= 0)
					{
						// Menu 2. State transitions mode selection is shown only when one parsing state is displayed.
						AddShowStateTransitionsModeMenu(menu_info, items_used);
					}

					// Menu 3b. Select what rules should be displayed.
					memcpy(menu_info+items_used, CTX_MENU_NAME(3b), sizeof(CTX_MENU_NAME(3b)));
					items_used += CTX_MLEN(3b);

					if (m_states_show_interm_positions == TRUE)
						menu_info[items_used-1].item_flags |= MF_CHECKED;
				}
				break;

		case grdm_xpct_conflicts:
		case grdm_grammar_conflicts:
				{
					if (m_dialog_mode == grdm_grammar_conflicts)
					{
						// Menu 4.
						AddShowXpctConflictNameMenu(menu_info, items_used);
					}

					// Menu 5.
					memcpy(menu_info+items_used, CTX_MENU_NAME(5), sizeof(CTX_MENU_NAME(5)));
					items_used += CTX_MLEN(5);

					if (m_show_conflicting_action_hdrs == TRUE)
						menu_info[items_used-2].item_flags |= MF_CHECKED;
					if (m_show_directly_conflicting_rules == TRUE)
						menu_info[items_used-1].item_flags |= MF_CHECKED;

					if (m_dialog_mode == grdm_xpct_conflicts && m_single_obj_index >= 0)
					{
						// Menu 5b. Show list of directly involved rules.
						memcpy(menu_info+items_used, CTX_MENU_NAME(5b), sizeof(CTX_MENU_NAME(5b)));
						items_used += CTX_MLEN(5b);

						bool mem_error = FALSE;
						TSymbolsArray involved_rules;
						int iconflict = m_grammar->GetFirstGrammarConflictInx(m_single_obj_index);
						while (iconflict >= 0 && mem_error == FALSE)
						{
							TRulePositionsArray directly_conflicting_positions;
							if (TLangViewMiscGrammarAnalysisHelper::CollectDirectlyInvolvedPositions(directly_conflicting_positions, m_grammar, iconflict) == FALSE)
								mem_error = TRUE;

							for (int ipos=0; ipos<directly_conflicting_positions.NumItems(); ++ipos)
							{
								WORD irule = directly_conflicting_positions[ipos].irule;
								if (involved_rules.IsSymInBuffer(irule) == FALSE && involved_rules.AppendItem(irule) == FALSE)
								{
									mem_error = TRUE;
									break;
								}
							}

							iconflict = m_grammar->GetNextGrammarConflictInx(m_single_obj_index, iconflict);
						}

						if (mem_error == FALSE)
						{
							// Sort the list of collected rule indexes.
							involved_rules.QuickSort(TSymbolsArray::CompareSymbols);

							wcscpy(dynamic_item_buff2, L"Directly involved rules: ");
							int space_left = sizeof(dynamic_item_buff2)/sizeof(wchar_t)-wcslen(dynamic_item_buff2)-8;
							for (int index=0; index<involved_rules.NumItems(); ++index)
							{
								wchar_t loc_buff[40];
								swprintf(loc_buff, 40, L"R%hu%s", involved_rules[index], ((index != involved_rules.NumItems()-1) ? L", " : L"."));

								if ((int)wcslen(loc_buff) <= space_left)
								{
									wcscat(dynamic_item_buff2, loc_buff);
									space_left -= wcslen(loc_buff);
								}
								else
								{
									wcscat(dynamic_item_buff2, L"....");
									break;
								}
							}

							// Give out the pointter to the generated string.
							menu_info[items_used-1].item_text = dynamic_item_buff2;
						}
						else
						{
							MsgBox(L"Low on memory while assembling the list of involved rules.");
							break;
						}
					}

					if (m_dialog_mode == grdm_grammar_conflicts && m_single_obj_index < 0)
					{
						// Menu 5c. Show only those conflicts, that have non empty analysis.
						memcpy(menu_info+items_used, CTX_MENU_NAME(5c), sizeof(CTX_MENU_NAME(5c)));
						items_used += CTX_MLEN(5c);

						if (m_show_with_analysis_only == TRUE)
							menu_info[items_used-1].item_flags |= MF_CHECKED;
					}
				}
				break;

		case grdm_key_rules_xpct:
		case grdm_key_rules_conflict:
				{
					if (m_dialog_mode == grdm_key_rules_conflict)
					{
						// Menu 4.
						AddShowXpctConflictNameMenu(menu_info, items_used);
					}

					// Menu 6.
					memcpy(menu_info+items_used, CTX_MENU_NAME(6), sizeof(CTX_MENU_NAME(6)));
					items_used += CTX_MLEN(6);

					if (m_show_key_conflicting_rules == TRUE)
					{
						menu_info[items_used-4].item_flags |= MF_CHECKED;

						if (m_merge_diff_key_rule_seqs == TRUE)
							menu_info[items_used-3].item_flags |= MF_CHECKED;
					}
					else
					{
						menu_info[items_used-3].item_flags |= (MF_CHECKED | MF_GRAYED);
					}

					if (m_show_predecessor_seqs == TRUE)
						menu_info[items_used-2].item_flags |= MF_CHECKED;
					if (m_show_follower_seqs == TRUE)
						menu_info[items_used-1].item_flags |= MF_CHECKED;
				}
				break;

		case grdm_dplist_symbols_xpct:
		case grdm_dplist_symbols_conflict:
		case grdm_dplist_symbols_action:
				{
					if (m_dialog_mode != grdm_dplist_symbols_xpct)
					{
						// Menu 4.
						AddShowXpctConflictNameMenu(menu_info, items_used);
					}

					// Menu 7 without last item.
					memcpy(menu_info+items_used, CTX_MENU_NAME(7), sizeof(CTX_MENU_NAME(7)));
					items_used += CTX_MLEN(7)-1;

					if (m_show_path_seq_props == TRUE)
						menu_info[items_used-1].item_flags |= MF_CHECKED;
				}
				break;

		case grdm_deriv_path_seq:
				{
					// Menu 7.
					memcpy(menu_info+items_used, CTX_MENU_NAME(7), sizeof(CTX_MENU_NAME(7)));
					items_used += CTX_MLEN(7);

					if (m_show_path_seq_props == TRUE)
						menu_info[items_used-2].item_flags |= MF_CHECKED;
					if (m_show_single_path_seq_lex_items == TRUE)
						menu_info[items_used-1].item_flags |= MF_CHECKED;
				}
				break;

		case grdm_deriv_path_steps:
				{
					// Menu 8.
					memcpy(menu_info+items_used, CTX_MENU_NAME(8), sizeof(CTX_MENU_NAME(8)));
					items_used += CTX_MLEN(8);

					if (m_show_deriv_path_dtls == TRUE)
						menu_info[items_used-2].item_flags |= MF_CHECKED;
					if (m_show_deriv_path_from_bottom == TRUE)
						menu_info[items_used-1].item_flags |= MF_CHECKED;
				}
				break;

		default:
			// All other dialog modes do not have mode specific menus.
			break;
	}

	// Optional footer 1.
	if (m_owner->IsGrammarBrowser() == TRUE && ((TLangViewGrammarBrowser*)m_owner)->CanPromoteChildViewer(this) == TRUE)
	{
		// Current viewer can promote its contents up. Show related menu item.
		memcpy(menu_info+items_used, g_RightBottomContextMenuFooter1, sizeof(g_RightBottomContextMenuFooter1));
		items_used += sizeof(g_RightBottomContextMenuFooter1)/sizeof(TMenuItemInfo);
	}

	// Optional footer 2.
	if (m_dialog_mode != grdm_terminal_symbols && m_dialog_mode != grdm_non_terminals)
	{
		memcpy(menu_info+items_used, g_RightBottomContextMenuFooter2, sizeof(g_RightBottomContextMenuFooter2));
		items_used += sizeof(g_RightBottomContextMenuFooter2)/sizeof(TMenuItemInfo);

		if (m_show_terminals_grm_names == TRUE)
			menu_info[items_used-2].item_flags |= MF_CHECKED;
		if (m_hgtlt_transp_non_terms == TRUE)
			menu_info[items_used-1].item_flags |= MF_CHECKED;
	}

	// Std footer 3. This array contains menu items list terminator.
	memcpy(menu_info+items_used, g_RightBottomContextMenuFooter3, sizeof(g_RightBottomContextMenuFooter3));
	items_used += sizeof(g_RightBottomContextMenuFooter3)/sizeof(TMenuItemInfo);

	if (m_dialog_mode != grdm_grms_overview && m_dialog_mode != grdm_terminal_symbols && m_dialog_mode != grdm_non_terminals)
	{
		// This is regular viewer that can have history.
		if (m_contents_history.GetNumItems() <= 0)
			menu_info[items_used-3].item_flags |= MF_GRAYED;
	}
	else
	{
		// Remove the "go back" menu item.
		menu_info[items_used-3] = menu_info[items_used-2];
		menu_info[items_used-2] = menu_info[items_used-1];
	}

	// Track the popup menu.
	DWORD res = m_view_helper.ShowAndTrackPopupMenu(menu_info, mouse_x, mouse_y);
	if (res != 0)
	{
		// Some menu item was selected.
		ExecPopupMenuCommand(res, dynamic_item_buff2);
	}
}

void TLangViewRulesGridDialog::AddShowStateTransitionsModeMenu(TMenuItemInfo *menu_info, int &menu_items_used)
{
	// Add menu items for the curr state transitions display mode.
	memcpy(menu_info+menu_items_used, CTX_MENU_NAME(2), sizeof(CTX_MENU_NAME(2)));
	menu_items_used += CTX_MLEN(2);

	// Some view mode is all the times selected.
	menu_info[menu_items_used-pstt_trnv_num_types+GetCurrTrnvMode()].item_flags |= MF_CHECKED;
}

void TLangViewRulesGridDialog::AddShowXpctConflictNameMenu(TMenuItemInfo *menu_info, int &menu_items_used)
{
	// Add menu item for showing/hiding the xpct conflict name on the grammar conflict header.
	memcpy(menu_info+menu_items_used, CTX_MENU_NAME(4), sizeof(CTX_MENU_NAME(4)));
	menu_items_used += CTX_MLEN(4);

	if (m_show_xpct_conflict_name == TRUE)
		menu_info[menu_items_used-1].item_flags |= MF_CHECKED;
}

void TLangViewRulesGridDialog::ExecPopupMenuCommand(DWORD menu_command, wchar_t *dynamic_item_buff)
{
	//
	//  Header menus.
	//
	if (menu_command == rvcm_hd1_zi)
	{
		// Zoom in the current view. Cast result to float to supress the compiler warning.
		m_zoom_level[m_dialog_mode] = (float)(m_zoom_level[m_dialog_mode] * 1.4);
		m_view_helper.SetZoomLevel(m_zoom_level[m_dialog_mode]);
		return;
	}
	else if (menu_command == rvcm_hd1_zo)
	{
		// Zoom out the current view. Cast result to float to supress the compiler warning.
		m_zoom_level[m_dialog_mode] = (float)(m_zoom_level[m_dialog_mode] / 1.4);
		m_view_helper.SetZoomLevel(m_zoom_level[m_dialog_mode]);
		return;
	}
	else if (menu_command == rvcm_hd1_zob)
	{
		// Zoom out the current view. Cast result to float to supress the compiler warning.
		m_zoom_level[m_dialog_mode] = (float)(m_zoom_level[m_dialog_mode] / 3.0);
		m_view_helper.SetZoomLevel(m_zoom_level[m_dialog_mode]);
		return;
	}
	else if (menu_command == rvcm_hd1_rz)
	{
		// Reset current zoom.
		m_zoom_level[m_dialog_mode] = 1.0;
		m_view_helper.SetZoomLevel(m_zoom_level[m_dialog_mode]);
		return;
	}
	else if (menu_command == rvcm_hd2_cnct)
	{
		// Change the placing mode.
		m_tree_placing_mode[m_dialog_mode] = plc_mode_concentric;
	}
	else if (menu_command == rvcm_hd2_rcp)
	{
		// Change the placing mode.
		m_tree_placing_mode[m_dialog_mode] = plc_mode_rect_lines;
	}

	//
	// Grammars tree overview.
	//
	else if (menu_command == rvcm_m1_igrc)
	{
		// Show/hide inidividual grammar conflicts on the overview.
		m_show_individual_grm_conflicts ^= 1;
	}

	//
	//  Symbol lists and non terminal dependencies.
	//
	else if (menu_command == rvcm_m1a_ssv || menu_command == rvcm_m1a_salp)
	{
		// Change the sort order in the list of terminal symbols.
		m_terminals_sort_alpha_num = (menu_command == rvcm_m1a_ssv) ? FALSE : TRUE;
	}
	else if (menu_command >= rvcm_m1b_so1 && menu_command < rvcm_m1b_so1+ntsm_num_types)
	{
		// Change the non terminals sort order.
		m_non_terms_list_sort_mode = (TNonTerminalsSortOrder)(menu_command-rvcm_m1b_so1);
	}
	else if (menu_command == rvcm_m1b_sstc)
	{
		// Toggle displaying lists of possible starter symbols.
		m_show_non_term_starters ^= 1;
	}
	else if (menu_command == rvcm_m1c_sts)
	{
		// Toggle displaying teminals in the dependencies tree.
		m_non_term_deps_show_terms ^= 1;
	}

	//
	//  Rule view and parsing states view specific menus.
	//
	else if (menu_command >= rvcm_m2 && menu_command < rvcm_m2+pstt_trnv_num_types)
	{
		// Change the parsing state transitions display mode.
		if (m_dialog_mode == grdm_grammar_rules)
		{
			bool symbol_use_report_mode = m_single_obj_index >= 0 && m_single_obj_index < actb_reduce;
			m_pstt_trnv_mode[(int)symbol_use_report_mode] = (TParsingStateTransitionsViewMode)(menu_command-rvcm_m2);
		}
		else if (m_dialog_mode == grdm_parsing_states)
		{
			m_pstt_trnv_mode[2] = (TParsingStateTransitionsViewMode)(menu_command-rvcm_m2);
		}
	}
	else if (menu_command == rvcm_m3a_sdr)
	{
		// Toggle showing rules for the symbol.
		m_sym_rprt_show_defn_rules ^= 1;
	}
	else if (menu_command == rvcm_m3a_ssu)
	{
		// Toggle showing rules where symbol is used.
		m_sym_rprt_show_sym_uses ^= 1;
	}
	else if (menu_command == rvcm_m3a_hsm)
	{
		// Toggle highlighting the symbol.
		m_sym_rprt_highlight_sym ^= 1;
	}
	else if (menu_command == rvcm_m3b_ipon)
	{
		// Toggle displaying all rules for parsing state or only some of them.
		m_states_show_interm_positions ^= 1;
	}

	//
	//  All views with grammar conflict headers.
	//
	else if (menu_command == rvcm_m4_sxn)
	{
		// Toggle conflicting action headers mode.
		m_show_xpct_conflict_name ^= 1;
	}

	//
	//  Conflicts overview and key conflicting rules.
	//
	else if (menu_command == rvcm_m5_scah)
	{
		// Toggle conflicting action headers mode.
		m_show_conflicting_action_hdrs ^= 1;
	}
	else if (menu_command == rvcm_m5_sdcr)
	{
		// Toggle directly conflicting rules mode.
		m_show_directly_conflicting_rules ^= 1;
	}
	else if (menu_command == rvcm_m5b_dinr)
	{
		// Place contents of the menu item into the clipboard.
		assert(dynamic_item_buff != NULL);
		DWORD err = TControlHelper::PutStringToClipboard(dynamic_item_buff);
		if (err != ERROR_SUCCESS)
			MsgBoxFmt(MB_OK, L"Error putting the menu item text into the clipboard (Win32 error: %d).", err);
	}
	else if (menu_command == rvcm_m5c_wan)
	{
		// Toggle all conflicts or only conflicts with analysis mode.
		m_show_with_analysis_only ^= 1;
	}
	else if (menu_command == rvcm_m6_skr)
	{
		// Toggle displaying key rules.
		m_show_key_conflicting_rules ^= 1;
	}
	else if (menu_command == rvcm_m6_mfd)
	{
		// Toggle context seqs groupping mode.
		m_merge_diff_key_rule_seqs ^= 1;
	}
	else if (menu_command == rvcm_m6_sps)
	{
		// Toggle displaying predecessors.
		m_show_predecessor_seqs ^= 1;
	}
	else if (menu_command == rvcm_m6_sfs)
	{
		// Toggle displaying followers.
		m_show_follower_seqs ^= 1;
	}

	//
	//  Derivation paths as symbol sequencies.
	//
	else if (menu_command == rvcm_m7_sdpp)
	{
		// Toggle derivation path props mode.
		m_show_path_seq_props ^= 1;
	}
	else if (menu_command == rvcm_m7_ssli)
	{
		// Toggle type of symbols that are used for displaying the path.
		m_show_single_path_seq_lex_items ^= 1;
	}

	//
	//  Derivation path as steps.
	//
	else if (menu_command == rvcm_m8_sdpd)
	{
		// Toggle derivation path details mode.
		m_show_deriv_path_dtls ^= 1;
	}
	else if (menu_command == rvcm_m8_sspb)
	{
		// Toggle derivation path steps display order.
		m_show_deriv_path_from_bottom ^= 1;
	}

	//
	//  Footer menus.
	//
	else if (menu_command == rvcm_ft1_pccu)
	{
		if (m_owner->IsGrammarBrowser() == TRUE)
		{
			// Shift current contents of the viewer to the upper viewer.
			TLangViewGrammarBrowser *browser = (TLangViewGrammarBrowser*)m_owner;
			browser->PromoteChildViewerContents(this, FALSE);
			return;
		}
	}
	else if (menu_command == rvcm_ft2_tsgn)
	{
		// Toggle mode of displaying the terminal symbol names.
		m_show_terminals_grm_names ^= 1;
	}
	else if (menu_command == rvcm_ft2_htnt)
	{
		// Toggle highlighting transparent non terminals mode.
		m_hgtlt_transp_non_terms ^= 1;
	}
	else if (menu_command == rvcm_ft3_gobk)
	{
		TDialogStateInfo info;
		if (m_contents_history.PopItem(info) == TRUE)
		{
			m_step_back_proc_flag = TRUE;
			LoadContentsFromInfo(info);
			m_step_back_proc_flag = FALSE;
			return;
		}
	}
	else if (menu_command == rvcm_ft3_ctc)
	{
		// Pick up the rect that describes the current contents of the viewer.
		RECT margins_rect = { 10, 10, 10, 10 };
		RECT margins_rect_for_trees = { 40, 20, 40, 40 };
		if (IsTreeBasedDialogMode() == TRUE)
			margins_rect = margins_rect_for_trees;

		// Place current contents of the viewer into the clipboard.
		PutScreenShotToClipboard(m_view_helper, &margins_rect, UpdateClipboardImageCallback, this);
	}

	// Reload contents to display the changes.
	LoadContentsFrom(this);
}

void TLangViewRulesGridDialog::UpdateScreenShotForClipboard(HDC hMemoryDC, long image_size_x, long image_size_y)
{
	RECT dummy_invalid_rect = { 0, 0, 0, 0 };
	if (m_dialog_mode == grdm_deriv_path_ast)
	{
		// Draw the title that will describe the path.
		TTextItem80 path_info(&m_clipboard_image_title_style);

		// Set contents of the text screen item.
		if (m_grammar->GetGrammarIndex() == 0)
			path_info.SetTextFmt(L"C%d-A%d-P%d", m_single_obj_index, m_single_sub_obj_index, m_single_sub_sub_obj_index);
		else path_info.SetTextFmt(L"G%d-C%d-A%d-P%d", m_grammar->GetGrammarIndex(), m_single_obj_index, m_single_sub_obj_index, m_single_sub_sub_obj_index);

		// Draw object in the fixed place for now.
		path_info.PlaceObject(hMemoryDC, 180, 48);
		path_info.OnDraw(hMemoryDC, dummy_invalid_rect);
	}

	if (g_need_image_origin_footer == TRUE)
	{
		// Draw the origin footer.
		TTextItem80 origin_info(&m_clipboard_image_footer_style);
		origin_info.SetText(L"www.codestructureanalysis.com");
		origin_info.PlaceObject(hMemoryDC, image_size_x-170, image_size_y-18);
		origin_info.OnDraw(hMemoryDC, dummy_invalid_rect);
	}
}

void TLangViewRulesGridDialog::ResetSelection()
{
	if (m_dialog_mode == grdm_terminal_symbols || m_dialog_mode == grdm_non_terminals)
	{
		// Clear selection vars and reload the viewer.
		if (m_grm_symbols_list_frgr_slct != 0xFFFF || m_non_terms_list_bkgr_slct != NULL)
			SetGrammarSymbolsListSelection(0xFFFF, NULL);
	}
	else if (m_dialog_mode == grdm_non_term_depends)
	{
		// Remove the local symbol selections if any.
		if (ReleaseSymbolsTreeSelections(list_bold_blue) == TRUE)
		{
			// Some selection items were removed.
			m_view_helper.UpdateControl();
		}
	}
}

void TLangViewRulesGridDialog::ResetContentsInternal(bool preserve_symbol_lists_slct)
{
	StoreMajorTaskParams(grdm_none, NULL);
	SetControlText(IDC_RULGRID_CURR_VIEW_TITLE, NULL);
	SetControlText(IDC_RULGRID_GRM_NAME_TITLE, NULL);
	SetUpdateControlsLayout();

	m_view_helper.ClearInternalDataSet();
	m_view_helper.SetZoomLevel(1.0);

	m_rules_grid.Reset();
	m_tree_items_root.ReleaseChildren();
	m_aux_upper_tree_root.ReleaseChildren();
	m_symbols_table_grid.Reset();

	if (preserve_symbol_lists_slct == FALSE)
	{
		m_grm_symbols_list_frgr_slct = 0xFFFF;
		if (m_non_terms_list_bkgr_slct != NULL)
		{
			delete m_non_terms_list_bkgr_slct;
			m_non_terms_list_bkgr_slct = NULL;
		}
	}

	m_sym_rprt_defn_rules = 0;
	m_sym_rprt_use_rules = 0;

	// This array of pointers own its objects. This call will properly release them.
	m_tree_view_extra_items.Clear();

	m_lex_items_style.Reset();
	m_local_console.CloseStatusPanel();
	m_view_helper.ShowInternalDataSet(TRUE);

	UpdateDialogWindow();
}

void TLangViewRulesGridDialog::PlaceCurrentContentsTo(TDialogStateInfo &info_buffer)
{
	// Store props of the current display mode.
	info_buffer.m_mode = m_dialog_mode;
	info_buffer.m_grammar = m_grammar;
	info_buffer.m_index1 = m_single_obj_index;
	info_buffer.m_index2 = m_single_sub_obj_index;
	info_buffer.m_index3 = m_single_sub_sub_obj_index;

	// Retrieve current scrolling position.
	TScreenItemsViewSpaceInfo space_info;
	m_view_helper.GetSpaceInfo(space_info);
	info_buffer.m_scroll_pos_x = space_info.scroll_pos.x;
	info_buffer.m_scroll_pos_y = space_info.scroll_pos.y;
}

void TLangViewRulesGridDialog::LoadContentsFrom(TLangViewRulesGridDialog *inst)
{
	TDialogStateInfo info;
	inst->PlaceCurrentContentsTo(info);
	LoadContentsFromInfo(info);
}

void TLangViewRulesGridDialog::LoadContentsFromInfo(TDialogStateInfo &info)
{
	// Call the appropriate loading function.
	switch (info.m_mode)
	{
		case grdm_none:
				ResetContentsInternal();
				break;

		case grdm_grms_overview:
				ShowGrammarsTreeOverview(info.m_grammar);
				break;

		case grdm_terminal_symbols:
				ShowTerminalSymbols(info.m_grammar);
				break;

		case grdm_non_terminals:
				ShowNonTerminals(info.m_grammar);
				break;

		case grdm_non_term_depends:
				ShowNonTerminalDependencies(info.m_grammar, info.m_index1);
				break;

		case grdm_grammar_rules:
				ShowGrammarRules(info.m_grammar, info.m_index1);
				break;

		case grdm_parsing_states:
				ShowParsingStates(info.m_grammar, info.m_index1);
				break;

		case grdm_processing_status:
				ShowConflictsAnalysisStatus(info.m_grammar);
				break;

		case grdm_xpct_conflicts:
				ShowXpctConflicts(info.m_grammar, info.m_index1);
				break;

		case grdm_grammar_conflicts:
				ShowGrammarConflicts(info.m_grammar, info.m_index1);
				break;

		case grdm_key_rules_xpct:
				ShowKeyRulesForXpctConflicts(info.m_grammar, info.m_index1);
				break;

		case grdm_key_rules_conflict:
				ShowKeyRulesForGrammarConflicts(info.m_grammar, info.m_index1);
				break;

		case grdm_dplist_differs_xpct:
				ShowDiffersTreeForXpctConflicts(info.m_grammar, info.m_index1);
				break;

		case grdm_dplist_differs_conflict:
				ShowDiffersTreeForGrammarConflicts(info.m_grammar, info.m_index1);
				break;

		case grdm_dplist_symbols_xpct:
				ShowSymbolSeqsForXpctConflicts(info.m_grammar, info.m_index1);
				break;

		case grdm_dplist_symbols_conflict:
				ShowSymbolSeqsForGrammarConflicts(info.m_grammar, info.m_index1);
				break;

		case grdm_dplist_differs_action:
				ShowDiffersTreeForConflictingAction(info.m_grammar, info.m_index1, info.m_index2);
				break;

		case grdm_dplist_symbols_action:
				ShowSymbolSeqsForConflictingAction(info.m_grammar, info.m_index1, info.m_index2);
				break;

		case grdm_deriv_path_ast:
				ShowDerivationPathAsAst(info.m_grammar, info.m_index1, info.m_index2, info.m_index3);
				break;

		case grdm_deriv_path_seq:
				ShowDerivationPathAsSeq(info.m_grammar, info.m_index1, info.m_index2, info.m_index3);
				break;

		case grdm_deriv_path_steps:
				ShowDerivationPathAsSteps(info.m_grammar, info.m_index1, info.m_index2, info.m_index3);
				break;

		default:
			assert(FALSE);
			break;
	}

	if (info.m_scroll_pos_x != 0 || info.m_scroll_pos_y != 0)
	{
		// Restore the original scroll pos even at the price of repainting.
		m_view_helper.SetScrollPos(info.m_scroll_pos_x, info.m_scroll_pos_y);
	}
}

void TLangViewRulesGridDialog::SetGrammarSymbolsListSelection(WORD slct_sym_frgr, TSymbolsArray *slct_syms_bkgr)
{
	if (m_dialog_mode == grdm_terminal_symbols || m_dialog_mode == grdm_non_terminals)
	{
		if (m_non_terms_list_bkgr_slct != NULL)
			delete m_non_terms_list_bkgr_slct;

		// Set new selection properties and reload the viewer.
		m_grm_symbols_list_frgr_slct = slct_sym_frgr;
		m_non_terms_list_bkgr_slct = slct_syms_bkgr;
		LoadContentsFrom(this);
	}
	else if (slct_syms_bkgr != NULL)
	{
		// The call is bogus. Get rid of the allocated structure.
		assert(FALSE);
		delete slct_syms_bkgr;
	}
}

void TLangViewRulesGridDialog::ShowGrammarsTreeOverview(TGrammar *grammar)
{
	assert(grammar != NULL);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_grms_overview, grammar);
	PrepareNewDialogMode(L"Grammars tree structure", L"Preparing grammars tree ...");

	if (grammar->parent_grammar != NULL)
	{
		PrepareAndShowNoObjectsMessage(L"Passed grammar is not a root grammar.");
		return;
	}
	else if (grammar->grm_props.processing_result == grpr_none || (grammar->NumRules() == 0 && grammar->NumParsingStates() == 0 && grammar->NumXpctConflicts() == 0 && grammar->NumConflicts() == 0))
	{
		// Show the grey text message instead of the single unclickable grammar object.
		PrepareAndShowNoObjectsMessage(L"Empty grammar", &m_analysis_warning_text_style);
		return;
	}

	// Create the tree.
	TLangViewGrammarTreesHelper hlpr(this, m_grammar);
	TLangViewTreeNode *overview = hlpr.CreateGrammarOverviewHier(NULL, m_grammar, m_show_individual_grm_conflicts);

	// Setup the root object.
	m_tree_items_root.Item().SetSimpleObjectProps(slit_center_button);
	if (overview != NULL)
	{
		// Hide the link between the root grammar and the center button object.
		overview->SetSelectedStateEx(FALSE, FALSE, FALSE, FALSE);
		m_tree_items_root.AdoptChild(overview);
	}

	// Display the tree. Note that if placing mode is concentric, the function below will shift the root grammar
	// object a little bit the in the direction of the invisible center button object.
	PrepareAndShowCurrentTree();
}

void TLangViewRulesGridDialog::ShowTerminalSymbols(TGrammar *grammar)
{
	assert(grammar != NULL);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal(TRUE);
	StoreMajorTaskParams(grdm_terminal_symbols, grammar);

	// Show titles and prepare console.
	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Terminal symbols (igrm=%d)", m_grammar->GetGrammarIndex());
	PrepareNewDialogMode(mode_name_buff, L"Preparing terminal symbols ...");

	if (m_grammar->NumTerminals() == 0)
	{
		// There is nothing to show.
		PrepareAndShowNoObjectsMessage(L"The grammar has no terminal symbols.");
		return;
	}

	// Figure out the length in pixels of the longest terminal symbol name.
	long names_clmn_width = 90;
	HDC hDC = ::GetDC(m_hDlg);
	if (hDC != NULL)
	{
		for (int isym=0; isym<m_grammar->NumTerminals(); ++isym)
		{
			if (m_grammar->symbols[isym].sym_type_name == TRUE)
			{
				long len = m_bold_index_text_style.GetStringWidth(hDC, m_grammar->symbols[isym].name_value)+8;
				if (len > names_clmn_width)
					names_clmn_width = len;
			}
		}

		::ReleaseDC(m_hDlg, hDC);
	}

	// Add columns to the grid.
	AddTermNonTermSymbolsTableColumn(50, &m_dark_delim_style);					// Symbol.
	AddTermNonTermSymbolsTableColumn(names_clmn_width, &m_dark_delim_style);	// Name.
	AddTermNonTermSymbolsTableColumn(60, NULL);								// App id.

	// Add the top frame rows.
	AddTermNonTermSymbolsTableRow(L"TerminalsTopFrame1", SYMS_ROW_none, 14, NULL, &m_semi_light_delim_style);
	AddTermNonTermSymbolsTableRow(L"TerminalsTopFrame2", SYMS_ROW_none, 4, &m_xpct_actions_map_hdr_style, &m_semi_light_delim_style);
	AddTermNonTermSymbolsTableRow(L"TerminalsTopFrame3", SYMS_ROW_none, 1, NULL, NULL);

	// Generate body of the table.
	TSymbolsArray terminal_symbols_list;
	if (TGrammarSymbolsHelper::CreateSortedTerminalSymbolsList(m_grammar->symbols, terminal_symbols_list, m_terminals_sort_alpha_num) == FALSE)
	{
		MsgBox(L"Error allocating sorted list of terminal symbols.");
	}
	else
	{
		WORD prev_sym = 0;

		for (int inx=0; inx<terminal_symbols_list.NumItems(); ++inx)
		{
			WORD sym = terminal_symbols_list[inx];

			if (m_terminals_sort_alpha_num == TRUE)
			{
				if (inx == 1 || m_grammar->symbols[sym].sym_type_name != m_grammar->symbols[prev_sym].sym_type_name)
				{
					AddTermNonTermSymbolsTableRow(L"TerminalsAlphaNumsDelim1", SYMS_ROW_none, 1, NULL, &m_semi_light_delim_style);
				}
				else if (inx != 0 && m_grammar->symbols[sym].sym_type_name == TRUE)
				{
					if (m_grammar->symbols[sym].name_value[0] <= L'Z' && m_grammar->symbols[prev_sym].name_value[0] >= L'a')
						AddTermNonTermSymbolsTableRow(L"TerminalsAlphaNumsDelim1", SYMS_ROW_none, 1, NULL, &m_semi_light_delim_style);
				}

				prev_sym = sym;
			}

			bool frgr_slct = (sym == m_grm_symbols_list_frgr_slct) ? TRUE : FALSE;
			TBasicStyle *row_bkgr = (sym == m_grm_symbols_list_frgr_slct) ? &m_light_delim_style : NULL;
			if (AddTerminalSymbolsTableTerminalRow(sym, row_bkgr, frgr_slct) == FALSE)
				break;
		}
	}

	// Add the bottom frame rows.
	AddTermNonTermSymbolsTableRow(L"TerminalsBottomFrame1", SYMS_ROW_none, 2, NULL, &m_semi_light_delim_style);
	AddTermNonTermSymbolsTableRow(L"TerminalsBottomFrame2", SYMS_ROW_none, 14);

	// Finalize and display the grid.
	m_symbols_table_grid.UpdateGrid();
	m_view_helper.SetZoomLevel(m_zoom_level[m_dialog_mode]);
	m_view_helper.ClearInternalDataSet();
	m_view_helper.AddToInternalDataSet(m_symbols_table_grid);
	m_view_helper.ShowInternalDataSet(TRUE);
	m_local_console.CloseStatusPanel();
}

void TLangViewRulesGridDialog::ShowNonTerminals(TGrammar *grammar)
{
	assert(grammar != NULL);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal(TRUE);
	StoreMajorTaskParams(grdm_non_terminals, grammar);

	// Show titles and prepare the console.
	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Non terminals (igrm=%d)", m_grammar->GetGrammarIndex());
	PrepareNewDialogMode(mode_name_buff, L"Preparing non terminals ...");

	if (m_grammar->NumNonTerminals() == 0)
	{
		// There is nothing to show.
		PrepareAndShowNoObjectsMessage(L"The grammar has no non terminals.");
		return;
	}

	TAnalysisTable first_and_follows, non_term_non_term_starters;
	if (m_show_non_term_starters == TRUE)
	{
		// Allocate and init matrixes.
		if (m_grammar->NumTerminals() == 0)
		{
			MsgBox(L"The grammar has no terminal symbols. The non terminal startes will not be displayed.");
			m_show_non_term_starters = FALSE;
		}
		else if (m_grammar->GenerateFirstsAndFollows(first_and_follows) == FALSE || m_grammar->GenerateNonTerminalFirsts(non_term_non_term_starters, first_and_follows) == FALSE)
		{
			MsgBox(L"Error generating the FIRSTs and FOLLOWs sets. The non terminal startes will not be displayed.");
			m_show_non_term_starters = FALSE;
		}
	}

	// Figure out the length in pixels of the longest non terminal name.
	long names_clmn_width = 140;
	HDC hDC = ::GetDC(m_hDlg);
	if (hDC != NULL)
	{
		for (int isym=0; isym<m_grammar->NumNonTerminals(); ++isym)
		{
			long len = m_bold_index_text_style.GetStringWidth(hDC, m_grammar->non_terminals[isym].symbol_name)+8;
			if (len > names_clmn_width)
				names_clmn_width = len;
		}

		::ReleaseDC(m_hDlg, hDC);
	}

	// Add columns to the grid.
	AddTermNonTermSymbolsTableColumn(50, &m_dark_delim_style);					// Symbol.
	AddTermNonTermSymbolsTableColumn(names_clmn_width, &m_dark_delim_style);	// Name.
	AddTermNonTermSymbolsTableColumn(52, &m_dark_delim_style);					// Public.
	AddTermNonTermSymbolsTableColumn(60, &m_dark_delim_style);					// Restriction.
	if (m_show_non_term_starters == FALSE)
	{
		AddTermNonTermSymbolsTableColumn(60, NULL);							// App id.
	}
	else
	{
		AddTermNonTermSymbolsTableColumn(60, &m_dark_delim_style);				// App id.
		AddTermNonTermSymbolsTableColumn(420, NULL);							// Non term starters.
	}

	// Add the top frame rows.
	TBasicStyle *header_bkgr_style = &m_parsing_states_hdr_style;
	AddTermNonTermSymbolsTableRow(L"NonTermsTopFrame1", SYMS_ROW_none, 14, NULL, &m_semi_light_delim_style);
	AddTermNonTermSymbolsTableRow(L"NonTermsTopFrame2", SYMS_ROW_none, 4, header_bkgr_style, &m_semi_light_delim_style);
	AddTermNonTermSymbolsTableRow(L"NonTermsTopFrame3", SYMS_ROW_none, 1, NULL, NULL);

	// Generate the body of the table.
	TSymbolsArray non_terms_list;
	if (TGrammarSymbolsHelper::CreateSortedNonTerminalsList(m_grammar->non_terminals, &(m_grammar->rules), non_terms_list, m_non_terms_list_sort_mode) == FALSE)
	{
		MsgBox(L"Error allocating sorted non terminals list.");
	}
	else
	{
		wchar_t *prev_subn = NULL;
		WORD prev_rules_sect = 0xFFFF;

		bool right_align = (m_non_terms_list_sort_mode == ntsm_special_anum) ? TRUE : FALSE;
		for (int inx=0; inx<non_terms_list.NumItems(); ++inx)
		{
			WORD sym = non_terms_list[inx];

			if (m_non_terms_list_sort_mode == ntsm_special_anum)
			{
				wchar_t *non_term_name = m_grammar->non_terminals[sym-NonTerminalsBase].symbol_name;
				wchar_t *subn = non_term_name+wcslen(non_term_name)-1;
				while (subn > non_term_name && *subn >= 'a' && *subn <= 'z')
					subn--;

				if (prev_subn != NULL && wcscmp(prev_subn, subn) != 0)
					AddTermNonTermSymbolsTableRow(L"NonTermsSpecialAlphaNumsDelim", SYMS_ROW_none, 1, NULL, &m_semi_light_delim_style);

				prev_subn = subn;
			}
			else if (m_non_terms_list_sort_mode == ntsm_sect_rule)
			{
				WORD rules_sect = m_grammar->non_terminals[sym-NonTerminalsBase].rules_sect;
				if (prev_rules_sect != 0xFFFF && rules_sect != prev_rules_sect)
				{
					AddTermNonTermSymbolsTableRow(L"NonTermsRuleSectsDelim1", SYMS_ROW_none, 2, NULL, &m_semi_light_delim_style);
					AddTermNonTermSymbolsTableRow(L"NonTermsRuleSectsDelim2", SYMS_ROW_none, 4, header_bkgr_style, &m_semi_light_delim_style);
					AddTermNonTermSymbolsTableRow(L"NonTermsRuleSectsDelim3", SYMS_ROW_none, 1, NULL, NULL);
				}

				prev_rules_sect = rules_sect;
			}

			TBasicStyle *row_bkgr = (sym == m_grm_symbols_list_frgr_slct) ? &m_light_delim_style : NULL;
			if (row_bkgr == NULL && m_non_terms_list_bkgr_slct != NULL)
			{
				for (int isym=0; isym<m_non_terms_list_bkgr_slct->NumItems(); ++isym)
				{
					if (m_non_terms_list_bkgr_slct->ItemRef(isym) == sym)
					{
						row_bkgr = &m_light_delim_style;
						break;
					}
				}
			}

			bool frgr_slct = (sym == m_grm_symbols_list_frgr_slct) ? TRUE : FALSE;
			if (AddNonTermSymbolsTableNonTermRow(sym, row_bkgr, frgr_slct, right_align, first_and_follows, non_term_non_term_starters) == FALSE)
				break;
		}
	}

	// Add the bottom frame rows.
	AddTermNonTermSymbolsTableRow(L"NonTermsBottomFrame1", SYMS_ROW_none, 2, NULL, &m_semi_light_delim_style);
	AddTermNonTermSymbolsTableRow(L"NonTermsBottomFrame1", SYMS_ROW_none, 14);

	// Finalize and display the grid.
	m_symbols_table_grid.UpdateGrid();
	m_view_helper.SetZoomLevel(m_zoom_level[m_dialog_mode]);
	m_view_helper.ClearInternalDataSet();
	m_view_helper.AddToInternalDataSet(m_symbols_table_grid);
	m_view_helper.ShowInternalDataSet(TRUE);
	m_local_console.CloseStatusPanel();
}

void TLangViewRulesGridDialog::ShowNonTerminalDependencies(TGrammar *grammar, WORD non_term)
{
	// Check/fix params.
	assert(grammar != NULL);
	assert(non_term >= NonTerminalsBase && non_term < NonTerminalsBase+grammar->NumNonTerminals());

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_non_term_depends, grammar, non_term);

	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Non terminal dependencies (sym=%hd, section=%hd)", non_term, grammar->non_terminals[non_term-NonTerminalsBase].rules_sect);
	PrepareNewDialogMode(mode_name_buff, L"Building dependencies ...");

	// Create the dependencies tree.
	TLangViewGrammarTreesHelper hlpr(this, m_grammar);
	hlpr.CreateNonTerminalDependenciesTree(&m_tree_items_root, non_term, m_non_term_deps_show_terms);

	if (m_owner->IsGrammarBrowser() == TRUE)
	{
		// The owner browser supports the grammar browser extensions. Create the list of used non terminals.
		TSymbolsArray *syms_list = new TSymbolsArray;
		if (syms_list != NULL)
		{
			TGenericTreeNodesIterator iter(&m_tree_items_root);
			for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
			{
				WORD sym_value = ((TLangViewTreeNode*)iter.GetCurrentItem())->GetSymbolValue();
				if (sym_value >= NonTerminalsBase && sym_value < NonTerminalsBase+grammar->NumNonTerminals())
				{
					if (syms_list->AppendItem(sym_value) == FALSE)
					{
						MsgBox(L"Out of memory while creating list of non terminal selections.");
						break;
					}
				}
			}

			// Highlight used non terminals in other child browser.
			TLangViewGrammarBrowser *frame = (TLangViewGrammarBrowser*)m_owner;
			frame->SetSymbolsListSelection(non_term, syms_list);
		}
		else
		{
			MsgBox(L"Unable to show the non terminal selections.");
		}
	}

	if (m_tree_placing_mode[m_dialog_mode] == plc_mode_rect_lines)
	{
		// The placing mode is rect lines. Build the second tree.
		TLangViewGrammarTreesHelper hlpr(this, m_grammar);
		hlpr.CreateNonTerminalRuleUsesTree(&m_aux_upper_tree_root, non_term);
		m_aux_upper_tree_root.Item().SetSimpleObjectProps(slit_center_button);
	}

	// Setup the root in the main tree.
	m_tree_items_root.Item().SetSimpleObjectProps(slit_center_button);

	// Display the tree.
	PrepareAndShowCurrentTree();
}

void TLangViewRulesGridDialog::ShowGrammarRules(TGrammar *grammar, int display_task)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixSimpleParam(display_task);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_grammar_rules, grammar, display_task);

	// Prepare title message and framing color.
	wchar_t mode_name_buff[120];
	TBasicStyle *row_hdr_color = NULL;
	if (display_task  < 0)
	{
		// Display all rules of the grammar.
		wcscpy(mode_name_buff, L"Grammar rules");
		row_hdr_color = &m_grammar_rules_hdr_style;
	}
	else if (display_task >= actb_reduce)
	{
		assert(display_task < actb_reduce+m_grammar->NumRules());

		// Display only one rule.
		swprintf(mode_name_buff, 120, L"Grammar rule (irule=%d)", display_task-actb_reduce);
		row_hdr_color = &m_grammar_rules_hdr_style;
	}
	else
	{
		// Ensure that passed param belongs to the current grammar as symbol value.
		assert(display_task < m_grammar->NumTerminals() || display_task >= NonTerminalsBase && display_task < NonTerminalsBase+grammar->non_terminals.NumItems());

		// Display symbol usage report.
		wchar_t buff40[40];
		swprintf(mode_name_buff, 120, L"Symbol use report for %s (sym=%d)", m_grammar->GetSymbolName(display_task, buff40), display_task);
		row_hdr_color = &m_symbol_use_rprt_hdr_style;
	}

	// Show the titles and prepare the console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing the grammar rules ...");

	if (m_grammar->NumRules() == 0)
	{
		// There is nothing to show.
		PrepareAndShowNoObjectsMessage(L"The grammar has no rules.");
		return;
	}
	else if (display_task == 0)
	{
		// There is nothing to show.
		PrepareAndShowNoObjectsMessage(L"The EOF cannot be present in the rules explicitly.");
		return;
	}

	// The rules are present in the passed grammar.
	if (PrepareGridCreation() == FALSE)
		return;

	// Select the transitions helper and prepare indexes there if this is needed.
	TLangViewBuildStateTransitionsHelper *psttr_hlpr = GetPreparedTransitionsHelper((display_task < actb_reduce) ? TRUE : FALSE);

	if (display_task >= actb_reduce)
	{
		// Show just one rule.
		int rule_index = display_task-actb_reduce;
		assert(rule_index < m_grammar->NumRules());

		ShowRulesViewRule(psttr_hlpr, rule_index, row_hdr_color);
	}
	else
	{
		// Show all rules or some of the rules.
		WORD symbol = (WORD)display_task;

		// Iterate all rules.
		for (int inx=0; inx<m_grammar->NumRules(); ++inx)
		{
			if (symbol == 0xFFFF)
			{
				// All rules should be displayed. Show the current rule.
				if (ShowRulesViewRule(psttr_hlpr, inx, row_hdr_color) == FALSE)
					break;
			}
			else
			{
				// Check if current rules should be displayed or not.
				bool show_rule = FALSE;
				TGrammarRule &rule = m_grammar->rules[inx];

				// Check if curr rule contains the requested symbol or not.
				bool sym_present = rule.symbols.IsSymInBuffer(symbol);
				if (m_sym_rprt_show_defn_rules == FALSE && m_sym_rprt_show_sym_uses == TRUE)
				{
					show_rule = (rule.non_term != symbol) && (sym_present == TRUE);
				}
				else if (m_sym_rprt_show_defn_rules == TRUE && m_sym_rprt_show_sym_uses == FALSE)
				{
					show_rule = (rule.non_term == symbol);
				}
				else if (m_sym_rprt_show_defn_rules == TRUE && m_sym_rprt_show_sym_uses == TRUE)
				{
					show_rule = (rule.non_term == symbol) || (sym_present == TRUE);
				}

				if (show_rule == TRUE)
				{
					// It is ok to show the curr rule.
					if (ShowRulesViewRule(psttr_hlpr, inx, row_hdr_color, (m_sym_rprt_highlight_sym == TRUE) ? symbol : 0xFFFF) == FALSE)
						break;
				}

				// Update rule count variables.
				if (rule.non_term == symbol)
					m_sym_rprt_defn_rules++;
				else if (sym_present == TRUE)
					m_sym_rprt_use_rules++;
			}

			// Check, maybe user wants to cancel.
			if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumRules()) == FALSE)
				break;
		}
	}

	// Add final lower delims.
	assert(row_hdr_color != NULL);
	AddThreeRowsFooter(L"RulesViewFinalDelim", 1, row_hdr_color);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
	psttr_hlpr->ClearResult();
}

void TLangViewRulesGridDialog::ShowParsingStates(TGrammar *grammar, int istate)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixSimpleParam(istate);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_parsing_states, grammar, istate);

	wchar_t mode_name_buff[120];
	if (istate < 0)
	{
		// Display all parsing states.
		swprintf(mode_name_buff, 120, L"Parsing states%s", (m_states_show_interm_positions == TRUE) ? L" (intermediate and final rule positions only)" : L"");
	}
	else
	{
		// Display only one state.
		swprintf(mode_name_buff, 120, L"Parsing state%s (istate=%d)", (m_states_show_interm_positions == TRUE) ? L" (non starting positions only)" : L"", istate);
	}

	// Show titles and prepare console.
	PrepareNewDialogMode(mode_name_buff, (istate < 0) ? L"Preparing parsing states ..." : L"Preparing parsing state ...");

	if (m_grammar->NumParsingStates() == 0)
	{
		// There is nothing to show.
		PrepareAndShowNoObjectsMessage(L"The grammar has no parsing states.");
		return;
	}
	else if (istate >= 0)
	{
		// Verify the value of the passed state with assert AFTER checking that the grammar has a parsing states table.
		assert(istate < m_grammar->NumParsingStates());
	}

	// Parsing states are present in the grammar.
	if (PrepareGridCreation() == FALSE)
		return;

	// Select transitions helper and prepare indexes there if this is needed.
	TLangViewBuildStateTransitionsHelper *psttr_hlpr = GetPreparedTransitionsHelper((istate >= 0) ? TRUE : FALSE);

	if (istate >= 0)
	{
		// Show just one parsing state.
		ShowStatesViewState(psttr_hlpr, istate, TRUE);
	}
	else
	{
		// Setup the width of the grid to the length of the longest rule at both sides of the parsing state ref column
		// because when the grid becomes very high it takes unacceptable long time to add new columns.
		int max_rule_len = 0;
		for (TRulesIterator iter(*m_grammar); iter; ++iter)
		{
			if (iter.CurrRule().Length() > max_rule_len)
				max_rule_len = iter.CurrRule().Length();
		}

		int iclmn_start_left = m_rules_grid.GetAlignmentSymIndex()-max_rule_len;
		bool left_clmns_success = AddMissingSymbolColumns(iclmn_start_left, max_rule_len);
		int iclmn_start_right = m_rules_grid.GetAlignmentSymIndex();
		bool right_clmns_success = AddMissingSymbolColumns(iclmn_start_right, max_rule_len);

		if (left_clmns_success == TRUE && right_clmns_success == TRUE)
		{
			// Iterate all parsing states.
			for (int inx=0; inx<m_grammar->NumParsingStates(); ++inx)
			{
				if (ShowStatesViewState(psttr_hlpr, inx, FALSE) == FALSE)
					break;

				if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumParsingStates()) == FALSE)
					break;
			}
		}
	}

	// Add final lower delims.
	AddThreeRowsFooter(L"ParsSttsViewFinalDelim", 4, &m_parsing_states_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
	psttr_hlpr->ClearResult();
}

void TLangViewRulesGridDialog::ShowConflictsAnalysisStatus(TGrammar *grammar)
{
	assert(grammar != NULL);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_processing_status, grammar);

	// Show titles and prepare console.
	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Grammar conflicts analysis status (igrm=%d)", m_grammar->GetGrammarIndex());
	PrepareNewDialogMode(mode_name_buff, L"Preparing the analysis status ...");

	if (CheckCurrGrammarForGrammarConflictsPresence(-4) == FALSE)
		return;

	// Conflicts are present. Collect the high level status.
	bool processing_started = FALSE, processing_finished = FALSE, processing_problems = FALSE;
	for (int iconflict=0; iconflict<m_grammar->NumConflicts(); ++iconflict)
	{
		// Collect info about the conflict and update the output vars.
		bool proc_started, proc_finished;
		processing_problems |= CheckConflictsAnalysisForConflict(iconflict, proc_started, proc_finished);
		processing_started |= proc_started;
		processing_finished |= proc_finished;
	}

	if (processing_started == FALSE)
	{
		PrepareAndShowNoObjectsMessage(L"Conflicts processing is not started", &m_analysis_warning_text_style);
		return;
	}
	else if (processing_finished == FALSE && processing_problems == FALSE)
	{
		PrepareAndShowNoObjectsMessage(L"Conflicts processing is running, no problems detected", &m_analysis_warning_text_style);
		return;
	}
	else if (processing_finished == TRUE && processing_problems == FALSE)
	{
		PrepareAndShowNoObjectsMessage(L"Conflicts processing fully succeeded", &m_analysis_warning_text_style);
		return;
	}

	// Processing problems are present.
	if (PrepareGridCreation() == FALSE)
		return;

	m_rules_grid.CreateRuleSymbolColumn(this, &m_symbols_vert_delim_style, TRUE);
	m_rules_grid.CreateRuleSymbolColumn(this, &m_symbols_vert_delim_style, TRUE);
	m_rules_grid.CreateRuleSymbolColumn(this, &m_symbols_vert_delim_style, TRUE);

	for (int inx=0; inx<m_grammar->NumConflicts(); ++inx)
	{
		bool proc_started, proc_finished;
		if (CheckConflictsAnalysisForConflict(inx, proc_started, proc_finished) == FALSE)
			continue;

		if (ShowGrammarConflictHeader(inx, &m_dark_delim_style, TRUE, TRUE) == FALSE)
			break;

		// Iterate conflicting actions.
		TBasicStyle *action_delim_style = NULL;
		for (int iact=0; iact<m_grammar->conflicts[inx].NumActions(); ++iact)
		{
			if (CheckConflictsAnalysisForAction(inx, iact, processing_started, processing_finished) == FALSE)
				continue;

			if (ShowConflicingActionHeader(inx, iact, action_delim_style) == FALSE)
				break;

			int status_row_inx = m_rules_grid.CreateRow(this, L"ConflictsAnalysisStatusRow", ROW_none, rule_body_row_height);
			if (status_row_inx < 0)
				break;

			// Show analysis result details.
			ShowConflictAnalysisDetails(status_row_inx, inx, iact);
			action_delim_style = &m_light_delim_style;
		}

		if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumConflicts()) == FALSE)
			break;
	}

	// Add final delims after the last conflict.
	AddThreeRowsFooter(L"ConflictsAnalysisStatusFinalDelim", 4, &m_grammar_cfct_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowXpctConflicts(TGrammar *grammar, int ixpct)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixSimpleParam(ixpct);

	// Reset prev contents.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_xpct_conflicts, grammar, ixpct);

	wchar_t mode_name_buff[120];
	if (ixpct  < 0)
	{
		wcscpy(mode_name_buff, L"Expected grammar conflicts");
	}
	else
	{
		assert(ixpct < m_grammar->NumXpctConflicts());
		swprintf(mode_name_buff, 120, L"Expected grammar conflict (ixpct=%d)", ixpct);
	}

	// Show titles and prepare console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing expected conflicts ...");

	if (CheckCurrGrammarForXpctConflictsPresence() == FALSE)
		return;

	// Expected conflicts are present in the grammar.
	if (PrepareGridCreation() == FALSE)
		return;

	bool cfcts_avail = FALSE;
	if (ixpct >= 0)
	{
		// Show just one expected conflict.
		ShowXpctConflictOverview(ixpct, cfcts_avail);
	}
	else
	{
		// Iterate all expected conflicts.
		for (int inx=0; inx<m_grammar->NumXpctConflicts(); ++inx)
		{
			if (ShowXpctConflictOverview(inx, cfcts_avail) == FALSE)
				break;

			if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumXpctConflicts()) == FALSE)
				break;
		}
	}

	// Add final delims after the last xpct conflict.
	AddThreeRowsFooter(L"XpctCfctsViewFinalDelim", 4, (cfcts_avail == TRUE) ? &m_grammar_cfct_hdr_style : &m_xpct_conflict_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowGrammarConflicts(TGrammar *grammar, int display_task)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixGrammarConflictParam(display_task);

	// Reset prev contents.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_grammar_conflicts, grammar, display_task);

	wchar_t mode_name_buff[120];
	if (display_task  == -4)
	{
		wcscpy(mode_name_buff, L"Grammar conflicts");
	}
	else if (display_task == -3)
	{
		wcscpy(mode_name_buff, L"Ambiguous grammar conflicts");
	}
	else if (display_task == -2)
	{
		wcscpy(mode_name_buff, L"Grammar conflicts that are owned by one expected conflict");
	}
	else if (display_task == -1)
	{
		wcscpy(mode_name_buff, L"Unexpected grammar conflicts");
	}
	else
	{
		assert(display_task < m_grammar->NumConflicts());
		swprintf(mode_name_buff, 120, L"Grammar conflict (iconflict=%d)", display_task);
	}

	// Show the titles and prepare the console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing grammar conflicts ...");

	if (CheckCurrGrammarForGrammarConflictsPresence(display_task) == FALSE)
		return;

	// Single conflict or conflicts of requested category are avilable. Check presence of requested conflicts
	// in case if analysis results presence if restriction is currently active.
	if (display_task < 0 && m_show_with_analysis_only == TRUE)
	{
		bool restricted_conflicts_are_present = FALSE;
		int iconflict = m_grammar->GetFirstGrammarConflictInx(display_task);
		while (iconflict >= 0)
		{
			if (CheckConflictAnalysisResultsPresence(iconflict) == TRUE)
			{
				restricted_conflicts_are_present = TRUE;
				break;
			}

			iconflict = m_grammar->GetNextGrammarConflictInx(display_task, iconflict);
		}

		if (restricted_conflicts_are_present == FALSE)
		{
			PrepareAndShowNoObjectsMessage(L"Grammar conflicts with analysis results are missing.");
			return;
		}
	}

	// The grammar conflicts are present in the current grammar.
	if (PrepareGridCreation() == FALSE)
		return;

	if (display_task <= -4)
	{
		// Iterate all grammar conflicts.
		for (int inx=0; inx<m_grammar->NumConflicts(); ++inx)
		{
			if (m_show_with_analysis_only == TRUE && CheckConflictAnalysisResultsPresence(inx) == FALSE)
				continue;

			if (ShowGrammarConflictOverview(inx, &m_dark_delim_style, TRUE) == FALSE)
				break;

			if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumConflicts()) == FALSE)
				break;
		}
	}
	else if (display_task < 0)
	{
		// Display all conflicts that belong to a certain category.
		int iconflict = m_grammar->GetFirstGrammarConflictInx(display_task);
		while (iconflict >= 0)
		{
			if (m_show_with_analysis_only == FALSE || CheckConflictAnalysisResultsPresence(iconflict) == TRUE)
			{
				if (ShowGrammarConflictOverview(iconflict, &m_dark_delim_style, TRUE) == FALSE)
					break;

				if (m_local_console.UpdateStatusPanel(iconflict, m_grammar->NumConflicts()) == FALSE)
					break;
			}

			iconflict = m_grammar->GetNextGrammarConflictInx(display_task, iconflict);
		}
	}
	else
	{
		// Show just one grammar conflict.
		ShowGrammarConflictOverview(display_task, &m_dark_delim_style, TRUE);
	}

	// Add final delims after the last conflict.
	AddThreeRowsFooter(L"ConflictsViewFinalDelim", 4, &m_grammar_cfct_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowKeyRulesForXpctConflicts(TGrammar *grammar, int ixpct)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixSimpleParam(ixpct);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_key_rules_xpct, grammar, ixpct);

	wchar_t mode_name_buff[120];
	if (ixpct  < 0)
	{
		wcscpy(mode_name_buff, L"Key conflicting rules for expected grammar conflicts");
	}
	else
	{
		assert(ixpct < m_grammar->NumXpctConflicts());
		swprintf(mode_name_buff, 120, L"Key conflicting rules for expected grammar conflict (ixpct=%d)", ixpct);
	}

	// Show titles and prepare console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing expected conflicts ...");

	if (CheckCurrGrammarForXpctConflictsPresence() == FALSE)
		return;

	// Expected grammar conflicts are present in the grammar.
	if (PrepareGridCreation() == FALSE)
		return;

	bool cfcts_avail = FALSE;
	if (ixpct >= 0)
	{
		// Show just one expected conflict.
		ShowXpctConflictKeyRules(ixpct, cfcts_avail);
	}
	else
	{
		// Iterate all expected conflicts.
		for (int inx=0; inx<m_grammar->NumXpctConflicts(); ++inx)
		{
			if (ShowXpctConflictKeyRules(inx, cfcts_avail) == FALSE)
				break;

			if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumXpctConflicts()) == FALSE)
				break;
		}
	}

	// Add final delims after the last xpct conflict.
	AddThreeRowsFooter(L"KeyConflicingRulesViewFinalDelim", 4, (cfcts_avail == TRUE) ? &m_grammar_cfct_hdr_style : &m_xpct_conflict_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowKeyRulesForGrammarConflicts(TGrammar *grammar, int display_task)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixGrammarConflictParam(display_task);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_key_rules_conflict, grammar, display_task);

	wchar_t mode_name_buff[120];
	if (display_task  < 0)
	{
		wcscpy(mode_name_buff, L"Key conflicting rules for grammar conflicts");
	}
	else
	{
		assert(display_task < m_grammar->NumConflicts());
		swprintf(mode_name_buff, 120, L"Key conflicting rules for grammar conflict (iconflict=%d)", display_task);
	}

	// Show titles and prepare console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing grammar conflicts ...");

	if (CheckCurrGrammarForGrammarConflictsPresence(display_task) == FALSE)
		return;

	// Grammar conflicts are present in the current grammar.
	if (PrepareGridCreation() == FALSE)
		return;

	if (display_task <= -4)
	{
		// Iterate all grammar conflicts.
		for (int inx=0; inx<m_grammar->NumConflicts(); ++inx)
		{
			if (ShowGrammarConflictKeyRules(inx, -1) == FALSE)
				break;

			if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumConflicts()) == FALSE)
				break;
		}
	}
	else if (display_task < 0)
	{
		// Display all conflicts that belong to the certain category.
		int iconflict = m_grammar->GetFirstGrammarConflictInx(display_task);
		while (iconflict >= 0)
		{
			if (ShowGrammarConflictKeyRules(iconflict, -1) == FALSE)
				break;

			if (m_local_console.UpdateStatusPanel(iconflict, m_grammar->NumConflicts()) == FALSE)
				break;

			iconflict = m_grammar->GetNextGrammarConflictInx(display_task, iconflict);
		}
	}
	else
	{
		// Show just one grammar conflict.
		ShowGrammarConflictKeyRules(display_task, -1);
	}

	// Add final delims after the last rule.
	AddThreeRowsFooter(L"KeyConflicingRulesViewFinalDelim", 4, &m_grammar_cfct_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowDiffersTreeForXpctConflicts(TGrammar *grammar, int ixpct)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixSimpleParam(ixpct);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_dplist_differs_xpct, grammar, ixpct);

	wchar_t mode_name_buff[120];
	if (ixpct  < 0)
	{
		wcscpy(mode_name_buff, L"Derivations paths tree for expected grammar conflicts");
	}
	else
	{
		assert(ixpct < m_grammar->NumXpctConflicts());
		swprintf(mode_name_buff, 120, L"Derivations paths tree for expected grammar conflict (ixpct=%d)", ixpct);
	}

	// Show titles and prepare console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing paths tree ...");

	// Ensure that expected conflicts are present.
	if (CheckCurrGrammarForXpctConflictsPresence() == FALSE)
		return;

	// Create the tree.
	TLangViewGrammarTreesHelper hlpr(this, m_grammar);
	hlpr.CreatePathsTreeForXpctConflict(&m_tree_items_root, ixpct);

	// Display the tree.
	PrepareAndShowCurrentTree();
}

void TLangViewRulesGridDialog::ShowDiffersTreeForGrammarConflicts(TGrammar *grammar, int display_task)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixGrammarConflictParam(display_task);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_dplist_differs_conflict, grammar, display_task);

	wchar_t mode_name_buff[120];
	if (display_task  < 0)
	{
		wcscpy(mode_name_buff, L"Derivations paths tree for grammar conflicts");
	}
	else
	{
		assert(display_task < m_grammar->NumConflicts());
		swprintf(mode_name_buff, 120, L"Derivations paths tree for grammar conflict (iconflict=%d)", display_task);
	}

	// Show titles and prepare console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing paths tree ...");

	// Ensure that grammar conflicts are present.
	if (CheckCurrGrammarForGrammarConflictsPresence(display_task) == FALSE)
		return;

	// Create the tree.
	TLangViewGrammarTreesHelper hlpr(this, m_grammar);
	hlpr.CreatePathsTreeForGrammarConflict(&m_tree_items_root, display_task);

	// Display the tree.
	PrepareAndShowCurrentTree();
}

void TLangViewRulesGridDialog::ShowSymbolSeqsForXpctConflicts(TGrammar *grammar, int ixpct)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixSimpleParam(ixpct);

	// Reset prev contents.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_dplist_symbols_xpct, grammar, ixpct);

	wchar_t mode_name_buff[120];
	if (ixpct  < 0)
	{
		wcscpy(mode_name_buff, L"Derivation paths list for expected grammar conflicts");
	}
	else
	{
		assert(ixpct < m_grammar->NumXpctConflicts());
		swprintf(mode_name_buff, 120, L"Derivation paths list for expected grammar conflict (ixpct=%d)", ixpct);
	}

	// Show titles and prepare console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing symbol sequencies ...");

	// Ensure that expected conflicts are present.
	if (CheckCurrGrammarForXpctConflictsPresence() == FALSE)
		return;

	if (PrepareGridCreation() == FALSE)
		return;

	bool cfcts_avail = FALSE;
	if (ixpct >= 0)
	{
		// Show just one expected conflict.
		ShowXpctConflictSymbolSeqs(ixpct, cfcts_avail);
	}
	else
	{
		// Iterate all expected conflicts.
		for (int inx=0; inx<m_grammar->NumXpctConflicts(); ++inx)
		{
			if (ShowXpctConflictSymbolSeqs(inx, cfcts_avail) == FALSE)
				break;

			if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumXpctConflicts()) == FALSE)
				break;
		}
	}

	// Add final delims after the last xpct conflict.
	AddThreeRowsFooter(L"KeyConflicingRulesViewFinalDelim", 4, (cfcts_avail == TRUE) ? &m_grammar_cfct_hdr_style : &m_xpct_conflict_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowSymbolSeqsForGrammarConflicts(TGrammar *grammar, int display_task)
{
	// Check/fix params.
	assert(grammar != NULL);
	FixGrammarConflictParam(display_task);

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_dplist_symbols_conflict, grammar, display_task);

	wchar_t mode_name_buff[120];
	if (display_task  < 0)
	{
		wcscpy(mode_name_buff, L"Derivation paths list for grammar conflicts");
	}
	else
	{
		assert(display_task < m_grammar->NumConflicts());
		swprintf(mode_name_buff, 120, L"Derivation paths list for grammar conflict (iconflict=%d)", display_task);
	}

	// Show titles and prepare console.
	PrepareNewDialogMode(mode_name_buff, L"Preparing symbol sequencies ...");

	// Ensure that grammar conflicts are present.
	if (CheckCurrGrammarForGrammarConflictsPresence(display_task) == FALSE)
		return;

	if (PrepareGridCreation() == FALSE)
		return;

	if (display_task <= -4)
	{
		// Iterate all grammar conflicts.
		for (int inx=0; inx<m_grammar->NumConflicts(); ++inx)
		{
			if (ShowGrammarConflictSymbolSeqs(inx, &m_dark_delim_style, TRUE) == FALSE)
				break;

			if (m_local_console.UpdateStatusPanel(inx, m_grammar->NumConflicts()) == FALSE)
				break;
		}
	}
	else if (display_task < 0)
	{
		// Display all conflicts that belong to the certain category.
		int iconflict = m_grammar->GetFirstGrammarConflictInx(display_task);
		while (iconflict >= 0)
		{
			if (ShowGrammarConflictSymbolSeqs(iconflict, &m_dark_delim_style, TRUE) == FALSE)
				break;

			if (m_local_console.UpdateStatusPanel(iconflict, m_grammar->NumConflicts()) == FALSE)
				break;

			iconflict = m_grammar->GetNextGrammarConflictInx(display_task, iconflict);
		}
	}
	else
	{
		// Show just one grammar conflict.
		ShowGrammarConflictSymbolSeqs(display_task, &m_dark_delim_style, TRUE);
	}

	// Add final delims after the last seq.
	AddThreeRowsFooter(L"KeyConflicingRulesViewFinalDelim", 4, &m_grammar_cfct_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowDiffersTreeForConflictingAction(TGrammar *grammar, int iconflict, int iaction)
{
	// Check params.
	assert(grammar != NULL);
	assert(grammar->grm_props.processing_result == grpr_full_success);
	assert(iconflict >= 0 && iconflict < grammar->NumConflicts());
	assert(iaction >= 0 && iaction < grammar->conflicts[iconflict].NumActions());

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_dplist_differs_action, grammar, iconflict, iaction);

	// Show titles and prepare console.
	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Derivations paths tree for conflicting action (iconflict=%d, iaction=%d)", iconflict, iaction);
	PrepareNewDialogMode(mode_name_buff, L"Preparing paths tree ...");

	// Create the tree.
	TLangViewGrammarTreesHelper hlpr(this, m_grammar);
	hlpr.CreatePathsTreeForConflictingAction(&m_tree_items_root, iconflict, iaction);

	// Display the tree.
	PrepareAndShowCurrentTree(g_RulesView_DiffersTreeForConflictingAction_MinLayerSteps);
}

void TLangViewRulesGridDialog::ShowSymbolSeqsForConflictingAction(TGrammar *grammar, int iconflict, int iaction)
{
	// Check params.
	assert(grammar != NULL);
	assert(grammar->grm_props.processing_result == grpr_full_success);
	assert(iconflict >= 0 && iconflict < grammar->NumConflicts());
	assert(iaction >= 0 && iaction < grammar->conflicts[iconflict].NumActions());

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_dplist_symbols_action, grammar, iconflict, iaction);

	// Show titles and prepare console.
	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Derivation paths list for conflicting action (iconflict=%d, iaction=%d)", iconflict, iaction);
	PrepareNewDialogMode(mode_name_buff, L"Preparing symbol sequences ...");

	// Create the initial layout.
	if (PrepareGridCreation() == FALSE)
		return;

	// Display the conflict header.
	ShowGrammarConflictHeader(iconflict, &m_dark_delim_style, TRUE, TRUE);

	// Show paths for one action.
	ShowConflictingActionSymbolSeqs(iconflict, iaction, NULL);

	// Add final delims after the last position.
	m_rules_grid.RemoveLastRowRowDelimiter();
	AddThreeRowsFooter(L"CfctingActionPathsAsSymbolsSeqFinalDelim", 4, &m_grammar_cfct_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowDerivationPathAsAst(TGrammar *grammar, int iconflict, int iaction, int ipath)
{
	assert(grammar != NULL);
	assert(grammar->grm_props.processing_result == grpr_full_success);
	assert(iconflict >= 0 && iconflict < grammar->NumConflicts());
	assert(iaction >= 0 && iaction < grammar->conflicts[iconflict].NumActions());
	assert(grammar->conflicts[iconflict].analysis_results.NumItems() == grammar->conflicts[iconflict].NumActions());
	assert(ipath >= 0 && ipath < grammar->conflicts[iconflict].analysis_results[iaction].m_derivation_paths.NumItems());

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_deriv_path_ast, grammar, iconflict, iaction, ipath);

	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Derivations path as abstract syntax tree (iconflict=%d, iaction=%d, ipath=%d)", iconflict, iaction, ipath);
	PrepareNewDialogMode(mode_name_buff, L"Preparing AST ...");

	// Pick up the derivation path.
	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
	TGrammarDerivationPath &path = cfct.analysis_results[iaction].m_derivation_paths[ipath];

	// Create the AST tree.
	TLangViewGrammarTreesHelper hlpr(this, m_grammar);
	TLangViewTreeNode *conflicting_symbol = NULL;
	hlpr.CreateDerivationPathAst(&m_tree_items_root, path, cfct.input_symbol, conflicting_symbol);

	// Setup the root.
	m_tree_items_root.Item().SetDerivationPathProps(m_grammar->GetGrammarIndex(), iconflict, iaction, ipath);

	// Display the tree.
	PrepareAndShowCurrentTree(NULL, conflicting_symbol);
}

void TLangViewRulesGridDialog::ShowDerivationPathAsSeq(TGrammar *grammar, int iconflict, int iaction, int ipath)
{
	assert(grammar != NULL);
	assert(grammar->grm_props.processing_result == grpr_full_success);
	assert(iconflict >= 0 && iconflict < grammar->NumConflicts());
	assert(iaction >= 0 && iaction < grammar->conflicts[iconflict].NumActions());
	assert(grammar->conflicts[iconflict].analysis_results.NumItems() == grammar->conflicts[iconflict].NumActions());
	assert(ipath >= 0 && ipath < grammar->conflicts[iconflict].analysis_results[iaction].m_derivation_paths.NumItems());

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_deriv_path_seq, grammar, iconflict, iaction, ipath);

	// Show titles and prepare console.
	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Derivation path as symbols sequence (iconflict=%d, iaction=%d, ipath=%d)", iconflict, iaction, ipath);
	PrepareNewDialogMode(mode_name_buff, L"Preparing symbol sequences ...");

	// Create initial layout.
	if (PrepareGridCreation() == FALSE)
		return;

	// Show simple header.
	m_rules_grid.CreateRow(this, L"DervPathAsSymsSeq1", ROW_derivation_path_body, 1, &m_light_delim_style, NULL, iconflict, iaction, ipath);

	bool want_4pixel_horz_delims = m_show_path_seq_props == TRUE || m_show_single_path_seq_lex_items == TRUE;
	if (want_4pixel_horz_delims == TRUE)
	{
		CreateHeaderRows(L"DervPathAsSymsSeq2", ROW_derivation_path_body, &m_dark_delim_style, 1, &m_derivation_path_hdr_style, &m_light_delim_style, iconflict, iaction, ipath);
	}

	// Display one derivation path.
	ShowDerivationPathAsSymbolsSeq(iconflict, iaction, ipath, m_show_single_path_seq_lex_items);

	// Add final delims.
	AddThreeRowsFooter(L"CfctingActionPathsAsSymbolsSeqFinalDelim", 1, &m_derivation_path_hdr_style, want_4pixel_horz_delims);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

void TLangViewRulesGridDialog::ShowDerivationPathAsSteps(TGrammar *grammar, int iconflict, int iaction, int ipath)
{
	// Path with passed indexes should exist in the current grammar. Ensure this.
	assert(grammar != NULL);
	assert(grammar->grm_props.processing_result == grpr_full_success);
	assert(iconflict >= 0 && iconflict < grammar->NumConflicts());
	assert(iaction >= 0 && iaction < grammar->conflicts[iconflict].NumActions());
	assert(grammar->conflicts[iconflict].analysis_results.NumItems() == grammar->conflicts[iconflict].NumActions());
	assert(ipath >= 0 && ipath < grammar->conflicts[iconflict].analysis_results[iaction].m_derivation_paths.NumItems());

	// Reset prev contents and setup the data fields.
	ResetContentsInternal();
	StoreMajorTaskParams(grdm_deriv_path_steps, grammar, iconflict, iaction, ipath);

	wchar_t mode_name_buff[120];
	swprintf(mode_name_buff, 120, L"Derivations path as list of steps (iconflict=%d, iaction=%d, ipath=%d - %s)", iconflict, iaction, ipath, (m_show_deriv_path_dtls == TRUE) ? L"all steps" : L"brief list");
	PrepareNewDialogMode(mode_name_buff, L"Preparing derivation path ...");

	// Create initial layout.
	if (PrepareGridCreation() == FALSE)
		return;

	// Add one more light line to balance the picture. The whole picture should end up that on the top, between
	// the steps and at the bottom all delimiters should consist of 4 lines of one pixel high each.
	m_rules_grid.CreateRow(this, L"PathAsStepsHdr", ROW_none, 1, &m_light_delim_style);

	// Pick up the derivation path.
	TGrammarConflict &cfct = grammar->conflicts[iconflict];
	TGrammarDerivationPath &path = cfct.analysis_results[iaction].m_derivation_paths[ipath];
	int hidden_conflict_step = path.GetHiddenConflictPosition();
	assert(hidden_conflict_step >= 0);

	// First step of the path is appended to the bottom of the grid regardless of display order. If display order
	// is from bottom to top, then at the end of processing the first step, this var will be replaced with the index
	// of the top row that belongs to the step, that was just generated.
	int prev_step_ins_before_row = SCRITEMS_GRID_APPEND;

	// Iterate and display the passed derivation path.
	int cnt_steps = 0;
	for (int istep=0; istep<path.derv_path_len; ++istep)
	{
		// Make copy of the current step in the local variables.
		TFullRulePosition step = path.derv_steps[istep];
		int spec_irule = -1;

		if (m_show_deriv_path_dtls == FALSE)
		{
			//
			// Only some of the steps should be displayed. Check for special sequences of derivation steps.
			//
			if (step.origin == rpo_initial_placement || step.origin == rpo_rule_call_place && istep == 0)
			{
				// Initial placement should be always displayed. Second condition of the if stmt above
				// is representation of initial placement for the conflicting action "shift".
			}
			else if (step.origin == rpo_rule_call_place && istep<path.derv_path_len-1 && path.derv_steps[istep+1].origin == rpo_step_over_symbol)
			{
				// Show these 2 steps as one step.
				assert(istep > 0);
				spec_irule = path.derv_steps[istep-1].irule;
				step = path.derv_steps[istep+1];
				istep += 1;
			}
			else if (step.origin == rpo_step_over_non_term || step.origin == rpo_non_term_defn_rule || step.origin == rpo_call_place_ctx)
			{
				// Show these slots.
			}
			else if (step.origin == rpo_axioma_action_sym || step.origin == rpo_axioma_action_ctx)
			{
				// These step types are rare. Show them.
			}
			else
			{
				// All other steps should not be displayed. Continue to the next step of the path.
				continue;
			}
		}

		// Current step will be displayed.
		int rule_body_row_inx = -1;
		int curr_step_header_row = -1;
		if (step.origin != rpo_axioma_action_sym && step.origin != rpo_axioma_action_ctx)
		{
			// This is normal step that has rule position.
			bool hgtlt_irule = FALSE;
			if (istep == hidden_conflict_step)
				hgtlt_irule = TRUE;

			// Create rows for the derivation step header.
			curr_step_header_row = CreateHeaderRows(L"DerivationStepHeader", ROW_grammar_rule_body, &m_dark_delim_style, 1, &m_derivation_path_hdr_style, &m_light_delim_style, step.irule, 0, 0, prev_step_ins_before_row);
			if (curr_step_header_row < 0)
				break;

			// Show the rule body.
			rule_body_row_inx = ShowRuleBody(curr_step_header_row+1, step.irule, m_rules_grid.GetAlignmentSymIndex()-step.isym, 0xFFFF, hgtlt_irule);
			if (rule_body_row_inx < 0)
				break;
		}
		else
		{
			// These types of steps do not have rule. Create rows and show one symbol there.
			curr_step_header_row = CreateHeaderRows(L"AxiomaActionSymRowHeader", ROW_none, &m_dark_delim_style, 1, &m_derivation_path_hdr_style, &m_light_delim_style, 0, 0, 0, prev_step_ins_before_row);
			if (curr_step_header_row < 0)
				break;

			rule_body_row_inx = m_rules_grid.CreateRow(this, L"AxiomaActionSymRow", ROW_none, rule_body_row_height, NULL, NULL, 0, 0, 0, curr_step_header_row+1);
			if (rule_body_row_inx < 0)
				break;

			if (ShowGrammarSymbols(rule_body_row_inx, m_rules_grid.GetAlignmentSymIndex(), &(step.action_sym), 1, TRUE) == FALSE)
				break;
		}

		// Show info about the origin of the curr step.
		if (ShowDerivationStepDetails(rule_body_row_inx, ipath, ++cnt_steps, step, spec_irule) == FALSE)
			break;

		// Show parsing state of the step as a small label.
		if (ShowParsingStateOnClmnRef(curr_step_header_row+2, step.istate, ROW_grammar_rule_body, step.irule) == FALSE)
			break;

		// If the order of painting steps is revesed, it is necessary to update the row index for the next step.
		if (m_show_deriv_path_from_bottom == TRUE)
		{
			prev_step_ins_before_row = curr_step_header_row-1;
		}
	}

	// Add final delims after the last step.
	AddThreeRowsFooter(L"DerivationPathFinalDelim", 1, &m_derivation_path_hdr_style);

	// Refresh column widths and show the grid.
	PrepareAndShowCurrentGrid();
}

//
//  ===========================  Section 2  ==============================
//

bool TLangViewRulesGridDialog::ShowRulesViewRule(TLangViewBuildStateTransitionsHelper *sttr_hlpr, int irule, TBasicStyle *row_hdr_color, WORD symbol_to_highlight)
{
	assert(m_dialog_mode == grdm_grammar_rules);

	// Create rows for the rule header.
	if (CreateHeaderRows(L"RuleViewHeader", ROW_grammar_rule_body, &m_dark_delim_style, 1, row_hdr_color, &m_light_delim_style, irule) < 0)
		return(FALSE);

	// Show the rule body.
	if (ShowRuleBody(SCRITEMS_GRID_APPEND, irule, 0, symbol_to_highlight) < 0)
		return(FALSE);

	// Show parsing states that are applicable to this rule if any.
	if (ShowStateTransitionRows(sttr_hlpr, 0, irule, 0, 0xFFFF, 0xFFFF, FALSE, TRUE) == FALSE)
		return(FALSE);

	// Add space below the state transition rows if any.
	if (m_rules_grid.CreateRow(this, L"StateTransitionsLowerDelim1", ROW_grammar_rule_body, 4, NULL, NULL, irule) < 0)
		return(FALSE);

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowStatesViewState(TLangViewBuildStateTransitionsHelper *sttr_hlpr, int istate, bool show_states)
{
	assert(m_dialog_mode == grdm_parsing_states);
	assert(istate >= 0 && istate < m_grammar->NumParsingStates());

	// Create rows for the parsing state header.
	int hdr_row_inx = CreateHeaderRows(L"ParsingStatesViewHeader", ROW_parsing_state_header, &m_dark_delim_style, 21, &m_parsing_states_hdr_style, &m_light_delim_style, istate);
	if (hdr_row_inx < 0)
	{
		// Erorr is already reported.
		return(FALSE);
	}

	// Show state index.
	if (CreateCenteredTextLabel(L"ParsSttIndex1", hdr_row_inx, m_rules_grid.RuleIndexClmn(), 0, &m_rule_index_text_style, L"State %d", istate) == FALSE)
		return(FALSE);

	if (m_single_obj_index == -1)
	{
		// When all states are displayed, state number should be displayed once again in the trailing column.
		if (CreateCenteredTextLabel(L"ParsSttIndex2", hdr_row_inx, m_rules_grid.TrailingClmn(), 0, &m_rule_index_text_style, L"State %d", istate) == FALSE)
			return(FALSE);
	}

	// Iterate positions of the passed state.
	TParsingState &stt = m_grammar->parsing_states[istate];
	WORD prev_irule = 0xFFFF, prev_isym = 0xFFFF;
	int rules_found = 0;

	for (int ipos=0; ipos<stt.NumItems(); ++ipos)
	{
		WORD isym = stt[ipos].isym;
		if (stt[ipos].irule == prev_irule && isym == prev_isym)
		{
			// Skip positions that differ only by the rule action symbol. State transitions display procedure
			// will show available action symbols.
			continue;
		}
		else if (m_states_show_interm_positions == TRUE && isym == 0)
		{
			// Current mode asks to skip starting rule positions.
			continue;
		}

		if (show_states == TRUE)
		{
			if (ShowRuleBody(SCRITEMS_GRID_APPEND, stt[ipos].irule, m_rules_grid.GetAlignmentSymIndex()-isym) < 0)
				return(FALSE);

			// Look for other states that the current rule may have.
			if (ShowStateTransitionRows(sttr_hlpr, m_rules_grid.GetAlignmentSymIndex()-isym, stt[ipos].irule, isym, istate, 0xFFFF, TRUE, TRUE) == FALSE)
				return(FALSE);

			// Add spacer with delimiter below the rule parsing states if any.
			if (m_rules_grid.CreateRow(this, L"StateTransitionsLowerDelim2", ROW_grammar_rule_body, 3, NULL, &m_light_delim_style, stt[ipos].irule) < 0)
				return(FALSE);
		}
		else
		{
			int irow = ShowRuleBody(SCRITEMS_GRID_APPEND, stt[ipos].irule, m_rules_grid.GetAlignmentSymIndex()-isym, 0xFFFF, FALSE, TRUE);
			if (irow < 0)
				return(FALSE);

			// Show current parsing state as a small label.
			if (ShowParsingStateLabelOnClmnRef(irow, istate) == FALSE)
				return(FALSE);
		}

		rules_found++;
		prev_irule = stt[ipos].irule;
		prev_isym = isym;
	}

	// Check the number of displayed rules.
	if (rules_found > 0)
	{
		// Last rule should have no grey delimiter below it.
		m_rules_grid.RemoveLastRowRowDelimiter();

		// Show the number of rules for this state.
		if (CreateCenteredTextLabel(L"ParsSttNumRules", hdr_row_inx, m_rules_grid.RuleNonTermClmn(), 0, &m_small_comment_text_style, L"%d rule%s", rules_found, PluralSuffix(rules_found)) == FALSE)
			return(FALSE);
	}
	else
	{
		if (m_states_show_interm_positions == TRUE && istate == 0)
		{
			// The situation is ok. Show the explanation message.
			if (CreateMessageRow(L"ParsSttRulesMissing1", ROW_none, m_rules_grid.RuleSymClmn(0), 6, L"Parsing state 0 cannot be present in the inermediate or final position of the rule.") == FALSE)
				return(FALSE);
		}
		else
		{
			MsgBoxFmt(MB_OK, L"Rules for the parsing state %d are missing.", istate);
		}
	}

	// Success.
	return(TRUE);
}

int TLangViewRulesGridDialog::ShowRuleBody(int ins_before_row, int irule, int iclmn_start, WORD symbol_to_highlight, bool key_conflicting_rule, bool want_spc_for_stt_on_clmn_ref_and_delim)
{
	assert(m_grammar != NULL);
	assert(irule >= 0 && irule < m_grammar->NumRules());

	// Pick up the rule info.
	TGrammarRule &rule = m_grammar->rules[irule];
	int rule_len = m_grammar->rules[irule].Length();

	// Create row for the rule body.
	short extra_row_hgt = (want_spc_for_stt_on_clmn_ref_and_delim == TRUE) ? pars_stt_on_clnm_ref_row_height : 0;
	TBasicStyle *row_bkgr = (want_spc_for_stt_on_clmn_ref_and_delim == TRUE) ? &m_light_delim_style : NULL;
	int rule_row_inx = m_rules_grid.CreateRow(this, L"RuleBodyRow", ROW_grammar_rule_body, rule_body_row_height+extra_row_hgt, NULL, row_bkgr, irule, 0, 0, ins_before_row);
	if (rule_row_inx < 0)
	{
		// Erorr is already reported.
		return(-1);
	}

	// Check columns presence.
	if (AddMissingSymbolColumns(iclmn_start, rule_len) == FALSE)
		return(-1);

	// Show the first rule index.
	TTextStyle *rule_label_style = (key_conflicting_rule == TRUE) ? &m_key_rule_index_text_style : &m_rule_index_text_style;
	if (CreateCenteredTextLabel(L"FirstRuleIndex", rule_row_inx, m_rules_grid.RuleIndexClmn(), extra_row_hgt, rule_label_style, L"R%d", irule) == FALSE)
		return(-1);

	if (m_dialog_mode == grdm_parsing_states && m_single_obj_index == -1)
	{
		// Show the second rule index.
		if (CreateCenteredTextLabel(L"SecondRuleIndex", rule_row_inx, m_rules_grid.TrailingClmn(), extra_row_hgt, rule_label_style, L"R%d", irule) == FALSE)
			return(-1);
	}

	// Show left hand side rule symbol.
	short off_bottom;
	TScreenItem *non_term_scr_item = CreateSymbolLexItem(rule.non_term, off_bottom);
	if (non_term_scr_item == NULL)
		return(-1);

	off_bottom += extra_row_hgt;
	m_rules_grid.SetupCenteredCell(this, L"RuleNonterm", rule_row_inx, m_rules_grid.RuleNonTermClmn(), non_term_scr_item, off_bottom);
	if (rule.non_term == symbol_to_highlight)
		AddGrammarSymbolSelection(rule_row_inx, m_rules_grid.RuleNonTermClmn());

	// Fill in the right hand side rule symbols.
	for (int isym=0; isym<rule_len; ++isym)
	{
		// Create and show screen item for the rule symbol.
		WORD sym = rule.symbols[isym];
		TScreenItem *scr_item = CreateSymbolLexItem(sym, off_bottom);
		if (scr_item == NULL)
			return(-1);

		m_rules_grid.SetupCenteredCell(this, L"RuleSymbol", rule_row_inx, m_rules_grid.RuleSymClmn(iclmn_start+isym), scr_item, off_bottom);
		if (sym == symbol_to_highlight)
			AddGrammarSymbolSelection(rule_row_inx, m_rules_grid.RuleSymClmn(iclmn_start+isym));
	}

	// Full success.
	return(rule_row_inx);
}

bool TLangViewRulesGridDialog::ShowParsingStateOnClmnRef(int ins_before_row, WORD istate, TLangViewRulesGridRowMeaning row_meaning, WORD inx2, WORD inx3, int inx4)
{
	// Add small space below the rule body and the footer delimiter.
	int stt_irow = m_rules_grid.CreateRow(this, L"ClmnRefParsSttLowerDelim", row_meaning, pars_stt_on_clnm_ref_row_height, NULL, &m_light_delim_style,  inx2, inx3, inx4, ins_before_row);
	if (stt_irow < 0)
		return(FALSE);

	// Create parsing state label object.
	return(ShowParsingStateLabelOnClmnRef(stt_irow, istate));
}

bool TLangViewRulesGridDialog::ShowParsingStateLabelOnClmnRef(int irow, WORD istate)
{
	// Create parsing state label object.
	TGrammarViewLabelItem *state_scr_item = CreateSpecLabelItem(L"ClmnRefParsStt", TGrammarViewLabelItem::gvlt_parsing_state, istate);
	if (state_scr_item == NULL)
		return(FALSE);

	// Pick up the ref clmn width.
	int iclmn = m_rules_grid.ParsSttClmn(m_rules_grid.GetAlignmentSymIndex());
	TScreenItemsGridColumnInfo clmn_info;
	m_rules_grid.GetGrid().GetColumnInfo(iclmn, clmn_info);

	// Put label into the cell.
	TScreenItemsGridCellData cell_data;
	cell_data.Clear();
	cell_data.PlaceCenterBottom(state_scr_item, TRUE, 4);
	cell_data.m_scr_item_shift_x[0] = (short)clmn_info.m_delim_width/2;
	m_rules_grid.GetGrid().SetCellData(irow, iclmn, cell_data);

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::AddMissingSymbolColumns(int &iclmn_start, int seq_len)
{
	// Check columns presence at the beginning of the sequence.
	while (iclmn_start < 0)
	{
		// Grid needs more columns at its left side.
		if (m_rules_grid.CreateRuleSymbolColumn(this, &m_symbols_vert_delim_style, FALSE) == FALSE)
			return(FALSE);

		iclmn_start++;
	}

	// Check columns presence at the end of the sequence.
	while (iclmn_start+seq_len > m_rules_grid.NumSymbolColumns())
	{
		// Grid needs more columns at its right side.
		if (m_rules_grid.CreateRuleSymbolColumn(this, &m_symbols_vert_delim_style, TRUE) == FALSE)
			return(FALSE);
	}

	// Success.
	return(TRUE);
}

void TLangViewRulesGridDialog::AddGrammarSymbolSelection(int irow, int iclmn)
{
	TScreenItemsGridCellData cd;
	m_rules_grid.GetGrid().GetCellData(irow, iclmn, cd);
	TLangViewLexItem *sym_scr_item = (TLangViewLexItem*)cd.m_scr_item[0];
	assert(sym_scr_item != NULL);

	TScreenItem *bkgr_scr_item = sym_scr_item->CreateSelectionScreenItem(list_light_roze);
	if (bkgr_scr_item == NULL)
	{
		MsgBox(L"Failure creating grammar symbol selection.");
		return;
	}

	cd.PlaceAtLayer(1, bkgr_scr_item, TRUE, 0, (bkgr_scr_item->GetItemTypeId() == TRectItem::type_ID) ? 3 : 0);
	m_rules_grid.GetGrid().SetCellData(irow, iclmn, cd);
}

//
//  ===========================  Section 3  ==============================
//

TParsingStateTransitionsViewMode TLangViewRulesGridDialog::GetCurrTrnvMode()
{
	if (m_dialog_mode == grdm_grammar_rules)
	{
		bool symbol_use_report_mode = m_single_obj_index >= 0 && m_single_obj_index < actb_reduce;
		return(m_pstt_trnv_mode[(int)symbol_use_report_mode]);
	}
	else if (m_dialog_mode == grdm_parsing_states)
	{
		return(m_pstt_trnv_mode[2]);
	}

	// Unsupported mode.
	return(pstt_trnv_none);
}

TLangViewBuildStateTransitionsHelper *TLangViewRulesGridDialog::GetPreparedTransitionsHelper(bool want_index)
{
	TLangViewBuildStateTransitionsHelper *phlpr = &m_sttr_helper;
	if (m_grammar->parent_grammar == NULL && m_owner->IsGrammarBrowser() == TRUE)
	{
		// Current grammar is the root grammar and an owning browser is the grammar browser.
		phlpr = ((TLangViewGrammarBrowser*)m_owner)->GetStateTransitionsHelper();
	}

	phlpr->SetGrammar(m_grammar);
	if (want_index == TRUE && GetCurrTrnvMode() != pstt_trnv_none && phlpr->IndexesPresent() == FALSE)
	{
		// The current mode expects a lot of parsing state transion trees.
		m_local_console.SetMinorStatus(L"Preparing indexes ...");

		// Passed progress handler will open the console only if needed. Besides that it will not change any messages there.
		if (phlpr->PrepareIterationIndexes(BuildIndexesProgressCallbackHandler, this) == FALSE)
		{
			MsgBox(L"Error creating RuleStartStatesIter index. Generation of the parsing state transitions might be slow but the picture will be still correct.");
		}
	}

	// The return value is either local or shared state transitions helper.
	return(phlpr);
}

bool TLangViewRulesGridDialog::ShowStateTransitionRows(TLangViewBuildStateTransitionsHelper *sttr_hlpr, int iclmn_rule_start, int irule, int isym, WORD pars_stt_at_isym, WORD action_sym_restr, bool xnd_left, bool xnd_right)
{
	if (GetCurrTrnvMode() == pstt_trnv_none)
	{
		// There is nothing to display.
		return(TRUE);
	}

	// State transitions can be displayed only when the grammar is fine. Check for this.
	if (m_grammar->grm_props.processing_result != grpr_full_success)
	{
		// Make a tricky call that is needed when transition states for empty rules are displayed. Without this call
		// the error message will be mistakenly placed into the trailing column of the grid if the rules grid is empty.
		if (AddMissingSymbolColumns(iclmn_rule_start, 1) == FALSE)
			return(FALSE);

		int status_error_msg_row_inx = CreateStateTransitionsRow(L"StateTransitionsRowErrorMsg", state_transitions_row_height+6, irule);
		if (status_error_msg_row_inx < 0)
			return(FALSE);

		const wchar_t *grammar_status_msg = L"The m_grammar->grm_props.processing_result field is bogus.";
		switch (m_grammar->grm_props.processing_result)
		{
			case grpr_none:				grammar_status_msg = L"State transitions cannot be displayed because the grammar is empty.";
										break;
			case grpr_syntax_errors:		grammar_status_msg = L"State transitions cannot be displayed because grammar contains syntax errors.";
										break;
			case grpr_conversion_errors:	grammar_status_msg = L"State transitions cannot be displayed because grammar has conversion errors.";
										break;
		}

		int msg_clmn_inx = m_rules_grid.RuleSymClmn(iclmn_rule_start);
		if (CreateLeftAlignedTextLabel(L"NoStateTransesBadGrammarState", status_error_msg_row_inx, msg_clmn_inx, 10, &m_row_comment_text_style, grammar_status_msg) == FALSE)
			return(FALSE);
	}
	else
	{
		if (m_grammar->NumParsingStates() == 0)
		{
			// This is special case when building the parsing states was skipped.
			return(TRUE);
		}

		// Call the helper to do the complex analysis work.
		bool collect_act_syms = (GetCurrTrnvMode() == pstt_trnv_show_act_syms);
		if (sttr_hlpr->PrepareStateTransitionRows(this, irule, isym, pars_stt_at_isym, action_sym_restr, xnd_left, xnd_right, collect_act_syms) == FALSE)
		{
			return(FALSE);
		}

		if (sttr_hlpr->NumResultRows() <= 0)
		{
			// State transitions are missing. Create single row for explanation.
			int msg_row_inx = CreateStateTransitionsRow(L"StateTransitionsRowMsg", state_transitions_row_height+6, irule);
			if (msg_row_inx < 0)
				return(FALSE);

			int msg_clmn_inx = m_rules_grid.RuleSymClmn(iclmn_rule_start);
			if (action_sym_restr != 0xFFFF)
			{
				if (CreateLeftAlignedTextLabel(L"NoStateTransesWithRestr", msg_row_inx, msg_clmn_inx, 10, &m_row_comment_text_style, L"State transitions for action symbol %d are empty.", action_sym_restr) == FALSE)
					return(FALSE);
			}
			else
			{
				if (CreateLeftAlignedTextLabel(L"NoStateTransesNoRestr", msg_row_inx, msg_clmn_inx, 10, &m_row_comment_text_style, L"State transitions for this rule are empty.") == FALSE)
					return(FALSE);
			}
		}
		else
		{
			// Create rows in the main grid for all lines of the results set.
			int row_inx_beg = -1;
			for (int irow=0; irow<sttr_hlpr->NumResultRows(); ++irow)
			{
				int row_inx = CreateStateTransitionsRow(L"StateTransitionsRow", state_transitions_row_height, irule);
				if (row_inx < 0)
					return(FALSE);

				if (row_inx_beg == -1)
					row_inx_beg = row_inx;
			}

			// Call the recursive function that will create necessary screen items on these lines.
			assert(sttr_hlpr->NumResultColumns() > 0);
			if (ShowRuleStatesTree(sttr_hlpr, NULL, row_inx_beg, iclmn_rule_start, 0, sttr_hlpr->NumResultColumns()-1, sttr_hlpr->NumResultRows()) == FALSE)
				return(FALSE);
		}
	}

	// All necessary allocations succeeded.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowRuleStatesTree(TLangViewBuildStateTransitionsHelper *sttr_hlpr, TGrammarViewLabelItem *owner, int main_grid_irow, int iclmn_start, int result_set_irow, int result_set_iclmn, int num_rows)
{
	assert(result_set_irow >= 0 && num_rows >= 1);
	assert(result_set_irow+num_rows <= sttr_hlpr->NumResultRows());
	assert(result_set_iclmn >= 0 && result_set_iclmn < sttr_hlpr->NumResultColumns());

	int range_beg_irow = result_set_irow;
	int range_beg_grid_irow = main_grid_irow;
	while (range_beg_irow < result_set_irow+num_rows)
	{
		TRuleStateTransitionNode &node = sttr_hlpr->ResultRow(range_beg_irow).m_transitions[result_set_iclmn];
		int len_range = node.same_state_rows;
		assert(len_range >= 1);

		// Create node for the final state in the subtree.
		TGrammarViewLabelItem *state_item = CreateSpecLabelItem(L"ParsStt", TGrammarViewLabelItem::gvlt_parsing_state, node.istate);
		if (state_item == NULL)
			return(FALSE);

		if (owner != NULL)
			state_item->SetupLink(owner, &m_pars_stt_link_style);

		int state_item_row = range_beg_grid_irow+(len_range-1)/2;
		int state_item_clmn = m_rules_grid.ParsSttClmn(iclmn_start+result_set_iclmn);

		// Put item into the cell.
		TScreenItemsGridColumnInfo clmn_info;
		m_rules_grid.GetGrid().GetColumnInfo(state_item_clmn, clmn_info);
		m_rules_grid.SetupCenteredCell(this, L"ParsStt", state_item_row, state_item_clmn, state_item, 0, (short)clmn_info.m_delim_width/2);

		if ((len_range & 1) == 0)
		{
			// The number of rows in the range is even. Shift the item down for the half of the row height.
			m_rules_grid.ShiftMainLayerItem(state_item_row, state_item_clmn, 0, (short)m_rules_grid.GetGrid().RowHeight(state_item_row)/2);
		}

		int exra_row_pairs = 0;
		if (owner == NULL)
		{
			// This is the top level node. Check if the action symbols subtree should be crerated.
			TSymbolsArray *act_syms = sttr_hlpr->ResultRow(range_beg_irow).m_action_symbols;
			if (act_syms != NULL)
			{
				exra_row_pairs = ShowRuleActionSymbols(range_beg_grid_irow, state_item_row, state_item_clmn, len_range, act_syms);
				if (exra_row_pairs < 0)
					return(FALSE);
			}
			else
			{
				// Create small one pixel vertical separators to set the state transition trees apart.
				exra_row_pairs = 1;
				if (CreateExtraRowsPair(range_beg_grid_irow, len_range, 1) == FALSE)
					return(FALSE);
			}

			range_beg_grid_irow += exra_row_pairs;
		}

		// Note that when the rule is empty, this method will be called with result_set_iclmn equal to zero and none
		// of the statements below will be executed. This is by design because one state is already created above.
		if (result_set_iclmn > 1)
		{
			// Create the lower level subtree.
			if (ShowRuleStatesTree(sttr_hlpr, state_item, range_beg_grid_irow, iclmn_start, range_beg_irow, result_set_iclmn-1, len_range) == FALSE)
				return(FALSE);
		}
		else if (result_set_iclmn == 1)
		{
			// Create the matrix of initial states.
			if (ShowRuleInitialStates(sttr_hlpr, state_item, range_beg_grid_irow, iclmn_start, range_beg_irow, len_range) == FALSE)
				return(FALSE);
		}

		// Shift to the next range.
		range_beg_irow += len_range;
		range_beg_grid_irow += (len_range+exra_row_pairs);
	}

	// All necessary allocations succeeded.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowRuleInitialStates(TLangViewBuildStateTransitionsHelper *sttr_hlpr, TGrammarViewLabelItem *owner, int main_grid_irow, int iclmn_start, int result_set_irow, int num_rows)
{
	assert(owner != NULL);

	for (int irow=0; irow<num_rows; ++irow)
	{
		// Pick up the initial states that should be placed on the current row.
		const TSymbolsArray &initial_states = sttr_hlpr->ResultRow(result_set_irow+irow).m_initial_states;
		assert(initial_states.NumItems() > 0);

		// Fix the number of subcolumns if needed.
		while (initial_states.NumItems() > m_rules_grid.NumParsSttClmns(iclmn_start))
		{
			// Number of subcolumns is not enough.
			if (m_rules_grid.CreateParsSttColumnsPair(this, iclmn_start) == FALSE)
				return(FALSE);
		}

		int iclmn_beg = m_rules_grid.ParsSttClmn(iclmn_start, -(initial_states.NumItems()-1)/2);
		int iclmn_center = m_rules_grid.ParsSttClmn(iclmn_start);
		for (int istt=0; istt<initial_states.NumItems() && initial_states[istt] != 0xFFFF; ++istt)
		{
			TGrammarViewLabelItem *state_item = CreateSpecLabelItem(L"InitialParsStt", TGrammarViewLabelItem::gvlt_parsing_state, initial_states[istt]);
			if (state_item == NULL)
				return(FALSE);

			// Figure out the required horz offset.
			short horz_offs = 0;
			if (iclmn_beg+istt < iclmn_center)
				horz_offs = -m_rules_grid.ExtraParsSttClmnWidth()/2;
			else if (iclmn_beg+istt > iclmn_center)
				horz_offs = m_rules_grid.ExtraParsSttClmnWidth()/2-1;

			// Put item into the cell.
			state_item->SetupLink(owner, &m_pars_stt_link_style);
			m_rules_grid.SetupCenteredCell(this, L"ParsStt", main_grid_irow+irow, iclmn_beg+istt, state_item, 0, horz_offs);
		}
	}

	// Success.
	return(TRUE);
}

int TLangViewRulesGridDialog::ShowRuleActionSymbols(int main_grid_irow, int final_stt_irow, int final_stt_sym_clmn, int num_rows, TSymbolsArray *action_symbols_buff)
{
	TGrammarViewLabelItem *final_stt_object = (TGrammarViewLabelItem*)m_rules_grid.GetGrid().GetMainLayerScreenItem(final_stt_irow, final_stt_sym_clmn);
	assert(final_stt_object != NULL);

	int num_added_row_pairs = 0;
	int num_symbols = action_symbols_buff->NumItems();
	long rows_range_height = num_rows*m_rules_grid.GetGrid().RowHeight(main_grid_irow);

	// Figure out the reasonable dimensions for the grid.
	int inner_grid_rows = ((3*rows_range_height)/4)/action_symbols_grid_cell_size_y;
	if (inner_grid_rows == 0)
		inner_grid_rows = 1;

	int inner_grid_columns = (num_symbols+inner_grid_rows-1)/inner_grid_rows;
	if (inner_grid_columns == 1 && inner_grid_rows > num_symbols)
		inner_grid_rows = num_symbols;

	// Check if the inner grid is too high or too wide.
	if (num_symbols > 3)
	{
		while (inner_grid_columns > 7*inner_grid_rows)
		{
			inner_grid_rows++;
			inner_grid_columns = (num_symbols+inner_grid_rows-1)/inner_grid_rows;
		}

		while (inner_grid_rows >= inner_grid_columns)
		{
			if (inner_grid_rows == inner_grid_columns && inner_grid_rows*inner_grid_columns == num_symbols)
				break;

			inner_grid_columns++;
			inner_grid_rows = (num_symbols+inner_grid_columns-1)/inner_grid_columns;
		}
	}

	long inner_grid_height = action_symbols_grid_cell_size_y*inner_grid_rows;
	long min_stripe_height = (num_rows <= 1) ? inner_grid_height : inner_grid_height+12;

	// Check if existing rows range is high enough for the inner grid or not.
	if (min_stripe_height > rows_range_height)
	{
		// Create 2 additional rows above and below the passed range.
		long extra_row_height = (min_stripe_height-rows_range_height+1)/2;
		if (CreateExtraRowsPair(main_grid_irow, num_rows, extra_row_height) == FALSE)
			return(-1);

		num_added_row_pairs++;
		main_grid_irow++;
	}

	// Allocate grid object for displaying the rule action symbols.
	TScreenItemsGridItem *inner_grid = CreateInnerGrid(inner_grid_rows, inner_grid_columns, action_symbols_grid_cell_size_x, action_symbols_grid_cell_size_y);
	if (inner_grid == NULL)
		return(-1);

	// Fill in the cells of the grid.
	int buff_inx = 0;
	for (int ir=0; ir<inner_grid_rows; ++ir)
	{
		for (int ic=0; ic<inner_grid_columns; ++ic)
		{
			if (buff_inx < action_symbols_buff->NumItems())
			{
				// Pick up the next symbol and place it into the cell.
				WORD sym = action_symbols_buff->ItemRef(buff_inx++);
				assert(sym < m_grammar->NumTerminals());

				TGrammarViewLabelItem *screen_item = CreateSpecLabelItem(L"ActionSym", TGrammarViewLabelItem::gvlt_grammar_symbol, sym);
				if (screen_item == NULL)
				{
					// Error is already displayed.
					delete inner_grid;
					return(-1);
				}

				screen_item->SetupLink(final_stt_object, &m_action_syms_link_style);

				TScreenItemsGridCellData inner_cell_data;
				inner_cell_data.Clear();
				inner_cell_data.PlaceInTheCenter(screen_item, TRUE);
				inner_grid->SetCellData(ir, ic, inner_cell_data);
			}
		}
	}

	// Finalize the inner grid and attach it to the owner object.
	inner_grid->UpdateGrid();
	final_stt_object->m_action_symbols_grid = inner_grid;

	// Success.
	return(num_added_row_pairs);
}

int TLangViewRulesGridDialog::CreateStateTransitionsRow(const wchar_t *descr, int row_height, int irule)
{
	return(m_rules_grid.CreateRow(this, descr, ROW_grammar_rule_body, row_height, NULL, NULL, irule, 0, 0, SCRITEMS_GRID_APPEND, ROW_EX_state_transitions));
}

bool TLangViewRulesGridDialog::CreateExtraRowsPair(int main_grid_irow, int num_rows, long extra_rows_height)
{
	TBasicStyle *extra_row_bkgr = NULL;
	WORD irule = m_rules_grid.GetIndex2(main_grid_irow);

	int lower_row_inx = m_rules_grid.CreateRow(this, L"LowerParsSttsExtraRow", ROW_grammar_rule_body, extra_rows_height, extra_row_bkgr, NULL, irule, 0, 0, main_grid_irow+num_rows, ROW_EX_state_transitions);
	if (lower_row_inx < 0)
		return(FALSE);

	int upper_row_inx = m_rules_grid.CreateRow(this, L"UpperParsSttsExtraRow", ROW_grammar_rule_body, extra_rows_height, extra_row_bkgr, NULL, irule, 0, 0, main_grid_irow, ROW_EX_state_transitions);
	if (upper_row_inx < 0)
	{
		m_rules_grid.GetGrid().DeleteRow(lower_row_inx);
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

//
//  ===========================  Section 4  ==============================
//

bool TLangViewRulesGridDialog::ShowXpctConflictOverview(int ixpct, bool &conflicts_avail)
{
	conflicts_avail = FALSE;
	if (ShowXpctConflictHeader(ixpct) == FALSE)
		return(FALSE);

	int iconflict = m_grammar->GetFirstGrammarConflictInx(ixpct);
	if (iconflict < 0)
	{
		// There are no participating grammar conflicts. Add an explanation message.
		if (CreateMessageRow(L"NoGrammarConflicts", ROW_none, m_rules_grid.RuleSymClmn(0), 6, L"Current expected conflict does not own grammar conficts.") == FALSE)
			return(FALSE);
	}
	else
	{
		conflicts_avail = TRUE;

		// Irerate available grammar conflicts.
		TBasicStyle *delim_style = NULL;
		while (iconflict >= 0)
		{
			if (ShowGrammarConflictOverview(iconflict, delim_style, FALSE) == FALSE)
				return(FALSE);

			delim_style = &m_light_delim_style;
			iconflict = m_grammar->GetNextGrammarConflictInx(ixpct, iconflict);
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowGrammarConflictOverview(int iconflict, TBasicStyle *upper_delim_style, bool allow_showing_xpct_name)
{
	// Pick up the conflict.
	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
	assert(cfct.NumActions() > 0);

	// Create rows for the conflict header.
	bool allows_obsc_footer = m_show_conflicting_action_hdrs == TRUE || m_show_directly_conflicting_rules == TRUE;
	if (ShowGrammarConflictHeader(iconflict, upper_delim_style, allow_showing_xpct_name, allows_obsc_footer) == FALSE)
		return(FALSE);

	// Iterate conflicting actions.
	TBasicStyle *action_delim_style = NULL;
	for (int iact=0; iact<cfct.NumActions(); ++iact)
	{
		// Pick up the action.
		WORD action = cfct.conflicting_actions[iact];

		if (m_show_conflicting_action_hdrs == TRUE)
		{
			// Create conflicting action header.
			if (ShowConflicingActionHeader(iconflict, iact, action_delim_style) == FALSE)
				return(FALSE);

			action_delim_style = &m_light_delim_style;
		}

		if (m_show_directly_conflicting_rules == TRUE)
		{
			if (action < actb_reduce)
			{
				// Current conflicting action is "shift". Look for rules that are using this shift.
				int positions_found = 0;
				TParsingStatePositionsIterator iter;
				WORD prev_irule = 0xFFFF, prev_isym = 0xFFFF;
				for (iter.Setup(m_grammar->parsing_states[cfct.parser_state]); iter; ++iter)
				{
					TRulePosition &pos = iter.CurrPos();
					if (pos.irule == prev_irule && pos.isym == prev_isym)
						continue;

					prev_irule = pos.irule;
					prev_isym = pos.isym;

					TGrammarRule &rule = m_grammar->rules[pos.irule];
					if (pos.isym >= rule.Length() || rule.symbols[pos.isym] != cfct.input_symbol)
						continue;

					positions_found++;

					// Required rule is available. Show its body.
					int rule_row = ShowRuleBody(SCRITEMS_GRID_APPEND, pos.irule, m_rules_grid.GetAlignmentSymIndex()-pos.isym);
					if (rule_row < 0)
						return(FALSE);

					// Show current state as a small label.
					if (ShowParsingStateOnClmnRef(SCRITEMS_GRID_APPEND, cfct.parser_state, ROW_grammar_rule_body, pos.irule) == FALSE)
						return(FALSE);

					// Check if analysis result details should be displayed or not.
					if (positions_found == 1)
						ShowConflictAnalysisDetails(rule_row, iconflict, iact);
				}

				// At least one shift position should be present.
				assert(positions_found > 0);
			}
			else
			{
				// Action is "apply rule".
				assert(action < actb_conflict);
				WORD irule = action-actb_reduce;
				int rule_len = m_grammar->rules[irule].Length();

				// Show body of the conflicting rule.
				int rule_row = ShowRuleBody(SCRITEMS_GRID_APPEND, irule, m_rules_grid.GetAlignmentSymIndex()-rule_len);
				if (rule_row < 0)
					return(FALSE);

				// Create colunm for the action symbol if it is not available.
				int alignment_clmn_index = m_rules_grid.GetAlignmentSymIndex();
				if (AddMissingSymbolColumns(alignment_clmn_index, 1) == FALSE)
					return(FALSE);

				TGrammarViewLabelItem *inp_sym_label = CreateSpecLabelItem(L"InpSymLabel", TGrammarViewLabelItem::gvlt_cfct_inp_symbol, cfct.input_symbol);
				if (inp_sym_label == NULL)
					return(FALSE);

				m_rules_grid.SetupCenteredCell(this, L"InpSymLabel", rule_row, m_rules_grid.RuleSymClmn(alignment_clmn_index), inp_sym_label);

				// Show current state as a small label.
				if (ShowParsingStateOnClmnRef(SCRITEMS_GRID_APPEND, cfct.parser_state, ROW_grammar_rule_body, irule) == FALSE)
					return(FALSE);

				// Show analysis result details if any.
				ShowConflictAnalysisDetails(rule_row, iconflict, iact);
			}
		}

		if (m_show_conflicting_action_hdrs == TRUE)
			m_rules_grid.RemoveLastRowRowDelimiter();
	}

	// Success.
	m_rules_grid.RemoveLastRowRowDelimiter();
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowXpctConflictHeader(int ixpct)
{
	// Create rows for the expected conflict header.
	int hdr_row_inx = CreateHeaderRows(L"XpctConflictHeader", ROW_xpct_conflict_header, &m_dark_delim_style, 25, &m_xpct_conflict_hdr_style, &m_semi_light_delim_style, ixpct);
	if (hdr_row_inx < 0)
	{
		return(FALSE);
	}

	// Show the xpct conflict index.
	if (CreateCenteredTextLabel(L"XpctConflictIndex", hdr_row_inx, m_rules_grid.RowDescrClmn(), 0, &m_sect_title_text_style, L"Expected conflict: X%d", ixpct) == FALSE)
	{
		return(FALSE);
	}

	if (m_rules_grid.IsNonTermClmnPresent() == TRUE)
	{
		// Show the number of low level conflicts.
		int num_conflicts = m_grammar->GetNumGrammarConflicts(ixpct);
		if (CreateCenteredTextLabel(L"XpctConflictNumConflicts", hdr_row_inx, m_rules_grid.RuleNonTermClmn(), 0, &m_small_comment_text_style, L"%d conflict%s", num_conflicts, PluralSuffix(num_conflicts)) == FALSE)
		{
			return(FALSE);
		}
	}

	// Show the name of the expected conflict.
	if (CreateLeftAlignedTextLabel(L"XpctConflictName", hdr_row_inx, m_rules_grid.RuleFrontSpaceClmn(), xpct_name_left_shift, &m_sect_title_text_style, m_grammar->xpct_conflicts[ixpct].xpct_conflict_name) == FALSE)
	{
		return(FALSE);
	}

	// Show obscure small pieces of vertical delims that are visible on the borders of the header.
	CreateFancyObscuringLine(L"XpctUpperObscuringItem", hdr_row_inx-1, &m_dark_delim_style);
	CreateFancyObscuringLine(L"XpctLowerObscuringItem", hdr_row_inx, &m_semi_light_delim_style);

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowGrammarConflictHeader(int iconflict, TBasicStyle *upper_delim_style, bool allow_showing_xpct_name, bool allow_lower_obscuring_line)
{
	// Create rows for the conflict header.
	int hdr_row_inx = CreateHeaderRows(L"GrammarConflictHeader", ROW_grammar_conflict_header, upper_delim_style, 21, &m_grammar_cfct_hdr_style, &m_light_delim_style, iconflict);
	if (hdr_row_inx < 0)
	{
		return(FALSE);
	}

	// Show an index of the conflict.
	bool full_cfct_name = m_dialog_mode != grdm_xpct_conflicts && m_dialog_mode != grdm_dplist_symbols_xpct;
	if (CreateCenteredTextLabel(L"ConflictIndex", hdr_row_inx, m_rules_grid.RowDescrClmn(), 0, &m_sect_title_text_style, (full_cfct_name == TRUE) ? L"Conflict: %d" : L"C%d       ", iconflict) == FALSE)
	{
		return(FALSE);
	}

	if (m_rules_grid.IsNonTermClmnPresent() == TRUE)
	{
		// Show the conflict details: the parsing state and the symbol.
		TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
		if (CreateCenteredTextLabel(L"ConflictDetails", hdr_row_inx, m_rules_grid.RuleNonTermClmn(), 0, &m_sect_title_text_style, L"State: %hu, Symbol: %hu.", cfct.parser_state, cfct.input_symbol) == FALSE)
		{
			return(FALSE);
		}
	}

	// Show the name of the expected conflict or the name of the category that the current conflict belongs to.
	if (allow_showing_xpct_name == TRUE && m_show_xpct_conflict_name == TRUE)
	{
		int xpct_conflict_inx = m_grammar->conflicts[iconflict].expected_conflict_inx;
		if (xpct_conflict_inx >= 0)
		{
			// Current conflict belongs to one expected conflict.
			TGrammarViewLabelItem *xpct_conflict_label = CreateSpecLabelItem(L"ConflictOwner1", TGrammarViewLabelItem::gvlt_xpct_conflict, xpct_conflict_inx);
			if (xpct_conflict_label == NULL)
				return(FALSE);

			// Place label into the grid.
			m_rules_grid.SetupLeftAlignedCellWithRowBkgr(this, L"ConflictOwner1", hdr_row_inx, m_rules_grid.RuleFrontSpaceClmn(), xpct_conflict_label, 0, xpct_name_left_shift);
		}
		else if (xpct_conflict_inx == -1)
		{
			// Current conflict does not belong to any expected conflict.
			if (CreateLeftAlignedTextLabel(L"ConflictOwner2", hdr_row_inx, m_rules_grid.RuleFrontSpaceClmn(), xpct_name_left_shift, &m_sect_title_text_style, L"Unexpected") == FALSE)
				return(FALSE);
		}
		else
		{
			// Current conflict belongs to more than one expected conflict.
			if (CreateLeftAlignedTextLabel(L"ConflictOwner3", hdr_row_inx, m_rules_grid.RuleFrontSpaceClmn(), xpct_name_left_shift, &m_sect_title_text_style, L"Ambiguous (%d)", -xpct_conflict_inx) == FALSE)
				return(FALSE);
		}

		// Creation of the label succeeded. Create fancy obscuring lines that should obscure small pieces of the vertical column delims.
		if (upper_delim_style != NULL)
			CreateFancyObscuringLine(L"GrmCfctUpperObscItem", hdr_row_inx-1, upper_delim_style);
		if (allow_lower_obscuring_line == TRUE)
			CreateFancyObscuringLine(L"GrmCfctLowerObscItem", hdr_row_inx, &m_light_delim_style);
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowConflicingActionHeader(int iconflict, int iaction, TBasicStyle *upper_delim_style)
{
	// Create the conflicting action header itself.
	int hdr_row_inx = CreateHeaderRows(L"ConflictingActionHeader", ROW_conflicting_action_header, upper_delim_style, 17, &m_cfcting_action_hdr_style, &m_light_delim_style, iconflict, iaction);
	if (hdr_row_inx < 0)
	{
		return(FALSE);
	}

	// Show conflicing action details.
	WORD action = m_grammar->conflicts[iconflict].conflicting_actions[iaction];
	wchar_t details_buffer[80];
	swprintf(details_buffer, 80, L"Action %d: %s %hu.", iaction, (action < actb_reduce) ? L"Shift to" : L"Apply rule", (action < actb_reduce) ? action : (action-actb_reduce));
	if (CreateLeftAlignedTextLabel(L"ConflictingActionDetails", hdr_row_inx, m_rules_grid.RowDescrClmn(), 6, &m_row_descr_text_style, details_buffer) == FALSE)
	{
		return(FALSE);
	}

	// Remove the action label backgound screen item.
	TScreenItemsGridCellData cell_data;
	m_rules_grid.GetGrid().GetCellData(hdr_row_inx, m_rules_grid.RowDescrClmn(), cell_data);
	if (cell_data.m_scr_item[1] != NULL)
	{
		delete cell_data.m_scr_item[1];
		cell_data.m_scr_item[1] = NULL;
	}

	// Set this change into the grid.
	m_rules_grid.GetGrid().SetCellData(hdr_row_inx, m_rules_grid.RowDescrClmn(), cell_data);

	if (m_rules_grid.IsNonTermClmnPresent() == TRUE)
	{
		// Check for analysis results presence.
		TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
		if (cfct.analysis_results.NumItems() > 0 && cfct.analysis_results[iaction].m_generation_result != TConflictingActionAnalysisResults::gres_not_started)
		{
			// Processing for the current action is either complete or it is still running.
			int num_paths = cfct.analysis_results[iaction].m_derivation_paths.NumItems();
			if (num_paths > 0)
			{
				if (CreateCenteredTextLabel(L"NumPaths1", hdr_row_inx, m_rules_grid.RuleNonTermClmn(), 0, &m_small_comment_text_style, L"%d path%s", num_paths, PluralSuffix(num_paths)) == FALSE)
					return(FALSE);
			}
			else
			{
				if (CreateCenteredTextLabel(L"NumPaths2", hdr_row_inx, m_rules_grid.RuleNonTermClmn(), 0, &m_analysis_error_text_style, L"No Paths") == FALSE)
					return(FALSE);
			}
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::CreateFancyObscuringLine(const wchar_t *descr, int irow, TBasicStyle *bkgr_style)
{
	// Allocate object with the right style.
	TRectItem *fancy_line = new TRectItem(bkgr_style);
	if (fancy_line == NULL)
	{
		MsgBoxFmt(MB_OK, L"Unablde to create fancy obscure screen item (%s).", descr);
		return(FALSE);
	}

	// The object will be positioned later. For now just link it into the given row and rule front space column of the grid.
	TScreenItemsGridCellData cd;
	m_rules_grid.GetGrid().GetCellData(irow, m_rules_grid.RuleFrontSpaceClmn(), cd);
	cd.PlaceAtLayer(2, fancy_line, TRUE);
	m_rules_grid.GetGrid().SetCellData(irow, m_rules_grid.RuleFrontSpaceClmn(), cd);
	return(TRUE);
}

void TLangViewRulesGridDialog::ShowConflictAnalysisDetails(int irow, int iconflict, int iaction)
{
	TTextItem80 *msg1 = NULL, *msg2 = NULL;

	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
	assert(cfct.NumActions() > 0);

	// Check situation on the conflict layer first.
	if (cfct.analysis_results.NumItems() <= 0)
	{
		// Array of analysis results is not allocated.
		msg1 = CreateTextScreenItem(L"CfctAnalysisDtls1", &m_analysis_warning_text_style);
		if (msg1 != NULL)
			msg1->SetText(L"No data");
	}
	else if (cfct.analysis_results.NumItems() != cfct.NumActions())
	{
		// Array of analysis results has bogus length.
		msg1 = CreateTextScreenItem(L"CfctAnalysisDtls2", &m_analysis_error_text_style);
		if (msg1 != NULL)
			msg1->SetText(L"Bad data length");
	}
	else
	{
		// Check specific situation for the particular conflicting action.
		assert(iaction >= 0 && iaction < cfct.NumActions());

		TConflictingActionAnalysisResults &anres = cfct.analysis_results[iaction];
		TConflictingActionAnalysisResults::TGenResult gres = anres.m_generation_result;
		if (gres == TConflictingActionAnalysisResults::gres_ok && anres.m_derivation_paths.NumItems() == 0)
			gres = TConflictingActionAnalysisResults::gres_no_paths_found;

		if (gres < TConflictingActionAnalysisResults::gres_ok)
		{
			// The work was not started or it is in some intermediate state.
			msg1 = CreateTextScreenItem(L"CfctAnalysisDtls3", &m_analysis_warning_text_style);
			if (msg1 != NULL)
				msg1->SetText(TConflictingActionAnalysisResults::GetGenResultDescription(gres, TRUE));
		}
		else if (gres > TConflictingActionAnalysisResults::gres_ok)
		{
			// The work has fished with error.
			msg1 = CreateTextScreenItem(L"CfctAnalysisDtls4", &m_analysis_error_text_style);
			if (msg1 != NULL)
				msg1->SetText(TConflictingActionAnalysisResults::GetGenResultDescription(gres, TRUE));
		}

		if (anres.m_too_deep_truncations > 0)
		{
			// Display the number of truncations.
			msg2 = CreateTextScreenItem(L"CfctAnalysisDtls5", &m_small_comment_text_style);
			if (msg2 != NULL)
			{
				wchar_t buffer[80];
				__int64 value = anres.m_too_deep_truncations;
				msg2->SetTextFmt(L"%s  path truncation%s.", FormatInt64(value, buffer, 80, fnms_dec_signed, L' '), PluralSuffix(value));

				if (msg1 == NULL)
				{
					msg1 = msg2;
					msg2 = NULL;
				}
			}
		}

		if (anres.m_partial_iteration_aborts > 0 && msg2 == NULL)
		{
			// Display the number of iteration setup aborts.
			msg2 = CreateTextScreenItem(L"CfctAnalysisDtls6", &m_small_comment_text_style);
			if (msg2 != NULL)
			{
				wchar_t buffer[80];
				__int64 value = anres.m_partial_iteration_aborts;
				msg2->SetTextFmt(L"%s  iter setup abort%s.", FormatInt64(value, buffer, 80, fnms_dec_signed, L' '), PluralSuffix(value));

				if (msg1 == NULL)
				{
					msg1 = msg2;
					msg2 = NULL;
				}
			}
		}
	}

	if (msg1 != NULL)
	{
		// Something was generated.
		if (msg2 == NULL)
		{
			// This is one message layout.
			msg1->UpdateSize(m_hDlg);
			m_rules_grid.SetupCenteredCell(this, L"CfctAnalysisDtls", irow, m_rules_grid.RowDescrClmn(), msg1);
		}
		else
		{
			// Show both messages.
			msg1->UpdateSize(m_hDlg);
			msg2->UpdateSize(m_hDlg);

			// Setup the cell using special offsets.
			TScreenItemsGridCellData cell_data;
			cell_data.Clear();
			cell_data.PlaceLeftCenter(NULL, FALSE);
			cell_data.PlaceAtLayer(1, msg1, TRUE, 16, -7);
			cell_data.PlaceAtLayer(0, msg2, TRUE, 16,  9);
			m_rules_grid.GetGrid().SetCellData(irow, m_rules_grid.RowDescrClmn(), cell_data);
		}
	}
}

bool TLangViewRulesGridDialog::CheckConflictAnalysisResultsPresence(int iconflict)
{
	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
	if (cfct.analysis_results.NumItems() <= 0)
		return(FALSE);
	else if (cfct.analysis_results.NumItems() != cfct.NumActions())
		return(TRUE);

	// Analysis results records are available for all actions.
	for (int iact=0; iact<cfct.NumActions(); ++iact)
	{
		if (cfct.analysis_results[iact].m_generation_result != TConflictingActionAnalysisResults::gres_not_started)
			return(TRUE);
	}

	// Processing is not started for any action.
	return(FALSE);
}

bool TLangViewRulesGridDialog::CheckConflictsAnalysisForConflict(int iconflict, bool &processing_started, bool &processing_finished)
{
	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];

	processing_started = processing_finished = FALSE;
	if (cfct.analysis_results.NumItems() <= 0)
	{
		// Array of analysis results is not allocated. Do not show this conflict.
		return(FALSE);
	}
	else if (cfct.analysis_results.NumItems() != cfct.NumActions())
	{
		// Array of analysis results has bogus length. This should be displayed.
		return(TRUE);
	}

	// Check individual actions.
	bool proc_problems = FALSE;
	for (int iact=0; iact<cfct.NumActions(); ++iact)
	{
		bool proc_started, proc_finished;
		proc_problems |= CheckConflictsAnalysisForAction(iconflict, iact, proc_started, proc_finished);
		processing_started |= proc_started;
		processing_finished |= proc_finished;
	}

	// The summary is collected.
	return(proc_problems);
}

bool TLangViewRulesGridDialog::CheckConflictsAnalysisForAction(int iconflict, int iaction, bool &processing_started, bool &processing_finished)
{
	TConflictingActionAnalysisResults &anres = m_grammar->conflicts[iconflict].analysis_results[iaction];

	processing_started = processing_finished = FALSE;
	if (anres.m_generation_result == TConflictingActionAnalysisResults::gres_not_started)
	{
		// Processing is not started. This also means that there are no problems.
		return(FALSE);
	}

	processing_started = TRUE;
	if (anres.m_generation_result <= TConflictingActionAnalysisResults::gres_timeout)
	{
		// Processing is in some intermediate state. If derivation paths are not discovered yet this means
		// that processing is currently in problem.
		return(anres.m_derivation_paths.NumItems() == 0);
	}

	// Return combination of status and presence of derivation paths.
	processing_finished = TRUE;
	return(anres.m_generation_result != TConflictingActionAnalysisResults::gres_ok || anres.m_derivation_paths.NumItems() == 0);
}

//
//  ===========================  Section 5  ==============================
//

bool TLangViewRulesGridDialog::ShowXpctConflictKeyRules(int ixpct, bool &cfcts_avail)
{
	assert(ixpct >= 0 && ixpct < m_grammar->NumXpctConflicts());
	cfcts_avail = FALSE;

	// Display the conflict header row.
	if (ShowXpctConflictHeader(ixpct) == FALSE)
		return(FALSE);

	// Error messages should be displayed in the column of the first rule symbol if is its visible.
	int iclnm_message = m_rules_grid.RuleSymClmn(0);
	if (m_show_key_conflicting_rules == FALSE && m_show_predecessor_seqs == FALSE)
		iclnm_message = m_rules_grid.TrailingClmn();

	// Check the passed expected conflict.
	int igrc1 = m_grammar->GetFirstGrammarConflictInx(ixpct);
	if (igrc1 < 0)
	{
		wchar_t buffer[120];
		swprintf(buffer, 120, L"Expected conflict X%d does not own grammar conflicts.", ixpct);
		if (CreateMessageRow(L"NoOwnedConflicts", ROW_none, iclnm_message, 6, buffer) == FALSE)
			return(FALSE);

		// It is not possible to process this conflict.
		return(TRUE);
	}
	else
	{
		// Participating grammar conflicts are present. Check them.
		bool results_pesent = FALSE;
		while (igrc1 >= 0)
		{
			TGrammarConflict &cfct = m_grammar->conflicts[igrc1];
			assert(cfct.NumActions() > 0);

			if (cfct.analysis_results.NumItems() != cfct.NumActions())
			{
				// Analysis records are not allocated or the number of these records in the conflict is bogus.
				wchar_t buffer[120];
				if (cfct.analysis_results.NumItems() == 0)
					swprintf(buffer, 120, L"Analysis results are missing for iconflict=%d.", igrc1);
				else swprintf(buffer, 120, L"Analysis results are bogus for iconflict=%d. NumActions=%d, NumAnalysisResultRecords=%d.", igrc1, cfct.NumActions(), cfct.analysis_results.NumItems());

				if (CreateMessageRow(L"NoXpctAnRes", ROW_none, iclnm_message, 6, buffer) == FALSE)
					return(FALSE);
			}
			else
			{
				// Ensure that analysis results for all conflicting actions contain derivation paths.
				for (int iact=0; iact<cfct.analysis_results.NumItems(); ++iact)
				{
					if (cfct.analysis_results[iact].m_derivation_paths.NumItems() == 0)
					{
						// Paths are missing.
						wchar_t buffer[120];
						swprintf(buffer, 120, L"Analysis for iconflict=%d, iaction=%d is present but it does not contain derivation paths.", igrc1, iact);

						if (CreateMessageRow(L"NoKeyRulesDervPaths", ROW_none, iclnm_message, 6, buffer) == FALSE)
							return(FALSE);
					}
					else
					{
						// Current action has derivation paths.
						results_pesent = TRUE;
					}
				}
			}

			igrc1 = m_grammar->GetNextGrammarConflictInx(ixpct, igrc1);
		}

		if (results_pesent == FALSE)
		{
			// It is not possible to process this conflict.
			return(TRUE);
		}
	}

	//
	// At this point it is clear that conflicts are available and that all these conflicts contain analysis results.
	//
	cfcts_avail = TRUE;

	// Pick up the data about possible predecessors and followers.
	m_local_console.SetMajorStatus(L"Generating conflict context ...");
	TDerivationPathAnalysisHelper helper(m_local_console, -1, m_grammar);

	TDerivationPathAnalysisHelper::TConflictContextRecordsArray context;
	if (helper.GenerateExpectedConflictContext(context, ixpct, (m_show_key_conflicting_rules == FALSE) || m_merge_diff_key_rule_seqs == TRUE) == FALSE)
	{
		// This is some low level error that has nothing to do with the grammar itself.
		MsgBoxFmt(MB_OK, L"Error getting context for the expected conflict X%d.", ixpct);
		return(FALSE);
	}
	else if (context.NumItems() == 0)
	{
		// Add an explanation message.
		if (CreateMessageRow(L"NoKeyPositionsNote", ROW_none, iclnm_message, 6, L"Retrieved context for the expected conflict is empty.") == FALSE)
			return(FALSE);
	}
	else
	{
		// Calculate the number of rows and columns in the embedded matrixes. Embedded matrixes for all key rules
		// have the same size. Number of rows is equal to the bigest number of conflicting actions across all owned
		// conflicts plus 2 rows for headers. The number of columns is equal the number of owned grammar conflicts.
		int num_emb_rows = 0;
		int igrc2 = m_grammar->GetFirstGrammarConflictInx(ixpct);
		while (igrc2 >= 0)
		{
			int num_actions = m_grammar->conflicts[igrc2].NumActions();
			if (num_emb_rows < num_actions)
				num_emb_rows = num_actions;

			igrc2 = m_grammar->GetNextGrammarConflictInx(ixpct, igrc2);
		}

		num_emb_rows += 2;
		int num_emb_columns = m_grammar->GetNumGrammarConflicts(ixpct);

		//
		// Iterate the retrieved context. Records of the context have the following properties:
		//
		//		do_not_merge_mode:		--	All records have only one key conflicting rule.
		//								--	Different records can have identical origins sets. Nevertheless identical origins sets are
		//									never displayed multiple times because loop on the records is checking the new_origins
		//									flag that is kindly set by the analysis helper when the origins set changes.
		//
		//		merge_diff_key_rule_seqs:	--	All records have different origin sets.
		//								--	Each record may have several key rules.
		//
		//  All action maps are different reagrdless of the selected "merge" mode.
		//
		//  Note that when uncomplete results are viewed there can be more action maps than it might be when
		//  calculation is finished (and not aborted) for all actions.
		//
		TBasicStyle *upper_delim_style = NULL;
		bool predecessors_followers_present = FALSE;
		for (int irecord=0; irecord<context.NumItems(); ++irecord)
		{
			if (context[irecord].new_origins == TRUE)
			{
				// Create the header row for displaying the actions map.
				int hdr_row_height = 12*num_emb_rows+4;
				int hdr_row_inx = CreateHeaderRows(L"ParticipatingActionsMapHeader", ROW_none, upper_delim_style, hdr_row_height, &m_xpct_actions_map_hdr_style, &m_light_delim_style);
				if (hdr_row_inx < 0)
				{
					// Erorr is already reported.
					return(FALSE);
				}

				// Create grid for displaying the origins.
				TScreenItemsGridItem *inner_grid = CreateActionsMapGrid(ixpct, num_emb_rows, num_emb_columns, context[irecord].origins);
				if (inner_grid == NULL)
				{
					// Erorr is already reported.
					return(FALSE);
				}

				// Place inner grid into the big grid.
				if (num_emb_columns*actions_map_cell_x+20 <= m_rules_grid.RowDescrClmnWidth())
				{
					// Inner matrix is not too wide. Center it in the column.
					m_rules_grid.SetupCenteredCell(this, L"ActsMapGrid1", hdr_row_inx, m_rules_grid.RowDescrClmn(), inner_grid);
				}
				else
				{
					// Matrix has moderate width. It should be left aligned.
					m_rules_grid.SetupLeftAlignedCellWithRowBkgr(this, L"ActsMapGrid2", hdr_row_inx, m_rules_grid.RowDescrClmn(), inner_grid, 0, 12);
				}

				predecessors_followers_present = FALSE;
			}

			// Display contents of the record under the actions map header.
			if (ShowCtxRecordKeyRulesAndSeqs(context[irecord], predecessors_followers_present) == FALSE)
				return(FALSE);

			if (irecord == context.NumItems()-1 || context[irecord+1].new_origins == TRUE)
				m_rules_grid.RemoveLastRowRowDelimiter();

			upper_delim_style = &m_light_delim_style;
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowGrammarConflictKeyRules(int iconflict, int iaction)
{
	// Display the conflict header row.
	if (ShowGrammarConflictHeader(iconflict, &m_dark_delim_style, TRUE, TRUE) == FALSE)
		return(FALSE);

	if (iaction >= 0)
	{
		// Show analysis just for one action.
		if (ShowConflictingActionKeyRules(iconflict, iaction, NULL) == FALSE)
			return(FALSE);
	}
	else
	{
		// Show analysis for all actions.
		int num_actions = m_grammar->conflicts[iconflict].NumActions();
		assert(num_actions > 0);

		TBasicStyle *upper_delim_style = NULL;
		for (int iact=0; iact<num_actions; ++iact)
		{
			if (ShowConflictingActionKeyRules(iconflict, iact, upper_delim_style) == FALSE)
				return(FALSE);

			upper_delim_style = &m_light_delim_style;
		}
	}

	// Success.
	return(TRUE);
}

TScreenItemsGridItem *TLangViewRulesGridDialog::CreateActionsMapGrid(int ixpct, int num_emb_rows, int num_emb_columns, TDerivationPathAnalysisHelper::TConflictContextOriginsArray &orgs)
{
	// Allocate grid for displaying key rules origins. Do not allocate rows right now because they have different height.
	TScreenItemsGridItem *inner_grid = CreateInnerGrid(0, num_emb_columns, actions_map_cell_x, 0);
	if (inner_grid == NULL)
		return(NULL);

	// Add rows to the grid.
	TScreenItemsGridRowInfo row_pros;
	for (int ir1=0; ir1<num_emb_rows; ++ir1)
	{
		long row_height;
		switch (ir1)
		{
			case 0:		row_height = 13; break;			// This row contains grammar conflict labels like "C%d".
			case 1:		row_height =  3; break;			// This tiny row is empty.
			default:
						row_height = action_map_cell_y;		// All other rows have default height.
						break;
		}

		row_pros.Setup(row_height);
		if (ir1 == 0)
			row_pros.SetupDelim(1, &m_dark_delim_style);

		if (inner_grid->AddRow(SCRITEMS_GRID_APPEND, row_pros) == FALSE)
		{
			MsgBox(L"Error adding row to the map of partcipating actions.");
			delete inner_grid;
			return(NULL);
		}
	}

	// Fill in the cells of the grid. The cells are filled column by column from left to right.
	int igrc = m_grammar->GetFirstGrammarConflictInx(ixpct);
	int inner_clmn = 0;
	while (igrc >= 0)
	{
		TScreenItemsGridCellData cell_data;

		// Display the conflict index.
		TGrammarViewLabelItem *index_scr_item = CreateSpecLabelItem(L"ConflictFollowersIndex", TGrammarViewLabelItem::gvlt_grammar_conflict, igrc);
		if (index_scr_item == NULL)
			return(FALSE);

		cell_data.Clear();
		cell_data.PlaceInTheCenter(index_scr_item, TRUE);
		inner_grid->SetCellData(0, inner_clmn, cell_data);

		// Look for origins that belong to the current conflict. Origins of the current conflict should
		// be displayed in the current column.
		for (int iorg=0; iorg<orgs.NumItems(); ++iorg)
		{
			if (orgs[iorg].iconflict == igrc)
			{
				// Current origin should be displayed.
				WORD action = m_grammar->conflicts[igrc].GetAction(orgs[iorg].iaction);
				TGrammarViewLabelItem *org_scr_item = CreateSpecLabelItem(L"ConflictFollowersOrigin", TGrammarViewLabelItem::gvlt_parsing_action, action);
				if (org_scr_item == NULL)
					return(FALSE);

				cell_data.Clear();
				cell_data.PlaceInTheCenter(org_scr_item, TRUE);
				inner_grid->SetCellData(2+orgs[iorg].iaction, inner_clmn, cell_data);
			}
		}

		// Fill in cells that were not filled in the loop above.
		int num_actions = m_grammar->conflicts[igrc].NumActions();
		for (int ia1=0; ia1<num_actions; ++ia1)
		{
			inner_grid->GetCellData(2+ia1, inner_clmn, cell_data);
			if (cell_data.m_scr_item[0] == NULL)
			{
				TTextItem80 *no_act_scr_item = CreateTextScreenItem(L"ConflictFollowersNoActionOrigin", &m_action_missing_text_style);
				if (no_act_scr_item == NULL)
					return(FALSE);

				no_act_scr_item->SetText(L"---");
				no_act_scr_item->UpdateSize(m_hDlg);

				cell_data.Clear();
				cell_data.PlaceInTheCenter(no_act_scr_item, TRUE);
				inner_grid->SetCellData(2+ia1, inner_clmn, cell_data);
			}
		}

		// Shift to the next participating grammar conflict.
		igrc = m_grammar->GetNextGrammarConflictInx(ixpct, igrc);
		inner_clmn++;
	}

	// Finalize the inner grid.
	inner_grid->UpdateGrid();
	return(inner_grid);
}

bool TLangViewRulesGridDialog::ShowConflictingActionKeyRules(int iconflict, int iaction, TBasicStyle *upper_delim_style)
{
	// Display conflicting action header row.
	if (ShowConflicingActionHeader(iconflict, iaction, upper_delim_style) == FALSE)
		return(FALSE);

	// Error messages should be displayed in the column of the first rule symbol if is its visible.
	int iclnm_message = m_rules_grid.RuleSymClmn(0);
	if (m_show_key_conflicting_rules == FALSE && m_show_predecessor_seqs == FALSE)
		iclnm_message = m_rules_grid.TrailingClmn();

	TGrammarConflictAnalysisResults &results = m_grammar->conflicts[iconflict].analysis_results;
	if (results.NumItems() != m_grammar->conflicts[iconflict].NumActions())
	{
		// Analysis records are not allocated or the number of these records is bogus.
		wchar_t buffer[120];
		if (results.NumItems() == 0)
			wcscpy(buffer, L"Conflict analysis results are missing.");
		else swprintf(buffer, 120, L"Number of analysis records is bogus. NumActions=%d, NumResultRecords=%d.", m_grammar->conflicts[iconflict].NumActions(), results.NumItems());

		// There is nothing to display.
		if (CreateMessageRow(L"NoAnResForGrammarConflict", ROW_none, iclnm_message, 6, buffer) == FALSE)
			return(FALSE);
	}
	else if (results[iaction].m_derivation_paths.NumItems() == 0)
	{
		// There is nothing to display.
		if (CreateMessageRow(L"NoPathsForGrammarConflict", ROW_none, iclnm_message, 6, L"Conflict analysis is present but it does not contain derivation paths.") == FALSE)
			return(FALSE);
	}
	else
	{
		// Instantiate the paths anaysis helper and pick up the context description.
		m_local_console.SetMajorStatus(L"Generating action context ...");
		TDerivationPathAnalysisHelper helper(m_local_console, -1, m_grammar);

		TDerivationPathAnalysisHelper::TConflictContextRecordsArray context;
		if (helper.GenerateConflictingActionContext(context, iconflict, iaction, (m_show_key_conflicting_rules == FALSE) || m_merge_diff_key_rule_seqs == TRUE) == FALSE)
		{
			// This is some low level error that has nothing to do with the grammar.
			MsgBoxFmt(MB_OK, L"Error getting conflicting action context for iconflict=%d, iaction=%d.", iconflict, iaction);
			return(FALSE);
		}
		else if (context.NumItems() == 0)
		{
			// Add explanation message.
			if (CreateMessageRow(L"NoKeyPositionsNote", ROW_none, iclnm_message, 6, L"There are no key conflicting positions for this conflicting action.") == FALSE)
				return(FALSE);
		}
		else
		{
			// Iterate the returned context.
			bool predecessors_followers_present = FALSE;
			for (int irec=0; irec<context.NumItems(); ++irec)
			{
				if (ShowCtxRecordKeyRulesAndSeqs(context[irec], predecessors_followers_present) == FALSE)
					return(FALSE);
			}

			m_rules_grid.RemoveLastRowRowDelimiter();
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowCtxRecordKeyRulesAndSeqs(TDerivationPathAnalysisHelper::TConflictContextRecord &ctx_record, bool &seqs_present)
{
	if (m_show_key_conflicting_rules == TRUE)
	{
		// Display the key rules.
		int num_key_positions = ctx_record.key_positions.NumItems();
		assert(num_key_positions > 0);

		if (seqs_present == TRUE)
		{
			// Predecessors and/or floowers are already present. Show delimeter above the rule.
			if (m_rules_grid.CreateRow(this, L"KeyConflictingRuleUpperDelim", ROW_grammar_rule_body, 1, &m_light_delim_style, NULL, ctx_record.key_positions[0].key_irule) == FALSE)
				return(FALSE);
		}

		for (int ikey=0; ikey<num_key_positions; ++ikey)
		{
			// Pick up the current key position.
			TDerivationPathAnalysisHelper::TKeyRulePosition &pos = ctx_record.key_positions[ikey];

			// Display rule that contains the key conflicting position.
			if (ShowRuleBody(SCRITEMS_GRID_APPEND, pos.key_irule, m_rules_grid.GetAlignmentSymIndex()-pos.key_isym) < 0)
				return(FALSE);

			// Show delimeter below the rule.
			if (m_rules_grid.CreateRow(this, L"KeyConflictingRuleLowerDelim", ROW_grammar_rule_body, 3, NULL, &m_light_delim_style, pos.key_irule) == FALSE)
				return(FALSE);
		}
	}

	if (m_show_predecessor_seqs == TRUE)
	{
		// Reorder the list of sequencies groupping them by the last symbol.
		ctx_record.predecessors.SortInPredecessorsStyle();

		// Display the predecessors sequences.
		for (int iseq1=0; iseq1<ctx_record.predecessors.NumItems(); ++iseq1)
		{
			int row_inx1 = m_rules_grid.CreateRow(this, L"PredecessorsSeqRow", ROW_none, symbol_seqs_row_height, &m_predec_flwr_seqs_bkgr_style, NULL);
			if (row_inx1 < 0)
				return(FALSE);

			seqs_present = TRUE;
			TSymbolsArray &seq = ctx_record.predecessors[iseq1].seq;
			if (ShowGrammarSymbols(row_inx1, m_rules_grid.GetAlignmentSymIndex()-seq.NumItems(), seq.DataPtr(), seq.NumItems(), FALSE) == FALSE)
				return(FALSE);
		}

		if (ctx_record.predecessors.NumItems() > 0 && m_show_follower_seqs == FALSE)
			m_rules_grid.CreateRow(this, L"PredecessorsSeqRowDelim", ROW_none, 2, &m_predec_flwr_seqs_bkgr_style);
	}

	if (m_show_follower_seqs == TRUE)
	{
		// Display the followers sequences.
		for (int iseq2=0; iseq2<ctx_record.followers.NumItems(); ++iseq2)
		{
			int row_inx2 = m_rules_grid.CreateRow(this, L"FollowersSeqRow", ROW_none, symbol_seqs_row_height, &m_predec_flwr_seqs_bkgr_style, NULL);
			if (row_inx2 < 0)
				return(FALSE);

			seqs_present = TRUE;
			TSymbolsArray &seq = ctx_record.followers[iseq2].seq;
			if (ShowGrammarSymbols(row_inx2, m_rules_grid.GetAlignmentSymIndex(), seq.DataPtr(), seq.NumItems(), FALSE) == FALSE)
				return(FALSE);
		}

		if (ctx_record.followers.NumItems() > 0)
			m_rules_grid.CreateRow(this, L"FollowersSeqRowDelim", ROW_none, 2, &m_predec_flwr_seqs_bkgr_style);
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowGrammarSymbols(int irow, int iclmn_start, WORD *sym_data, int sym_data_len, bool show_lex_items)
{
	// Check the columns presence.
	if (AddMissingSymbolColumns(iclmn_start, sym_data_len) == FALSE)
		return(FALSE);

	// Note that this loop allows empty sequences.
	for (int isym=0; isym<sym_data_len; ++isym)
	{
		// Create and show the symbol.
		short off_bottom = -1;
		TScreenItem *scr_item = NULL;

		if (show_lex_items == TRUE)
			scr_item = CreateSymbolLexItem(sym_data[isym], off_bottom);
		else scr_item = CreateSpecLabelItem(L"GrammarSymbol", TGrammarViewLabelItem::gvlt_grammar_symbol, sym_data[isym]);

		if (scr_item == NULL)
			return(FALSE);

		m_rules_grid.SetupCenteredCell(this, L"GrammarSymbol", irow, m_rules_grid.RuleSymClmn(iclmn_start+isym), scr_item, off_bottom);
	}

	// Full success.
	return(TRUE);
}

//
//  ===========================  Section 6  ==============================
//

bool TLangViewRulesGridDialog::ShowXpctConflictSymbolSeqs(int ixpct, bool &conflicts_avail)
{
	conflicts_avail = FALSE;
	if (ShowXpctConflictHeader(ixpct) == FALSE)
		return(FALSE);

	int iconflict = m_grammar->GetFirstGrammarConflictInx(ixpct);
	if (iconflict < 0)
	{
		// There are no participating conflicts. Add the explanation message.
		if (CreateMessageRow(L"NoGrammarConflictsNote", ROW_none, m_rules_grid.RuleSymClmn(0), 6, L"Current expected conflict does not own grammar conficts.") == FALSE)
			return(FALSE);
	}
	else
	{
		// Irerate available grammar conflicts.
		TBasicStyle *delim_style = NULL;
		while (iconflict >= 0)
		{
			if (ShowGrammarConflictSymbolSeqs(iconflict, delim_style, FALSE) == FALSE)
				return(FALSE);

			conflicts_avail = TRUE;
			delim_style = &m_light_delim_style;
			iconflict = m_grammar->GetNextGrammarConflictInx(ixpct, iconflict);
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowGrammarConflictSymbolSeqs(int iconflict, TBasicStyle *upper_delim_style, bool allow_showing_xpct_name)
{
	// Pick up the conflict.
	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
	assert(cfct.NumActions() > 0);

	// Create rows for the conflict header.
	if (ShowGrammarConflictHeader(iconflict, upper_delim_style, allow_showing_xpct_name, TRUE) == FALSE)
		return(FALSE);

	// Iterate conflicting actions.
	TBasicStyle *delim_style = NULL;
	for (int iact=0; iact<cfct.NumActions(); ++iact)
	{
		if (ShowConflictingActionSymbolSeqs(iconflict, iact, delim_style) == FALSE)
			return(FALSE);

		delim_style = &m_light_delim_style;
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowConflictingActionSymbolSeqs(int iconflict, int iaction, TBasicStyle *upper_delim_style)
{
	// Show the action header.
	if (ShowConflicingActionHeader(iconflict, iaction, upper_delim_style) == FALSE)
		return(FALSE);

	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
	assert(cfct.NumActions() > 0);

	if (cfct.analysis_results.NumItems() != cfct.NumActions())
	{
		wchar_t buffer[120];
		if (cfct.analysis_results.NumItems() == 0)
			wcscpy(buffer, L"Conflict analysis results are missing.");
		else swprintf(buffer, 120, L"Number of analysis records is bogus. NumActions=%d, NumResultRecords=%d.", cfct.NumActions(), cfct.analysis_results.NumItems());

		if (CreateMessageRow(L"NoAnResForSymbolsSeq", ROW_none, m_rules_grid.RuleSymClmn(0), 6, buffer) == FALSE)
			return(FALSE);
	}
	else
	{
		// Analysis results for requested action are available.
		int num_paths = cfct.analysis_results[iaction].m_derivation_paths.NumItems();
		if (num_paths == 0)
		{
			if (CreateMessageRow(L"NoPathsForSymbolsSeq", ROW_none, m_rules_grid.RuleSymClmn(0), 6, L"Derivation paths are missing.") == FALSE)
				return(FALSE);
		}
		else
		{
			// Iterate paths of the conflicting action.
			for (int ipath=0; ipath<num_paths; ++ipath)
			{
				if (ShowDerivationPathAsSymbolsSeq(iconflict, iaction, ipath, FALSE) == FALSE)
					return(FALSE);
			}

			m_rules_grid.RemoveLastRowRowDelimiter();
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowDerivationPathAsSymbolsSeq(int iconflict, int iaction, int ipath, bool show_path_in_lex_items)
{
	// Pick up the derivation path. This is protected method. The path should be existing.
	TGrammarConflict &cfct = m_grammar->conflicts[iconflict];
	TGrammarDerivationPath &path = cfct.analysis_results[iaction].m_derivation_paths[ipath];

	// Convert path into the symbol sequences.
	m_local_console.SetMajorStatus(L"Generating path context ...");
	TDerivationPathAnalysisHelper helper(m_local_console, -1, m_grammar);

	TSymbolsArray predecs, flwrs;
	if (helper.GetDerivationPathContext(predecs, flwrs, iconflict, iaction, ipath) == FALSE)
	{
		MsgBoxFmt(MB_OK, L"Error getting conflicting path context for iconflict=%d, iaction=%d, ipath=%d.", iconflict, iaction, ipath);
		return(FALSE);
	}

	// Create row for the sequence.
	int row_height = (show_path_in_lex_items == TRUE) ? rule_body_row_height : symbol_seqs_row_height;
	int row_inx = m_rules_grid.CreateRow(this, L"SymbolsSeqRow", ROW_derivation_path_body, row_height, NULL, NULL, iconflict, iaction, ipath);
	if (row_inx < 0)
		return(FALSE);

	if (m_show_path_seq_props == TRUE)
	{
		// Put labels.
		int hidden_key_step = path.GetHiddenConflictPosition();
		assert(hidden_key_step >= 0 && hidden_key_step < path.derv_path_len);
		TFullRulePosition &key_pos = path.derv_steps[hidden_key_step];

		if (CreateCenteredTextLabel(L"PathInx", row_inx, m_rules_grid.RuleIndexClmn(), 0, &m_rule_index_text_style, L"Path %d.", ipath) == FALSE)
			return(FALSE);
		if (CreateCenteredTextLabel(L"PathKeyPos", row_inx, m_rules_grid.RuleNonTermClmn(), 0, &m_rule_index_text_style, L"Key pos:  R%hd, sym %hd.", key_pos.irule, key_pos.isym) == FALSE)
			return(FALSE);
	}

	if (predecs.NumItems() > 0)
	{
		// Show the leading sequence.
		if (ShowGrammarSymbols(row_inx, m_rules_grid.GetAlignmentSymIndex()-predecs.NumItems(), predecs.DataPtr(), predecs.NumItems(), show_path_in_lex_items) == FALSE)
			return(FALSE);
	}

	// Sequence of followers should be always non empty because it should contain at least the conflicting symbol.
	assert(flwrs.NumItems() > 0);
	if (ShowGrammarSymbols(row_inx, m_rules_grid.GetAlignmentSymIndex(), flwrs.DataPtr(), flwrs.NumItems(), show_path_in_lex_items) == FALSE)
		return(FALSE);

	// Add small space below the symbols and light delimiter.
	if (m_rules_grid.CreateRow(this, L"SymbolsSeqLowerDelim", ROW_derivation_path_body, 1, NULL, &m_light_delim_style, iconflict, iaction, ipath) < 0)
		return(FALSE);

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::ShowDerivationStepDetails(int irow, int ipath, int istep, TFullRulePosition &path_slot, int special_rule_index)
{
	TTextItem80 *line1 = CreateTextScreenItem(L"DervStepOrg1", &m_row_descr_text_style);
	if (line1 == NULL)
	{
		return(FALSE);
	}

	TTextItem80 *line2 = CreateTextScreenItem(L"DervStepOrg2", &m_row_descr_text_style);
	if (line2 == NULL)
	{
		delete line1;
		return(FALSE);
	}

	if (special_rule_index < 0)
	{
		// This is ordinary case.
		switch (path_slot.origin)
		{
			case rpo_initial_placement:
					line1->SetTextFmt(L"%d. Initail placement.", istep);
					line2->SetTextFmt(L"Path index %d.", ipath);
					break;

			case rpo_rule_start_state:
			case rpo_start_state_ctx:
					line1->SetTextFmt(L"%d. Select state %d as starting", istep, path_slot.istate);
					line2->SetTextFmt(L"state for R%d.", path_slot.irule);
					break;

			case rpo_rule_call_place:
			case rpo_call_place_ctx:
					{
						if (path_slot.origin == rpo_rule_call_place && istep == 1)
						{
							line1->SetTextFmt(L"%d. Select R%d, symbol pos %d", istep, path_slot.irule, path_slot.isym);
							line2->SetTextFmt(L"as initial placement.");
						}
						else
						{
							line1->SetTextFmt(L"%d. Select R%d, symbol pos %d", istep, path_slot.irule, path_slot.isym);
							line2->SetTextFmt(L"as call place for symbol %d.", m_grammar->rules[path_slot.irule].symbols[path_slot.isym]);
						}
					}
					break;

			case rpo_step_over_symbol:
					line1->SetTextFmt(L"%d. Step over the symbol.", istep);
					break;

			case rpo_step_over_non_term:
					line1->SetTextFmt(L"%d. Step over the transparent", istep);
					line2->SetTextFmt(L"non terminal symbol.");
					break;

			case rpo_non_term_defn_rule:
					line1->SetTextFmt(L"%d. Select R%d as definition", istep, path_slot.irule);
					line2->SetTextFmt(L"rule for non terminal %d.", m_grammar->rules[path_slot.irule].non_term);
					break;

			case rpo_step_up_the_tree:
			case rpo_step_up_ctx:
					line1->SetTextFmt(L"%d. Step up the tree.", istep);
					break;

			case rpo_axioma_action_sym:
			case rpo_axioma_action_ctx:
					line1->SetTextFmt(L"%d. Select symbol %d as action", istep, path_slot.action_sym);
					line2->SetTextFmt(L"symbol of the axioma.");
					break;

			case rpo_step_over_ctx:
					line1->SetTextFmt(L"%d. Step over the symbol(s)", istep);
					line2->SetText(L"to get more context.");
					break;

			default:
				assert(FALSE);
				break;
		}
	}
	else
	{
		// Current slot is the sum of several slots.
		assert(path_slot.origin == rpo_step_over_symbol);
		line1->SetTextFmt(L"%d. Select R%d, symbol pos %d", istep, path_slot.irule, path_slot.isym);
		line2->SetTextFmt(L"as rule call place for R%d.", special_rule_index);
	}

	line1->UpdateSize(m_hDlg);
	line2->UpdateSize(m_hDlg);

	// Setup the cell.
	TScreenItemsGridCellData cell_data;
	cell_data.Clear();
	cell_data.PlaceLeftCenter(NULL, FALSE);
	cell_data.PlaceAtLayer(1, line1, TRUE, 7, -7);
	cell_data.PlaceAtLayer(0, line2, TRUE, 7,  9);
	m_rules_grid.GetGrid().SetCellData(irow, m_rules_grid.RowDescrClmn(), cell_data);

	// Success.
	return(TRUE);
}

//
//  ===========================  Section 7  ==============================
//

void TLangViewRulesGridDialog::ResetSecondaryFlags()
{
	// Primary view type flags.
	for (int inx = grdm_none; inx<grdm_num_types; ++inx)
	{
		m_zoom_level[inx] = 1.0;
		m_tree_placing_mode[inx] = plc_mode_concentric;
	}

	// Default mode for non term depends and for derivation path as AST tree is rect lines.
	m_tree_placing_mode[grdm_non_term_depends] = plc_mode_rect_lines;
	m_tree_placing_mode[grdm_deriv_path_ast] = plc_mode_rect_lines;

	// General flags.
	m_hgtlt_transp_non_terms = FALSE;
	m_show_terminals_grm_names = FALSE;

	// Grammars tree overview.
	m_show_individual_grm_conflicts = FALSE;

	// Symbol lists and symbol dependencies.
	m_terminals_sort_alpha_num = FALSE;
	m_non_terms_list_sort_mode = ntsm_sect_rule;
	m_show_non_term_starters = FALSE;
	m_non_term_deps_show_terms = FALSE;

	// State transitions in rules and states view.
	m_pstt_trnv_mode[0] = pstt_trnv_show_act_syms;
	m_pstt_trnv_mode[1] = pstt_trnv_none;
	m_pstt_trnv_mode[2] = pstt_trnv_show_act_syms;

	// Symbol use report.
	m_sym_rprt_show_defn_rules = TRUE;
	m_sym_rprt_show_sym_uses = TRUE;
	m_sym_rprt_highlight_sym = TRUE;

	// List of all parsing states and display of single parsing state.
	m_states_show_interm_positions = FALSE;

	// Misc views with grammmar conflict headers.
	m_show_xpct_conflict_name = TRUE;

	// Conflicts overview flags.
	m_show_conflicting_action_hdrs = TRUE;
	m_show_directly_conflicting_rules = TRUE;
	m_show_with_analysis_only = FALSE;

	// Predecessors/followers report.
	m_show_key_conflicting_rules = TRUE;
	m_merge_diff_key_rule_seqs = FALSE;
	m_show_predecessor_seqs = FALSE;
	m_show_follower_seqs = TRUE;

	// Derivation paths/path as sequecies/sequecie of symbols.
	m_show_path_seq_props = TRUE;
	m_show_single_path_seq_lex_items = TRUE;

	// Derivation path and symbol use report.
	m_show_deriv_path_dtls = TRUE;
	m_show_deriv_path_from_bottom = TRUE;
}

void TLangViewRulesGridDialog::FixSimpleParam(int &param)
{
	// Fix possible 16-bits problems in the passed param.
	if (param < -1 || param >= 0xFFFF)
		param = -1;
}

void TLangViewRulesGridDialog::FixGrammarConflictParam(int &param)
{
	// Fix possible 16-bits problems in the passed param.
	if (param < -4)
		param = -4;
	else if (param >= 0x8000)
		param |= 0xFFFF0000;
}

void TLangViewRulesGridDialog::StoreMajorTaskParams(TLangViewRulesGridDialogMode mode, TGrammar *grm, int inx_lev2, int inx_lev3, int inx_lev4)
{
	if (m_step_back_proc_flag == FALSE)
	{
		TDialogStateInfo new_state_info = { mode, grm, inx_lev2, inx_lev3, inx_lev4 };
		if (m_dialog_mode != grdm_none)
		{
			TDialogStateInfo curr_state_info;
			PlaceCurrentContentsTo(curr_state_info);
			if (new_state_info.CompareTo(curr_state_info) == FALSE)
			{
				// Current state is not empty and new state differs from the current state. Save the current state.
				m_contents_history.PushItem(curr_state_info);
			}
		}
		else
		{
			if (m_contents_history.CompareWithTipItem(new_state_info) == TRUE)
			{
				// Current state is empty and new state is the same to the recent saved state. This situation is basically
				// the same to what happens when the step back is done. Discard one history item.
				TDialogStateInfo old_state_info;
				m_contents_history.PopItem(old_state_info);
			}
		}
	}

	m_dialog_mode = mode;
	m_grammar = grm;
	m_single_obj_index = inx_lev2;
	m_single_sub_obj_index = inx_lev3;
	m_single_sub_sub_obj_index = inx_lev4;
}

void TLangViewRulesGridDialog::SetUpdateControlsLayout()
{
	// Pick up the main dialog rect and the title control rects.
	RECT rc_client, rc_view_name, rc_grm_name, rc_main_view;
	::GetClientRect(m_hDlg, &rc_client);
	GetControlRect(IDC_RULGRID_CURR_VIEW_TITLE, rc_view_name);
	GetControlRect(IDC_RULGRID_GRM_NAME_TITLE, rc_grm_name);
	GetControlRect(IDC_RULGRID_SCR_ITEMS_VIEW, rc_main_view);
	rc_main_view.top = m_initial_main_view_top;
	rc_main_view.bottom = rc_client.bottom-m_main_view_off_bottom;

	// Do not show titles if the viewer is empty.
	bool show_dialog_mode = (m_dialog_mode != grdm_none) ? m_show_dialog_mode : FALSE;
	bool show_grammar_name = (m_dialog_mode != grdm_none) ? m_show_grammar_name : FALSE;

	// Process the first title.
	ShowControl(IDC_RULGRID_CURR_VIEW_TITLE, show_dialog_mode);
	if (show_dialog_mode == TRUE)
	{
		SetControlPos(IDC_RULGRID_CURR_VIEW_TITLE, rc_view_name.left, rc_main_view.top-2);
		rc_main_view.top += (rc_view_name.bottom-rc_view_name.top);
	}

	// Process the second title.
	ShowControl(IDC_RULGRID_GRM_NAME_TITLE, show_grammar_name);
	if (show_grammar_name == TRUE)
	{
		SetControlPos(IDC_RULGRID_GRM_NAME_TITLE, rc_grm_name.left, rc_main_view.top);
		rc_main_view.top += (rc_grm_name.bottom-rc_grm_name.top)+1;
	}

	// Setup the main view control.
	SetControlRect(IDC_RULGRID_SCR_ITEMS_VIEW, rc_main_view);

	// Prepare resizing.
	SetupControlPositioning(IDC_RULGRID_CURR_VIEW_TITLE, cpm_fix_top_side);
	SetupControlPositioning(IDC_RULGRID_GRM_NAME_TITLE, cpm_fix_top_side);
	SetupControlPositioning(IDC_RULGRID_SCR_ITEMS_VIEW, cpm_fix_all_corners);
}

void TLangViewRulesGridDialog::PrepareNewDialogMode(const wchar_t *mode_name, const wchar_t *status_panel_msg)
{
	assert(m_dialog_mode != grdm_none);
	SetControlText(IDC_RULGRID_CURR_VIEW_TITLE, mode_name);
	if (m_show_grammar_name == TRUE)
	{
		TNameBuffer buffer;
		m_grammar->GenerateGrammarName(buffer, L"/");
		SetControlTextFmt(IDC_RULGRID_GRM_NAME_TITLE, L"Grammar (%d):  %s", m_grammar->GetGrammarIndex(), buffer.DataPtr());
	}

	// Finally show the headers if needed.
	SetUpdateControlsLayout();

	// Reload the profile.
	LoadStylesProfile(TRUE);

	// Prepare the lex items style. It should be prepared for displaying non terminals of the passed grammar.
	m_lex_items_style.SetupLexItemsProps(m_grammar, 0, m_show_terminals_grm_names, m_hgtlt_transp_non_terms, TLangViewLexItemStyle::g_DefaultProfileName, &m_local_console);
	m_lex_items_style.SetupForDeviceContextEx(m_hDlg);

	// Initate the status panel because creationof the grid/tree may require long processing.
	m_local_console.SetAbortFlag(FALSE);
	m_local_console.PrepareStatusPanel(status_panel_msg, FALSE);

	// Repaint the window before starting the long processing.
	UpdateDialogWindow();
}

bool TLangViewRulesGridDialog::CheckCurrGrammarForXpctConflictsPresence()
{
	if (m_grammar->grm_props.processing_result != grpr_full_success)
	{
		// Show just one static label.
		PrepareAndShowGrammarNotInFullSucceesMessage();
		return(FALSE);
	}
	else if (m_grammar->NumXpctConflicts() == 0)
	{
		// Show just one static label.
		PrepareAndShowNoObjectsMessage(L"The grammar has no expected conflicts.");
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::CheckCurrGrammarForGrammarConflictsPresence(int display_task)
{
	if (m_grammar->grm_props.processing_result != grpr_full_success)
	{
		// Show just one static label.
		PrepareAndShowGrammarNotInFullSucceesMessage();
		return(FALSE);
	}
	else if (m_grammar->NumConflicts() == 0)
	{
		// Show just one static label.
		PrepareAndShowNoObjectsMessage(L"The grammar has no conflicts.");
		return(FALSE);
	}
	else if (display_task >= -3 && display_task <= -1)
	{
		if (m_grammar->GetFirstGrammarConflictInx(display_task) < 0)
		{
			// Show just one static label.
			PrepareAndShowNoObjectsMessage(L"The grammar has no conflicts of the requested category.");
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::PrepareGridCreation()
{
	TBasicStyle *alignment_clmn_bkgr_style = NULL;
	bool want_row_descr_clmn = FALSE;
	bool want_rind_and_nterm_clms = TRUE;
	bool want_wide_rule_index_clmn = FALSE;
	bool want_wide_alignment_clmn = FALSE;
	bool want_alignment_clmn_ext_spc = FALSE;
	bool want_wide_trailing_clmn = FALSE;

	switch (m_dialog_mode)
	{
		case grdm_parsing_states:
				alignment_clmn_bkgr_style = &m_pars_stt_align_clmn_style;
				want_wide_rule_index_clmn = TRUE;
				if (m_single_obj_index < 0)
				{
					want_alignment_clmn_ext_spc = TRUE;
					want_wide_trailing_clmn = TRUE;
				}
				break;

		case grdm_processing_status:
				want_row_descr_clmn = TRUE;
				break;

		case grdm_xpct_conflicts:
		case grdm_grammar_conflicts:
				want_row_descr_clmn = TRUE;
				alignment_clmn_bkgr_style = &m_conflict_align_clmn_style;
				want_wide_alignment_clmn = TRUE;
				want_alignment_clmn_ext_spc = TRUE;
				break;

		case grdm_key_rules_xpct:
		case grdm_key_rules_conflict:
				want_row_descr_clmn = TRUE;
				want_rind_and_nterm_clms = m_show_key_conflicting_rules;
				alignment_clmn_bkgr_style = &m_conflict_align_clmn_style;
				want_wide_alignment_clmn = TRUE;
				break;

		case grdm_dplist_symbols_xpct:
		case grdm_dplist_symbols_conflict:
		case grdm_dplist_symbols_action:
				want_row_descr_clmn = TRUE;
				want_rind_and_nterm_clms = m_show_path_seq_props;
				want_wide_rule_index_clmn = TRUE;
				alignment_clmn_bkgr_style = &m_derv_path_align_clmn_style;
				break;

		case grdm_deriv_path_seq:
				alignment_clmn_bkgr_style = &m_derv_path_align_clmn_style;
				want_rind_and_nterm_clms = m_show_path_seq_props;
				want_wide_rule_index_clmn = TRUE;
				break;

		case grdm_deriv_path_steps:
				want_row_descr_clmn = TRUE;
				alignment_clmn_bkgr_style = &m_derv_path_align_clmn_style;
				want_alignment_clmn_ext_spc = TRUE;
				break;
	}

	// Create initial layout of the grid.
	m_rules_grid.SetExtraParsSttClmnMode((m_grammar != NULL && m_grammar->NumParsingStates() >= 10000) ? TRUE : FALSE);
	if (m_rules_grid.CreateInitialLayout(this, &m_dark_delim_style, &m_non_term_clmn_style, &m_symbols_vert_delim_style,
								alignment_clmn_bkgr_style, want_row_descr_clmn, want_rind_and_nterm_clms, want_wide_rule_index_clmn,
								(alignment_clmn_bkgr_style != NULL), want_wide_alignment_clmn, want_alignment_clmn_ext_spc,
								want_wide_trailing_clmn) == FALSE)
	{
		m_local_console.CloseStatusPanel();
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

int TLangViewRulesGridDialog::CreateHeaderRows(const wchar_t *rows_descr, TLangViewRulesGridRowMeaning row_meaning,
									TBasicStyle *upper_delim_style, int header_height, TBasicStyle *bkgr_style, TBasicStyle *lower_delim_style,
									WORD inx2, WORD inx3, int inx4, int ins_before_row, TLangViewRulesGridRowMeaningEx ex_meaning)
{
	// Create the optional upper header row one pixel high.
	if (upper_delim_style != NULL)
	{
		if (m_rules_grid.CreateRow(this, rows_descr, row_meaning, 1, upper_delim_style, NULL, inx2, inx3, inx4, ins_before_row, ex_meaning) < 0)
			return(-1);

		if (ins_before_row != SCRITEMS_GRID_APPEND)
			ins_before_row++;
	}

	// Create the main header row.
	int row_inx = m_rules_grid.CreateRow(this, rows_descr, row_meaning, header_height, bkgr_style, lower_delim_style, inx2, inx3, inx4, ins_before_row, ex_meaning);
	return(row_inx);
}

bool TLangViewRulesGridDialog::CreateMessageRow(const wchar_t *row_descr, TLangViewRulesGridRowMeaning row_meaning,
									int iclmn, long horz_offset, const wchar_t *message, TBasicStyle *lower_delim_style,
									WORD inx2, WORD inx3, int inx4, int ins_before_row)
{
	// Add row for the message.
	int row_inx = m_rules_grid.CreateRow(this, row_descr, row_meaning, 23, NULL, lower_delim_style, inx2, inx3, inx4, ins_before_row);
	if (row_inx < 0)
		return(FALSE);

	// Add explanation message.
	TTextItem256 *note = new TTextItem256(&m_rule_index_text_style);
	if (note == NULL)
	{
		MsgBoxFmt(MB_OK, L"Message row (%s) creation failed.", (row_descr != NULL) ? row_descr : L"<NoDescr>");
		return(FALSE);
	}

	note->SetText(message);
	note->UpdateSize(m_hDlg);
	m_rules_grid.SetupCenteredCell(this, row_descr, row_inx, iclmn, note, 6);

	// Shift the message a little bit down.
	m_rules_grid.ShiftMainLayerItem(row_inx, iclmn, 0, 3);

	// Success.
	return(TRUE);
}

void TLangViewRulesGridDialog::AddThreeRowsFooter(const wchar_t *rows_descr, int middle_row_height, TBasicStyle *middle_row_bkgr_style, bool want_full_footer)
{
	// Create next row only if creation of the previous rows succeeded to avoid showing several error messages.
	int irow = 0;

	if (want_full_footer == TRUE)
	{
		irow = m_rules_grid.CreateRow(this, rows_descr, ROW_none, 1, &m_dark_delim_style);
		if (irow >= 0)
			irow = m_rules_grid.CreateRow(this, rows_descr, ROW_none, middle_row_height, middle_row_bkgr_style, &m_light_delim_style);
	}

	if (irow >= 0)
		m_rules_grid.CreateRow(this, rows_descr, ROW_none, 14);
}

void TLangViewRulesGridDialog::PrepareAndShowCurrentGrid()
{
	// Setup the view mode specific zoom level.
	m_view_helper.SetZoomLevel(m_zoom_level[m_dialog_mode]);

	if (m_rules_grid.IsNonTermClmnPresent() == TRUE)
	{
		m_local_console.SetMinorStatus(L"Updating non terminal column width ...");

		// Setup rule non term column width.
		int nterm_clmn_min_width = m_rules_grid.RuleSymbolClmnWidth();
		if (m_dialog_mode == grdm_dplist_symbols_xpct || m_dialog_mode == grdm_dplist_symbols_conflict || m_dialog_mode == grdm_dplist_symbols_action || m_dialog_mode == grdm_deriv_path_seq)
			nterm_clmn_min_width += 60;

		m_rules_grid.UpdateGridColumnWidth(m_rules_grid.RuleNonTermClmn(), nterm_clmn_min_width);
	}

	// Setup the symbol column widths.
	int symbol_clmn_min_width = (2*m_rules_grid.RuleSymbolClmnWidth())/3;
	if (m_dialog_mode == grdm_dplist_symbols_xpct || m_dialog_mode == grdm_dplist_symbols_conflict || m_dialog_mode == grdm_dplist_symbols_action)
		symbol_clmn_min_width -= 40;
	else if ((m_dialog_mode == grdm_key_rules_conflict || m_dialog_mode == grdm_key_rules_xpct) && m_show_key_conflicting_rules == FALSE)
		symbol_clmn_min_width -= 40;
	else if (m_dialog_mode == grdm_deriv_path_seq && m_show_single_path_seq_lex_items == FALSE)
		symbol_clmn_min_width -= 40;

	m_local_console.SetMinorStatusAndLastUpdateTime(NULL);
	for (int iclmn=0; iclmn<m_rules_grid.NumSymbolColumns(); ++iclmn)
	{
		m_rules_grid.UpdateGridColumnWidth(m_rules_grid.RuleSymClmn(iclmn), symbol_clmn_min_width);
		m_local_console.UpdateStatusPanel(iclmn, m_rules_grid.NumSymbolColumns(), L"Column widths:  ");
	}

	// In rare cases the trailing column may contain messages. Process this column to align these messages.
	m_rules_grid.UpdateGridColumnWidth(m_rules_grid.TrailingClmn(), 20);

	if ((m_dialog_mode == grdm_key_rules_conflict || m_dialog_mode == grdm_key_rules_xpct) && m_show_key_conflicting_rules == FALSE && m_show_predecessor_seqs == FALSE)
	{
		// In this situation the symbol column[0] is always empty. Remove it.
		m_rules_grid.RemoveRuleSymbolColumn(this, 0);
	}

	// Ensure that the whole grid is wide enough to display the names of expected conflicts.
	m_local_console.SetMinorStatusAndLastUpdateTime(NULL);
	int avail_width = m_rules_grid.GetGrid().GetColumnOffsetExact(SCRITEMS_GRID_NUM_COLUMNS)-m_rules_grid.GetGrid().GetColumnOffsetExact(m_rules_grid.RuleFrontSpaceClmn());
	for (int irow=0; irow<m_rules_grid.NumRows(); ++irow)
	{
		TLangViewRulesGridRowMeaning rm = m_rules_grid.GetRowMeaning(irow);
		if (rm == ROW_xpct_conflict_header || rm == ROW_grammar_conflict_header)
		{
			// These types of rows may carry in the rule front space column the name of the xpct conflict and
			// at the same time these names can be present only there.
			TScreenItemsGridCellData cd1;
			m_rules_grid.GetGrid().GetCellData(irow, m_rules_grid.RuleFrontSpaceClmn(), cd1);
			int req_width = 0;
			if (cd1.m_scr_item[1] != NULL)
				req_width = cd1.m_scr_item_shift_x[1]+cd1.m_scr_item[1]->GetWidth();
			else if (cd1.m_scr_item[0] != NULL)
				req_width = cd1.m_scr_item_shift_x[0]+cd1.m_scr_item[0]->GetWidth();

			if (req_width > avail_width)
			{
				// The grid is not wide enough.
				TScreenItemsGridColumnInfo clmn_info;
				m_rules_grid.GetGrid().GetColumnInfo(m_rules_grid.TrailingClmn(), clmn_info);
				clmn_info.m_width += req_width-avail_width;
				m_rules_grid.GetGrid().UpdateColumnInfo(m_rules_grid.TrailingClmn(), clmn_info);
				avail_width += req_width-avail_width;
			}
		}
	}

	// Layout of the grid is finalized. Setup the screen item positions in all cells of the grid.
	m_rules_grid.GetGrid().UpdateGrid();

	//
	//  Setup the links between:
	//
	//     --  source and destination parsing states.
	//     --  final parsing states and rule action symbols.
	//
	//  Obscure the vertical lines behind the action symbol grids.
	//  Set position to fancy rects that should obscure the column delims.
	//
	int state_transitions_range_beg = -1;
	int isym_to_obscure_beg = -1, isym_to_obscure_end = -1;
	for (int row_inx=0; row_inx<m_rules_grid.NumRows(); ++row_inx)
	{
		TLangViewRulesGridRowMeaning row_meaning = m_rules_grid.GetRowMeaning(row_inx);
		if (row_meaning == ROW_xpct_conflict_header || row_meaning == ROW_grammar_conflict_header)
		{
			// Check for the rect item in the layer[2] on this row. If it is present there, this is horz fancy obscuring lines.
			TScreenItemsGridCellData cd2a;
			m_rules_grid.GetGrid().GetCellData(row_inx, m_rules_grid.RuleFrontSpaceClmn(), cd2a);
			if (cd2a.m_scr_item[2] != NULL)
			{
				long row_height = m_rules_grid.GetGrid().RowHeightWithoutDelim(row_inx);

				// Use background rect from the layer[1] if it is available. Otherwise use text item from layer[0] and add 20 extra pixels.
				long extra_width = 0;
				TScreenItem *ref_item = cd2a.m_scr_item[1];
				if (ref_item == NULL)
				{
					extra_width = 20;
					ref_item = cd2a.m_scr_item[0];
					if (ref_item == NULL)
					{
						// This should be the upper delim line. Pick up the item from the next line.
						assert(row_height == 1 && row_inx < m_rules_grid.GetGrid().NumRows()-1);

						extra_width = 0;
						ref_item = m_rules_grid.GetGrid().GetScreenItem(row_inx+1, m_rules_grid.RuleFrontSpaceClmn(), 1);
						if (ref_item == NULL)
						{
							extra_width = 20;
							ref_item = m_rules_grid.GetGrid().GetMainLayerScreenItem(row_inx+1, m_rules_grid.RuleFrontSpaceClmn());
							assert(ref_item != NULL);
						}
					}
				}

				// When current line is the upper delim that is one pixel high, the rect should be placed at the top of the row.
				// Otherwise, when the row is bigger, the rect should obscure the lower delim that is again one pixel high.
				long pos_y = m_rules_grid.GetGrid().GetRowOffset(row_inx);
				if (row_height > 1)
					pos_y += row_height;

				// In the horz direction the rect should have the same position/size as the ref item.
				TRectItem *rect_item = (TRectItem*)cd2a.m_scr_item[2];
				rect_item->SetHotSpotPos(ref_item->GetBoundingRectX(), pos_y);
				rect_item->SetBaseRectSize(ref_item->GetWidth()+extra_width, 1);

				// Set this rect back into the grid.
				m_rules_grid.GetGrid().SetPositionedScreenItemAtLayerNoInvalidate(row_inx, m_rules_grid.RuleFrontSpaceClmn(), 2, rect_item, TRUE);
			}
		}

		if (row_meaning != ROW_grammar_rule_body)
			continue;

		// Current row is the rule body row.
		if (m_rules_grid.GetRowMeaningEx(row_inx) == ROW_EX_state_transitions && state_transitions_range_beg == -1)
		{
			// Current row starts the range of rows that shows the parsing state transitions.
			state_transitions_range_beg = row_inx;
			isym_to_obscure_beg = isym_to_obscure_end = -1;
		}

		if (m_rules_grid.GetRowMeaningEx(row_inx) != ROW_EX_state_transitions && state_transitions_range_beg != -1)
		{
			// Current row is a row that immediately follows the range of rows with parsing state transitions.
			if (isym_to_obscure_beg != -1)
			{
				// Obscure the state transition alignment lines.
				for (int row_to_obsc=state_transitions_range_beg; row_to_obsc<row_inx; ++row_to_obsc)
				{
					for (int isym_to_obsc=isym_to_obscure_beg; isym_to_obsc <= isym_to_obscure_end; ++isym_to_obsc)
					{
						// Current row and symbol column needs to be obscured.
						TRectItem *obcs_item = new TRectItem(&m_light_syms_vert_delim_style);
						if (obcs_item == NULL)
						{
							MsgBox(L"Unablde to create the obscure screen item.");
							continue;
						}

						long cell_pos_x, cell_pos_y;
						int clmn_to_obsc = m_rules_grid.ParsSttClmn(isym_to_obsc);
						m_rules_grid.GetGrid().GetCellPosition(row_to_obsc, clmn_to_obsc, cell_pos_x, cell_pos_y);

						obcs_item->SetBoundingRectPos(cell_pos_x, cell_pos_y);
						obcs_item->SetBaseRectSize(m_rules_grid.GetGrid().ColumnWidth(clmn_to_obsc), m_rules_grid.GetGrid().RowHeight(row_to_obsc));
						m_rules_grid.GetGrid().SetPositionedScreenItemAtLayerNoInvalidate(row_to_obsc, clmn_to_obsc, 2, obcs_item, TRUE);
					}
				}
			}

			// Reset the rows range flag.
			state_transitions_range_beg = -1;
		}

		// Current row may have parsing states. Iterate rule symbol columns of the grid.
		for (int iclmn=0; iclmn <= m_rules_grid.NumSymbolColumns(); ++iclmn)
		{
			int num_pstt_clmns = m_rules_grid.NumParsSttClmns(iclmn);
			int iclmn_first = m_rules_grid.ParsSttClmn(iclmn, -num_pstt_clmns/2);

			// Iterate the parsing state subcolumns of the curr symbol column.
			for (int clmn_inx=iclmn_first; clmn_inx<iclmn_first+num_pstt_clmns; ++clmn_inx)
			{
				// Check the primary item of the cell.
				TGrammarViewLabelItem *state_scr_item = (TGrammarViewLabelItem*)m_rules_grid.GetGrid().GetMainLayerScreenItem(row_inx, clmn_inx);
				if (state_scr_item != NULL)
				{
					//
					// Curr cell contains the parsing state label.
					//

					// Check for dependent objects.
					if (state_scr_item->m_dest_link_object != NULL)
					{
						// Resetup the link and place it into the layer [1] of the same cell of the main grid.
						state_scr_item->SetupLink(state_scr_item->m_dest_link_object, state_scr_item->m_dest_link.Style());
						m_rules_grid.GetGrid().SetPositionedScreenItemAtLayerNoInvalidate(row_inx, clmn_inx, 1, &(state_scr_item->m_dest_link), FALSE);
					}
					else if (state_scr_item->m_action_symbols_grid != NULL)
					{
						// The action symbols grid is present.
						TScreenItemsGridItem *inner_grid = state_scr_item->m_action_symbols_grid;

						// Setup the position of the inner grid.
						long state_scr_item_height = state_scr_item->GetHeight();
						long state_scr_item_width = state_scr_item->GetWidth();
						long inner_grid_height = inner_grid->GetHeight();

						long state_scr_item_x, state_scr_item_y;
						state_scr_item->GetHotSpotPos(state_scr_item_x, state_scr_item_y);

						long inner_grid_x = state_scr_item_x+state_scr_item_width/2+60;
						long inner_grid_y = state_scr_item_y-(inner_grid_height-state_scr_item_height)/2-1;
						inner_grid->SetHotSpotPos(inner_grid_x, inner_grid_y);
						inner_grid->UpdateGrid();

						// Setup the links for all non empty cells in the inner grid.
						long  inner_grid_row_y = inner_grid_y;
						for (int inner_row=0; inner_row<inner_grid->NumRows(); ++inner_row)
						{
							long special_src_y = inner_grid_row_y+inner_grid->RowHeight(inner_row)/2;
							long prev_act_sym_width = -1, prev_act_sym_end = -1;

							for (int inner_clmn=0; inner_clmn<inner_grid->NumColumns(); ++inner_clmn)
							{
								TGrammarViewLabelItem *act_sym = (TGrammarViewLabelItem*)inner_grid->GetMainLayerScreenItem(inner_row, inner_clmn);
								if (act_sym != NULL)
								{
									// The current cell is not empty.
									if (prev_act_sym_width >= 0)
									{
										// Current symbol is not the first symbol in the row.
										long min_spacing = 9;
										if (prev_act_sym_width > 32 && act_sym->GetWidth() > 32)
										{
											// Both labels are wide. Use smaller min space.
											min_spacing = 4;
										}

										if (act_sym->GetBoundingRectX() < prev_act_sym_end+min_spacing)
										{
											// Current spacing between the labels is not big enough.
											act_sym->SetBoundingRectPos(prev_act_sym_end+min_spacing, act_sym->GetBoundingRectY());
											inner_grid->SetPositionedScreenItemAtLayerNoInvalidate(inner_row, inner_clmn, 0, act_sym, TRUE);
										}
									}

									act_sym->SetupLink(act_sym->m_dest_link_object, act_sym->m_dest_link.Style(), &special_src_y);
									inner_grid->SetPositionedScreenItemAtLayerNoInvalidate(inner_row, inner_clmn, 1, &(act_sym->m_dest_link), FALSE);

									// Save the horz info about the current action sym.
									prev_act_sym_width = act_sym->GetWidth();
									prev_act_sym_end = act_sym->GetBoundingRect()->right;
								}
							}

							inner_grid_row_y += inner_grid->RowHeight(inner_row);
						}

						// Finalize the inner grid and place it into the main grid.
						inner_grid->RebuildRects();
						m_rules_grid.GetGrid().SetPositionedScreenItemAtLayerNoInvalidate(row_inx, clmn_inx, 1, inner_grid, FALSE);

						// Figure out what column delimiters this action symbols grid should obscure.
						if (iclmn < m_rules_grid.NumSymbolColumns())
						{
							// Calc the horz position of the last pixel that should be obscured.
							long last_obscured_offs_x = inner_grid_x+inner_grid->GetWidth()+10-m_rules_grid.GetGrid().GetBoundingRectX();

							for (int iclmn_obsc=iclmn+1; iclmn_obsc <= m_rules_grid.NumSymbolColumns(); ++iclmn_obsc)
							{
								if (m_rules_grid.GetGrid().GetColumnOffset(m_rules_grid.ParsSttClmn(iclmn_obsc)) > last_obscured_offs_x)
									break;

								if (isym_to_obscure_beg == -1)
								{
									// Start the new range.
									isym_to_obscure_beg = isym_to_obscure_end = iclmn_obsc;
								}
								else
								{
									// Fix existing range.
									if (iclmn_obsc < isym_to_obscure_beg)
										isym_to_obscure_beg = iclmn_obsc;
									if (iclmn_obsc > isym_to_obscure_end)
										isym_to_obscure_end = iclmn_obsc;
								}
							}
						}
					}
				}
			}
		}
	}

	// Setting up the links requires rebuilding rects in the main grid.
	m_rules_grid.GetGrid().RebuildRects();

	// Setup data set in the screen items view.
	m_view_helper.ClearInternalDataSet();
	m_view_helper.AddToInternalDataSet(m_rules_grid.GetGrid());
	m_view_helper.ShowInternalDataSet(TRUE);

	// Close processing status panel.
	m_local_console.CloseStatusPanel();
}

void TLangViewRulesGridDialog::PrepareAndShowCurrentTree(int *min_radius_steps_info, TLangViewTreeNode *conflicting_symbol)
{
	// Update the status panel.
	m_local_console.SetMajorStatus(L"Positioning tree...");

	// Remove the old data set if any.
	m_view_helper.InternalDataSet().Clear();
	m_view_helper.ShowInternalDataSet(TRUE);

	// Setup the view mode specific zoom level.
	m_view_helper.SetZoomLevel(m_zoom_level[m_dialog_mode]);

	// Place the tree without displaying it.
	TTreeItemsPlacingMode placing_mode = m_tree_placing_mode[m_dialog_mode];
	void *contexts = &m_lex_items_style;
	switch (placing_mode)
	{
		case plc_mode_concentric:
				{
					TConcentricModePlacingProps concentric_props = TLangViewLexItemStyle::g_DefaultConcentricPlacingProps;
					concentric_props.min_radius_steps = min_radius_steps_info;
					TConcentricModeTreeGraphPlacer::DisplayGraph(m_view_helper, m_view_helper.InternalDataSet(), &m_tree_items_root,
													concentric_props, &contexts, 0, NULL, FALSE, &m_local_console);
				}
				break;

		case plc_mode_rect_lines:
				{
					TRectLinesModeTreeGraphPlacer::DisplayGraph(m_view_helper, m_view_helper.InternalDataSet(), &m_tree_items_root,
													TLangViewLexItemStyle::g_DefaultRectLinesPlacingProps, &contexts, 0, NULL, FALSE,
													&m_local_console);
				}
				break;

		default:
			assert(FALSE);
			break;
	}

	// Do the post processing if needed.
	if (m_dialog_mode == grdm_grms_overview)
	{
		// Hide the center button object. This will also exclude it from evaluating the bounding rect.
		m_tree_items_root.ResetScreenItems(m_view_helper.InternalDataSet());

		if (placing_mode == plc_mode_concentric)
		{
			// Shift the lex item of the root grammar in the direction to the tree root.
			TLangViewSpecialTreeNode *root_grm_obj = (TLangViewSpecialTreeNode*)m_tree_items_root.GetFirstChild(FALSE);
			if (root_grm_obj != NULL)
			{
				POINT root_pos, root_grm_pos_initial;
				m_tree_items_root.GetPosition(root_pos.x, root_pos.y);
				root_grm_obj->GetPosition(root_grm_pos_initial.x, root_grm_pos_initial.y);

				POINT root_grm_pos;
				root_grm_obj->GetPosition(root_grm_pos.x, root_grm_pos.y);
				TGeometryHelper::ShiftPoint(root_grm_pos, root_pos, 0.5);

				root_grm_obj->SetPosition(root_grm_pos.x, root_grm_pos.y);
				root_grm_obj->Item().ShiftItem(root_grm_pos.x-root_grm_pos_initial.x, root_grm_pos.y-root_grm_pos_initial.y);

				HDC hDC = ::GetDC(m_hDlg);
				TGenericTreeNode *chld = root_grm_obj->GetFirstChild(FALSE);
				while (chld != NULL)
				{
					chld->PrepareConcentricModeUpperLinkItem(hDC, chld->GetTreeLinkStyle(), &m_lex_items_style);
					chld = chld->GetNextSibling(FALSE);
				}

				::ReleaseDC(m_hDlg, hDC);
			}
		}
	}
	else if (m_dialog_mode == grdm_non_term_depends)
	{
		// Highlight foreign non terminals on the main tree.
		WORD non_term_sect = m_grammar->non_terminals[m_single_obj_index-NonTerminalsBase].rules_sect;
		CreateSymbolsTreeSelections(m_tree_items_root, list_frin_nterm, non_term_sect);

		if (placing_mode == plc_mode_rect_lines)
		{
			// Position the second tree.
			TList second_tree_src_items;
			TRectLinesModePlacingProps aux_tree_props = TLangViewLexItemStyle::g_DefaultRectLinesPlacingProps;
			aux_tree_props.up_side_down_placing = TRUE;
			TRectLinesModeTreeGraphPlacer::DisplayGraph(m_view_helper, second_tree_src_items, &m_aux_upper_tree_root, aux_tree_props, &contexts, 0, NULL, FALSE, &m_local_console);

			// Shift the upper tree to place roots of both trees on the same place.
			long sec_tree_shift_x = m_tree_items_root.GetPrimaryBkgrItem()->GetBoundingRectX()-m_aux_upper_tree_root.GetPrimaryBkgrItem()->GetBoundingRectX();
			long sec_tree_shift_y = m_tree_items_root.GetPrimaryBkgrItem()->GetBoundingRectY()-m_aux_upper_tree_root.GetPrimaryBkgrItem()->GetBoundingRectY();
			for (TListIter<TScreenItem> iter(second_tree_src_items); iter; ++iter)
				iter.CurrItem().ShiftItem(sec_tree_shift_x, sec_tree_shift_y);

			// Add screen items of the second tree to the painting list.
			m_view_helper.InternalDataSet().PrependList(second_tree_src_items);

			// Highlight foreign non terminals on the second tree.
			CreateSymbolsTreeSelections(m_aux_upper_tree_root, list_frin_nterm, non_term_sect);
		}
	}
	else if (m_dialog_mode == grdm_deriv_path_ast)
	{
		if (conflicting_symbol != NULL)
			MapDerivationPathAstSpashIcon(conflicting_symbol, placing_mode);

		if (placing_mode == plc_mode_rect_lines)
			RemapDerivationPathAstRuleLabels(TLangViewLexItemStyle::g_DefaultRectLinesPlacingProps);
	}

	// Show the new contents of the screen items list.
	m_view_helper.ShowInternalDataSet(FALSE);

	if (m_dialog_mode != grdm_grms_overview)
		m_view_helper.ScrollDataSetIntoCenter();

	// Processing is complete.
	m_local_console.CloseStatusPanel();
}

void TLangViewRulesGridDialog::MapDerivationPathAstSpashIcon(TLangViewTreeNode *conflicting_symbol, TTreeItemsPlacingMode placing_mode)
{
	//
	// Current view mode is the derivation path AST and the conflicting symbol is present. Highlight this symbol
	// with the splash like icon that should be placed at the point where the incoming link meets the object border.
	//

	// Look for the node that points into the conflicting symbol and pick up its link.
	TLangViewTreeNode *arrow_node_from = (TLangViewTreeNode*)(conflicting_symbol->GetPrevSibling(TRUE));
	TLinesChainItem *arrow_link_item = NULL;
	long extra_icon_offs_y = 0;
	if (arrow_node_from != NULL && arrow_node_from->GetRightLinkSlct() == TRUE)
	{
		// Right link of the left sibling is highlighted. This means that left sibling is the right object.
		arrow_link_item = arrow_node_from->GetRightLinkItem();
	}
	else
	{
		// Pick up the object above the conflicting symbol.
		arrow_node_from = conflicting_symbol->GetOwner();
		if (arrow_node_from != NULL && conflicting_symbol->GetUpperLinkSlct() == TRUE)
		{
			// Upper link of the conflicting symbol is highlighted. The upper object is the right object.
			arrow_link_item = conflicting_symbol->GetUpperLinkItem();
			extra_icon_offs_y = -1;
		}
	}

	if (arrow_link_item != NULL)
	{
		TPoint pos_icon;
		bool pos_prepared = FALSE;
		if (placing_mode == plc_mode_concentric)
		{
			// Proper predecessor node is present. Pick up its interger positions.
			POINT node_from_pos, cfct_symbol_pos;
			arrow_node_from->GetPosition(node_from_pos.x, node_from_pos.y);
			conflicting_symbol->GetPosition(cfct_symbol_pos.x, cfct_symbol_pos.y);

			// Prepare position on the main line between the nodes.
			TPoint node_from_point(node_from_pos);
			pos_icon.SetPos(cfct_symbol_pos.x, cfct_symbol_pos.y);
			pos_icon.ShiftAbsolute(node_from_point, conflicting_symbol->GetDiameter()/2);
			pos_prepared = TRUE;
		}
		else if (placing_mode == plc_mode_rect_lines)
		{
			// Pick up the coordinates of the end of the link.
			int jct_inx = arrow_link_item->NumJctPoints()-1;
			pos_icon.SetPos(arrow_link_item->JctPosX(jct_inx), arrow_link_item->JctPosY(jct_inx)+extra_icon_offs_y);
			pos_prepared = TRUE;
		}

		if (pos_prepared == TRUE)
		{
			// Map two bitmap items. The first before the link and the second and after the link item.
			m_cfct_sym_highlight1.SetStyle(m_lex_items_style.GetAppIconsStyle());
			m_cfct_sym_highlight1.SetPosition(pos_icon.X(), pos_icon.Y(), 304);
			m_view_helper.AddToInternalDataSetBeforeItem(m_cfct_sym_highlight1, arrow_link_item);
			m_cfct_sym_highlight2.SetStyle(m_lex_items_style.GetAppIconsStyle());
			m_cfct_sym_highlight2.SetPosition(pos_icon.X(), pos_icon.Y(), 305);
			m_view_helper.AddToInternalDataSetAfterItem(m_cfct_sym_highlight2, arrow_link_item);
		}
	}
}

void TLangViewRulesGridDialog::RemapDerivationPathAstRuleLabels(TRectLinesModePlacingProps &rect_lines_props)
{
	HDC hDC = ::GetDC(m_hDlg);
	if (hDC == NULL)
		return;

	// Replace rect rule label objects with the circular rule labels.
	TGenericTreeNodesIterator iter(&m_tree_items_root);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TLangViewTreeNode *node = (TLangViewTreeNode*)iter.GetCurrentItem();
		if (node->GetItemType() != litp_special_lex_item || ((TLangViewSpecialTreeNode*)node)->Item().GetSpecType() != slit_rule_label)
			continue;

		// Shift the hot spot into the center of the rect because the hot spot of the circle is in its center.
		int size_x = node->GetRectLinesModePlacingParam(hDC, rcln_size_x, rect_lines_props, &m_lex_items_style);
		int size_y = node->GetRectLinesModePlacingParam(hDC, rcln_size_y, rect_lines_props, &m_lex_items_style);
		node->ShiftSubtree(size_x/2, size_y/2);

		// Reset screen items and prepare them once again in the concentric mode. There is a trick here because
		// position of the right link will be changed while it will continue to be part of the painting list.
		node->ResetScreenItems(m_view_helper.InternalDataSet(), FALSE);
		node->PrepareForPlacing(hDC, plc_mode_concentric, FALSE, &m_lex_items_style);
		node->PrepareConcentricModeScreenItems(hDC, &m_lex_items_style);

		TLangViewTreeNode *next_sibl = (TLangViewTreeNode*)node->GetNextSibling(FALSE);
		if (next_sibl != NULL)
		{
			// Pick up the current position of the right link.
			TLinesChainItem1 *rlink = (TLinesChainItem1*)node->GetRightLinkItem();
			POINT right_link_data[2] = { rlink->JctPosX(0), rlink->JctPosY(0), rlink->JctPosX(1), rlink->JctPosY(1) };

			if (next_sibl->GetNextSibling(FALSE) != NULL)
			{
				// Use vertical position of the right link of the right sibling.
				TLinesChainItem *next_sibl_rlink = next_sibl->GetRightLinkItem();
				right_link_data[0].y = next_sibl_rlink->JctPosY(0);
				right_link_data[1].y = next_sibl_rlink->JctPosY(1);
			}
			else
			{
				// Use horz link on the layer of the center of the circle.
				right_link_data[1].y = right_link_data[0].y;
			}

			rlink->SetPosition(2, right_link_data);
		}

		// Add screen items back. They will be at the end of the list now.
		node->ShowBkgrScreenItems(plc_mode_concentric, m_view_helper.InternalDataSet());
		node->ShowFrgrScreenItems(plc_mode_concentric, m_view_helper.InternalDataSet());
	}

	::ReleaseDC(m_hDlg, hDC);
	if (iter.CheckIterationSuccess() == FALSE)
		MsgBox(L"Some of the rule labels were not iterated.");
}

void TLangViewRulesGridDialog::PrepareAndShowNoObjectsMessage(const wchar_t *message, TTextStyle *style)
{
	// Prepare the screen item.
	m_no_objects_message.SetStyle((style != NULL) ? style : &m_no_objects_text_style);
	m_no_objects_message.SetContents(message);
	m_no_objects_message.UpdateSize(m_hDlg);

	// Setup data set in the screen items view.
	m_view_helper.ClearInternalDataSet();
	m_view_helper.AddToInternalDataSet(m_no_objects_message);
	m_view_helper.ShowInternalDataSet(TRUE);

	// Processing is complete.
	m_local_console.CloseStatusPanel();
}

void TLangViewRulesGridDialog::PrepareAndShowGrammarNotInFullSucceesMessage()
{
	assert(m_grammar != NULL);
	switch (m_grammar->grm_props.processing_result)
	{
		case grpr_none:				PrepareAndShowNoObjectsMessage(L"The grammar is empty.");
									break;
		case grpr_syntax_errors:		PrepareAndShowNoObjectsMessage(L"Grammar contains syntax errors.");
									break;
		case grpr_conversion_errors:	PrepareAndShowNoObjectsMessage(L"Grammar contains the conversion errors.");
									break;
		case grpr_full_success:			PrepareAndShowNoObjectsMessage(L"The grammar is in a full success state.");
									break;
		default:
			PrepareAndShowNoObjectsMessage(L"The m_grammar->grm_props.processing_result is bogus.");
			break;
	}
}

//
//  ===========================  Section 8  ==============================
//

TTextItem80 *TLangViewRulesGridDialog::CreateTextScreenItem(const wchar_t *descr, TTextStyle *text_style)
{
	TTextItem80 *scr_item = new TTextItem80(text_style);
	if (scr_item == NULL)
		MsgBoxFmt(MB_OK, L"Text label (%s) creation failed.", (descr != NULL) ? descr : L"<NoDescr>");
	return(scr_item);
}

TGrammarViewLabelItem *TLangViewRulesGridDialog::CreateSpecLabelItem(const wchar_t *descr, TGrammarViewLabelItem::TLabelType gvlt, WORD app_value)
{
	// Select appropriate style. Style for the action symbol labels will be changed later.
	TTextStyle *style = &m_row_descr_text_style;
	if (gvlt == TGrammarViewLabelItem::gvlt_parsing_state)
		style = &m_pars_stt_label_text_style;
	else if (gvlt == TGrammarViewLabelItem::gvlt_xpct_conflict)
		style = &m_sect_title_text_style;
	else if (gvlt == TGrammarViewLabelItem::gvlt_parsing_action)
		style = &m_action_present_text_style;

	TGrammarViewLabelItem *scr_item = new TGrammarViewLabelItem(style);
	if (scr_item == NULL)
	{
		MsgBoxFmt(MB_OK, L"Grammar view label (%s) creation failed.", (descr != NULL) ? descr : L"<NoDescr>");
		return(NULL);
	}

	// Setup the object.
	if (gvlt == TGrammarViewLabelItem::gvlt_grammar_symbol)
	{
		scr_item->SetupGrammarSymbolObject(m_grammar, m_lex_items_style, app_value, TRUE);
	}
	else if (gvlt == TGrammarViewLabelItem::gvlt_cfct_inp_symbol)
	{
		scr_item->SetupGrammarSymbolObject(m_grammar, m_lex_items_style, app_value, FALSE);
	}
	else if (gvlt == TGrammarViewLabelItem::gvlt_xpct_conflict)
	{
		scr_item->SetupXpctConflictObject(m_grammar, app_value);
	}
	else
	{
		scr_item->SetupObject(gvlt, app_value);
	}

	scr_item->UpdateSize(m_hDlg);
	scr_item->SetOwner(&m_grm_view_labels_owner);
	return(scr_item);
}

TLangViewLexItem *TLangViewRulesGridDialog::CreateSymbolLexItem(WORD sym, short &off_bottom)
{
	// Create the screen item.
	TLangViewLexItem *screen_item = NULL;
	if (sym < NonTerminalsBase)
	{
		// Passed symbol is a terminal symbol.
		assert(sym < m_grammar->NumTerminals());
		TTerminalSymbol &sym_info = m_grammar->symbols[sym];
		screen_item = new TLangViewLexemaLexItem(sym, sym_info.lex_type, sym_info.check_subt, sym_info.lex_subt.subtype);
	}
	else
	{
		// Passed symbol is a non terminal.
		assert(sym < NonTerminalsBase+m_grammar->NumNonTerminals());
		screen_item = new TLangViewNonTerminalLexItem(sym);
	}

	if (screen_item == NULL)
	{
		wchar_t buff40[40];
		MsgBoxFmt(MB_OK, L"Creation of the screen item for grammar symbol %s (%hu) failed.", m_grammar->GetSymbolName(sym, buff40), sym);
		return(NULL);
	}

	// Prepare the screen item.
	screen_item->SetStyle(&m_lex_items_style);
	screen_item->SetOwner(&m_frgr_lex_items_owner);

	HDC hDC = ::GetDC(m_hDlg);
	if (hDC == NULL)
	{
		MsgBox(L"Failure getting screen DC for showing grammar symbol item.");
		return(NULL);
	}

	if (sym < NonTerminalsBase)
	{
		// Terminal symbols should appear as circles.
		screen_item->PrepareForConcentricMode(hDC, 0, 0);
	}
	else
	{
		// Non terminals should appear as rects.
		screen_item->PrepareForRectLinesMode(hDC, 0, 0);
	}

	// Give out the recommended vertical offset.
	off_bottom = (sym < NonTerminalsBase) ? -1 : 4;
	::ReleaseDC(m_hDlg, hDC);
	return(screen_item);
}

TScreenItemsGridItem *TLangViewRulesGridDialog::CreateInnerGrid(int num_rows, int num_columns, long cell_x, long cell_y, TBasicStyle *bkgr_style)
{
	// Allocate the grid object.
	TScreenItemsGridItem *inner_grid = new TScreenItemsGridItem(bkgr_style);
	if (inner_grid == NULL)
	{
		MsgBoxFmt(MB_OK, L"Error allocating inner grid (%d rows x %d columns).", num_rows, num_columns);
		return(NULL);
	}

	// Inner grid should be not cliclable and self owneable. This is necessary to allow the grid class to return
	// its cell items when the territory of the grid is clicked.
	inner_grid->SetClickableBkgrProp(FALSE);
	inner_grid->SetOwner(inner_grid);

	// Add columns to the grid.
	TScreenItemsGridColumnInfo clmn_pros;
	clmn_pros.Setup(cell_x);
	for (int ic1=0; ic1<num_columns; ++ic1)
	{
		if (inner_grid->AddColumn(SCRITEMS_GRID_APPEND, clmn_pros) == FALSE)
		{
			MsgBoxFmt(MB_OK, L"Error adding column to the inner grid (%d rows x %d columns).", num_rows, num_columns);
			delete inner_grid;
			return(NULL);
		}
	}

	// Add rows to the grid.
	TScreenItemsGridRowInfo row_pros;
	row_pros.Setup(cell_y);
	for (int ir1=0; ir1<num_rows; ++ir1)
	{
		if (inner_grid->AddRow(SCRITEMS_GRID_APPEND, row_pros) == FALSE)
		{
			MsgBoxFmt(MB_OK, L"Error adding row to the inner grid (%d rows x %d columns).", num_rows, num_columns);
			delete inner_grid;
			return(NULL);
		}
	}

	// Success.
	return(inner_grid);
}

bool TLangViewRulesGridDialog::CreateCenteredTextLabel(const wchar_t *descr, int physical_row_inx, int physical_clmn_inx, short off_bottom, TTextStyle *text_style, const wchar_t *format, ...)
{
	// Create screen item for displaying the label.
	TTextItem80 *screen_item = CreateTextScreenItem(descr, text_style);
	if (screen_item == NULL)
		return(FALSE);

	// Setup the label contents.
	va_list vargs;
	va_start(vargs, format);
	screen_item->SetTextFmtVargs(format, vargs);
	va_end(vargs);
	screen_item->UpdateSize(m_hDlg);

	// Place label into the grid.
	m_rules_grid.SetupCenteredCell(this, descr, physical_row_inx, physical_clmn_inx, screen_item, off_bottom);
	return(TRUE);
}

bool TLangViewRulesGridDialog::CreateLeftAlignedTextLabel(const wchar_t *descr, int physical_row_inx, int physical_clmn_inx, short horz_shift, TTextStyle *text_style, const wchar_t *format, ...)
{
	// Create screen item for displaying the label.
	TTextItem80 *screen_item = CreateTextScreenItem(descr, text_style);
	if (screen_item == NULL)
		return(FALSE);

	// Setup the label contents.
	va_list vargs;
	va_start(vargs, format);
	screen_item->SetTextFmtVargs(format, vargs);
	va_end(vargs);
	screen_item->UpdateSize(m_hDlg);

	// Place label into the grid.
	m_rules_grid.SetupLeftAlignedCellWithRowBkgr(this, descr, physical_row_inx, physical_clmn_inx, screen_item, 0, horz_shift);
	return(TRUE);
}

//
//  ===========================  Section 9  ==============================
//

void TLangViewRulesGridDialog::AddTermNonTermSymbolsTableColumn(int clmn_width, TBasicStyle *delim_style)
{
	TScreenItemsGridColumnInfo clmn_info;
	clmn_info.Setup(clmn_width);
	if (delim_style != NULL)
		clmn_info.SetupDelim(1, delim_style);
	m_symbols_table_grid.AddColumn(SCRITEMS_GRID_APPEND, clmn_info);
}

int TLangViewRulesGridDialog::AddTermNonTermSymbolsTableRow(const wchar_t *descr, TSymbolsTableColumnMeaning row_meaning, int row_height, TBasicStyle *bkgr_style, TBasicStyle *delim_style, WORD main_index)
{
	TScreenItemsGridRowInfo row_info;
	row_info.Setup(row_height, bkgr_style);
	if (delim_style != NULL)
		row_info.SetupDelim(1, delim_style);

	// Setup the app data of the row.
	row_info.m_row_app_data = MAKELONG(row_meaning, main_index);

	if (m_symbols_table_grid.AddRow(SCRITEMS_GRID_APPEND, row_info) == FALSE)
	{
		MsgBoxFmt(MB_OK, L"TermNonTermSymbolsTable: Error adding \"%s\" row.", descr);
		return(-1);
	}

	// Success.
	return(m_symbols_table_grid.NumRows()-1);
}

bool TLangViewRulesGridDialog::AddTermNonTermSymbolsTableTextLabel(const wchar_t *descr, int row_inx, int clmn_inx, short horz_shift, TTextStyle *text_style, const wchar_t *format, ...)
{
	// Create screen item for displaying the label.
	TTextItem80 *text_label = CreateTextScreenItem(descr, text_style);
	if (text_label == NULL)
		return(FALSE);

	// Setup the label contents.
	va_list vargs;
	va_start(vargs, format);
	text_label->SetTextFmtVargs(format, vargs);
	va_end(vargs);
	text_label->UpdateSize(m_hDlg);

	// Place label into the grid.
	TScreenItemsGridCellData sym_table_cell_data;
	sym_table_cell_data.Clear();
	if (horz_shift >= 0)
	{
		// Use left alignment.
		sym_table_cell_data.PlaceLeftCenter(NULL, FALSE);
		sym_table_cell_data.PlaceAtLayer(0, text_label, TRUE, horz_shift);
	}
	else if (horz_shift == (short)SYMS_TABLE_CELL_CENTER)
	{
		// Caller wants centered label.
		sym_table_cell_data.PlaceInTheCenter(text_label, TRUE);
	}
	else
	{
		// Use right alignment.
		sym_table_cell_data.PlaceRightCenter(NULL, FALSE);
		sym_table_cell_data.PlaceAtLayer(0, text_label, TRUE, horz_shift);
	}

	m_symbols_table_grid.SetCellData(row_inx, clmn_inx, sym_table_cell_data);
	return(TRUE);
}

bool TLangViewRulesGridDialog::AddTerminalSymbolsTableTerminalRow(WORD term_sym, TBasicStyle *row_bkgr, bool select_frgr)
{
	// Create row for the symbol.
	int irow = AddTermNonTermSymbolsTableRow(L"TerminalSymbolInfo", SYMS_ROW_grammar_symbol, 16, row_bkgr, NULL, term_sym);
	if (irow < 0)
		return(FALSE);

	TTextStyle *frgr_style_names = (select_frgr == TRUE) ? &m_bold_index_text_style : &m_rule_index_text_style;

	// Filll in the cells of the row.
	TTerminalSymbol &info = m_grammar->symbols[term_sym-TerminalSymbolsBase];
	if (AddTermNonTermSymbolsTableTextLabel(L"TerminalSymbolValue", irow, term_sym_clmn_sym, SYMS_TABLE_CELL_CENTER, frgr_style_names, L"%d", term_sym) == FALSE)
		return(FALSE);

	wchar_t sym_name_buff[40];
	if (AddTermNonTermSymbolsTableTextLabel(L"TerminalSymbolName", irow, term_sym_clmn_name, 8, frgr_style_names, L"%s", m_grammar->GetSymbolName(term_sym, sym_name_buff)) == FALSE)
		return(FALSE);

	if (info.app_id != 0)
	{
		if (AddTermNonTermSymbolsTableTextLabel(L"TerminalSymbolAppId", irow, term_sym_clmn_app_id, 12, frgr_style_names, L"%hu", info.app_id) == FALSE)
			return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TLangViewRulesGridDialog::AddNonTermSymbolsTableNonTermRow(WORD non_term, TBasicStyle *row_bkgr, bool select_frgr, bool right_align_symbol_name, TAnalysisTable &first_and_follows, TAnalysisTable &non_term_non_term_starters)
{
	// Create row for the symbol.
	int row_height = (m_show_non_term_starters == FALSE) ? 16 : 18;
	int irow = AddTermNonTermSymbolsTableRow(L"NonTerminalSymbolInfo", SYMS_ROW_grammar_symbol, row_height, row_bkgr, NULL, non_term);
	if (irow < 0)
		return(FALSE);

	TTextStyle *frgr_style_names = (select_frgr == TRUE) ? &m_bold_index_text_style : &m_rule_index_text_style;
	TTextStyle *frgr_style_props = (select_frgr == TRUE) ? &m_rule_index_text_style : &m_row_descr_text_style;

	// Filll in the cells of the row.
	TNonTerminalSymbol &info = m_grammar->non_terminals[non_term-NonTerminalsBase];
	if (AddTermNonTermSymbolsTableTextLabel(L"NonTermSymbolValue", irow, non_term_clmn_sym, SYMS_TABLE_CELL_CENTER, frgr_style_names, L"%d", non_term) == FALSE)
		return(FALSE);

	short horz_offset = (right_align_symbol_name == FALSE) ? 8 : -8;
	if (AddTermNonTermSymbolsTableTextLabel(L"NonTermSymbolName", irow, non_term_clmn_name, horz_offset, frgr_style_names, L"%s", info.symbol_name) == FALSE)
		return(FALSE);

	if (info.public_symbol == TRUE)
	{
		if (AddTermNonTermSymbolsTableTextLabel(L"NonTermSymbolPublic", irow, non_term_clmn_public, SYMS_TABLE_CELL_CENTER, frgr_style_props, L"public") == FALSE)
			return(FALSE);
	}

	if (info.restr_type != ntrs_none)
	{
		if (AddTermNonTermSymbolsTableTextLabel(L"NonTermSymbolRestr", irow, non_term_clmn_restr, SYMS_TABLE_CELL_CENTER, frgr_style_props, TGrammar::MakeNonTermRestrTypeEnumName(info.restr_type)) == FALSE)
			return(FALSE);
	}

	if (info.app_id != 0)
	{
		if (AddTermNonTermSymbolsTableTextLabel(L"NonTermSymbolAppId", irow, non_term_clmn_app_id, SYMS_TABLE_CELL_CENTER, frgr_style_names, L"%hu", info.app_id) == FALSE)
			return(FALSE);
	}

	if (m_show_non_term_starters == TRUE)
	{
		assert(first_and_follows.IsInited() == TRUE && non_term_non_term_starters.IsInited() == TRUE);
		WORD *terminal_firsts = first_and_follows.GetNonTermRowPtr(non_term);
		WORD *non_term_firsts = non_term_non_term_starters.GetNonTermRowPtr(non_term);

		// Figure out the number of symbols that should be displayed.
		int cnt_starters = 0;

		for (int iterm1=0; iterm1<m_grammar->NumTerminals(); ++iterm1)
			if ((terminal_firsts[iterm1] & FUN_FIRST_MASK) != 0)
				cnt_starters++;

		for (int inont1=0; inont1<m_grammar->NumNonTerminals(); ++inont1)
			if ((non_term_firsts[inont1] & FUN_FIRST_MASK) != 0)
				cnt_starters++;

		// Allocate grid object for displaying the starter symbols.
		TScreenItemsGridItem *inner_grid = CreateInnerGrid(1, cnt_starters, 8, action_symbols_grid_cell_size_y);
		if (inner_grid == NULL)
			return(FALSE);

		// Fill in the cells of the grid.
		TScreenItemsGridColumnInfo clmn_info;
		TScreenItemsGridCellData cell_data;
		int iclmn = 0;

		for (int iterm2=0; iterm2<m_grammar->NumTerminals(); ++iterm2)
		{
			if ((terminal_firsts[iterm2] & FUN_FIRST_MASK) != 0)
			{
				TGrammarViewLabelItem *scr_item1 = CreateSpecLabelItem(L"StarterSym1", TGrammarViewLabelItem::gvlt_grammar_symbol, TerminalSymbolsBase+iterm2);
				if (scr_item1 == NULL)
					break;

				cell_data.Clear();
				cell_data.PlaceInTheCenter(scr_item1, TRUE);
				clmn_info.Setup(scr_item1->GetWidth()+4);
				inner_grid->UpdateColumnInfo(iclmn, clmn_info);
				inner_grid->SetCellData(0, iclmn++, cell_data);
			}
		}

		for (int inont2=0; inont2<m_grammar->NumNonTerminals(); ++inont2)
		{
			if ((non_term_firsts[inont2] & FUN_FIRST_MASK) != 0)
			{
				TGrammarViewLabelItem *scr_item2 = CreateSpecLabelItem(L"StarterSym2", TGrammarViewLabelItem::gvlt_grammar_symbol, NonTerminalsBase+inont2);
				if (scr_item2 == NULL)
					break;

				cell_data.Clear();
				cell_data.PlaceInTheCenter(scr_item2, TRUE);
				clmn_info.Setup(scr_item2->GetWidth()+4);
				inner_grid->UpdateColumnInfo(iclmn, clmn_info);
				inner_grid->SetCellData(0, iclmn++, cell_data);
			}
		}

		// Finalize the inner grid and attach it into the big row.
		TScreenItemsGridCellData sym_table_cell_data;
		inner_grid->UpdateGrid();
		sym_table_cell_data.Clear();
		sym_table_cell_data.PlaceLeftCenter(inner_grid, TRUE, 32);
		m_symbols_table_grid.SetCellData(irow, non_term_clmn_starters, sym_table_cell_data);
	}

	// Success.
	return(TRUE);
}

void TLangViewRulesGridDialog::CreateSymbolsTreeSelections(TLangViewSpecialTreeNode &root, TLangViewLexItemSelectionType selt, WORD slct_value)
{
	// Iterate objects in the tree. The root of the tree is not checked.
	TGenericTreeNodesIterator iter(&root);
	for (iter.StartIteration(); iter.CheckIteration(); iter.StepIteration())
	{
		TLangViewTreeNode *item = (TLangViewTreeNode*)iter.GetCurrentItem();
		WORD sym = item->GetSymbolValue();
		bool non_term_sym = (sym >= NonTerminalsBase && sym < NonTerminalsBase+m_grammar->non_terminals.NumItems());

		if (selt == list_bold_blue && sym == slct_value || selt == list_frin_nterm && non_term_sym == TRUE && m_grammar->non_terminals[sym-NonTerminalsBase].rules_sect != slct_value)
		{
			// Current node should be selected using the passed selection type.
			TScreenItem *sel_obj = item->CreateSelectionScreenItem(selt);
			if (sel_obj == NULL)
			{
				MsgBox(L"Out of memory while allocating selection object.");
				return;
			}
			else if (m_tree_view_extra_items.AppendItem(sel_obj) == FALSE)
			{
				MsgBox(L"Out of memory while storing selection object.");
				delete sel_obj;
				break;
			}

			// Allocation and storing succeeded. Setup the item.
			sel_obj->SetOwner(NULL, selt);
			m_view_helper.InternalDataSet().PrependItem(sel_obj);
			m_view_helper.InvalidateScreenItemRect(sel_obj);
		}
	}

	if (iter.CheckIterationSuccess() == FALSE)
		MsgBox(L"Some of the tree noded were not iterated.");
}

bool TLangViewRulesGridDialog::ReleaseSymbolsTreeSelections(TLangViewLexItemSelectionType selt)
{
	// This method expects that all members of extra list are part of the painting list. Note that it is
	// the responsibility of the caller to update the child viewer.
	bool smth_released = FALSE;
	for (int islct=0; islct<m_tree_view_extra_items.NumItems(); ++islct)
	{
		TScreenItem *slct_item = m_tree_view_extra_items[islct];
		if (slct_item->GetOwnerData() == selt)
		{
			m_view_helper.InvalidateScreenItemRect(slct_item);
			m_view_helper.InternalDataSet().RemoveItem(slct_item);
			m_tree_view_extra_items.ReleaseItem(islct--);
			delete slct_item;
			smth_released = TRUE;
		}
	}

	return(smth_released);
}

void TLangViewRulesGridDialog::LoadStylesProfile(bool prepare_styles)
{
	bool new_profile_flag = FALSE;
	TScreenItemStylePropsCollection profile_props;
	if (profile_props.LoadFromExeDirectory(g_StylesProfileName, &m_local_console, &g_LatestScannedProfileDate, &new_profile_flag) == TRUE)
	{
		PrepareBasicProfileStyle(profile_props, m_dark_delim_style,			L"dark_delim_style");
		PrepareBasicProfileStyle(profile_props, m_semi_light_delim_style,		L"semi_light_delim_style");
		PrepareBasicProfileStyle(profile_props, m_light_delim_style,			L"light_delim_style");
		PrepareBasicProfileStyle(profile_props, m_symbols_vert_delim_style,	L"symbols_vert_delim_style");
		PrepareBasicProfileStyle(profile_props, m_light_syms_vert_delim_style,	L"light_syms_vert_delim_style");

		PrepareBasicProfileStyle(profile_props, m_grammar_rules_hdr_style,	L"grammar_rules_hdr_style");
		PrepareBasicProfileStyle(profile_props, m_parsing_states_hdr_style,	L"parsing_states_hdr_style");

		PrepareBasicProfileStyle(profile_props, m_xpct_conflict_hdr_style,		L"xpct_conflict_hdr_style");
		PrepareBasicProfileStyle(profile_props, m_grammar_cfct_hdr_style,		L"grammar_cfct_hdr_style");
		PrepareBasicProfileStyle(profile_props, m_cfcting_action_hdr_style,		L"cfcting_action_hdr_style");
		PrepareBasicProfileStyle(profile_props, m_xpct_actions_map_hdr_style,	L"xpct_actions_map_hdr_style");

		PrepareBasicProfileStyle(profile_props, m_derivation_path_hdr_style,	L"derivation_path_hdr_style");
		PrepareBasicProfileStyle(profile_props, m_symbol_use_rprt_hdr_style,	L"symbol_use_rprt_hdr_style");

		PrepareBasicProfileStyle(profile_props, m_predec_flwr_seqs_bkgr_style, L"predec_flwr_seqs_bkgr_style");

		PrepareBasicProfileStyle(profile_props, m_non_term_clmn_style,		L"non_term_clmn_style");
		PrepareBasicProfileStyle(profile_props, m_pars_stt_align_clmn_style,	L"pars_stt_align_clmn_style");
		PrepareBasicProfileStyle(profile_props, m_conflict_align_clmn_style,	L"conflict_align_clmn_style");
		PrepareBasicProfileStyle(profile_props, m_derv_path_align_clmn_style,	L"derv_path_align_clmn_style");

		PrepareBasicProfileStyle(profile_props, m_extern_non_term_slct_style,	L"extern_non_term_slct");
		PrepareBasicProfileStyle(profile_props, m_recurs_non_term_slct_style,	L"recurs_non_term_slct");

		PrepareTextProfileStyle(profile_props, m_rule_index_text_style,		L"rule_index_text_style");
		PrepareTextProfileStyle(profile_props, m_key_rule_index_text_style,	L"key_rule_index_text_style");
		PrepareTextProfileStyle(profile_props, m_bold_index_text_style,		L"bold_index_text_style");
		PrepareTextProfileStyle(profile_props, m_sect_title_text_style,			L"sect_title_text_style");

		PrepareTextProfileStyle(profile_props, m_row_descr_text_style,		L"row_descr_text_style");
		PrepareTextProfileStyle(profile_props, m_row_comment_text_style,		L"row_comment_text_style");
		PrepareTextProfileStyle(profile_props, m_analysis_warning_text_style,	L"analysis_warning_text_style");
		PrepareTextProfileStyle(profile_props, m_analysis_error_text_style,		L"analysis_error_text_style");
		PrepareTextProfileStyle(profile_props, m_action_missing_text_style,	L"action_missing_text_style");
		PrepareTextProfileStyle(profile_props, m_action_present_text_style,	L"action_present_text_style");
		PrepareTextProfileStyle(profile_props, m_small_comment_text_style,	L"small_comment_text_style");

		PrepareTextProfileStyle(profile_props, m_pars_stt_label_text_style,		L"pars_stt_label_text_style");
		PrepareTextProfileStyle(profile_props, m_no_objects_text_style,		L"no_objects_text_style");

		PrepareTextProfileStyle(profile_props, m_clipboard_image_title_style,	L"clipboard_image_title_style");
		PrepareTextProfileStyle(profile_props, m_clipboard_image_footer_style,	L"clipboard_image_footer_style");

		PrepareLineProfileStyle(profile_props, m_pars_stt_link_style,			L"pars_stt_link_style");
		PrepareLineProfileStyle(profile_props, m_action_syms_link_style,		L"action_syms_link_style");

		if (new_profile_flag == TRUE)
			profile_props.ReportUnusedStyles(&m_local_console, g_StylesProfileName);

		if (prepare_styles == TRUE)
		{
			HDC hDC = ::GetDC(m_hDlg);
			if (hDC != NULL)
			{
				PrepareStylesForDeviceContext(hDC);
				::ReleaseDC(m_hDlg, hDC);
			}
		}
	}
}

void TLangViewRulesGridDialog::PrepareStylesForDeviceContext(HDC hDC)
{
	m_rule_index_text_style.SetupForDeviceContext(hDC);
	m_key_rule_index_text_style.SetupForDeviceContext(hDC);
	m_bold_index_text_style.SetupForDeviceContext(hDC);
	m_sect_title_text_style.SetupForDeviceContext(hDC);

	m_row_descr_text_style.SetupForDeviceContext(hDC);
	m_row_comment_text_style.SetupForDeviceContext(hDC);
	m_analysis_warning_text_style.SetupForDeviceContext(hDC);
	m_analysis_error_text_style.SetupForDeviceContext(hDC);
	m_action_missing_text_style.SetupForDeviceContext(hDC);
	m_action_present_text_style.SetupForDeviceContext(hDC);
	m_small_comment_text_style.SetupForDeviceContext(hDC);

	m_pars_stt_label_text_style.SetupForDeviceContext(hDC);
	m_no_objects_text_style.SetupForDeviceContext(hDC);

	m_clipboard_image_title_style.SetupForDeviceContext(hDC);
	m_clipboard_image_footer_style.SetupForDeviceContext(hDC);
}

void TLangViewRulesGridDialog::PrepareBasicProfileStyle(TScreenItemStylePropsCollection &profile_props, TBasicStyle &style, const wchar_t *style_name)
{
	TBasicStyleProps props;
	if (profile_props.PrepareBasicProps(props, style_name, NULL, g_StylesProfileName) == TRUE)
		style.Setup(props);
}

void TLangViewRulesGridDialog::PrepareTextProfileStyle(TScreenItemStylePropsCollection &profile_props, TTextStyle &style, const wchar_t *style_name)
{
	TTextStyleProps props;
	if (profile_props.PrepareTextProps(props, style_name, NULL, g_StylesProfileName) == TRUE)
		style.Setup(props);
}

void TLangViewRulesGridDialog::PrepareLineProfileStyle(TScreenItemStylePropsCollection &profile_props, TLineStyle &style, const wchar_t *style_name)
{
	TLineStyleProps props;
	if (profile_props.PrepareLineProps(props, style_name, NULL, g_StylesProfileName) == TRUE)
		style.Setup(props);
}

// --------------------------------------------------------------------------------
//    ===================  TLangViewGrammarBrowser  =====================
// --------------------------------------------------------------------------------

static wchar_t *g_OpenAnalysisResults_FilterSpec = L"Conflicts Analysis Results (*.XML)\0*.xml\0All Files (*.*)\0*.*\0";
static wchar_t *g_OpenAnalysisResults_DialogTitle = L" Open Analysis Results Database ";

TLangViewGrammarBrowser::TLangViewGrammarBrowser(TLangViewBrowserFrame *parent_handler, TMidLevStoreReader *db_link)
									: TLangViewBrowserFrame(parent_handler, db_link, IDD_LANGV_GRMV_MAIN_FRAME),
									m_ofn_anres(NULL, g_OpenAnalysisResults_FilterSpec, L"XML", g_OpenAnalysisResults_DialogTitle),
									m_global_horz_dock(NULL, 0, IDC_GRMBRW_SHOW_HIDE_EVENT),
									m_symbols_vert_dock(NULL, 40, IDC_GRMBRW_SHOW_HIDE_EVENT),
									m_main_vert_dock(NULL, 160, IDC_GRMBRW_SHOW_HIDE_EVENT),
									m_upper_horz_dock(NULL, 40, IDC_GRMBRW_SHOW_HIDE_EVENT),
									m_lower_horz_dock(NULL, 40, IDC_GRMBRW_SHOW_HIDE_EVENT),
									m_lower_vert_dock(NULL, 80, IDC_GRMBRW_SHOW_HIDE_EVENT),
									m_summary(db_link), m_terminal_symbols(db_link), m_non_terminals(db_link),
									m_level1_view(db_link),  m_level2_view(db_link), m_level3_view(db_link), m_level4_view(db_link), m_level5_view(db_link),
									m_local_grammar_parsing_log(NULL, TRUE)
{
	// Splitters and docks.
	m_global_horz_dock.SetDialogParams(this);
	m_symbols_vert_dock.SetDialogParams(this);
	m_main_vert_dock.SetDialogParams(this);
	m_upper_horz_dock.SetDialogParams(this);
	m_lower_horz_dock.SetDialogParams(this);
	m_lower_vert_dock.SetDialogParams(this);

	//
	// Child browsers.
	//

	m_summary.SetDialogParams(this);
	m_summary.SetOwner(this);
	m_summary.ShowTitles(TRUE, FALSE);

	m_terminal_symbols.SetDialogParams(this);
	m_terminal_symbols.SetOwner(this);
	m_terminal_symbols.ShowTitles(TRUE, FALSE);

	m_non_terminals.SetDialogParams(this);
	m_non_terminals.SetOwner(this);
	m_non_terminals.ShowTitles(TRUE, FALSE);

	m_level1_view.SetDialogParams(this);
	m_level1_view.SetOwner(this);
	m_level1_view.ShowTitles(TRUE, FALSE);

	m_level2_view.SetDialogParams(this);
	m_level2_view.SetOwner(this);
	m_level2_view.ShowTitles(TRUE, FALSE);

	m_level3_view.SetDialogParams(this);
	m_level3_view.SetOwner(this);
	m_level3_view.ShowTitles(TRUE, FALSE);

	m_level4_view.SetDialogParams(this);
	m_level4_view.SetOwner(this);
	m_level4_view.ShowTitles(TRUE, FALSE);

	m_level5_view.SetDialogParams(this);
	m_level5_view.SetOwner(this);
	m_level5_view.ShowTitles(TRUE, FALSE);

	// Clear the task.
	m_browser_grammar = NULL;

	// Setup the local database reader.
	m_local_grammar_parsing_log.AttachReader(&m_local_grammar_parsing_log_reader);
	AttachReader(&m_local_grammar_parsing_log_reader);
}

LRESULT TLangViewGrammarBrowser::OnInitDialog(HWND hDefFocusControl)
{
	// Setup objects on the dialog level.
	SetControlPos(IDCANCEL, 0, -30);

	// Create the main dock dialog.
	HWND hMainDockWnd = m_global_horz_dock.OpenNonModalDialog(m_hDlg, IDC_GRMBRW_MAIN_VERT_DOCK);
	if (hMainDockWnd == NULL)
		return(-1);

	// Place the main dock over the whole frame.
	RECT rc_client;
	::GetClientRect(m_hDlg, &rc_client);

	int upper_offs = 2;
	SetControlPos(IDC_GRMBRW_MAIN_VERT_DOCK, 0, upper_offs);
	SetControlSize(IDC_GRMBRW_MAIN_VERT_DOCK, rc_client.right, rc_client.bottom-upper_offs);
	SetupControlPositioning(IDC_GRMBRW_UPPER_GRP_BOX, cpm_fix_top_side);
	SetupControlPositioning(IDC_GRMBRW_MAIN_VERT_DOCK, cpm_fix_all_corners);

	// Fill in the global horz dock.
	if (m_global_horz_dock.ShowChildDialog(&m_symbols_vert_dock, FALSE) == FALSE)
		return(-1);
	if (m_global_horz_dock.ShowChildDialog(&m_main_vert_dock, FALSE) == FALSE)
		return(-1);

	// Fill in the symbols vert dock.
	if (m_symbols_vert_dock.ShowChildDialog(&m_terminal_symbols, FALSE, 80) == FALSE)
		return(-1);
	if (m_symbols_vert_dock.ShowChildDialog(&m_non_terminals, FALSE) == FALSE)
		return(-1);

	// Fill in the main vert dock.
	if (m_main_vert_dock.ShowChildDialog(&m_upper_horz_dock, FALSE) == FALSE)
		return(-1);
	if (m_main_vert_dock.ShowChildDialog(&m_lower_horz_dock, FALSE) == FALSE)
		return(-1);

	// Fill in the upper horz dock.
	if (m_upper_horz_dock.ShowChildDialog(&m_level1_view, FALSE) == FALSE)
		return(-1);
	if (m_upper_horz_dock.ShowChildDialog(&m_summary, FALSE) == FALSE)
		return(-1);

	// Fill in the lower horz dock.
	if (m_lower_horz_dock.ShowChildDialog(&m_level2_view, FALSE, 320) == FALSE)
		return(-1);
	if (m_lower_horz_dock.ShowChildDialog(&m_level3_view, FALSE) == FALSE)
		return(-1);
	if (m_lower_horz_dock.ShowChildDialog(&m_lower_vert_dock, FALSE) == FALSE)
		return(-1);

	// Fill in the lower vert dock.
	if (m_lower_vert_dock.ShowChildDialog(&m_level4_view, FALSE) == FALSE)
		return(-1);
	if (m_lower_vert_dock.ShowChildDialog(&m_level5_view, FALSE) == FALSE)
		return(-1);

	// Display the grammar file name if any and init browsing.
	SetupGrammar(m_browser_grammar, m_db);

	// Success.
	return(FALSE);
}

void TLangViewGrammarBrowser::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDM_GRMBRW_MENU_FILE_LOAD_DEFN:
				{
					// Scan the grammar, build the list of parsing states and detect the grammar conflicts.
					DoGrammarDefnLoading(TRUE);
				}
				break;

		case IDM_GRMBRW_MENU_FILE_LOAD_DFNR:
				{
					// Scan the grammar without building the analysis table.
					DoGrammarDefnLoading(FALSE);
				}
				break;

		case IDM_GRMBRW_MENU_FILE_LOAD_ANDB:
				{
					if (m_browser_grammar == NULL)
					{
						MsgBox(L"The grammar is not set.");
					}
					else
					{
						// The grammar is present. Ask for the file name.
						if (m_ofn_anres.GetOpenFileName(m_hDlg) == TRUE)
						{
							// Allocate reader objects for scanning the file.
							TGenericConsole cons;
							TXmlStream xmlSource(m_ofn_anres.GetCurrFileName());

							GUID dummy_guid;
							TGrammarConflictAnalysisProps dummy_props;
							TGrammarAnalysisDatabaseXmlReader readerHandler(dummy_guid, dummy_props, m_browser_grammar);

							// Scan the data.
							int errors_count = 0;
							HCURSOR old_mouse_cursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
							bool res = readerHandler.ScanXmlSource(xmlSource, &cons, &errors_count);
							if (old_mouse_cursor != NULL)
								::SetCursor(old_mouse_cursor);

							if (res == TRUE)
							{
								// The scanning succeeded.
								assert(readerHandler.CheckGrammarsMatch() == TRUE);
								CloseChildBrowsers();
								SetupGrammar(m_browser_grammar, m_db);
							}
							else if (xmlSource.CheckDataBufferPresence() == FALSE)
							{
								MsgBox(L"Error loading XML document into memory. Current grammar was not changed.");
							}
							else
							{
								assert(errors_count > 0);

								// Check if the first error was detected before scanning and comparing the grammar props
								// or after success of this action.
								if (readerHandler.CheckGrammarsMatch() == FALSE)
								{
									MsgBoxFmt(MB_OK, L"There %s %d scanning error%s. Check the console output. Current grammar was not changed.",
												(errors_count <= 1) ? L"was" :  L"were", errors_count, PluralSuffix(errors_count));
								}
								else
								{
									MsgBoxFmt(MB_OK, L"There %s %d scanning error%s. Check the console output. Current grammar may have changed.",
												(errors_count <= 1) ? L"was" :  L"were", errors_count, PluralSuffix(errors_count));

									CloseChildBrowsers();
									SetupGrammar(m_browser_grammar, m_db);
								}
							}
						}
					}
				}
				break;

		case IDCANCEL:
		case IDM_GRMBRW_MENU_FILE_EXIT:
				{
					// Plaing close the window.
					::DestroyWindow(m_hDlg);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_OPEN_EXTBR:
				{
					// New instance is needed. Note that ctor of the new frame will add itself into the list of children
					// of the current frame. This will automatically close this child frame when needed.
					TLangViewGrammarBrowser *child_inst = new TLangViewGrammarBrowser(this, m_db);
					if (child_inst == NULL)
					{
						MsgBox(L"Error allocating child browser.");
						break;
					}

					// Istance is created. Put the same grammar there and open the viewer.
					child_inst->SetupGrammar(m_browser_grammar, m_db);
					child_inst->OpenNonModalDialog(m_hDlg);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_OPEN_MIDLEV:
				{
					if (m_db == NULL)
					{
						MsgBox(L"Current grammar does not have the parsing events database.");
						break;
					}

					// Create new instance of the events viewer.
					TLangViewSimpleBrowser *mid_lev_viewer = new TLangViewSimpleBrowser(this, m_db);
					if (mid_lev_viewer == NULL)
					{
						MsgBox(L"Error allocating child parsing events viewer.");
						break;
					}

					// Setup the task and open the browser.
					mid_lev_viewer->SetupTask(0, PSET_ALL_TYPES);
					mid_lev_viewer->OpenNonModalDialog(m_hDlg);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_PROC_CFCTS:
				{
					if (m_browser_grammar == NULL)
					{
						MsgBox(L"The grammar is not set. There is nothing to process.");
						break;
					}

					// Figure out the conflicts processing props.
					TLangViewProcessGrammarProps an_props;
					an_props.SetDefaultValues();
					TLangViewEditConflictAnalysisProps edit_dialog(*m_browser_grammar, an_props, FALSE, TRUE);
					if (edit_dialog.EditAnalysisProps(m_hDlg) == TRUE)
					{
						assert(an_props.m_antask.m_only_iconflict >= -1);

						// Hide existing grammar if any to avoid painting problems. Block the menus.
						CloseChildBrowsers();
						TGrammar *curr_grammar = m_browser_grammar;
						SetupGrammar(NULL, NULL);
						SetMenuEnabledState(FALSE);

						// Prepare the analysis console.
						TGrammarAnalysisConsole analysis_console;
						analysis_console.SetDialogParams(this);
						analysis_console.SetParentWindow(m_hDlg);
						analysis_console.OpenConsole();

						// Allocate helper and set the props.
						TGrammarConflictAnalysisHelper analysis_helper(analysis_console, (gcfan_critical | gcfan_progress));
						analysis_helper.SetProps(an_props.m_anprops);

						// Do the job as requested.
						analysis_helper.ProcessConflicts(curr_grammar, an_props.m_antask.m_only_igrammar, an_props.m_antask.m_only_iconflict, an_props.m_antask.m_only_iaction);
						analysis_helper.TraceStdDelimiterLine();
						analysis_helper.TraceMsgWithDateAttr(TBasicGrammarAnalysisHelper::YellowAttr, NULL);
						analysis_helper.TraceMsgWithDateAttr(TBasicGrammarAnalysisHelper::YellowAttr, L" Sync processing complete.");
						analysis_helper.TraceMsgWithDateAttr(TBasicGrammarAnalysisHelper::YellowAttr, NULL);

						// Show the results.
						analysis_console.CloseConsole();
						SetupGrammar(curr_grammar, &m_local_grammar_parsing_log_reader);
						SetMenuEnabledState(TRUE);
					}

				}
				break;

		case IDM_GRMBRW_MENU_ACTS_EMPTRA_NTERMS:
				{
					// Empty and transparent non terminals.
					DoMiscGrammarAnalysis(&TLangViewMiscGrammarAnalysisHelper::FindEmptyAndTransparentNonTerminals);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_TRIVAL_NTERMS:
				{
					// Trivial non terminals allow construction directly from terminal symbols.
					DoMiscGrammarAnalysis(&TLangViewMiscGrammarAnalysisHelper::FindPrimitiveNonTerminals);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_NTERMS_USAGE:
				{
					// Intensity of non terminals use in the rules of other non terminals.
					DoMiscGrammarAnalysis(&TLangViewMiscGrammarAnalysisHelper::CreateNonTermsUseStatistics);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_NTERMS_DISTS:
				{
					// Statistics on the distance between non terminal definitions and use.
					DoMiscGrammarAnalysis(&TLangViewMiscGrammarAnalysisHelper::CreateNonTermDefsDistanceStatistics);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_NTERMS_CDISTS:
				{
					// Statistics on the distance between non terminal call places.
					DoMiscGrammarAnalysis(&TLangViewMiscGrammarAnalysisHelper::CreateNonTermDefsCallsDistanceStatistics);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_PATH_LENGTHS:
				{
					// Figure out what are the lengths of the existing paths.
					DoMiscGrammarAnalysis(&TLangViewMiscGrammarAnalysisHelper::CreatePathLengthsStatistics);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_CLASSIFY_CFCTS:
				{
					// Classify grammar conflicts by involved rules.
					DoMiscGrammarAnalysis(&TLangViewMiscGrammarAnalysisHelper::ClassifyAllGrammarConflicts);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_CLASSIFY_UNXPS:
				{
					// Classify only unexpected grammar conflicts by involved rules.
					DoMiscGrammarAnalysis(&TLangViewMiscGrammarAnalysisHelper::ClassifyUnexpectedConflicts);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_WINDOW_SIZE:
				{
					// Set special window size for making screen shots in consistent resolution.
					::SetWindowPos(m_hDlg, NULL, 0, 0, 1200, 820, SWP_NOMOVE | SWP_NOZORDER);
				}
				break;

		case IDM_GRMBRW_MENU_ACTS_COLOR_EDIT:
				{
					TControlHelper::ShowColorSelectDialogEx(this, L" Select color ");
				}
				break;

		case IDM_GRMBRW_MENU_HELP_ABOUT:
				{
					TLangViewAboutBox about_box(L"Grammar Structure Viewer", L"Version September 2012", COMMUNITY_EDITION);
					about_box.OpenModalDialog(m_hDlg);
				}
				break;
	}
}

void TLangViewGrammarBrowser::ProcessSingleClick(TLangViewChildBrowser *evt_src_inst, TLangViewClickEvent &event_info)
{
	TLangViewRulesGridDialog *next_layer_inst = GetNextLayerViewer((TLangViewRulesGridDialog*)evt_src_inst);
	if (next_layer_inst != NULL)
	{
		TDestViewersArray dest_viewers;
		PrepareDestViewers(dest_viewers, next_layer_inst);
		SetupDestViewers(dest_viewers, (TLangViewRulesGridDialog*)evt_src_inst, event_info);
	}
}

void TLangViewGrammarBrowser::ProcessDoubleClick(TLangViewChildBrowser *evt_src_inst, TLangViewClickEvent &event_info)
{
	if (evt_src_inst == NULL)
	{
		assert(FALSE);
		return;
	}
	else if (evt_src_inst == &m_summary || evt_src_inst == &m_terminal_symbols || evt_src_inst == &m_non_terminals)
	{
		ProcessSingleClick(evt_src_inst, event_info);
		return;
	}

	TDestViewersArray dest_viewers;
	PrepareDestViewers(dest_viewers, (TLangViewRulesGridDialog*)evt_src_inst);
	SetupDestViewers(dest_viewers, (TLangViewRulesGridDialog*)evt_src_inst, event_info);
}

void TLangViewGrammarBrowser::RemoveSelection(TLangViewChildBrowser *inst)
{
	// Remove selection on the caller.
	inst->ResetSelection();
}

void TLangViewGrammarBrowser::SetupGrammar(TGrammar *grammar, TMidLevStoreReader *db_link)
{
	// Store the pointer for future use.
	m_browser_grammar = grammar;
	SetupDataBase(db_link);

	if (m_hDlg != NULL)
	{
		// Do the real work.
		if (m_browser_grammar != NULL)
		{
			wchar_t *fname = m_browser_grammar->grm_props.grms_file_name;
			if (fname[0] == 0)
				fname = L"Empty_Grammar_File_Name";

			SetTitleTextFmt(L" Grammar Viewer - %s", fname);
			m_summary.ShowGrammarsTreeOverview(m_browser_grammar);
		}
		else
		{
			SetTitleText(L" Grammar Definition Viewer ");
			m_summary.ResetContents();
		}

		// Clear rest of the viewers.
		m_terminal_symbols.ResetContents();
		m_non_terminals.ResetContents();
		m_level1_view.ResetContents();
		m_level2_view.ResetContents();
		m_level3_view.ResetContents();
		m_level4_view.ResetContents();
		m_level5_view.ResetContents();
	}
}

void TLangViewGrammarBrowser::PromoteChildViewerContents(TLangViewRulesGridDialog *inst, bool promote_to_top)
{
	m_terminal_symbols.ResetSelection();
	m_non_terminals.ResetSelection();

	if (promote_to_top == TRUE)
	{
		// Dest level is lev1.
		m_level1_view.LoadContentsFrom(inst);
		m_level2_view.ResetContents();
		m_level3_view.ResetContents();
		m_level4_view.ResetContents();
		m_level5_view.ResetContents();
	}
	else if (inst == &m_level2_view)
	{
		// Dest level is lev1.
		m_level1_view.LoadContentsFrom(&m_level2_view);
		m_level2_view.LoadContentsFrom(&m_level3_view);
		m_level3_view.LoadContentsFrom(&m_level4_view);
		m_level4_view.LoadContentsFrom(&m_level5_view);
		m_level5_view.ResetContents();
	}
	else if (inst == &m_level3_view)
	{
		// Dest level is lev2.
		m_level2_view.LoadContentsFrom(&m_level3_view);
		m_level3_view.LoadContentsFrom(&m_level4_view);
		m_level4_view.LoadContentsFrom(&m_level5_view);
		m_level5_view.ResetContents();
	}
	else if (inst == &m_level4_view)
	{
		// Dest level is lev3.
		m_level3_view.LoadContentsFrom(&m_level4_view);
		m_level4_view.LoadContentsFrom(&m_level5_view);
		m_level5_view.ResetContents();
	}
	else if (inst == &m_level5_view)
	{
		// Dest level is lev4.
		m_level4_view.LoadContentsFrom(&m_level5_view);
		m_level5_view.ResetContents();
	}
}

void TLangViewGrammarBrowser::SetSymbolsListSelection(WORD slct_sym_frgr, TSymbolsArray *slct_syms_bkgr)
{
	// Only one viewer should be called because ownership on the pointer should
	// be passed to one viewer only.
	if (slct_sym_frgr < NonTerminalsBase)
		m_terminal_symbols.SetGrammarSymbolsListSelection(slct_sym_frgr, slct_syms_bkgr);
	else m_non_terminals.SetGrammarSymbolsListSelection(slct_sym_frgr, slct_syms_bkgr);
}

TLangViewRulesGridDialog *TLangViewGrammarBrowser::GetNextLayerViewer(TLangViewRulesGridDialog *inst)
{
	if (inst == NULL)
		return(NULL);

	if (inst == &m_summary || inst == &m_terminal_symbols || inst == &m_non_terminals)
	{
		return(&m_level1_view);
	}
	else if (inst == &m_level1_view)
	{
		return(&m_level2_view);
	}
	else if (inst == &m_level2_view)
	{
		return(&m_level3_view);
	}
	else if (inst == &m_level3_view)
	{
		return(&m_level4_view);
	}
	else if (inst == &m_level4_view)
	{
		return(&m_level5_view);
	}

	// All other viewers do not have next layer viewer.
	return(NULL);
}

void TLangViewGrammarBrowser::PrepareDestViewers(TDestViewersArray &dvs, TLangViewRulesGridDialog *inst)
{
	if (inst == &m_summary)
	{
		m_terminal_symbols.ResetContents();
		m_non_terminals.ResetContents();
	}
	else
	{
		m_terminal_symbols.ResetSelection();
		m_non_terminals.ResetSelection();
	}

	// Setup destination viewers.
	for (int inx=0; (inx < num_dest_viewers || inst != NULL); ++inx)
	{
		if (inx < num_dest_viewers)
			dvs[inx] = inst;

		inst = GetNextLayerViewer(inst);
		if (inst != NULL)
			inst->ResetContents();
	}
}

void TLangViewGrammarBrowser::SetupDestViewers(TDestViewersArray &dvs, TLangViewRulesGridDialog *inst, TLangViewClickEvent &event_info)
{
	// This browser frame should have only grammar space child browsers.
	assert(event_info.type == clet_grbr_object);
	assert(dvs[pri_viewer] != NULL);

	// All grammar space events carry the index of the grammar. Pick up the grammar with this index.
	TGrammar *grammar = m_browser_grammar->GetGrammarByIndex(event_info.grbr_grm_index);
	if (grammar == NULL)
	{
		MsgBoxFmt(MB_OK, L"Index of the grammar (%hu) in the click event data is bogus.", event_info.grbr_grm_index);
		return;
	}

	if (inst == &m_summary)
	{
		// The current grammar may change. Reload the symbols viewers.
		m_terminal_symbols.ShowTerminalSymbols(grammar);
		m_non_terminals.ShowNonTerminals(grammar);
	}

	// Do event specific type processing.
	TLangViewRulesGridDialogMode src_viewer_mode = inst->GetCurrDialogMode();
	int src_viewer_obj_index = inst->GetSingleItemModeObjIndex();
	switch (event_info.grbr_evt)
	{
		case grbr_evt_grammar:
				{
					// Show current state of the grammar conflicts analysis.
					dvs[pri_viewer]->ShowConflictsAnalysisStatus(grammar);
				}
				break;

		case grbr_evt_symbol:
				{
					// Show report on the symbol use. Select symbol in the appropriate list first. Note that if the symbol
					// is non terminal, selection will be set once again later when its dependencies will be determined.
					SetSymbolsListSelection(event_info.grbr_obj_inx, NULL);

					// Show list of rules, filtered acording to the current criteria.
					dvs[pri_viewer]->ShowGrammarRules(grammar, event_info.grbr_obj_inx);

					// If clicked symbol is non terminal show its dependencies.
					if (event_info.grbr_obj_inx >= NonTerminalsBase && dvs[sec_viewer] != NULL)
						dvs[sec_viewer]->ShowNonTerminalDependencies(grammar, event_info.grbr_obj_inx);
				}
				break;

		case grbr_evt_grammar_rule:
				{
					if (event_info.grbr_obj_inx == 0xFFFF)
					{
						// Show all rules of the grammar.
						dvs[pri_viewer]->ShowGrammarRules(grammar, -1);
					}
					else
					{
						// Show single grammar rule.
						dvs[pri_viewer]->ShowGrammarRules(grammar, actb_reduce+event_info.grbr_obj_inx);
					}
				}
				break;

		case grbr_evt_parsing_state:
				{
					// Show all parsing states or single parsing state. Target function will distinguish these cases.
					dvs[pri_viewer]->ShowParsingStates(grammar, event_info.grbr_obj_inx);
				}
				break;

		case grbr_evt_xpct_conflict:
				{
					short obj_inx = event_info.grbr_obj_inx;
					if (obj_inx < 0)
					{
						// This is displaying list of expected conflicts. Show only two primary list.
						dvs[pri_viewer]->ShowXpctConflicts(grammar, obj_inx);

						if (dvs[sec_viewer] != NULL)
							dvs[sec_viewer]->ShowKeyRulesForXpctConflicts(grammar, obj_inx);
					}
					else
					{
						// Caller asks to show only one conflict.
						if (src_viewer_mode == grdm_xpct_conflicts && src_viewer_obj_index >= 0)
						{
							// Source viewer is in single xpct conflist view mode. Show reduced number of secondary views.
							dvs[pri_viewer]->ShowKeyRulesForXpctConflicts(grammar, obj_inx);

							if (dvs[sec_viewer] != NULL)
								dvs[sec_viewer]->ShowDiffersTreeForXpctConflicts(grammar, obj_inx);
							if (dvs[tri_viewer] != NULL)
								dvs[tri_viewer]->ShowSymbolSeqsForXpctConflicts(grammar, obj_inx);
						}
						else
						{
							// Show the xpct conflict itself, key conflicting rules, path differs tree and derivation paths as symbol seqs.
							dvs[pri_viewer]->ShowXpctConflicts(grammar, obj_inx);

							if (dvs[sec_viewer] != NULL)
								dvs[sec_viewer]->ShowKeyRulesForXpctConflicts(grammar, obj_inx);
							if (dvs[tri_viewer] != NULL)
								dvs[tri_viewer]->ShowDiffersTreeForXpctConflicts(grammar, obj_inx);
							if (dvs[frt_viewer] != NULL)
								dvs[frt_viewer]->ShowSymbolSeqsForXpctConflicts(grammar, obj_inx);
						}
					}
				}
				break;

		case grbr_evt_grammar_conflict:
				{
					short obj_inx = event_info.grbr_obj_inx;
					if (obj_inx < 0)
					{
						// This is request to display all conflicts or conflicts that belong to certain category.
						dvs[pri_viewer]->ShowGrammarConflicts(grammar, obj_inx);

						if (dvs[sec_viewer] != NULL)
							dvs[sec_viewer]->ShowKeyRulesForGrammarConflicts(grammar, obj_inx);
					}
					else
					{
						// Caller asks to show only one conflict.
						if (src_viewer_mode == grdm_grammar_conflicts && src_viewer_obj_index >= 0)
						{
							// This view should display single grammar conflist.
							dvs[pri_viewer]->ShowKeyRulesForGrammarConflicts(grammar, obj_inx);

							if (dvs[sec_viewer] != NULL)
								dvs[sec_viewer]->ShowDiffersTreeForGrammarConflicts(grammar, obj_inx);
							if (dvs[tri_viewer] != NULL)
								dvs[tri_viewer]->ShowSymbolSeqsForGrammarConflicts(grammar, obj_inx);
						}
						else
						{
							// Show the conflict itself, key conflicting rules, path differs tree and derivation paths as symbol seqs.
							dvs[pri_viewer]->ShowGrammarConflicts(grammar, obj_inx);

							if (dvs[sec_viewer] != NULL)
								dvs[sec_viewer]->ShowKeyRulesForGrammarConflicts(grammar, obj_inx);
							if (dvs[tri_viewer] != NULL)
								dvs[tri_viewer]->ShowDiffersTreeForGrammarConflicts(grammar, obj_inx);
							if (dvs[frt_viewer] != NULL)
								dvs[frt_viewer]->ShowSymbolSeqsForGrammarConflicts(grammar, obj_inx);
						}
					}
				}
				break;

		case grbr_evt_conflicting_action:
				{
					// Show derivations tree and symbol sequences.
					dvs[pri_viewer]->ShowDiffersTreeForConflictingAction(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx);

					if (dvs[sec_viewer] != NULL)
						dvs[sec_viewer]->ShowSymbolSeqsForConflictingAction(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx);
				}
				break;

		case grbr_evt_derivation_path:
				{
					if (src_viewer_mode == grdm_deriv_path_ast)
					{
						// Show symbols seq and sequence of steps.
						dvs[pri_viewer]->ShowDerivationPathAsSeq(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);

						if (dvs[sec_viewer] != NULL)
							dvs[sec_viewer]->ShowDerivationPathAsSteps(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);
					}
					else if (src_viewer_mode == grdm_deriv_path_seq)
					{
						// Show AST and sequence of steps.
						dvs[pri_viewer]->ShowDerivationPathAsAst(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);

						if (dvs[sec_viewer] != NULL)
							dvs[sec_viewer]->ShowDerivationPathAsSteps(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);
					}
					else if (src_viewer_mode == grdm_deriv_path_steps)
					{
						// Show AST and symbols seq.
						dvs[pri_viewer]->ShowDerivationPathAsAst(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);

						if (dvs[sec_viewer] != NULL)
							dvs[sec_viewer]->ShowDerivationPathAsSeq(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);
					}
					else
					{
						// Show all tree path views.
						dvs[pri_viewer]->ShowDerivationPathAsAst(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);

						if (dvs[sec_viewer] != NULL)
							dvs[sec_viewer]->ShowDerivationPathAsSeq(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);
						if (dvs[tri_viewer] != NULL)
							dvs[tri_viewer]->ShowDerivationPathAsSteps(grammar, event_info.grbr_obj_inx, event_info.grbr_sub_obj_inx, event_info.grbr_path_index);
					}
				}
				break;

		default:
			assert(FALSE);
			break;
	}
}

void TLangViewGrammarBrowser::DoGrammarDefnLoading(bool build_parsing_table)
{
	// Allocate the helper that will do the work.
	TLangViewLoadGrammarHelper helper((sctr_err_warn | gdtr_steps | gdtr_reports), build_parsing_table, L"GrammarBrowser: ");
	if (helper.GetGrammarFileName(this) == TRUE)
	{
		// Hide the existing grammar if any to avoid problems in painting the child windows.
		CloseChildBrowsers();
		SetupGrammar(NULL, NULL);

		// Load the definition.
		helper.LoadGrammarDefinition(this, &m_local_grammar, &m_local_grammar_parsing_log);
		helper.ShowSessionResults(this);

		// Reset the iteration indexes because the contents of the local grammar has changed.
		m_shared_sttr_helper.SetGrammar(NULL);
		m_level1_view.ResetStateTransitionsHelper();
		m_level2_view.ResetStateTransitionsHelper();
		m_level3_view.ResetStateTransitionsHelper();
		m_level4_view.ResetStateTransitionsHelper();
		m_level5_view.ResetStateTransitionsHelper();

		// Bring the main window into focus if this is not so and show the results.
		::SetForegroundWindow(m_hDlg);
		SetupGrammar(&m_local_grammar, &m_local_grammar_parsing_log_reader);
	}
}

void TLangViewGrammarBrowser::DoMiscGrammarAnalysis(void (TLangViewMiscGrammarAnalysisHelper::*pfn)(TDestinationFile &rprt))
{
	// Note that this function needs current grammar that might differ from the root of the grammars tree.
	TGrammar *grm = m_non_terminals.GetCurrGrammar();
	if (grm == NULL)
	{
		MsgBox(L"Current grammar is not selected. Select one first.");
		return;
	}
	else if (grm->grm_props.processing_result != grpr_full_success)
	{
		MsgBoxFmt(MB_OK, L"Status of the current grammar is not good: %s.", TGrammar::GetProcResultName(grm->grm_props.processing_result));
		return;
	}

	// Some grammar is selected and it is in the file shape. Generation might take a while.
	HCURSOR old_mouse_cursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

	// Gereate report in the memory buffer.
	TDestinationFile rprt;
	rprt.PrepareInMemoryStream();
	TLangViewMiscGrammarAnalysisHelper hlpr(this, grm);
	(hlpr.*pfn)(rprt);

	if (old_mouse_cursor != NULL)
		::SetCursor(old_mouse_cursor);

	// Display the report.
	if (rprt.GetErrorFlag() == TRUE)
	{
		MsgBox(L"Low on memory while preparing the report data.");
	}
	else
	{
		DWORD res = TControlHelper::PutStringToClipboard(rprt.GetBodyPtr());
		if (res != ERROR_SUCCESS)
			MsgBoxFmt(MB_OK, L"Error placing data to clipboard: %d.", res);

		MsgBox(rprt.GetBodyPtr());
	}
}

void TLangViewGrammarBrowser::SetMenuEnabledState(bool new_value)
{
	HMENU hMenu = ::GetMenu(m_hDlg);
	if (hMenu != NULL)
	{
		// Enab/disab all three submenus.
		MENUITEMINFO mi_info;
		mi_info.cbSize = sizeof(MENUITEMINFO);
		mi_info.fMask = MIIM_STATE;
		mi_info.fState = (new_value == FALSE) ? MFS_DISABLED : 0;
		BOOL r1 = ::SetMenuItemInfo(hMenu, 0, TRUE, &mi_info);
		BOOL r2 = ::SetMenuItemInfo(hMenu, 1, TRUE, &mi_info);
		BOOL r3 = ::SetMenuItemInfo(hMenu, 2, TRUE, &mi_info);

		// Update the window.
		if (new_value == TRUE)
			::SetForegroundWindow(m_hDlg);
		::DrawMenuBar(m_hDlg);
	}
}


