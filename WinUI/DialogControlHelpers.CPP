//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <shlobj.h>
#include  <assert.h>

#include  "Common/FormatNumber.H"
#include  "Common/FileDataHelper.H"
#include  "WinUi/DialogControlHelpers.H"
#include  "WinUi/BasicDialog.H"

//-------------------------------------------------------------------------
//  ===================  TControlHelper  ========================
//-------------------------------------------------------------------------

void TControlHelper::PaintInsetFrame(HDC hDC, RECT &rect)
{
	// Pick up the colors from the current dialog colors scheme.
	HPEN hPenShadow = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));
	HPEN hPenDarkShadow = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DDKSHADOW));
	HPEN hPenHighlight = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DHIGHLIGHT));
	HPEN hPenDialogFace = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DFACE));

	// Paint the upper and the left outer lines.
	POINT oldPenPosition;
	HPEN  hPenOld = (HPEN)::SelectObject(hDC, hPenShadow);
	::MoveToEx(hDC, rect.right-2, rect.top+0, &oldPenPosition);
	::LineTo(hDC, rect.left+0, rect.top+0);
	::LineTo(hDC, rect.left+0, rect.bottom-1);

	// Paint the upper and the left inner lines.
	::SelectObject(hDC, hPenDarkShadow);
	::MoveToEx(hDC, rect.right-3, rect.top+1, NULL);
	::LineTo(hDC, rect.left+1, rect.top+1);
	::LineTo(hDC, rect.left+1, rect.bottom-2);

	// Paint the right and the bottom outer lines.
	::SelectObject(hDC, hPenHighlight);
	::MoveToEx(hDC, rect.right-1, rect.top+0, NULL);
	::LineTo(hDC, rect.right-1, rect.bottom-1);
	::LineTo(hDC, rect.left-1, rect.bottom-1);

	// Paint the right and the bottom inner lines.
	::SelectObject(hDC, hPenDialogFace);
	::MoveToEx(hDC, rect.right-2,  rect.top+1, NULL);
	::LineTo(hDC, rect.right-2, rect.bottom-2);
	::LineTo(hDC, rect.left+0, rect.bottom-2);

	// Restore all affected DC props and release all temp objects.
	::MoveToEx(hDC, oldPenPosition.x, oldPenPosition.y, NULL);
	::SelectObject(hDC, hPenOld);
	::DeleteObject(hPenShadow);
	::DeleteObject(hPenDarkShadow);
	::DeleteObject(hPenHighlight);
	::DeleteObject(hPenDialogFace);
}

void TControlHelper::CenterOverTheWindow(HWND hWndToCenter, HWND hWndToCenterOver, long offs_x, long offs_y)
{
	RECT rcWindow;
	if (hWndToCenterOver != NULL && ::IsIconic(hWndToCenterOver) == FALSE && (::GetWindowLong(hWndToCenterOver, GWL_STYLE) & WS_VISIBLE) != 0)
	{
		// The anchor window is existing and it is opened.
		::GetWindowRect(hWndToCenterOver, &rcWindow);
	}
	else
	{
		// Use the rect of the primary monitor.
		rcWindow.left = rcWindow.top = 0;
		rcWindow.right = ::GetSystemMetrics(SM_CXSCREEN);
		rcWindow.bottom = ::GetSystemMetrics(SM_CYSCREEN);
	}

	// Apply the requested offset if any.
	rcWindow.left += offs_x;
	rcWindow.right += offs_x;
	rcWindow.top += offs_y;
	rcWindow.bottom += offs_y;

	// Call the worker method.
	CenterOverTheScreenRect(hWndToCenter, rcWindow);
}

void TControlHelper::CenterOverTheScreenRect(HWND hWndToCenter, RECT &rcScreenRect)
{
	RECT rcMonitorRect;

	HMONITOR hMon = ::MonitorFromRect(&rcScreenRect, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	if (::GetMonitorInfo(hMon, &mi) != 0)
	{
		// Use the rect of the monitor. Unfortunately is is not clear how to get the current scroll
		// pos of the work area if the work area is smaller than the rect of the whole monitor.
		rcMonitorRect = mi.rcMonitor;
	}
	else
	{
		// Use rect of the primary monitor. This procedure is ok even if the primary monitor is not
		// the leftmost monitor.
		rcMonitorRect.left = rcMonitorRect.top = 0;
		rcMonitorRect.right = ::GetSystemMetrics(SM_CXSCREEN);
		rcMonitorRect.bottom = ::GetSystemMetrics(SM_CYSCREEN);
	}

	// Pick up the current size of the dialog.
	RECT rcDialog;
	::GetWindowRect(hWndToCenter, &rcDialog);
	int dlg_size_x = rcDialog.right-rcDialog.left;
	int dlg_size_y = rcDialog.bottom-rcDialog.top;

	#define  MIN_SIDE_OFFS  4

	// Figure out the new pos of the doalg.
	int pos_x = (rcScreenRect.left + rcScreenRect.right)/2 - dlg_size_x/2;
	int pos_y = (rcScreenRect.top + rcScreenRect.bottom)/2 - dlg_size_y/2;

	if (pos_x+dlg_size_x > rcMonitorRect.right)
		pos_x = rcMonitorRect.right-dlg_size_x-MIN_SIDE_OFFS;
	if (pos_x < rcMonitorRect.left)
		pos_x = rcMonitorRect.left+MIN_SIDE_OFFS;

	if (pos_y+dlg_size_y > rcMonitorRect.bottom)
		pos_y = rcMonitorRect.bottom-dlg_size_y-MIN_SIDE_OFFS;
	if (pos_y < rcMonitorRect.top)
		pos_y = rcMonitorRect.top+MIN_SIDE_OFFS;

	// Apply the resulting position.
	::SetWindowPos(hWndToCenter, NULL, pos_x, pos_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

HMENU TControlHelper::CreatePopupMenuHandle(const TMenuItemInfo *info)
{
	HMENU hMenu = ::CreatePopupMenu();
	if (hMenu == NULL)
		return(NULL);

	while (info->item_text != NULL || (info->item_flags & MF_SEPARATOR) != 0)
	{
		UINT flags = info->item_flags | MF_BYPOSITION | MF_STRING;
		if (::InsertMenuW(hMenu, (DWORD)-1, flags, info->item_id, info->item_text) == FALSE)
		{
			::DestroyMenu(hMenu);
			return(NULL);
		}

		info++;
	}

	return(hMenu);
}

//------------------------------------------------------------------------

TColor TControlHelper::m_color_select_result_color;
TColor TControlHelper::m_color_select_custom_colors[16];

bool TControlHelper::ShowColorSelectDialog(HWND hParent, const wchar_t *window_title)
{
	CHOOSECOLOR info;
	memset(&info, 0, sizeof(CHOOSECOLOR));
	info.lStructSize = sizeof(CHOOSECOLOR);
	info.hwndOwner = hParent;
	info.rgbResult = m_color_select_result_color;
	info.lpCustColors = m_color_select_custom_colors;
	info.Flags = CC_ANYCOLOR | CC_RGBINIT | CC_FULLOPEN | CC_ENABLEHOOK;
	info.lCustData = (LPARAM)window_title;
	info.lpfnHook = ColorSelectDialogBoxHook;

	BOOL res = ::ChooseColor(&info);
	m_color_select_result_color = info.rgbResult;
	return((res != FALSE) ? TRUE : FALSE);
}

void TControlHelper::ShowColorSelectDialogEx(TBasicDialog *dialog, const wchar_t *window_title)
{
	if (ShowColorSelectDialog(dialog->GetWnd(), window_title) == TRUE)
	{
		// User has pressed Ok on the Windows color select dialog.
		PlaceColorToClipboardAsRgbString(dialog, TControlHelper::m_color_select_result_color);
	}
}

UINT_PTR CALLBACK TControlHelper::ColorSelectDialogBoxHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			CHOOSECOLOR *pinfo = (CHOOSECOLOR*)lParam;
			::SetWindowTextW(hWnd, (wchar_t*)pinfo->lCustData);
			::ShowWindow(::GetDlgItem(hWnd, 0x2CF), SW_HIDE);
			CenterOverTheWindow(hWnd, pinfo->hwndOwner);
			return(TRUE);
		}
	}

	return(FALSE);
}

void TControlHelper::PlaceColorToClipboardAsRgbString(TBasicDialog *dialog, TColor color_value)
{
	wchar_t buffer[80];
	swprintf(buffer, 80, L"%d, %d, %d", color_value & 0xFF, (color_value >> 8) & 0xFF, (color_value >> 16) & 0xFF);

	DWORD err = TControlHelper::PutStringToClipboard(buffer);
	if (err != ERROR_SUCCESS)
		dialog->MsgBoxFmt(MB_OK, L"Error putting the color data to the clipboard (Win32 error: %d).", err);
}

void	TControlHelper::PlaceMessageBufferToClipboard(TBasicDialog *dialog, bool gen_res, TMessageBuffer &buffer)
{
	if (gen_res == FALSE)
	{
		dialog->MsgBox(L"Error generating the report message.");
	}
	else if (buffer.GetXpndError() == TRUE)
	{
		dialog->MsgBox(L"Error buffering the report message.");
	}
	else
	{
		DWORD err = TControlHelper::PutStringToClipboard(buffer);
		if (err != ERROR_SUCCESS)
			dialog->MsgBoxFmt(MB_OK, L"Error putting message data to the clipboard (Win32 error: %d).", err);
	}
}

void	TControlHelper::PlaceDestinationFileToClipboard(TBasicDialog *dialog, bool gen_res, TDestinationFile &buffer, bool show_success_message)
{
	if (gen_res == FALSE)
	{
		dialog->MsgBox(L"Error generating the report data.");
	}
	else if (buffer.GetBodyPtr() == NULL)
	{
		dialog->MsgBoxFmt(MB_OK, L"The passed data is not an in memory buffer (%s).", buffer.FileName());
	}
	else if (buffer.GetErrorFlag() == TRUE)
	{
		dialog->MsgBox(L"Error buffering the report data.");
	}
	else
	{
		DWORD err = TControlHelper::PutStringToClipboard(buffer.GetBodyPtr());
		if (err != ERROR_SUCCESS)
			dialog->MsgBoxFmt(MB_OK, L"Error putting message data to the clipboard (Win32 error: %d).", err);
		else if (show_success_message == TRUE)
			dialog->MsgBoxFmt(MB_OK, L"Message (%ld byte%s) was placed into the clipboard.", buffer.GetCurrLen(), PluralSuffix(buffer.GetCurrLen()));
	}
}

//------------------------------------------------------------------------

DWORD TControlHelper::PutStringToClipboard(const wchar_t *message)
{
	if (message == NULL)
		return(ERROR_INVALID_DATA);

	// Open the clipboard.
	if (::OpenClipboard(NULL) == FALSE)
		return(::GetLastError());

	// Clear it.
	::EmptyClipboard();

	// Allocate memory block for the data.
	int memory_size = (int)(wcslen(message)+1)*sizeof(wchar_t);
	HANDLE hMemBlock = ::GlobalAlloc(GMEM_MOVEABLE, memory_size);
	if (hMemBlock == NULL)
	{
		::CloseClipboard();
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

	// Lock the memory handle and copy the passed message into the buffer.
	wchar_t *ptr_buffer = (wchar_t*)::GlobalLock(hMemBlock);
	wcscpy(ptr_buffer, message);
	::GlobalUnlock(hMemBlock);

	// Pass the handle to the clipboard. This function is not working proprely when stepping over it in the debugger.
	// In this case it returns 1418 - ERROR_CLIPBOARD_NOT_OPEN. Without the debugger it seems to work fine.
	HANDLE hMemBlockCopy = ::SetClipboardData(CF_UNICODETEXT, hMemBlock);
	if (hMemBlockCopy == NULL)
	{
		DWORD err = ::GetLastError();
		::GlobalFree(hMemBlock);
		::CloseClipboard();
		return(err);
	}

	// Close the clipboard and return success.
	::CloseClipboard();
	return(ERROR_SUCCESS);
}

DWORD TControlHelper::PutBitmapToClipboard(HANDLE hBitmap)
{
	if (hBitmap == NULL)
		return(ERROR_INVALID_DATA);

	// Open the clipboard. Param NULL means that the opened clipboard will be assoiciated with the current task.
	if (::OpenClipboard(NULL) == FALSE)
		return(::GetLastError());

	// Clear it.
	::EmptyClipboard();

	// Pass bitmap to the clipboard. This function is not working proprely when stepping over it in the debugger.
	// In this case it returns 1418 - ERROR_CLIPBOARD_NOT_OPEN. Without the debugger it seems to work fine.
	HANDLE hBitmapCopy = ::SetClipboardData(CF_BITMAP, hBitmap);
	if (hBitmapCopy == NULL)
	{
		DWORD err = ::GetLastError();
		::DeleteObject(hBitmap);
		::CloseClipboard();
		return(err);
	}

	// Close the clipboard and return success.
	::CloseClipboard();
	return(ERROR_SUCCESS);
}

//------------------------------------------------------------------------
//  ==================  TFontsListHelper  ========================
//------------------------------------------------------------------------

TFontsListHelper::~TFontsListHelper()
{
	for (int iFont=0; iFont < m_data.NumItems(); ++iFont)
	{
		HFONT hFont = m_data[iFont].m_fontHandle;
		if (hFont != NULL)
			::DeleteObject(hFont);
	}
}

HFONT TFontsListHelper::Lookup(HWND hWnd, const wchar_t *fontFamilyName, int fontSize, bool fontBold, bool fontItalic)
{
	// Check the directory of fonts.
	for (int iFont=0; iFont < m_data.NumItems(); ++iFont)
	{
		TFontInfo &fi = m_data[iFont];
		if (wcscmp(fi.m_fontName, fontFamilyName) == 0 && fi.m_fontSize == fontSize && fi.m_fontBold == fontBold && fi.m_fontItalic == fontItalic)
		{
			// This font is already known.
			return(fi.m_fontHandle);
		}
	}

	// Create the new font.
	HDC hDC = ::GetDC(hWnd);
	int logHeight = ::MulDiv(fontSize, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
	::ReleaseDC(hWnd, hDC);
	HFONT hFont = ::CreateFontW(
						-logHeight,									// Height.
						0,											// Width.
						0,											// Escapament.
						0,											// Orientation.
						(fontBold == TRUE) ? FW_BOLD : FW_NORMAL,		// Weight.
						fontItalic,									// Italic.
						FALSE,										// Underline.
						FALSE,										// StrikeOut.
						DEFAULT_CHARSET,
						OUT_DEFAULT_PRECIS,
						CLIP_DEFAULT_PRECIS,
						DEFAULT_QUALITY,
						FF_DONTCARE | DEFAULT_PITCH,
						fontFamilyName);
	if (hFont == NULL)
		return(NULL);

	// Add info about this font to the list.
	TFontInfo fontInfo = { hFont, (wchar_t*)fontFamilyName, fontSize, fontBold, fontItalic };
	if (m_data.AppendItem(fontInfo) == FALSE)
	{
		::DeleteObject(hFont);
		return(NULL);
	}

	// Success.
	return(hFont);
}

//-----------------------------------------------------------------------
//  ================  TControlColorHelper  ======================
//-----------------------------------------------------------------------

TControlColorHelper::~TControlColorHelper()
{
	// Release all custom bkgr brushes if any.
	for (int iCtrl=0; iCtrl < m_data.NumItems(); ++iCtrl)
	{
		if (m_data[iCtrl].m_custom_bkgr_brush == TRUE)
			::DeleteObject(m_data[iCtrl].m_bkgr_brush);
	}
}

TControlColorHelper::TControlColorInfo *TControlColorHelper::FindControlInfo(short ctrlId)
{
	int inx = GetIndex(ctrlId);
	return((inx < 0) ? NULL : m_data.ItemPtr(inx));
}

bool TControlColorHelper::AddControlInfo(TControlColorInfo &info)
{
	assert(info.m_ctrl_id != 0 && (info.m_frgr_color != cs_transparent || info.m_bkgr_brush != NULL));
	return(m_data.AppendItem(info));
}

bool TControlColorHelper::DeleteControlInfo(short ctrlId)
{
	int inx = GetIndex(ctrlId);
	if (inx < 0)
		return(FALSE);

	if (m_data[inx].m_custom_bkgr_brush == TRUE)
		::DeleteObject(m_data[inx].m_bkgr_brush);

	m_data.ReleaseItem(inx);
	return(TRUE);
}

int TControlColorHelper::GetIndex(short ctrlId)
{
	for (int iCtrl=0; iCtrl < m_data.NumItems(); ++iCtrl)
	{
		if (m_data[iCtrl].m_ctrl_id == ctrlId)
			return(iCtrl);
	}

	// The passed id is unknown.
	return(-1);
}

//--------------------------------------------------------------------------
//  ================  THueSatLumColorHelper  ======================
//--------------------------------------------------------------------------

void THueSatLumColorHelper::Setup(TColor color)
{
	// Save the passed color.
	original_color = color;

	if ((color & cs_color_type_mask) == 0)
	{
		// The RGB values of the color are known.
		double var_R = (double)((color) & 0xFF) / 255;
		double var_G = (double)((color >> 8) & 0xFF) / 255;
		double var_B = (double)((color >> 16) & 0xFF) / 255;

		double var_Min = var_R;
		if (var_G < var_Min)
			var_Min = var_G;
		if (var_B < var_Min)
			var_Min = var_B;

		double var_Max = var_R;
		if (var_G > var_Max)
			var_Max = var_G;
		if (var_B > var_Max)
			var_Max = var_B;

		double del_Max = var_Max - var_Min;

		lum_comp = (var_Max+var_Min)/2;

		if (del_Max <= 0.003)
		{
			// This is a plain grey scale color.
			hue_comp = 0;
			sat_comp = 0;
		}
		else
		{
			// Calculate the saturation.
			if (lum_comp <= 0.5)
			{
				sat_comp = del_Max / (var_Max+var_Min);
			}
			else
			{
				sat_comp = del_Max / (2.0 - (var_Max+var_Min));
			}

			if (var_R == var_Max)
			{
				// The main component is Red.
				hue_comp = 60 * (var_G-var_B)/del_Max;
				if (var_G < var_B)
					hue_comp += 360.0;
			}
			else if (var_G == var_Max)
			{
				// The main component is Green.
				hue_comp = 60 * (var_B-var_R)/del_Max + 120.0;
			}
			else
			{
				// The main component is Blue.
				assert(var_B == var_Max);
				hue_comp = 60 * (var_R-var_G)/del_Max + 240.0;
			}

			hue_comp /= 360.0;
		}
	}
}

TColor THueSatLumColorHelper::GetRgb()
{
	if ((original_color & cs_color_type_mask) == 0)
	{
		// Reconstruct the RGB from the HSL components.
		if (sat_comp <= 0.003)
		{
			// Grey scale color.
			BYTE R = (BYTE)(lum_comp * 255);
			BYTE G = (BYTE)(lum_comp * 255);
			BYTE B = (BYTE)(lum_comp * 255);
			original_color = RGB(R, G, B);
		}
		else
		{
			// The color is present.
			double var_2;
			if (lum_comp < 0.5)
			{
				var_2 = lum_comp * (1.0 + sat_comp);
			}
			else
			{
				var_2 = (lum_comp + sat_comp) - (lum_comp * sat_comp);
			}

			double var_1 = 2*lum_comp - var_2;

			BYTE R = (BYTE)(255 * HueToRgb(var_1, var_2, hue_comp+(1.0/3.0)));
			BYTE G = (BYTE)(255 * HueToRgb(var_1, var_2, hue_comp));
			BYTE B = (BYTE)(255 * HueToRgb(var_1, var_2, hue_comp-(1.0/3.0)));

			original_color = RGB(R, G, B);
		}
	}

	return(original_color);
}

void THueSatLumColorHelper::AdjustLuminosity(double middle_level, double compressionFactor)
{
	if (middle_level < 0)
		middle_level = 0;
	else if (middle_level > 1)
		middle_level = 1;

	if (compressionFactor < 0)
		compressionFactor = 0;
	else if (compressionFactor > 1)
		compressionFactor = 1;

	if (lum_comp > middle_level)
	{
		double diff = lum_comp-middle_level;
		lum_comp = middle_level + compressionFactor * diff;
	}
	else
	{
		double diff = middle_level-lum_comp;
		lum_comp = middle_level - compressionFactor * diff;
	}
}

double THueSatLumColorHelper::HueToRgb(double v1, double v2, double vH)
{
	if (vH < 0)
		vH += 1.0;
	if (vH > 1)
		vH -= 1.0;

	if (vH < 1.0/6.0)
		return(v1 + (v2-v1) * 6*vH);
	if (vH < 1.0/2.0)
		return(v2);
	if (vH < 2.0/3.0)
		return(v1 + (v2-v1) * 6*(2.0/3.0-vH));

	return(v1);
}

//----------------------------------------------------------------------------
//  ===================  TFileOpenHelper  ==========================
//----------------------------------------------------------------------------

int	TFileOpenHelper::g_latest_dialog_width = 0;
int	TFileOpenHelper::g_latest_dialog_height = 0;

TFileOpenHelper::TFileOpenHelper(HINSTANCE hInst, const wchar_t *files_filtering_spec, const wchar_t *default_extension, const wchar_t *open_title,  const wchar_t *save_title)
{
	memset(&m_ofn_data, 0, sizeof(OPENFILENAME));

	// Fill in the constant fields of the OPENFILENAME struct.
	m_ofn_data.lStructSize = sizeof(OPENFILENAME);
	m_ofn_data.hInstance = (hInst != NULL) ? hInst : ::GetModuleHandle(NULL);
	m_ofn_data.lpstrFilter = files_filtering_spec;
	m_ofn_data.nFilterIndex = 1;						// This is a one based index of the initial file filtering option.
	m_ofn_data.lpstrFile = m_file_name_buffer;
	m_ofn_data.nMaxFile = FILE_OPEN_MAX_PATH;
	m_ofn_data.lpstrInitialDir = m_initial_directory;
	m_ofn_data.Flags = OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_HIDEREADONLY;
	m_ofn_data.lpstrDefExt = default_extension;
	m_ofn_data.lCustData = (LPARAM)this;
	m_ofn_data.lpfnHook = FileOpenHelperDialogHook;
	m_ofn_data.FlagsEx = OFN_EX_NOPLACESBAR;

	// Fill in the data fields of the helper.
	m_initial_directory[0] = 0;
	m_target_file_name[0] = 0;
	m_file_name_buffer[0]  = 0;

	// Save the pointers.
	m_get_open_file_dialog_title = (wchar_t*)open_title;
	m_get_save_file_dialog_title = (wchar_t*)save_title;
}

void  TFileOpenHelper::SetInitialDirectory(const wchar_t *path, BOOL path_is_file_name)
{
	if (path == NULL || path[0] == 0)
	{
		// Reset the initial directory.
		m_initial_directory[0] = 0;
		return;
	}

	TFileNameBuffer buffer;
	if (path_is_file_name == TRUE)
	{
		if (TPathHelper::GetDirectoryName(buffer, path) == FALSE)
		{
			// A rare failure has happened.
			m_initial_directory[0] = 0;
			return;
		}

		path = buffer.DataPtr();
	}

	// Accept the path.
	wcsncpy(m_initial_directory, path, FILE_OPEN_MAX_PATH);
	m_initial_directory[FILE_OPEN_MAX_PATH-1] = 0;
}

bool  TFileOpenHelper::GetOpenFileName(HWND hParent)
{
	// Filll in the request specific fields.
	m_ofn_data.hwndOwner = hParent;
	m_ofn_data.lpstrTitle = m_get_open_file_dialog_title;
	m_ofn_data.Flags |= OFN_FILEMUSTEXIST;

	// Make the major call.
	wcscpy(m_file_name_buffer, m_target_file_name);
	m_file_open_request = TRUE;
	if (::GetOpenFileNameW(&m_ofn_data) == FALSE)
		return(FALSE);

	wcscpy(m_target_file_name, m_file_name_buffer);
	return(TRUE);
}

bool  TFileOpenHelper::GetSaveFileName(HWND hParent, bool want_file_overwrite_warning)
{
	// Filll in the request specific fields.
	m_ofn_data.hwndOwner = hParent;
	m_ofn_data.lpstrTitle = m_get_save_file_dialog_title;
	m_ofn_data.Flags &= ~OFN_FILEMUSTEXIST;

	// Make the major call.
	wcscpy(m_file_name_buffer, m_target_file_name);
	m_file_open_request = FALSE;
	m_file_overwrite_warn = want_file_overwrite_warning;
	if (::GetSaveFileNameW(&m_ofn_data) == FALSE)
		return(FALSE);

	wcscpy(m_target_file_name, m_file_name_buffer);
	return(TRUE);
}

void  TFileOpenHelper::SetCurrFileName(const wchar_t *path)
{
	if (path == NULL || path[0] == 0)
	{
		m_target_file_name[0] = 0;
		return;
	}

	// Accept the path.
	wcsncpy(m_target_file_name, path, FILE_OPEN_MAX_PATH);
	m_target_file_name[FILE_OPEN_MAX_PATH-1] = 0;
}

UINT_PTR CALLBACK TFileOpenHelper::FileOpenHelperDialogHook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Pick up the handle of the dialog.
	HWND hFileOpenDlg = ::GetParent(hDlg);

	// Just a few notifications are important here.
	if (msg == WM_DESTROY)
	{
		RECT rcDialog;
		if (hFileOpenDlg != NULL && ::GetWindowRect(hFileOpenDlg, &rcDialog) != FALSE)
		{
			// Store the final size of the dialog.
			g_latest_dialog_width = rcDialog.right-rcDialog.left;
			g_latest_dialog_height = rcDialog.bottom-rcDialog.top;
		}
	}
	else if (msg == WM_NOTIFY)
	{
		// This message carries the address of the structure in its lParam.
		NMHDR *nmhdr = (NMHDR*)lParam;

		if (nmhdr->code == CDN_INITDONE)
		{
			// Pick up the rect of the parent window if possible.
			HWND hFileOpenParent = ::GetParent(hFileOpenDlg);
			RECT rcParent;
			if (hFileOpenParent == NULL || ::IsIconic(hFileOpenParent) != 0 || ::GetWindowRect(hFileOpenParent, &rcParent) == FALSE)
			{
				// Use the screen rect instead of the window rect.
				rcParent.left = rcParent.top = 0;
				rcParent.right = ::GetSystemMetrics(SM_CXSCREEN);
				rcParent.bottom = ::GetSystemMetrics(SM_CYSCREEN);
			}

			if (hFileOpenDlg != NULL)
			{
				RECT rcDialog;
				if (g_latest_dialog_width == 0 && g_latest_dialog_height == 0 && ::GetWindowRect(hFileOpenDlg, &rcDialog) != FALSE)
				{
					// Store the initial size of the dialog.
					g_latest_dialog_width = rcDialog.right-rcDialog.left;
					g_latest_dialog_height = rcDialog.bottom-rcDialog.top;
				}

				// Center the current dialog over its parent with the minor shift up.
				::SetWindowPos(hFileOpenDlg, NULL,
							(rcParent.left+rcParent.right)/2 - g_latest_dialog_width/2,
							(rcParent.top+rcParent.bottom)/2 - g_latest_dialog_height/2 - 8,
							0, 0, SWP_NOSIZE | SWP_NOZORDER);
			}
		}
		else if (nmhdr->code == CDN_FILEOK)
		{
			OFNOTIFYW *ofnhdr = (OFNOTIFYW*)lParam;
			TFileOpenHelper *inst = (TFileOpenHelper*)(ofnhdr->lpOFN->lCustData);
			if (inst->m_file_open_request == FALSE && inst->m_file_overwrite_warn == TRUE)
			{
				// This is a file save request and an overwrite warning is requested.
				bool file_exists = TPathHelper::IsPathAFile(inst->m_file_name_buffer);
				if (file_exists == TRUE && wcscmp(inst->m_file_name_buffer, inst->m_target_file_name) != 0)
				{
					// The file is existing and it is not the file where the app wanted to store the data.
					wchar_t buffer[2*MAX_PATH+80];
					swprintf(buffer, 2*MAX_PATH+80, L"%s  is already existing.\nDo you want to replace it?", inst->m_file_name_buffer);
					if (::MessageBoxW(hDlg, buffer, ofnhdr->lpOFN->lpstrTitle, MB_OKCANCEL | MB_ICONWARNING) != IDOK)
					{
						::SetWindowLong(hDlg, DWL_MSGRESULT, 1);
						return(TRUE);
					}
				}
			}
		}
	}

	return(FALSE);
}


//-----------------------------------------------------------------------------------
//  ======================  TGetDirectoryNameHelper  ======================
//-----------------------------------------------------------------------------------

TGetDirectoryNameHelper::TGetDirectoryNameHelper(HINSTANCE hInst)
{
	m_opened_at_least_once = FALSE;
	m_target_dir_name[0] = 0;
}

bool TGetDirectoryNameHelper::GetDirectoryName(HWND hParent, const wchar_t *instructions_string)
{
	// Prepare the info struct with options of displaying the dialog.
	BROWSEINFO info = { 0 };
	info.hwndOwner = hParent;
	info.pszDisplayName = m_target_dir_name;
	info.lpszTitle = instructions_string;
	info.ulFlags = BIF_USENEWUI;
	info.lpfn = BrowseFolderCallback;
	info.lParam = (LPARAM)this;

	// Show the system dialog.
	m_curr_parent = hParent;
	PIDLIST_ABSOLUTE pidla = ::SHBrowseForFolder(&info);
	if (pidla == NULL)
		return(FALSE);

	// The system dialog returns only the short name of selected directory.
	if (::SHGetPathFromIDList(pidla, m_target_dir_name) == FALSE)
		return(FALSE);

	// Something was selected.
	assert(m_target_dir_name[0] != 0);
	return(TRUE);
}

int CALLBACK TGetDirectoryNameHelper::BrowseFolderCallback(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	TGetDirectoryNameHelper *inst = (TGetDirectoryNameHelper*)lpData;

	if (uMsg == BFFM_INITIALIZED)
	{
		::SetWindowTextW(hWnd, L" Select Folder ");
		::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)L"C:\\");

		// For some reason the system dialog is not preserving its position.
		if (inst->m_opened_at_least_once == FALSE)
		{
			// Center the dialog over the parent window. For now the dialog is centered using a wrong size.
			// Unfortunately Windows changes the size of the dialog later and is not providing appropriate
			// hook to customize this behaviour.
			TControlHelper::CenterOverTheWindow(hWnd, inst->m_curr_parent);
		}
		else
		{
			// Restore the old position.
			::SetWindowPos(hWnd, NULL, inst->m_latest_dialog_pos.x, inst->m_latest_dialog_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}

		// Setup the subclassing.
		::SetWindowSubclass(hWnd, BrowseFolderSubclassProc, 0, (DWORD_PTR)inst);
	}

	return(0);
}

LRESULT CALLBACK TGetDirectoryNameHelper::BrowseFolderSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	TGetDirectoryNameHelper *inst = (TGetDirectoryNameHelper*)dwRefData;

	switch (uMsg)
	{
		case WM_DESTROY:
				{
					RECT rcDialog;
					if (::GetWindowRect(hWnd, &rcDialog) != FALSE)
					{
						// Store the final position of the dialog.
						inst->m_opened_at_least_once = TRUE;
						inst->m_latest_dialog_pos.x = rcDialog.left;
						inst->m_latest_dialog_pos.y = rcDialog.top;
					}
				}
				break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

//---------------------------------------------------------------------------
//  ======================  Custom controls  ======================
//---------------------------------------------------------------------------

//
// This class should not be used in the app level code. This is an internal structure of the control.
//
class TSplitterControlState
{
protected:

	TSplitterControlState(HWND hCtrl, const wchar_t *window_class_name);
	~TSplitterControlState();

	void		PaintSplitter(RECT &rcInvalid, HDC hDC);
	void		PaintShiftedSplitter(short mouse_x, short mouse_y);
	void		HideShiftedSplitter();
	void		GetCurrentRect(RECT &currRect);

	static LRESULT CALLBACK SplitterWndProc(HWND, UINT, WPARAM, LPARAM);

protected:

	HWND		m_hCtrl;

	bool			m_vertSplitter;
	DWORD		m_faceColor;

	int			m_leftMargin;
	int			m_rightMargin;

	RECT		m_currShiftedRect;			// Client coordinates of the parent window.
	POINT		m_mouseDownPoint;			// Client coordinates.
	bool			m_mousePressed;

	friend void DialogControlsRegisterWindowClasses(HINSTANCE);
};

//
// This class should not be used in the app level code. This is an internal structure of the control.
//
class TBitmapControlState
{
protected:

	TBitmapControlState() { m_hBmp = NULL; m_icon = 0; m_bmp_props.Clear(); }
	~TBitmapControlState() { if (m_hBmp != NULL) ::DeleteObject(m_hBmp); }

	void		PaintBitmap(HWND hWnd, HDC hDC);

	static LRESULT CALLBACK	BitmapWndProc(HWND, UINT, WPARAM, LPARAM);

protected:

	HBITMAP						m_hBmp;
	short						m_icon;
	TIconsGridStyleProps			m_bmp_props;
	TBitmapPaintHelper			m_helper;

	friend void DialogControlsRegisterWindowClasses(HINSTANCE);
};

LRESULT  CALLBACK  DlgCheckBoxControlWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT  CALLBACK  DlgBrushedStaticControlWndProc(HWND, UINT, WPARAM, LPARAM);

//-----------------------------------------------------------------
//  ================  Static function  ======================
//-----------------------------------------------------------------

void DialogControlsRegisterWindowClasses(HINSTANCE hInst)
{
	// Fill in the common props.
	WNDCLASSW wcInfo;
	wcInfo.style = 0;
	wcInfo.cbClsExtra = 0;
	wcInfo.hInstance = (hInst != NULL) ? hInst : ::GetModuleHandle(NULL);
	wcInfo.hIcon = NULL;
	wcInfo.hbrBackground = NULL;
	wcInfo.lpszMenuName = NULL;

	// Horizontal Splitter.
	wcInfo.lpfnWndProc = TSplitterControlState::SplitterWndProc;
	wcInfo.cbWndExtra = sizeof(INT_PTR);
	wcInfo.hCursor = ::LoadCursor(NULL, IDC_SIZENS);
	wcInfo.lpszClassName = DLG_CTRL_HORZ_SPLITTER;
	::RegisterClassW(&wcInfo);

	// Vertical Splitter.
	wcInfo.lpfnWndProc = TSplitterControlState::SplitterWndProc;
	wcInfo.cbWndExtra = sizeof(INT_PTR);
	wcInfo.hCursor = ::LoadCursor(NULL, IDC_SIZEWE);
	wcInfo.lpszClassName = DLG_CTRL_VERT_SPLITTER;
	::RegisterClassW(&wcInfo);

	// CheckBox Control.
	wcInfo.lpfnWndProc = DlgCheckBoxControlWndProc;
	wcInfo.cbWndExtra = 4*sizeof(INT_PTR);
	wcInfo.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcInfo.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
	wcInfo.lpszClassName = DLG_CTRL_CHKBOX_CONTROL;
	::RegisterClassW(&wcInfo);
	wcInfo.hbrBackground = NULL;

	// Bitmap Control.
	wcInfo.lpfnWndProc = TBitmapControlState::BitmapWndProc;
	wcInfo.cbWndExtra = sizeof(INT_PTR);
	wcInfo.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcInfo.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
	wcInfo.lpszClassName = DLG_CTRL_BITMAP_CONTROL;
	::RegisterClassW(&wcInfo);
	wcInfo.hbrBackground = NULL;

	// Brushed static control.
	wcInfo.lpfnWndProc = DlgBrushedStaticControlWndProc;
	wcInfo.cbWndExtra = 4*sizeof(INT_PTR);
	wcInfo.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcInfo.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
	wcInfo.lpszClassName = DLG_CTRL_BRUSHED_STATIC;
	::RegisterClassW(&wcInfo);
	wcInfo.hbrBackground = NULL;
}

//--------------------------------------------------------------------
//  ================  TSplitterControlState  ====================
//--------------------------------------------------------------------

TSplitterControlState::TSplitterControlState(HWND hCtrl, const wchar_t *window_class_name)
{
	m_hCtrl = hCtrl;
	m_vertSplitter = (wcscmp(window_class_name, DLG_CTRL_HORZ_SPLITTER) != 0) ? TRUE : FALSE;
	m_faceColor = RGB(80, 80, 80);

	m_leftMargin = 0;
	m_rightMargin = 0;
	m_mousePressed = FALSE;
}

TSplitterControlState::~TSplitterControlState()
{
	HideShiftedSplitter();
}

void TSplitterControlState::PaintSplitter(RECT &rcInvalid, HDC hDC)
{
	if (m_faceColor == cs_transparent)
		return;

	HBRUSH hBrush = ::CreateSolidBrush(m_faceColor);
	::FillRect(hDC, &rcInvalid, hBrush);
	::DeleteObject(hBrush);
}

void TSplitterControlState::PaintShiftedSplitter(short mouse_x, short mouse_y)
{
	// Prepare and save the rect to fill.
	RECT currentParentRect;
	::GetClientRect(::GetParent(m_hCtrl), &currentParentRect);
	GetCurrentRect(m_currShiftedRect);

	if (m_vertSplitter == TRUE)
	{
		int delta_x = mouse_x - m_mouseDownPoint.x;

		// Apply the limits.
		if (m_currShiftedRect.left + delta_x < m_leftMargin)
			delta_x = m_leftMargin - m_currShiftedRect.left;
		if (m_currShiftedRect.right + delta_x > currentParentRect.right-m_rightMargin)
			delta_x = currentParentRect.right-m_rightMargin - m_currShiftedRect.right;

		m_currShiftedRect.left += delta_x;
		m_currShiftedRect.right += delta_x;
	}
	else
	{
		int delta_y = mouse_y - m_mouseDownPoint.y;

		// Apply the limits.
		if (m_currShiftedRect.top + delta_y < m_leftMargin)
			delta_y = m_leftMargin - m_currShiftedRect.top;
		if (m_currShiftedRect.bottom + delta_y > currentParentRect.bottom-m_rightMargin)
			delta_y = currentParentRect.bottom-m_rightMargin - m_currShiftedRect.bottom;

		m_currShiftedRect.top += delta_y;
		m_currShiftedRect.bottom += delta_y;
	}

	// Do the painting itself.
	HDC hDC = ::GetDC(::GetParent(m_hCtrl));
	HBRUSH hBrush = ::CreateHatchBrush(HS_DIAGCROSS, RGB(8, 8, 8));
	::FillRect(hDC, &m_currShiftedRect, hBrush);
	::DeleteObject(hBrush);
	::ReleaseDC(::GetParent(m_hCtrl), hDC);
}

void TSplitterControlState::HideShiftedSplitter()
{
	if (m_mousePressed == FALSE)
		return;

	// Invalidate the area where the shifted splitter was recently painted.
	HWND hParent = ::GetParent(m_hCtrl);
	::InvalidateRect(hParent, &m_currShiftedRect, TRUE);
	::UpdateWindow(hParent);
}

void TSplitterControlState::GetCurrentRect(RECT &currRect)
{
	HWND hParent = ::GetParent(m_hCtrl);
	::GetWindowRect(m_hCtrl, &currRect);
	::ScreenToClient(hParent, (POINT*)(&currRect.left));
	::ScreenToClient(hParent, (POINT*)(&currRect.right));
}

LRESULT CALLBACK TSplitterControlState::SplitterWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TSplitterControlState *pState = (TSplitterControlState*)(LONG_PTR)::GetWindowLongPtr(hWnd, 0);

	switch (msg)
	{
		case WM_NCCREATE:
				{
					// This struct is not properly passed into the WM_CREATE handler.
					CREATESTRUCTW *pCreateStruct = (CREATESTRUCTW*)lParam;

					// Create the state object.
					pState = new TSplitterControlState(hWnd, pCreateStruct->lpszClass);
					if (pState == NULL)
					{
						// Creation of the state object failed. Fail the creation of the whole control.
						return(FALSE);
					}

					::SetWindowLongPtr(hWnd, 0, (LONG_PTR)pState);
				}
				return(TRUE);	// WM_NCCREATE message requires to return this value to continue creation of the control.

		case WM_DESTROY:
				{
					// Get rid of the state object.
					delete pState;
					::SetWindowLongPtr(hWnd, 0, (LONG_PTR)0);
				}
				return(0);

		case WM_LBUTTONDOWN:
				{
					::SetCapture(hWnd);
					pState->HideShiftedSplitter();
					pState->m_mouseDownPoint.x = LOWORD(lParam);
					pState->m_mouseDownPoint.y = HIWORD(lParam);
					pState->PaintShiftedSplitter(LOWORD(lParam), HIWORD(lParam));
					pState->m_mousePressed = TRUE;
				}
				break;

		case WM_LBUTTONUP:
				{
					pState->HideShiftedSplitter();
					if (pState->m_mousePressed == TRUE)
					{
						// Prepare the difference between the latest dragged image and the current control position.
						int shift;
						RECT controlRect;
						pState->GetCurrentRect(controlRect);
						if (pState->m_vertSplitter == TRUE)
						{
							shift = pState->m_currShiftedRect.left-controlRect.left;
						}
						else
						{
							shift = pState->m_currShiftedRect.top-controlRect.top;
						}

						// Check the difference.
						if (shift != 0)
						{
							// Send notification to the parent window.
							::SendMessage(::GetParent(hWnd), WM_COMMAND, MAKEWPARAM((WORD)::GetWindowLong(hWnd, GWL_ID), (WORD)shift), (LPARAM)hWnd);
						}
					}

					::ReleaseCapture();
					pState->m_mousePressed = FALSE;
				}
				break;

		case WM_MOUSEMOVE:
				{
					if ((wParam & MK_LBUTTON) == 0)
						break;

					// Draw or update the shifted splitter.
					if (pState->m_mousePressed == TRUE)
					{
						pState->HideShiftedSplitter();
						pState->PaintShiftedSplitter(LOWORD(lParam), HIWORD(lParam));
					}
				}
				break;

		case WM_CAPTURECHANGED:
				{
					pState->HideShiftedSplitter();
					pState->m_mousePressed = FALSE;
				}
				return(0);

		case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC hDC = ::BeginPaint(hWnd, &ps);
					pState->PaintSplitter(ps.rcPaint, hDC);
					::EndPaint(hWnd, &ps);
				}
				return(0);

		case DLG_CTRL_SET_SPLITTER_COLOR:
				{
					// Accept the new color.
					pState->m_faceColor = (DWORD)wParam;
					::InvalidateRect(hWnd, NULL, TRUE);
				}
				break;

		case DLG_CTRL_SET_SPLITTER_LIMITS:
				{
					// Accept the new shifting limits.
					pState->m_leftMargin = (int)wParam;
					pState->m_rightMargin = (int)lParam;
				}
				break;
	}

	return(::DefWindowProc(hWnd, msg, wParam, lParam));
}

//----------------------------------------------------------------
//  ===============  Custom CheckBox  ===================
//----------------------------------------------------------------

// Offsets inside the window memory of the CHECKBOX control.
#define	VCHK_GWL_CHECKED					 0
#define	VCHK_GWL_CHK_STATE				 4
#define	VCHK_GWL_NORMAL_BKGR_COLOR		 8
#define	VCHK_GWL_DISABLED_BKGR_COLOR	12

// Window function for the checkbox control.
LRESULT CALLBACK DlgCheckBoxControlWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_NCCREATE:
				{
					// Fill in the default values into window LONGs.
					::SetWindowLong(hWnd, VCHK_GWL_CHECKED, 0);
					::SetWindowLong(hWnd, VCHK_GWL_CHK_STATE, 0);
					::SetWindowLong(hWnd, VCHK_GWL_NORMAL_BKGR_COLOR, ::GetSysColor(COLOR_WINDOW));
					::SetWindowLong(hWnd, VCHK_GWL_DISABLED_BKGR_COLOR, ::GetSysColor(COLOR_3DFACE));
				}
				return(TRUE);	// WM_NCCREATE message requires to return this value to continue creation of the control.

		case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC hPaintDC = ::BeginPaint(hWnd, &ps);
					RECT rc;
					::GetClientRect(hWnd, &rc);

					// Paint the frame around the checkbox.
					TControlHelper::PaintInsetFrame(hPaintDC, rc);

					// Apply the frame offsets.
					rc.left += 2;
					rc.top += 2;
					rc.right -= 2;
					rc.bottom -= 2;

					// Paint the background.
					LONG win_stt = ::GetWindowLong(hWnd, GWL_STYLE);
					LONG app_stt = ::GetWindowLong(hWnd, VCHK_GWL_CHK_STATE);
					if ((win_stt & WS_DISABLED) != 0 || (app_stt & 1) != 0)
					{
						// Use the disabled color from the app scheme.
						HBRUSH hBrush = ::CreateSolidBrush(::GetWindowLong(hWnd, VCHK_GWL_DISABLED_BKGR_COLOR));
						::FillRect(hPaintDC, &rc, hBrush);
						::DeleteObject(hBrush);
					}
					else
					{
						// Use the current application defined color.
						HBRUSH hBrush = ::CreateSolidBrush(::GetWindowLong(hWnd, VCHK_GWL_NORMAL_BKGR_COLOR));
						::FillRect(hPaintDC, &rc, hBrush);
						::DeleteObject(hBrush);
					}

					// The size of the icon is 7x7 pixels.
					int xb = (rc.left+rc.right)/2-3;
					int yb = (rc.top+rc.bottom)/2-3;
					if ((::GetWindowLong(hWnd, VCHK_GWL_CHECKED) & BST_CHECKED) != 0)
					{
						// Paint the checkmark.
						LONG color = ((win_stt & WS_DISABLED) != 0) ? ::GetSysColor(COLOR_3DSHADOW) : RGB(0, 0, 0);
						::SetPixel(hPaintDC, xb+0, yb+2, color);
						::SetPixel(hPaintDC, xb+0, yb+3, color);
						::SetPixel(hPaintDC, xb+0, yb+4, color);
						::SetPixel(hPaintDC, xb+1, yb+3, color);
						::SetPixel(hPaintDC, xb+1, yb+4, color);
						::SetPixel(hPaintDC, xb+1, yb+5, color);
						::SetPixel(hPaintDC, xb+2, yb+4, color);
						::SetPixel(hPaintDC, xb+2, yb+5, color);
						::SetPixel(hPaintDC, xb+2, yb+6, color);
						::SetPixel(hPaintDC, xb+3, yb+3, color);
						::SetPixel(hPaintDC, xb+3, yb+4, color);
						::SetPixel(hPaintDC, xb+3, yb+5, color);
						::SetPixel(hPaintDC, xb+4, yb+2, color);
						::SetPixel(hPaintDC, xb+4, yb+3, color);
						::SetPixel(hPaintDC, xb+4, yb+4, color);
						::SetPixel(hPaintDC, xb+5, yb+1, color);
						::SetPixel(hPaintDC, xb+5, yb+2, color);
						::SetPixel(hPaintDC, xb+5, yb+3, color);
						::SetPixel(hPaintDC, xb+6, yb+0, color);
						::SetPixel(hPaintDC, xb+6, yb+1, color);
						::SetPixel(hPaintDC, xb+6, yb+2, color);
					}

					::EndPaint(hWnd, &ps);
				}
				return(0);

		case WM_ENABLE:
				{
					// Request painting of the new state of the window.
					::InvalidateRect(hWnd, NULL, FALSE);
				}
				return(0);

		case BM_SETCHECK:
				{
					// Save new checked state and repaint the window.
					::SetWindowLong(hWnd, VCHK_GWL_CHECKED, ((LONG)wParam & BST_CHECKED));
					::InvalidateRect(hWnd, NULL, FALSE);
				}
				return(0);

		case BM_GETCHECK:
				{
					// Give out the current checked state.
					return(::GetWindowLong(hWnd, VCHK_GWL_CHECKED));
				}
				break;

		case DLG_CTRL_SET_CHKBOX_BKGR_COLOR:
				{
					// Save the new bkgr color and repaint the window.
					::SetWindowLong(hWnd, VCHK_GWL_NORMAL_BKGR_COLOR, (LONG)lParam);
					::InvalidateRect(hWnd, NULL, FALSE);
				}
				return(0);

		case DLG_CTRL_SET_CHKBOX_DISABLED_BKGR_COLOR:
				{
					// Save the new bkgr color and repaint the window.
					::SetWindowLong(hWnd, VCHK_GWL_DISABLED_BKGR_COLOR, (LONG)lParam);
					::InvalidateRect(hWnd, NULL, FALSE);
				}
				return(0);

		case WM_LBUTTONDOWN:
				{
					// Request posting the mouse leave message.
					TRACKMOUSEEVENT info = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, HOVER_DEFAULT };
					::TrackMouseEvent(&info);

					// Setup mouse down flag and repaint the window.
					::SetWindowLong(hWnd, VCHK_GWL_CHK_STATE, 1);
					::InvalidateRect(hWnd, NULL, FALSE);
				}
				return(0);

		case WM_LBUTTONUP:
				{
					// Toggle the current checked state.
					LONG chk_stt = ::GetWindowLong(hWnd, VCHK_GWL_CHECKED);
					::SetWindowLong(hWnd, VCHK_GWL_CHECKED, (chk_stt ^ BST_CHECKED));

					// Reset the mouse down flag and repaint.
					::SetWindowLong(hWnd, VCHK_GWL_CHK_STATE, 0);
					::InvalidateRect(hWnd, NULL, FALSE);

					// Send notification to the parent window.
					LONG ctrlId = ::GetWindowLong(hWnd, GWL_ID);
					::SendMessage(::GetParent(hWnd), WM_COMMAND, ctrlId, (LPARAM)hWnd);
				}
				return(0);

		case WM_MOUSELEAVE:
				{
					::SetWindowLong(hWnd, VCHK_GWL_CHK_STATE, 0);
					::InvalidateRect(hWnd, NULL, FALSE);
				}
				return(0);
	}

	return(::DefWindowProc(hWnd, msg, wParam, lParam));
}

//------------------------------------------------------------------
//  =================  Bitmap Control  =====================
//------------------------------------------------------------------

void TBitmapControlState::PaintBitmap(HWND hWnd, HDC hDC)
{
	if (m_hBmp == NULL || m_icon < 100)
		return;

	RECT rc;
	::GetClientRect(hWnd, &rc);

	int xOnBmp = m_bmp_props.x_base + (m_icon%100)*m_bmp_props.x_step;
	int yOnBmp = m_bmp_props.y_base + (m_icon/100-1)*m_bmp_props.y_step;

	m_helper.Paint(hDC, xOnBmp, yOnBmp, m_bmp_props.x_cell, m_bmp_props.y_cell,
					(rc.right-m_bmp_props.x_cell)/2, (rc.bottom-m_bmp_props.y_cell)/2);
}

LRESULT CALLBACK TBitmapControlState::BitmapWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TBitmapControlState *pState = (TBitmapControlState*)(LONG_PTR)::GetWindowLongPtr(hWnd, 0);

	switch (msg)
	{
		case WM_NCCREATE:
				{
					// This struct is not properly passed into the WM_CREATE handler.
					CREATESTRUCTW *pCreateStruct = (CREATESTRUCTW*)lParam;

					// Create the state object.
					pState = new TBitmapControlState();
					if (pState == NULL)
					{
						// Creation of the state object failed. Fail the control creation.
						return(FALSE);
					}

					// State object was created. Store its pointer.
					::SetWindowLongPtr(hWnd, 0, (LONG_PTR)pState);

					// Parse the title of the control.
					short icon_id = 100;
					if (swscanf(pCreateStruct->lpszName, L"%hd", &icon_id) == 1)
						pState->m_icon = icon_id;
				}
				return(TRUE);	// WM_NCCREATE message requires to return this value to continue creation of the control.

		case WM_DESTROY:
				{
					// Get rid of the state object.
					delete pState;
					::SetWindowLongPtr(hWnd, 0, (LONG)0);
				}
				return(0);

		case WM_NCHITTEST:
				{
					// This control should not accept the focus.
					return(HTTRANSPARENT);
				}

		case WM_PAINT:
				{
					// Paint the icon if any.
					PAINTSTRUCT ps;
					HDC hDC = ::BeginPaint(hWnd, &ps);
					pState->PaintBitmap(hWnd, hDC);
					::EndPaint(hWnd, &ps);
				}
				return(0);

		case DLG_CTRL_SET_BITMAP_PROPS:
				{
					if (pState->m_hBmp != NULL)
					{
						::DeleteObject(pState->m_hBmp);
						pState->m_helper.Release();
					}

					HINSTANCE hInst = (HINSTANCE)wParam;
					if (hInst == NULL)
						hInst = ::GetModuleHandle(NULL);

					// Accept the new bitmap resource id and props.
					pState->m_bmp_props = *((TIconsGridStyleProps*)lParam);
					pState->m_hBmp = ::LoadBitmap(hInst, MAKEINTRESOURCE(pState->m_bmp_props.resource_id));

					if (pState->m_hBmp != NULL)
						pState->m_helper.Initialize(hWnd, pState->m_hBmp, 1.0, pState->m_bmp_props.transp_pixels);

					::InvalidateRect(hWnd, NULL, TRUE);
				}
				break;

		case DLG_CTRL_SET_BITMAP_ICON:
				{
					if (((short)lParam) != pState->m_icon)
					{
						// Accept the new icon id.
						pState->m_icon = (short)lParam;
						::InvalidateRect(hWnd, NULL, TRUE);
					}
				}
				break;
	}

	return(::DefWindowProc(hWnd, msg, wParam, lParam));
}

//-----------------------------------------------------------------
//  =================  Brushed Static  ====================
//-----------------------------------------------------------------

LRESULT CALLBACK DlgBrushedStaticControlWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_NCCREATE:
				{
					// This struct is not properly passed into the WM_CREATE handler.
					CREATESTRUCTW *pCreateStruct = (CREATESTRUCTW*)lParam;

					// Pick up the foreground color from the title.
					DWORD  frgr_color = RGB(128, 128, 128);
					int iRed, iGreen, iBlue;
					if (swscanf(pCreateStruct->lpszName, L"%d,%d,%d", &iRed, &iGreen, &iBlue) == 3)
						frgr_color = RGB(iRed, iGreen, iBlue);

					::SetWindowLong(hWnd, 0, frgr_color);
				}
				return(TRUE);	// WM_NCCREATE message requires to return this value to continue creation of the control.

		case WM_PAINT:
				{
					// Paint the brush.
					PAINTSTRUCT ps;
					HDC hPaintDC = ::BeginPaint(hWnd, &ps);
					DWORD frgr_color = ::GetWindowLong(hWnd, 0);

					RECT rc;
					::GetClientRect(hWnd, &rc);
					HPEN hPen = ::CreatePen(PS_SOLID, 1, frgr_color);
					HGDIOBJ hOldPen = ::SelectObject(hPaintDC, hPen);

					POINT oldPos;
					for (int y = rc.top; y < rc.bottom; y += 2)
					{
						::MoveToEx(hPaintDC, rc.left, y, &oldPos);
						::LineTo(hPaintDC, rc.right, y);
					}

					::SelectObject(hPaintDC, hOldPen);
					::DeleteObject(hPen);
					::EndPaint(hWnd, &ps);
				}
				return(0);
	}

	return(::DefWindowProc(hWnd, msg, wParam, lParam));
}


