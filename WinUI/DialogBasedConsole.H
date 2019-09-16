//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	WinUi_DialogBasedConsole_H
#define	WinUi_DialogBasedConsole_H

#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif
#ifndef   WinUi_BasicDialog_H
#include  "WinUi/BasicDialog.H"
#endif

class TDialogBasedConsole;

//
//  This class stores contents of the static text control when the dialog is not opened.
//
template <int len_buffer>
class TSavedConsoleControlState
{
public:

	TSavedConsoleControlState(short ctrl_id, bool val_present = FALSE) { m_ctrl_id = ctrl_id; m_value_present = val_present; m_ctrl_text[0] = 0; }

	short	ControlId() { return(m_ctrl_id); }

	void		SaveControlText(const wchar_t *msg) { m_value_present = TRUE; if (msg != NULL) { wcsncpy(m_ctrl_text, msg, len_buffer+1); m_ctrl_text[len_buffer] = 0; } else { m_ctrl_text[0] = 0; } }
	void		RestoreText(TBasicDialog *inst) { if (m_ctrl_id != 0 && m_value_present == TRUE && inst->GetWnd() != NULL) { inst->SetControlText(m_ctrl_id, m_ctrl_text); inst->UpdateDialogWindow(); } }
	void		RestoreTitle(TBasicDialog *inst) { if (m_value_present == TRUE) inst->SetTitleText(m_ctrl_text); }

protected:

	short		m_ctrl_id;
	bool			m_value_present;
	wchar_t		m_ctrl_text[len_buffer+1];
};

typedef  TSavedConsoleControlState<40>  TSavedConsoleControlState40;
typedef  TSavedConsoleControlState<80>  TSavedConsoleControlState80;

class TStatusPanelDialog : public TBasicDialog
{
public:

	TStatusPanelDialog(HINSTANCE hInst, short dialogTemplateIdent) : TBasicDialog(hInst,  dialogTemplateIdent) { m_owner = NULL; }

	virtual  LRESULT	OnInitDialog(HWND hDefFocusControl);
	virtual  void		OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	virtual  void		OnWmTimer(int timerId);
	virtual  void		OnWmMoving(RECT *pRect);
	virtual  bool		OnWmDestroy();

protected:

	void		SetOwner(TDialogBasedConsole *owner) { m_owner = owner; }
	bool		IsMemoryTemplate() { return(m_inMemoryDlgTemplate != NULL); }
	void		PrepareTemplate(bool want_control_button, int dialog_width, int dialog_height);

protected:

	enum { TEMPLATE_BUFF_LEN = (sizeof(DLGTEMPLATE)+40+3*(sizeof(DLGITEMTEMPLATE)+40)) };


	TDialogBasedConsole	*m_owner;

	DWORD				m_dialog_template[TEMPLATE_BUFF_LEN/sizeof(DWORD)];

	friend class TDialogBasedConsole;
};

class TDialogBasedConsole : public TGenericConsoleEx
{
public:

	TDialogBasedConsole(HINSTANCE hInst = NULL, const wchar_t *console_title = NULL, bool want_control_button = FALSE, int panel_width_in_dbu = 160, int panel_height_in_dbu = 41);
	TDialogBasedConsole(HINSTANCE hInst, short dialogTemplateId, short ctrlMajorStatus = 0, short ctrlMinorStatus = 0, short ctrlControlButton = IDCANCEL);

	void		SetDialogParams(TBasicDialog *proto) { m_panel_dialog.SetDialogParams(proto); }
	void		SetParentWindow(HWND hDlg) { m_parent_window = hDlg; }

	void		ShowControlButton(bool value);
				// Result of calling this method will take effect only when the console will be opened next time.

	void		SetShowRunTimeErrorsAsPopups(bool new_mode) { m_show_run_time_errors_as_popups = new_mode; }
				// By default the run time errors are displayed as popups. Switching this mode to OFF passes
				// these messages to the generic console.

	virtual LRESULT	ProcessInitDialog(HWND hDefFocusControl) { return(TRUE); }
	virtual  void		ProcessWmCommand(short controlId, short notificationCode, HWND hWndControl);
	virtual  void		ProcessWmTimer(int timerId) { }
	virtual  void		ProcessWmDestroy() { }
						// These methods are primarily the callback stubs for overriding them in derived classes.

	//
	//  Implementation of the base class API.
	//

	virtual void	HandleTrace(const wchar_t *message, WORD highlight_mode = DefHighlightMode);
	virtual int		ShowRunTimeError(const wchar_t *message, int buttons = CONS_BTN_OK, bool inc_errors_count = TRUE);
	virtual bool	IsUserResponseSupported() const { return(TRUE); }
	virtual  void	ProcessWindowsMessages() { TBasicDialog::ProcessPendingMessages(); }

	virtual void	OpenConsole();
	virtual void	CloseConsole();
	virtual bool	IsOpened() const { return(m_panel_dialog.GetWnd() != NULL); }

	virtual int		OpenModalConsole();
					// This method returns control when console is already closed. There is no API to close this console programmatically.
					// Return value is the parameter of the EndDialog call that has closed the console.

	virtual void	SetMajorStatus(const wchar_t *msg = NULL);
	virtual void	SetMinorStatus(const wchar_t *msg = NULL);
	virtual void	SetConsoleLabel(int labelId, const wchar_t *msg = NULL);
	virtual void	SetControlButtonState(const wchar_t *label, bool enabled_state = TRUE);

protected:

	bool								m_show_run_time_errors_as_popups;

	bool								m_want_control_button;
	int								m_panel_width_in_dbu;
	int								m_panel_height_in_dbu;

	TStatusPanelDialog					m_panel_dialog;
	HWND							m_parent_window;

	POINT							m_saved_window_pos;
	bool								m_restore_window_pos;

	TSavedConsoleControlState40		m_title_state;
	TSavedConsoleControlState80		m_major_status;
	TSavedConsoleControlState80		m_minor_status;

	TSavedConsoleControlState40		m_control_button_state;
	bool								m_control_button_enabled;

	friend class TStatusPanelDialog;
};

#endif	// WinUi_DialogBasedConsole_H


