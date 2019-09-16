//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Common/DebugLogger.H"
#include  "WinUi/ListBoxObject.H"
#include  "ScreenItems/ScreenItemsView.H"
#include  "LangView/LangView.H"
#include  "LangView/LangViewResources.H"

// ---------------------------------------------------------------------------
//   =================  TLangViewChildBrowser =======================
// ---------------------------------------------------------------------------

void TLangViewChildBrowser::ResetUserCancel()
{
	if (m_owner != NULL)
		m_owner->SetUserCancel(FALSE);
}

void TLangViewChildBrowser::SendForProcessingFromListBox(TListBoxNotificationCode notificationCode, TLangViewClickEvent &info)
{
	if (m_owner == NULL)
		return;

	switch (notificationCode)
	{
		case lbnc_left_btn_down:
				m_owner->ProcessSingleClick(this, info);
				break;

		case lbnc_left_btn_dblclk:
				m_owner->ProcessDoubleClick(this, info);
				break;
	}
}

void TLangViewChildBrowser::SendForProcessingFromScreenItemsView(TScreenItemsViewNtfCode notificationCode, TLangViewClickEvent &info)
{
	if (m_owner == NULL)
		return;

	switch (notificationCode)
	{
		case xiv_left_btn_down:
				m_owner->ProcessSingleClick(this, info);
				break;

		case xiv_left_btn_dblclk:
				m_owner->ProcessDoubleClick(this, info);
				break;
	}
}

static TMenuItemInfo g_ScreenItemsViewControlContextMenu[] =
{
	{ 0, 810,	L"Zoom In"						},	// 0
	{ 0, 811,	L"Zoom Out"						},	// 1
	{ 0, 812,	L"Zoom Out (bigger step)"			},	// 2
	{ 0, 813,	L"Reset Zoom"					},	// 3
	{ MF_SEPARATOR, -1, NULL					},	// 4
	{ 0, 850,	L"Set concentric placing mode"		},	// 5
	{ 0, 851,	L"Set rect lines placing mode"		},	// 6
	{ MF_SEPARATOR, -1, NULL					},	// 7
	{ 0, 890,	L"Copy to clipboard as Bitmap"		},	// 8
	{ 0, 0, NULL },								// 9
};

TLangViewChildBrowser::TSIVMR TLangViewChildBrowser::ProcessScreenItemsViewControlMenu(TScreenItemsViewObject &view_helper, TTreeItemsPlacingMode &placing_mode, long click_x, long click_y)
{
	// Allocate local copy of the menu info and set the check marks there.
	TMenuItemInfo menu_info[sizeof(g_ScreenItemsViewControlContextMenu)/sizeof(TMenuItemInfo)];
	memcpy(menu_info, g_ScreenItemsViewControlContextMenu, sizeof(g_ScreenItemsViewControlContextMenu));

	TScreenItemsViewSpaceInfo space_info;
	view_helper.GetSpaceInfo(space_info);
	if (space_info.zoom_level > 0.9999 && space_info.zoom_level < 1.00001)
		menu_info[3].item_flags |= MF_GRAYED;
	if (placing_mode == plc_mode_concentric)
		menu_info[5].item_flags |= MF_CHECKED;
	else if (placing_mode == plc_mode_rect_lines)
		menu_info[6].item_flags |= MF_CHECKED;

	// Display the menu and process its selection if any.
	DWORD res = view_helper.ShowAndTrackPopupMenu(menu_info, click_x, click_y);
	if (res == 810)
	{
		// Zoom in the current view. The view helper will repaint itself.
		view_helper.SetZoomLevel((float)(space_info.zoom_level * 1.4));
	}
	else if (res == 811)
	{
		// Zoom out the current view. The view helper will repaint itself.
		view_helper.SetZoomLevel((float)(space_info.zoom_level / 1.4));
	}
	else if (res == 812)
	{
		// Zoom out the current view using a big step. The view helper will repaint itself.
		view_helper.SetZoomLevel((float)(space_info.zoom_level / 3.0));
	}
	else if (res == 813)
	{
		// Reset the zoom. The view helper will repaint itself.
		view_helper.SetZoomLevel(1.0);
	}
	else if (res == 850)
	{
		// Change the placing mode.
		placing_mode = plc_mode_concentric;
		return(sivmr_reload_view);
	}
	else if (res == 851)
	{
		// Change the placing mode.
		placing_mode = plc_mode_rect_lines;
		return(sivmr_reload_view);
	}
	else if (res == 890)
	{
		// Ask the upper layer to place the contents of the viewer to the clipboard.
		return(sivmr_copy_to_clipborad);
	}

	// User has pressed cancel or the upper layer does not need to do anything.
	return(sivmr_none);
}

void TLangViewChildBrowser::PutScreenShotToClipboard(TScreenItemsViewObject &items_view, RECT *margins_rect, ClipboardImageGenCallback extra_cbk, void *cbk_context)
{
	HDC hMemoryDC;
	HBITMAP hBitmapObject, hBitmapOld;
	SIZE bitmap_size;

	DWORD err_code = items_view.CreateScreenShotProto(margins_rect, hMemoryDC, hBitmapObject, hBitmapOld, bitmap_size);

	if (err_code == ERROR_SUCCESS)
	{
		// Creating bitmap with the screen shot succeeded. Do the extra painting if needed.
		if (extra_cbk != NULL)
		{
			// Upper layer wants to modify the image.
			(*extra_cbk)(cbk_context, hMemoryDC, bitmap_size);
		}

		items_view.ScreenShotProtoCleanup(hMemoryDC, hBitmapOld);
		DWORD err_code = TControlHelper::PutBitmapToClipboard(hBitmapObject);
		if (err_code != ERROR_SUCCESS)
			MsgBoxFmt(MB_OK, L"Error putting the screen shot to the clipboard (Win32 error: %d).", err_code);
	}
	else
	{
		MsgBoxFmt(MB_OK, L"Error picking up the screen shot (Win32 error: %d).", err_code);
	}
}

// --------------------------------------------------------------------------
//   =================  TLangViewBrowserFrame =====================
// --------------------------------------------------------------------------

TLangViewBrowserFrame::TLangViewBrowserFrame(TLangViewBrowserFrame *parent_handler, TMidLevStoreReader *db_link, short resource_template_id)
		: TBasicDialog(NULL, resource_template_id)
{
	m_parent = parent_handler;
	if (parent_handler != NULL)
		parent_handler->m_child_browsers.AppendItem(this);

	m_db = NULL;
	SetupDataBase(db_link);
	m_frame_console.m_owner_frame = this;
}

bool TLangViewBrowserFrame::OnWmDestroy()
{
	if (m_parent != NULL)
	{
		// Child dialog is about to be destroyed.
		m_parent->ChildDestructionNotification(this);

		// It is ok to plain delete the instance. Look at the comments to TBasicDialog::OnWmDestroy()
		// for details on how the infinite recursion is prevented here.
		delete this;
		return(TRUE);
	}
	else
	{
		// This is a root of the dialogs hierarchy. Do not delete the instance.
		return(FALSE);
	}
}

// --------------------------------------------------------------------------
//   =================  TLangViewConsole =========================
// --------------------------------------------------------------------------

TLangViewConsole::TLangViewConsole(HINSTANCE hInst, const wchar_t *console_title)
					: TDialogBasedConsole(hInst, console_title, TRUE)
{
	m_processing_started = 0;
	m_last_update = 0;
	m_prev_mouse_cursor = NULL;
}

void TLangViewConsole::PrepareStatusPanel(const wchar_t *initial_main_msg, bool want_button)
{
	// Ensure that the open/close logic works properly.
	assert(IsOpened() == FALSE);
	assert(m_prev_mouse_cursor == NULL);

	// Prepare the status panel.
	TDialogBasedConsole::ShowControlButton(want_button);
	TDialogBasedConsole::SetMajorStatus(initial_main_msg);
	TDialogBasedConsole::SetMinorStatus(NULL);
	TDialogBasedConsole::SetControlButtonState(NULL, TRUE);

	// Setup new beginning date time.
	m_processing_started = CurrDateTime();
	m_last_update = 0;

	// Change mouse cursor into the hour glass.
	m_prev_mouse_cursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
}
void TLangViewConsole::CloseStatusPanel()
{
	// Change the mouse cursor back.
	if (m_prev_mouse_cursor != NULL)
	{
		TDateTime msec_elapsed = (CurrDateTime()-m_processing_started)/MILLI_SEC;
		if (msec_elapsed >= 0 && msec_elapsed < 20)
		{
			// Sleep a bit to ensure that the mouse cursor was visible.
			::Sleep((DWORD)(20-msec_elapsed));
		}

		::SetCursor(m_prev_mouse_cursor);
		m_prev_mouse_cursor = NULL;
	}

	// Close the panel and reset the related vars.
	TDialogBasedConsole::CloseConsole();
	m_processing_started = 0;
}

void TLangViewConsole::CheckOpenStatusPanel()
{
	if (TDialogBasedConsole::IsOpened() == FALSE)
	{
		// Check, if it is high time to open the status panel.
		assert(m_processing_started != 0);
		TDateTime curr_time = CurrDateTime();
		if ((curr_time - m_processing_started) > 50*MILLI_SEC)
		{
			// More than 50 ms has elapsed.
			OpenStatusPanelSpecial();
		}
	}
}

void TLangViewConsole::OpenStatusPanelSpecial()
{
	// Open the panel using the proper parent.
	HWND saved_parent = m_parent_window;
	if (m_parent_window != NULL && ::IsWindowVisible(m_parent_window) == FALSE)
		TDialogBasedConsole::SetParentWindow(::GetParent(m_parent_window));

	TDialogBasedConsole::OpenConsole();
	TDialogBasedConsole::SetParentWindow(saved_parent);
}

void TLangViewConsole::SetMajorStatus(const wchar_t *msg)
{
	CheckOpenStatusPanel();
	TDialogBasedConsole::SetMajorStatus(msg);
}

void TLangViewConsole::SetMinorStatus(const wchar_t *msg)
{
	CheckOpenStatusPanel();
	TDialogBasedConsole::SetMinorStatus(msg);
}

void TLangViewConsole::SetMinorStatusAndLastUpdateTime(const wchar_t *msg)
{
	SetMinorStatus(msg);
	m_last_update = CurrDateTime();
}

bool TLangViewConsole::UpdateStatusPanel(int num_processed, int num_total, const wchar_t *msg_prefix)
{
	CheckOpenStatusPanel();
	if (TDialogBasedConsole::IsOpened() == TRUE)
	{
		// Check if it is high time to update details.
		TDateTime curr_time = CurrDateTime();
		if ((curr_time - m_last_update) > 150*MILLI_SEC)
		{
			// More than 150 ms has elapsed.
			wchar_t buffer[80];
			swprintf(buffer, 80, L"%s%d out of %d processed.", ((msg_prefix != NULL) ? msg_prefix : L""), num_processed, num_total);
			SetMinorStatus(buffer);
			m_last_update = curr_time;
		}

		// Update the panel and check, if user has pressed the cancel button or not.
		TBasicDialog::ProcessPendingMessages();
		if (m_abort_signalled == TRUE)
			return(FALSE);
	}

	return(TRUE);
}

// ---------------------------------------------------------------------------
//   ==================  TLangViewAboutBox ========================
// ---------------------------------------------------------------------------

TLangViewAboutBox::TLangViewAboutBox(const wchar_t *app_name, const wchar_t *app_version, const wchar_t *version_purpose)
		: TBasicDialog(NULL, IDD_LANGV_ABOUT_BOX)
{
	m_app_name = (wchar_t*)app_name;
	m_app_version = (wchar_t*)app_version;
	m_version_purpose = (wchar_t*)version_purpose;
}

LRESULT TLangViewAboutBox::OnInitDialog(HWND hDefFocusControl)
{
	CenterOverTheParent(0, -4);
	SetControlFont(IDC_ABOUTBOX_APP_NAME, VERDANA, 9, TRUE);
	SetControlFont(IDC_ABOUTBOX_APP_VERSION, SMALL_FONTS, 7, FALSE);
	SetControlFont(IDC_ABOUTBOX_COMPANY_NAME, VERDANA, 9, TRUE);
	SetControlFont(IDC_ABOUTBOX_VERSION_PURPOSE, SMALL_FONTS, 7, TRUE);

	SetLabelColor(IDC_ABOUTBOX_APP_NAME);
	SetLabelColor(IDC_ABOUTBOX_APP_VERSION);
	SetLabelColor(IDC_ABOUTBOX_COMPANY_NAME);
	SetLabelColor(IDC_ABOUTBOX_COPYRIGHT);
	SetControlColor(IDC_ABOUTBOX_VERSION_PURPOSE, RGB(120, 0, 0));

	SetControlText(IDC_ABOUTBOX_APP_NAME, m_app_name);

#ifdef _DEBUG
	SetControlText(IDC_ABOUTBOX_APP_VERSION, m_app_version);
#else
	SetControlTextFmt(IDC_ABOUTBOX_APP_VERSION, L"%s  (Release)", m_app_version);
#endif

	SetControlText(IDC_ABOUTBOX_VERSION_PURPOSE, m_version_purpose);
	ShowControl(IDC_ABOUTBOX_VERSION_PURPOSE, FALSE);

#ifdef _DEBUG
	SetControlFont(IDC_ABOUTBOX_BUILD_TYPE, ARIAL, 8, TRUE);
	SetControlColor(IDC_ABOUTBOX_BUILD_TYPE, RGB(104, 0, 0));
	SetControlText(IDC_ABOUTBOX_BUILD_TYPE, L"DEBUG");
#else
	SetControlText(IDC_ABOUTBOX_BUILD_TYPE, NULL);
#endif

	return(FALSE);
}


