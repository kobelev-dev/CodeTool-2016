//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <windows.h>
#include  <stdio.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "TextScan/MidScan.H"
#include  "LangView/LangViewResources.H"
#include  "LangView/MidScanConsole.H"

// ---------------------------------------------------------------------
//   ================  TMidLevScannerConsole  ===================
// ---------------------------------------------------------------------

TMidLevScannerConsole::TMidLevScannerConsole(HINSTANCE hInst, bool batch_mode)
		: TDialogBasedConsole(hInst, GetTemplateId(batch_mode),
							IDC_SCCONS_MAIN_MESSAGE, 0, IDC_SCCONS_CONTROL_BUTTON),
			m_major_file_name_stt(IDC_SCCONS_MAJOR_FILE_NAME, TRUE), m_mid_lev_lexemas_stt(IDC_SCCONS_MID_LEV_LEXEMAS, TRUE),
			m_mid_lev_events_stt(IDC_SCCONS_MID_LEV_EVENTS, TRUE), m_num_errors_stt(IDC_SCCONS_NUM_ERRORS, TRUE),
			m_num_warns_stt(IDC_SCCONS_NUM_WARNS, TRUE), m_elapsed_time_stt(IDC_SCCONS_ELAPSED_TIME, TRUE),
			m_run_time_errs_stt(IDC_SCCONS_RUN_TIME_ERRORS, TRUE), m_batch_files_processed_stt(IDC_SCCONS_BATCH_NUM_FILES, TRUE),
			m_batch_dirs_processed_stt(IDC_SCCONS_BATCH_NUM_DIRS, TRUE), m_batch_num_errors_stt(IDC_SCCONS_BATCH_NUM_ERRORS, TRUE),
			m_batch_num_warns_stt(IDC_SCCONS_BATCH_NUM_WARNS, TRUE), m_batch_elapsed_time_stt(IDC_SCCONS_BATCH_ELAPS_TIME, TRUE)
{
	// Ensure that labels indexes are consistent with resources definitions.
	assert(IDC_SCCONS_MAJOR_FILE_NAME == IDC_SCCONS_LABELS_BASE+sclb_major_file_name);
	assert(IDC_SCCONS_MID_LEV_LEXEMAS == IDC_SCCONS_LABELS_BASE+sclb_mid_lev_lexemas);
	assert(IDC_SCCONS_MID_LEV_EVENTS == IDC_SCCONS_LABELS_BASE+sclb_mid_lev_events);
	assert(IDC_SCCONS_NUM_ERRORS == IDC_SCCONS_LABELS_BASE+sclb_num_errors);
	assert(IDC_SCCONS_NUM_WARNS == IDC_SCCONS_LABELS_BASE+sclb_num_warnings);
	assert(IDC_SCCONS_ELAPSED_TIME == IDC_SCCONS_LABELS_BASE+sclb_elapsed_time);
	assert(IDC_SCCONS_RUN_TIME_ERRORS == IDC_SCCONS_LABELS_BASE+sclb_run_time_errors);
	assert(IDC_SCCONS_BATCH_NUM_FILES == IDC_SCCONS_LABELS_BASE+sclb_batch_files_processed);
	assert(IDC_SCCONS_BATCH_NUM_DIRS == IDC_SCCONS_LABELS_BASE+sclb_batch_dirs_processed);
	assert(IDC_SCCONS_BATCH_NUM_ERRORS == IDC_SCCONS_LABELS_BASE+sclb_batch_num_errors);
	assert(IDC_SCCONS_BATCH_NUM_WARNS == IDC_SCCONS_LABELS_BASE+sclb_batch_num_warns);
	assert(IDC_SCCONS_BATCH_ELAPS_TIME == IDC_SCCONS_LABELS_BASE+sclb_batch_elapsed_time);

	m_curr_batch_mode = batch_mode;
	m_run_time_errs_stt.SaveControlText(L"none");
}

void TMidLevScannerConsole::SetBatchMode(bool value)
{
	if (m_curr_batch_mode != value)
	{
		// Replace the dialog template. This will take effect the next time when the console will be opened.
		m_curr_batch_mode = value;
		m_panel_dialog.SetDialogTemplate(NULL, GetTemplateId(m_curr_batch_mode == FALSE));
	}
}

LRESULT TMidLevScannerConsole::ProcessInitDialog(HWND hDefFocusControl)
{
	short mainMsg = IDC_SCCONS_MAIN_MESSAGE;
	m_panel_dialog.SetControlFont(mainMsg, VERDANA, 10, TRUE);
	m_panel_dialog.SetControlColor(mainMsg, RGB(0, 0, 96));

	// Restore the prev state of the console where possible.
	m_major_file_name_stt.RestoreText(&m_panel_dialog);
	m_mid_lev_lexemas_stt.RestoreText(&m_panel_dialog);
	m_mid_lev_events_stt.RestoreText(&m_panel_dialog);
	m_num_errors_stt.RestoreText(&m_panel_dialog);
	m_num_warns_stt.RestoreText(&m_panel_dialog);
	m_elapsed_time_stt.RestoreText(&m_panel_dialog);
	m_run_time_errs_stt.RestoreText(&m_panel_dialog);
	m_batch_files_processed_stt.RestoreText(&m_panel_dialog);
	m_batch_dirs_processed_stt.RestoreText(&m_panel_dialog);
	m_batch_num_errors_stt.RestoreText(&m_panel_dialog);
	m_batch_num_warns_stt.RestoreText(&m_panel_dialog);
	m_batch_elapsed_time_stt.RestoreText(&m_panel_dialog);
	return(FALSE);
}

void TMidLevScannerConsole::ProcessWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	if (controlId == IDCANCEL)
		controlId = IDC_SCCONS_CONTROL_BUTTON;

	TDialogBasedConsole::ProcessWmCommand(controlId, notificationCode, hWndControl);
}

void TMidLevScannerConsole::SetConsoleLabel(int labelId, const wchar_t *msg)
{
	switch (labelId)
	{
		case sclb_major_file_name:
				m_major_file_name_stt.SaveControlText(msg);
				m_major_file_name_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_mid_lev_lexemas:
				m_mid_lev_lexemas_stt.SaveControlText(msg);
				m_mid_lev_lexemas_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_mid_lev_events:
				m_mid_lev_events_stt.SaveControlText(msg);
				m_mid_lev_events_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_num_errors:
				m_num_errors_stt.SaveControlText(msg);
				m_num_errors_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_num_warnings:
				m_num_warns_stt.SaveControlText(msg);
				m_num_warns_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_elapsed_time:
				m_elapsed_time_stt.SaveControlText(msg);
				m_elapsed_time_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_run_time_errors:
				m_run_time_errs_stt.SaveControlText(msg);
				m_run_time_errs_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_batch_files_processed:
				m_batch_files_processed_stt.SaveControlText(msg);
				m_batch_files_processed_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_batch_dirs_processed:
				m_batch_dirs_processed_stt.SaveControlText(msg);
				m_batch_dirs_processed_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_batch_num_errors:
				m_batch_num_errors_stt.SaveControlText(msg);
				m_batch_num_errors_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_batch_num_warns:
				m_batch_num_warns_stt.SaveControlText(msg);
				m_batch_num_warns_stt.RestoreText(&m_panel_dialog);
				break;

		case sclb_batch_elapsed_time:
				m_batch_elapsed_time_stt.SaveControlText(msg);
				m_batch_elapsed_time_stt.RestoreText(&m_panel_dialog);
				break;
	}
}

void TMidLevScannerConsole::UpdateErrorsCountLabel(int old_num_errors)
{
	if (m_num_errors != old_num_errors)
	{
		// Update visible count of the run time errors.
		wchar_t buff[80];
		swprintf(buff, 80, L"%d", m_num_errors);
		SetConsoleLabel(sclb_run_time_errors, buff);
	}
}

short TMidLevScannerConsole::GetTemplateId(bool batch_mode)
{
	return((batch_mode == FALSE) ? IDD_LANGV_CONS_SINGLE_FILE_SCAN : IDD_LANGV_CONS_BATCH_PROC_SCAN);
}

// --------------------------------------------------------------------------------
//   ==================  TLangViewBasicAsyncConsole  =====================
// --------------------------------------------------------------------------------

TLangViewBasicAsyncConsole::TLangViewBasicAsyncConsole(const wchar_t *console_title)
		: TDialogBasedConsole(NULL, console_title, TRUE)
{
	m_hThread = NULL;
	m_dwThreadId = 0;
	m_dwThreadExitCode = sres_not_started;
}

LRESULT TLangViewBasicAsyncConsole::ProcessInitDialog(HWND hDefFocusCtrl)
{
	// Create timer for tracking the state of the worker thread.
	if (::SetTimer(m_panel_dialog.GetWnd(), ConsoleTimerIdent, 100, NULL) == 0)
	{
		// This failure is unlikely to happen. Nevertheless.
		m_dwThreadExitCode = sres_console_error;
		return(-1);
	}

	// Show fixed main message.
	SetMajorStatus(L"Scanning ...");

	// Show session startup date time.
	wchar_t buffer1[120], buffer2[80];
	swprintf(buffer1, 120, L"Startup:   %s.", FormatDateTime(CurrDateTime(), buffer2, 80));
	SetMinorStatus(buffer1);

	// Launch the worker thread.
	m_dwThreadId = 0;
	m_dwThreadExitCode = STILL_ACTIVE;
	m_hThread = ::CreateThread(NULL, 0, WorkerThreadProc, this, 0, &m_dwThreadId);
	if (m_hThread == NULL)
	{
		::KillTimer(m_panel_dialog.GetWnd(), ConsoleTimerIdent);
		m_dwThreadExitCode = sres_thread_error;
		return(-1);
	}

	// Widows should not set the focus anywhere.
	return(FALSE);
}

void TLangViewBasicAsyncConsole::ProcessWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	short btnId = m_control_button_state.ControlId();
	if (btnId != 0 && btnId == controlId)
	{
		// User wants to abort the process. Update the local console.
		SetAbortFlag(TRUE);
		SetControlButtonState(NULL, FALSE);

		// Send aborting event to the worker thread and do not close the dialog.
		m_worker_thread_console.SetAbortFlag(TRUE);
	}
}

void TLangViewBasicAsyncConsole::ProcessWmTimer(int timer_id)
{
	// Check for the worker thread.
	if (m_hThread == NULL)
		return;

	// Check, maybe the worker thread has posted a new major status.
	if (m_worker_thread_console.m_major_msg_buff[0] != 0)
	{
		::EnterCriticalSection(&m_worker_thread_console.m_crits);
		SetMajorStatus(m_worker_thread_console.m_major_msg_buff);
		m_worker_thread_console.m_major_msg_buff[0] = 0;
		::LeaveCriticalSection(&m_worker_thread_console.m_crits);
	}

	// Check, maybe the worker thread has posted a new minor status.
	if (m_worker_thread_console.m_minor_msg_buff[0] != 0)
	{
		::EnterCriticalSection(&m_worker_thread_console.m_crits);
		SetMinorStatus(m_worker_thread_console.m_minor_msg_buff);
		m_worker_thread_console.m_minor_msg_buff[0] = 0;
		::LeaveCriticalSection(&m_worker_thread_console.m_crits);
	}

	// Check the state of the worker thread.
	if (::GetExitCodeThread(m_hThread, &m_dwThreadExitCode) == FALSE)
	{
		// Win32 returned error. This should be a rare situation.
		assert(FALSE);
		return;
	}

	if (m_dwThreadExitCode != STILL_ACTIVE)
	{
		// The thread has terminated its work. Thread exit code contains the application layer result.
		::CloseHandle(m_hThread);
		m_hThread = NULL;
		::KillTimer(m_panel_dialog.GetWnd(), ConsoleTimerIdent);

		::Sleep(200);
		TDialogBasedConsole::CloseConsole();
	}
}

TLangViewBasicAsyncConsole::SessionResults TLangViewBasicAsyncConsole::DoSession(TBasicDialog *proto_and_owner)
{
	assert(proto_and_owner != NULL);
	assert(m_hThread == NULL);

	SetDialogParams(proto_and_owner);
	SetParentWindow(proto_and_owner->GetWnd());
	m_dwThreadExitCode = sres_not_started;

	SetAbortFlag(FALSE);
	m_worker_thread_console.SetAbortFlag(FALSE);

	int res = TDialogBasedConsole::OpenModalConsole();
	if (res == -1 && m_dwThreadExitCode == sres_not_started)
	{
		// An app session has not started.
		m_dwThreadExitCode = sres_console_error;
	}

	return((SessionResults)m_dwThreadExitCode);
}

DWORD WINAPI TLangViewBasicAsyncConsole::WorkerThreadProc(void *context)
{
	// Call the app level worker procedure.
	TLangViewBasicAsyncConsole *inst = (TLangViewBasicAsyncConsole*)context;
	assert(inst != NULL);

	inst->SessionWorkerProcedure();
	return(sres_app_result);
}


