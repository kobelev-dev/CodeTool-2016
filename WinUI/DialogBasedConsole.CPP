//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "Common/DebugLogger.H"
#include  "WinUi/DialogBasedConsole.H"

//-----------------------------------------------------------------------
//  ==================  TStatusPanelDialog  =====================
//-----------------------------------------------------------------------

#define	STATUS_PANEL_MAJOR_MSG_CTRL		170		// Control ID in the generated template.
#define	STATUS_PANEL_MINOR_MSG_CTRL		171		// Control ID in the generated template.

#define	STATUS_PANEL_SPACE_ABOVE		8		// Space above the first message.
#define	STATUS_PANEL_BETWEEN1			6		// Space between the first and the second messages.
#define	STATUS_PANEL_SPACE_BELOW1		12		// Space below the messages when there is no control button.

#define	STATUS_PANEL_BETWEEN2			10		// Space between the second message and the control button.
#define	STATUS_PANEL_SPACE_BELOW2		9		// Space below the button.

#define	STATUS_PANEL_MSGY				8		// Height of the messages controls.

#define	STATUS_PANEL_BTNX				40		// Width of the control button.
#define	STATUS_PANEL_BTNY				12		// Height of the control button.

#define	STATUS_PANEL_MIN_HEIGHT(want_btn)		(STATUS_PANEL_SPACE_ABOVE+STATUS_PANEL_MSGY+STATUS_PANEL_BETWEEN1+STATUS_PANEL_MSGY+ \
													((want_btn == FALSE) ?  STATUS_PANEL_SPACE_BELOW1 : (STATUS_PANEL_BETWEEN2+STATUS_PANEL_BTNY+STATUS_PANEL_SPACE_BELOW2)))
#define	STATUS_PANEL_FIXED_HEIGHT(want_btn)		(2*STATUS_PANEL_MSGY+((want_btn == TRUE) ? STATUS_PANEL_BTNY : 0))

LRESULT TStatusPanelDialog::OnInitDialog(HWND hDefFocusControl)
{
	if (m_inMemoryDlgTemplate != NULL)
	{
		// Set the title and font for the minor status message.
		SetTitleText(L" Status Panel ");
		SetControlFont(STATUS_PANEL_MINOR_MSG_CTRL, SMALL_FONTS, 7);

		if (m_owner != NULL)
			m_owner->m_title_state.RestoreTitle(this);
	}

	if (m_owner != NULL)
	{
		// Setup the status messages if their ids are known.
		m_owner->m_major_status.RestoreText(this);
		m_owner->m_minor_status.RestoreText(this);

		// Setup the control button if its id is known.
		short btnId = m_owner->m_control_button_state.ControlId();
		if (btnId != 0)
		{
			m_owner->m_control_button_state.RestoreText(this);
			EnableControl(btnId, m_owner->m_control_button_enabled);
		}
	}

	// Position the dialog.
	if (m_owner == NULL || m_owner->m_restore_window_pos == FALSE)
	{
		// The console is opened for the first time.
		CenterOverTheParent();
	}
	else
	{
		// The preferred position is already existing.
		::SetWindowPos(m_hDlg, NULL, m_owner->m_saved_window_pos.x, m_owner->m_saved_window_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	// Give chance to the derived class to do its work.
	LRESULT retVal = TRUE;
	if (m_owner != NULL)
	{
		retVal = m_owner->ProcessInitDialog(hDefFocusControl);
	}

	// Do the final action.
	::ShowWindow(m_hDlg, SW_SHOW);
	return(retVal);
}

void TStatusPanelDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	if (m_owner != NULL)
	{
		// Pass event handling to the console object.
		m_owner->ProcessWmCommand(controlId, notificationCode, hWndControl);
	}
}

void TStatusPanelDialog::OnWmTimer(int timerId)
{
	if (m_owner != NULL)
	{
		// Pass event handling to the console object.
		m_owner->ProcessWmTimer(timerId);
	}
}

void TStatusPanelDialog::OnWmMoving(RECT *pRect)
{
	if (m_owner != NULL)
	{
		// Save the position of the window.
		m_owner->m_saved_window_pos.x = pRect->left;
		m_owner->m_saved_window_pos.y = pRect->top;
		m_owner->m_restore_window_pos = TRUE;
	}
}

bool TStatusPanelDialog::OnWmDestroy()
{
	if (m_owner != NULL)
	{
		// Pass event handling to the console object.
		m_owner->ProcessWmDestroy();
	}

	return(TBasicDialog::OnWmDestroy());
}

void TStatusPanelDialog::PrepareTemplate(bool want_control_button, int dialog_width, int dialog_height)
{
	WORD *buff_ptr = (WORD*)m_dialog_template;

	if (dialog_width < STATUS_PANEL_BTNX+10)
		dialog_width = STATUS_PANEL_BTNX+10;
	if (dialog_height < STATUS_PANEL_MIN_HEIGHT(want_control_button))
		dialog_height = STATUS_PANEL_MIN_HEIGHT(want_control_button);

	// Requested height will be evenly distributed among the spaces between controls.
	int reference_height = STATUS_PANEL_MIN_HEIGHT(want_control_button)-STATUS_PANEL_FIXED_HEIGHT(want_control_button);
	int height_to_disrib = dialog_height-STATUS_PANEL_FIXED_HEIGHT(want_control_button);
	int pos_y = 0;

	// Fill in the description of the dialog.
	PrepareBasicDialogTemplate(buff_ptr, BASIC_DIALOG_STYLE | DS_MODALFRAME, (want_control_button == TRUE) ? 3 : 2, dialog_width, dialog_height, TRUE);

	// Fill in descriptions of the static texts.
	pos_y += ::MulDiv(STATUS_PANEL_SPACE_ABOVE, height_to_disrib, reference_height);
	AddStaticControlDescriptor(buff_ptr, STATUS_PANEL_MAJOR_MSG_CTRL, 0, pos_y, dialog_width, STATUS_PANEL_MSGY);
	pos_y += STATUS_PANEL_MSGY+::MulDiv(STATUS_PANEL_BETWEEN1, height_to_disrib, reference_height);
	AddStaticControlDescriptor(buff_ptr, STATUS_PANEL_MINOR_MSG_CTRL, 0, pos_y, dialog_width, STATUS_PANEL_MSGY);

	// Fill in the description of the button.
	if (want_control_button == TRUE)
	{
		short pos_x = (dialog_width-STATUS_PANEL_BTNX)/2;
		pos_y += STATUS_PANEL_MSGY+::MulDiv(STATUS_PANEL_BETWEEN2, height_to_disrib, reference_height);
		AddCancelButtonDescriptor(buff_ptr, pos_x, pos_y, STATUS_PANEL_BTNX, STATUS_PANEL_BTNY);
	}

	// Assign the template pointer. This will also indicate to the InitDialog handler the way how the dialog was created.
	m_inMemoryDlgTemplate = (DLGTEMPLATE*)m_dialog_template;
}

//-------------------------------------------------------------------------
//  ==================  TDialogBasedConsole  =====================
//-------------------------------------------------------------------------

TDialogBasedConsole::TDialogBasedConsole(HINSTANCE hInst, const wchar_t *console_title, bool want_control_button, int panel_width, int panel_height)
	: m_panel_dialog(hInst, 0), m_title_state(0), m_major_status(STATUS_PANEL_MAJOR_MSG_CTRL), m_minor_status(STATUS_PANEL_MINOR_MSG_CTRL), m_control_button_state(IDCANCEL)
{
	// Do not allow the apps to skip setting up the stnd params of the basic dialog.
	assert(TBasicDialog::CheckStdParamsSet() == TRUE);

	m_show_run_time_errors_as_popups = TRUE;

	m_want_control_button = want_control_button;
	m_panel_width_in_dbu = panel_width;
	m_panel_height_in_dbu = panel_height;

	m_panel_dialog.SetOwner(this);
	m_panel_dialog.PrepareTemplate(want_control_button, panel_width, panel_height);
	m_parent_window = NULL;
	m_restore_window_pos = FALSE;

	if (console_title != NULL)
		m_title_state.SaveControlText(console_title);

	m_control_button_enabled = TRUE;
}

TDialogBasedConsole::TDialogBasedConsole(HINSTANCE hInst, short dialogTemplateId, short ctrlMajorStatus, short ctrlMinorStatus, short ctrlControlButton)
	: m_panel_dialog(hInst, dialogTemplateId), m_title_state(0), m_major_status(ctrlMajorStatus), m_minor_status(ctrlMinorStatus), m_control_button_state(ctrlControlButton)
{
	// Do not allow the apps to skip setting up the stnd params of the basic dialog.
	assert(TBasicDialog::CheckStdParamsSet() == TRUE);

	m_show_run_time_errors_as_popups = TRUE;

	m_want_control_button = FALSE;
	m_panel_width_in_dbu = m_panel_height_in_dbu = 0;

	m_panel_dialog.SetOwner(this);
	m_parent_window = NULL;
	m_restore_window_pos = FALSE;

	m_control_button_enabled = TRUE;
}

void TDialogBasedConsole::ShowControlButton(bool value)
{
	if (m_panel_dialog.IsMemoryTemplate() == FALSE || value == m_want_control_button)
		return;

	// Dialog template should be rebuilt.
	m_want_control_button = value;
	m_panel_dialog.PrepareTemplate(m_want_control_button, m_panel_width_in_dbu, m_panel_height_in_dbu);
}

void TDialogBasedConsole::ProcessWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	short btnId = m_control_button_state.ControlId();
	if (btnId != 0 && btnId == controlId)
	{
		if (m_panel_dialog.GetModalFlag() == FALSE)
		{
			// Raise the abort status and disable the control button.
			SetAbortFlag(TRUE);
			SetControlButtonState(NULL, FALSE);
		}
		else
		{
			// Close the modal dialog.
			::EndDialog(m_panel_dialog.GetWnd(), FALSE);
		}
	}
}

void TDialogBasedConsole::HandleTrace(const wchar_t *message, WORD highlight_mode)
{
	// Allow Windows to refresh the UI.
	TBasicDialog::ProcessPendingMessages();

	// Forward handling to the base class.
	TGenericConsole::HandleTrace(message, highlight_mode);
}

int TDialogBasedConsole::ShowRunTimeError(const wchar_t *message, int buttons, bool inc_errors_count)
{
	// Allow Windows to refresh the UI.
	TBasicDialog::ProcessPendingMessages();

	if (m_show_run_time_errors_as_popups == TRUE)
	{
		if (inc_errors_count == TRUE)
			BumpErrorsCount();

		// Proccess this action locally by displaying the message box.
		return(m_panel_dialog.MsgBox(message, buttons, 0, 60));
	}

	// Forward handling to the base class.
	return(TGenericConsole::ShowRunTimeError(message, buttons, inc_errors_count));
}

void TDialogBasedConsole::OpenConsole()
{
	if (m_panel_dialog.GetWnd() != NULL)
		return;

	// The dialog template is already prepared in the ctor or its id was passed by the upper layer.
	m_panel_dialog.OpenNonModalDialog(m_parent_window);
	TBasicDialog::ProcessPendingMessages();
}

void TDialogBasedConsole::CloseConsole()
{
	if (m_panel_dialog.GetWnd() != NULL)
	{
		if (m_panel_dialog.GetModalFlag() == FALSE)
		{
			// Destroy the non modal dialog.
			::DestroyWindow(m_panel_dialog.GetWnd());
			TBasicDialog::ProcessPendingMessages();
		}
		else
		{
			// Close the modal dialog.
			::EndDialog(m_panel_dialog.GetWnd(), FALSE);
		}
	}
}

int TDialogBasedConsole::OpenModalConsole()
{
	if (m_panel_dialog.GetWnd() != NULL)
	{
		// The console is already opened.
		assert(FALSE);
		return(FALSE);
	}

	// Open console as the modal dialog.
	return(m_panel_dialog.OpenModalDialog(m_parent_window));
}

void TDialogBasedConsole::SetMajorStatus(const wchar_t *msg)
{
	// Save the passed status string.
	m_major_status.SaveControlText(msg);

	// Update the dialog if it is opened.
	m_major_status.RestoreText(&m_panel_dialog);
	TBasicDialog::ProcessPendingMessages();
}

void TDialogBasedConsole::SetMinorStatus(const wchar_t *msg)
{
	// Save the passed status string.
	m_minor_status.SaveControlText(msg);

	// Update the dialog if it is opened.
	m_minor_status.RestoreText(&m_panel_dialog);
	TBasicDialog::ProcessPendingMessages();
}

void TDialogBasedConsole::SetConsoleLabel(int labelId, const wchar_t *msg)
{
	if (m_panel_dialog.GetWnd() != NULL)
	{
		m_panel_dialog.SetControlText(labelId, msg);
		m_panel_dialog.UpdateDialogWindow();
		TBasicDialog::ProcessPendingMessages();
	}
}

void TDialogBasedConsole::SetControlButtonState(const wchar_t *label, bool enabled_state)
{
	// Save the passed props of the button.
	if (label != NULL)
	{
		m_control_button_state.SaveControlText(label);
	}

	m_control_button_enabled = enabled_state;

	// Update the dialog if it is opened.
	short btnId = m_control_button_state.ControlId();
	if (m_panel_dialog.GetWnd() != NULL && btnId != 0)
	{
		m_panel_dialog.EnableControl(btnId, enabled_state);
		m_control_button_state.RestoreText(&m_panel_dialog);
		m_panel_dialog.UpdateDialogWindow();
		TBasicDialog::ProcessPendingMessages();
	}
}


