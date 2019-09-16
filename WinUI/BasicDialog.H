//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	WinUi_BasicDialog_H
#define	WinUi_BasicDialog_H

#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif
#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif
#ifndef   WinUi_DialogControlHelpers_H
#include  "WinUi/DialogControlHelpers.H"
#endif
#ifndef   WinUi_ControlsPositioningHelpers_H
#include  "WinUi/ControlsPositioningHelpers.H"
#endif

//
//  Base class for all dialogs of the project.
//
//  An object that handles the non modal dialog can be destroyed at any time. Destructor will close
//  the window if it is opened. Objects that handle modal dialog boxes cannot be destroyed before
//  closing the dialog box.
//
class TBasicDialog : public TListItem
{
public:

	TBasicDialog(HINSTANCE hInst = NULL, short dialogTemplateIdent = 0);
	~TBasicDialog();

	void		SetDialogParams(TBasicDialog *proto, short dialogTemplateId = 0);

	void		SetDialogIcons(HINSTANCE hInst, short rcIconBig, short rcIconSmall);
	void		SetDialogTemplate(HINSTANCE hInst, short dialogTemplateId) { if (hInst != NULL) m_hInst = hInst; m_dialogTemplateIdent = dialogTemplateId; }
	void		SetMessageBoxTitle(const wchar_t *mbTitle) { m_msgBoxTitle = (wchar_t*)mbTitle; }

	HWND	OpenNonModalDialog(HWND hParent, int windowId = 0, short dialogTemplateId = 0);
	HWND	OpenNonModalDialogIndirect(HWND hParent, int windowId, DLGTEMPLATE *dialogTemplate);
				//
				//  Note: In fact, Windows keeps id of the window and hMenu in the same place. Because of that
				//  it is not possible to specify non zero window Id for the top level dialog with a menu.
				//
				//  To close the non modal dialog, the following Windows API can be called at any point:
				//
				//      DestroyWindow(m_hDlg);
				//

	int		OpenModalDialog(HWND hParent, short dialogTemplateIdent = 0);
	int		OpenModalDialogIndirect(HWND hParent, DLGTEMPLATE *dialogTemplate);
				//
				//  Return value of these methods is the second parameter of the EndDialog() function
				//  that closed the dialog.
				//
				//  To close the window, call the following Windows API from inside WM_COMMAND handler or
				//  from handler of some other appropriate message:
				//
				//      EndDialog(m_hDlg, <dialog_box_return_value>);
				//

	HWND		GetWnd() const					{ return(m_hDlg);  }
	bool			GetModalFlag() const			{ return(m_modalDialog); }
	HINSTANCE	GetInst() const					{ return(m_hInst); }
	short		GetDialogTemplateId() const	{ return(m_dialogTemplateIdent); }

	//
	//  Title of the dialog and messages on the controls.
	//

	void		SetTitleText(const wchar_t *titleText) { ::SetWindowTextW(m_hDlg, titleText); }
	void		SetTitleTextFmt(const wchar_t *format, ...);

	void		GetControlText(int rcId, wchar_t *ctrlTextBuff, int ctrlTextBuffLen) { ::GetDlgItemTextW(m_hDlg, rcId, ctrlTextBuff, ctrlTextBuffLen); }
	void		SetControlText(int rcId, const wchar_t *ctrlText) { ::SetDlgItemTextW(m_hDlg, rcId, ctrlText); }
	void		SetControlTextFmt(int rcId, const wchar_t *format, ...);
				// Note that these methods do not work with custom controls due to the problems with UNICODE in Windows.

	bool		SetControlColor(int rcId, TColor frgrColor);
	bool		SetControlBkgrBrush(int rcId, HBRUSH hBrush, bool customBrush);
	void		ResetControlColor(int rcId);
	void		ResetControlBkgrBrush(int rcId);

	bool		SetControlFont(int rcId, const wchar_t *fontFamilyName, int fontSize, bool fontBold = FALSE, bool fontItalic = FALSE);
				// The name of the font family should be the static string.

	//
	//  Moving and/or resizing the dialog and its controls.
	//

	void		CenterOverTheWindow(HWND hWnd, long offs_x = 0, long offs_y = 0) { TControlHelper::CenterOverTheWindow(m_hDlg, hWnd, offs_x, offs_y); }
	void		CenterOverTheParent(long offs_x = 0, long offs_y = 0) { CenterOverTheWindow(::GetParent(m_hDlg), offs_x, offs_y); }
	void		CenterOverTheScreenRect(RECT &rcScreenRect) { TControlHelper::CenterOverTheScreenRect(m_hDlg, rcScreenRect); }

	bool		GetControlRect(int rcId, RECT &rcControl);
	bool		GetControlPos(int rcId, long &pos_x, long &pos_y);
	bool		GetControlSize(int rcId, long &size_x, long &size_y);

	void		SetControlRect(int rcId, RECT &rcControl);
	void		SetControlPos(int rcId, long pos_x, long pos_y);
	void		SetControlSize(int rcId, long size_x, long size_y);

	void		SetupControlsSize(const short *rcIdsArray, long size_x, long size_y);

	void		ShiftControl(int rcId, long shift_x, long shift_y);
	void		ShiftControlsList(const short *rcIdsArray, long shift_x, long shift_y);

	void		AdjustControlRightSide(int rcId);
				// Make offset from the right side of the control to the right side of the dialog equal to the similar left offset.

	void		SetupControlsSpacing(const short *rcIdsArray, int even_spacing_x = -1, int even_spacing_y = -1);
				// Method applies even spacing to the passed list of controls. If spacing value is omitted, then the spacing
				// value is taken from the current shift between the first 2 controls.

	bool		SetupControlPositioning(int rcId, TControlPosMode positioningMode);
	bool		SetupControlsListPositioning(const TControlPosInfo *controlsInfo);

	//
	//  Style and visiblity of the controls.
	//

	void		ControlStyleBitsOr(int rcId, DWORD stylesToSet, bool repaintControlNow = FALSE);
	void		ControlStyleBitsAnd(int rcId, DWORD stylesToReset, bool repaintControlNow = FALSE);

	void		EnableControl(int rcId, bool value)	{ ::EnableWindow(::GetDlgItem(m_hDlg, rcId), value); }
	void		ShowControl(int rcId, bool value)	{ ::ShowWindow(::GetDlgItem(m_hDlg, rcId), (value == TRUE) ? SW_SHOW : SW_HIDE); }

	bool		IsControlEnabled(int rcId)			{ return((::IsWindowEnabled(::GetDlgItem(m_hDlg, rcId)) != 0) ? TRUE : FALSE); }
	bool		IsControlVisible(int rcId)			{ return((::IsWindowVisible(::GetDlgItem(m_hDlg, rcId)) != 0) ? TRUE : FALSE); }

	void		SetCheckMark(int rcid, bool value)	{ ::SendDlgItemMessage(m_hDlg, rcid, BM_SETCHECK, (WPARAM)((value == TRUE) ? BST_CHECKED : BST_UNCHECKED), 0); }
	bool		GetCheckMark(int rcid)			{ return(::SendDlgItemMessage(m_hDlg, rcid, BM_GETCHECK, 0, 0) == BST_CHECKED); }

	void		SetFocusControl(int rcId)			{ ::SetFocus(::GetDlgItem(m_hDlg, rcId)); }

	//
	//  Min/max dialog size.
	//

	void		GetMinTrackingSize(POINT &pt)	{ pt = m_minDialogSize; }

	void		SetMinTrackingSize();
				// This method sets the minimum tracking size to the current size of the dialog.

	void		SetMinTrackingSize(RECT &rc) { m_minDialogSize.x = rc.right-rc.left; m_minDialogSize.y = rc.bottom-rc.top; }
	void		SetMinTrackingSize(int size_x, int size_y) { m_minDialogSize.x = size_x; m_minDialogSize.y = size_y; }
	void		SetMaximumTrackingSize(int size_x, int size_y);
				// Windows requires that caller should pass the window size, not the client size.
				// SetMaxTrackingSize() can accept zeroes, this refers to the size of the screen.

	void		UpdateDialogWindow() { ::UpdateWindow(m_hDlg); }

	int		MsgBox(const wchar_t *message, int actionButtons = MB_OK, long offs_x = 0, long offs_y = 0);
	int		MsgBoxFmt(int actionButtons, const wchar_t *format, ...);
				// Both methods display modal message box that is centered over the current dialog if it is opened.

	//
	//  Handling Windows messages.
	//

	virtual  LRESULT	OnInitDialog(HWND hDefFocusControl) { return(TRUE); }
	virtual  void		OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	virtual  void		OnWmSysCommand(int cmd)  { }
	virtual  void		OnWmNotify(NMHDR *nmhdr) { }
	virtual  void		OnWmTimer(int timerId)  { }

	virtual  void		OnWmSize() { }
						// This handler is called AFTER moving/resizing controls with the built in resizing helper.

	virtual  void		OnWmHorzScroll(int notificationCode, WORD currPos, HWND hControl)  { }
	virtual  void		OnWmVertScroll(int notificationCode, WORD currPos, HWND hControl)  { }
	virtual  void		OnWmMoving(RECT *pRect)  { }
	virtual  void		OnWmWindowPosChanged(WINDOWPOS *pInfo)  { }
	virtual  void		OnWmUser(int msgCode, WPARAM wParam, LPARAM lParam) { }

	virtual  bool		OnWmDestroy() { return(FALSE); }
						//
						// Return value from this method shows if the object was destroyed or not. Derived class can delete
						// the object from inside this method. In this case it should return the appropriate return value to show
						// what action was taken. It is the responsibility of application to allocate instance of the self deletable
						// dialog on the heap.
						//
						// This method has the following non obvious aspect. If application code decides to delete instance of
						// derived class that calls "delete this" from its OnWmDestroy() method, then destructor of that class
						// is called first and after that the destructor of TBasicDialog class. Destructor of TBasicDialog actually
						// closes the opened window and its WM_DESTROY handler will call OnWmDestroy(). But at this time
						// it will be OnWmDestroy() of the TBasicDialog, not OnWmDestroy() of the derived class. This trick
						// prevents recursion and no special precautions are needed in the code of the derived class.
						//
public:

	static  void		RotateMessageLoop();
						// This function picks up and dispatches messages until the list of non modal dialogs becomes empty
						// or until the WM_QUIT message is received.

	static  void		RotateMessageLoopWhileIsWindow(HWND hWnd);
						// This function dispatches messages from the queue to all windows and not only to the window that is
						// passed as a parameter. Function exits either when the passed window handle is destroyed or when
						// the WM_QUIT message is received.

	static  void		ProcessPendingMessages(int minProcessingTimeInMilliseconds = 0);
						// This function is similar to the RotateMessageLoop() method, only it exits when all pending messages
						// are processed and the minimum polling time elapses.
						//
						//   When application has just a console window and it is not picking anything from the message queue,
						//   the console window does not seem to be frozen.
						//
						//   When application has console window PLUS opened non modal dialogs and is not processing messages
						//   from the message queue, both the console window and the dialogs are frozen for extensive intervals.
						//

	static void		SetStdParams(short rcIconBig, short rcIconSmall, const wchar_t *msgBoxTitle);
	static bool		CheckStdParamsSet() { return(g_stdParamsAreSet); }
	static void		PingMainUiThread();

protected:

	void			ProcessGetMinMaxInfo(MINMAXINFO *pmmi);
	void			ProcessWmSizeMessage();
	LRESULT		ProcessCtlColorEvents(HDC hDC, HWND hWnd);

	static  bool				IsNonModalDialogMessage(MSG &msg);
	static  LRESULT CALLBACK	NonModalDialogProcedure(HWND, UINT, WPARAM, LPARAM);
	static  LRESULT CALLBACK	ModalDialogBoxProcedure(HWND, UINT, WPARAM, LPARAM);

	static  void		PrepareBasicDialogTemplate(WORD *&buff_ptr, DWORD dlg_style, WORD cnt_controls, int dialog_width, int dialog_height, bool want_font);
	static  void		AddStaticControlDescriptor(WORD *&buff_ptr, WORD ctrl_id, short pos_x, short pos_y, short size_x, short size_y, bool center_text = TRUE);
	static  void		AddCancelButtonDescriptor(WORD *&buff_ptr, short pos_x, short pos_y, short size_x, short size_y);
	static  void		AddDlgTemplateString(WORD *&buff_ptr, const wchar_t *str);

	enum { DLG_TEMPLATE_PLUS_5CTRLS_BUFF_LEN = (sizeof(DLGTEMPLATE)+80+5*(sizeof(DLGITEMTEMPLATE)+80)) };
	enum { BASIC_DIALOG_STYLE = WS_POPUP | WS_CAPTION | WS_SYSMENU };

protected:

	struct TBasicDialogListItem : public TListItem
	{
		TBasicDialog		*m_data;
	};

	enum { FORMATTING_MESSAGE_BUFF_SIZE = 2048 };

	HWND					m_hDlg;							// When the dialog is opened this field is not NULL. Otherwise it is NULL.
	bool						m_modalDialog;
	TBasicDialogListItem		m_nonModalDialogsListItem;

	bool						m_bInWmDestroyHandler;		// This flag is used in destructor to decide if it is necessary to close the opened non
															// modal dialog or not. This also prevents the infinite recursion if OnWmDestroy()
															// decides to delete the object. Application code should not check or modify this flag.
	HINSTANCE				m_hInst;
	short					m_iconIdentBig, m_iconIdentSmall;
	short					m_dialogTemplateIdent;
	DLGTEMPLATE				*m_inMemoryDlgTemplate;		// Ctor sets this field to NULL. Derived class may set this field to non NULL. In this
															// case OpenNonModalDialog() or OpenModalDialog() will use this template without
															// making attempt to load the dialog template from resources.
	TFontsListHelper			m_controlFonts;
	TControlColorHelper		m_controlColors;

	POINT					m_minDialogSize;
	POINT					m_maxDialogSize;
	TControlPosHelper			m_ctrlsPosHelper;

	wchar_t					*m_msgBoxTitle;				// The dialog is not owning this string.

protected:

	static	short			g_stdDialogIconBig;
	static	short			g_stdDialogIconSmall;
	static	wchar_t			g_stdMessageBoxTitle[80];
	static	bool				g_stdParamsAreSet;

private:

	struct TStaticLoader
	{
		TStaticLoader() { TGenericConsole::m_proc_pending_msgs_cbk = ProcessPendingMessages; }
	};

	static	TList				g_nonModalDialogsList;
								//
								//  Static list of all currently opened non modal dialogs. This list consists of TBasicDialogListItem records.
								//

	static	DWORD			g_UiThreadId;					// Identifier of the UI thread of the process.
	static	TStaticLoader		g_StaticLoader;

	friend class TMessageBoxDialog;
	friend class TControlHelper;
};

class TMessageBoxDialog : public TBasicDialog
{
public:

	TMessageBoxDialog(TBasicDialog *owner_and_proto, const wchar_t *message,
							bool self_delete = TRUE, int async_event_command_id = 0);

	virtual LRESULT	OnInitDialog(HWND hDefFocusControl);
	virtual void		OnWmCommand(short controlId, short notificationCode, HWND hWndControl);
	virtual bool		OnWmDestroy();

	static  HWND		ShowNonModal(TBasicDialog *proto, int buttons, const wchar_t *message,
										TMessageBoxDialog *instance_to_reuse = NULL, int async_event_id = 0);
	static  int			ShowModal(TBasicDialog *proto, int buttons, const wchar_t *message, long offs_x = 0, long offs_y = 0);

protected:

	void		CalcStaticTextSize(long &size_x, long &size_y);
	void		ShiftButtonControls(long sh_x, long sh_y);

	static void	PrepareTemplate(DWORD *buffer, int buttons, const wchar_t *message);
	static void	AddButtonControlDescriptor(WORD *&buff_ptr, short &pos_x, WORD button_id, int fDefPushButton);

protected:

	TBasicDialog		*m_owner;
	bool				m_self_delete_flag;
	int				m_async_event_id;

	long				m_offs_x, m_offs_y;

private:

	wchar_t			*m_message;			// This is temp storage for the pointer to the message between the call to the ctor
											// of the object and processing of the WM_INITDIALOG message.
};

#endif	// WinUi_BasicDialog_H


