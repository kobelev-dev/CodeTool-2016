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
#include  "LangView/GrammarAnalysisApp.H"
#include  "LangView/LangViewResources.H"

// -----------------------------------------------------------------------------
//  ==============  TGrammarConflictsAnalysisStartupDialog  ================
// -----------------------------------------------------------------------------

static wchar_t *g_SelectAnalysisDirFilterSpec = L"All Files (*.*)\0*.*\0";
static wchar_t *g_SelectAnalysisDirOpenTitle = L" Select Analysis Database Directory ";

TGrammarConflictsAnalysisStartupDialog::TGrammarConflictsAnalysisStartupDialog(TGrammarConflictAnalysisDriverApp *owner)
		: TBasicDialog(NULL, IDD_LANGV_ANPROC_STARTUP_DIALOG),
		m_ofn_andir(NULL, g_SelectAnalysisDirFilterSpec, L"GRM", g_SelectAnalysisDirOpenTitle)
{
	assert(owner != NULL);
	m_owner = owner;
	m_database_file_present = FALSE;
}

LRESULT TGrammarConflictsAnalysisStartupDialog::OnInitDialog(HWND hDefFocusControl)
{
	// Prepare the dialog itself.
	CenterOverTheParent();
	SetControlFont(IDC_ANPSTART_SRC_FNAME, VERDANA, 8, TRUE);
	SetControlColor(IDC_ANPSTART_SRC_FNAME, RGB(0, 0, 96));

	// Show the props of the grammar.
	wchar_t date_buffer[80];
	SetControlText(IDC_ANPSTART_SRC_FNAME, m_owner->m_grammar->grm_props.grms_file_name);
	SetControlTextFmt(IDC_ANPSTART_SRC_LENGTH, L"%ld", m_owner->m_grammar->grm_props.grms_file_length);
	SetControlText(IDC_ANPSTART_SRC_DATE, FormatDateTime(m_owner->m_grammar->grm_props.grms_file_date, date_buffer, 80));
	SetControlTextFmt(IDC_ANPSTART_SRC_GRMS_HIER, L"%ld", m_owner->m_grammar->GetNumGrammars());

	int num_cfcts = m_owner->m_grammar->GetNumGrammarConflictsHier();
	if (num_cfcts == 0)
		SetControlText(IDC_ANPSTART_SRC_CFCTS_HIER, L"none");
	else SetControlTextFmt(IDC_ANPSTART_SRC_CFCTS_HIER, L"%ld", num_cfcts);

	// Show the directory name.
	SetControlFont(IDC_ANPSTART_DIR_BTN_BROWSE, TAHOMA, 7);
	SetControlFont(IDC_ANPSTART_DIR_BTN_ENTER_NAME, SMALL_FONTS, 4);
	UpdateDirectoryStatus();

	// Hide the first small button (temp).
	ShowControl(IDC_ANPSTART_DIR_BTN_BROWSE, FALSE);

	// Do not set the focus anywhere.
	::PostMessage(m_hDlg, WM_COMMAND, IDC_ANPSTART_EVT_CHECK_GRM, 0);
	SetFocusControl(IDC_ANPSTART_BTN_CANCEL);
	return(FALSE);
}

void TGrammarConflictsAnalysisStartupDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	if (controlId == IDC_ANPSTART_DIR_BTN_BROWSE)
	{
		// For now just show the dialog.
		bool res = m_ofn_andir.GetOpenFileName(m_hDlg);
		return;
	}
	else if (controlId == IDC_ANPSTART_DIR_BTN_ENTER_NAME)
	{
		// Prompt user with the dialog.
		TGrammarConflictsAnalysisEnterDirName dlg(this);
		dlg.OpenModalDialog(m_hDlg);
		UpdateDirectoryStatus();
		return;
	}
	else if (controlId == IDC_ANPSTART_BTN_STARTOVER)
	{
		if (CheckGrammar() == FALSE)
			return;
		if (CheckCreateDirectory(this) == FALSE)
			return;

		// Ask for confirmation before deleting the old analysis files.
		if (m_database_file_present == TRUE && MsgBox(L"\r\nThe database file is present. Do you want to delete it?\r\n", MB_YESNO) != IDYES)
			return;

		// It is necessary to remove the old analysis results files if any.
		m_owner->SetAnalysisDirectory(m_ofn_andir.GetCurrFileName());
		if (m_owner->DeleteAnalysisFiles() == FALSE)
		{
			MsgBox(L"Error deleting the analysis files. Check the directory name, the read only flags on the files and the directory permissions.");
			return;
		}

		// It is ok to start the analysis.
		controlId = TRUE;
	}
	else if (controlId == IDC_ANPSTART_BTN_CONTINUE)
	{
		if (CheckGrammar() == FALSE)
			return;
		if (CheckCreateDirectory(this) == FALSE)
			return;

		// It is ok to start the analysis.
		controlId = TRUE;
	}
	else if (controlId == IDC_ANPSTART_BTN_CANCEL || controlId == IDCANCEL)
	{
		// User does not want or cannot start the analysis.
		controlId = FALSE;
	}
	else if (controlId == IDC_ANPSTART_EVT_CHECK_GRM)
	{
		// This is a posted message from the WM_INITDIALOG handler.
		CheckGrammar();
		return;
	}
	else
	{
		// Unknown control id.
		return;
	}

	::EndDialog(m_hDlg, controlId);
}

int TGrammarConflictsAnalysisStartupDialog::ShowDialog(HWND hParent, const wchar_t *database_directory)
{
	m_ofn_andir.SetCurrFileName(database_directory);
	if (m_ofn_andir.GetCurrFileName()[0] == 0)
	{
		// Setup the default directory name.
		TFileNameBuffer dir_name_buff;
		if (TPathHelper::GetDirectoryName(dir_name_buff, m_owner->m_grammar->grm_props.grms_file_name) == TRUE)
		{
			// Directory name of the major grammar source file name was successfully extracted.
			dir_name_buff.Append(L"\\");
			dir_name_buff.Append(TBasicGrammarAnalysisHelper::DEF_AN_RES_SUBDIR);
			m_ofn_andir.SetCurrFileName(dir_name_buff);
		}
	}

	return(OpenModalDialog(hParent));
}

void TGrammarConflictsAnalysisStartupDialog::UpdateDirectoryStatus()
{
	wchar_t *dir_name = (wchar_t*)m_ofn_andir.GetCurrFileName();
	SetControlText(IDC_ANPSTART_DIR_NAME, m_ofn_andir.GetCurrFileName());

	EnableControl(IDC_ANPSTART_BTN_CONTINUE, FALSE);
	m_database_file_present = FALSE;

	if (dir_name[0] == 0)
	{
		SetControlText(IDC_ANPSTART_DIR_MESSAGE, L"The directory name is empty.");
	}
	else if (TPathHelper::IsPathADir(dir_name) == FALSE)
	{
		SetControlText(IDC_ANPSTART_DIR_MESSAGE, L"The directory is not existing.");
	}
	else
	{
		// Check if directory contains a database file or not.
		TFileNameBuffer fname_buff;
		fname_buff.Append(dir_name);
		fname_buff.Append(L"\\");
		fname_buff.Append(TGrammarConflictAnalysisDriver::g_AnalysisDatabaseFileName);
		if (fname_buff.GetXpndError() == TRUE)
		{
			MsgBox(L"Error preparing the database file name.");
		}
		else if (TPathHelper::IsPathAFile(fname_buff) == FALSE)
		{
			SetControlText(IDC_ANPSTART_DIR_MESSAGE, L"The directory is existing and there is no database file there.");
		}
		else
		{
			SetControlText(IDC_ANPSTART_DIR_MESSAGE, L"The directory is existing and the database file is present there.");
			EnableControl(IDC_ANPSTART_BTN_CONTINUE, TRUE);
			m_database_file_present = TRUE;
		}
	}

}

bool TGrammarConflictsAnalysisStartupDialog::CheckGrammar()
{
	if (m_owner->m_grammar->parent_grammar != NULL)
	{
		MsgBox(L"The grammar to process is not a root grammar.");
		return(FALSE);
	}
	else if (m_owner->m_grammar->grm_props.processing_result != grpr_full_success)
	{
		MsgBox(L"The grammar to process is not in the full success state.");
		return(FALSE);
	}
	else if (m_owner->m_grammar->NumConflicts() == 0)
	{
		MsgBox(L"The grammar to process has no grammar conflicts.");
		return(FALSE);
	}

	// All conditions are fine.
	return(TRUE);
}

bool TGrammarConflictsAnalysisStartupDialog::CheckCreateDirectory(TBasicDialog *dlg)
{
	wchar_t *dir_name = (wchar_t*)m_ofn_andir.GetCurrFileName();
	if (dir_name[0] == 0)
	{
		dlg->MsgBox(L"The directory name is empty.");
		return(FALSE);
	}
	else if (TPathHelper::IsPathADir(dir_name) == FALSE)
	{
		if (::CreateDirectoryW(dir_name, NULL) == FALSE)
		{
			DWORD err = ::GetLastError();
			dlg->MsgBoxFmt(MB_OK, L"Error creating the analysis database directory. Win32 error: %d.", err);
			return(FALSE);
		}
	}

	// Success.
	return(TRUE);
}

// -----------------------------------------------------------------------------
//  ==============  TGrammarConflictsAnalysisEnterDirName  ================
// -----------------------------------------------------------------------------

TGrammarConflictsAnalysisEnterDirName::TGrammarConflictsAnalysisEnterDirName(TGrammarConflictsAnalysisStartupDialog *owner)
	: TBasicDialog(NULL, IDD_LANGV_ANPROC_ENTER_DIR_NAME)
{
	assert(owner != NULL);
	m_owner = owner;

	SetDialogParams(owner);
}

LRESULT TGrammarConflictsAnalysisEnterDirName::OnInitDialog(HWND hDefFocusControl)
{
	CenterOverTheParent();
	SetControlText(IDC_ANPDN_ENTRY_FIELD, m_owner->m_ofn_andir.GetCurrFileName());
	return(TRUE);
}

void TGrammarConflictsAnalysisEnterDirName::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDOK:
				{
					// Pick up the data from the control.
					wchar_t buff1[TFileOpenHelper::FILE_OPEN_MAX_PATH];
					GetControlText(IDC_ANPDN_ENTRY_FIELD, buff1, TFileOpenHelper::FILE_OPEN_MAX_PATH);

					// Save and replace data in the parent dialog.
					wchar_t buff2[TFileOpenHelper::FILE_OPEN_MAX_PATH];
					wcscpy(buff2, m_owner->m_ofn_andir.GetCurrFileName());
					m_owner->m_ofn_andir.SetCurrFileName(buff1);
					if (m_owner->CheckCreateDirectory(this) == FALSE)
					{
						// Restore the data back.
						m_owner->m_ofn_andir.SetCurrFileName(buff2);
						break;
					}

					// The directory was either already existing or it was possible to create it.
					::EndDialog(m_hDlg, FALSE);
				}
				break;

		case IDCANCEL:
				{
					// User decided to cancel.
					::EndDialog(m_hDlg, FALSE);
				}
				break;
	}
}

// --------------------------------------------------------------------------
//   ==============  TGrammarConflictAnalysisDriverApp  ================
// --------------------------------------------------------------------------

bool TGrammarConflictAnalysisDriverApp::PrepareAnalysisTaskAndProps(TGrammarConflictAnalysisTask &prm_task, TGrammarConflictAnalysisProps &prm_props, bool prepare_props)
{
	TLangViewProcessGrammarProps props;
	props.SetDefaultValues();

	if (prepare_props == FALSE)
		props.m_anprops = prm_props;

	TLangViewEditConflictAnalysisProps inst(*m_grammar, props, TRUE, prepare_props);
	if (inst.EditAnalysisProps(m_hParent) == FALSE)
		return(FALSE);

	// Give out the prepared/modified data.
	prm_task.TakeDataFrom(props.m_antask);
	if (prepare_props == TRUE)
		prm_props = props.m_anprops;

	SaveAnalysisTask(prm_task);
	return(TRUE);
}

void TGrammarConflictAnalysisDriverApp::DoConflictsProcessingApp(const wchar_t *proposed_database_directory)
{
	TGrammarConflictsAnalysisStartupDialog startup_dialog(this);
	int res = startup_dialog.ShowDialog(m_hParent, proposed_database_directory);
	if (res < 0)
	{
		// Most likely this is a problem in the resources.
		::MessageBoxW(m_hParent, L"Unable to open the startup dialog.", L" Grammar Conflicts Analysis ", MB_OK);
	}
	else if (res > 0)
	{
		wchar_t buff_cons_title[2*MAX_PATH+80];
		swprintf(buff_cons_title, 2*MAX_PATH+80, L" GrammarAnalysisTool: %s (Pid=%lu) ", startup_dialog.GetDirectoryName(), ::GetCurrentProcessId());
		::SetConsoleTitleW(buff_cons_title);

		// User has asked to do the processing. It is ok to cast the console because it was passed
		// as a dialog based console into the ctor of this class.
		TDialogBasedConsole &cons = (TDialogBasedConsole&)m_console;

		// Prepare the console.
		cons.SetParentWindow(m_hParent);
		cons.SetShowRunTimeErrorsAsPopups(FALSE);

		// Carry out the processing.
		SetAnalysisDirectory(startup_dialog.GetDirectoryName());
		DoConflictsProcessing();
	}
}

void TGrammarConflictAnalysisDriverApp::SaveAnalysisTask(TGrammarConflictAnalysisTask &prm_task)
{
	wchar_t buff_short_fname[80];
	swprintf(buff_short_fname, 80, L"\\Analysis_Task_%lu.TXT", ::GetCurrentProcessId());

	TFileNameBuffer buff_full_fname;
	buff_full_fname.Append(m_database_directory);
	buff_full_fname.Append(buff_short_fname);
	HardAssert(buff_full_fname.GetXpndError() == FALSE, L"AnalysisTask path is too long.");

	TDestinationFile rprt(buff_full_fname);
	rprt.PrepareDiskFile();
	rprt.WriteLine();

	// Generate the body of the file.
	if (prm_task.m_only_igrammar < 0)
	{
		rprt.WriteFmtLine(L"ProcessGrammars: \tALL");
	}
	else
	{
		rprt.WriteFmtLine(L"ProcessGrammar: \t%d.", prm_task.m_only_igrammar);
	}

	if (prm_task.m_only_iconflict <= -3)
	{
		rprt.WriteFmtLine(L"ProcessXpctConflict: \tX%d.", -(3+prm_task.m_only_iconflict));
		rprt.WriteFmtLine(L"ProcessActions: \tALL");
	}
	else if (prm_task.m_only_iconflict == -2)
	{
		rprt.Write(L"ProcessConflicts: \t");
		for (int icon=0; icon<prm_task.m_cfct_indexes_list.NumItems(); ++icon)
			rprt.WriteFmt(L"%hd%s", prm_task.m_cfct_indexes_list[icon], (icon != prm_task.m_cfct_indexes_list.NumItems()-1) ? L", " : L".");

		rprt.WriteLine();
		rprt.WriteLine(L"ProcessActions: \tALL");
	}
	else if (prm_task.m_only_iconflict == -1)
	{
		rprt.WriteLine(L"ProcessConflicts: \tALL");
		rprt.WriteLine(L"ProcessActions: \tALL");
	}
	else
	{
		rprt.WriteFmtLine(L"ProcessConflict: \tC%d.", prm_task.m_only_iconflict);

		if (prm_task.m_only_iaction < 0)
		{
			rprt.WriteFmtLine(L"ProcessActions: \tALL");
		}
		else
		{
			rprt.WriteFmtLine(L"ProcessAction: \t%d.", prm_task.m_only_iaction);
		}
	}

	rprt.WriteLine();
	bool close_res = rprt.Close();
	HardAssert(close_res == TRUE, L"Error saving AnalysisTask.");
}


