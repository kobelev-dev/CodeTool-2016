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
#include  "TextScan/CppFrontEnd.H"
#include  "TextScan/CppCodeGen.H"
#include  "LangView/VisualParser.H"
#include  "LangView/LangViewResources.H"

#define	VPARS_TRACE_PREFIX   L"-Visual-Parser-: "

// -------------------------------------------------------------------------
//   ============  TLangViewVisualParserSelectDriver  ==================
// -------------------------------------------------------------------------

static const TControlPosInfo g_VisualParserSelectDriverCtrls[] =
{
	IDC_VPSLPD_GRPBOX,				cpm_fix_all_corners,
	IDC_VPSLPD_DRVLIST_TITLE,		cpm_fix_top_side,
	IDC_VPSLPD_DRVLIST_LISTBOX,		cpm_fix_all_corners,
	IDC_VPSLPD_USEAGE_NOTE,		cpm_fix_left_bottom,
	IDOK,							cpm_fix_right_bottom,
	IDCANCEL,						cpm_fix_right_bottom,
	0, cpm_none,
};

static TListBoxColumnProps g_VisualParserSelectDriverColumnsInfo[] =
{
	{ align_center,	5,	    62,	   0,		 4,	},		// Current Driver
	{ align_left,		0,	    80,	  10,		 4,	},		// Parser: LangType
	{ align_left,		0,	    62,	  20,		11,	},		// Parser: Description
	{ align_left,		0,	    76,	  10,		 4,	},		// Grammar: Status
	{ align_left,		0,	    62,	  50,		 4,	},		// Grammar: Description
	{ align_right,		0,	    82,	  10,		12,	},		// Grammar: Version
	LBX_CLMNP_EOF
};

static TListBoxColumnTilteProps g_VisualParserSelectDriverTitleInfo[] =
{
	{ 1,	 FALSE,	L"Current"				},
	{ 1,	 FALSE,	L"Parser language"			},
	{ 1,	 FALSE,	L"Parser description"		},
	{ 1,	 FALSE,	L"Grammar status"			},
	{ 1,	 FALSE,	L"Grammar description"		},
	{ 1,	 FALSE,	L"Grammar version"		},
	{ 0,	 FALSE,	NULL },	// EOF record.
};

TLangViewVisualParserSelectDriver::TLangViewVisualParserSelectDriver()
		: TBasicDialog(NULL, IDD_LANGV_VPARS_SELECT_DRIVER)
{
	m_owner = NULL;
}

LRESULT TLangViewVisualParserSelectDriver::OnInitDialog(HWND hDefFocusControl)
{
	// Prepare resizing.
	SetControlFont(IDC_VPSLPD_USEAGE_NOTE, SMALL_FONTS, 7);
	SetControlColor(IDC_VPSLPD_USEAGE_NOTE, RGB(108, 108, 108));
	SetControlBkgrBrush(IDC_VPSLPD_USEAGE_NOTE, ::GetSysColorBrush(COLOR_3DFACE), FALSE);
	SetupControlsListPositioning(g_VisualParserSelectDriverCtrls);
	SetMinTrackingSize(510, 200);

	// Setup listbox windows.
	m_list_hlp.SetWindow(::GetDlgItem(m_hDlg, IDC_VPSLPD_DRVLIST_LISTBOX));
	m_list_hlp.SetTitleWindow(::GetDlgItem(m_hDlg, IDC_VPSLPD_DRVLIST_TITLE));

	// Setup listbox styles.
	m_list_hlp.SetupSystemStdStyles(2, 2);
	m_list_hlp.SetupFrgrStyleSlot(fcslot_curr_mark, L"IMPACT", 11, TRUE, ::GetSysColor(COLOR_WINDOWTEXT), -3, 3, 0);
	m_list_hlp.SetupFrgrStyleSlot(fcslot_curr_mark_sel, L"IMPACT", 11, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT), -3, 3, 0);
	m_list_hlp.SetupFrgrStyleSlot(fcslot_no_grammar, ARIAL, 8, TRUE, RGB(160, 160, 160));
	m_list_hlp.SetupFrgrStyleSlot(fcslot_no_grammar_sel, ARIAL, 8, FALSE, RGB(240, 240, 240));

	// Setup listbox column props and column titles.
	m_list_hlp.SetupColumnProps(g_VisualParserSelectDriverColumnsInfo);
	m_list_hlp.SetupColumnTitles(g_VisualParserSelectDriverTitleInfo);

	// Fill in the listsbox.
	int num_parsers = m_owner->m_num_drivers;
	if (m_owner->m_drivers_array != NULL && num_parsers > 0)
	{
		// Parsing drivers are available in the current setup.
		for (int ipars=0; ipars<num_parsers; ++ipars)
		{
			TParsingDriver *driver = m_owner->m_drivers_array[ipars];
			assert(driver != NULL);

			// Pick up the parser and ensure that it has the right type.
			TGenericParser &gen_pars = driver->Parser();
			assert(gen_pars.GetParserIdentificationCode() == TGrammarBasedParser::ParserIdentCode);

			// Parser has the correct type. Do the typecast.
			TGrammarBasedParser &parser = (TGrammarBasedParser&)gen_pars;

			// Allocate list item for this parser/grammar.
			TBasicListBoxItem *item = new TBasicListBoxItem();
			if (item == NULL)
			{
				m_owner->MsgBox(L"Out of memory on the new ListBox item.\r\nSome drivers will not be displayed.");
				break;
			}

			if (ipars == m_owner->m_curr_driver_inx)
			{
				// Mark this row.
				item->SetStringToCell(m_list_hlp, lvc_curr_driver, L">>>>");
				item->SetFrgrStyleToCell(m_list_hlp, lvc_curr_driver, fcslot_curr_mark, fcslot_curr_mark_sel);
			}

			// Parser: LangType.
			THighLevelLangType langt = parser.GetParserLangType();
			item->SetStringToCell(m_list_hlp, lvc_parser_lang_type, (langt != lang_none) ? THighLevelDatabase::GetLangTypeName(langt) : L"n/a");

			// Parser: Descrition.
			wchar_t buff80[80];
			const wchar_t *handler_name = parser.GetParserDescription(buff80);
			if (handler_name != NULL && handler_name[0] != 0)
				item->SetStringToCell(m_list_hlp, lvc_parser_descr, handler_name);
			else item->SetStringToCell(m_list_hlp, lvc_parser_descr, L"n/a");

			// Grammar info: ParsingResult, Description, Version.
			TGrammar *pgrm = parser.RootGrammar();
			if (pgrm == NULL)
			{
				// The grammar is missing.
				item->SetCellWidth(m_list_hlp, lvc_grm_proc_res, 3);
				item->SetStringToCell(m_list_hlp, lvc_grm_proc_res, L"Grammar is not set into the parser");
				item->SetFrgrStyleToCell(m_list_hlp, lvc_grm_proc_res, fcslot_no_grammar, fcslot_no_grammar_sel);
			}
			else
			{
				// Show the processing result.
				item->SetStringToCell(m_list_hlp, lvc_grm_proc_res, TGrammar::GetProcResultName(pgrm->grm_props.processing_result));

				if (parser.CheckGrammar() == FALSE)
				{
					// The grammar is bad.
					item->SetCellWidth(m_list_hlp, lvc_grm_descr, 2);
					item->SetStringToCell(m_list_hlp, lvc_grm_descr, L"Grammar is not compatible with the parser");
					item->SetFrgrStyleToCell(m_list_hlp, lvc_grm_descr, fcslot_no_grammar, fcslot_no_grammar_sel);
				}
				else
				{
					// The grammar is fine. Show description and version.
					item->SetStringToCell(m_list_hlp, lvc_grm_descr, pgrm->grm_props.grammar_description);
					if (pgrm->grm_props.grammar_revision != 0)
						item->SetFmtStrToCell(m_list_hlp, lvc_grm_vers, L"0x%X", pgrm->grm_props.grammar_revision);
				}
			}

			// Item is formatted. Append it.
			m_list_hlp.AddItem(item);
		}
	}

	// Widows should not set the focus anywhere.
	return(FALSE);
}

void TLangViewVisualParserSelectDriver::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	if (controlId == IDOK || (controlId == IDC_VPSLPD_DRVLIST_LISTBOX && notificationCode == lbnc_left_btn_dblclk))
	{
		int islct = m_list_hlp.GetSelectionIndex();
		if (islct < 0 || islct == m_owner->m_curr_driver_inx)
		{
			// There is no selection or the current driver is selected. Act as if the cancel button was pressed.
			::EndDialog(m_hDlg, -2);
			return;
		}

		// There should be no active parsing session.
		if (m_owner->m_owner->m_vpst != vpst_idle && m_owner->m_owner->m_vpst != vpst_final)
		{
			MsgBox(L"It is not possible to change the parsing driver because the parsing session is active.");
			return;
		}

		// Return the index of the selected driver.
		::EndDialog(m_hDlg, islct);
	}
	else if (controlId == IDCANCEL)
	{
		// Return -2 because -1 means that the dialog has failed to open.
		::EndDialog(m_hDlg, -2);
	}
}

void TLangViewVisualParserSelectDriver::OnWmSize()
{
	// Show/hide the note message.
	RECT rc_label, rc_btn1;
	GetControlRect(IDC_VPSLPD_USEAGE_NOTE, rc_label);
	GetControlRect(IDOK, rc_btn1);
	ShowControl(IDC_VPSLPD_USEAGE_NOTE, (rc_label.right+2 < rc_btn1.left) ? TRUE : FALSE);
}

// -----------------------------------------------------------------------
//   =============  TLangViewVisualParserLoadPanel  =================
// -----------------------------------------------------------------------

static const wchar_t *g_SrcFilterSpecCpp = L"Source Files (*.cpp, *.hpp)\0*.c*;*.h*\0All Files (*.*)\0*.*\0";
static const wchar_t *g_SrcFilterSpecCsh = L"Source Files (*.cs)\0*.cs\0All Files (*.*)\0*.*\0";
static const wchar_t *g_SourceOpenTitle = L" Open Source File ";

static const wchar_t *g_CmdlFilterSpec = L"CL command line files (*.cmdl)\0*.cmdl\0All Files (*.*)\0*.*\0";
static const wchar_t *g_CmdlOpenTitle = L" Open Command Line File ";

static short g_VisualParserLoadPanelButtons[] =
{
	IDC_VPLDCT_BTN_SELECT_DRIVER,
	IDC_VPLDCT_BTN_LOAD_GRAMMAR,
	IDC_VPLDCT_BTN_LOAD_SOURCE,
	IDC_VPLDCT_BTN_PROCESS_CMDL,
	0,
};

TLangViewVisualParserLoadPanel::TLangViewVisualParserLoadPanel(bool initial_cpp_prompt)
		: TBasicDialog(NULL, IDD_LANGV_VPARS_LOAD_PANEL),
		m_processed_grammar_parsing_log(NULL, TRUE),
		m_ofn_source(NULL, initial_cpp_prompt ? g_SrcFilterSpecCpp : g_SrcFilterSpecCsh, (initial_cpp_prompt == TRUE) ? L"CPP" : L"CS", g_SourceOpenTitle),
		m_ofn_cmdl(NULL, g_CmdlFilterSpec, L"CL", g_CmdlOpenTitle)
{
	m_owner = NULL;
	m_drivers_array = NULL;
	m_num_drivers = 0;
	m_curr_driver_inx = -1;

	m_processed_grammar.Clear();
}

void TLangViewVisualParserLoadPanel::SetOwner(TLangViewVisualParserFrame *owner)
{
	m_owner = owner;
	m_select_driver.SetDialogParams(owner);
}

void TLangViewVisualParserLoadPanel::SetDriversInfo(TParsingDriver **drivers, int num_drivers, int curr_drv_inx)
{
	if (drivers != NULL && num_drivers != 0)
	{
		// Ensure that other params are reasonable.
		assert(num_drivers > 0 && num_drivers <= 100);
		assert(curr_drv_inx >= 0 && curr_drv_inx < num_drivers);

		m_drivers_array = drivers;
		m_num_drivers = num_drivers;
		m_curr_driver_inx = curr_drv_inx;
	}
	else
	{
		// The empty list is passed.
		m_drivers_array = NULL;
		m_num_drivers = 0;
		m_curr_driver_inx = -1;
	}
}

LRESULT TLangViewVisualParserLoadPanel::OnInitDialog(HWND hDefFocusControl)
{
	SetupControlsSpacing(g_VisualParserLoadPanelButtons);
	SetupControlPositioning(IDC_VPLDCT_GRPBOX, cpm_fix_all_corners);
	SetMinTrackingSize();

	// Widows should not set focus anywhere.
	return(FALSE);
}

void TLangViewVisualParserLoadPanel::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	bool btn_ctrl = FALSE;
	switch (controlId)
	{
		case IDC_VPLDCT_BTN_SELECT_DRIVER:
				{
					// Ensure the correct state.
					btn_ctrl = TRUE;
					if (CheckParsingDriversPresence(FALSE) == FALSE)
						break;

					// Prompt the user with the list of available drivers.
					m_select_driver.SetDialogParams(this);
					m_select_driver.SetOwner(this);
					int res = m_select_driver.OpenModalDialog(m_hDlg);
					if (res >= 0)
					{
						// New driver is selected.
						assert(m_owner->m_vpst == vpst_idle || m_owner->m_vpst == vpst_final);

						if (m_owner->m_vpst == vpst_final)
						{
							// Terminate the old session.
							m_owner->UnloadParsingSession();
						}

						// Update index of the curr driver and driver info panel.
						m_curr_driver_inx = res;
						m_owner->UpdateParserAndGrammarInfo(GetDriver());
					}
				}
				break;

		case IDC_VPLDCT_BTN_LOAD_GRAMMAR:
				{
					// Ensure the correct state.
					btn_ctrl = TRUE;
					if (CheckParsingDriverIdle() == FALSE)
					{
						// Current state does not allow to load the new grammar.
						break;
					}

					// Prompt the user for the name of the grammar file.
					TLangViewLoadGrammarHelper helper((sctr_err_warn | gdtr_steps | gdtr_reports), TRUE, VPARS_TRACE_PREFIX);
					if (helper.GetGrammarFileName(this) == TRUE)
					{
						// User has selected some grammar definition file.
						if (m_owner->m_vpst == vpst_final)
						{
							// Terminate the old session.
							m_owner->UnloadParsingSession();
						}

						// Prepare the task and run the session.
						DoGrammarLoadingSession(helper);
					}
				}
				break;

		case IDC_VPLDCT_BTN_LOAD_SOURCE:
				{
					// Ensure the correct state.
					btn_ctrl = TRUE;
					if (CheckParsingDriverIdle() == FALSE)
					{
						// Current state is not fine.
						break;
					}

					// Prompt user for the name of the source file.
					if (m_ofn_source.GetOpenFileName(m_owner->GetWnd()) == TRUE)
					{
						// User selected some source file.
						if (m_owner->m_vpst == vpst_final)
						{
							// Terminate the old session.
							m_owner->UnloadParsingSession();
						}

						// Start the source code parsing session.
						TProcessSourceFileInfo info;
						info.Clear();
						info.lang_type = GetParser()->RootGrammar()->grm_props.lang_type;
						info.src_fname = (wchar_t*)m_ofn_source.GetCurrFileName();
						m_owner->StartProcessingSourceFile(*GetDriver(), info);
					}
				}
				break;

		case IDC_VPLDCT_BTN_PROCESS_CMDL:
				{
					// Ensure the correct state.
					btn_ctrl = TRUE;
					if (CheckParsingDriverIdle() == FALSE)
					{
						// Current state is not fine.
						break;
					}

					// Prompt user for the name of the command line file.
					if (m_ofn_cmdl.GetOpenFileName(m_owner->GetWnd()) == TRUE)
					{
						// User has selected some CL command line file.
						if (m_owner->m_vpst == vpst_final)
						{
							// Terminate the old session.
							m_owner->UnloadParsingSession();
						}

						// For now just show the message.
						MsgBoxFmt(MB_OK, L"CmdLine: %s\r\n\r\nNot implemented yet.", m_ofn_cmdl.GetCurrFileName());
					}
				}
				break;
	}

	if (btn_ctrl == TRUE)
		ControlStyleBitsAnd(controlId, BS_DEFPUSHBUTTON);
}

void TLangViewVisualParserLoadPanel::UpdateLowerFrameOffset(long lower_frame_offs)
{
	assert(m_hDlg != NULL);
	RECT rc_dlg, rc_frame;
	::GetWindowRect(m_hDlg, &rc_dlg);
	GetControlRect(IDC_VPLDCT_GRPBOX, rc_frame);
	SetControlSize(IDC_VPLDCT_GRPBOX, rc_frame.right-rc_frame.left, rc_dlg.bottom-rc_dlg.top-lower_frame_offs-rc_frame.top);
	SetupControlPositioning(IDC_VPLDCT_GRPBOX, cpm_fix_all_corners);
}

bool TLangViewVisualParserLoadPanel::CheckParsingDriversPresence(bool full_check)
{
	if (m_drivers_array == NULL || m_num_drivers <= 0)
	{
		m_owner->MsgBox(L"Parsing drivers are not available.");
		return(FALSE);
	}

	if (full_check == TRUE)
	{
		// There should be no active parsing session.
		if (m_owner->m_vpst != vpst_idle && m_owner->m_vpst != vpst_final)
		{
			m_owner->MsgBox(L"Parsing session is active.");
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

bool TLangViewVisualParserLoadPanel::CheckParsingDriverIdle()
{
	// Driver should be present.
	if (GetParser() == NULL)
	{
		m_owner->MsgBox(L"Parsing driver is not set.");
		return(FALSE);
	}

	// There should be no active parsing session.
	if (m_owner->m_vpst != vpst_idle && m_owner->m_vpst != vpst_final)
	{
		m_owner->MsgBox(L"Parsing session is active. It is necessary to finish it first.");
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

void TLangViewVisualParserLoadPanel::DoGrammarLoadingSession(TLangViewLoadGrammarHelper &helper)
{
	// Pick up the parser from the driver.
	TGrammarBasedParser *parser = GetParser();
	assert(parser != NULL);

	// Execute the async session. The function below will open the modal dialog and it will process messages.
	// Besides that it will start the worker thread. Once the worker thread will finish its work, the modal dialog
	// will be closed and the function will return control.
	TGrammar grammar;
	TMidLevRamLogger grammar_parsing_log;
	TLangViewBasicAsyncConsole::SessionResults sres = helper.LoadGrammarDefinition(this, &grammar, &grammar_parsing_log);

	// Show messages about loading problems if any.
	helper.ShowSessionResults(this);
	if (sres == TLangViewBasicAsyncConsole::sres_app_result && grammar.grm_props.processing_result == grpr_full_success)
	{
		// Loading and parsing/converting the grammar succceeded. Check the compatibility between
		// the grammar and the current parser.
		TGrammar *saved_grammar = parser->RootGrammar();
		parser->SetRootGrammar(&grammar);
		if (parser->CheckGrammar() == FALSE)
		{
			wchar_t buff_supp[80];
			const wchar_t *parser_handles = parser->GetParserDescription(buff_supp);
			wchar_t *grammar_wants = grammar.grm_props.cbk_handler_name;
			MsgBoxFmt(MB_OK, L"Loaded grammar is not compatible with the current parsing driver. Check the description fields:\r\n\r\n"
						L"   Parser says: \t%s\r\n   Grammar: \t%s\r\n\r\nThe grammar in the current parsing driver has not changed. The new grammar is discarded.",
						(parser_handles != NULL && parser_handles[0] != 0) ? parser_handles : L"None", (grammar_wants[0] != 0) ? grammar_wants : L"None");

			// Set the old grammar back.
			parser->SetRootGrammar(saved_grammar);
		}
		else
		{
			// Accept the parsing results.
			m_processed_grammar.TakeContentsFrom(grammar);
			m_processed_grammar_parsing_log.TakeContentsFrom(grammar_parsing_log);

			// Set converted grammar into the parser and display details.
			parser->SetRootGrammar(&m_processed_grammar);
			m_owner->UpdateParserAndGrammarInfo(GetDriver());

			// Check, if the loaded grammar is used in other drivers or not.
			for (int idrv=0; idrv<m_num_drivers; ++idrv)
			{
				TGrammarBasedParser &grb_parser = (TGrammarBasedParser&)m_drivers_array[idrv]->Parser();
				if (idrv != m_curr_driver_inx && grb_parser.RootGrammar() == &m_processed_grammar)
				{
					MsgBoxFmt(MB_OK, L"WARNING: Dynamically loaded grammar is also used by the parsing driver with index %d.", idrv);
				}
			}
		}
	}
}

// --------------------------------------------------------------------
//   ===========  TLangViewVisualParserControlPanel  ===============
// --------------------------------------------------------------------

static const TControlPosInfo g_VisualParserControlPanelCtrls[] =
{
	IDC_VPCTRL_CTRL_GRPBOX,			cpm_fix_top_side,
	IDC_VPCTRL_CURR_STATE_VAL,			cpm_fix_top_side,
	IDC_VPCTRL_STATE_DETAILS1,			cpm_fix_top_side,
	IDC_VPCTRL_STATE_DETAILS2,			cpm_fix_top_side,
	IDC_VPCTRL_STATE_DETAILS3,			cpm_fix_top_side,
	IDC_VPCTRL_APP_NOTE_NOTE,			cpm_fix_top_side,
	IDC_VPCTRL_MANUAL_RES_HEADER,		cpm_fix_top_side,
	IDC_VPCTRL_BTN_KEEP_CONFLICT,		cpm_fix_top_side,
	IDC_VPCTRL_MORE_ACTIONS_LAB,		cpm_fix_top_side,
	IDC_VPCTRL_SETUP_GRPBOX,			cpm_fix_right_top,
	IDC_VPCTRL_RUN_LEFT,				cpm_fix_right_top,
	IDC_VPCTRL_STEP_LEFT,				cpm_fix_right_top,
	IDC_VPCTRL_CHK_STARTUP_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_STARTUP_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_STARTUP_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_COMPLETE_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_COMPLETE_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_COMPLETE_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_PICK_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_PICK_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_PICK_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_UPDT_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_UPDT_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_UPDT_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_DSCRD_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_DSCRD_STEP,	cpm_fix_right_top,
	IDC_VPCTRL_CHK_SYM_DSCRD_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_RECOVERY_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_RECOVERY_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_RECOVERY_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_RUN_RIGHT,				cpm_fix_right_top,
	IDC_VPCTRL_STEP_RIGHT,				cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_PICK_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_PICK_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_PICK_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_CFCT_PICK_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_CFCT_PICK_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_CFCT_PICK_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_UPDT_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_UPDT_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_UPDT_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_SHIFT_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_SHIFT_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_SHIFT_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_APPLY_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_APPLY_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ACT_APPLY_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ENTR_NEST_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ENTR_NEST_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_ENTR_NEST_LAB,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_EXIT_NEST_RUN,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_EXIT_NEST_STEP,		cpm_fix_right_top,
	IDC_VPCTRL_CHK_EXIT_NEST_LAB,		cpm_fix_right_top,
	0, cpm_none,
};

static short g_ControlPanelLeftColumnRun[] =
{
	IDC_VPCTRL_CHK_STARTUP_RUN,		IDC_VPCTRL_CHK_COMPLETE_RUN,
	IDC_VPCTRL_CHK_SYM_PICK_RUN,		IDC_VPCTRL_CHK_SYM_UPDT_RUN,
	IDC_VPCTRL_CHK_SYM_DSCRD_RUN,		IDC_VPCTRL_CHK_RECOVERY_RUN,
	0,
};

static short g_ControlPanelLeftColumnStep[] =
{
	IDC_VPCTRL_CHK_STARTUP_STEP,		IDC_VPCTRL_CHK_COMPLETE_STEP,
	IDC_VPCTRL_CHK_SYM_PICK_STEP,		IDC_VPCTRL_CHK_SYM_UPDT_STEP,
	IDC_VPCTRL_CHK_SYM_DSCRD_STEP,	IDC_VPCTRL_CHK_RECOVERY_STEP,
	0,
};

static short g_ControlPanelLeftColumnLabels[] =
{
	IDC_VPCTRL_CHK_STARTUP_LAB,		IDC_VPCTRL_CHK_COMPLETE_LAB,
	IDC_VPCTRL_CHK_SYM_PICK_LAB,		IDC_VPCTRL_CHK_SYM_UPDT_LAB,
	IDC_VPCTRL_CHK_SYM_DSCRD_LAB,		IDC_VPCTRL_CHK_RECOVERY_LAB,
	0,
};

static short g_ControlPanelRightColumnRun[] =
{
	IDC_VPCTRL_CHK_ACT_PICK_RUN,		IDC_VPCTRL_CHK_CFCT_PICK_RUN,
	IDC_VPCTRL_CHK_ACT_UPDT_RUN,		IDC_VPCTRL_CHK_ACT_SHIFT_RUN,
	IDC_VPCTRL_CHK_ACT_APPLY_RUN,		IDC_VPCTRL_CHK_ENTR_NEST_RUN,
	IDC_VPCTRL_CHK_EXIT_NEST_RUN,		0,
};

static short g_ControlPanelRightColumnStep[] =
{
	IDC_VPCTRL_CHK_ACT_PICK_STEP,		IDC_VPCTRL_CHK_CFCT_PICK_STEP,
	IDC_VPCTRL_CHK_ACT_UPDT_STEP,		IDC_VPCTRL_CHK_ACT_SHIFT_STEP,
	IDC_VPCTRL_CHK_ACT_APPLY_STEP,		IDC_VPCTRL_CHK_ENTR_NEST_STEP,
	IDC_VPCTRL_CHK_EXIT_NEST_STEP,		0,
};

static short g_ControlPanelRightColumnLabels[] =
{
	IDC_VPCTRL_CHK_ACT_PICK_LAB,		IDC_VPCTRL_CHK_CFCT_PICK_LAB,
	IDC_VPCTRL_CHK_ACT_UPDT_LAB,		IDC_VPCTRL_CHK_ACT_SHIFT_LAB,
	IDC_VPCTRL_CHK_ACT_APPLY_LAB,		IDC_VPCTRL_CHK_ENTR_NEST_LAB,
	IDC_VPCTRL_CHK_EXIT_NEST_LAB,		0,
};

static short g_ControlPanelRunCheckBoxes[pcev_num_items] =
{
	0,									// pcev_none
	IDC_VPCTRL_CHK_STARTUP_RUN,		// pcev_parsing_startup
	IDC_VPCTRL_CHK_COMPLETE_RUN,		// pcev_parsing_complete
	IDC_VPCTRL_CHK_SYM_PICK_RUN,		// pcev_symbol_pickup
	IDC_VPCTRL_CHK_SYM_UPDT_RUN,		// pcev_symbol_update
	IDC_VPCTRL_CHK_SYM_DSCRD_RUN,		// pcev_symbol_discard
	IDC_VPCTRL_CHK_ACT_PICK_RUN,		// pcev_action_pickup
	0,									// pcev_action_resolution
	IDC_VPCTRL_CHK_ACT_UPDT_RUN,		// pcev_action_update
	IDC_VPCTRL_CHK_ACT_SHIFT_RUN,		// pcev_action_shift
	IDC_VPCTRL_CHK_ACT_APPLY_RUN,		// pcev_action_resolve
	IDC_VPCTRL_CHK_ENTR_NEST_RUN,		// pcev_enter_nesting
	IDC_VPCTRL_CHK_EXIT_NEST_RUN,		// pcev_exit_nesting
	IDC_VPCTRL_CHK_RECOVERY_RUN,		// pcev_errec_sym_added
	0,									// pcev_errec_sym_updated
	0,									// pcev_errec_sym_removed
	IDC_VPCTRL_CHK_CFCT_PICK_RUN,		// pcev_grpars_cfct_pickup
};

static short g_ControlPanelStepCheckBoxes[pcev_num_items] =
{
	0,									// pcev_none
	IDC_VPCTRL_CHK_STARTUP_STEP,		// pcev_parsing_startup
	IDC_VPCTRL_CHK_COMPLETE_STEP,		// pcev_parsing_complete
	IDC_VPCTRL_CHK_SYM_PICK_STEP,		// pcev_symbol_pickup
	IDC_VPCTRL_CHK_SYM_UPDT_STEP,		// pcev_symbol_update
	IDC_VPCTRL_CHK_SYM_DSCRD_STEP,	// pcev_symbol_discard
	IDC_VPCTRL_CHK_ACT_PICK_STEP,		// pcev_action_pickup
	0,									// pcev_action_resolution
	IDC_VPCTRL_CHK_ACT_UPDT_STEP,		// pcev_action_update
	IDC_VPCTRL_CHK_ACT_SHIFT_STEP,		// pcev_action_shift
	IDC_VPCTRL_CHK_ACT_APPLY_STEP,		// pcev_action_resolve
	IDC_VPCTRL_CHK_ENTR_NEST_STEP,		// pcev_enter_nesting
	IDC_VPCTRL_CHK_EXIT_NEST_STEP,		// pcev_exit_nesting
	IDC_VPCTRL_CHK_RECOVERY_STEP,		// pcev_errec_sym_added
	0,									// pcev_errec_sym_updated
	0,									// pcev_errec_sym_removed
	IDC_VPCTRL_CHK_CFCT_PICK_STEP,		// pcev_grpars_cfct_pickup
};

#define VPCTRL_CHK_BOX_SIZE_X 13
#define VPCTRL_CHK_BOX_SIZE_Y 13

#define VPCTRL_CHK_BOX_BKGR_NORMAL	::GetSysColor(COLOR_WINDOW)
#define VPCTRL_CHK_BOX_BKGR_HGTLT		RGB(226, 168, 61)

TLangViewVisualParserControlPanel::TLangViewVisualParserControlPanel()
		: TBasicDialog(NULL, IDD_LANGV_VPARS_CONTROL_PANEL)
{
	m_owner = NULL;
	m_stt_chkbox = pcev_none;
}

LRESULT TLangViewVisualParserControlPanel::OnInitDialog(HWND hDefFocusControl)
{
	// Setup Fonts.
	SetupDarkBlueText(IDC_VPCTRL_CURR_STATE_VAL);
	SetControlFont(IDC_VPCTRL_RUN_LEFT, TAHOMA, 7);
	SetControlFont(IDC_VPCTRL_STEP_LEFT, TAHOMA, 7);
	SetControlFont(IDC_VPCTRL_RUN_RIGHT, TAHOMA, 7);
	SetControlFont(IDC_VPCTRL_STEP_RIGHT, TAHOMA, 7);
	SetupDarkBlueText(IDC_VPCTRL_APP_NOTE_TITLE);
	SetControlFont(IDC_VPCTRL_APP_NOTE_TITLE, VERDANA, 8, TRUE);
	SetControlFont(IDC_VPCTRL_APP_NOTE_NOTE, TAHOMA, 7);
	SetControlColor(IDC_VPCTRL_APP_NOTE_NOTE, RGB(116, 116, 116));
	SetControlBkgrBrush(IDC_VPCTRL_APP_NOTE_NOTE, ::GetSysColorBrush(COLOR_3DFACE), FALSE);

	// Adjust positioning.
	SetupControlsSpacing(g_ControlPanelLeftColumnRun);
	SetupControlsSpacing(g_ControlPanelLeftColumnStep);
	SetupControlsSpacing(g_ControlPanelLeftColumnLabels);
	SetupControlsSpacing(g_ControlPanelRightColumnRun);
	SetupControlsSpacing(g_ControlPanelRightColumnStep);
	SetupControlsSpacing(g_ControlPanelRightColumnLabels);
	SetupControlsSize(g_ControlPanelLeftColumnRun, VPCTRL_CHK_BOX_SIZE_X, VPCTRL_CHK_BOX_SIZE_Y);
	SetupControlsSize(g_ControlPanelLeftColumnStep, VPCTRL_CHK_BOX_SIZE_X, VPCTRL_CHK_BOX_SIZE_Y);
	SetupControlsSize(g_ControlPanelRightColumnRun, VPCTRL_CHK_BOX_SIZE_X, VPCTRL_CHK_BOX_SIZE_Y);
	SetupControlsSize(g_ControlPanelRightColumnStep, VPCTRL_CHK_BOX_SIZE_X, VPCTRL_CHK_BOX_SIZE_Y);
	ShiftControl(IDC_VPCTRL_STEP_LEFT, -1, 0);
	ShiftControlsList(g_ControlPanelLeftColumnRun, 0, 1);
	ShiftControlsList(g_ControlPanelLeftColumnStep, 0, 1);
	ShiftControlsList(g_ControlPanelRightColumnRun, 0, 1);
	ShiftControlsList(g_ControlPanelRightColumnStep, 1, 1);
	ShiftControlsList(g_ControlPanelRightColumnLabels, 1, 0);

	// Prepare resizing.
	RECT rc;
	GetControlRect(IDC_VPCTRL_CURR_STATE_LAB, rc);
	SetControlPos(IDC_VPCTRL_CURR_STATE_LAB, rc.left, rc.top+1);
	SetupControlsListPositioning(g_VisualParserControlPanelCtrls);
	SetMinTrackingSize();

	// Check and grey "parsing complete" check boxes. In fact they are not associated with any event.
	SetCheckMark(IDC_VPCTRL_CHK_COMPLETE_RUN, TRUE);
	SetCheckMark(IDC_VPCTRL_CHK_COMPLETE_STEP, TRUE);
	EnableControl(IDC_VPCTRL_CHK_COMPLETE_RUN, FALSE);
	EnableControl(IDC_VPCTRL_CHK_COMPLETE_STEP, FALSE);

	// Initial state of the frame is idle.
	SetVisibleState(vpst_idle);
	HideManualConflictResolutionControls();

	// Widows should not set focus anywhere.
	return(FALSE);
}

void TLangViewVisualParserControlPanel::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		//
		// Parsing start group.
		//
		case IDC_VPCTRL_CHK_STARTUP_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_STARTUP_RUN, pcev_parsing_startup);
				break;
		case IDC_VPCTRL_CHK_STARTUP_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_STARTUP_STEP, pcev_parsing_startup);
				break;

		//
		// Input symbol pickup/update/discard group.
		//
		case IDC_VPCTRL_CHK_SYM_PICK_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_SYM_PICK_RUN, pcev_symbol_pickup);
				break;
		case IDC_VPCTRL_CHK_SYM_PICK_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_SYM_PICK_STEP, pcev_symbol_pickup);
				break;
		case IDC_VPCTRL_CHK_SYM_UPDT_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_SYM_UPDT_RUN, pcev_symbol_update);
				break;
		case IDC_VPCTRL_CHK_SYM_UPDT_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_SYM_UPDT_STEP, pcev_symbol_update);
				break;
		case IDC_VPCTRL_CHK_SYM_DSCRD_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_SYM_DSCRD_RUN, pcev_symbol_discard);
				break;
		case IDC_VPCTRL_CHK_SYM_DSCRD_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_SYM_DSCRD_STEP, pcev_symbol_discard);
				break;

		//
		// Syntax error recovery group.
		//
		case IDC_VPCTRL_CHK_RECOVERY_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_RECOVERY_RUN, pcev_errec_sym_added);
				break;
		case IDC_VPCTRL_CHK_RECOVERY_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_RECOVERY_STEP, pcev_errec_sym_added);
				break;

		//
		// Parsing action pickup/pickup_conflict/update/shift/resolve group.
		//
		case IDC_VPCTRL_CHK_ACT_PICK_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ACT_PICK_RUN, pcev_action_pickup);
				break;
		case IDC_VPCTRL_CHK_ACT_PICK_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ACT_PICK_STEP, pcev_action_pickup);
				break;
		case IDC_VPCTRL_CHK_CFCT_PICK_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_CFCT_PICK_RUN, pcev_grpars_cfct_pickup);
				break;
		case IDC_VPCTRL_CHK_CFCT_PICK_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_CFCT_PICK_STEP, pcev_grpars_cfct_pickup);
				break;
		case IDC_VPCTRL_CHK_ACT_UPDT_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ACT_UPDT_RUN, pcev_action_update);
				break;
		case IDC_VPCTRL_CHK_ACT_UPDT_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ACT_UPDT_STEP, pcev_action_update);
				break;
		case IDC_VPCTRL_CHK_ACT_SHIFT_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ACT_SHIFT_RUN, pcev_action_shift);
				break;
		case IDC_VPCTRL_CHK_ACT_SHIFT_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ACT_SHIFT_STEP, pcev_action_shift);
				break;
		case IDC_VPCTRL_CHK_ACT_APPLY_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ACT_APPLY_RUN, pcev_action_resolve);
				break;
		case IDC_VPCTRL_CHK_ACT_APPLY_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ACT_APPLY_STEP, pcev_action_resolve);
				break;

		//
		// Nested parsing entry/exit.
		//
		case IDC_VPCTRL_CHK_ENTR_NEST_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ENTR_NEST_RUN, pcev_enter_nesting);
				break;
		case IDC_VPCTRL_CHK_ENTR_NEST_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_ENTR_NEST_STEP, pcev_enter_nesting);
				break;
		case IDC_VPCTRL_CHK_EXIT_NEST_RUN:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_EXIT_NEST_RUN, pcev_exit_nesting);
				break;
		case IDC_VPCTRL_CHK_EXIT_NEST_STEP:
				UpdateMaskCheckBoxesAndFlags(IDC_VPCTRL_CHK_EXIT_NEST_STEP, pcev_exit_nesting);
				break;

		default:
			// Send all other commands to the parent for processing.
			m_owner->OnWmCommand(controlId, notificationCode, hWndControl);
			break;
	}
}

void TLangViewVisualParserControlPanel::SetupInitialMaskCheckBoxesState(DWORD run_mask, DWORD step_mask)
{
	// Setup left column checkboxes with break masks.
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_parsing_startup);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_symbol_pickup);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_symbol_update);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_symbol_discard);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_errec_sym_added);

	// Setup right column checkboxes with break masks.
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_action_pickup);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_grpars_cfct_pickup);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_action_update);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_action_shift);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_action_resolve);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_enter_nesting);
	SetupMaskCheckBoxes(run_mask, step_mask, pcev_exit_nesting);
}

void TLangViewVisualParserControlPanel::SetVisibleState(TLangViewVisualParserState stt,
											TGenericParserConsoleEvent stt_chk_box,
											const wchar_t *stt_label, const wchar_t *stt_message,
											const wchar_t *stt_dtls1, const wchar_t *stt_dtls2, const wchar_t *stt_dtls3,
											const wchar_t *application_note,
											TGrammar *grammar, int iconflict)
{
	//
	//  Step1: Prepare state of the buttons on top (6 buttons).
	//

	// Show/hide action buttons.
	bool many_btns = TRUE;
	if (stt == vpst_stopped_error || stt == vpst_final)
		many_btns = FALSE;

	ShowControl(IDC_VPCTRL_BTN_STEP, many_btns);
	ShowControl(IDC_VPCTRL_BTN_RUN,  many_btns);
	ShowControl(IDC_VPCTRL_BTN_BREAK, many_btns);
	ShowControl(IDC_VPCTRL_BTN_ABORT, many_btns);
	ShowControl(IDC_VPCTRL_BTN_GOON, (stt == vpst_stopped_error));
	ShowControl(IDC_VPCTRL_BTN_UNLOAD, (stt == vpst_final));
	ShowControl(IDC_VPCTRL_BTN_MIDLEVDB, (stt == vpst_final));

	if (many_btns == TRUE)
	{
		// Remove black frames if any.
		ControlStyleBitsAnd(IDC_VPCTRL_BTN_STEP, BS_DEFPUSHBUTTON);
		ControlStyleBitsAnd(IDC_VPCTRL_BTN_RUN,  BS_DEFPUSHBUTTON);
		ControlStyleBitsAnd(IDC_VPCTRL_BTN_BREAK, BS_DEFPUSHBUTTON);
		ControlStyleBitsAnd(IDC_VPCTRL_BTN_ABORT, BS_DEFPUSHBUTTON);
	}

	// Predisable all buttons.
	EnableControl(IDC_VPCTRL_BTN_STEP, FALSE);
	EnableControl(IDC_VPCTRL_BTN_RUN,  FALSE);
	EnableControl(IDC_VPCTRL_BTN_BREAK, FALSE);
	EnableControl(IDC_VPCTRL_BTN_ABORT, FALSE);

	// Enable buttons in those states that have at least one enabled action button.
	// Note that "Go on" "Unload" buttons are never disabled. They are either visible or hidden.
	switch (stt)
	{
		case vpst_stopped:
				// Enable alsmost all buttons.
				EnableControl(IDC_VPCTRL_BTN_STEP, TRUE);
				EnableControl(IDC_VPCTRL_BTN_RUN, TRUE);
				EnableControl(IDC_VPCTRL_BTN_ABORT, TRUE);
				break;

		case vpst_running_step:
				EnableControl(IDC_VPCTRL_BTN_BREAK, TRUE);
				EnableControl(IDC_VPCTRL_BTN_ABORT, TRUE);
				break;

		case vpst_running_run:
				EnableControl(IDC_VPCTRL_BTN_BREAK, TRUE);
				EnableControl(IDC_VPCTRL_BTN_ABORT, TRUE);
				break;

		case vpst_running_break:
				EnableControl(IDC_VPCTRL_BTN_ABORT, TRUE);
				break;
	}

	if (grammar == NULL && IsControlVisible(IDC_VPCTRL_MANUAL_RES_HEADER) == TRUE)
	{
		// Hide all manual conflict resolution related controls.
		HideManualConflictResolutionControls();
	}

	// Idle state is special.
	if (stt == vpst_idle)
	{
		// Hide label and all messages.
		SetControlText(IDC_VPCTRL_CURR_STATE_LAB, NULL);
		SetControlText(IDC_VPCTRL_CURR_STATE_VAL, NULL);
		SetControlText(IDC_VPCTRL_STATE_DETAILS1, NULL);
		SetControlText(IDC_VPCTRL_STATE_DETAILS2, NULL);
		SetControlText(IDC_VPCTRL_STATE_DETAILS3, NULL);
		ShowControl(IDC_VPCTRL_APP_NOTE_TITLE, FALSE);
		SetControlText(IDC_VPCTRL_APP_NOTE_NOTE, NULL);
		UpdateMaskCheckBoxesBackground(stt_chk_box);
		return;
	}

	//
	//  Step 2: Show main state and details labels.
	//

	// Rest of the states should pass state label and message explicitly.
	assert(stt_label != NULL && stt_message != NULL);

	// Ensure good position of the label message.
	HFONT hDialogFont = (HFONT)::SendMessage(m_hDlg, WM_GETFONT, 0, 0);
	if (hDialogFont != NULL)
	{
		// Figure out width of the new label in dialog font.
		HDC hDC = ::GetDC(m_hDlg);
		assert(hDC != NULL);

		SIZE text_size;
		HFONT oldFont = (HFONT)::SelectObject(hDC, hDialogFont);
		::GetTextExtentPoint32W(hDC, stt_label, (int)wcslen(stt_label), &text_size);
		::SelectObject(hDC, oldFont);
		::ReleaseDC(m_hDlg, hDC);

		// Pick up rects of label and message.
		RECT rc_lab, rc_val;
		GetControlRect(IDC_VPCTRL_CURR_STATE_LAB, rc_lab);
		GetControlRect(IDC_VPCTRL_CURR_STATE_VAL, rc_val);

		// Fugure out relation and adjust control,
		long new_val_left = rc_lab.left + text_size.cx + 8;
		long diff = new_val_left-rc_val.left;
		if (diff > 0 || diff < -6)
		{
			rc_val.left = new_val_left;
			SetControlRect(IDC_VPCTRL_CURR_STATE_VAL, rc_val);
			SetupControlPositioning(IDC_VPCTRL_CURR_STATE_VAL, cpm_fix_top_side);
		}
	}

	// Update label only if new label differs from the old one.
	wchar_t buff[80];
	GetControlText(IDC_VPCTRL_CURR_STATE_LAB, buff, 80);
	if (wcscmp(stt_label, buff) != 0)
	{
		SetControlText(IDC_VPCTRL_CURR_STATE_LAB, stt_label);
	}

	// Main message and details are updated all the times.
	SetControlText(IDC_VPCTRL_CURR_STATE_VAL, stt_message);
	SetControlText(IDC_VPCTRL_STATE_DETAILS1, stt_dtls1);
	SetControlText(IDC_VPCTRL_STATE_DETAILS2, stt_dtls2);
	SetControlText(IDC_VPCTRL_STATE_DETAILS3, stt_dtls3);

	//
	//  Step 3: Show the app note, manual conflict resolution controls and the checkbox bkgr.
	//

	if (application_note != NULL)
	{
		ShowControl(IDC_VPCTRL_APP_NOTE_TITLE, TRUE);
		UpdateDialogWindow();
		SetControlText(IDC_VPCTRL_APP_NOTE_NOTE, application_note);
	}
	else
	{
		ShowControl(IDC_VPCTRL_APP_NOTE_TITLE, FALSE);
		SetControlText(IDC_VPCTRL_APP_NOTE_NOTE, NULL);
	}

	if (grammar != NULL)
	{
		// Show info about the conflict.
		TGrammarConflict &cfct = grammar->conflicts[iconflict];
		ShowControl(IDC_VPCTRL_MANUAL_RES_HEADER, TRUE);

		// Display possible parsing actions.
		int rc_id = IDC_VPCTRL_BTN_ACT1;
		for (int iact=0; iact<numManualCfctResRadioBtns; ++iact, ++rc_id)
		{
			SetCheckMark(rc_id, FALSE);
			if (iact < cfct.NumActions())
			{
				// Display control and set its title.
				ShowControl(rc_id, TRUE);
				wchar_t buff40[40];
				SetControlText(rc_id, grammar->GetShortParsingActionName(cfct.GetAction(iact), buff40));
			}
			else
			{
				// Hide the control.
				ShowControl(rc_id, FALSE);
			}
		}

		// Show/hide the more indicator.
		bool bMoreVisib = (cfct.NumActions() > numManualCfctResRadioBtns) ? TRUE : FALSE;
		ShowControl(IDC_VPCTRL_MORE_ACTIONS_LAB, bMoreVisib);
		if (bMoreVisib == TRUE)
			SetControlTextFmt(IDC_VPCTRL_MORE_ACTIONS_LAB, L"... %d actions total.", cfct.NumActions());

		// Check "keep the conflict" option by default. Do this in a bit ricky way. I.e. first show the unckecked button
		// and then show check only after updating the window. This makes the painting better.
		SetControlTextFmt(IDC_VPCTRL_BTN_KEEP_CONFLICT, L"Keep conflict (%s)",
						(cfct.nested_grammar == NULL) ? L"cbk resolution" : L"nested grammar");
		SetCheckMark(IDC_VPCTRL_BTN_KEEP_CONFLICT, FALSE);
		ShowControl(IDC_VPCTRL_BTN_KEEP_CONFLICT, TRUE);
		UpdateDialogWindow();
		SetCheckMark(IDC_VPCTRL_BTN_KEEP_CONFLICT, TRUE);
	}

	UpdateMaskCheckBoxesBackground(stt_chk_box);
}

int TLangViewVisualParserControlPanel::GetCurrentConflictingActionSelection()
{
	int rc_id = IDC_VPCTRL_BTN_ACT1;
	for (int iact=0; iact<numManualCfctResRadioBtns; ++iact, ++rc_id)
	{
		if (IsControlVisible(rc_id) == TRUE && GetCheckMark(rc_id) == TRUE)
			return(iact);
	}
	return(-1);
}

long TLangViewVisualParserControlPanel::GetLowerFrameOffset()
{
	assert(m_hDlg != NULL);

	RECT rc_dlg, rc_frame;
	::GetWindowRect(m_hDlg, &rc_dlg);
	GetControlRect(IDC_VPCTRL_CTRL_GRPBOX, rc_frame);
	return(rc_dlg.bottom-rc_dlg.top-rc_frame.bottom);
}

void TLangViewVisualParserControlPanel::SetupDarkBlueText(int rc_id)
{
	SetControlFont(rc_id, VERDANA, 9, TRUE);
	SetControlColor(rc_id, RGB(0, 0, 96));
	SetControlBkgrBrush(rc_id, ::GetSysColorBrush(COLOR_3DFACE), FALSE);
}

void TLangViewVisualParserControlPanel::HideManualConflictResolutionControls()
{
	ShowControl(IDC_VPCTRL_MANUAL_RES_HEADER, FALSE);
	ShowControl(IDC_VPCTRL_BTN_KEEP_CONFLICT, FALSE);
	ShowControl(IDC_VPCTRL_BTN_ACT1, FALSE);
	ShowControl(IDC_VPCTRL_BTN_ACT2, FALSE);
	ShowControl(IDC_VPCTRL_BTN_ACT3, FALSE);
	ShowControl(IDC_VPCTRL_BTN_ACT4, FALSE);
	ShowControl(IDC_VPCTRL_MORE_ACTIONS_LAB, FALSE);
}

void TLangViewVisualParserControlPanel::SetupMaskCheckBoxes(DWORD run_mask, DWORD step_mask, TGenericParserConsoleEvent mask_bit)
{
	// This method is called only from OnInitDialog so it is ok to assume that check boxes are not checked.
	short rc_step_chkbox = g_ControlPanelStepCheckBoxes[mask_bit];
	if ((run_mask & (1 << mask_bit)) != 0)
	{
		// Show enabled state on the run checkbox plus check and disable step chkbox.
		SetCheckMark(g_ControlPanelRunCheckBoxes[mask_bit], TRUE);
		SetCheckMark(rc_step_chkbox, TRUE);
		EnableControl(rc_step_chkbox, FALSE);
	}
	else if ((step_mask & (1 << mask_bit)) != 0)
	{
		// Just show enabled state on the step check box.
		SetCheckMark(rc_step_chkbox, TRUE);
	}
}

void TLangViewVisualParserControlPanel::UpdateMaskCheckBoxesAndFlags(int rc_clicked, TGenericParserConsoleEvent mask_bit)
{
	// Update corresponding checkboxes if any.
	bool bCheck = GetCheckMark(rc_clicked);
	int rc_run = g_ControlPanelRunCheckBoxes[mask_bit];
	int rc_step = g_ControlPanelStepCheckBoxes[mask_bit];

	if (rc_clicked == rc_run)
	{
		// Update the corresponding step check box.
		if (bCheck == TRUE)
		{
			// Check and disable the step checkbox.
			SetCheckMark(rc_step, TRUE);
			EnableControl(rc_step, FALSE);
			if (mask_bit == m_stt_chkbox)
			{
				// Move highlight mask from Step to Run.
				::SendDlgItemMessage(m_hDlg, rc_step, DLG_CTRL_SET_CHKBOX_BKGR_COLOR, 0, VPCTRL_CHK_BOX_BKGR_NORMAL);
				::SendDlgItemMessage(m_hDlg, rc_run, DLG_CTRL_SET_CHKBOX_BKGR_COLOR, 0, VPCTRL_CHK_BOX_BKGR_HGTLT);
			}
		}
		else
		{
			// Enable the step checkbox and show value from the step mask there.
			EnableControl(rc_step, TRUE);
			if ((m_owner->m_step_brks_mask & (1 << mask_bit)) == 0)
				SetCheckMark(rc_step, FALSE);

			if (mask_bit == m_stt_chkbox)
			{
				// Move highlight mask from Run to Step.
				::SendDlgItemMessage(m_hDlg, rc_run, DLG_CTRL_SET_CHKBOX_BKGR_COLOR, 0, VPCTRL_CHK_BOX_BKGR_NORMAL);
				::SendDlgItemMessage(m_hDlg, rc_step, DLG_CTRL_SET_CHKBOX_BKGR_COLOR, 0, VPCTRL_CHK_BOX_BKGR_HGTLT);
			}
		}
	}

	// Send notification to the main frame.
	m_owner->UpdateSuspensionMasksFromControlPanel((rc_clicked == rc_run), mask_bit, bCheck);
}

void TLangViewVisualParserControlPanel::UpdateMaskCheckBoxesBackground(TGenericParserConsoleEvent stt_chk_box)
{
	if (stt_chk_box == m_stt_chkbox)
		return;

	// Remove the old background.
	if (m_stt_chkbox != pcev_none)
	{
		if (m_stt_chkbox == pcev_parsing_complete)
		{
			// Reset disabled background color in both check boxes.
			::SendDlgItemMessage(m_hDlg, IDC_VPCTRL_CHK_COMPLETE_RUN, DLG_CTRL_SET_CHKBOX_DISABLED_BKGR_COLOR, 0, ::GetSysColor(COLOR_3DFACE));
			::SendDlgItemMessage(m_hDlg, IDC_VPCTRL_CHK_COMPLETE_STEP, DLG_CTRL_SET_CHKBOX_DISABLED_BKGR_COLOR, 0, ::GetSysColor(COLOR_3DFACE));
		}
		else
		{
			// Reset the normal background color.
			short rc_run = g_ControlPanelRunCheckBoxes[m_stt_chkbox];
			if (GetCheckMark(rc_run) == TRUE)
			{
				::SendDlgItemMessage(m_hDlg, rc_run, DLG_CTRL_SET_CHKBOX_BKGR_COLOR, 0, VPCTRL_CHK_BOX_BKGR_NORMAL);
			}
			else
			{
				short rc_step = g_ControlPanelStepCheckBoxes[m_stt_chkbox];
				::SendDlgItemMessage(m_hDlg, rc_step, DLG_CTRL_SET_CHKBOX_BKGR_COLOR, 0, VPCTRL_CHK_BOX_BKGR_NORMAL);
			}
		}
	}

	// Setup backgound of the new check box.
	if (stt_chk_box != pcev_none)
	{
		if (stt_chk_box == pcev_parsing_complete)
		{
			// Setup disabled background color in both check boxes.
			::SendDlgItemMessage(m_hDlg, IDC_VPCTRL_CHK_COMPLETE_RUN, DLG_CTRL_SET_CHKBOX_DISABLED_BKGR_COLOR, 0, RGB(201, 168, 148));
			::SendDlgItemMessage(m_hDlg, IDC_VPCTRL_CHK_COMPLETE_STEP, DLG_CTRL_SET_CHKBOX_DISABLED_BKGR_COLOR, 0, RGB(201, 168, 148));
		}
		else
		{
			// Setup normal background color.
			short rc_run = g_ControlPanelRunCheckBoxes[stt_chk_box];
			if (GetCheckMark(rc_run) == TRUE)
			{
				::SendDlgItemMessage(m_hDlg, rc_run, DLG_CTRL_SET_CHKBOX_BKGR_COLOR, 0, VPCTRL_CHK_BOX_BKGR_HGTLT);
			}
			else
			{
				short rc_step = g_ControlPanelStepCheckBoxes[stt_chk_box];
				::SendDlgItemMessage(m_hDlg, rc_step, DLG_CTRL_SET_CHKBOX_BKGR_COLOR, 0, VPCTRL_CHK_BOX_BKGR_HGTLT);
			}
		}
	}

	// Store the new current check box value.
	m_stt_chkbox = stt_chk_box;
}

// ------------------------------------------------------------------------
//   ============  TLangViewVisualParserSrcBreakPoints  ================
// ------------------------------------------------------------------------

static const TControlPosInfo g_VisualParserSrcBreakPointsControls[] =
{
	IDC_VPSRC_BRKPNT_GRPBOX,		cpm_fix_all_corners,
	IDC_VPSRC_BRKPNT_TITLE,			cpm_fix_top_side,
	IDC_VPSRC_BRKPNT_LISTBOX,		cpm_fix_all_corners,
	IDC_VPSRC_BRKPNT_BTN_GOTO,	cpm_fix_left_bottom,
	IDC_VPSRC_BRKPNT_BTN_ADD,		cpm_fix_right_bottom,
	IDC_VPSRC_BRKPNT_BTN_EDIT,		cpm_fix_right_bottom,
	IDC_VPSRC_BRKPNT_BTN_DEL,		cpm_fix_right_bottom,
	0, cpm_none,
};

static TListBoxColumnProps g_VisualParserSrcBreakPointsColumnsInfo[] =
{
	{ align_center,	2,	    28,	   0,		 4,	},		// NN
	{ align_left,		0,	    63,	  90,		 4,	},		// File
	{ align_right,		0,	    42,	  20,		 6,	},		// Line
	{ align_right,		0,	    35,	   5,		 4,	},		// Char
	{ align_right,		0,	    28,	   5,		 8,	},		// Hits
	LBX_CLMNP_EOF
};

static TListBoxColumnTilteProps g_VisualParserSrcBreakPointsTitleInfo[] =
{
	{ 1,	 FALSE,	L"NN"		},
	{ 1,	 FALSE,	L"File name"	},
	{ 1,	 FALSE,	L"Line"		},
	{ 1,	 FALSE,	L"Char"		},
	{ 1,	 FALSE,	L"Hits"		},
	{ 0,	 FALSE,	NULL		},	// EOF record.
};

TLangViewVisualParserSrcBreakPoints::TLangViewVisualParserSrcBreakPoints()
		: TBasicDialog(NULL, IDD_LANGV_VPARS_SRC_BRK_POINTS)
{
	m_owner = NULL;
}

LRESULT TLangViewVisualParserSrcBreakPoints::OnInitDialog(HWND hDefFocusControl)
{
	// Prepare control fonts and resizing.
	SetControlFont(IDC_VPSRC_BRKPNT_BTN_GOTO, TAHOMA, 7);
	SetControlFont(IDC_VPSRC_BRKPNT_BTN_ADD, TAHOMA, 7);
	SetControlFont(IDC_VPSRC_BRKPNT_BTN_EDIT, TAHOMA, 7);
	SetControlFont(IDC_VPSRC_BRKPNT_BTN_DEL, TAHOMA, 7);
	SetupControlsListPositioning(g_VisualParserSrcBreakPointsControls);
	SetMinTrackingSize();

	// Prepare listbox.
	m_list_hlp.SetWindow(::GetDlgItem(m_hDlg, IDC_VPSRC_BRKPNT_LISTBOX));
	m_list_hlp.SetTitleWindow(::GetDlgItem(m_hDlg, IDC_VPSRC_BRKPNT_TITLE));
	m_list_hlp.SetupSystemStdStyles();
	m_list_hlp.SetupColumnProps(g_VisualParserSrcBreakPointsColumnsInfo);
	m_list_hlp.SetupColumnTitles(g_VisualParserSrcBreakPointsTitleInfo);

	// Widows should not set focus anywhere.
	return(FALSE);
}

void TLangViewVisualParserSrcBreakPoints::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_VPSRC_BRKPNT_BTN_GOTO:
		case IDC_VPSRC_BRKPNT_BTN_ADD:
		case IDC_VPSRC_BRKPNT_BTN_EDIT:
		case IDC_VPSRC_BRKPNT_BTN_DEL:
				{
					SetFocusControl(IDC_VPSRC_BRKPNT_LISTBOX);
					ControlStyleBitsAnd(controlId, BS_DEFPUSHBUTTON, TRUE);
				}
				break;
	}
}

void TLangViewVisualParserSrcBreakPoints::UpdateLowerFrameOffset(long lower_frame_offs)
{
	assert(m_hDlg != NULL);

	RECT rc_dlg, rc_frame;
	::GetWindowRect(m_hDlg, &rc_dlg);
	GetControlRect(IDC_VPSRC_BRKPNT_GRPBOX, rc_frame);

	long shift_value = (rc_dlg.bottom-rc_dlg.top-rc_frame.bottom)-lower_frame_offs;
	if (shift_value != 0)
	{
		SetControlSize(IDC_VPSRC_BRKPNT_GRPBOX, rc_frame.right-rc_frame.left, rc_frame.bottom-rc_frame.top+shift_value);
		ShiftControl(IDC_VPSRC_BRKPNT_BTN_GOTO, 0, shift_value);
		ShiftControl(IDC_VPSRC_BRKPNT_BTN_ADD, 0, shift_value);
		ShiftControl(IDC_VPSRC_BRKPNT_BTN_EDIT, 0, shift_value);
		ShiftControl(IDC_VPSRC_BRKPNT_BTN_DEL, 0, shift_value);
		SetupControlsListPositioning(g_VisualParserSrcBreakPointsControls);
	}
}

// -------------------------------------------------------------------------------
//   ================  TLangViewAbstractSyntaxTreeViewer  ==================
// -------------------------------------------------------------------------------

static const TControlPosInfo g_SyntaxTreeViewerCtrls[] =
{
	IDC_SYNTR_LABEL,			cpm_fix_top_side,
	IDC_SYNTR_TREE_VIEW,		cpm_fix_all_corners,
	IDC_SYNTR_TOTAL_LAB,		cpm_fix_left_bottom,
	IDC_SYNTR_TOTAL_VAL,		cpm_fix_left_bottom,
	IDC_SYNTR_CURR_ZOOM,		cpm_fix_right_bottom,
	0, cpm_none,
};

TLangViewAbstractSyntaxTreeViewer::TLangViewAbstractSyntaxTreeViewer()
		: TLangViewChildBrowser(NULL, cbrt_vpars_syntax_tree, IDD_LANGV_VPARS_SYNTAX_TREE)
{
	m_null.SetOwnedFlag(FALSE);
	m_null.SetParserNodeType(vplit_null_item);
	m_pars_stt = 0xFFFF;

	m_view_mode  = plc_mode_concentric;
	m_view_style = NULL;
	m_bkgr_color = RGB(255, 255, 255);
}

LRESULT TLangViewAbstractSyntaxTreeViewer::OnInitDialog(HWND hDefFocusControl)
{
	// Prepare control fonts and resizing.
	::GetWindowRect(m_hDlg, &m_original_rect);
	SetControlFont(IDC_SYNTR_LABEL, VERDANA, 8);
	AdjustControlRightSide(IDC_SYNTR_TREE_VIEW);
	SetupControlsListPositioning(g_SyntaxTreeViewerCtrls);

	// Prepare the screen items control.
	m_view_hlpr.SetWindow(::GetDlgItem(m_hDlg, IDC_SYNTR_TREE_VIEW));
	SetupBkgrColor(m_bkgr_color, FALSE);

	// Prepare the rest of controls.
	SetControlText(IDC_SYNTR_LABEL, NULL);
	SetControlText(IDC_SYNTR_TOTAL_VAL, NULL);

	// Show the current state of the viewer whatever it is.
	PrepareAndShowScreenItems();

	// Windows should not set the focus anywhere.
	return(FALSE);
}

void TLangViewAbstractSyntaxTreeViewer::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_SYNTR_TREE_VIEW:
				{
					// Notification came from the screen items view.
					if (notificationCode == xiv_right_btn_down)
					{
						TScreenItemsViewMouseNotify *mnti = (TScreenItemsViewMouseNotify*)hWndControl;
						if (mnti->object != NULL)
						{
							// This is the right click on some screem item with non NULL owner. All owned screen items
							// of this viewer should be owned only by their LexItem.
							TLangViewLexItem *owner_object = (TLangViewLexItem*)mnti->object->GetOwner();

							TMenuItemInfo menu_info[7];
							wchar_t  sporg_descr_buffer[80], buff_irule[80], buff_lin_area[80];
							int cnt_items = 0;

							if (owner_object->GetItemType() == litp_lexema)
							{
								TLangViewLexemaLexItem *lexema = (TLangViewLexemaLexItem*)owner_object;
								assert(lexema->m_grammar_object == FALSE);
								wchar_t *sporg_descr = PrepareSpecialOriginDescription(sporg_descr_buffer, 80, lexema->m_shift_dest_state, lexema->m_spec_origin);
								if (sporg_descr != NULL)
									menu_info[cnt_items++].Setup(100, sporg_descr);

								swprintf(buff_lin_area, 80, L"LinearBeg: %ld, LinearLen: %ld.", lexema->m_lex_info.origin.lin_area.linear_beg, lexema->m_lex_info.origin.lin_area.linear_len);
								menu_info[cnt_items++].Setup(101, buff_lin_area);
							}
							else if (owner_object->GetItemType() == litp_non_terminal)
							{
								TLangViewNonTerminalLexItem *non_term = (TLangViewNonTerminalLexItem*)owner_object;
								assert(non_term->m_grammar_object == FALSE);
								wchar_t *sporg_descr = PrepareSpecialOriginDescription(sporg_descr_buffer, 80, actb_reduce+non_term->m_rule_index, non_term->m_spec_origin);
								if (sporg_descr != NULL)
									menu_info[cnt_items++].Setup(100, sporg_descr);

								swprintf(buff_irule, 80, L"Non terminal was created using R%d.", non_term->m_rule_index);
								menu_info[cnt_items++].Setup(101, buff_irule);

								swprintf(buff_lin_area, 80, L"LinearBeg: %ld, LinearLen: %ld.", non_term->m_origin.lin_area.linear_beg, non_term->m_origin.lin_area.linear_len);
								menu_info[cnt_items++].Setup(101, buff_lin_area);
							}

							if (cnt_items > 0)
							{
								// Just display the menu. The response is not expected.
								menu_info[cnt_items].SetEndOfMenu();
								m_view_hlpr.ShowAndTrackPopupMenu(menu_info, mnti->click_place.x, mnti->click_place.y);
							}
						}
						else
						{
							// Show the background menu.
							ProcessBrgrContextMenu(mnti->click_place.x, mnti->click_place.y);
						}
					}
					else if (notificationCode == xiv_corner_clicked)
					{
						// User has clicked the right bottom corner.
						TScreenItemsViewNotify *nti = (TScreenItemsViewNotify*)hWndControl;
						if (nti->right_btn == TRUE)
						{
							ProcessBrgrContextMenu(nti->pos_x, nti->pos_y);
						}
					}
				}
				break;
	}
}

void TLangViewAbstractSyntaxTreeViewer::ResetContents()
{
	// (1) Reset bkgr setting.
	SetupBkgrColor(RGB(255, 255, 255), FALSE);

	// (2) Reset the parsing state if any.
	m_pars_stt_scr_item.SetStyle(NULL);
	m_pars_stt = 0xFFFF;

	// (3) Reset style in upper picture and upper labels. This will indicate that they are not inited.
	m_upper_picture.SetStyle(NULL);
	for (int i=0; i<NUM_UPPER_LABELS; ++i)
		m_upper_labels[i].SetStyle(NULL);

	// (4) Cancel previous subtree if any and clear view style to indicate that tree is not set up.
	m_root.ReleaseChildren();
	m_view_style = NULL;

	// Show empty screen items viewer.
	m_view_hlpr.ClearInternalDataSet();
	m_view_hlpr.ShowInternalDataSet(TRUE);

	// (5) Reset zoom and tree placing style.
	ShowZoom(1.0);
	m_view_mode = plc_mode_concentric;

	// Clear the static controls.
	SetControlText(IDC_SYNTR_LABEL, NULL);
	SetControlText(IDC_SYNTR_TOTAL_VAL, NULL);
}

void TLangViewAbstractSyntaxTreeViewer::SetSelection(TLangViewClickEvent &event, bool ext_margins)
{
	// This viewer is not selecting anything.
	assert(FALSE);
}

void TLangViewAbstractSyntaxTreeViewer::ResetSelection()
{
	// Selection cannot be removed because nothing can be selected.
	assert(FALSE);
}

void TLangViewAbstractSyntaxTreeViewer::SetupBkgrColor(TColor bkgr_color, bool paint_now)
{
	TScreenItemsViewFrameProps props;
	if (m_view_mode == plc_mode_concentric)
	{
		props.SetStndScrollableProps(bkgr_color, concentric_margin_x, concentric_margin_y);
	}
	else
	{
		props.SetStndScrollableProps(bkgr_color, pcb_margin_x, pcb_margin_y);
	}

	props.center_small_data_set_y = FALSE;
	m_view_hlpr.SetProperties(props, paint_now);
	m_bkgr_color = bkgr_color;

	if (paint_now == TRUE)
		PrepareAndShowScreenItems();
}

void TLangViewAbstractSyntaxTreeViewer::SetupUpperLabel(int lab_index, TTextStyle *style, const wchar_t *message, bool paint_now)
{
	assert(lab_index >= 0 && lab_index < NUM_UPPER_LABELS);

	m_upper_labels[lab_index].SetStyle(style);
	m_upper_labels[lab_index].SetText(message);

	if (paint_now == TRUE)
		PrepareAndShowScreenItems();
}

void TLangViewAbstractSyntaxTreeViewer::SetupUpperPicture(TBitmapStyle *style, short icon_index, bool paint_now)
{
	m_upper_picture.SetStyle(style);
	m_upper_picture.SetIconIndex(icon_index);

	if (paint_now == TRUE)
		PrepareAndShowScreenItems();
}

void TLangViewAbstractSyntaxTreeViewer::SetupParsingState(WORD pars_stt, TTextStyle *style, bool paint_now)
{
	m_pars_stt_scr_item.SetStyle(style);
	m_pars_stt_scr_item.SetTextFmt(L"%hd", pars_stt);
	m_pars_stt = pars_stt;

	if (paint_now == TRUE)
		PrepareAndShowScreenItems();
}

void TLangViewAbstractSyntaxTreeViewer::SetupTree(TTreeItemsPlacingMode view_mode, TLangViewLexItemStyle *view_style,
											TVisualParserLexItemType root_type, WORD root_inx, TLangViewTreeNode *subtree,
											bool clone_subtree, bool paint_now)
{
	assert(view_style != NULL);

	// Cancel the previous subtree if any.
	m_root.ReleaseChildren();

	// Save style params for future use.
	if (view_mode != -1)
		m_view_mode = view_mode;
	m_view_style = view_style;

	// Prepare the root object for display.
	m_root.SetParserNodeType(root_type);
	m_root.SetParserSlotIndex(root_inx);

	if (subtree != NULL)
	{
		// Adopt the passed subtree.
		if (clone_subtree == TRUE)
		{
			subtree = subtree->Clone(TRUE);
		}

		if (subtree != NULL)
		{
			// Non empty subtree is passed and if clone was requested, it also succeeded.
			m_root.AdoptChild(subtree);
		}
		else
		{
			// Show "null" item that is a data field of the dialog.
			m_root.AdoptChild(&m_null);
		}
	}

	if (paint_now == TRUE)
	{
		// Place and display the info.
		PrepareAndShowScreenItems();
	}
}

void TLangViewAbstractSyntaxTreeViewer::UpdateUpperLabelStyle(int lab_index, TTextStyle *style, bool paint_now)
{
	assert(lab_index >= 0 && lab_index < NUM_UPPER_LABELS);

	if (m_upper_labels[lab_index].GetBasicStyle() != NULL)
	{
		// Current label is set up.
		m_upper_labels[lab_index].SetStyle(style);
		if (paint_now == TRUE)
		{
			// Rerun the placing.
			PrepareAndShowScreenItems();
		}
	}
}

void TLangViewAbstractSyntaxTreeViewer::UpdateLexItemsStyle(TLangViewLexItemStyle *view_style, bool paint_now)
{
	assert(view_style != NULL);
	m_view_style = view_style;

	if (paint_now == TRUE)
	{
		// Rerun the placing.
		PrepareAndShowScreenItems();
	}
}

void TLangViewAbstractSyntaxTreeViewer::UpdateRootIndex(WORD new_root_inx, bool paint_now)
{
	assert(m_view_style != NULL);
	m_root.SetParserSlotIndex(new_root_inx);

	if (paint_now == TRUE)
	{
		// Rerun the placing.
		PrepareAndShowScreenItems();
	}
}

void TLangViewAbstractSyntaxTreeViewer::PrepareAndShowScreenItems(TGenericConsoleEx *cons)
{
	// Check if the dialog is opened.
	if (m_hDlg == NULL)
		return;

	// Dialog is present. Start from empty list of screen items.
	m_view_hlpr.ClearInternalDataSet();
	HDC hDC = ::GetDC(m_hDlg);
	assert(hDC != NULL);

	long pstt_pos_x = -10, pstt_pos_y = 0;
	long upper_pos_x = 0, upper_pos_y = -10;

	// Calculate height of the objects above the tree.
	long height_above = 0, upper_part_of_root = 0;

	// Check if tree should be displayed or not.
	SetControlText(IDC_SYNTR_LABEL, NULL);
	SetControlText(IDC_SYNTR_TOTAL_VAL, NULL);
	if (m_view_style != NULL)
	{
		// Prepare the tree and ref positions for other screen items.
		int num_visibs;
		switch (m_view_mode)
		{
			case plc_mode_concentric:
					{
						TConcentricModeTreeGraphPlacer::DisplayGraph(m_view_hlpr, m_view_hlpr.InternalDataSet(),
											&m_root, TLangViewLexItemStyle::g_DefaultConcentricPlacingProps,
											(void**)&m_view_style, 0, &num_visibs, FALSE, cons);

						pstt_pos_x -= m_root.GetWidth()/2;
						pstt_pos_y -= 7;
						upper_part_of_root = m_root.GetHeight()/2;
						upper_pos_y -= upper_part_of_root;
					}
					break;

			case plc_mode_rect_lines:
					{
						TRectLinesModeTreeGraphPlacer::DisplayGraph(m_view_hlpr, m_view_hlpr.InternalDataSet(),
											&m_root, TLangViewLexItemStyle::g_DefaultRectLinesPlacingProps,
											(void**)&m_view_style, 0, &num_visibs, FALSE, cons);
						pstt_pos_x -= 4;
						pstt_pos_y += 13;
						upper_pos_x += m_root.GetWidth()/2;
						upper_part_of_root = m_root.GetHeight()/4;
						upper_pos_y -= upper_part_of_root;
					}
					break;

			default:
				assert(FALSE);
				break;
		}

		// Prepare root symbol name and value.
		wchar_t buff_20_chars[20], buff_40_chars[40];
		wchar_t *symbol_name = L"";
		wchar_t *symbol_value = L"n/a";

		TLangViewTreeNode *pri_child = m_root.GetLowerLeftChild();
		WORD pri_child_sym = (pri_child != NULL) ? pri_child->GetSymbolValue() : 0xFFFF;

		if (pri_child_sym != 0xFFFF)
		{
			// Symbol item is present in the tree.
			swprintf(buff_20_chars, 20, L"%hd", pri_child_sym);
			symbol_value = buff_20_chars;

			if (m_view_style->GetGrammar() != NULL)
			{
				// Name of the symbol can be picked up.
				symbol_name = m_view_style->GetGrammar()->GetSymbolName(pri_child_sym, buff_40_chars);
			}
		}

		// Setup the title.
		if (GetRootType() == vplit_parser_stk_slot)
			SetControlTextFmt(IDC_SYNTR_LABEL, L"Stk %d: %s (%s)", GetRootIndex(), symbol_name, symbol_value);
		else if (GetRootType() == vplit_parser_inp_sym)
			SetControlTextFmt(IDC_SYNTR_LABEL, L"Sym %d: %s (%s)", GetRootIndex(), symbol_name, symbol_value);

		// Number of visible items should not include the root item.
		SetControlTextFmt(IDC_SYNTR_TOTAL_VAL, L"%d", num_visibs-1);
	}

	if (m_pars_stt_scr_item.GetBasicStyle() != NULL)
	{
		// Parsing state screen item is visible.
		m_pars_stt_scr_item.PlaceAroundCenter(hDC, pstt_pos_x, pstt_pos_y);
		m_pars_stt_scr_item.ShiftItem(-m_pars_stt_scr_item.GetWidth()/2, 0);
		m_view_hlpr.AddToInternalDataSet(m_pars_stt_scr_item);
	}

	if (m_upper_picture.GetBasicStyle() != NULL)
	{
		// Upper pict screen item is visible.
		m_upper_picture.SetPosition(upper_pos_x, upper_pos_y);
		long height = m_upper_picture.GetHeight();
		m_upper_picture.ShiftItem(0, -height/2);
		upper_pos_y -= height+10;
		height_above = -m_upper_picture.GetBoundingRect()->top;
		m_view_hlpr.AddToInternalDataSet(m_upper_picture);
	}

	// Check and place the upper messages.
	for (int i=NUM_UPPER_LABELS-1; i >= 0; --i)
	{
		if (m_upper_labels[i].GetBasicStyle() != NULL)
		{
			// Current label is visible.
			m_upper_labels[i].PlaceAroundCenter(hDC, upper_pos_x, upper_pos_y);
			long height = m_upper_labels[i].GetHeight();
			m_upper_labels[i].ShiftItem(0, -height/2);
			upper_pos_y -= height;
			height_above = -m_upper_labels[i].GetBoundingRect()->top;
			m_view_hlpr.AddToInternalDataSet(m_upper_labels[i]);
		}
	}

	// Check the value of the upper margin.
	TScreenItemsViewFrameProps props;
	m_view_hlpr.GetCurrentFrameProps(props);
	long margin_y = (m_view_mode == plc_mode_concentric) ? concentric_margin_y : pcb_margin_y;
	if (height_above > 0)
	{
		long offs = margin_y-(height_above-upper_part_of_root);
		if (offs < margin_y/3)
			margin_y = margin_y/3;
		else
			margin_y = offs;
	}

	if (margin_y != props.vert_margin)
	{
		// Update the vert margin.
		props.vert_margin = (short)margin_y;
		m_view_hlpr.SetProperties(props, FALSE);
	}

	// Show the resulting data set.
	::ReleaseDC(m_hDlg, hDC);
	m_view_hlpr.ShowInternalDataSet(TRUE);
}

void TLangViewAbstractSyntaxTreeViewer::OnWmSize()
{
	// Pick up the current height of the screen items view control.
	long sz_x, sz_y;
	GetControlSize(IDC_SYNTR_TREE_VIEW, sz_x, sz_y);

	// Show/hide the number of items in the screen items vew.
	bool bShowLabels = (sz_y > 0) ? TRUE : FALSE;
	ShowControl(IDC_SYNTR_TOTAL_LAB, bShowLabels);
	ShowControl(IDC_SYNTR_TOTAL_VAL, bShowLabels);

	// Determine size of the space between lower labels and lower buttons.
	bool bShowButtons = FALSE;
	if (bShowLabels == TRUE)
	{
		// Determine size of the space between lower labels and lower buttons.
		RECT rc_total_val, rc_zoom_in;
		GetControlRect(IDC_SYNTR_TOTAL_VAL, rc_total_val);
		GetControlRect(IDC_SYNTR_CURR_ZOOM, rc_zoom_in);

		if (rc_total_val.right+12 < rc_zoom_in.left)
			bShowButtons = TRUE;
	}

	// Show/hide lower right group of buttons.
	ShowControl(IDC_SYNTR_CURR_ZOOM, bShowButtons);
}

void TLangViewAbstractSyntaxTreeViewer::ShowZoom(float new_zoom)
{
	SetControlTextFmt(IDC_SYNTR_CURR_ZOOM, L"Current zoom: %.4f", new_zoom);
}

void TLangViewAbstractSyntaxTreeViewer::ProcessBrgrContextMenu(long click_pos_x, long click_pos_y)
{
	// Prepare and show the popup menu.
	TScreenItemsViewSpaceInfo space_info;
	m_view_hlpr.GetSpaceInfo(space_info);
	float old_zoom = space_info.zoom_level;

	TLangViewChildBrowser::TSIVMR sivmr = ProcessScreenItemsViewControlMenu(m_view_hlpr, m_view_mode, click_pos_x, click_pos_y);
	if (sivmr == TLangViewChildBrowser::sivmr_reload_view)
	{
		// Resetup and repaint the screen items view.
		SetupBkgrColor(m_bkgr_color, FALSE);
		PrepareAndShowScreenItems();
	}
	else if (sivmr == TLangViewChildBrowser::sivmr_copy_to_clipborad)
	{
		// Make a screen shot of the current AST.
		RECT margins = { 40, 40, 40, 40 };
		PutScreenShotToClipboard(m_view_hlpr, &margins);
	}

	m_view_hlpr.GetSpaceInfo(space_info);
	if (space_info.zoom_level != old_zoom)
		ShowZoom(space_info.zoom_level);
}

wchar_t *TLangViewAbstractSyntaxTreeViewer::PrepareSpecialOriginDescription(wchar_t *buffer, int buff_len, WORD final_action, TParsingActionOriginInfo &info)
{
	switch (TLangViewLexItem::GetSpecOriginFromSpecOriginInfo(final_action, info))
	{
		case sporg_none:		return(NULL);
		case sporg_cooked:	return(L"Parsing action was changed by the app callback.");

		case sporg_resolved_conflict:
				swprintf(buffer, buff_len, L"Grammar conflict C%hd was resolved by the app callback.", info.initial_action-actb_conflict);
				return(buffer);

		case sporg_nested_parsing:
				swprintf(buffer, buff_len, L"Grammar conflict C%hd was resolved by the nested parsing.", info.conflict_action-actb_conflict);
				return(buffer);

		case sporg_resolved_nres:
				swprintf(buffer, buff_len, L"Processing of C%hd generated NR%hd, that was resolved by the app callback.", info.conflict_action-actb_conflict, info.initial_action-actb_conflict);
				return(buffer);
	}

	assert(FALSE);
	return(NULL);
}

// ------------------------------------------------------------------------
//   ===========  TLangViewVisualParserCurrStatePanel  =================
// ------------------------------------------------------------------------

static const TControlPosInfo g_CurrStatePanelCtrls[] =
{
	IDC_VPCSTT_LABEL,		cpm_fix_top_side,
	IDC_VPCSTT_SVIEW,		cpm_fix_all_corners,
	0, cpm_none,
};

static TTextStyleProps g_CurrStatePanelTextStyleProps[3] =
{
	// "Curr" and "State" labels.
	{ NULL, VERDANA,  12, TRUE, RGB(84, 84, 84), STD_TXT },

	// Curr state value (the number in the rect).
	{ NULL, TAHOMA, 14, TRUE, RGB(0, 0, 0), RGB(181, 235, 255), STD_NO_FRSH, 1, 0, 2, 10, 10 },

	// Style for the "(final)" label.
	{ NULL, VERDANA,  8, TRUE, RGB(84, 84, 84), STD_TXT },
};

TLangViewVisualParserCurrStatePanel::TLangViewVisualParserCurrStatePanel()
		: TBasicDialog(NULL, IDD_LANGV_VPARS_CURRENT_STATE)
{
	m_curr_state = 0;
	m_final_state = FALSE;
}

LRESULT TLangViewVisualParserCurrStatePanel::OnInitDialog(HWND hDefFocusControl)
{
	// Prepare resizing.
	::GetWindowRect(m_hDlg, &m_original_rect);
	AdjustControlRightSide(IDC_VPCSTT_SVIEW);
	SetupControlsListPositioning(g_CurrStatePanelCtrls);
	SetControlFont(IDC_VPCSTT_LABEL, VERDANA, 8);

	// Prepare styles.
	m_label_style.Setup(g_CurrStatePanelTextStyleProps[0]);
	if (m_label_style.SetupForDeviceContextEx(m_hDlg) == FALSE)
		return(-1);

	m_curr_state_style.Setup(g_CurrStatePanelTextStyleProps[1]);
	m_curr_state_style.SetupFrameProps(RGB(104, 134, 215), fcs_1pix_add_inner_dot);
	if (m_curr_state_style.SetupForDeviceContextEx(m_hDlg) == FALSE)
		return(-1);

	m_final_style.Setup(g_CurrStatePanelTextStyleProps[2]);
	if (m_final_style.SetupForDeviceContextEx(m_hDlg) == FALSE)
		return(-1);

	m_delimiter_style1.Setup(RGB(170, 170, 170));
	m_delimiter_style2.Setup(RGB(116, 116, 116));

	// Prepare labels.
	m_label_scr_item[0].SetStyle(&m_label_style);
	m_label_scr_item[1].SetStyle(&m_label_style);
	m_label_scr_item[2].SetStyle(&m_final_style);
	m_label_scr_item[0].SetText(L"Curr");
	m_label_scr_item[1].SetText(L"State");
	m_label_scr_item[2].SetText(L"(final)");
	m_curr_state_scr_item.SetStyle(&m_curr_state_style);
	m_curr_state_scr_item.SetTextFmt(L"%hu", m_curr_state);

	// Prepare the screen items control.
	m_view_hlpr.SetWindow(::GetDlgItem(GetWnd(), IDC_VPCSTT_SVIEW));

	// Show the current state.
	PrepareAndShowContents();

	// Widows should not set focus anywhere.
	return(FALSE);
}

void TLangViewVisualParserCurrStatePanel::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	// There is nothing to process on this panel so far.
	// This panel cannot be even closed.
}

void TLangViewVisualParserCurrStatePanel::ResetContents()
{
	m_curr_state = 0;
	m_curr_state_scr_item.SetTextFmt(L"%hu", m_curr_state);
	m_final_state = FALSE;

	for (int inx=0; inx<NUM_LOWER_COMMENTS; ++inx)
		m_comment_scr_items[inx].SetStyle(NULL);

	// Show the current state.
	PrepareAndShowContents();
}

void TLangViewVisualParserCurrStatePanel::SetupParsingState(WORD new_state, bool final_state, bool paint_now)
{
	if (new_state != m_curr_state)
	{
		m_curr_state = new_state;
		m_curr_state_scr_item.SetTextFmt(L"%hu", m_curr_state);
	}

	if (final_state != m_final_state)
	{
		m_final_state = final_state;
		SetupBackgroundColor(FALSE);
	}

	if (paint_now == TRUE)
		PrepareAndShowContents();
}

void TLangViewVisualParserCurrStatePanel::SetupLowerComment(int comment_index, TTextStyle *style, const wchar_t *message, bool paint_now)
{
	assert(comment_index >= 0 && comment_index < NUM_LOWER_COMMENTS);

	m_comment_scr_items[comment_index].SetStyle(style);
	m_comment_scr_items[comment_index].SetText(message);

	if (paint_now == TRUE)
		PrepareAndShowContents();
}

void TLangViewVisualParserCurrStatePanel::PrepareAndShowContents()
{
	// Check if dialog is opened.
	if (m_hDlg == NULL)
		return;

	// Dialog is present. Show current parsing state on the static text above the screen items view.
	SetControlTextFmt(IDC_VPCSTT_LABEL, L"Stt: %hu", m_curr_state);

	// Start screen items preparation from empty list.
	m_view_hlpr.ClearInternalDataSet();
	SetupBackgroundColor(FALSE);
	HDC hDC = ::GetDC(m_hDlg);
	assert(hDC != NULL);

	long pos_y = 0;

	// Show the upper labels and curr parsing state.
	MapTextScreenItem(hDC, m_label_scr_item[0], pos_y);
	pos_y -= 2;
	MapTextScreenItem(hDC, m_label_scr_item[1], pos_y);
	pos_y += 7;
	MapTextScreenItem(hDC, m_curr_state_scr_item, pos_y);
	if (m_final_state == TRUE)
	{
		pos_y += 7;
		MapTextScreenItem(hDC, m_label_scr_item[2], pos_y);
	}

	// Prepare the picture.
	pos_y += 10;
	long half_width = 33;
	for (int i=0; i<10 && half_width > 0; ++i, half_width -= 10)
	{
		m_delimiter_scr_item[i].SetStyle((m_final_state == FALSE) ? &m_delimiter_style1 : &m_delimiter_style2);
		RECT rc = { -half_width, pos_y, half_width, pos_y+2 };
		m_delimiter_scr_item[i].SetBaseRect(rc);
		m_view_hlpr.AddToInternalDataSet(m_delimiter_scr_item[i]);
		pos_y += 7;
	}

	// Check and place the comment messages.
	pos_y += 10;
	for (int i=0; i<NUM_LOWER_COMMENTS; ++i)
	{
		if (m_comment_scr_items[i].GetBasicStyle() != NULL)
		{
			// Current comment is visible.
			MapTextScreenItem(hDC, m_comment_scr_items[i], pos_y);
		}
	}

	// Show the resulting data set.
	::ReleaseDC(m_hDlg, hDC);
	m_view_hlpr.ShowInternalDataSet(TRUE);
}

void TLangViewVisualParserCurrStatePanel::OnWmSize()
{
	// Scroll screen items view only in horz direction.
	m_view_hlpr.ScrollDataSetIntoCenter(TRUE, FALSE);
}

void TLangViewVisualParserCurrStatePanel::SetupBackgroundColor(bool paint_now)
{
	// Pick up appropriate color first.
	TColor bkgr_color = (m_final_state == TRUE) ? RGB(255, 200, 192) : RGB(235, 254, 252);

	// Set it up into control and display changes if needed.
	TScreenItemsViewFrameProps props;
	props.SetStndScrollableProps(bkgr_color, 0, 14);
	props.use_horz_scroll = FALSE;
	props.use_vert_scroll = FALSE;
	props.center_small_data_set_y = FALSE;
	m_view_hlpr.SetProperties(props, paint_now);
}

void TLangViewVisualParserCurrStatePanel::MapTextScreenItem(HDC hDC, TTextItem &scr_item, long &pos_y)
{
	scr_item.PlaceAroundCenter(hDC, 0, pos_y);
	long height = scr_item.GetHeight();
	scr_item.ShiftItem(0, height/2);
	pos_y += height+1;
	m_view_hlpr.AddToInternalDataSet(scr_item);
}

// --------------------------------------------------------------------
//   ===========  TLangViewVisualParserSupportPanel  ===============
// --------------------------------------------------------------------

static const TControlPosInfo g_SupportPanelCtrls[] =
{
	IDC_VPSUPP_LABEL,	cpm_fix_top_side,
	IDC_VPSUPP_SVIEW,	cpm_fix_all_corners,
	0, cpm_none,
};

static TTextStyleProps g_SupportPanelTextStyleProps[2] =
{
	{ NULL, COMIC_SANS_MS,	12,	FALSE, RGB(180, 180, 180),	   STD_TXT },		// Header.
	{ NULL, TAHOMA,			10,	FALSE, RGB(160, 160, 160),	   STD_TXT },		// Message.
};

LRESULT TLangViewVisualParserSupportPanel::OnInitDialog(HWND hDefFocusControl)
{
	// Prepare resizing.
	::GetWindowRect(m_hDlg, &m_original_rect);
	SetControlFont(IDC_VPSUPP_LABEL, VERDANA, 8);
	AdjustControlRightSide(IDC_VPSUPP_SVIEW);
	SetupControlsListPositioning(g_SupportPanelCtrls);

	// Prepare styles.
	m_style1.Setup(g_SupportPanelTextStyleProps[0]);
	m_style2.Setup(g_SupportPanelTextStyleProps[1]);

	// Prepare messages.
	switch (GetDialogTemplateId())
	{
		case IDD_LANGV_VPARS_EMPTY_STACK:
				m_label1.SetText(L" Parser Stack");
				m_label2.SetText(L"(empty)");
				break;

		case IDD_LANGV_VPARS_EMPTY_INPUT:
				m_label1.SetText(L"Input Queue");
				m_label2.SetText(L"(empty)");
				break;

		default:
			assert(FALSE);
			return(-1);
	}

	// Prepare screen items control.
	m_view_hlpr.SetWindow(::GetDlgItem(GetWnd(), IDC_VPSUPP_SVIEW));
	SetupNestingLayerProps(m_nest_layer, m_bkgr_color);

	// Show initial state of the messages.
	HDC hDC = ::GetDC(m_hDlg);
	MapTextScreenItem(hDC, m_style1, m_label1, 0);
	MapTextScreenItem(hDC, m_style2, m_label2, 20);
	::ReleaseDC(m_hDlg, hDC);
	m_view_hlpr.ShowInternalDataSet(FALSE);

	// Widows should not set the focus anywhere.
	return(FALSE);
}

void TLangViewVisualParserSupportPanel::SetupNestingLayerProps(int nest_layer, TColor bkgr_clr)
{
	m_nest_layer = nest_layer;
	m_bkgr_color = bkgr_clr;

	if (m_nest_layer != 0)
		SetControlTextFmt(IDC_VPSUPP_LABEL, L"Layer: %d.", m_nest_layer);
	else SetControlText(IDC_VPSUPP_LABEL, NULL);

	TScreenItemsViewFrameProps props;
	props.SetStndScrollableProps(m_bkgr_color, 0, 37);
	props.use_horz_scroll = FALSE;
	props.use_vert_scroll = FALSE;
	props.center_small_data_set_y = FALSE;
	m_view_hlpr.SetProperties(props);
}

void TLangViewVisualParserSupportPanel::MapTextScreenItem(HDC hDC, TTextStyle &style, TTextItem &scr_item, long offs_y)
{
	// Setup  the text style.
	if (style.SetupForDeviceContext(hDC) == FALSE)
		return;

	// Setup text screen item.
	scr_item.SetStyle(&style);
	scr_item.PlaceHorzCentered(hDC, 0, offs_y);
	m_view_hlpr.AddToInternalDataSet(scr_item);
}

// ---------------------------------------------------------------------------
//   ==============  TLangViewParserAndGrammarInfo  ==================
// ---------------------------------------------------------------------------

static const TControlPosInfo g_VisualParserGrammarInfoCtrls[] =
{
	IDC_VPGRMI_PARS_DESCR,			cpm_fix_top_side,
	IDC_VPGRMI_GRMF_NAME,			cpm_fix_top_side,
	IDC_VPGRMI_BTN_GRM_VIEW,		cpm_fix_right_top,
	IDC_VPGRMI_GRMF_VIEW,			cpm_fix_all_corners,
	0,  cpm_none,
};

// File view props for displaying grammar definition source.
static TDecoratedFileViewStyleProps g_GrammarView_FileViewProps =
{
	NULL,

	44, 1,						// Nums width, marks width.
	8, 12, 7, 7,					// Text offsets: left, right, top, bottom.
	L"%d:", align_right, 4,			// Line numbers painting info.
	{ 0, 0 },						// Text area may have any size.
	{ 0, 0 },						// No min object size.

	RGB(248, 248, 248),			// Nums area background.
	RGB(167, 198, 248),			// Marks area background.
	RGB(255, 255, 255),			// Text area background.
	NULL, NULL, NULL,

	0, RGB(255, 255, 255),			// Shadow information.
	RGB(80, 0, 0),					// Mouse selection bkgr color.
	RGB(160, 200, 160),			// Mouse selection frgr color.
	2,							// Mouse selection horz ext.
};

static TTabsSpacesViewMode g_GrammarView_FileViewTabsMode =
{
	FALSE,			// Visible tabs.
	FALSE,			// Visible spaces.
	FALSE,			// Implicit multispace tabs.
	5,				// Number of char places in one tab.
	75, 120, 50,		// Widths of the tab symbol and char place that are used
					// in tab pos calculation in the percents of width of the basic
					// character in the default text style.
};

// Fonts for displaying the grammar definition source.
static TDecoratedFileViewTextProps g_GrammarView_FileViewTextProps =
{
	{ NULL,	COURIER_NEW,	8,	FALSE, RGB(80, 80, 80),	STD_TXT },		// Line num props.
	{ NULL,	COURIER_NEW,	9,	FALSE, RGB(0, 0, 0),		STD_TXT },		// Default text props.
	{ NULL,	ARIAL,			9,	TRUE,  RGB(60, 0, 0),		STD_TXT },		// Line overflow props.
};

TLangViewParserAndGrammarInfo::TLangViewParserAndGrammarInfo()
		: TBasicDialog(NULL, IDD_LANGV_VPARS_GRAMMAR_INFO),
		m_grammar_view(NULL, NULL)
{
	m_owner = NULL;
	m_curr_parser = NULL;
}

void TLangViewParserAndGrammarInfo::SetOwner(TLangViewVisualParserFrame *owner)
{
	m_owner = owner;
	m_grammar_view.SetDialogParams(owner);
}

void TLangViewParserAndGrammarInfo::SetDriver(TParsingDriver *driver)
{
	// Close the secondary viewer if it is opened.
	if (m_grammar_view.GetWnd() != NULL)
		::DestroyWindow(m_grammar_view.GetWnd());

	// Retrieve the parser out of the passed driver.
	if (driver != NULL)
		m_curr_parser = (TGrammarBasedParser*)&driver->Parser();
	else m_curr_parser = NULL;

	if (m_hDlg != NULL)
	{
		// Window needs to be updated.
		SetupControlContents();
	}
}

LRESULT TLangViewParserAndGrammarInfo::OnInitDialog(HWND hDefFocusControl)
{
	// Prepare resizing.
	AdjustControlRightSide(IDC_VPGRMI_GRMF_VIEW);
	SetupControlsListPositioning(g_VisualParserGrammarInfoCtrls);

	RECT dlg_rect, rc_btn;
	::GetClientRect(m_hDlg, &dlg_rect);
	GetControlRect(IDC_VPGRMI_BTN_GRM_VIEW, rc_btn);
	SetMinTrackingSize(rc_btn.right-rc_btn.left+2*(dlg_rect.right-rc_btn.right), 0);

	// Prepare the control fonts.
	SetControlFont(IDC_VPGRMI_BTN_GRM_VIEW, TAHOMA, 7);

	// Prepare styles.
	m_file_line_nums_style.Setup(g_GrammarView_FileViewTextProps.nums_props);
	m_file_default_text_style.Setup(g_GrammarView_FileViewTextProps.text_props);
	m_file_horz_ovfl_style.Setup(g_GrammarView_FileViewTextProps.ovfl_props);
	m_file_view_style.Setup(g_GrammarView_FileViewProps, g_GrammarView_FileViewTabsMode, FALSE,
							&m_file_line_nums_style, &m_file_default_text_style, &m_file_horz_ovfl_style);

	if (m_file_view_style.SetupForDeviceContextEx(m_hDlg) == FALSE)
		return(-1);

	// Prepare the screen items view control.
	m_view_hlpr.SetWindow(::GetDlgItem(GetWnd(), IDC_VPGRMI_GRMF_VIEW));
	m_view_hlpr.SetStndDocStyleProps(TRUE, TRUE);

	// Show info about current grammar.
	SetupControlContents();

	// Widows should not set focus anywhere.
	return(FALSE);
}

void TLangViewParserAndGrammarInfo::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_VPGRMI_BTN_GRM_VIEW:
				{
					UpdateButton(controlId);

					if (m_grammar_view.GetWnd() == NULL)
					{
						// Open the grammar viewer.
						m_grammar_view.SetupGrammar(m_curr_parser->RootGrammar(), NULL);
						m_grammar_view.OpenNonModalDialog(::GetParent(m_owner->GetWnd()));
					}
					else
					{
						// Bring the grammar rules viewer on top.
						if (::IsIconic(m_grammar_view.GetWnd()) == TRUE)
							::OpenIcon(m_grammar_view.GetWnd());
						::SetFocus(m_grammar_view.GetWnd());
					}
				}
				break;

		case IDC_VPGRMI_FPROBLEM_MSG:
				{
					// Message was queued because the window was not opened yet.
					if (m_source_loading_problem.grammar != NULL)
						ShowFileProblemMessage(&m_source_loading_problem, TRUE);
				}
				break;
	}
}

void TLangViewParserAndGrammarInfo::OnWmSize()
{
	// Show/hide the "Parser:" label.
	RECT rc_label, rc_btn;
	GetControlRect(IDC_VPGRMI_PARS_TITLE, rc_label);
	GetControlRect(IDC_VPGRMI_BTN_GRM_VIEW, rc_btn);
	ShowControl(IDC_VPGRMI_PARS_TITLE, (rc_label.right < rc_btn.left) ? TRUE : FALSE);

	if (m_file_view_scr_item.IsInited() == TRUE)
	{
		// Resetup the min size of file view item. Make it identical to the current size of the screen items view.
		TScreenItemsViewSpaceInfo space_info;
		m_view_hlpr.GetSpaceInfo(space_info);
		m_file_view_scr_item.SetupMinBoundingRectSize(space_info.client_size.cx, space_info.client_size.cy);
		m_view_hlpr.ShowInternalDataSet();
	}
}

void TLangViewParserAndGrammarInfo::UpdateButton(int ctrl_id)
{
	SetFocusControl(IDC_VPGRMI_GRMF_VIEW);
	ControlStyleBitsAnd(ctrl_id, BS_DEFPUSHBUTTON, TRUE);
}

void TLangViewParserAndGrammarInfo::SetupControlContents()
{
	// Remove the source file of the grammar if any.
	m_view_hlpr.ClearInternalDataSet();
	m_file_view_scr_item.Reset();

	// Show callback handler description.
	if (m_curr_parser != NULL)
	{
		EnableControl(IDC_VPGRMI_PARS_TITLE, TRUE);

		wchar_t buff80[80];
		const wchar_t *handler_name = m_curr_parser->GetParserDescription(buff80);

		if (handler_name != NULL && handler_name[0] != 0)
			SetControlText(IDC_VPGRMI_PARS_DESCR, handler_name);
		else SetControlText(IDC_VPGRMI_PARS_DESCR, L"no description");
	}
	else
	{
		EnableControl(IDC_VPGRMI_PARS_TITLE, FALSE);
		SetControlText(IDC_VPGRMI_PARS_DESCR, NULL);
	}

	// Check the grammar.
	TGrammar *grammar = (m_curr_parser != NULL) ? m_curr_parser->RootGrammar() : NULL;
	if (grammar == NULL || grammar->grm_props.processing_result == grpr_none)
	{
		// Grammar is not set or it is empty. Clear and disable all grammar related controls.
		EnableControl(IDC_VPGRMI_GRMF_TITLE, FALSE);
		SetControlText(IDC_VPGRMI_GRMF_NAME, NULL);
		EnableControl(IDC_VPGRMI_BTN_GRM_VIEW, FALSE);
	}
	else
	{
		// Grammar is present and it is not empty. Show the grammar source file name.
		EnableControl(IDC_VPGRMI_GRMF_TITLE, TRUE);
		SetControlText(IDC_VPGRMI_GRMF_NAME, grammar->grm_props.grms_file_name);

		// Try to load and display the grammar source code.
		TBasicFileInfo info;
		info.InitBasicFileInfo();
		TFileProblemInfo problem_info(grammar);
		TLoadSaveResult res = TFileBodyHelper::LoadAsciiFileBody(info, grammar->grm_props.grms_file_name);
		if (res != ldres_success)
		{
			// Attempt to load the file failed.
			problem_info.load_res = res;
			ShowFileProblemMessage(&problem_info, FALSE);
		}
		else
		{
			// Some data was picked up from the disk.
			if (info.file_len != grammar->grm_props.grms_file_length || info.file_date != grammar->grm_props.grms_file_date)
			{
				// Picked up file differs from what is mentioned in the grammar. Display warning message about this.
				problem_info.disk_file_len = info.file_len;
				problem_info.disk_file_date = info.file_date;
				ShowFileProblemMessage(&problem_info, FALSE);
			}

			// Show contents of the file in the screen items view.
			if (m_file_view_scr_item.Setup(&m_view_hlpr, &m_file_view_style, info.file_body, info.file_len, dbm_take_ownsh) == TRUE)
			{
				// File data ownership was passed into the file view screen item.
				m_view_hlpr.AddToInternalDataSet(m_file_view_scr_item);
				info.file_body = NULL;
			}
			else
			{
				m_owner->MsgBox(L"Error displaying grammar definition file.");
			}

			// Release the rest of resources in the structure.
			info.FreeBasicFileInfo();
		}

		// Enable/disable the grammar table viewer buttons. Note that grammar cannot be NULL here.
		EnableControl(IDC_VPGRMI_BTN_GRM_VIEW, grammar->grm_props.processing_result == grpr_full_success);
	}

	// Redisplay the current data set.
	m_view_hlpr.ShowInternalDataSet();
}

void TLangViewParserAndGrammarInfo::ShowFileProblemMessage(TFileProblemInfo *info, bool forceDisplay)
{
	if (m_hDlg != NULL && (::IsWindowVisible(m_hDlg) == TRUE) || forceDisplay == TRUE)
	{
		// Display this message immediately.
		if (info->load_res == ldres_fname_missing)
		{
			// Special message for this error.
			m_owner->MsgBox(L"Name of the grammar source file is empty.");
		}
		else if (info->load_res != ldres_success)
		{
			// General loading error.
			m_owner->MsgBoxFmt(MB_OK, L"Error loading the grammar definition file.\r\n\r\nFile name: \t%s          \r\nProblem: \t%s.",
								info->grammar->grm_props.grms_file_name, TFileBodyHelper::GetLoadSaveResultText(info->load_res, TRUE));
		}
		else if (info->disk_file_len != info->grammar->grm_props.grms_file_length)
		{
			// Length is different, check the date also.
			if (info->disk_file_date == info->grammar->grm_props.grms_file_date)
			{
				m_owner->MsgBox(L"WARNING:\r\n\r\nGrammar definition file on disk has different length.");
			}
			else
			{
				m_owner->MsgBox(L"WARNING:\r\n\r\nGrammar definition file on disk has different length and date.");
			}
		}
		else if (info->disk_file_date != info->grammar->grm_props.grms_file_date)
		{
			m_owner->MsgBox(L"WARNING:\r\n\r\nGrammar definition file on disk has different date.");
		}
	}
	else
	{
		// Queue this error message for displaying it later.
		memcpy(&m_source_loading_problem, info, sizeof(TFileProblemInfo));
		::PostMessage(m_hDlg, WM_COMMAND, IDC_VPGRMI_FPROBLEM_MSG, 0);
	}
}

// -----------------------------------------------------------------------------
//   ==============  TLangViewVisualParserConsoleObject  ==================
// -----------------------------------------------------------------------------

TLangViewVisualParserConsoleObject::TLangViewVisualParserConsoleObject()
{
	m_owner = NULL;
	m_driver = NULL;
	m_saved_symbol = 0xFFFF;
	m_saved_action = 0xFFFF;
	m_latest_picked_symbol = NULL;
	m_latest_event = pcev_none;

	m_abort_request = FALSE;
	m_abort_raised = FALSE;

	m_hEvent = NULL;
	m_hWorkerThread = NULL;
	m_dwThreadId = 0;
	m_dwThreadExitCode = 0;
	m_suspension_mask = 0;
}

TLangViewVisualParserConsoleObject::~TLangViewVisualParserConsoleObject()
{
	// At this point the thread should be already inactive.
	bool completed_right_now;
	DWORD exit_code;
	assert(CheckThread(completed_right_now, exit_code) == FALSE);

	// Close the handles if they are not closed yet.
	if (m_hWorkerThread != NULL)
		::CloseHandle(m_hWorkerThread);
	if (m_hEvent != NULL)
		::CloseHandle(m_hEvent);
}

void TLangViewVisualParserConsoleObject::ProcessEvent(TGenericParserConsoleEvent evt_code, WORD &evt_data, DWORD event_data_ex)
{
	//
	// This is the callback from the TGrammarBasedParser.
	//

	if (evt_code == pcev_symbol_discard && evt_data >= IgnoreLexTypeBase && evt_data <= GenericIgnoreLex)
	{
		// Ignore discarding these symbols because they are expected to be discarded by the grammar.
		return;
	}

	if (evt_code == pcev_action_pickup)
	{
		// Check the action that was just picked up.
		if (evt_data >= actb_conflict && evt_data < actb_nested_result)
		{
			// Conflict was picked up. Modify the event code.
			evt_code = pcev_grpars_cfct_pickup;
		}
	}

	m_latest_event = evt_code;
	TParsingActionOriginInfo spec_origin_info;
	spec_origin_info.dw_action_origin = event_data_ex;

	// (1) Save some of the event params and parser fields.
	if (evt_code == pcev_symbol_pickup)
	{
		// Save pointer to symbol state item and symbol value. It makes sense to save symbol value
		// since it may change in the symbol state item structure.
		m_saved_symbol = evt_data;
		m_latest_picked_symbol = (TSymbolStateItem*)m_owner->m_parser->m_inp_symbols.GetLast();
		CreateRecreateLexemaLexItem(m_latest_picked_symbol, 0xFFFF, spec_origin_info);
	}
	else if (evt_code == pcev_action_pickup || evt_code == pcev_action_resolution || evt_code == pcev_grpars_cfct_pickup)
	{
		// Save parsing action that was just picked up.
		m_saved_action = evt_data;
	}

	// (2) Check if abort is requested and if parsing should be suspended or not.
	if (m_abort_request == TRUE)
	{
		if (m_abort_raised == FALSE)
		{
			// Async abort was issued.
			AbortParsingProcess(TRUE);
		}
	}
	else if ((m_suspension_mask & (1 << evt_code)) != 0 || (evt_code == pcev_symbol_pickup && m_owner->CheckLexemaBreakPoint() == TRUE))
	{
		// Suspend execution until event will be signalled.
		::PostMessage(m_owner->GetWnd(), WM_COMMAND, MAKEWPARAM(IDC_VPARS_PARSER_CONS_NTF, evt_code), evt_data);
		::WaitForSingleObject(m_hEvent, INFINITE);

		// Event is signalled.
		::ResetEvent(m_hEvent);
	}

	// (3) Update console related data in the parser structures.
	if (evt_code == pcev_symbol_update)
	{
		// Grammar based parser is organized so, that it can update only symbol that was puicked up before.
		CreateRecreateLexemaLexItem(m_latest_picked_symbol, 0xFFFF, spec_origin_info);
	}
	else if (evt_code == pcev_grpars_cfct_pickup)
	{
		// Give out the action value that might change.
		evt_data = m_saved_action;
	}
	else if (evt_code == pcev_action_shift)
	{
		//
		//  Shift action.
		//

		// Input queue should contain symbol that is about to be shifted and this symbol should be the terminal symbol.
		TSymbolStateItem *shifted_sym_stt = m_owner->m_parser->GetInpSymPtr(0);
		assert(shifted_sym_stt != NULL && shifted_sym_stt->m_symbol < IgnoreLexTypeBase);

		if (shifted_sym_stt->m_dbg_data == NULL)
		{
			CreateRecreateLexemaLexItem(shifted_sym_stt, evt_data, spec_origin_info);
		}
		else
		{
			TLangViewLexemaTreeNode *lex_node = (TLangViewLexemaTreeNode*)(shifted_sym_stt->m_dbg_data);
			if (evt_data != lex_node->GetShiftDestState() || spec_origin_info.dw_action_origin != lex_node->GetSpecOriginDword())
			{
				// The origin has changed.
				lex_node->SetShiftDestState(evt_data);
				lex_node->SetSpecOriginDword(spec_origin_info.dw_action_origin);
				lex_node->SetUniqueNodeId(++(m_owner->m_lex_item_ids_cnt));
			}
		}
	}
	else if (evt_code == pcev_action_resolve && m_owner->m_parser->CheckForCriticalError() == FALSE)
	{
		//
		//  Reduce action.
		//

		// Pick up the symbol state item that will become the rule non term.
		TSymbolStateItem *slot_proto = m_owner->m_parser->m_non_term_proto;
		assert(slot_proto != NULL && slot_proto->m_symbol >= NonTerminalsBase);
		assert(slot_proto->m_dbg_data == NULL);

		// Grammar based parser should have already prepared the origin of the new non terminal.
		TSourceLocation non_term_origin;
		non_term_origin.InitFromPrHeader(m_owner->m_parser->GetParsingId(), slot_proto->GetOriginHeader());

		// Pick up the unfo about the rule being applied.
		TGrammarRule &rule = m_owner->m_parser->m_grammar->rules[evt_data-actb_reduce];
		int rhs_length = rule.Length();

		// Create a LexItem for the new non terminal.
		TLangViewNonTerminalTreeNode *non_term_item = new TLangViewNonTerminalTreeNode(NULL, 0, rule.non_term, evt_data-actb_reduce, non_term_origin, spec_origin_info);
		if (non_term_item != NULL)
		{
			if (rhs_length > 0)
			{
				// Stack of the parser contains objects that participated in creation of non terminal. Pick up ptr to the first object.
				TSymbolStateItem *rhs_first_sym = m_owner->m_parser->GetStackSlotPtr(rhs_length);

				// Convert objects that constitute the right hand side of the rule into the children of this brand new non terminal.
				TSymbolStateItem *rhs = rhs_first_sym;
				for (int i=0; i<rhs_length; ++i)
				{
					TLangViewTreeNode *chld = (TLangViewTreeNode*)(rhs->m_dbg_data);
					if (chld != NULL)
					{
						non_term_item->AdoptChild(chld);
						rhs->m_dbg_data = NULL;				// Clear the pointer to mark the fact that dbg data is taken.

						if (chld->m_missing_chld == TRUE)
							non_term_item->m_missing_chld = TRUE;
					}
					else
					{
						non_term_item->m_missing_chld = TRUE;
					}

					rhs = (TSymbolStateItem*)(m_owner->m_parser->m_stack.GetNext(rhs));
				}
			}
			else
			{
				// Non terminal is created "out of nothing". Create empty lexema that will symbolize this.
				TLangViewSpecialTreeNode *empty_lexema_node = new TLangViewSpecialTreeNode();
				if (empty_lexema_node != NULL)
				{
					// Empty lexema should still contain the correct origin field.
					TLexemaInfo empty_lex_info;
					empty_lex_info.Clear();
					empty_lex_info.origin = non_term_origin;

					empty_lexema_node->SetLexItemClickOwner();
					empty_lexema_node->Item().SetEmptySymbolProps(empty_lex_info);
					non_term_item->AdoptChild(empty_lexema_node);
				}
				else
				{
					m_owner->ShowAsyncMessage(L"Error creating TLangViewTreeNode for representing the empty lexema.");
					non_term_item->m_missing_chld = TRUE;
				}
			}

			// Finalize the tree node.
			non_term_item->SetLexItemClickOwner();
			non_term_item->SetUniqueNodeId(++(m_owner->m_lex_item_ids_cnt));
			slot_proto->m_dbg_data = non_term_item;
		}
		else
		{
			m_owner->ShowAsyncMessage(L"Error creating TLangViewTreeNode for non terminal symbol.");
		}
	}
	else if (evt_code == pcev_errec_sym_updated)
	{
		// It is necessary to discard the debug data in the symbol being modified.
		TSymbolStateItem *sym_stt = m_owner->m_parser->GetInpSymPtr(evt_data);
		CreateRecreateLexemaLexItem(sym_stt, 0xFFFF, spec_origin_info);
	}
}

void TLangViewVisualParserConsoleObject::RecycleParserConsoleData(TListItem *data)
{
	// There is no look aside list so far. This method can be called from both threads.
	delete data;
}

bool TLangViewVisualParserConsoleObject::ProcessSourceFileMethod(DWORD initial_suspension_mask, TParsingDriver &driver, TProcessSourceFileInfo &info)
{
	assert(m_owner->GetWnd() != NULL);
	assert(&driver != NULL);

	bool completed_right_now;
	DWORD exit_code;
	if (CheckThread(completed_right_now, exit_code) == TRUE)
	{
		m_owner->MsgBox(L"Cannot process the source file. Background thread is still active.");
		return(FALSE);
	}

	// The worker thread is inactive. Save params and launch the new thread.
	m_driver = &driver;
	m_src_file_info = info;
	return(LaunchThread(initial_suspension_mask, ProcessSourceFileThreadProc));
}

bool TLangViewVisualParserConsoleObject::ProcessCommandLineMethod(DWORD initial_suspension_mask, TParsingDriver &driver, TProcessCommandLineInfo &info)
{
	assert(m_owner->GetWnd() != NULL);
	assert(&driver != NULL);

	bool completed_right_now;
	DWORD exit_code;
	if (CheckThread(completed_right_now, exit_code) == TRUE)
	{
		m_owner->MsgBox(L"Cannot process the command line. Background thread is still active.");
		return(FALSE);
	}

	// Thread is unactive. Save params and launch the new thread.
	m_driver = &driver;
	m_cmd_line_info = info;
	return(LaunchThread(initial_suspension_mask, ProcessCommandLineThreadProc));
}

void TLangViewVisualParserConsoleObject::SetupSuspensionMask(DWORD mask)
{
	// Duplicate syntax error recovery bits.
	if ((mask & (1 << pcev_errec_sym_added)) != 0)
	{
		mask |= (1 << pcev_errec_sym_updated);
		mask |= (1 << pcev_errec_sym_removed);
	}

	// Duplicate action pickup bit.
	if ((mask & (1 << pcev_action_pickup)) != 0)
	{
		mask |= (1 << pcev_action_resolution);
	}

	// Remove parsing complete bit if it is there.
	mask &= ~(1 << pcev_parsing_complete);

	// Finaly assign the mask.
	m_suspension_mask = mask;
}

void TLangViewVisualParserConsoleObject::AbortParsingProcess(bool worker_thread_stopped)
{
	if (m_abort_raised == TRUE)
	{
		// Abort is already sent.
		assert(FALSE);
		return;
	}

	m_abort_request = TRUE;
	if (worker_thread_stopped == TRUE)
	{
		// It is ok to call methods on the parser object.
		m_owner->m_parser->RaiseCriticalError(NULL, TGrammarBasedParser::err_aborted_by_debugger, L"Aborted by the user.");
		m_abort_raised = TRUE;
	}
}

bool TLangViewVisualParserConsoleObject::CheckThread(bool &event_detected, DWORD &exit_code)
{
	event_detected = FALSE;
	exit_code = m_dwThreadExitCode;
	if (m_hWorkerThread == NULL)
	{
		// Thread has terminated long ago.
		return(FALSE);
	}

	if (::GetExitCodeThread(m_hWorkerThread, &m_dwThreadExitCode) == FALSE)
	{
		// Win32 returned error. This should be a rare situation.
		assert(FALSE);
		return(TRUE);
	}

	exit_code = m_dwThreadExitCode;
	if (m_dwThreadExitCode != STILL_ACTIVE)
	{
		// Thread has terminated its work.
		event_detected = TRUE;
		::CloseHandle(m_hWorkerThread);
		m_hWorkerThread = NULL;
		m_driver = NULL;
		return(FALSE);
	}

	// Thread is still active.
	return(TRUE);
}

void TLangViewVisualParserConsoleObject::CreateRecreateLexemaLexItem(TSymbolStateItem *slot, WORD final_action, TParsingActionOriginInfo spec_origin_info)
{
	// Ensure that passed symbol state item contains the terminal symbol. Note that this can also
	// be ignore_lex, error_lex or lexema that was not mapped to any symbol (0xFFFF);
	assert(slot->m_symbol < NonTerminalsBase || slot->m_symbol == 0xFFFF);

	if (slot->m_dbg_data != NULL)
	{
		if (final_action == 0xFFFF)
			final_action = ((TLangViewLexemaTreeNode*)(slot->m_dbg_data))->GetShiftDestState();
		if (spec_origin_info.dw_action_origin == 0xFFFFFFFF)
			spec_origin_info.dw_action_origin = ((TLangViewLexemaTreeNode*)(slot->m_dbg_data))->GetSpecOriginDword();

		RecycleParserConsoleData(slot->m_dbg_data);
		slot->m_dbg_data = NULL;
	}

	// Create LexemaInfo out of lexema.
	TLexemaInfo lex_info;
	if (lex_info.Init(m_owner->m_parser->GetParsingId(), slot->m_lex, &(m_owner->m_lex_info_strs_dict)) == FALSE)
	{
		m_owner->ShowAsyncMessage(L"Error duplicating lexema string while creating the TLangViewTreeNode.");
		return;
	}

	// Create new lex item.
	slot->m_dbg_data = new TLangViewLexemaTreeNode(NULL, 0, slot->m_symbol, lex_info, final_action, spec_origin_info);
	if (slot->m_dbg_data != NULL)
	{
		// Assign unique id to the object that was just created.
		m_owner->GetSlotData(*slot)->SetLexItemClickOwner();
		m_owner->GetSlotData(*slot)->SetUniqueNodeId(++(m_owner->m_lex_item_ids_cnt));
	}
	else
	{
		m_owner->ShowAsyncMessage(L"Error creating TLangViewTreeNode for terminal symbol.");
	}
}

bool TLangViewVisualParserConsoleObject::LaunchThread(DWORD initial_suspension_mask, LPTHREAD_START_ROUTINE thread_entry_point)
{
	assert(m_hWorkerThread == NULL);

	// Check if event is already created or not.
	if (m_hEvent == NULL)
	{
		// Create unnamed manual reset event with initial state "reset".
		m_hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		if (m_hEvent == NULL)
		{
			m_owner->MsgBox(L"Cannot create event. Action is aborted.");
			m_driver = NULL;
			return(FALSE);
		}
	}

	// Clear app fields describing earlier parsings if any.
	m_saved_symbol = 0xFFFF;
	m_saved_action = 0xFFFF;
	m_latest_picked_symbol = NULL;
	m_abort_request = FALSE;
	m_abort_raised = FALSE;

	// Setup suspension mask before starting the thread.
	SetupSuspensionMask(initial_suspension_mask);

	// Start the background thread.
	m_dwThreadId = 0;
	m_dwThreadExitCode = STILL_ACTIVE;
	m_hWorkerThread = ::CreateThread(NULL, 0, thread_entry_point, this, 0, &m_dwThreadId);
	if (m_hWorkerThread == NULL)
	{
		m_owner->MsgBox(L"Cannot start the worker thread. Action is aborted.");
		m_driver = NULL;
		return(FALSE);
	}

	// Caller should wait for async result.
	return(TRUE);
}

// -----------------------------------------------------------------------------
//   ================  TLangViewVisualParserFrame  =====================
// -----------------------------------------------------------------------------

static TTextStyleProps g_VisualParserFrameTextStyleProps[4] =
{
	// Upper messages in the syntax tree viewer.
	{ NULL, VERDANA, 8, FALSE, RGB(0, 0, 0), STD_TXT },

	// Parser state in the syntax tree viewer.
	{ NULL, VERDANA, 10, TRUE, RGB(0, 0, 0), RGB(181, 235, 255), STD_NO_FRSH, 3, 0, 4, 10, 10 },

	// Normal messages in current state comments.
	{ NULL, VERDANA, 10, TRUE, RGB(80, 80, 80), STD_TXT },

	// Bold messages in current state comments.
	{ NULL, TAHOMA, 13, TRUE, RGB(40, 40, 40), STD_BKGR, 4, 0, 5, 0, 0 },
};

#define  VPARS_CONSOLE_TITLE	L" Visual Parser "

TLangViewVisualParserFrame::TLangViewVisualParserFrame(TLangViewBrowserFrame *parent_handler, TMidLevStoreReader *db_link, bool cpp_prompt)
			: TLangViewBrowserFrame(parent_handler, db_link, IDD_LANGV_VPARS_MAIN_FRAME),
			m_wait_panel(NULL, VPARS_CONSOLE_TITLE), m_proc_stt_panel(NULL, VPARS_CONSOLE_TITLE),
			//
			//  Dock object instantiation params:
			//    NULL - proto params will be set up later;
			//    100/80/400/140 - dock weight in dbu;
			//    0 - children of this dock do not need show/hide notifications.
			//
			m_main_vert_dock(NULL, 100, 0),
			m_upper_horz_dock(NULL, 80, 0), m_main_horz_dock(NULL, 100, 0),
			m_second_vert_dock(NULL, 200, 0), m_right_vert_dock(NULL, 40, 0),
			m_middle_horz_dock(NULL, 400, 0), m_lower_horz_dock(NULL, 140, 0),
			m_load_panel(cpp_prompt), m_pars_stt_details(NULL), m_src_view(NULL),
			m_cpp_item_details(NULL), m_mid_lev_viewer(parent_handler, NULL)
{
	SetMessageBoxTitle(L" Visual Grammar Based Parser ");

	m_vpst = vpst_idle;
	m_parser = NULL;
	m_parser_console.SetOwner(this);
	m_latest_resume_time = 0;
	m_wait_panel.SetDialogParams(this);
	m_proc_stt_panel.SetDialogParams(this);

	// Setup the initial state of the parsing suspension masks. This state will be loaded on the checkboxes later.
	m_step_brks_mask = (1 << pcev_action_shift) | (1 << pcev_action_resolve) | (1 << pcev_enter_nesting) | (1 << pcev_exit_nesting);
	m_run_brks_mask  = (1 << pcev_parsing_startup);
	m_lex_item_ids_cnt = 0;

	m_main_vert_dock.SetDialogParams(this);
	m_upper_horz_dock.SetDialogParams(this);
	m_main_horz_dock.SetDialogParams(this);
	m_second_vert_dock.SetDialogParams(this);
	m_right_vert_dock.SetDialogParams(this);
	m_middle_horz_dock.SetDialogParams(this);
	m_lower_horz_dock.SetDialogParams(this);

	m_load_panel.SetDialogParams(this);
	m_load_panel.SetOwner(this);
	m_control_panel.SetDialogParams(this);
	m_control_panel.SetOwner(this);
	m_scr_break_points.SetDialogParams(this);
	m_scr_break_points.SetOwner(this);

	m_curr_pars_stt.SetDialogParams(this);
	m_empty_input.SetDialogParams(this, IDD_LANGV_VPARS_EMPTY_INPUT);

	m_grammar_info.SetDialogParams(this);
	m_grammar_info.SetOwner(this);

	m_pars_stt_details.SetDialogParams(this);
	m_pars_stt_details.SetOwner(this);
	m_pars_stt_details.ShowTitles(FALSE, TRUE);

	m_src_view.SetDialogParams(this);
	m_src_view.SetOwner(this);

	// Prepare name table viewers for all supported languages.
	m_nam_tab_view_code = pntvc_none;
	NameTableView_InitChildDialogs();

	m_source_file_load_res = ldres_success;
	m_source_file_name[0] = 0;
}

LRESULT TLangViewVisualParserFrame::OnInitDialog(HWND hDefFocusControl)
{
	// This dialog requires checking for async events.
	if (::SetTimer(m_hDlg, MainTimerIdent, 100, NULL) == 0)
	{
		// This failure is unlikely to happen. Nevertheless.
		return(-1);
	}

	// Setup objects on the dialog level.
	SetControlPos(IDCANCEL, 0, -30);

	m_wait_panel.SetParentWindow(m_hDlg);
	m_proc_stt_panel.SetParentWindow(m_hDlg);
	m_wait_panel.SetMajorStatus(L"Closing ...");
	m_wait_panel.SetMinorStatus(L"(waiting for parsing termination)");

	// Create main dock dialog and place it over the whole frame.
	HWND hMainDockWnd = m_main_vert_dock.OpenNonModalDialog(m_hDlg, IDC_VPARS_MAIN_VERT_DOCK);
	if (hMainDockWnd == NULL)
		return(-1);

	// Retrieve the width of the load panel and the height of the control panel. Temorarily set the frame window
	// as the parent window for these panels. Later on the splitter will reassign the parents.
	HWND hLoadPanel = m_load_panel.OpenNonModalDialog(m_hDlg);
	if (hLoadPanel == NULL)
		return(-1);
	HWND hCtrlPanel = m_control_panel.OpenNonModalDialog(m_hDlg);
	if (hCtrlPanel == NULL)
		return(-1);
	HWND hSrcBrksPanel = m_scr_break_points.OpenNonModalDialog(m_hDlg);
	if (hSrcBrksPanel == NULL)
		return(-1);

	RECT rc_load, rc_ctrl;
	::GetWindowRect(hLoadPanel, &rc_load);
	::GetWindowRect(hCtrlPanel, &rc_ctrl);

	RECT rc_window, rc_client;
	::GetWindowRect(m_hDlg, &rc_window);
	::GetClientRect(m_hDlg, &rc_client);

	int upper_offs = 2;
	SetControlPos(IDC_VPARS_MAIN_VERT_DOCK, 0, upper_offs);
	SetControlSize(IDC_VPARS_MAIN_VERT_DOCK, rc_client.right, rc_client.bottom-upper_offs);
	SetupControlPositioning(IDC_VPARS_UPPER_GRP_BOX, cpm_fix_top_side);
	SetupControlPositioning(IDC_VPARS_MAIN_VERT_DOCK, cpm_fix_all_corners);
	SetMinTrackingSize(rc_load.right-rc_load.left + (rc_window.right-rc_window.left) - (rc_client.right-rc_client.left), 0);

	long ctrl_frame_lower_offs = m_control_panel.GetLowerFrameOffset();
	m_load_panel.UpdateLowerFrameOffset(ctrl_frame_lower_offs);
	m_scr_break_points.UpdateLowerFrameOffset(ctrl_frame_lower_offs);

	// (1) Add the horz docks to the main vertical dock.
	if (m_main_vert_dock.ShowChildDialog(&m_upper_horz_dock, FALSE, -1, NULL, FALSE, rc_ctrl.bottom-rc_ctrl.top) == FALSE)
		return(-1);
	if (m_main_vert_dock.ShowChildDialog(&m_main_horz_dock, FALSE) == FALSE)
		return(-1);

	// (2) Add vertical dock to the main horz dock.
	if (m_main_horz_dock.ShowChildDialog(&m_second_vert_dock, FALSE) == FALSE)
		return(-1);
	if (m_main_horz_dock.ShowChildDialog(&m_right_vert_dock, FALSE) == FALSE)
		return(-1);

	// (3) Add horz docks to the second vert dock.
	if (m_second_vert_dock.ShowChildDialog(&m_middle_horz_dock, FALSE) == FALSE)
		return(-1);
	if (m_second_vert_dock.ShowChildDialog(&m_lower_horz_dock, FALSE) == FALSE)
		return(-1);

	// (4) Fill in the upper horzontal splitter.
	if (m_upper_horz_dock.ShowChildDialog(&m_load_panel, FALSE, -1, NULL, FALSE, rc_load.right-rc_load.left) == FALSE)
		return(-1);
	if (m_upper_horz_dock.ShowChildDialog(&m_control_panel, FALSE, -1, NULL, TRUE, 550) == FALSE)
		return(-1);
	if (m_upper_horz_dock.ShowChildDialog(&m_scr_break_points, FALSE, -1, NULL, TRUE, 260) == FALSE)
		return(-1);

	// (5) Fill in the lower horzontal splitter. Hide pars stt details and source code panels.
	if (m_lower_horz_dock.ShowChildDialog(&m_grammar_info, FALSE) == FALSE)
		return(-1);
	if (m_lower_horz_dock.ShowChildDialog(&m_pars_stt_details, FALSE) == FALSE)
		return(-1);
	if (m_lower_horz_dock.ShowChildDialog(&m_src_view, FALSE) == FALSE)
		return(-1);

	m_lower_horz_dock.HideChildDialog(&m_pars_stt_details);
	m_lower_horz_dock.HideChildDialog(&m_src_view);

	// (6) Fill in the right vert splitter. When it will become clear that its dialogs can be opened,
	// hide the whole splitter.
	if (NameTableView_OnInitDialog() == FALSE)
		return(-1);

	m_main_horz_dock.HideChildDialog(&m_right_vert_dock);

	// Prepare screen item styles for syntax tree viewers.
	assert(TLangViewLexItemStyle::MIN_DIM_GRADE == 0);
	for (int istyle=0; istyle<TLangViewLexItemStyle::NUM_DIM_GRADES; ++istyle)
	{
		// Background color.
		TColor bkgr_color = RGB(255, 255, 255);
		TLangViewLexItemStyle::ApplyDimGrade(bkgr_color, istyle);
		m_synt_tree_bkgr_color[istyle] = bkgr_color;

		// Upper text messages.
		TTextStyleProps props1 = g_VisualParserFrameTextStyleProps[0];
		TLangViewLexItemStyle::ApplyDimGrade(props1, istyle);
		m_synt_tree_upper_msgs_style[istyle].Setup(props1);
		if (m_synt_tree_upper_msgs_style[istyle].SetupForDeviceContextEx(m_hDlg) == FALSE)
			return(-1);

		// Current parsing state labels.
		TTextStyleProps props2 = g_VisualParserFrameTextStyleProps[1];
		props2.SetFrameProps(RGB(104, 134, 215), fcs_1pix_add_inner_dot);
		TLangViewLexItemStyle::ApplyDimGrade(props2, istyle);
		m_synt_tree_pars_stt_style[istyle].Setup(props2);
		if (m_synt_tree_pars_stt_style[istyle].SetupForDeviceContextEx(m_hDlg) == FALSE)
			return(-1);
	}

	// Send notification to the derived class.
	OnInitDialogPrepareMasks();

	// Enusre that update and discard error recovery bits are cleared.
	m_step_brks_mask &= ~(1 << pcev_errec_sym_updated);
	m_run_brks_mask  &= ~(1 << pcev_errec_sym_updated);
	m_step_brks_mask &= ~(1 << pcev_errec_sym_removed);
	m_run_brks_mask  &= ~(1 << pcev_errec_sym_removed);

	// Propagate masks information to the control panel.
	m_control_panel.SetupInitialMaskCheckBoxesState(m_run_brks_mask, m_step_brks_mask);

	// Let derived class to do something.
	OnInitDialogDoAppActions();

	// Do not put the focus anywhere.
	return(FALSE);
}

void TLangViewVisualParserFrame::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDCANCEL:
		case IDM_VPARS_MENU_FILE_EXIT:
				{
					// User asked to close the window.
					if (m_vpst != vpst_idle)
					{
						// Parsing session is present.
						if (m_vpst != vpst_final)
						{
							// Parsing process is still active.
							if (m_vpst != vpst_running_abort)
							{
								// State is not abort. Make it abort.
								SetupRunningState(vpst_running_abort);
							}

							// Open the modal dialog. It will stay opened until the background thread will terminate.
							// Waiting will take place inside the message loop of the modal dialog.
							m_wait_panel.OpenModalConsole();
						}

						UnloadParsingSession();
					}

					::DestroyWindow(m_hDlg);
				}
				break;

		case IDM_VPARS_MENU_ACTS_CODE_GEN:
				{
					// Run the code generation test.
					if (m_parser == NULL)
					{
						MsgBox(L"The parsing session is not active.");
					}
					else if (m_parser->GetParserLangType() != lang_cpp)
					{
						MsgBox(L"Current parser is not the Cpp parser.");
					}
					else
					{
						// The session is active and current parser is the Cpp parser.
						TCppItemBase *base_item = m_cpp_global_namesp.GetSelectedCppItem();
						if (base_item == NULL)
						{
							MsgBox(L"There is no selection in the global namespace child window.");
						}
						else if (base_item->ItemType() != cxx_code_entry_point && base_item->ItemType() != cxx_func_template_inst)
						{
							MsgBoxFmt(MB_OK, L"Selected object is not a function or function template instantiation. It is %s.", base_item->ItemTypeEnumName());
						}
						else
						{
							// Allocate code generator and build code for the selected function.
							TMessageBuffer error_buffer;
							TCppFunctionCode function_code;
							TCppFunctionCodeGenerator code_gen(((TCppParser*)m_parser)->GetDatabase());
							if (code_gen.GenerateFunctionCode(error_buffer, function_code, (TCppCodeEntryPointDefn*)base_item) == FALSE)
							{
								MsgBoxFmt(MB_OK, L"===>  Function code generation error.\r\n\r\n%s", (const wchar_t*)error_buffer);
							}
							else
							{
								// Generate report about the generated code and place it into the clipboard.
								TDestinationFile report_buffer;
								report_buffer.PrepareInMemoryStream();
								bool gen_res = function_code.GenerateBasicReport(report_buffer);
								TControlHelper::PlaceDestinationFileToClipboard(this, gen_res, report_buffer, TRUE);
							}
						}
					}
				}
				break;

		case IDC_VPCTRL_BTN_STEP:
				{
					assert(m_vpst == vpst_stopped);
					SetupRunningState(vpst_running_step);
				}
				break;

		case IDC_VPCTRL_BTN_RUN:
				{
					assert(m_vpst == vpst_stopped);
					SetupRunningState(vpst_running_run);
				}
				break;

		case IDC_VPCTRL_BTN_BREAK:
				{
					SetupRunningState(vpst_running_break);
				}
				break;

		case IDC_VPCTRL_BTN_ABORT:
				{
					SetupRunningState(vpst_running_abort);
				}
				break;

		case IDC_VPCTRL_BTN_GOON:
				{
					// This button should be visible only in stopped state with error.
					assert(m_vpst == vpst_stopped_error);
					SetupRunningState(vpst_running_run);
				}
				break;

		case IDC_VPCTRL_BTN_UNLOAD:
				{
					// This button should be visible only in the final state.
					assert(m_vpst == vpst_final);
					UnloadParsingSession();
				}
				break;

		case IDC_VPCTRL_BTN_MIDLEVDB:
				{
					// This button should be visible only in the final state.
					assert(m_vpst == vpst_final);
					if (m_mid_lev_viewer.GetWnd() == NULL)
					{
						// Viewer is not opened. Show parsing events in the viewer.
						assert(m_mid_lev_viewer.DataBase() == NULL);

						wchar_t buffer_4_mpath[4*MAX_PATH];
						TMidLevStoreReader *reader = m_parser->m_scanner->CbkHandler().AllocateReaderObject(buffer_4_mpath);
						if (reader == NULL)
						{
							// Display the error.
							MsgBox(buffer_4_mpath);
							break;
						}

						m_mid_lev_viewer.SetupDataBase(reader);
						m_mid_lev_viewer.SetupTask(0, PSET_ALL_TYPES);
						if (m_nam_tab_view_code == pntvc_cpp)
						{
							// Current parser is the cpp parser. Open viewer is the special mode.
							TCppParser *cpp_parser = (TCppParser*)m_parser;
							// -BUGBUG- m_mid_lev_viewer.SetupCppDatabase(&cpp_parser->GetDatabase());
						}

						m_mid_lev_viewer.OpenNonModalDialog(m_hDlg);
					}
					else
					{
						// The viewer is opened. Bring it to the top.
						if (::IsIconic(m_mid_lev_viewer.GetWnd()) == TRUE)
							::OpenIcon(m_mid_lev_viewer.GetWnd());
						::SetFocus(m_mid_lev_viewer.GetWnd());
					}
				}
				break;

		case IDC_VPARS_PARSER_CONS_NTF:
				{
					// Background thread has suspended its work. It asks to show its state and asks for further instructions.
					ProcessConsoleNtf((TGenericParserConsoleEvent)notificationCode, (WORD)hWndControl);
				}
				break;

		case IDC_VPARS_ASYNC_MESSAGE1:
				{
					// Worker thread has sent an error message for displaying. There is strict resriction on displaying
					// static const messages only. That is why there is no need to take care of the message data.
					MsgBox((const wchar_t*)hWndControl);
				}
				break;

		case IDC_VPARS_ASYNC_MESSAGE2:
				{
					if (m_source_file_load_res != ldres_success)
					{
						ShowSourceFileLoadingProblem(m_source_file_load_res, m_source_file_name, TRUE);
						m_source_file_load_res = ldres_success;
					}
				}
				break;

		case IDM_VPARS_MENU_ACTS_WINDOW_SIZE:
				{
					// Set special window size for making screen shots in consistent resolution.
					::SetWindowPos(m_hDlg, NULL, 0, 0, 1200, 820, SWP_NOMOVE | SWP_NOZORDER);
				}
				break;

		case IDM_VPARS_MENU_ACTS_COLOR_EDIT:
				{
					TControlHelper::ShowColorSelectDialogEx(this, L" Select color ");
				}
				break;

		case IDM_VPARS_MENU_HELP_ABOUT:
				{
					TLangViewAboutBox about_box(L"Visual Parser Application", L"Version August 2011", COMMUNITY_EDITION);
					about_box.OpenModalDialog(m_hDlg);
				}
				break;
	}
}

void TLangViewVisualParserFrame::OnWmTimer(int timer_id)
{
	if (m_vpst != vpst_idle && m_vpst != vpst_final)
	{
		// Background thread should be active. Check is status. It is necessary to react only if the worker
		// thread has terminated right now.
		bool thread_completed;
		DWORD thread_exit_code;
		m_parser_console.CheckThread(thread_completed, thread_exit_code);

		if (thread_completed == TRUE)
		{
			// Thread has finished right now. Check if closing of the main dialog is pending or not.
			if (m_wait_panel.IsOpened() == FALSE)
			{
				// Closing is not pending. Check and display the parsing results.
				bool fParsingSuccess = FALSE;
				SetupFinalState(thread_exit_code, fParsingSuccess);

				// Notify derived class about the parsing termination.
				OnParsingComplete(fParsingSuccess);
			}
			else
			{
				// Closing of the main dialog is pending. Do not show anything, just continue.
				m_wait_panel.CloseConsole();
			}
		}
	}
}

bool TLangViewVisualParserFrame::OnWmDestroy()
{
	::KillTimer(m_hDlg, MainTimerIdent);
	return(TLangViewBrowserFrame::OnWmDestroy());
}

void TLangViewVisualParserFrame::ProcessSingleClick(TLangViewChildBrowser *inst, TLangViewClickEvent &event_info)
{
	if (inst == &m_cpp_global_namesp)
	{
		m_cpp_global_namesp.SetSelection(event_info, (inst != &m_cpp_global_namesp) ? TRUE : FALSE);

		// Forward these clicks to the details view.
		m_cpp_item_details.LoadContents(event_info);

		if (event_info.type == clet_cpp_object)
			m_cpp_items_report.LoadCppItems(*(event_info.cpp_item));
	}
}

void TLangViewVisualParserFrame::ProcessDoubleClick(TLangViewChildBrowser *inst, TLangViewClickEvent &event_info)
{
	if (inst == &m_cpp_global_namesp)
	{
		m_cpp_global_namesp.SetSelection(event_info, (inst != &m_cpp_global_namesp) ? TRUE : FALSE);

		// Forward these clicks to the details view.
		m_cpp_item_details.LoadContents(event_info);

		if (event_info.type == clet_cpp_object)
			m_cpp_items_report.LoadCppItems(*(event_info.cpp_item));
	}
}

void TLangViewVisualParserFrame::RemoveSelection(TLangViewChildBrowser *inst)
{
	if (inst == &m_cpp_global_namesp)
	{
		m_cpp_global_namesp.ResetSelection();
		m_cpp_item_details.ResetContents();
		m_cpp_items_report.ResetContents();
	}
}

bool TLangViewVisualParserFrame::StartProcessingSourceFile(TParsingDriver &driver, TProcessSourceFileInfo &info)
{
	if (PrepareForProcessing(driver) == FALSE)
		return(FALSE);

	assert(m_vpst == vpst_idle);
	if (m_parser_console.ProcessSourceFileMethod(m_run_brks_mask, driver, info) == FALSE)
		return(FALSE);

	// Parsing process managed to start.
	LoadParsingSession(info.src_fname);
	SetupRunningState(vpst_running_run, TRUE);
	return(TRUE);
}

bool TLangViewVisualParserFrame::StartProcessingCommandLine(TParsingDriver &driver, TProcessCommandLineInfo &info)
{
	if (PrepareForProcessing(driver) == FALSE)
		return(FALSE);

	assert(m_vpst == vpst_idle);
	if (m_parser_console.ProcessCommandLineMethod(m_run_brks_mask, driver, info) == FALSE)
		return(FALSE);

	// Parsing process managed to start.
	LoadParsingSession(info.cmdl_fname);
	SetupRunningState(vpst_running_run, TRUE);
	return(TRUE);
}

void TLangViewVisualParserFrame::ProcessConsoleNtf(TGenericParserConsoleEvent evt_code, WORD evt_data)
{
	if (evt_code == pcev_parsing_complete)
	{
		// Let the background thread continue its actions. Visual parser will show the final state after
		// the termination of the worker thread.
		m_parser_console.ResumeThread();
		return;
	}

	// Setup the stopped state. Later on it will be necessary to figure out what breakpoint caused calling
	// this function if current breaks mask does not expect to stop on the current event.
	__int64 active_time_msec = (CurrDateTime()-m_latest_resume_time)/10000;
	SetupStoppedState(evt_code, evt_data);
}

void TLangViewVisualParserFrame::SetupStoppedState(TGenericParserConsoleEvent evt_code, WORD evt_data)
{
	// Presetup for the new state.
	TLangViewVisualParserState new_vpst = vpst_stopped;

	// Prepare variables that will be passed to the function that will display details about the current parsing state.
	TGenericParserConsoleEvent stt_chkbox = evt_code;
	wchar_t *label = NULL, *message = NULL;
	wchar_t *details1 = NULL, *details2 = NULL, *details3 = NULL;
	wchar_t *application_note = NULL;
	TGrammar *manual_conflict_res_grammar = NULL;
	int manual_conflict_res_iconflict = 0;

	// Allocate support buffers for preparing messages.
	wchar_t buff_msg[128];
	wchar_t buff_dtls1[80+TGrammar::MAX_NAME_LENGTH+1];
	wchar_t buff_dtls2[380], buff_dtls3[4096];
	wchar_t buff_supp1[80], buff_supp2[80], buff_supp3[80+TGrammar::MAX_NAME_LENGTH+1];

	int len_msg = sizeof(buff_msg)/sizeof(wchar_t);
	int len_dtls1 = sizeof(buff_dtls1)/sizeof(wchar_t);
	int len_dtls2 = sizeof(buff_dtls2)/sizeof(wchar_t);
	int len_dtls3 = sizeof(buff_dtls3)/sizeof(wchar_t);
	int len_supp3 = sizeof(buff_supp3)/sizeof(wchar_t);

	// Setup for empty main message by default.
	message = buff_msg;
	buff_msg[0] = 0;

	// Prepare messages.
	WORD sym = 0;
	int ixpct = 0;
	switch (evt_code)
	{
		//
		// Start/stop group.
		//

		case pcev_parsing_startup:
				{
					if (evt_data == TRUE)
					{
						label = L"Event:";
						message = L"Parsing has started.";
					}
					else
					{
						// Parsing startup failed. Show the reason of this failure. Note that procedure below should
						// be kept in close sync with TGrammarBasedParser::Parse().
						new_vpst = vpst_stopped_error;
						label = L"Error:";

						if (m_parser->m_root_grammar == NULL)
						{
							// Grammar is missing.
							message = L"Grammar is missing.";
							details1 = L"Check setup of the current parsing driver.";
						}
						else if (m_parser->m_root_grammar->grm_props.processing_result == grpr_none)
						{
							message = L"Grammar is empty.";
							details1 = L"Check setup of the current parsing driver.";
						}
						else if (m_parser->m_root_grammar->grm_props.processing_result != grpr_full_success)
						{
							message = L"Grammar problems.";
							details1 = L"Grammar contains syntax or conversion errors.";
						}
						else if (m_parser->m_root_grammar->NumParsingStates() <= 0)
						{
							message = L"Grammar problems.";
							details1 = L"The parsing table in the grammar is missing.";
						}
						else if (m_parser->m_root_grammar->parent_grammar != NULL)
						{
							message = L"Grammar problems.";
							details1 = L"Initial grammar is not the root grammar.";
						}
						else
						{
							// Grammar is present and it is in the correct state, but app layer does not want to use it.
							message = L"Incompatible grammar.";
							details1 = L"Check the description fields:";

							const wchar_t *handler_avail = m_parser->GetParserDescription(buff_supp2);
							swprintf(buff_dtls2, len_dtls2, L"Parser says: \t%s", (handler_avail != NULL && *handler_avail != 0) ? handler_avail : L"none");
							details2 = buff_dtls2;

							wchar_t *handler_wanted = m_parser->m_root_grammar->grm_props.cbk_handler_name;
							swprintf(buff_dtls3, len_dtls3, L"Grammar is: \t%s", (handler_wanted != NULL && handler_wanted[0] != 0) ? handler_wanted : L"none");
							details3 = buff_dtls3;
						}
					}
				}
				break;

		//
		// ReadSymbol group.
		//

		case pcev_symbol_pickup:
				{
					label = L"Event:";
					message = L"New symbol is picked up.";

					swprintf(buff_dtls1, len_dtls1, L"Symbol:  %s (%hd).", m_parser->m_grammar->GetSymbolName(evt_data, buff_supp1), evt_data);
					details1 = buff_dtls1;

					if (m_parser->m_nest_cstk > 0)
					{
						swprintf(buff_dtls2, len_dtls2, L"Nesting layer:  %d.", m_parser->m_nest_cstk);
						details2 = buff_dtls2;
					}
				}
				break;

		case pcev_symbol_update:
				{
					label = L"Action:";
					message = L"Updating latest symbol.";

					swprintf(buff_dtls1, len_dtls1, L"From: \t%s (%hd).", m_parser->m_grammar->GetSymbolName(m_parser_console.GetSavedSymbol(), buff_supp1), m_parser_console.GetSavedSymbol());
					details1 = buff_dtls1;

					swprintf(buff_dtls2, len_dtls2, L"To: \t%s (%hd).", m_parser->m_grammar->GetSymbolName(evt_data, buff_supp2), evt_data);
					details2 = buff_dtls2;
				}
				break;

		case pcev_symbol_discard:
				{
					label = L"Action:";
					message = L"Discarding latest symbol.";

					swprintf(buff_dtls1, len_dtls1, L"Symbol:  %s (%hd).", m_parser->m_grammar->GetSymbolName(evt_data, buff_supp1), evt_data);
					details1 = buff_dtls1;

					if (m_parser->m_nest_cstk > 0)
					{
						swprintf(buff_dtls2, len_dtls2, L"Nesting layer:  %d.", m_parser->m_nest_cstk);
						details2 = buff_dtls2;
					}
				}
				break;

		//
		// Syntax error recovery group.
		//

		case pcev_errec_sym_added:
				{
					label = L"Recovery:";
					message = L"Adding new input symbol.";
					sym = m_parser->GetInpSymPtr(evt_data)->m_symbol;
					swprintf(buff_dtls1, len_dtls1, L"Symbol [%d]:  %s (%hd).", evt_data, m_parser->m_grammar->GetSymbolName(sym, buff_supp1), sym);
					details1 = buff_dtls1;
				}
				break;

		case pcev_errec_sym_updated:
				{
					stt_chkbox = pcev_errec_sym_added;
					label = L"Recovery:";
					message = L"Updating input symbol.";
					sym = m_parser->GetInpSymPtr(evt_data)->m_symbol;
					swprintf(buff_dtls1, len_dtls1, L"Symbol [%d] from: \t%s (%hd).", evt_data, m_parser->m_grammar->GetSymbolName(m_parser_console.GetSavedSymbol(), buff_supp1), m_parser_console.GetSavedSymbol());
					swprintf(buff_dtls2, len_dtls2, L"Symbol [%d] to: \t%s (%hd).", evt_data, m_parser->m_grammar->GetSymbolName(sym, buff_supp2), sym);
				}
				break;

		case pcev_errec_sym_removed:
				{
					stt_chkbox = pcev_errec_sym_added;
					label = L"Recovery:";
					message = L"Discarding input symbol.";
					sym = m_parser->GetInpSymPtr(evt_data)->m_symbol;
					swprintf(buff_dtls1, len_dtls1, L"Symbol [%d]:  %s (%hd).", evt_data, m_parser->m_grammar->GetSymbolName(sym, buff_supp1), sym);
					details1 = buff_dtls1;
				}
				break;

		//
		// Grammar action group.
		//

		case pcev_action_pickup:
				{
					// Picked action should not be a grammar conflict. (ActionPickup radio button).
					assert(evt_data < actb_conflict || evt_data >= actb_nested_result);
					label = L"Event:";
					message = L"Parsing action pickup.";
					swprintf(buff_dtls1, len_dtls1, L"Action:  %s.", m_parser->m_grammar->GetParsingActionName(evt_data, buff_supp1));
					details1 = buff_dtls1;

					if (m_parser->m_nest_cstk > 0)
					{
						// Show non empty nesting layer.
						swprintf(buff_dtls2, len_dtls2, L"Nesting layer:  %d.", m_parser->m_nest_cstk);
						details2 = buff_dtls2;
					}
				}
				break;

		case pcev_action_resolution:
				{
					// Picked action should not be a grammar conflict. (ActionPickup radio button).
					stt_chkbox = pcev_action_pickup;
					assert(evt_data < actb_conflict || evt_data >= actb_nested_result);
					label = L"Event:";
					message = L"Conflict resolution pickup.";
					swprintf(buff_dtls1, len_dtls1, L"Action:  %s.", m_parser->m_grammar->GetParsingActionName(evt_data, buff_supp1));
					details1 = buff_dtls1;

					if (m_parser->m_nest_cstk > 0)
					{
						// Show non empty nesting layer.
						swprintf(buff_dtls2, len_dtls2, L"Nesting layer:  %d.", m_parser->m_nest_cstk);
						details2 = buff_dtls2;
					}
				}
				break;

		case pcev_grpars_cfct_pickup:
				{
					// Picked action should be the conflict action.
					assert(evt_data >= actb_conflict && evt_data < actb_nested_result);
					label = L"Event:";
					message = L"Grammar conflict pickup.";

					// Prepare name of expected conflict.
					ixpct = m_parser->m_grammar->GetExpectedConflictInx(evt_data-actb_conflict);
					if (ixpct >= 0)
					{
						swprintf(buff_supp3, len_supp3, L"%s  ( X%d )", m_parser->m_grammar->xpct_conflicts[ixpct].xpct_conflict_name, ixpct);
					}
					else
					{
						wcscpy(buff_supp3, L"(Unexpected)");
					}

					swprintf(buff_dtls1, len_dtls1, L"Conflict %hd:  %s.", evt_data-actb_conflict, buff_supp3);
					details1 = buff_dtls1;

					// Request displaying info about the conflict.
					manual_conflict_res_grammar = m_parser->m_grammar;
					manual_conflict_res_iconflict = evt_data-actb_conflict;
				}
				break;

		case pcev_action_update:
				{
					label = L"Action:";
					message = L"Updating parsing action.";
					if (m_parser_console.GetSavedAction() >= actb_conflict && m_parser_console.GetSavedAction() < actb_nested_result)
					{
						// Prepare name of expected conflict.
						ixpct = m_parser->m_grammar->GetExpectedConflictInx(m_parser_console.GetSavedAction()-actb_conflict);
						if (ixpct >= 0)
						{
							swprintf(buff_supp3, len_supp3, L"  (%s)", m_parser->m_grammar->xpct_conflicts[ixpct].xpct_conflict_name);
						}
						else
						{
							wcscpy(buff_supp3, L"  (Unexpected)");
						}
					}
					else
					{
						// No additional info.
						buff_supp3[0] = 0;
					}

					swprintf(buff_dtls1, len_dtls1, L"From: \t%s%s.", m_parser->m_grammar->GetParsingActionName(m_parser_console.GetSavedAction(), buff_supp1), buff_supp3);
					swprintf(buff_dtls2, len_dtls2, L"To: \t%s.", m_parser->m_grammar->GetParsingActionName(evt_data, buff_supp2));
					details1 = buff_dtls1, details2 = buff_dtls2;

					if (m_parser->m_app_callback_note[0] != 0)
					{
						application_note = m_parser->m_app_callback_note;
					}
				}
				break;

		case pcev_action_shift:
				{
					label = L"Action:";
					swprintf(buff_msg, len_msg, L"Shifting to state %hu.", evt_data-actb_shift);
					swprintf(buff_dtls1, len_dtls1, L"Stack length:  %d.", m_parser->m_cstk);
					details1 = buff_dtls1;

					if (m_parser->m_nest_cstk > 0)
					{
						swprintf(buff_dtls2, len_dtls2, L"Nesting layer:  %d.", m_parser->m_nest_cstk);
						details2 = buff_dtls2;
					}
				}
				break;

		case pcev_action_resolve:
				{
					label = L"Action:";
					swprintf(buff_msg, len_msg, L"Applying rule %hu.", evt_data-actb_reduce);
					swprintf(buff_dtls1, len_dtls1, L"Stack length:  %d.", m_parser->m_cstk);
					details1 = buff_dtls1;

					if (m_parser->m_nest_cstk > 0)
					{
						swprintf(buff_dtls2, len_dtls2, L"Nesting layer:  %d.", m_parser->m_nest_cstk);
						details2 = buff_dtls2;
					}
				}
				break;

		//
		// Nested parsing start/stop.
		//

		case pcev_enter_nesting:
				{
					label = L"Event:";
					message = L"Entering nested parsing.";

					sym = evt_data-actb_conflict;
					TParsingLayerInfo &pli = m_parser->m_nest_stk[m_parser->m_nest_cstk-1];
					int ixpct = pli.m_layer_grammar->GetExpectedConflictInx(sym);
					assert(ixpct >= 0);

					swprintf(buff_dtls1, len_dtls1, L"Grammar :  %s.", pli.m_layer_grammar->xpct_conflicts[ixpct].xpct_conflict_name);
					swprintf(buff_dtls2, len_dtls2, L"Num rules:  %d.", m_parser->m_grammar->NumRules());
					details1 = buff_dtls1, details2 = buff_dtls2;
				}
				break;

		case pcev_exit_nesting:
				{
					label = L"Event:";
					message = L"Exiting nested parsing.";

					TParsingLayerInfo &pli = m_parser->m_nest_stk[m_parser->m_nest_cstk-1];
					TGrammarRule &rule = m_parser->m_grammar->rules[evt_data-actb_reduce];
					WORD wTargetSym = (rule.Length() > 0) ? rule.symbols[0] : 0;
					WORD wResolution = pli.m_layer_conflict->conflict_resolution[evt_data-actb_reduce];

					if (wResolution >= actb_reduce && wResolution < actb_conflict)
					{
						// Show app id of the rule.
						swprintf(buff_supp3, len_supp3, L"  (AppId: %hd)", pli.m_layer_grammar->rules[wResolution-actb_reduce].rule_app_id);
					}
					else if (wResolution >= actb_nested_result && wResolution < actb_bogus_val)
					{
						// Show app id of the nested result.
						swprintf(buff_supp3, len_supp3, L"  (AppId: %hd)", pli.m_layer_grammar->nested_processing_results[wResolution-actb_nested_result].nest_res_app_id);
					}
					else
					{
						// This is shift action. No additional info.
						buff_supp3[0] = 0;
					}

					swprintf(buff_dtls1, len_dtls1, L"Target symbol:  %s  (%hd).", m_parser->m_grammar->GetSymbolName(wTargetSym, buff_supp1), wTargetSym);
					swprintf(buff_dtls2, len_dtls2, L"Conclusion:  %s%s.", pli.m_layer_grammar->GetParsingActionName(wResolution, buff_supp2), buff_supp3);
					swprintf(buff_dtls3, len_dtls3, L"Current nesting layer:  %d.", m_parser->m_nest_cstk);
					details1 = buff_dtls1, details2 = buff_dtls2, details3 = buff_dtls3;
				}
				break;

		default:
			assert(FALSE);
			break;
	}

	// Update the window.
	m_vpst = new_vpst;
	m_control_panel.SetVisibleState(m_vpst, stt_chkbox, label, message,
								details1, details2, details3, application_note,
								manual_conflict_res_grammar, manual_conflict_res_iconflict);
	UpdateParserStateSplitter(evt_code, evt_data);
	UpdateParsingStateDetails();
	UpdateParsingLocationMarker();
	if (m_nam_tab_view_code != pntvc_none)
		NameTableView_SetupStoppedState();
	UpdateDialogWindow();
}

void TLangViewVisualParserFrame::SetupRunningState(TLangViewVisualParserState new_stt, bool processing_startup)
{
	assert(new_stt >= vpst_running_step && new_stt <= vpst_running_abort);

	//
	// The caveat of this method is in the "processing_startup" parameter. When this param is TRUE, this means
	// that this method was called right after starting the worker thread. Depending on the initial suspension mask
	// and threads scheduling, it may happen that worker thread has not done anything yet or it is still working.
	// In this case it is necessary to setup "running" state. On the other hand it may happen that worker thread
	// decided to sop, posted WM_COMMNAND and this event is already processed on the UI thread. In this case
	// this method should do almost nothing.
	//

	if (processing_startup == FALSE || m_vpst == vpst_idle)
	{
		if (m_nam_tab_view_code != pntvc_none)
		{
			// Hide name tables. Do this before wakening up the bkgr thread.
			NameTableView_SetupRunningState();
		}
	}

	if (processing_startup == FALSE)
	{
		// Update suspension mask before resuming the thread.
		SetupConsoleMask(new_stt);

		if (new_stt == vpst_running_abort)
		{
			m_parser_console.AbortParsingProcess((m_vpst == vpst_stopped) ? TRUE : FALSE);
		}

		if (m_vpst == vpst_stopped || m_vpst == vpst_stopped_error)
		{
			TGenericParserConsoleEvent latest_evt = m_parser_console.GetLatestEvent();
			if (latest_evt == pcev_grpars_cfct_pickup)
			{
				WORD wAction = m_parser_console.GetSavedAction();
				if (wAction >= actb_conflict && wAction < actb_nested_result)
				{
					// Control box currently displays conflicting actions as radio buttons. Check if user decided to resolve
					// the conflict manualy or not.
					int iact = m_control_panel.GetCurrentConflictingActionSelection();
					if (iact >= 0)
					{
						TGrammarConflict &cfct = m_parser->m_grammar->conflicts[wAction-actb_conflict];
						m_parser_console.SetSavedAction(cfct.GetAction(iact));
					}
				}
			}

			m_latest_resume_time = CurrDateTime();
			m_parser_console.ResumeThread();
		}
	}

	if (processing_startup == FALSE || m_vpst == vpst_idle)
	{
		if (m_vpst != vpst_stopped_error)
		{
			// Prepare messages for displaying on the control panel.
			const wchar_t *stt_msg = NULL;
			switch (new_stt)
			{
				case vpst_running_step:
						stt_msg = L"Running (step mode) ...";
						break;

				case vpst_running_run:
						stt_msg = L"Running ...";
						break;

				case vpst_running_break:
						stt_msg = L"Waiting for break ...";
						break;

				case vpst_running_abort:
						stt_msg = L"Aborting ...";
						break;
			}

			// Update the window.
			m_vpst = new_stt;
			m_control_panel.SetVisibleState(m_vpst, pcev_none, L"State:", stt_msg);
		}
		else
		{
			// Assign the new state without updating the control panel.
			m_vpst = new_stt;
		}
	}

	UpdateDialogWindow();
}

void TLangViewVisualParserFrame::SetupFinalState(DWORD thread_exit_code, bool &parsing_success)
{
	// Fugure out parsing results and prepare messages.
	wchar_t buffer[128], *message = NULL, *details = NULL;
	wchar_t ext_dtls_buffer[280], *ext_details = NULL;
	parsing_success = FALSE;
	if (thread_exit_code != TRUE)
	{
		// The thread exit code can be only TRUE and FALSE. False is a generic error that indicates
		// either a thread starting problem or an app error during the parsing startup.
		message = L"Startup problem.";
	}
	else if (m_parser->CheckForCriticalError() == TRUE)
	{
		// Parsing was stopped by the critical error.
		message = (wchar_t*)m_parser->GetCriticalErrorDescription();
		swprintf(buffer, 128, L"Error code:  %d.", m_parser->GetCriticalErrorCode());
		details = buffer;
		if (m_parser->m_critical_error_details[0] != 0)
		{
			swprintf(ext_dtls_buffer, 280, L"Details:  %s", m_parser->m_critical_error_details);
			ext_details = ext_dtls_buffer;
		}
	}
	else if (m_parser->Scanner()->GetErrorsCount() != 0)
	{
		// Grammar succceded but parsing errors are present.
		message = L"Partial success.";
		swprintf(buffer, 128, L"Parsing errors:  %d.", m_parser->Scanner()->GetErrorsCount());
		details = buffer;
	}
	else
	{
		// Full success.
		message = L"Success.";
		parsing_success = TRUE;
	}

	// Update and display the major state.
	m_vpst = vpst_final;
	m_control_panel.SetVisibleState(m_vpst, pcev_parsing_complete, L"Result:", message, details, ext_details);
	m_proc_stt_panel.SetMajorStatus(L"Placing trees ...");
	m_proc_stt_panel.OpenConsole();

	// Display the parsing results.
	bool grammar_succeeded = UpdateParserStateSplitter(pcev_parsing_complete, thread_exit_code);
	if (grammar_succeeded == TRUE)
	{
		// Hide the parsing state details.
		m_pars_stt_details.ResetContents();
		m_lower_horz_dock.HideChildDialog(&m_pars_stt_details);
		m_src_view.SetupSpecialLocationMarker();
	}
	else
	{
		UpdateParsingStateDetails();
		UpdateParsingLocationMarker();
	}

	if (m_nam_tab_view_code != pntvc_none)
		NameTableView_SetupFinalState(grammar_succeeded);
	UpdateDialogWindow();

	m_proc_stt_panel.CloseConsole();
}

void TLangViewVisualParserFrame::LoadParsingSession(const wchar_t *session_root_fname)
{
	assert(m_parser != 0);

	// Update the window title.
	SetTitleTextFmt(L" Visual Parser - %s ", session_root_fname);

	m_nam_tab_view_code = m_parser->GetNamesTableViewerCode();
	if (m_nam_tab_view_code != pntvc_none)
	{
		// Open the name table viewers. They will be filled in later.
		NameTableView_LoadParsingSession();
	}

	// Open the parsing state details and the source code viewers.
	m_lower_horz_dock.ShowChildDialog(&m_pars_stt_details, FALSE);
	m_lower_horz_dock.ShowChildDialog(&m_src_view, FALSE);

	// Try to show the major source file in the source viewer.
	TFileInfo src_info;
	src_info.Clear();
	TLoadSaveResult load_res = TFileBodyHelper::LoadAsciiFileBody(src_info.info, session_root_fname);
	if (load_res == ldres_success)
	{
		// File was loaded.
		if (src_info.BuildLinesInfo() == TRUE)
		{
			// Pass prepared file into the child viewer.
			if (m_src_view.LoadContents(src_info, TRUE) == FALSE)
			{
				// File data was not accepted. Release it.
				src_info.ReleaseBasicFileInfo();
				src_info.ReleaseLinesInfo();
			}
		}
		else
		{
			MsgBox(L"Error building lines info for the major source file of the session.");
			src_info.ReleaseBasicFileInfo();
		}
	}
	else
	{
		ShowSourceFileLoadingProblem(load_res, session_root_fname);
	}
}

void TLangViewVisualParserFrame::UnloadParsingSession()
{
	TMidLevStoreReader *mid_lev_viewer_reader = m_mid_lev_viewer.DataBase();
	if (mid_lev_viewer_reader != NULL)
	{
		// Database should be detached from the viewer before releasing it.
		m_mid_lev_viewer.SetupDataBase(NULL);
		m_parser->m_scanner->CbkHandler().DisposeReaderObject(mid_lev_viewer_reader);
	}

	if (m_mid_lev_viewer.GetWnd() != NULL)
	{
		::DestroyWindow(m_mid_lev_viewer.GetWnd());
	}

	if (m_nam_tab_view_code != pntvc_none)
	{
		// Reset and hide the name tables in the right dock.
		NameTableView_UnloadParsingSession();
		m_nam_tab_view_code = pntvc_none;
	}

	// Release and hide child dialogs in the middle dock.
	TParserStateDockIterationHelper hlpr(this, m_middle_horz_dock);
	while (hlpr.m_curr_dialog != NULL)
		hlpr.HideAndDismissCurrDialog();

	// Hide pars stt details and the source code in the lower dock.
	m_pars_stt_details.ResetContents();
	m_lower_horz_dock.HideChildDialog(&m_pars_stt_details);
	m_src_view.ResetContents();
	m_lower_horz_dock.HideChildDialog(&m_src_view);

	// Resetup all lex item styles for empty grammar.
	for (int i=0; i<TLangViewLexItemStyle::NUM_DIM_GRADES; ++i)
		m_lex_items_styles[i].SetupLexItemsProps(NULL);

	// Update the main state and reset the parser.
	SetTitleText(L" Visual Parser ");
	m_vpst = vpst_idle;
	m_control_panel.SetVisibleState(m_vpst);
	m_parser->ResetParser();
	m_parser = NULL;
}

void TLangViewVisualParserFrame::SetupConsoleMask(TLangViewVisualParserState stt)
{
	// Process only those states that require certain state of the mask. State of the suspension mask
	// is not important for the rest of the debugger states.
	switch (stt)
	{
		case vpst_running_step:
				// Stop on step and run events.
				m_parser_console.SetupSuspensionMask(m_step_brks_mask | m_run_brks_mask);
				break;

		case vpst_running_run:
				// Stop on run events only.
				m_parser_console.SetupSuspensionMask(m_run_brks_mask);
				break;

		case vpst_running_break:
				// Stop on any event.
				m_parser_console.SetupSuspensionMask((DWORD)-1);
				break;

		case vpst_running_abort:
				// Do not stop on anything. Let the background thread to terminate. It should do this soon,
				// since the abort flag is set there.
				m_parser_console.SetupSuspensionMask(0);
				break;
	}
}

void TLangViewVisualParserFrame::UpdateSuspensionMasksFromControlPanel(bool run_mask, TGenericParserConsoleEvent mask_bit, bool new_maks_value)
{
	// Update the appropriate mask.
	DWORD &mask = (run_mask == TRUE) ? m_run_brks_mask : m_step_brks_mask;
	if (new_maks_value == TRUE)
		mask |=  (1 << mask_bit);
	else mask &= ~(1 << mask_bit);

	// Update console with the changed mask.
	SetupConsoleMask(m_vpst);
}

bool TLangViewVisualParserFrame::UpdateParserStateSplitter(TGenericParserConsoleEvent event, DWORD event_dtls)
{
	// This method displays the current state of the grammar based parser in the middle of the horz dock.
	assert(m_parser != NULL);
	TParserStateDockIterationHelper hlpr(this, m_middle_horz_dock);

	// Prepare the root layer lex_items style.
	bool root_style_updated = FALSE;
	if (m_lex_items_styles[0].GetGrammar() != m_parser->m_grammar)
	{
		// Style is not up to date.
		HDC hDC = ::GetDC(m_hDlg);
		assert(hDC != NULL);

		TGenericConsole cons;
		m_lex_items_styles[0].SetupLexItemsProps(m_parser->m_grammar, 0, FALSE, FALSE, TLangViewLexItemStyle::g_DefaultProfileName, &cons);
		m_lex_items_styles[0].SetupForDeviceContext(hDC);
		::ReleaseDC(m_hDlg, hDC);
		root_style_updated = TRUE;
	}

	// Check for the final state of the parser. This state should be displayed in a special way.
	if (m_vpst == vpst_final && m_parser->CheckForCriticalError() == FALSE && m_parser->m_nest_cstk == 0)
	{
		TSymbolStateItem *prog_item = m_parser->m_non_term_proto;
		if (prog_item != NULL && prog_item->m_symbol == m_parser->RootGrammar()->axioma_ident)
		{
			// The non term proto holds an axioma symbol.
			TSymbolStateItem *inp_sym = m_parser->GetInpSymPtr(0);
			if (m_parser->m_stack.IsEmpty() == TRUE && inp_sym != NULL && inp_sym->m_symbol == TLexToSymConverter::lt_eof)
			{
				// Parser succeeded the parsing. Hide all visible panels. They are not needed any more.
				while (hlpr.m_curr_dialog != NULL)
					hlpr.HideAndDismissCurrDialog();

				// Show only one panel with non term proto data.
				TLangViewAbstractSyntaxTreeViewer *viewer = AllocateSyntaxTreeViewer();
				if (viewer != NULL)
				{
					if (viewer->GetWnd() == NULL)
						viewer->OpenNonModalDialog(m_middle_horz_dock.GetWnd());

					// This viewer should contain only the syntax tree.
					viewer->SetupTree(plc_mode_concentric, &m_lex_items_styles[0], vplit_program, 0, GetSlotData(*prog_item), TRUE, FALSE);
					viewer->PrepareAndShowScreenItems(&m_proc_stt_panel);

					// Display this viewer as only one child.
					hlpr.ShowDialogBeforeCurr(*viewer, TRUE);
					return(TRUE);
				}
			}
		}
	}

	// Process the stack of the symbols and states.
	HCURSOR hPrevCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
	for (int istk=0; istk <= m_parser->m_nest_cstk; ++istk)
	{
		// Prepare pointer to the right list of items.
		TList *pstack = &(m_parser->m_stack);
		if (istk != m_parser->m_nest_cstk)
			pstack = &(m_parser->m_nest_stk[m_parser->m_nest_cstk-1].m_pars_stack);

		// Prepare the bkgr color grade and the lex items style.
		int istyle = m_parser->m_nest_cstk-istk;
		if (istyle > TLangViewLexItemStyle::MAX_DIM_GRADE)
			istyle = TLangViewLexItemStyle::MAX_DIM_GRADE;

		bool style_updated = FALSE;
		if (istyle != 0)
		{
			if (m_lex_items_styles[istyle].GetGrammar() != m_parser->m_nest_stk[istk].m_layer_grammar)
			{
				// Update the style.
				HDC hDC = ::GetDC(m_hDlg);
				assert(hDC != NULL);

				TGenericConsole cons;
				m_lex_items_styles[istyle].SetupLexItemsProps(m_parser->m_nest_stk[istk].m_layer_grammar, istyle, FALSE, FALSE, TLangViewLexItemStyle::g_DefaultProfileName, &cons);
				m_lex_items_styles[istyle].SetupForDeviceContext(hDC);
				::ReleaseDC(m_hDlg, hDC);
				style_updated = TRUE;
			}
		}
		else
		{
			style_updated = root_style_updated;
		}

		if (pstack->IsEmpty() == FALSE)
		{
			//
			// Stack of states and symbols on the curr layer is not empty.
			//

			// Hide all empty stack panels if any.
			while (hlpr.m_template_id == IDD_LANGV_VPARS_EMPTY_STACK)
				hlpr.HideAndDismissCurrDialog();

			// Loop over symbols in the stack of the parser.
			TListIter<TSymbolStateItem> iter(pstack);
			for (int isym=0; iter; ++iter, ++isym)
			{
				// Pick up the slot and create lexema item if it is not there yet.
				TSymbolStateItem &slot = iter.CurrItem();
				assert(slot.m_symbol < RuleObjectsBase);

				TLangViewTreeNode *slot_data = GetSlotData(slot);

				// Check if the current panel is compatible with the current parser stack slot or not.
				if (hlpr.m_syntax_tree_root_type == vplit_parser_stk_slot && hlpr.m_syntax_tree->GetRootIndex() == isym)
				{
					// Current panel has the same index.
					DWORD unq_sym_id = hlpr.m_syntax_tree->GetUniqueSymbolId();
					if (slot_data != NULL && slot_data->GetUniqueNodeId() == unq_sym_id || slot_data == NULL && unq_sym_id == 0)
					{
						// Panel is displaying the right tree. Check the upper labels. They can be changed by the app layer.
						if (CheckUpperLabelsState(hlpr.m_syntax_tree, slot) == TRUE)
						{
							// App level data is identical. Ensure the correct color grades.
							if (style_updated == TRUE || hlpr.m_syntax_tree->GetBkgrColor() != m_synt_tree_bkgr_color[istyle])
							{
								// Update all color elements.
								for (int ilabel=0; ilabel<TLangViewAbstractSyntaxTreeViewer::NUM_UPPER_LABELS; ++ilabel)
									hlpr.m_syntax_tree->UpdateUpperLabelStyle(ilabel, &m_synt_tree_upper_msgs_style[istyle], FALSE);

								hlpr.m_syntax_tree->UpdateLexItemsStyle(&m_lex_items_styles[istyle], FALSE);
								hlpr.m_syntax_tree->SetupBkgrColor(m_synt_tree_bkgr_color[istyle]);
							}

							// Ensure correct parsing state and its color.
							if (hlpr.m_syntax_tree->GetParsingState() != slot.m_parsing_state || hlpr.m_syntax_tree->GetParsSttStyle() != &m_synt_tree_pars_stt_style[istyle])
								hlpr.m_syntax_tree->SetupParsingState(slot.m_parsing_state, &m_synt_tree_pars_stt_style[istyle]);

							hlpr.ShiftToNextDialog();
							continue;
						}
					}
				}

				// Hide all parser stack panels if any.
				while (hlpr.m_syntax_tree_root_type == vplit_parser_stk_slot)
					hlpr.HideAndDismissCurrDialog();

				// Allocate a new panel.
				TLangViewAbstractSyntaxTreeViewer *panel = AllocateSyntaxTreeViewer();
				if (panel == NULL)
				{
					// Error is already displayed.
					continue;
				}

				// Prepare and show the panel.
				SetupUpperLabels(panel, slot, istyle);
				panel->SetupBkgrColor(m_synt_tree_bkgr_color[istyle], FALSE);
				panel->SetupParsingState(slot.m_parsing_state, &m_synt_tree_pars_stt_style[istyle], FALSE);
				panel->SetupTree((TTreeItemsPlacingMode)-1, &m_lex_items_styles[istyle], vplit_parser_stk_slot, isym, slot_data);
				hlpr.ShowDialogBeforeCurr(*panel, TRUE);
			}
		}
		else
		{
			// Parser stack on the curr layer is empty. Hide the stack slot panels if any.
			while (hlpr.m_syntax_tree_root_type == vplit_parser_stk_slot)
				hlpr.HideAndDismissCurrDialog();

			// Check type of the current panel.
			TLangViewVisualParserSupportPanel *panel = NULL;
			if (hlpr.m_template_id == IDD_LANGV_VPARS_EMPTY_STACK)
			{
				// Right panel is already there.
				panel = (TLangViewVisualParserSupportPanel*)hlpr.m_curr_dialog;
				hlpr.ShiftToNextDialog();
			}
			else
			{
				// Allocate and show the empty stack panel.
				panel = AllocateEmptyStackPanel();
				if (panel != NULL)
				{
					hlpr.ShowDialogBeforeCurr(*panel, FALSE);
				}
			}

			// Refresh props.
			if (panel != NULL)
			{
				panel->SetupNestingLayerProps(istk, m_synt_tree_bkgr_color[istyle]);
			}
		}
	}

	// The stack is processed. Restore the mouse cursor back.
	if (hPrevCursor != NULL)
		::SetCursor(hPrevCursor);

	// Stack of the parser could become shorter than its older representation on the screen. Hide extra panels.
	while (hlpr.m_syntax_tree_root_type == vplit_parser_stk_slot || hlpr.m_template_id == IDD_LANGV_VPARS_EMPTY_STACK)
		hlpr.HideAndDismissCurrDialog();

	// Process the current state of the parser. This panel should be present all the time.
	m_curr_pars_stt.SetupParsingState(m_parser->m_state, (event == pcev_parsing_complete) ? TRUE : FALSE);
	if (hlpr.m_template_id == IDD_LANGV_VPARS_CURRENT_STATE)
	{
		// Panel already belongs to the dock.
		hlpr.ShiftToNextDialog();
	}
	else
	{
		// Add the current state panel to the dock.
		if (m_curr_pars_stt.GetWnd() != NULL)
			m_curr_pars_stt.RestoreOriginalSize();
		hlpr.ShowDialogBeforeCurr(m_curr_pars_stt, FALSE);
	}

	// Prosess input queue.
	if (m_parser->m_inp_symbols.IsEmpty() == FALSE)
	{
		//
		// Input queue is not empty.
		//

		// Check if current view displays empty stack panel and hide this support panel if it is there.
		if (hlpr.m_template_id == IDD_LANGV_VPARS_EMPTY_INPUT)
			hlpr.HideAndDismissCurrDialog();

		// Loop over the symbols in the input queue.
		TListIter<TSymbolStateItem> iter(m_parser->m_inp_symbols);
		for (int inx=0; iter; ++iter, ++inx)
		{
			// Pick up the slot and create lexema item in it if it was not created there before.
			TSymbolStateItem &slot = iter.CurrItem();
			TLangViewTreeNode *slot_data = GetSlotData(slot);

			// Search for same or compatible panel among the existing panels.
			bool sym_processed = FALSE;
			while (hlpr.m_syntax_tree_root_type == vplit_parser_inp_sym)
			{
				DWORD unq_sym_id = hlpr.m_syntax_tree->GetUniqueSymbolId();
				if (slot_data != NULL && slot_data->GetUniqueNodeId() == unq_sym_id || slot_data == NULL && unq_sym_id == 0)
				{
					// App data level of the panel is the same to app data in the inp queue item. Ensure that panel has the same index.
					if (hlpr.m_syntax_tree->GetRootIndex() != inx)
						hlpr.m_syntax_tree->UpdateRootIndex(inx);

					hlpr.ShiftToNextDialog();
					sym_processed = TRUE;
					break;
				}
				else
				{
					// This is a different item. Hide this panel.
					hlpr.HideAndDismissCurrDialog();
				}
			}

			if (sym_processed == FALSE)
			{
				// New panel should be oppened.
				TLangViewAbstractSyntaxTreeViewer *panel = AllocateSyntaxTreeViewer();
				if (panel != NULL)
				{
					// Panel was allocated.
					panel->SetupTree((TTreeItemsPlacingMode)-1, &m_lex_items_styles[0], vplit_parser_inp_sym, inx, slot_data);
					hlpr.ShowDialogBeforeCurr(*panel, TRUE);
				}
			}
		}

		// Input queue of the parser could become shorter than its older representation on the screen. Hide the extra panels.
		while (hlpr.m_syntax_tree_root_type == vplit_parser_inp_sym)
			hlpr.HideAndDismissCurrDialog();
	}
	else
	{
		// The queue is empty. Hide input queue panels if any and show the empty queue panel if it is not already there.
		while (hlpr.m_syntax_tree_root_type == vplit_parser_inp_sym)
			hlpr.HideAndDismissCurrDialog();

		if (hlpr.m_template_id == IDD_LANGV_VPARS_EMPTY_INPUT)
		{
			// Panel already belongs to the dock.
			hlpr.ShiftToNextDialog();
		}
		else
		{
			// Add empty input panel to the dock.
			if (m_empty_input.GetWnd() != NULL)
				m_empty_input.RestoreOriginalSize();
			hlpr.ShowDialogBeforeCurr(m_empty_input, FALSE);
		}
	}

	// The code above should process all child dialogs.
	assert(hlpr.IsIterationComplete() == TRUE);
	return(FALSE);
}

void TLangViewVisualParserFrame::UpdateParsingStateDetails()
{
	// Update the current displayed state.
	if (m_parser->m_grammar != m_pars_stt_details.GetCurrGrammar() || m_parser->m_state != m_pars_stt_details.GetSingleItemModeObjIndex())
		m_pars_stt_details.ShowParsingStates(m_parser->m_grammar, m_parser->m_state);
}

void TLangViewVisualParserFrame::UpdateParsingLocationMarker()
{
	TFileInfo finfo = m_src_view.CurrFileInfo();
	finfo.info.file_name = (wchar_t*)m_src_view.CurrFileName();
	if (finfo.info.file_name != NULL && finfo.info.file_name[0] != 0)
	{
		// Some file is present in the source viewer.
		PrEventHeader *curr_pos = (m_parser != NULL) ? m_parser->GetCurrentLocation() : NULL;
		long curr_pos_offs = -1;
		if (curr_pos != NULL)
		{
			// It is necessary to figure out if current pos belongs to the file, that is currently displayed in the viewer or not.
			// However, the file from the source viewer does not belong to the parsing database. This means that the props
			// of these files should be compared, not the file identifiers.
			assert(curr_pos->src_area.file_info != NULL);
			if (TBasicFileInfo::Compare(&(finfo.info), (const TBasicFileInfo*)&(curr_pos->src_area.file_info->info)) == TRUE)
			{
				curr_pos_offs = curr_pos->src_area.area_beg;
			}
		}

		m_src_view.SetupSpecialLocationMarker(curr_pos_offs);
	}
}

void TLangViewVisualParserFrame::SetupUpperLabels(TLangViewAbstractSyntaxTreeViewer *panel, TSymbolStateItem &slot, int istyle)
{
	// Retrieve app description using the virtual method.
	TDestinationFile app_descr(L"AppDescr.XML");
	app_descr.PrepareInMemoryStream();
	if (slot.GetAppLayerDescription(app_descr) == FALSE)
		return;

	// App layer tells that description is available.
	if (app_descr.GetErrorFlag() == TRUE)
	{
		MsgBox(L"Error preparing the symbol app description.");
		return;
	}

	int iline = 0;
	wchar_t *descr_body = app_descr.GetBodyPtr();
	assert(descr_body != NULL);

	// Prepare and show the labels. Split app descriptions into lines and show them.
	while (*descr_body != 0 && iline < TLangViewAbstractSyntaxTreeViewer::NUM_UPPER_LABELS)
	{
		// Look for the end of the line.
		int cch = GetDescriptionLineLength(descr_body);

		// Show the current line and advance the loop.
		panel->SetupUpperLabel(iline++, &m_synt_tree_upper_msgs_style[istyle], descr_body, FALSE);
		descr_body += cch;
	}
}

bool TLangViewVisualParserFrame::CheckUpperLabelsState(TLangViewAbstractSyntaxTreeViewer *panel, TSymbolStateItem &slot)
{
	// Retrieve app description using the virtual method.
	TDestinationFile app_descr(L"AppDescr.XML");
	app_descr.PrepareInMemoryStream();

	int iline = 0;
	if (slot.GetAppLayerDescription(app_descr) == TRUE)
	{
		// App layer tells that description is available.
		if (app_descr.GetErrorFlag() == TRUE)
		{
			MsgBox(L"Error preparing the symbol app description.");
			return(FALSE);
		}

		wchar_t *descr_body = app_descr.GetBodyPtr();
		assert(descr_body != NULL);

		// Split app descriptions into lines and compare them to existing labels.
		while (*descr_body != 0 && iline < TLangViewAbstractSyntaxTreeViewer::NUM_UPPER_LABELS)
		{
			// Look for the end of the line.
			int cch = GetDescriptionLineLength(descr_body);

			// Compare lines.
			const wchar_t *existing_lab_body = panel->GetUpperLabelDataPtr(iline);
			if (existing_lab_body == NULL || wcscmp(existing_lab_body, descr_body) != 0)
				return(FALSE);

			// Advance the loop.
			iline++;
			descr_body += cch;
		}
	}

	// All lines strating from iline should be blank.
	while (iline < TLangViewAbstractSyntaxTreeViewer::NUM_UPPER_LABELS)
	{
		if (panel->GetUpperLabelDataPtr(iline++) != NULL)
			return(FALSE);
	}

	// Everything matched.
	return(TRUE);
}

int TLangViewVisualParserFrame::GetDescriptionLineLength(wchar_t *descr_body)
{
	// Note that this function is modifying the line.
	int cch = 1;
	for (;; ++cch)
	{
		if (descr_body[cch] == 0)
			break;

		if (descr_body[cch] == L'\r' || descr_body[cch] == L'\n')
		{
			descr_body[cch++] = 0;
			if (descr_body[cch] == L'\n')
				cch++;
			break;
		}
	}

	return(cch);
}

void TLangViewVisualParserFrame::NameTableView_InitChildDialogs()
{
	// Cpp language.
	m_cpp_global_namesp.SetDialogParams(this);
	m_cpp_global_namesp.SetOwner(this);

	// Common click details.
	m_cpp_item_details.SetDialogParams(this);
	m_cpp_item_details.SetOwner(this);

	// Clicked cpp items extra report.
	m_cpp_items_report.SetDialogParams(this);
	m_cpp_items_report.SetOwner(this);
}

bool TLangViewVisualParserFrame::NameTableView_OnInitDialog()
{
	//
	//  When this function is entered, the right vert splitter is already mounted.
	//  Add/remove the viewers there. Splitter will be dismounted outside.
	//

	// Cpp language.
	if (m_right_vert_dock.ShowChildDialog(&m_cpp_global_namesp, FALSE) == FALSE)
		return(FALSE);

	m_right_vert_dock.HideChildDialog(&m_cpp_global_namesp);

	// Extra click details and reports.
	if (m_right_vert_dock.ShowChildDialog(&m_cpp_item_details, FALSE) == FALSE)
		return(FALSE);
	if (m_right_vert_dock.ShowChildDialog(&m_cpp_items_report, FALSE) == FALSE)
		return(FALSE);

	m_right_vert_dock.HideChildDialog(&m_cpp_item_details);
	m_right_vert_dock.HideChildDialog(&m_cpp_items_report);

	// Success.
	return(TRUE);
}

void TLangViewVisualParserFrame::NameTableView_LoadParsingSession()
{
	// Add the right vert splitter that will be used by all languages.
	m_main_horz_dock.ShowChildDialog(&m_right_vert_dock, FALSE);

	assert(m_nam_tab_view_code != pntvc_none);
	switch (m_nam_tab_view_code)
	{
		case pntvc_cpp: // Cpp language.
		{
			m_right_vert_dock.ShowChildDialog(&m_cpp_global_namesp, FALSE);
			m_right_vert_dock.ShowChildDialog(&m_cpp_item_details, FALSE);
			m_right_vert_dock.ShowChildDialog(&m_cpp_items_report, FALSE);
		}
		break;
	}
}

void TLangViewVisualParserFrame::NameTableView_UnloadParsingSession()
{
	assert(m_nam_tab_view_code != pntvc_none);
	switch (m_nam_tab_view_code)
	{
		case pntvc_cpp: // Cpp language.
		{
			m_cpp_global_namesp.ResetContents();
			m_cpp_item_details.ResetContents();
			m_cpp_items_report.ResetContents();
			m_right_vert_dock.HideChildDialog(&m_cpp_global_namesp);
			m_right_vert_dock.HideChildDialog(&m_cpp_item_details);
			m_right_vert_dock.HideChildDialog(&m_cpp_items_report);
		}
		break;
	}

	// Finally hide the splitter itself.
	m_main_horz_dock.HideChildDialog(&m_right_vert_dock);
}

void TLangViewVisualParserFrame::NameTableView_SetupStoppedState()
{
	assert(m_nam_tab_view_code != pntvc_none);
	switch (m_nam_tab_view_code)
	{
		case pntvc_cpp: // Cpp language.
		{
			// Show the current state of the global namespace.
			TCppParser *cpp_parser = (TCppParser*)m_parser;
			m_cpp_global_namesp.LoadCppItems(cpp_parser->GetDatabase().GetGlobalNamespace());
		}
		break;
	}
}

void TLangViewVisualParserFrame::NameTableView_SetupRunningState()
{
	assert(m_nam_tab_view_code != pntvc_none);
	switch (m_nam_tab_view_code)
	{
		case pntvc_cpp: // Cpp language.
		{
			m_cpp_global_namesp.ResetContents();
			m_cpp_item_details.ResetContents();
			m_cpp_items_report.ResetContents();
		}
		break;
	}
}

void TLangViewVisualParserFrame::NameTableView_SetupFinalState(bool parsing_succeeded)
{
	assert(m_nam_tab_view_code != pntvc_none);
	switch (m_nam_tab_view_code)
	{
		case pntvc_cpp: // Cpp language.
		{
			// Global definitions are always displayed.
			TCppParser *cpp_parser = (TCppParser*)m_parser;
			m_cpp_global_namesp.LoadCppItems(cpp_parser->GetDatabase().GetGlobalNamespace());
		}
		break;
	}
}

bool TLangViewVisualParserFrame::CheckLexemaBreakPoint()
{
	// -BUGBUG- Not implemented yet.
	return(FALSE);
}

bool TLangViewVisualParserFrame::PrepareForProcessing(TParsingDriver &driver)
{
	if (m_vpst == vpst_final)
	{
		// Finish the old session without prompting the user.
		UnloadParsingSession();
	}

	if (m_vpst != vpst_idle || m_parser != NULL)
	{
		MsgBox(L"Previous parsing session is still active.");
		return(FALSE);
	}

	if (driver.Parser().GetParserIdentificationCode() != TGrammarBasedParser::ParserIdentCode)
	{
		MsgBox(L"Passed parser is not a TGrammarBasedParser.");
		return(FALSE);
	}

	// Setup the data fields.
	driver.Scanner().MidLevReset(TRUE, TRUE);
	driver.Parser().SetupParserConsole(&m_parser_console);
	m_parser = (TGrammarBasedParser*)&driver.Parser();
	m_lex_item_ids_cnt = 0;

	// Success.
	return(TRUE);
}

TLangViewAbstractSyntaxTreeViewer *TLangViewVisualParserFrame::AllocateSyntaxTreeViewer()
{
	// Check for the look aside panels.
	TLangViewAbstractSyntaxTreeViewer *second_hand_panel = m_stv_lookaside_list.GetItemNoAlloc();
	if (second_hand_panel != NULL)
	{
		if (second_hand_panel->GetWnd() != NULL)
			second_hand_panel->RestoreOriginalSize();
		return(second_hand_panel);
	}

	// Allocate new panel on the heap.
	TLangViewAbstractSyntaxTreeViewer *new_panel = new TLangViewAbstractSyntaxTreeViewer();
	if (new_panel != NULL)
	{
		new_panel->SetDialogParams(this);
		new_panel->SetOwner(this);
		return(new_panel);
	}
	else
	{
		MsgBox(L"Error creating TLangViewAbstractSyntaxTreeViewer.");
		return(NULL);
	}
}

TLangViewVisualParserSupportPanel *TLangViewVisualParserFrame::AllocateEmptyStackPanel()
{
	// Allocate new panel on the heap.
	TLangViewVisualParserSupportPanel *panel = new TLangViewVisualParserSupportPanel();
	if (panel != NULL)
	{
		panel->SetDialogParams(this, IDD_LANGV_VPARS_EMPTY_STACK);
	}
	else
	{
		MsgBox(L"Error creating TLangViewAbstractSyntaxTreeViewer.");
	}
	return(panel);
}

void TLangViewVisualParserFrame::RecycleSyntaxTreeViewer(TLangViewAbstractSyntaxTreeViewer *viewer)
{
	viewer->ResetContents();
	m_stv_lookaside_list.RecycleItem(viewer);
}

void TLangViewVisualParserFrame::ShowAsyncMessage(const wchar_t *message)
{
	if (m_hDlg != NULL)
	{
		::PostMessage(m_hDlg, WM_COMMAND, IDC_VPARS_ASYNC_MESSAGE1, (LPARAM)message);
	}
}

void TLangViewVisualParserFrame::ShowSourceFileLoadingProblem(TLoadSaveResult load_res, const wchar_t *fname, bool forceDisplay)
{
	assert(load_res != ldres_success);

	if (m_hDlg != NULL && (::IsWindowVisible(m_hDlg) == TRUE) || forceDisplay == TRUE)
	{
		// Display this message immediately.
		if (load_res == ldres_fname_missing || fname == NULL || fname[0] == 0)
		{
			// Show special message for this error.
			MsgBox(L"Unable to load source code file into the viewer because file name is missing or it is empty.");
		}
		else
		{
			// Show general loading error.
			MsgBoxFmt(MB_OK, L"Unable to load source code file into the viewer.\r\n\r\n    File name:  %s\r\n\r\n    Problem:  %s.",
						fname, TFileBodyHelper::GetLoadSaveResultText(load_res));
		}
	}
	else
	{
		// Queue this error message for displaying it later.
		m_source_file_load_res = load_res;
		wcscpy(m_source_file_name, fname);
		::PostMessage(m_hDlg, WM_COMMAND, IDC_VPARS_ASYNC_MESSAGE2, 0);
	}
}

// --------------------------------------------------------------------
//   ===========  TParserStateDockIterationHelper  =================
// --------------------------------------------------------------------

void TLangViewVisualParserFrame::TParserStateDockIterationHelper::Reset()
{
	m_inx = 0;
	m_num_dialogs = m_dock.NumDialogs();
	PickUpCurrentDialog();
}

void TLangViewVisualParserFrame::TParserStateDockIterationHelper::ShiftToNextDialog()
{
	if (m_inx < m_num_dialogs)
	{
		// Iteration is not finished yet.
		m_inx++;
	}

	// Call the pickup function in any case.
	PickUpCurrentDialog();
}

void TLangViewVisualParserFrame::TParserStateDockIterationHelper::ShowDialogBeforeCurr(TBasicDialog &dlg, bool bypass_ownership)
{
	if (m_dock.ShowChildDialog(&dlg, bypass_ownership, -1, m_curr_dialog) == FALSE)
	{
		m_owner->MsgBox(L"Error displaying child dialog in the parser state dock.");
		if (bypass_ownership == TRUE)
		{
			// Ownership was bypassed and the dock has not accepted it. Relase the resource.
			delete &dlg;
		}
		return;
	}

	m_inx++;
	m_num_dialogs++;
	PickUpCurrentDialog();
}

void TLangViewVisualParserFrame::TParserStateDockIterationHelper::HideAndDismissCurrDialog()
{
	assert(m_curr_dialog != NULL);
	bool isOwned = m_dock.GetDialogOwnedFlag(m_inx);

	if (m_dock.HideChildDialog(m_curr_dialog) == FALSE)
	{
		m_owner->MsgBox(L"Error hiding child dialog in the parser state dock.");
		ShiftToNextDialog();
		return;
	}

	if (isOwned == TRUE)
	{
		// This dialog needs dismissing.
		if (m_template_id == IDD_LANGV_VPARS_SYNTAX_TREE)
		{
			m_owner->RecycleSyntaxTreeViewer(m_syntax_tree);
		}
		else
		{
			delete m_curr_dialog;
		}
	}

	m_num_dialogs--;
	PickUpCurrentDialog();
}

TBasicDialog *TLangViewVisualParserFrame::TParserStateDockIterationHelper::RemoveCurrDialog(bool &ownership_returned)
{
	assert(m_curr_dialog != NULL);
	ownership_returned = m_dock.GetDialogOwnedFlag(m_inx);

	if (m_dock.HideChildDialog(m_curr_dialog) == FALSE)
	{
		m_owner->MsgBox(L"Error removing child dialog from the parser state dock.");
		ShiftToNextDialog();
		return(NULL);
	}

	TBasicDialog *dlg = m_curr_dialog;
	m_num_dialogs--;
	PickUpCurrentDialog();
	return(dlg);
}

void TLangViewVisualParserFrame::TParserStateDockIterationHelper::PickUpCurrentDialog()
{
	m_syntax_tree = NULL;
	m_syntax_tree_root_type = vplit_num_types;

	if (m_inx < m_num_dialogs)
	{
		// Pick up the raw dialog from the dock.
		m_curr_dialog = m_dock.GetChildDialog(m_inx);
		assert(m_curr_dialog != NULL);

		m_template_id = m_curr_dialog->GetDialogTemplateId();

		switch (m_template_id)
		{
			case IDD_LANGV_VPARS_SYNTAX_TREE:
					{
						// Current panel is syntax tree viewer.
						m_syntax_tree = (TLangViewAbstractSyntaxTreeViewer*)m_curr_dialog;
						m_syntax_tree_root_type = m_syntax_tree->GetRootType();
					}
					break;
		}
	}
	else
	{
		// End of iteration or there are no dialogs in the dock at all.
		m_curr_dialog = NULL;
		m_template_id = 0;
	}
}


