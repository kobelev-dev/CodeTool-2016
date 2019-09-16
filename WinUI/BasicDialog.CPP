//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "WinUi/BasicDialog.H"

//------------------------------------------------------------------------
//  ===================  TBasicDialog  =========================
//------------------------------------------------------------------------

short	TBasicDialog::g_stdDialogIconBig = 0;
short	TBasicDialog::g_stdDialogIconSmall = 0;
wchar_t	TBasicDialog::g_stdMessageBoxTitle[80] = L" == MessageBox == ";
bool		TBasicDialog::g_stdParamsAreSet = FALSE;
			// Static props of all dialogs in the process.

TList		TBasicDialog::g_nonModalDialogsList;
			// List of all currently opened non modal dialogs.

DWORD	TBasicDialog::g_UiThreadId = ::GetCurrentThreadId();
			// This data field will be inited before entering the function main() on the main thread.

TBasicDialog::TStaticLoader	TBasicDialog::g_StaticLoader;
				// Static loader is needed just to force calling its ctor at the application startup.

TBasicDialog::TBasicDialog(HINSTANCE hInst, short dialogTemplateIdent)
{
	m_hDlg  = NULL;
	m_modalDialog = FALSE;
	m_nonModalDialogsListItem.m_data = this;
	m_bInWmDestroyHandler = FALSE;

	// Setup the resource related props. Use the HINSTANCE that is either provided by the app layer or the instance
	// of the current executable. For some strange reason if dialog is created with hinstance NULL, in general it seems
	// to work fine but attempts to assign icons to the dialog fail.
	m_hInst = (hInst != NULL) ? hInst : ::GetModuleHandle(NULL);
	m_iconIdentBig = g_stdDialogIconBig;
	m_iconIdentSmall = g_stdDialogIconSmall;
	m_dialogTemplateIdent = dialogTemplateIdent;
	m_inMemoryDlgTemplate = NULL;

	m_msgBoxTitle = g_stdMessageBoxTitle;
	m_minDialogSize.x = m_minDialogSize.y = 0;
	m_maxDialogSize.x = m_maxDialogSize.y = 0;
}

TBasicDialog::~TBasicDialog()
{
	if (m_modalDialog == FALSE)
	{
		// This object handles the non modal dialog.
		if (m_hDlg != NULL && m_bInWmDestroyHandler == FALSE)
		{
			// Destructor of the object is called when the window is closed and flag shows that object
			// is destroyed not from the WM_DESTROY handler. Get rid of the opened window.
			::DestroyWindow(m_hDlg);
		}
	}
	else
	{
		// This is a dialog box object. When the object is destroyed, the window should be already closed.
		assert(m_hDlg == NULL);
	}
}

void TBasicDialog::SetDialogParams(TBasicDialog *proto, short dialogTemplateId)
{
	assert(proto != NULL);
	m_hInst = proto->m_hInst;
	m_iconIdentBig = proto->m_iconIdentBig;
	m_iconIdentSmall = proto->m_iconIdentSmall;

	if (dialogTemplateId != 0)
		m_dialogTemplateIdent = dialogTemplateId;

	m_msgBoxTitle = proto->m_msgBoxTitle;
}

void TBasicDialog::SetDialogIcons(HINSTANCE hInst, short rcIconBig, short rcIconSmall)
{
	if (hInst != NULL)
	{
		m_hInst = hInst;
	}

	if (rcIconBig != 0)
	{
		m_iconIdentBig = rcIconBig;
		if (m_hDlg != NULL)
		{
			HICON hIconBig = ::LoadIcon(m_hInst, MAKEINTRESOURCE(rcIconBig));
			::SendMessage(m_hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
		}
	}

	if (rcIconSmall != 0)
	{
		m_iconIdentSmall = rcIconSmall;
		if (m_hDlg != NULL)
		{
			HICON hIconSmall = ::LoadIcon(m_hInst, MAKEINTRESOURCE(rcIconSmall));
			::SendMessage(m_hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
		}
	}
}

HWND TBasicDialog::OpenNonModalDialog(HWND hParent, int windowId, short dialogTemplateId)
{
	assert(m_hDlg == NULL);

	m_hDlg = NULL;
	m_modalDialog = FALSE;

	// Note that the return value from CreateDialogParam()/CreateDialogIndirectParam() is not checked.
	if (m_inMemoryDlgTemplate != NULL)
	{
		// Use the in memory dialog template.
		::CreateDialogIndirectParam(m_hInst, m_inMemoryDlgTemplate, hParent, (DLGPROC)NonModalDialogProcedure, (LPARAM)this);
	}
	else
	{
		// Use the dialog template that is coming from the resources.
		if (dialogTemplateId == 0)
			dialogTemplateId = m_dialogTemplateIdent;

		::CreateDialogParam(m_hInst, MAKEINTRESOURCE(dialogTemplateId), hParent, (DLGPROC)NonModalDialogProcedure, (LPARAM)this);
	}

	// Note that m_hDlg is NOT assigned here explicitly. This is done in the WM_INITDIALOG message handler.
	// At this point it could be even reset back to NULL if the overwritten OnInitDialog() will return -1.
	if (m_hDlg != NULL)
	{
		// Dialog has managed to open.
		if (windowId != 0)
			::SetWindowLong(m_hDlg, GWL_ID, (LONG)windowId);
	}
	else
	{
		// Failure. Picking up the Win32 error is a pure debug feature. When the dialog fails to open, in 99% of
		// cases this happens because either the dialog template with the requested is in the resources is missing
		// or because there are no resources in the excutable/dll at all.
		DWORD lastError = ::GetLastError();
	}

	return(m_hDlg);
}

HWND TBasicDialog::OpenNonModalDialogIndirect(HWND hParent, int windowId, DLGTEMPLATE *explicitTemplate)
{
	m_inMemoryDlgTemplate = explicitTemplate;
	return(OpenNonModalDialog(hParent, windowId));
}

int TBasicDialog::OpenModalDialog(HWND hParent, short dialogTemplateId)
{
	assert(m_hDlg == NULL);

	m_hDlg = NULL;
	m_modalDialog = TRUE;
	int retValue = 0;

	if (m_inMemoryDlgTemplate != NULL)
	{
		// Use the in memory dialog template.
		retValue = (int)::DialogBoxIndirectParam(m_hInst, m_inMemoryDlgTemplate, hParent, (DLGPROC)ModalDialogBoxProcedure, (LPARAM)this);
	}
	else
	{
		// Use dialog template that is coming from the resources.
		if (dialogTemplateId == 0)
			dialogTemplateId = m_dialogTemplateIdent;

		retValue = (int)::DialogBoxParam(m_hInst, MAKEINTRESOURCE(dialogTemplateId), hParent, (DLGPROC)ModalDialogBoxProcedure, (LPARAM)this);
	}

	return(retValue);
}

int TBasicDialog::OpenModalDialogIndirect(HWND hParent, DLGTEMPLATE *explicitTemplate)
{
	assert(m_hDlg == NULL);

	m_hDlg = NULL;
	m_modalDialog = TRUE;
	int retValue = (int)::DialogBoxIndirectParam(m_hInst, explicitTemplate, hParent, (DLGPROC)ModalDialogBoxProcedure, (LPARAM)this);
	return(retValue);
}

void TBasicDialog::SetTitleTextFmt(const wchar_t *format, ...)
{
	// This method allows only limited sizes of the window titles.
	wchar_t messageBuff[FORMATTING_MESSAGE_BUFF_SIZE];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(messageBuff, FORMATTING_MESSAGE_BUFF_SIZE, format, vargs);
	va_end(vargs);

	::SetWindowTextW(m_hDlg, messageBuff);
}

void TBasicDialog::SetControlTextFmt(int rcid, const wchar_t *format, ...)
{
	// This method allows only limited sizes of the messages.
	wchar_t messageBuff[FORMATTING_MESSAGE_BUFF_SIZE];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(messageBuff, FORMATTING_MESSAGE_BUFF_SIZE, format, vargs);
	va_end(vargs);

	::SetDlgItemTextW(m_hDlg, rcid, messageBuff);
}

bool TBasicDialog::SetControlColor(int rcId, TColor frgrColor)
{
	assert(frgrColor != cs_transparent);

	TControlColorHelper::TControlColorInfo *info = m_controlColors.FindControlInfo(rcId);
	if (info != NULL)
	{
		// Set or replace the color in the existing record.
		info->m_frgr_color = frgrColor;
		return(TRUE);
	}
	else
	{
		// Add the new record.
		TControlColorHelper::TControlColorInfo newInfo = { rcId, frgrColor, NULL, FALSE };
		return(m_controlColors.AddControlInfo(newInfo));
	}
}

bool TBasicDialog::SetControlBkgrBrush(int rcId, HBRUSH hBrush, bool customBrush)
{
	assert(hBrush != NULL);

	TControlColorHelper::TControlColorInfo *info = m_controlColors.FindControlInfo(rcId);
	if (info != NULL)
	{
		// Set or replace the brush in the existing record.
		if (info->m_custom_bkgr_brush == TRUE)
			::DeleteObject(info->m_bkgr_brush);
		info->m_bkgr_brush = hBrush;
		info->m_custom_bkgr_brush = customBrush;
		return(TRUE);
	}
	else
	{
		// Add the new record.
		TControlColorHelper::TControlColorInfo newInfo = { rcId, cs_transparent, hBrush, customBrush };
		return(m_controlColors.AddControlInfo(newInfo));
	}
}

void TBasicDialog::ResetControlColor(int rcId)
{
	TControlColorHelper::TControlColorInfo *info = m_controlColors.FindControlInfo(rcId);
	if (info != NULL)
	{
		if (info->m_bkgr_brush != NULL)
		{
			// Reset the frgr color in the existing record.
			info->m_frgr_color = cs_transparent;
		}
		else
		{
			// Both parts of the record are empty.
			m_controlColors.DeleteControlInfo(rcId);
		}
	}
}

void TBasicDialog::ResetControlBkgrBrush(int rcId)
{
	TControlColorHelper::TControlColorInfo *info = m_controlColors.FindControlInfo(rcId);
	if (info != NULL)
	{
		if (info->m_frgr_color != cs_transparent)
		{
			// Reset the bkgr brush in the existing record.
			if (info->m_custom_bkgr_brush == TRUE)
				::DeleteObject(info->m_bkgr_brush);
			info->m_bkgr_brush = NULL;
			info->m_custom_bkgr_brush = FALSE;
		}
		else
		{
			// Both parts of the record are empty.
			m_controlColors.DeleteControlInfo(rcId);
		}
	}
}

bool TBasicDialog::SetControlFont(int rcId, const wchar_t *fontFamilyName, int fontSize, bool fontBold, bool fontItalic)
{
	HFONT hFont = m_controlFonts.Lookup(m_hDlg, fontFamilyName, fontSize, fontBold, fontItalic);
	if (hFont == NULL)
		return(FALSE);

	// The font handle is available. Pass it to the control.
	::SendDlgItemMessage(m_hDlg, rcId, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE);
	return(TRUE);
}

bool TBasicDialog::GetControlRect(int rcId, RECT &rcControl)
{
	HWND hCtrl = ::GetDlgItem(m_hDlg, rcId);
	if (hCtrl == NULL)
		return(FALSE);

	::GetWindowRect(hCtrl, &rcControl);
	::ScreenToClient(m_hDlg, (POINT*)&(rcControl.left));
	::ScreenToClient(m_hDlg, (POINT*)&(rcControl.right));
	return(TRUE);
}

bool TBasicDialog::GetControlPos(int rcId, long &pos_x, long &pos_y)
{
	HWND hCtrl = ::GetDlgItem(m_hDlg, rcId);
	if (hCtrl == NULL)
		return(FALSE);

	RECT rc;
	::GetWindowRect(hCtrl, &rc);
	::ScreenToClient(m_hDlg, (POINT*)&(rc.left));
	pos_x = rc.left;
	pos_y = rc.top;
	return(TRUE);
}

bool TBasicDialog::GetControlSize(int rcId, long &size_x, long &size_y)
{
	HWND hCtrl = ::GetDlgItem(m_hDlg, rcId);
	if (hCtrl == NULL)
		return(FALSE);

	RECT rc;
	::GetClientRect(hCtrl, &rc);
	size_x = rc.right;
	size_y = rc.bottom;
	return(TRUE);
}

void TBasicDialog::SetControlRect(int rcId, RECT &rcControl)
{
	::SetWindowPos(::GetDlgItem(m_hDlg, rcId), NULL, rcControl.left, rcControl.top, rcControl.right-rcControl.left, rcControl.bottom-rcControl.top, SWP_NOZORDER);
}

void TBasicDialog::SetControlPos(int rcId, long pos_x, long pos_y)
{
	::SetWindowPos(::GetDlgItem(m_hDlg, rcId), NULL, pos_x, pos_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void TBasicDialog::SetControlSize(int rcId, long size_x, long size_y)
{
	::SetWindowPos(::GetDlgItem(m_hDlg, rcId), NULL, 0, 0, size_x, size_y, SWP_NOMOVE | SWP_NOZORDER);
}

void TBasicDialog::SetupControlsSize(const short *rcIdsArray, long size_x, long size_y)
{
	assert(rcIdsArray != NULL);

	while (*rcIdsArray != 0)
		SetControlSize(*rcIdsArray++, size_x, size_y);
}

void TBasicDialog::ShiftControl(int rcId, long shift_x, long shift_y)
{
	long pos_x, pos_y;
	GetControlPos(rcId, pos_x, pos_y);
	SetControlPos(rcId, pos_x+shift_x, pos_y+shift_y);
}

void TBasicDialog::ShiftControlsList(const short *rcIdsArray, long shift_x, long shift_y)
{
	assert(rcIdsArray != NULL);

	while (*rcIdsArray != 0)
		ShiftControl(*rcIdsArray++, shift_x, shift_y);
}

void TBasicDialog::AdjustControlRightSide(int rcId)
{
	RECT dialog_rect, control_rect;
	::GetClientRect(m_hDlg, &dialog_rect);
	GetControlRect(rcId, control_rect);

	// Make offset from the right side of the control to the right side of the dialog equal to the similar left offset.
	SetControlSize(rcId, dialog_rect.right-2*control_rect.left, control_rect.bottom-control_rect.top);
}

void TBasicDialog::SetupControlsSpacing(const short *rcIdsArray, int even_spacing_x, int even_spacing_y)
{
	assert(rcIdsArray != NULL);

	// Pick up the id of the first control.
	short rcId1 = *rcIdsArray++;
	if (rcId1 == 0)
		return;

	// Pick up the position of the first control.
	long pos_x, pos_y;
	if (GetControlPos(rcId1, pos_x, pos_y) == FALSE)
		return;

	if (even_spacing_x < 0 || even_spacing_y < 0)
	{
		// An id of the second control is needed.
		short rcId2 = *rcIdsArray;
		if (rcId2 == 0)
			return;

		// Pick up position of the second control.
		long ctrl2_pos_x, ctrl2_pos_y;
		if (GetControlPos(rcId2, ctrl2_pos_x, ctrl2_pos_y) == FALSE)
			return;

		// Assign the shift values.
		if (even_spacing_x < 0)
			even_spacing_x = ctrl2_pos_x-pos_x;
		if (even_spacing_y < 0)
			even_spacing_y = ctrl2_pos_y-pos_y;
	}

	// Loop over the controls and setup the even spacing.
	while (*rcIdsArray != 0)
	{
		pos_x += even_spacing_x;
		pos_y += even_spacing_y;
		SetControlPos(*rcIdsArray++, pos_x, pos_y);
	}
}

bool TBasicDialog::SetupControlPositioning(int rcId, TControlPosMode positioningMode)
{
	bool res = m_ctrlsPosHelper.SetupControl(rcId, positioningMode);
	RECT current_ref_rect;
	::GetClientRect(m_hDlg, &current_ref_rect);
	m_ctrlsPosHelper.RecalcControlProps(m_hDlg, rcId, current_ref_rect);
	return(res);
}

bool TBasicDialog::SetupControlsListPositioning(const TControlPosInfo *controlsInfo)
{
	bool res = m_ctrlsPosHelper.SetupControlsList(controlsInfo);
	RECT current_ref_rect;
	::GetClientRect(m_hDlg, &current_ref_rect);
	m_ctrlsPosHelper.RecalcAllControlsProps(m_hDlg, current_ref_rect);
	return(res);
}

void TBasicDialog::ControlStyleBitsOr(int rcid, DWORD stylesToSet, bool repaintNow)
{
	HWND hCtrl = ::GetDlgItem(m_hDlg, rcid);
	if (hCtrl == NULL)
		return;

	::SetWindowLong(hCtrl, GWL_STYLE, ::GetWindowLong(hCtrl, GWL_STYLE) | stylesToSet);
	::InvalidateRect(hCtrl, NULL, TRUE);

	if (repaintNow == TRUE)
		::UpdateWindow(hCtrl);
}

void TBasicDialog::ControlStyleBitsAnd(int rcid, DWORD stylesToReset, bool repaintNow)
{
	HWND hCtrl = ::GetDlgItem(m_hDlg, rcid);
	if (hCtrl == NULL)
		return;

	::SetWindowLong(hCtrl, GWL_STYLE, ::GetWindowLong(hCtrl, GWL_STYLE) & ~stylesToReset);
	::InvalidateRect(hCtrl, NULL, TRUE);

	if (repaintNow == TRUE)
		::UpdateWindow(hCtrl);
}

void TBasicDialog::SetMinTrackingSize()
{
	RECT rc;
	::GetWindowRect(m_hDlg, &rc);
	SetMinTrackingSize(rc);
}

int TBasicDialog::MsgBox(const wchar_t *message, int actionButtons, long offs_x, long offs_y)
{
	return(TMessageBoxDialog::ShowModal(this, actionButtons, message, offs_x, offs_y));
}

int TBasicDialog::MsgBoxFmt(int actionButtons, const wchar_t *format, ...)
{
	wchar_t message_buff[FORMATTING_MESSAGE_BUFF_SIZE];
	va_list vargs;
	va_start(vargs, format);
	vswprintf(message_buff, FORMATTING_MESSAGE_BUFF_SIZE, format, vargs);
	va_end(vargs);

	return(TMessageBoxDialog::ShowModal(this, actionButtons, message_buff));
}

void TBasicDialog::SetMaximumTrackingSize(int sizeX, int sizeY)
{
	if (sizeX <= 0)
		sizeX = ::GetSystemMetrics(SM_CXSCREEN);

	if (sizeY <= 0)
		sizeY = ::GetSystemMetrics(SM_CYSCREEN);

	m_maxDialogSize.x = sizeX;
	m_maxDialogSize.y = sizeY;
}

void TBasicDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	//
	//  This is a dummy implementation of the WM_COMMAND handler in the base class.
	//  It simply closes the dialog. Derived class should provide its own meaningful handling.
	//
	if (m_hDlg != NULL && controlId == IDCANCEL)
	{
		if (m_modalDialog == FALSE)
		{
			::DestroyWindow(m_hDlg);
		}
		else
		{
			::EndDialog(m_hDlg, IDCANCEL);
		}
	}
}

void TBasicDialog::RotateMessageLoop()
{
	for(;;)
	{
		// Check the loop termination condition at the beginning of the loop.
		if (g_nonModalDialogsList.IsEmpty() == TRUE)
		{
			// There are no more opened dialogs. Break out of the loop.
			PingMainUiThread();
			break;
		}

		MSG msg;
		int bRet = ::GetMessage(&msg, NULL, 0, 0);
		if (bRet <= 0)
			break;

		if (IsNonModalDialogMessage(msg) == TRUE)
			continue;

		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

void TBasicDialog::RotateMessageLoopWhileIsWindow(HWND hWnd)
{
	// Ensure that the passed window handle was created on the current thread.
	assert(::IsWindow(hWnd) == FALSE || ::GetWindowThreadProcessId(hWnd, NULL) == ::GetCurrentThreadId());

	for(;;)
	{
		// Check the loop termination condition at the beginning of the loop.
		if (::IsWindow(hWnd) == FALSE)
			break;

		MSG msg;
		int bRet = ::GetMessage(&msg, NULL, 0, 0);
		if (bRet <= 0)
			break;

		if (IsNonModalDialogMessage(msg) == TRUE)
			continue;

		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	if (g_nonModalDialogsList.IsEmpty() == TRUE)
		PingMainUiThread();
}

void TBasicDialog::ProcessPendingMessages(int minProcessingTimeInMilliseconds)
{
	MSG msg;
	DWORD tickStart = ::GetTickCount();
	for(;;)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
		{
			// The message for processing on the current thread is available.
			if (IsNonModalDialogMessage(msg) == FALSE)
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}

			// The message was processed.
			continue;
		}
		else if ((::GetTickCount()-tickStart) < (DWORD)minProcessingTimeInMilliseconds)
		{
			// Suspend the current thread for a while.
			::Sleep(2);
			continue;
		}

		// None of the actions were taken. Break out of the loop.
		break;
	}

	if (g_nonModalDialogsList.IsEmpty() == TRUE)
		PingMainUiThread();
}

void TBasicDialog::SetStdParams(short rcIconBig, short rcIconSmall, const wchar_t *msgBoxTitle)
{
	g_stdDialogIconBig = rcIconBig;
	g_stdDialogIconSmall = rcIconSmall;
	wcsncpy(g_stdMessageBoxTitle, msgBoxTitle, sizeof(g_stdMessageBoxTitle)/sizeof(wchar_t));
	g_stdMessageBoxTitle[sizeof(g_stdMessageBoxTitle)/sizeof(wchar_t)-1] = 0;

	g_stdParamsAreSet = TRUE;
}

void TBasicDialog::PingMainUiThread()
{
	if (::GetCurrentThreadId() != g_UiThreadId)
	{
		::PostThreadMessage(g_UiThreadId, 0, 0, 0);
	}
}

void TBasicDialog::ProcessGetMinMaxInfo(MINMAXINFO *pmmi)
{
	if (m_minDialogSize.x != 0 || m_minDialogSize.y != 0)
	{
		pmmi->ptMinTrackSize.x = m_minDialogSize.x;
		pmmi->ptMinTrackSize.y = m_minDialogSize.y;
	}

	if (m_maxDialogSize.x != 0 && m_maxDialogSize.y != 0)
	{
		pmmi->ptMaxTrackSize.x = m_maxDialogSize.x;
		pmmi->ptMaxTrackSize.y = m_maxDialogSize.y;
	}
}

void TBasicDialog::ProcessWmSizeMessage()
{
	if (m_ctrlsPosHelper.NumControls() > 0)
	{
		RECT rect;
		::GetClientRect(m_hDlg, &rect);
		m_ctrlsPosHelper.PositionToRect(m_hDlg, rect);
		::InvalidateRect(m_hDlg, NULL, TRUE);
	}

	// Allow derived class to do some extra processing.
	OnWmSize();
}

LRESULT TBasicDialog::ProcessCtlColorEvents(HDC hDC, HWND hWnd)
{
	short ctrlId = (short)::GetWindowLong(hWnd, GWL_ID);
	if (ctrlId <= 0)
		return(FALSE);

	TControlColorHelper::TControlColorInfo *info = m_controlColors.FindControlInfo(ctrlId);
	if (info == NULL)
		return(FALSE);

	// At least something should be not empty in the returned record.
	assert(info->m_frgr_color != cs_transparent || info->m_bkgr_brush != NULL);

	if (::IsWindowEnabled(hWnd) == FALSE)
	{
		// This seems to be a bug in Windows GDI.
		::SetBkColor(hDC, ::GetSysColor(COLOR_GRAYTEXT));
	}

	if (info->m_frgr_color != cs_transparent)
	{
		::SetTextColor(hDC, info->m_frgr_color);
		::SetBkMode(hDC, TRANSPARENT);
	}

	// The return value should be a brush, maybe NULL.
	return((LRESULT)((info->m_bkgr_brush != NULL) ? info->m_bkgr_brush : (::GetSysColorBrush(COLOR_3DFACE))));
}

bool TBasicDialog::IsNonModalDialogMessage(MSG &msg)
{
	for (TListIter<TBasicDialogListItem> iter(g_nonModalDialogsList); iter; ++iter)
	{
		// Testing shows that checking message with a dialog that was created on a different thread
		// works fine. Messages are not lost and they are properly dispatched.
		if (::IsDialogMessage(iter.CurrItem().m_data->GetWnd(), &msg) != 0)
			return(TRUE);
	}

	// Passed message is not directed to any currently opened non modal dialog.
	return(FALSE);
}

LRESULT CALLBACK TBasicDialog::NonModalDialogProcedure(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TBasicDialog *inst = (TBasicDialog*)(INT_PTR)::GetWindowLong(hDlg, GWL_USERDATA);
	if (inst == NULL && message != WM_INITDIALOG)
		return(FALSE);

	switch (message)
	{
		case WM_INITDIALOG:
				{
					inst = (TBasicDialog*)lParam;
					::SetWindowLongPtr(hDlg, GWL_USERDATA, (LONG)(INT_PTR)inst);

					inst->m_hDlg = hDlg;
					inst->SetDialogIcons(NULL, inst->m_iconIdentBig, inst->m_iconIdentSmall);

					LRESULT res = inst->OnInitDialog((HWND)wParam);
					g_nonModalDialogsList.PrependItem(inst->m_nonModalDialogsListItem);

					if (res == -1)
					{
						// App layer decided not to open the dialog.
						::DestroyWindow(hDlg);
					}

					return(res);
				}
				break;

		case WM_COMMAND:
				inst->OnWmCommand((short)LOWORD(wParam), (short)HIWORD(wParam), (HWND)lParam);
				break;

		case WM_SYSCOMMAND:
				inst->OnWmSysCommand((int)wParam);
				break;

		case WM_NOTIFY:
				inst->OnWmNotify((NMHDR*)lParam);
				break;

		case WM_TIMER:
				inst->OnWmTimer((int)wParam);
				break;

		case WM_HSCROLL:
				inst->OnWmHorzScroll(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
				break;

		case WM_VSCROLL:
				inst->OnWmVertScroll(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
				break;

		case WM_MOVING:
				inst->OnWmMoving((RECT*)lParam);
				break;

		case WM_WINDOWPOSCHANGED:
				inst->OnWmWindowPosChanged((WINDOWPOS*)lParam);
				break;

		case WM_GETMINMAXINFO:
				inst->ProcessGetMinMaxInfo((MINMAXINFO*)lParam);
				break;

		case WM_SIZE:
				inst->ProcessWmSizeMessage();
				break;

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
				return(inst->ProcessCtlColorEvents((HDC)wParam, (HWND)lParam));

		case WM_CLOSE:
				{
					// Experiments with debugger show that when the button CANCEL or the Alt-F4 are pressed,
					// Windows generates WM_CLOSE event and after that it generates WM_COMMAND with IDCANCEL.
					// So, this message can be safely ignored if closing request is handled in the WM_COMMAND handler.
				}
				break;

		case WM_DESTROY:
				{
					g_nonModalDialogsList.RemoveItem(inst->m_nonModalDialogsListItem);
					inst->m_bInWmDestroyHandler = TRUE;
					if (inst->OnWmDestroy() == FALSE)
					{
						// Derived class decided not to delete the object. Some of its fields should
						// be cleared because it is necessary to allow opening it later.
						inst->m_hDlg = NULL;
						inst->m_ctrlsPosHelper.ResetHelper();
						inst->m_bInWmDestroyHandler = FALSE;
					}
				}
				break;

		default:
			{
				if (message >= WM_USER && message < WM_USER+1000)
					inst->OnWmUser(message, wParam, lParam);
			}
			break;
	}

	return(FALSE);
}

LRESULT CALLBACK TBasicDialog::ModalDialogBoxProcedure(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TBasicDialog *inst = (TBasicDialog*)(INT_PTR)::GetWindowLong(hDlg, GWL_USERDATA);
	if (inst == NULL && message != WM_INITDIALOG)
		return(FALSE);

	switch (message)
	{
		case WM_INITDIALOG:
				{
					inst = (TBasicDialog*)lParam;
					::SetWindowLongPtr(hDlg, GWL_USERDATA, (LONG)(INT_PTR)inst);

					inst->m_hDlg = hDlg;
					inst->SetDialogIcons(NULL, inst->m_iconIdentBig, inst->m_iconIdentSmall);
					return(inst->OnInitDialog((HWND)wParam));
				}
				break;

		case WM_COMMAND:
				inst->OnWmCommand((short)LOWORD(wParam), (short)HIWORD(wParam), (HWND)lParam);
				break;

		case WM_SYSCOMMAND:
				inst->OnWmSysCommand((int)wParam);
				break;

		case WM_NOTIFY:
				inst->OnWmNotify((NMHDR*)lParam);
				break;

		case WM_TIMER:
				inst->OnWmTimer((int)wParam);
				break;

		case WM_HSCROLL:
				inst->OnWmHorzScroll(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
				break;

		case WM_VSCROLL:
				inst->OnWmVertScroll(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
				break;

		case WM_MOVING:
				inst->OnWmMoving((RECT*)lParam);
				break;

		case WM_WINDOWPOSCHANGED:
				inst->OnWmWindowPosChanged((WINDOWPOS*)lParam);
				break;

		case WM_GETMINMAXINFO:
				inst->ProcessGetMinMaxInfo((MINMAXINFO*)lParam);
				break;

		case WM_SIZE:
				inst->ProcessWmSizeMessage();
				break;

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
				return(inst->ProcessCtlColorEvents((HDC)wParam, (HWND)lParam));

		case WM_CLOSE:
				{
					// Experiments with debugger show that when the button CANCEL or the Alt-F4 are pressed,
					// Windows generates WM_CLOSE event and after that it generates WM_COMMAND with IDCANCEL.
					// So, this message can be safely ignored if closing request is handled in the WM_COMMAND handler.
				}
				break;

		case WM_DESTROY:
				{
					inst->m_bInWmDestroyHandler = TRUE;
					if (inst->OnWmDestroy() == FALSE)
					{
						// Derived class decided not to delete the object. Some of its fields should
						// be cleared because it is necessary to allow opening it later.
						inst->m_hDlg = NULL;
						inst->m_ctrlsPosHelper.ResetHelper();
						inst->m_bInWmDestroyHandler = FALSE;
					}
				}
				break;

		default:
			{
				if (message >= WM_USER && message < WM_USER+1000)
					inst->OnWmUser(message, wParam, lParam);
			}
			break;
	}

	return(FALSE);
}

void TBasicDialog::PrepareBasicDialogTemplate(WORD *&buff_ptr, DWORD dlg_style, WORD cnt_controls, int dialog_width, int dialog_height, bool want_font)
{
	if (want_font == TRUE)
		dlg_style |= DS_SETFONT;

	// Fill in the description of the dialog.
	DLGTEMPLATE *dlg_info = (DLGTEMPLATE*)buff_ptr;
	dlg_info->style = dlg_style;
	dlg_info->dwExtendedStyle = 0;
	dlg_info->cdit = cnt_controls;
	dlg_info->cx = dialog_width;
	dlg_info->cy = dialog_height;

	buff_ptr += sizeof(DLGTEMPLATE)/sizeof(WORD);
	*buff_ptr++ = 0;		// No menu.
	*buff_ptr++ = 0;		// Standard Window class.
	*buff_ptr++ = 0;		// Empty title.

	if (want_font == TRUE)
	{
		// The information about the font is in the DLGTEMPLATE format. Note that the DLGTEMPLATEEX format,
		// as it is described in the MSDN, is different.
		*buff_ptr++ = 8;		// Point size.
		AddDlgTemplateString(buff_ptr, L"MS Sans Serif");
	}
}

void TBasicDialog::AddStaticControlDescriptor(WORD *&buff_ptr, WORD ctrl_id, short pos_x, short pos_y, short size_x, short size_y, bool center_text)
{
	// Align the WORD aligned pointer to the DWORD boundary.
	if ((((DWORD)(DWORD_PTR)buff_ptr) & 3) != 0)
		buff_ptr++;

	// Fill in the control desc header.
	DLGITEMTEMPLATE *ptpl = (DLGITEMTEMPLATE*)buff_ptr;
	ptpl->style = WS_CHILD | WS_VISIBLE | ((center_text == TRUE) ? SS_CENTER : SS_LEFTNOWORDWRAP) | SS_NOPREFIX;
	ptpl->dwExtendedStyle = 0;
	ptpl->x  = pos_x;
	ptpl->y  = pos_y;
	ptpl->cx = size_x;
	ptpl->cy = size_y;
	ptpl->id = ctrl_id;
	buff_ptr += sizeof(DLGITEMTEMPLATE)/sizeof(WORD);

	// Fill in the class, title and the creation data.
	*buff_ptr++ = 0xFFFF;
	*buff_ptr++ = 0x0082;
	*buff_ptr++ = 0x0000;
	*buff_ptr++ = 0x0000;
}

void TBasicDialog::AddCancelButtonDescriptor(WORD *&buff_ptr, short pos_x, short pos_y, short size_x, short size_y)
{
	// Align the WORD aligned pointer to the DWORD boundary.
	if ((((DWORD)(DWORD_PTR)buff_ptr) & 3) != 0)
		buff_ptr++;

	// Fill in the control desc header.
	DLGITEMTEMPLATE *ptpl = (DLGITEMTEMPLATE*)buff_ptr;
	ptpl->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON;
	ptpl->dwExtendedStyle = 0;
	ptpl->x = pos_x;
	ptpl->y = pos_y;
	ptpl->cx = size_x;
	ptpl->cy = size_y;
	ptpl->id = IDCANCEL;
	buff_ptr += sizeof(DLGITEMTEMPLATE)/sizeof(WORD);

	// Fill in the class of the control.
	*buff_ptr++ = 0xFFFF;
	*buff_ptr++ = 0x0080;

	// Fill in the tilte.
	AddDlgTemplateString(buff_ptr, L"Abort");

	// Fill in the creation data.
	*buff_ptr++ = 0;
}

void TBasicDialog::AddDlgTemplateString(WORD *&buff_ptr, const wchar_t *str)
{
	if (str != NULL)
	{
		wcscpy((wchar_t*)buff_ptr, str);
		buff_ptr += wcslen(str)+1;
	}
	else
	{
		*buff_ptr++ = 0;
	}
}

//---------------------------------------------------------------------------
//  ===================  TMessageBoxDialog  =======================
//---------------------------------------------------------------------------

#define	TCMBOX_IDCMSG		60		// Control ID of the static control that keeps the message on the message box.
									// There is no need to define control IDs for the buttons because this message
									// box class supports only stnd Windows action buttons.

#define	TCMBOX_HSP1		13		// Min space on the sides of the message.
#define	TCMBOX_HSP2		20		// Min space on the sides of the buttons.
#define	TCMBOX_HSP3		12		// Spacing between the buttons.

#define	TCMBOX_VSP1		 8		// Space above the message.
#define	TCMBOX_VSP2		12		// Space between the message and buttons.
#define	TCMBOX_VSP3		 9		// Space below the buttons.

#define	TCMBOX_BTNX		50		// Width of the button.
#define	TCMBOX_BTNY		14		// Height of the button.

TMessageBoxDialog::TMessageBoxDialog(TBasicDialog *owner_and_proto, const wchar_t *message, bool self_delete, int async_event_command_id)
{
	SetDialogParams(owner_and_proto);
	m_owner = owner_and_proto;
	m_self_delete_flag = self_delete;
	m_async_event_id = async_event_command_id;
	m_offs_x = m_offs_y = 0;
	m_message = (wchar_t*)message;
}

LRESULT TMessageBoxDialog::OnInitDialog(HWND hDefFocusControl)
{
	// Retrieve and calculate the new coordinates.
	RECT rc_dlg, rc_msg;
	::GetClientRect(m_hDlg, &rc_dlg);
	GetControlRect(TCMBOX_IDCMSG, rc_msg);
	long msg_x, msg_y;
	CalcStaticTextSize(msg_x, msg_y);

	// Resize the dialog and shift controls in the horz direction.
	int delta_x = 2*rc_msg.left+msg_x - rc_dlg.right;
	if (delta_x > 0)
	{
		// The dialog should be enlarged in the horz direction.
		::GetWindowRect(m_hDlg, &rc_dlg);
		::SetWindowPos(m_hDlg, NULL, 0, 0, rc_dlg.right-rc_dlg.left+delta_x, rc_dlg.bottom-rc_dlg.top, SWP_NOMOVE | SWP_NOZORDER);

		// Center buttons in the enlarged dialog.
		ShiftButtonControls(delta_x/2+1, 0);
	}
	else
	{
		// The width of the dialog is ok. Although message control should be centered.
		SetControlPos(TCMBOX_IDCMSG, (rc_dlg.right-msg_x)/2+1, rc_msg.top);
	}

	// Resize the dialog and shift controls in the vertical direction.
	::GetWindowRect(m_hDlg, &rc_dlg);
	::SetWindowPos(m_hDlg, NULL, 0, 0, rc_dlg.right-rc_dlg.left, rc_dlg.bottom-rc_dlg.top+msg_y, SWP_NOMOVE | SWP_NOZORDER);
	ShiftButtonControls(0, msg_y);

	// Finalize the dialog.
	SetTitleText(m_msgBoxTitle);
	SetControlSize(TCMBOX_IDCMSG, msg_x, msg_y);
	SetControlText(TCMBOX_IDCMSG, m_message);
	m_message = NULL;

	// Position the dialog above its parent.
	CenterOverTheParent(m_offs_x, m_offs_y);

	::ShowWindow(m_hDlg, SW_SHOW);
	return(TRUE);
}

void TMessageBoxDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	// The message box should be closed on any WM_COMMAND event.
	if (m_modalDialog == FALSE)
	{
		if (m_async_event_id != 0)
		{
			::PostMessage(m_owner->GetWnd(), WM_COMMAND, MAKEWPARAM(m_async_event_id, controlId), (LPARAM)m_hDlg);
		}

		// Close as non modal mode requires this.
		::DestroyWindow(m_hDlg);
	}
	else
	{
		// Close as modal dialog.
		::EndDialog(m_hDlg, controlId);
	}
}

bool TMessageBoxDialog::OnWmDestroy()
{
	if (m_self_delete_flag == TRUE)
	{
		// App wants to get rid of the object.
		delete this;
		return(TRUE);
	}

	// Object was not deleted.
	return(TBasicDialog::OnWmDestroy());
}

HWND TMessageBoxDialog::ShowNonModal(TBasicDialog *proto, int buttons, const wchar_t *message, TMessageBoxDialog *instance_to_reuse, int event_id)
{
	DWORD dialog_template[DLG_TEMPLATE_PLUS_5CTRLS_BUFF_LEN/sizeof(DWORD)];
	memset(dialog_template, 0, sizeof(dialog_template));
	PrepareTemplate(dialog_template, buttons, message);

	// Prepare or create the dialog object and show it.
	TMessageBoxDialog *inst = NULL;
	if (instance_to_reuse != NULL)
	{
		// Application passed reusable message box.
		inst = instance_to_reuse;

		if (instance_to_reuse->GetWnd() != NULL)
		{
			// The window is opened. Close it.
			::DestroyWindow(inst->GetWnd());
		}

		// Setup the new props.
		instance_to_reuse->m_owner = proto;
		instance_to_reuse->m_async_event_id = event_id;
		instance_to_reuse->m_message = (wchar_t*)message;
	}
	else
	{
		// Create new instance of the dialog.
		inst = new TMessageBoxDialog(proto, message, TRUE, event_id);
	}

	if (inst == NULL)
	{
		assert(FALSE);
		return(NULL);
	}

	HWND res = inst->OpenNonModalDialogIndirect(proto->GetWnd(), event_id, (DLGTEMPLATE*)dialog_template);
	return(res);
}

int TMessageBoxDialog::ShowModal(TBasicDialog *proto, int buttons, const wchar_t *message, long offs_x, long offs_y)
{
	DWORD dialog_template[DLG_TEMPLATE_PLUS_5CTRLS_BUFF_LEN/sizeof(DWORD)];
	memset(dialog_template, 0, sizeof(dialog_template));
	PrepareTemplate(dialog_template, buttons, message);

	// Create the dialog object and show it.
	TMessageBoxDialog mbox(proto, message, FALSE);
	mbox.m_offs_x = offs_x;
	mbox.m_offs_y = offs_y;
	int res = mbox.OpenModalDialogIndirect(proto->GetWnd(), (DLGTEMPLATE*)dialog_template);
	return(res);
}

void TMessageBoxDialog::PrepareTemplate(DWORD *buffer, int buttons, const wchar_t *message)
{
	//
	//  This function will prepares the template prototype. The number of controls will be correct there.
	//  Size of the dialog and sizes/positions of the controls will be adjusted later on inside the OnInitDialog().
	//
	//  The message is always placed into one static control regardless of how many actual lines od text it has.
	//  Messages without line feeds are displayed as centered, otherwise they are left aligned.
	//

	int  nButtons = 0;
	WORD *buff_ptr = (WORD*)buffer;

	// Fill in the description of the dialog.
	DLGTEMPLATE *dlg_info = (DLGTEMPLATE*)buff_ptr;
	dlg_info->style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT;
	dlg_info->cdit = 0;
	dlg_info->cx = 2*TCMBOX_HSP2;
	dlg_info->cy = TCMBOX_VSP1+TCMBOX_VSP2+TCMBOX_BTNY+TCMBOX_VSP3;
	buff_ptr += sizeof(DLGTEMPLATE)/sizeof(WORD);
	*buff_ptr++ = 0;	// No menu.
	*buff_ptr++ = 0;	// Standard Window class.
	*buff_ptr++ = 0;	// Empty title.

	// The information about the font is in the DLGTEMPLATE format.
	// Note that the DLGTEMPLATEEX format, as it is described in the MSDN, is different.
	*buff_ptr++ = 8; // Point size.
	AddDlgTemplateString(buff_ptr, L"MS Sans Serif");

	// Fill in description of the static text.
	bool fNoLineFeed = (wcschr(message, L'\n') != NULL) ? FALSE : TRUE;
	AddStaticControlDescriptor(buff_ptr, TCMBOX_IDCMSG, TCMBOX_HSP1, TCMBOX_VSP1, 20, 20, fNoLineFeed);
	dlg_info->cdit++;

	// Fill in the descriptions of the buttons.
	short pos_x = TCMBOX_HSP2;
	switch (buttons & MB_TYPEMASK)
	{
		case MB_OK:
				AddButtonControlDescriptor(buff_ptr, pos_x, IDOK, buttons & MB_DEFBUTTON1);
				nButtons = 1;
				break;

		case MB_OKCANCEL:
				AddButtonControlDescriptor(buff_ptr, pos_x, IDOK, buttons & MB_DEFBUTTON1);
				AddButtonControlDescriptor(buff_ptr, pos_x, IDCANCEL, buttons & MB_DEFBUTTON2);
				nButtons = 2;
				break;

		case MB_ABORTRETRYIGNORE:
				AddButtonControlDescriptor(buff_ptr, pos_x, IDABORT, buttons & MB_DEFBUTTON1);
				AddButtonControlDescriptor(buff_ptr, pos_x, IDRETRY, buttons & MB_DEFBUTTON2);
				AddButtonControlDescriptor(buff_ptr, pos_x, IDIGNORE, buttons & MB_DEFBUTTON3);
				nButtons = 3;
				break;

		case MB_YESNOCANCEL:
				AddButtonControlDescriptor(buff_ptr, pos_x, IDYES, buttons & MB_DEFBUTTON1);
				AddButtonControlDescriptor(buff_ptr, pos_x, IDNO, buttons & MB_DEFBUTTON2);
				AddButtonControlDescriptor(buff_ptr, pos_x, IDCANCEL, buttons & MB_DEFBUTTON3);
				nButtons = 3;
				break;

		case MB_YESNO:
				AddButtonControlDescriptor(buff_ptr, pos_x, IDYES, buttons & MB_DEFBUTTON1);
				AddButtonControlDescriptor(buff_ptr, pos_x, IDNO, buttons & MB_DEFBUTTON2);
				nButtons = 2;
				break;

		case MB_RETRYCANCEL:
				AddButtonControlDescriptor(buff_ptr, pos_x, IDRETRY, buttons & MB_DEFBUTTON1);
				AddButtonControlDescriptor(buff_ptr, pos_x, IDCANCEL, buttons & MB_DEFBUTTON2);
				nButtons = 2;
				break;
	}

	// Update the header with the info about the buttons.
	if (nButtons > 0)
	{
		// Recent compiler has problems understanding +=.
		dlg_info->cx = dlg_info->cx + (short)(nButtons*TCMBOX_BTNX + (nButtons-1)*TCMBOX_HSP3);
		dlg_info->cdit = dlg_info->cdit + (short)nButtons;
	}
}

void TMessageBoxDialog::AddButtonControlDescriptor(WORD *&buff_ptr, short &pos_x, WORD buttonId, int fDefPushButton)
{
	// Align pointer to the DWORD boundary.
	if ((((DWORD)(DWORD_PTR)buff_ptr) & 3) != 0)
		buff_ptr++;

	// Fill in the control desc header.
	DLGITEMTEMPLATE *ptpl = (DLGITEMTEMPLATE*)buff_ptr;
	ptpl->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
	if (fDefPushButton != 0)
		ptpl->style |= BS_DEFPUSHBUTTON;
	ptpl->x  = pos_x;
	ptpl->y  = TCMBOX_VSP1+TCMBOX_VSP2;
	ptpl->cx = TCMBOX_BTNX;
	ptpl->cy = TCMBOX_BTNY;
	ptpl->id = buttonId;
	buff_ptr += sizeof(DLGITEMTEMPLATE)/sizeof(WORD);

	// Fill in the class of the control.
	*buff_ptr++ = 0xFFFF;
	*buff_ptr++ = 0x0080;

	// Fill in the tilte.
	switch (buttonId)
	{
		case IDOK:		AddDlgTemplateString(buff_ptr, L"OK");		break;
		case IDCANCEL:	AddDlgTemplateString(buff_ptr, L"Cancel");	break;
		case IDABORT:	AddDlgTemplateString(buff_ptr, L"Abort");		break;
		case IDRETRY:	AddDlgTemplateString(buff_ptr, L"Retry");		break;
		case IDIGNORE:	AddDlgTemplateString(buff_ptr, L"Ignore");	break;
		case IDYES:		AddDlgTemplateString(buff_ptr, L"Yes");		break;
		case IDNO:		AddDlgTemplateString(buff_ptr, L"No");		break;
		case IDCLOSE:	AddDlgTemplateString(buff_ptr, L"Close");	break;
		default:			AddDlgTemplateString(buff_ptr, L"UnknId");	break;
	}

	// Fill in the creation data.
	*buff_ptr++ = 0;

	// Adjust the position for the next button.
	pos_x += TCMBOX_BTNX+TCMBOX_HSP3;
}

void TMessageBoxDialog::CalcStaticTextSize(long &size_x, long &size_y)
{
	// Prepare DC for making evaluations.
	size_x = size_y = 20;
	HWND hWnd = ::GetDlgItem(m_hDlg, TCMBOX_IDCMSG);
	HDC  hDC  = ::GetDC(hWnd);
	if (hDC == NULL)
		return;

	HFONT hFont = (HFONT)::SendMessage(hWnd, WM_GETFONT, 0, 0);
	HFONT hFontOld = NULL;
	long fontHeight = 4;
	if (hFont != NULL)
	{
		hFontOld = (HFONT)::SelectObject(hDC, hFont);

		TEXTMETRIC fontMetrics;
		::GetTextMetrics(hDC, &fontMetrics);
		fontHeight = fontMetrics.tmHeight;
	}

	if (wcschr(m_message, L'\n') != NULL)
	{
		// The text contains the line feed characters. Its width will be the width of the longest string
		// and height will be determined from the number of the lines in the text.
		size_x = size_y = 0;
		wchar_t *ptr = m_message;
		for(;;)
		{
			wchar_t *ptr_lf = wcschr(ptr, L'\n');
			int len = (ptr_lf != NULL) ? (int)(INT_PTR)(ptr_lf-ptr) : (int)wcslen(ptr);
			if (len != 0)
			{
				SIZE sz;
				::GetTextExtentPoint32W(hDC, ptr, len, &sz);
				if (sz.cx > size_x)
					size_x = sz.cx;
				size_y += sz.cy;
			}
			else
			{
				size_y += fontHeight;
			}

			ptr += len;
			if (*ptr == 0)
				break;
			ptr++;
		}
	}
	else
	{
		// The text does not have explicit line feeds. Nevertheless it can be still multiline and windows will
		// do the line breaking. Find out the width of the widest word in the message.
		long widest_word_width = 0;
		wchar_t *ptr = m_message;
		wchar_t *ptr_word_beg = NULL;
		for(;;)
		{
			wchar_t ch = *ptr;
			if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n' || ch == L'\0')
			{
				// Current character is space.
				if (ptr_word_beg != NULL)
				{
					// This is an end of some word. Check its size.
					SIZE sz;
					::GetTextExtentPoint32W(hDC, ptr_word_beg, (ptr-ptr_word_beg), &sz);
					if (sz.cx > widest_word_width)
						widest_word_width = sz.cx;

					ptr_word_beg = NULL;
				}
			}
			else
			{
				// Current char is not space.
				if (ptr_word_beg == NULL)
				{
					// Save current pointer as the beginning of the new word.
					ptr_word_beg = ptr;
				}
			}

			if (ch == 0)
				break;
			ptr++;
		}

		// The text will be centered and wrapped.
		size_x = __max(widest_word_width+10, 300);
		size_y = 60;
	}

	// Release the GDI resources.
	if (hFont != NULL)
		::SelectObject(hDC, hFontOld);
	::ReleaseDC(hWnd, hDC);
}

void TMessageBoxDialog::ShiftButtonControls(long sh_x, long sh_y)
{
	for (short id = IDOK; id <= IDCLOSE; ++id)
	{
		if (::GetDlgItem(m_hDlg, id) != NULL)
		{
			RECT rc;
			GetControlRect(id, rc);
			SetControlPos(id, rc.left+sh_x, rc.top+sh_y);
		}
	}
}


