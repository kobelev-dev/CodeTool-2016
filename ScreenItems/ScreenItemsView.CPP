//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <windows.h>
#include  <assert.h>

#include  "ScreenItems/ScreenItemsView.H"
#include  "ScreenItems/ScreenItemsCore.H"

//------------------------------------------------------------------
//   ================  Static Functions  =====================
//------------------------------------------------------------------

void ScreenItemsRegisterWindowClasses(HINSTANCE hInst)
{
	if (hInst == NULL)
		hInst = ::GetModuleHandle(NULL);

	WNDCLASSEXW wndClass;
	memset(&wndClass, 0, sizeof(WNDCLASSEX));

	wndClass.style = CS_DBLCLKS;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = sizeof(LONG);
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.hInstance = hInst;
	wndClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wndClass.lpfnWndProc = (WNDPROC)TScreenItemsViewControlState::ScreenItemsViewWndProc;
	wndClass.lpszClassName = SCREEN_ITEMS_VIEW_CTRL_CLASS;

	::RegisterClassExW(&wndClass);
}

// ----------------------------------------------------------------
//  ============  TScreenItemsViewFrameProps  ===============
// ----------------------------------------------------------------

void TScreenItemsViewFrameProps::SetStndScrollableProps(TColor bkgr_color, int horz_marg, int vert_marg)
{
	memset(this, 0, sizeof(TScreenItemsViewFrameProps));

	use_inset_frame = TRUE;
	use_horz_scroll = TRUE;
	use_vert_scroll = TRUE;
	show_focus_stt  = TRUE;

	bkgr_brush = bkgr_color;
	corner_brush = ::GetSysColor(COLOR_3DFACE);
	focus_stt_dots = RGB(138, 138, 138);

	center_small_data_set_x = TRUE;
	center_small_data_set_y = TRUE;
	excl_ltop_empty_space = TRUE;

	horz_margin = horz_marg;
	vert_margin = vert_marg;
}

void TScreenItemsViewFrameProps::SetStndDocStyleProps(bool show_scroller_horz, bool show_scroller_vert, TColor bkgr_color)
{
	memset(this, 0, sizeof(TScreenItemsViewFrameProps));
	use_inset_frame = TRUE;
	use_horz_scroll = show_scroller_horz;
	use_vert_scroll = show_scroller_vert;
	show_focus_stt  = TRUE;
	bkgr_brush = bkgr_color;
	corner_brush = ::GetSysColor(COLOR_3DFACE);
	focus_stt_dots = RGB(138, 138, 138);
}

// ---------------------------------------------------------------
//  ==============  TScreenItemsViewObject  ================
// ---------------------------------------------------------------

void TScreenItemsViewObject::SetProperties(TScreenItemsViewFrameProps &new_props, bool paint_now)
{
	if (m_control != NULL)
	{
		::SendMessage(m_control, SM_SET_PROPERTIES, 0, (LPARAM)(&new_props));
		if (paint_now == TRUE)
			RepaintControl();
	}
}

void TScreenItemsViewObject::SetStndScrollableProps(TColor bkgr_color, int horz_marg, int vert_marg, bool paint_now)
{
	TScreenItemsViewFrameProps props;
	props.SetStndScrollableProps(bkgr_color, horz_marg, vert_marg);
	SetProperties(props, paint_now);
}

void TScreenItemsViewObject::SetStndDocStyleProps(bool horz_scroller, bool vert_scroller, TColor bkgr_color, bool paint_now)
{
	TScreenItemsViewFrameProps props;
	props.SetStndDocStyleProps(horz_scroller, vert_scroller, bkgr_color);
	SetProperties(props, paint_now);
}

void TScreenItemsViewObject::SetScrollPos(long pos_x, long pos_y, bool paint_now)
{
	if (m_control != NULL)
	{
		::SendMessage(m_control, SM_SET_SCROLL_POS, (LPARAM)pos_x, (LPARAM)pos_y);
		if (paint_now == TRUE)
			UpdateControl();
	}
}

void TScreenItemsViewObject::SetZoomLevel(float new_zoom_level, bool paint_now)
{
	if (m_control != NULL)
	{
		void *ptr = &new_zoom_level;
		::SendMessage(m_control, SM_SET_ZOOM_LEVEL, 0, *((LPARAM*)ptr));
		if (paint_now == TRUE)
			RepaintControl();
	}
}

// - - - - - - - - - - - Group of data set related actions - - - - - - - - - - - -

void TScreenItemsViewObject::ShowDataSet(TList *new_data_set, bool paint_now)
{
	if (m_control != NULL)
	{
		::SendMessage(m_control, SM_SHOW_MAIN_LAYER_DATA_SET, 0, (LPARAM)new_data_set);
		if (paint_now == TRUE)
			RepaintControl();
	}
}

void TScreenItemsViewObject::ShowFrontLayerDataSet(TList *new_data_set, bool paint_now)
{
	if (m_control != NULL)
	{
		::SendMessage(m_control, SM_SHOW_FRONT_LAYER_DATA_SET, 0, (LPARAM)new_data_set);
		if (paint_now == TRUE)
			RepaintControl();
	}
}

TList *TScreenItemsViewObject::CurrDataSet()
{
	if (m_control == NULL)
		return(NULL);

	return((TList*)::SendMessage(m_control, SM_GET_MAIN_LAYER_DATA_SET, 0, 0));
}

TList *TScreenItemsViewObject::CurrFrontLayerDataSet()
{
	if (m_control == NULL)
		return(NULL);

	return((TList*)::SendMessage(m_control, SM_GET_FRONT_LAYER_DATA_SET, 0, 0));
}

bool TScreenItemsViewObject::AddToInternalDataSetBeforeItem(TScreenItem &scr_item, TScreenItem *ins_before)
{
	for (TListIter<TScreenItem> iter(m_main_layer_screen_items); iter; ++iter)
	{
		if (ins_before == iter.CurrItemPtr())
		{
			scr_item.Clear();
			m_main_layer_screen_items.InsertBefore(scr_item, iter.CurrItem());
			return(TRUE);
		}
	}

	return(FALSE);
}

bool TScreenItemsViewObject::AddToInternalDataSetAfterItem(TScreenItem &scr_item, TScreenItem *ins_after)
{
	for (TListIter<TScreenItem> iter(m_main_layer_screen_items); iter; ++iter)
	{
		if (ins_after == iter.CurrItemPtr())
		{
			scr_item.Clear();
			m_main_layer_screen_items.InsertAfter(scr_item, iter.CurrItem());
			return(TRUE);
		}
	}

	return(FALSE);
}

void TScreenItemsViewObject::ScrollDataSetIntoCenter(bool center_x, bool center_y)
{
	// Scroll center of the graph into the middle of the window. This will create visual difference
	// only when the graph is bigger than the current size of the window.
	TScreenItemsViewSpaceInfo info;
	GetSpaceInfo(info);

	// Figure out the new scroll pos.
	long new_scroll_x = -1, new_scroll_y = -1;
	if (center_x == TRUE && info.app_zm_size.cx > info.client_size.cx)
		new_scroll_x = (info.app_zm_size.cx-info.client_size.cx)/2;
	if (center_y == TRUE && info.app_zm_size.cy > info.client_size.cy)
		new_scroll_y = (info.app_zm_size.cy-info.client_size.cy)/2;

	// Finally apply the scroll.
	if (new_scroll_x != -1 || new_scroll_y != -1)
		SetScrollPos(new_scroll_x, new_scroll_y);
}

bool TScreenItemsViewObject::CheckFullOrPartialScrollOut(TScreenItem &scr_item)
{
	long ifw;
	RECT rc_client;
	if (PrepareScrollCalculations(scr_item, rc_client, ifw) == FALSE)
	{
		// Preparation failed. Tell to the upper layer that scrolling is not needed.
		return(FALSE);
	}

	long horz_marg = scr_item.GetWidth()/4;
	long vert_marg = scr_item.GetHeight()/4;
	TScreenItemsViewSpaceInfo spc_info;
	GetSpaceInfo(spc_info);

	if (rc_client.right < ifw+horz_marg)
		return(TRUE);
	else if (rc_client.left > ifw+spc_info.client_size.cx-horz_marg)
		return(TRUE);
	if (rc_client.bottom < ifw+vert_marg)
		return(TRUE);
	else if (rc_client.top > ifw+spc_info.client_size.cy-vert_marg)
		return(TRUE);

	// Significant part of the object is visible. The scrolling is not needed.
	return(FALSE);
}

void TScreenItemsViewObject::ScrollScreenItemIntoTheView(TScreenItem &scr_item, long horz_marg, long vert_marg)
{
	long ifw;
	RECT rc_client;
	if (PrepareScrollCalculations(scr_item, rc_client, ifw) == FALSE)
		return;

	// Pick up the info about the current state of the view.
	TScreenItemsViewSpaceInfo spc_info;
	GetSpaceInfo(spc_info);

	if (horz_marg < 0)
		horz_marg = 0;
	if (vert_marg < 0)
		vert_marg = 0;

	// Setup initial values as if the scrolling is not needed.
	long new_scroll_x = -1, new_scroll_y = -1;

	if (rc_client.right-rc_client.left+2*horz_marg > spc_info.client_size.cx)
	{
		// The object is wider than the current view. The object should be horz centered.
		new_scroll_x = spc_info.scroll_pos.x + ((rc_client.left+rc_client.right-spc_info.client_size.cx)/2-ifw);
	}
	else if (spc_info.app_zm_size.cx > spc_info.client_size.cx)
	{
		// The view is wide enough to display the object with its margins if any completely but it is not
		// wide enough to display the whole data set. Check, if the object is completely visible or not.
		if (rc_client.left-horz_marg < ifw)
			new_scroll_x = spc_info.scroll_pos.x - (ifw-(rc_client.left-horz_marg));
		else if (rc_client.right+horz_marg > ifw+spc_info.client_size.cx)
			new_scroll_x = spc_info.scroll_pos.x + (rc_client.right+horz_marg-(ifw+spc_info.client_size.cx));
	}

	if (rc_client.bottom-rc_client.top+2*vert_marg > spc_info.client_size.cy)
	{
		// The object is higher than the current view. The object should be vert centered.
		new_scroll_y = spc_info.scroll_pos.y + ((rc_client.top+rc_client.bottom-spc_info.client_size.cy)/2-ifw);
	}
	else if (spc_info.app_zm_size.cy > spc_info.client_size.cy)
	{
		// The view is high enough to display the object with its margins if any completely but it is not
		// high enough to display the whole data set. Check, if the object is completely visible or not.
		if (rc_client.top-vert_marg < ifw)
			new_scroll_y = spc_info.scroll_pos.y - (ifw-(rc_client.top-vert_marg));
		else if (rc_client.bottom+vert_marg > ifw+spc_info.client_size.cy)
			new_scroll_y = spc_info.scroll_pos.y + (rc_client.bottom+vert_marg-(ifw+spc_info.client_size.cy));
	}

	// Finally apply the scroll.
	if (new_scroll_x != -1 || new_scroll_y != -1)
		SetScrollPos(new_scroll_x, new_scroll_y);
}

void TScreenItemsViewObject::GetSpaceInfo(TScreenItemsViewSpaceInfo &buffer)
{
	if (m_control != NULL)
	{
		::SendMessage(m_control, SM_GET_SPACE_INFO, 0, (LPARAM)(&buffer));
	}
	else
	{
		buffer.Clear();
	}
}

void TScreenItemsViewObject::GetCurrentFrameProps(TScreenItemsViewFrameProps &props)
{
	if (m_control != NULL)
	{
		::SendMessage(m_control, SM_GET_CURRENT_PROPS, 0, (LPARAM)(&props));
	}
	else
	{
		memset(&props, 0, sizeof(TScreenItemsViewFrameProps));
	}
}

void TScreenItemsViewObject::GetClientSpacePos(POINT &app_point, POINT &client_point)
{
	if (m_control != NULL)
		::SendMessage(m_control, SM_GET_CLIENT_POS, (WPARAM)&app_point, (LPARAM)&client_point);
}

void TScreenItemsViewObject::GetAppSpacePos(POINT &client_point, POINT &app_point)
{
	if (m_control != NULL)
		::SendMessage(m_control, SM_GET_APPSPACE_POS, (WPARAM)&client_point, (LPARAM)&app_point);
}

DWORD TScreenItemsViewObject::CreateScreenShotProto(RECT *margins_rect_prm, HDC &hMemoryDC, HBITMAP &hBitmapObject, HBITMAP &hBitmapOld, SIZE &bitmap_size)
{
	hMemoryDC = NULL;
	hBitmapObject = NULL;
	hBitmapOld = NULL;
	bitmap_size.cx = bitmap_size.cy = 0;

	// Pick up the DC. This will also ensure that the control window is present.
	HDC hDC = ::GetDC(m_control);
	if (hDC == NULL)
	{
		DWORD err1 = ::GetLastError();
		return(err1);
	}

	UpdateControl();
	TScreenItemsViewFrameProps props;
	GetCurrentFrameProps(props);

	long inset_frame_width = (props.use_inset_frame == TRUE) ? TControlHelper::GetInsetFrameWidth() : 0;

	// Figure out the area of the window control that should be copied to the bitmap.
	bool screen_copying_needed = FALSE;
	long screen_from_x, screen_from_y, size_to_copy_x, size_to_copy_y;
	TScreenItemsViewSpaceInfo spc_info;
	GetSpaceInfo(spc_info);

	// Prepare copy params as if there are no margins.
	if (spc_info.appsp_rect.left < spc_info.appsp_rect.right && spc_info.appsp_rect.top < spc_info.appsp_rect.bottom)
	{
		// The rect with the screen items is not empty.
		if (spc_info.client_size.cx > 0 && spc_info.client_size.cy > 0)
		{
			// The client area is not empty. Map the rect with the screen items onto the client space.
			RECT rc_client;
			GetClientSpacePos((POINT&)spc_info.appsp_rect.left, (POINT&)rc_client.left);
			GetClientSpacePos((POINT&)spc_info.appsp_rect.right, (POINT&)rc_client.right);

			// Apply the client rect constrains.
			rc_client.left = __max(rc_client.left, inset_frame_width);
			rc_client.top = __max(rc_client.top, inset_frame_width);
			rc_client.right = __min(rc_client.right, inset_frame_width+spc_info.client_size.cx);
			rc_client.bottom = __min(rc_client.bottom, inset_frame_width+spc_info.client_size.cy);

			if (rc_client.left < rc_client.right && rc_client.top < rc_client.bottom)
			{
				// The intersection that should be copied is not empty.
				screen_from_x = rc_client.left;
				screen_from_y = rc_client.top;
				size_to_copy_x = rc_client.right-rc_client.left;
				size_to_copy_y = rc_client.bottom-rc_client.top;
				screen_copying_needed = TRUE;
			}
		}
	}

	// Use either the directly specified margins or the margins from the current props.
	RECT margins_rect;
	if (margins_rect_prm == NULL)
	{
		margins_rect.left = props.horz_margin;
		margins_rect.top = props.vert_margin;
		margins_rect.right = props.horz_margin;
		margins_rect.bottom = props.vert_margin;
	}
	else
	{
		margins_rect = *margins_rect_prm;
	}

	// Calculate the size of the required bitmap.
	long bmp_size_x = margins_rect.left+margins_rect.right;
	long bmp_size_y = margins_rect.top+margins_rect.bottom;
	if (screen_copying_needed == TRUE)
	{
		bmp_size_x += size_to_copy_x;
		bmp_size_y += size_to_copy_y;
	}

	// Make a safeguard check for the zero sized bitmap. This may happen sometimes.
	if (bmp_size_x <= 0)
		bmp_size_x = 10;
	if (bmp_size_y <= 0)
		bmp_size_y = 10;

	// Pick up the second DC.
	HDC hDCMem = ::CreateCompatibleDC(hDC);
	if (hDCMem == NULL)
	{
		DWORD err2 = ::GetLastError();
		::ReleaseDC(m_control, hDC);
		return(err2);
	}

	// Create the in memory bitmap.
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, bmp_size_x, bmp_size_y);
	if (hBitmap == NULL)
	{
		DWORD err3 = ::GetLastError();
		::DeleteDC(hDCMem);
		::ReleaseDC(m_control, hDC);
		return(err3);
	}

	// Put bitmat into the memory DC. This will make it functional.
	HBITMAP hBmpOld = (HBITMAP)::SelectObject(hDCMem, hBitmap);

	// Clear the background.
	HBRUSH hBkgr = ::CreateSolidBrush(props.bkgr_brush);
	RECT bitmap_rect = { 0, 0, bmp_size_x, bmp_size_y };
	::FillRect(hDCMem, &bitmap_rect, hBkgr);
	::DeleteObject(hBkgr);

	// Copy in the data from the screen.
	if (screen_copying_needed == TRUE)
	{
		// Enlarge the area to copy if there is space on the surface of the control. This will avoid clipping
		// on the right and bottom sides on the zoomed out pictures.
		if (screen_from_x+size_to_copy_x < inset_frame_width+spc_info.client_size.cx)
			size_to_copy_x++;
		if (screen_from_y+size_to_copy_y < inset_frame_width+spc_info.client_size.cy)
			size_to_copy_y++;

		// Do the job.
		::BitBlt(hDCMem, margins_rect.left, margins_rect.top, size_to_copy_x, size_to_copy_y, hDC, screen_from_x, screen_from_y, SRCCOPY);
	}

	// Release the resources.
	::ReleaseDC(m_control, hDC);

	// Success. Give the handles out.
	hMemoryDC = hDCMem;
	hBitmapObject = hBitmap;
	hBitmapOld = hBmpOld;
	bitmap_size.cx = bmp_size_x;
	bitmap_size.cy = bmp_size_y;
	return(ERROR_SUCCESS);
}

void TScreenItemsViewObject::ScreenShotProtoCleanup(HDC &hMemoryDC, HBITMAP &hBitmapOld)
{
	assert(hMemoryDC != NULL);

	// Release the resources.
	::SelectObject(hMemoryDC, hBitmapOld);
	::DeleteDC(hMemoryDC);
}

int TScreenItemsViewObject::GetScreenItemsFromPoint(long client_x, long client_y, TScreenItem **buffer, int len_buff)
{
	TScreenItemSummarizeInfo info = { { client_x, client_y }, { 0, 0 }, 0, buffer, len_buff, 0 };
	if (m_control != NULL)
		::SendMessage(m_control, SM_GET_SCRITEMS_FROM_POINT, 0, (LPARAM)&info);
	return(info.cnt_found);
}

void TScreenItemsViewObject::InvalidateAppSpaceRect(RECT &appsp_rc)
{
	if (m_control != NULL)
		::SendMessage(m_control, SM_INVALIDATE_APPSP_RECT, 0, (LPARAM)&appsp_rc);
}

void TScreenItemsViewObject::SaveClickedPointColor(TScreenItemsViewMouseNotify *ev_info)
{
	HDC hDC = ::GetDC(m_control);
	if (hDC != NULL)
	{
		// Pick up the color of the clicked pixel and place it on the static vars of the color editor.
		COLORREF rgb = ::GetPixel(hDC, ev_info->click_place.x, ev_info->click_place.y);
		TControlHelper::m_color_select_result_color = rgb;
		::ReleaseDC(m_control, hDC);
	}
}

DWORD TScreenItemsViewObject::ShowAndTrackPopupMenu(const TMenuItemInfo *menu_info, long click_x, long click_y)
{
	HMENU hMenu = TControlHelper::CreatePopupMenuHandle(menu_info);
	if (hMenu ==  NULL)
		return(0);

	RECT rc_ctrl;
	::GetWindowRect(m_control, &rc_ctrl);

	//
	//  Do not step over this system function in debugger.
	//  Testing shows that this will prevent menu from opening.
	//
	DWORD res = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_BOTTOMALIGN,
								rc_ctrl.left+click_x, rc_ctrl.top+click_y, 0, m_control, NULL);

	::DestroyMenu(hMenu);
	return(res);
}

bool TScreenItemsViewObject::PrepareScrollCalculations(TScreenItem &scr_item, RECT &rc_src_item_client, long &inset_frame_width)
{
	if (m_control == NULL)
		return(FALSE);

	// Pick up the screen rect of the passed item.
	RECT rc_appsp = *scr_item.GetBoundingRect();
	GetClientSpacePos((POINT&)rc_appsp.left, (POINT&)rc_src_item_client.left);
	GetClientSpacePos((POINT&)rc_appsp.right, (POINT&)rc_src_item_client.right);

	if (rc_src_item_client.right < rc_src_item_client.left)
		rc_src_item_client.right = rc_src_item_client.left;
	if (rc_src_item_client.bottom < rc_src_item_client.top)
		rc_src_item_client.bottom = rc_src_item_client.top;

	TScreenItemsViewFrameProps props;
	GetCurrentFrameProps(props);
	inset_frame_width = (props.use_inset_frame == TRUE) ? TControlHelper::GetInsetFrameWidth() : 0;

	return(TRUE);
}


