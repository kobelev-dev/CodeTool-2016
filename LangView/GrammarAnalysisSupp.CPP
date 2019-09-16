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
#include  "LangView/GrammarAnalysisSupp.H"
#include  "LangView/LangViewResources.H"

// -------------------------------------------------------------------------------
//  ================  TLangViewEditConflictAnalysisProps  ===================
// -------------------------------------------------------------------------------

TLangViewEditConflictAnalysisProps::TLangViewEditConflictAnalysisProps(TGrammar &root_grm, TLangViewProcessGrammarProps &props, bool allow_cfct_indexes_sets, bool allow_iter_props_changes)
		: TBasicDialog(NULL, IDD_LANGV_ANPROC_EDIT_ANPROPS), m_grammar(root_grm), m_props(props)
{
	// The passed grammar should be a root grammar.
	assert(m_grammar.parent_grammar == NULL);

	m_allow_cfct_indexes_sets = allow_cfct_indexes_sets;
	m_allow_iter_props_changes = allow_iter_props_changes;
}

LRESULT TLangViewEditConflictAnalysisProps::OnInitDialog(HWND hDefFocusControl)
{
	// Prepare the dialog itself.
	CenterOverTheParent();

	// Setup the grammar/conflict/action selection.
	LoadActionSelectionControls();

	// Show 2 boolean flags.
	SetCheckMark(IDC_EDIT_ANP_USE_ALL_RULES, m_props.m_anprops.m_use_all_non_term_defn_rules);
	SetCheckMark(IDC_EDIT_ANP_SEARCH_FOR_CTX, m_props.m_anprops.m_search_for_non_empty_ctx);

	// Show 3 integer props.
	SetControlTextFmt(IDC_EDIT_ANP_MAX_PATH_LEN, L"%d", m_props.m_anprops.m_max_path_length);
	SetControlTextFmt(IDC_EDIT_ANP_MAX_PATH_LEN_NOTE, L"The number of steps should be in the range  4 ... %d.", TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN);
	SetControlTextFmt(IDC_EDIT_ANP_MAX_NUM_PATHS, L"%d", m_props.m_anprops.m_max_num_paths);
	SetControlTextFmt(IDC_EDIT_ANP_ACT_PROC_TO, L"%d", m_props.m_anprops.m_action_proc_timeout);

	if (m_allow_iter_props_changes == FALSE)
	{
		// Disable controls.
		EnableControl(IDC_EDIT_ANP_USE_ALL_RULES, FALSE);
		EnableControl(IDC_EDIT_ANP_SEARCH_FOR_CTX, FALSE);
		EnableControl(IDC_EDIT_ANP_MAX_PATH_LEN_LAB, FALSE);
		EnableControl(IDC_EDIT_ANP_MAX_PATH_LEN, FALSE);
		EnableControl(IDC_EDIT_ANP_MAX_PATH_LEN_NOTE, FALSE);
		EnableControl(IDC_EDIT_ANP_MAX_NUM_PATHS_LAB, FALSE);
		EnableControl(IDC_EDIT_ANP_MAX_NUM_PATHS, FALSE);
		EnableControl(IDC_EDIT_ANP_MAX_NUM_PATHS_NOTE, FALSE);
		EnableControl(IDC_EDIT_ANP_ACT_PROC_TO_LAB, FALSE);
		EnableControl(IDC_EDIT_ANP_ACT_PROC_TO, FALSE);
		EnableControl(IDC_EDIT_ANP_ACT_PROC_TO_NOTE, FALSE);
	}

	// The focus should not be set anywhere.
	SetFocusControl(IDCANCEL);
	return(FALSE);
}

void TLangViewEditConflictAnalysisProps::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_EDIT_ANP_ALL_GRAMMARS:
		case IDC_EDIT_ANP_SINGLE_GRAMMAR:
				{
					SetupGrammarTypeSelection(GetCheckMark(IDC_EDIT_ANP_SINGLE_GRAMMAR));
				}
				break;

		case IDC_EDIT_ANP_ALL_CONFLICTS:
		case IDC_EDIT_ANP_CFCTS_SINGLE_XPCT:
		case IDC_EDIT_ANP_CFCTS_DIRECT_LIST:
		case IDC_EDIT_ANP_SINGLE_CONFLICT:
				{
					SetupConflictTypeSelection(TRUE);
				}
				break;

		case IDC_EDIT_ANP_ALL_ACTIONS:
		case IDC_EDIT_ANP_SINGLE_ACTION:
				{
					SetupActionTypeSelection(GetCheckMark(IDC_EDIT_ANP_SINGLE_ACTION));
				}
				break;

		case IDOK:
				{
					// Check, if conflicts processing is possible or not.
					if (m_grammar.grm_props.processing_result != grpr_full_success)
					{
						MsgBox(L"It is not possible to process conflicts because the current grammar is either empty or it contains syntax/conversion errors.");
						break;
					}
					else if (m_grammar.NumConflicts() == 0)
					{
						MsgBox(L"It is not possible to process conflicts because the current grammar does not have conflicts.");
						break;
					}

					// Save the data.
					if (CheckSaveData() == FALSE)
						break;

					// Close the dialog with the return value TRUE. This will initiate processing of the conflicts.
					::EndDialog(m_hDlg, TRUE);
				}
				break;

		case IDCANCEL:
				{
					// Simply close the dialog.
					::EndDialog(m_hDlg, FALSE);
				}
				break;
	}
}

bool TLangViewEditConflictAnalysisProps::EditAnalysisProps(HWND hParent)
{
	return((OpenModalDialog(hParent) == TRUE) ? TRUE : FALSE);
}

bool TLangViewEditConflictAnalysisProps::GetIntValue(TBasicDialog *inst, int rcId, int &value, const wchar_t *prefix_chars_to_ignore)
{
	wchar_t buff[80];
	inst->GetControlText(rcId, buff, 80);

	wchar_t *pbuff = buff;
	if (prefix_chars_to_ignore != NULL)
	{
		// Ignore prefix characters in the input data.
		while (*pbuff != 0 && wcschr(prefix_chars_to_ignore, *pbuff) != NULL)
			pbuff++;
	}

	__int64 val;
	if (AssembleInt64(pbuff, val, anxvt_signed) == FALSE)
		return(FALSE);

	if (val < -1000000000 || val > 1000000000)
		return(FALSE);

	value = (int)val;
	return(TRUE);
}

void TLangViewEditConflictAnalysisProps::LoadActionSelectionControls()
{
	// All/one grammar, all/one conflict, all/one action props.
	if (m_props.m_antask.m_only_igrammar < 0)
	{
		// Setup for all grammars.
		SetCheckMark(IDC_EDIT_ANP_ALL_GRAMMARS, TRUE);
		EnableControl(IDC_EDIT_ANP_GRAMMAR_INX_LAB, FALSE);
		EnableControl(IDC_EDIT_ANP_GRAMMAR_INX, FALSE);

		EnableControl(IDC_EDIT_ANP_ALL_CONFLICTS, FALSE);
		EnableControl(IDC_EDIT_ANP_CFCTS_SINGLE_XPCT, FALSE);
		EnableControl(IDC_EDIT_ANP_CFCTS_DIRECT_LIST, FALSE);
		EnableControl(IDC_EDIT_ANP_SINGLE_CONFLICT, FALSE);
		EnableControl(IDC_EDIT_ANP_CONFLICT_INX_LAB, FALSE);
		EnableControl(IDC_EDIT_ANP_CONFLICT_INX, FALSE);

		EnableControl(IDC_EDIT_ANP_ALL_ACTIONS, FALSE);
		EnableControl(IDC_EDIT_ANP_SINGLE_ACTION, FALSE);
		EnableControl(IDC_EDIT_ANP_ACTION_INX_LAB, FALSE);
		EnableControl(IDC_EDIT_ANP_ACTION_INX, FALSE);
	}
	else
	{
		// Set the checkbox and show the grammar index.
		SetCheckMark(IDC_EDIT_ANP_SINGLE_GRAMMAR, TRUE);
		SetControlTextFmt(IDC_EDIT_ANP_GRAMMAR_INX, L"%d", m_props.m_antask.m_only_igrammar);

		EnableControl(IDC_EDIT_ANP_CFCTS_SINGLE_XPCT, m_allow_cfct_indexes_sets);
		EnableControl(IDC_EDIT_ANP_CFCTS_DIRECT_LIST, m_allow_cfct_indexes_sets);

		if (m_allow_cfct_indexes_sets == TRUE && m_props.m_antask.m_only_iconflict <= -3)
		{
			// Setup for single expected conflict.
			SetCheckMark(IDC_EDIT_ANP_CFCTS_SINGLE_XPCT, TRUE);
			SetControlTextFmt(IDC_EDIT_ANP_CONFLICT_INX, L"%d", -(3+m_props.m_antask.m_only_iconflict));
		}
		else if (m_allow_cfct_indexes_sets == TRUE && m_props.m_antask.m_only_iconflict == -2)
		{
			// Setup for explicit list of grammar conflicts.
			SetCheckMark(IDC_EDIT_ANP_CFCTS_DIRECT_LIST, TRUE);
			TMessageBuffer buff;
			for (int icon=0; icon<m_props.m_antask.m_cfct_indexes_list.NumItems(); ++icon)
			{
				wchar_t icon_buff[80];
				swprintf(icon_buff, 80, L"C%hd%s", m_props.m_antask.m_cfct_indexes_list[icon], (icon != m_props.m_antask.m_cfct_indexes_list.NumItems()-1) ? L", " : L".");
				buff.Append(icon_buff);
			}

			SetControlTextFmt(IDC_EDIT_ANP_CONFLICT_INX, buff.DataPtr());
		}
		else if (m_props.m_antask.m_only_iconflict < 0)
		{
			// Setup for all grammar conflicts.
			SetCheckMark(IDC_EDIT_ANP_ALL_CONFLICTS, TRUE);
		}
		else
		{
			// Setup for single grammar conflict.
			SetCheckMark(IDC_EDIT_ANP_SINGLE_CONFLICT, TRUE);
			SetControlTextFmt(IDC_EDIT_ANP_CONFLICT_INX, L"%d", m_props.m_antask.m_only_iconflict);

			// Setup the action selection controls.
			if (m_props.m_antask.m_only_iaction >= 0)
			{
				// Set the checkbox and show the action index.
				SetCheckMark(IDC_EDIT_ANP_SINGLE_ACTION, TRUE);
				SetControlTextFmt(IDC_EDIT_ANP_ACTION_INX, L"%d", m_props.m_antask.m_only_iaction);
			}
		}

		// Finally refresh the dependent controls.
		SetupConflictTypeSelection(FALSE);
	}
}

void TLangViewEditConflictAnalysisProps::SetupGrammarTypeSelection(bool bOneGrammar)
{
	EnableControl(IDC_EDIT_ANP_GRAMMAR_INX_LAB, bOneGrammar);
	EnableControl(IDC_EDIT_ANP_GRAMMAR_INX, bOneGrammar);

	EnableControl(IDC_EDIT_ANP_ALL_CONFLICTS, bOneGrammar);
	EnableControl(IDC_EDIT_ANP_CFCTS_SINGLE_XPCT, bOneGrammar && m_allow_cfct_indexes_sets);
	EnableControl(IDC_EDIT_ANP_CFCTS_DIRECT_LIST, bOneGrammar && m_allow_cfct_indexes_sets);
	EnableControl(IDC_EDIT_ANP_SINGLE_CONFLICT, bOneGrammar);

	if (bOneGrammar == TRUE)
	{
		SetFocusControl(IDC_EDIT_ANP_GRAMMAR_INX);

		wchar_t buff[80];
		GetControlText(IDC_EDIT_ANP_GRAMMAR_INX, buff, 80);
		if (buff[0] == 0)
		{
			SetControlText(IDC_EDIT_ANP_GRAMMAR_INX, L"0");
			::SendDlgItemMessage(m_hDlg, IDC_EDIT_ANP_GRAMMAR_INX, EM_SETSEL, 1, 1);
		}

		if (GetCheckMark(IDC_EDIT_ANP_ALL_CONFLICTS) == FALSE && GetCheckMark(IDC_EDIT_ANP_CFCTS_SINGLE_XPCT) == FALSE && GetCheckMark(IDC_EDIT_ANP_CFCTS_DIRECT_LIST) == FALSE && GetCheckMark(IDC_EDIT_ANP_SINGLE_CONFLICT) == FALSE)
		{
			// All radio buttons of the group are unselected. Select the first one.
			SetCheckMark(IDC_EDIT_ANP_ALL_CONFLICTS, TRUE);
		}

		SetupConflictTypeSelection(FALSE);
	}
	else
	{
		EnableControl(IDC_EDIT_ANP_CONFLICT_INX_LAB, FALSE);
		EnableControl(IDC_EDIT_ANP_CONFLICT_INX, FALSE);
		EnableControl(IDC_EDIT_ANP_ALL_ACTIONS, FALSE);
		EnableControl(IDC_EDIT_ANP_SINGLE_ACTION, FALSE);
		SetupActionTypeSelection(FALSE);
	}
}

void TLangViewEditConflictAnalysisProps::SetupConflictTypeSelection(bool set_focus_to_grm_inx_entry_field)
{
	bool bOneConflict = GetCheckMark(IDC_EDIT_ANP_SINGLE_CONFLICT);
	EnableControl(IDC_EDIT_ANP_ALL_ACTIONS, bOneConflict);
	EnableControl(IDC_EDIT_ANP_SINGLE_ACTION, bOneConflict);

	if (GetCheckMark(IDC_EDIT_ANP_ALL_CONFLICTS) == TRUE)
	{
		// Disable the conflict index controls.
		EnableControl(IDC_EDIT_ANP_CONFLICT_INX_LAB, FALSE);
		EnableControl(IDC_EDIT_ANP_CONFLICT_INX, FALSE);
		SetControlText(IDC_EDIT_ANP_CONFLICT_INX_LAB, L"Conflict index:");
		SetupActionTypeSelection(FALSE);
	}
	else
	{
		// Enable the conflict index controls.
		EnableControl(IDC_EDIT_ANP_CONFLICT_INX_LAB, TRUE);
		EnableControl(IDC_EDIT_ANP_CONFLICT_INX, TRUE);

		if (set_focus_to_grm_inx_entry_field == TRUE)
			SetFocusControl(IDC_EDIT_ANP_CONFLICT_INX);

		if (GetCheckMark(IDC_EDIT_ANP_CFCTS_SINGLE_XPCT) == TRUE)
		{
			SetControlText(IDC_EDIT_ANP_CONFLICT_INX_LAB, L"Xpct index:");
			SetupActionTypeSelection(FALSE);
		}
		else if (GetCheckMark(IDC_EDIT_ANP_CFCTS_DIRECT_LIST) == TRUE)
		{
			SetControlText(IDC_EDIT_ANP_CONFLICT_INX_LAB, L"List of indexes:");
			SetupActionTypeSelection(FALSE);
		}
		else
		{
			SetControlText(IDC_EDIT_ANP_CONFLICT_INX_LAB, L"Conflict index:");
			SetupActionTypeSelection(GetCheckMark(IDC_EDIT_ANP_SINGLE_ACTION));

			if (GetCheckMark(IDC_EDIT_ANP_ALL_ACTIONS) == FALSE && GetCheckMark(IDC_EDIT_ANP_SINGLE_ACTION) == FALSE)
				SetCheckMark(IDC_EDIT_ANP_ALL_ACTIONS, TRUE);
		}
	}
}

void TLangViewEditConflictAnalysisProps::SetupActionTypeSelection(bool bOneAction)
{
	EnableControl(IDC_EDIT_ANP_ACTION_INX_LAB, bOneAction);
	EnableControl(IDC_EDIT_ANP_ACTION_INX, bOneAction);

	if (bOneAction == TRUE)
		SetFocusControl(IDC_EDIT_ANP_ACTION_INX);
}

bool TLangViewEditConflictAnalysisProps::CheckSaveData()
{
	// Create a local copy of the task.
	TLangViewProcessGrammarProps loc_task;
	loc_task.SetDefaultValues();

	// All/one grammar, all/some/one conflict, all/one action.
	if (GetCheckMark(IDC_EDIT_ANP_SINGLE_GRAMMAR) == TRUE)
	{
		// This is setup for processing all or some of the conflicts in a single grammar.
		int num_grammars =  m_grammar.GetNumGrammars();
		if (GetIntValue(this, IDC_EDIT_ANP_GRAMMAR_INX, loc_task.m_antask.m_only_igrammar, L" gG") == FALSE || loc_task.m_antask.m_only_igrammar < 0)
		{
			MsgBox(L"Error getting the value of the grammar index.");
			return(FALSE);
		}
		else if (loc_task.m_antask.m_only_igrammar >= num_grammars)
		{
			MsgBoxFmt(MB_OK, L"The value of the grammar index is too big. Current tree of grammars contains only %d grammar%s.", num_grammars, PluralSuffix(num_grammars));
			return(FALSE);
		}

		// The index of the grammar is valid. Pick up the grammar itself.
		TGrammar *curr_grammar = m_grammar.GetGrammarByIndex(loc_task.m_antask.m_only_igrammar);

		// All/some/one conflict, all/one action.
		if (GetCheckMark(IDC_EDIT_ANP_CFCTS_SINGLE_XPCT) == TRUE)
		{
			// This is setup for a single expected conglict.
			int ixpct, num_xpcts = curr_grammar->NumXpctConflicts();
			if (GetIntValue(this, IDC_EDIT_ANP_CONFLICT_INX, ixpct, L" xX") == FALSE || ixpct < 0)
			{
				MsgBox(L"Error getting the value of the expected conflict index.");
				return(FALSE);
			}
			else if (ixpct >= num_xpcts)
			{
				MsgBoxFmt(MB_OK, L"The index of the expected conflict is too big. The selected grammar contains only %d expected conflict%s.", num_xpcts, PluralSuffix(num_xpcts));
				return(FALSE);
			}

			// The value of the xpct index is fine.
			loc_task.m_antask.m_only_iconflict = -3-ixpct;
			loc_task.m_antask.m_only_iaction = -1;
		}
		else if (GetCheckMark(IDC_EDIT_ANP_CFCTS_DIRECT_LIST) == TRUE)
		{
			// This is setup for a list of the grammar conflict indexes.
			if (AssembleListOfConflictIndexes(curr_grammar, loc_task.m_antask.m_cfct_indexes_list) == FALSE)
			{
				// Message box with and error is already dispalyed.
				return(FALSE);
			}

			// The list of the grammar conflict indexes is fine.
			loc_task.m_antask.m_only_iconflict = -2;
			loc_task.m_antask.m_only_iaction = -1;
		}
		else if (GetCheckMark(IDC_EDIT_ANP_SINGLE_CONFLICT) == TRUE)
		{
			// This is setup for processing a single grammar conflict.
			int num_conflicts = curr_grammar->NumConflicts();
			if (GetIntValue(this, IDC_EDIT_ANP_CONFLICT_INX, loc_task.m_antask.m_only_iconflict, L" cC") == FALSE || loc_task.m_antask.m_only_iconflict < 0)
			{
				MsgBox(L"Error getting the value of the grammar conflict index. Note that the current selection expects an index of only one conflict.");
				return(FALSE);
			}
			else if (loc_task.m_antask.m_only_iconflict >= num_conflicts)
			{
				MsgBoxFmt(MB_OK, L"The index of the grammar conflict is too big. The selected grammar contains only %d conflict%s.", num_conflicts, PluralSuffix(num_conflicts));
				return(FALSE);
			}

			// The index of the conflict is valid. Pick up the description conflict.
			TGrammarConflict &curr_conflict = curr_grammar->conflicts[loc_task.m_antask.m_only_iconflict];

			// All/one conflicting action.
			if (GetCheckMark(IDC_EDIT_ANP_SINGLE_ACTION) == TRUE)
			{
				// This is setup for processing a single action.
				int num_actions = curr_conflict.NumActions();
				if (GetIntValue(this, IDC_EDIT_ANP_ACTION_INX, loc_task.m_antask.m_only_iaction, L" aA") == FALSE || loc_task.m_antask.m_only_iaction < 0)
				{
					MsgBox(L"Error getting the value of the conflicting action index.");
					return(FALSE);
				}
				else if (loc_task.m_antask.m_only_iaction >= num_actions)
				{
					MsgBoxFmt(MB_OK, L"The index of the conflicting action is too big. The selected grammar conflict contains only %d action%s.", num_actions, PluralSuffix(num_actions));
					return(FALSE);
				}
			}
		}
	}

	if (m_allow_iter_props_changes == TRUE)
	{
		// Pick up 2 boolean flags.
		loc_task.m_anprops.m_use_all_non_term_defn_rules = GetCheckMark(IDC_EDIT_ANP_USE_ALL_RULES);
		loc_task.m_anprops.m_search_for_non_empty_ctx = GetCheckMark(IDC_EDIT_ANP_SEARCH_FOR_CTX);

		// Pick up the maximum derivation path length.
		int path_length;
		if (GetIntValue(this, IDC_EDIT_ANP_MAX_PATH_LEN, path_length) == FALSE || path_length < 0 || (path_length > 0 && path_length < 4) || path_length > TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN)
		{
			MsgBoxFmt(MB_OK, L"Error getting the value of the maximum path length.\r\n\r\nThe path length can be either 0 or it should be in the range  4 ... %d.", TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN);
			return(FALSE);
		}

		loc_task.m_anprops.m_max_path_length = path_length;

		// Pick up the maximum number of derivation paths.
		int num_paths;
		if (GetIntValue(this, IDC_EDIT_ANP_MAX_NUM_PATHS, num_paths) == FALSE || num_paths < 0)
		{
			MsgBoxFmt(MB_OK, L"Error getting the value of the maximum allowed number of paths.");
			return(FALSE);
		}

		loc_task.m_anprops.m_max_num_paths = num_paths;

		// Pick up the action processing timeout.
		int proc_timeout, max_timeout = 365*24*3600;
		if (GetIntValue(this, IDC_EDIT_ANP_ACT_PROC_TO, proc_timeout) == FALSE || proc_timeout < 0 || proc_timeout > max_timeout)
		{
			MsgBoxFmt(MB_OK, L"Error getting the value of the conflicting action processing timeout.\r\n\r\nThe timeout value should be in the range  0 ... %ld  (one year).", max_timeout);
			return(FALSE);
		}

		loc_task.m_anprops.m_action_proc_timeout = proc_timeout;
	}
	else
	{
		// Pick up the original props as they were passed to the ctor.
		loc_task.m_anprops = m_props.m_anprops;
	}

	// All checks have succeeded.
	m_props.TakeDataFrom(loc_task);
	return(TRUE);
}

bool TLangViewEditConflictAnalysisProps::AssembleListOfConflictIndexes(TGrammar *curr_grammar, TSymbolsArray &res_array)
{
	// Pick up the length and reserve space for this text.
	TMessageBuffer buff;
	int text_len = ::SendDlgItemMessage(m_hDlg, IDC_EDIT_ANP_CONFLICT_INX, WM_GETTEXTLENGTH, 0, 0);
	if (text_len <= 0)
	{
		MsgBox(L"The edit control is empty. It should contain a list of the conflict indexes.");
		return(FALSE);
	}
	else if (buff.ReserveSpace(text_len) == FALSE)
	{
		MsgBoxFmt(MB_OK, L"The text in the edit control is too long (TextLen=%d).", text_len);
		return(FALSE);
	}

	// The buffer is allocated. Pick up the data.
	GetControlText(IDC_EDIT_ANP_CONFLICT_INX, buff.DataPtr(), buff.NumAllocedItems());

	// Loop over the picked characters.
	int inx = 0;
	int printable_area_beg = 0;
	int num_conflicts = curr_grammar->NumConflicts();
	while (inx< text_len)
	{
		wchar_t ch = buff[inx++];
		if (ch == L'\t' || ch == L'\n' || ch == L'\r')
		{
			printable_area_beg = inx;
			continue;
		}
		else if (ch == 0)
		{
			break;
		}

		// This should be some printable character.
		if (ch == L'c' || ch == L'C' || ch == L' ' || ch == L',' || ch == L'.')
		{
			// Ignore these characters.
			continue;
		}
		else if (ch >= L'0' && ch <= L'9')
		{
			// This is a beginning of the number.
			int value = ch-L'0';
			while (inx< text_len)
			{
				wchar_t ch = buff[inx];
				if (ch < L'0' || ch > L'9')
					break;

				value = value*10 + ch-L'0';
				inx++;

				if (value < 0 || value >= 32000)
				{
					MsgBox(L"Error assembling a grammar conflict index.");
					return(FALSE);
				}
			}

			WORD word_inx = (WORD)value;
			if (res_array.IsSymInBuffer(word_inx) == TRUE)
			{
				MsgBoxFmt(MB_OK, L"Duplication of the grammar conflict index C%hd.", word_inx);
				return(FALSE);
			}
			else if (word_inx >= num_conflicts)
			{
				MsgBoxFmt(MB_OK, L"The conflict index C%hd is too big. Selected grammar contains only %d grammar conflict%s.", word_inx, num_conflicts, PluralSuffix(num_conflicts));
				return(FALSE);
			}
			else if (res_array.AppendItem(word_inx) == FALSE)
			{
				MsgBoxFmt(MB_OK, L"Error appending the conflict index C%hd to the list of the grammar conflicts to process..", word_inx);
				return(FALSE);
			}
		}
		else
		{
			// This is a bogus character.
			MsgBox(L"Error assembling the list of the grammar conflict indexes. Bogus character in the entry field.");
			return(FALSE);
		}
	}

	if (res_array.NumItems() <= 0)
	{
		MsgBox(L"Processing data from the edit control resulted in an empty list of the grammar conflict indexes.");
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

// -------------------------------------------------------------------------
//   =================  TGrammarAnalysisConsole  ===================
// -------------------------------------------------------------------------

TGrammarAnalysisConsole::TGrammarAnalysisConsole()
	: TDialogBasedConsole(NULL, IDD_LANGV_ANPROC_CONSOLE, IDC_VPGRPR_MAIN_MESSAGE, IDC_VPGRPR_EXT_MESSAGE, IDCANCEL)
{
}

LRESULT  TGrammarAnalysisConsole::ProcessInitDialog(HWND hDefFocusControl)
{
	if (::SetTimer(m_panel_dialog.GetWnd(), ConsoleTimerIdent, 100, NULL) == 0)
	{
		// This failure is unlikely to happen. Nevertheless.
		m_panel_dialog.MsgBox(L"Error creating the console timer.");
		return(-1);
	}

	m_panel_dialog.SetControlFont(IDC_VPGRPR_MAIN_MESSAGE, VERDANA, 8, TRUE);
	m_panel_dialog.SetControlColor(IDC_VPGRPR_MAIN_MESSAGE, RGB(0, 0, 48));

	// Show the date time of the session startup.
	SetMajorStatus(L"Running ...");
	wchar_t buffer1[120], buffer2[80];
	swprintf(buffer1, 120, L"Startup:   %s.", FormatDateTime(CurrDateTime(), buffer2, 80));
	SetMinorStatus(buffer1);

	// Show the PID of the process.
	m_panel_dialog.SetControlFont(IDC_VPGRPR_PRP_STT_GRPBOX, SMALL_FONTS, 7);
	m_panel_dialog.SetControlTextFmt(IDC_VPGRPR_PROCESS_ID, L"%lu", ::GetCurrentProcessId());
	m_panel_dialog.ShowControl(IDC_VPGRPR_ACTS_SCHEDULED_LAB, FALSE);
	m_panel_dialog.SetControlText(IDC_VPGRPR_ACTS_SCHEDULED, NULL);
	m_panel_dialog.SetControlText(IDC_VPGRPR_ACTS_STATUS_LAB, NULL);
	m_panel_dialog.SetControlText(IDC_VPGRPR_ACTS_STATUS, NULL);

	// Prepare second group box.
	m_panel_dialog.SetControlFont(IDC_VPGRPR_BKR_INX_GRPBOX, SMALL_FONTS, 7);
	m_panel_dialog.SetControlFont(IDC_VPGRPR_BKR_INX_LAB, SMALL_FONTS, 7);
	m_panel_dialog.SetControlFont(IDC_VPGRPR_BKR_INX_BUTTON, SMALL_FONTS, 7);

#ifdef _DEBUG
	// Highlight the fact that this is a DEBUG build.
	m_panel_dialog.SetControlFont(IDC_VPGRPR_MSG_DEBUG, ARIAL, 8, TRUE);
	m_panel_dialog.SetControlColor(IDC_VPGRPR_MSG_DEBUG, RGB(104, 0, 0));
	m_panel_dialog.SetControlText(IDC_VPGRPR_MSG_DEBUG, L"DEBUG");
#else
	// This is a release build.
	m_panel_dialog.SetControlText(IDC_VPGRPR_MSG_DEBUG, NULL);
#endif

	return(TRUE);
}

void TGrammarAnalysisConsole::ProcessWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	short btnId = m_control_button_state.ControlId();
	if (btnId != 0 && btnId == controlId)
	{
		// This is an abort event.
		TDialogBasedConsole::ProcessWmCommand(controlId, notificationCode, hWndControl);
		SetMinorStatus(L"Aborting ... ");
		EnabDisabManageIterationControls(FALSE);
	}
	else if (controlId == IDC_VPGRPR_BKR_INX_BUTTON)
	{
		// This is an attempt to manage the iteration.
		DWORD paramValue;
		if (GetPostedParamValue(0, paramValue, FALSE) == TRUE)
		{
			// The target process is not ready to receive a new event.
			return;
		}

		// Pick up the index value.
		int index;
		if (TLangViewEditConflictAnalysisProps::GetIntValue(&m_panel_dialog, IDC_VPGRPR_BKR_INX_EDIT, index) == FALSE || index < 0 || index >= TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN)
		{
			m_panel_dialog.MsgBoxFmt(MB_OK, L"Error getting a zero based iteration layer index. The value should be in the range 0 ... %d.", TGrammarConflictAnalysisHelper::MAX_DERIV_PATH_LEN);
			return;
		}

		// Post an abort message for the curr iteration layer setup.
		PostParamValue(0, index);
		EnabDisabManageIterationControls(FALSE);
	}
}

void TGrammarAnalysisConsole::ProcessWmTimer(int timer_id)
{
	// Check for the partial abort event state.
	DWORD paramValue;
	if (GetPostedParamValue(0, paramValue, FALSE) == FALSE)
	{
		// The param value is either already retrieved or it was not sent.
		if (GetAbortFlag() == FALSE && m_panel_dialog.IsControlEnabled(IDC_VPGRPR_BKR_INX_LAB) == FALSE)
		{
			// Enable the controls back.
			EnabDisabManageIterationControls(TRUE);
		}
	}
}

void TGrammarAnalysisConsole::ProcessWmDestroy()
{
	::KillTimer(m_panel_dialog.GetWnd(), ConsoleTimerIdent);
}

void TGrammarAnalysisConsole::SetConsoleLabel(int labelId, const wchar_t *msg)
{
	if (labelId == TBasicGrammarAnalysisHelper::cons_label_num_scheduled_actions)
	{
		m_panel_dialog.ShowControl(IDC_VPGRPR_ACTS_SCHEDULED_LAB, TRUE);
		TDialogBasedConsole::SetConsoleLabel(IDC_VPGRPR_ACTS_SCHEDULED, msg);
	}
	else if (labelId == TBasicGrammarAnalysisHelper::cons_label_num_started_actions)
	{
		m_panel_dialog.SetControlText(IDC_VPGRPR_ACTS_STATUS_LAB, L"Actions started:");
		TDialogBasedConsole::SetConsoleLabel(IDC_VPGRPR_ACTS_STATUS, msg);
	}
	else if (labelId == TBasicGrammarAnalysisHelper::cons_label_num_no_paths_actions)
	{
		m_panel_dialog.SetControlText(IDC_VPGRPR_ACTS_STATUS_LAB, L"No paths actions:");
		TDialogBasedConsole::SetConsoleLabel(IDC_VPGRPR_ACTS_STATUS, msg);
	}
	else if (labelId == TBasicGrammarAnalysisHelper::cons_label_num_remaining_actions)
	{
		m_panel_dialog.SetControlText(IDC_VPGRPR_ACTS_STATUS_LAB, L"Actions remaining:");
		TDialogBasedConsole::SetConsoleLabel(IDC_VPGRPR_ACTS_STATUS, msg);
	}
}

void TGrammarAnalysisConsole::EnabDisabManageIterationControls(bool bEnable)
{
	m_panel_dialog.EnableControl(IDC_VPGRPR_BKR_INX_GRPBOX, bEnable);
	m_panel_dialog.EnableControl(IDC_VPGRPR_BKR_INX_LAB, bEnable);
	m_panel_dialog.EnableControl(IDC_VPGRPR_BKR_INX_EDIT, bEnable);
	m_panel_dialog.EnableControl(IDC_VPGRPR_BKR_INX_BUTTON, bEnable);

	if (bEnable == TRUE)
	{
		m_panel_dialog.SetControlText(IDC_VPGRPR_BKR_INX_EDIT, NULL);
		m_panel_dialog.SetFocusControl(IDC_VPGRPR_BKR_INX_EDIT);
	}
}


