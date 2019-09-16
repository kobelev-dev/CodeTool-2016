//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "WinUi/DialogControlHelpers.H"
#include  "ScreenItems/ScreenItemsView.H"
#include  "ScreenItems/ScreenItemsCore.H"

// ----------------------------------------------------------------------
//  =============  TScreenItemsViewControlState  ==================
// ----------------------------------------------------------------------

static XFORM null_xform = { 1, 0, 0, 1, 0, 0 };

TScreenItemsViewControlState::TScreenItemsViewControlState(HWND hCtrl)
{
	m_hControl = hCtrl;
	m_ctrl_id = ::GetDlgCtrlID(hCtrl);
	m_hParent = ::GetParent(hCtrl);

	// Initialise the properties with default values.
	memset(&m_props, 0, sizeof(m_props));
	m_props.use_inset_frame = TRUE;
	m_props.bkgr_brush = ::GetSysColor(COLOR_WINDOW);
	m_props.corner_brush = ::GetSysColor(COLOR_3DFACE);
	m_props.horz_margin = m_props.vert_margin = 10;

	m_hHorzScroll = m_hVertScroll = NULL;
	m_main_layer_screen_items = NULL;
	m_front_layer_screen_items = NULL;
	m_app_rect_chg = FALSE;
	memset(&m_space, 0, sizeof(m_space));
	m_space.zoom_level = 1.0;
	SetupAndInvalidate(TRUE, TRUE);
}

LRESULT CALLBACK TScreenItemsViewControlState::ScreenItemsViewWndProc(HWND hWnd, DWORD msgid, WPARAM wParam, LPARAM lParam)
{
	TScreenItemsViewControlState *stt = (TScreenItemsViewControlState*)(LONG_PTR)::GetWindowLong(hWnd, GWL_USERDATA);

	switch (msgid)
	{
		//
		// Window creation and destruction.
		//

		case WM_CREATE:
				{
					stt = new TScreenItemsViewControlState(hWnd);
					if (stt == NULL)
					{
						// Fail creation of the winow.
						return(-1);
					}

					// Store pointer to the object for future use.
					::SetWindowLong(hWnd, GWL_USERDATA, (LONG)(LONG_PTR)stt);
				}
				return(0);

		case WM_DESTROY:
				{
					if (stt != NULL)
						delete stt;
				}
				return(0);

		//
		// General windows events.
		//

		case WM_PAINT:
				{
					if (stt->m_app_rect_chg == TRUE)
						stt->SetupAndInvalidate(FALSE, TRUE);

					stt->OnPaint();
				}
				return(0);

		case WM_SIZE:
				{
					stt->SetupAndInvalidate(TRUE, stt->m_app_rect_chg);
				}
				return(0);

		case WM_HSCROLL:
				{
					if (lParam != (LPARAM)stt->m_hHorzScroll || stt->m_props.use_horz_scroll == FALSE)
						break;

					if (stt->m_app_rect_chg == TRUE)
					{
						stt->SetupAndInvalidate(FALSE, TRUE);
						break;
					}

					stt->OnHorzScroll(LOWORD(wParam));
				}
				return(0);

		case WM_VSCROLL:
				{
					if (lParam != (LPARAM)stt->m_hVertScroll || stt->m_props.use_vert_scroll == FALSE)
						break;

					if (stt->m_app_rect_chg == TRUE)
					{
						stt->SetupAndInvalidate(FALSE, TRUE);
						break;
					}

					stt->OnVertScroll(LOWORD(wParam));
				}
				return(0);

		case WM_SETFOCUS:
		case WM_KILLFOCUS:
				{
					if (stt->m_props.show_focus_stt == TRUE)
					{
						// Show/hide the dotted frame on the small corner rect.
						::InvalidateRect(hWnd, &(stt->m_corner_area), TRUE);
					}
				}
				return(0);

		case WM_MOUSEMOVE:
				{
					// Grab the focus if the mose is moved above the window. This is needed to start receiving
					// the mouse wheel messages.
					if (::GetFocus() != hWnd)
						::SetFocus(hWnd);
				}
				break;

		//
		// Mouse and keyboard events.
		//

		case WM_LBUTTONDOWN:
				{
					if (stt->m_app_rect_chg == TRUE)
						stt->SetupAndInvalidate(FALSE, TRUE);

					::SetFocus(hWnd);
					stt->OnMouseBtnEvent(wParam, lParam, xiv_left_btn_down);
				}
				return(0);

		case WM_RBUTTONDOWN:
				{
					if (stt->m_app_rect_chg == TRUE)
						stt->SetupAndInvalidate(FALSE, TRUE);

					::SetFocus(hWnd);
					stt->OnMouseBtnEvent(wParam, lParam, xiv_right_btn_down);
				}
				return(0);

		case WM_LBUTTONDBLCLK:
				{
					if (stt->m_app_rect_chg == TRUE)
						stt->SetupAndInvalidate(FALSE, TRUE);

					::SetFocus(hWnd);
					stt->OnMouseBtnEvent(wParam, lParam, xiv_left_btn_dblclk);
				}
				return(0);

		case WM_MOUSEWHEEL:
				{
					// Use simple implementation for low resolution mouses. Make it better when
					// better mouses will be around.
					int zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
					if (zDelta != 0)
						stt->OnVertScroll((zDelta > 0) ? SB_LINELEFT : SB_LINERIGHT);
				}
				return(0);

		case WM_CHAR:
				{
					TScreenItemsViewNotify info;
					info.wparam = wParam;
					info.lparam = lParam;
					stt->SendNotification(xiv_wm_char, &info);
				}
				return(0);

		//
		// Control class specific messages.
		//

		case SM_SET_PROPERTIES:
				{
					stt->m_props = *((TScreenItemsViewFrameProps*)lParam);
					stt->SetupAndInvalidate(TRUE, stt->m_app_rect_chg);
				}
				return(0);

		case SM_SET_SCROLL_POS:
				{
					if (stt->m_app_rect_chg == TRUE)
						stt->SetupAndInvalidate(FALSE, TRUE);

					stt->VerifyAndSetupScrollPos((long)wParam, (long)lParam);
				}
				return(0);

		case SM_SET_ZOOM_LEVEL:
				{
					// This trick is a work around the point that LPARAM is a signed integer.
					// Direct typecast will change the binary value.
					void *ptr = &lParam;
					stt->m_space.zoom_level = *((float*)ptr);

					assert(stt->m_space.zoom_level != 0);
					stt->SetupAndInvalidate(FALSE, TRUE);
				}
				return(0);

		case SM_SHOW_MAIN_LAYER_DATA_SET:
				{
					stt->m_main_layer_screen_items = (TList*)lParam;
					stt->SetupAndInvalidate(FALSE, TRUE);
				}
				return(0);

		case SM_SHOW_FRONT_LAYER_DATA_SET:
				{
					stt->m_front_layer_screen_items = (TList*)lParam;
					::InvalidateRect(stt->m_hControl, NULL, TRUE);
				}
				return(0);

		case SM_INVALIDATE_APPSP_RECT:
				{
					stt->InvalidateAppSpaceRect(*(RECT*)lParam);
				}
				return(0);

		case SM_GET_SPACE_INFO:
				{
					if (stt->m_app_rect_chg == TRUE)
						stt->SetupAndInvalidate(FALSE, TRUE);

					*((TScreenItemsViewSpaceInfo*)lParam) = stt->m_space;
				}
				return(0);

		case SM_GET_CURRENT_PROPS:
				{
					*((TScreenItemsViewFrameProps*)lParam) = stt->m_props;
				}
				return(0);

		case SM_GET_CLIENT_POS:
				{
					stt->AppSpacePointToClientPoint((POINT*)wParam, (POINT*)lParam);
				}
				return(0);

		case SM_GET_APPSPACE_POS:
				{
					stt->ClientPointToAppSpacePoint((POINT*)wParam, (POINT*)lParam);
				}
				return(0);

		case SM_GET_MAIN_LAYER_DATA_SET:
				{
					return((LRESULT)stt->m_main_layer_screen_items);
				}

		case SM_GET_FRONT_LAYER_DATA_SET:
				{
					return((LRESULT)stt->m_front_layer_screen_items);
				}

		case SM_GET_SCRITEMS_FROM_POINT:
				{
					TScreenItemSummarizeInfo *info = (TScreenItemSummarizeInfo*)lParam;
					stt->ClientPointToAppSpacePoint(&(info->click_pt), &(info->app_pt));
					stt->ProcessScreenItems(info->click_pt, info->app_pt, info);
				}
				return(0);
	}

	// Message was not handled in the switch statement.
	return(::DefWindowProc(hWnd, msgid, wParam, lParam));
}

int TScreenItemsViewControlState::CalcBoundingRect(TList *data, RECT &rc_out)
{
	if (data == NULL)
		return(0);

	int num_placed_items = 0;

	// Iterate the items of the list.
	RECT rc_max;
	for (TListIter<TScreenItem> iter(data); iter; ++iter)
	{
		TScreenItem &item = iter.CurrItem();
		if (item.GetBasicStyle() == NULL)
			continue;

		RECT *rc = item.GetBoundingRect();
		if (num_placed_items == 0)
		{
			rc_max = *rc;
			num_placed_items = 1;
			continue;
		}

		if (rc->left < rc_max.left)
			rc_max.left = rc->left;
		if (rc->top < rc_max.top)
			rc_max.top = rc->top;
		if (rc->right > rc_max.right)
			rc_max.right = rc->right;
		if (rc->bottom > rc_max.bottom)
			rc_max.bottom = rc->bottom;

		num_placed_items++;
	}

	// Give out the results, collected so far.
	if (num_placed_items > 0)
		rc_out = rc_max;
	return(num_placed_items);
}

void TScreenItemsViewControlState::OnPaint()
{
	// Prepare the painting.
	PAINTSTRUCT ps;
	HDC dc = ::BeginPaint(m_hControl, &ps);
	if (dc == NULL)
		return;

#ifdef TRACE_SCREEN_ITEMS_PAINT
	//
	//  This code should be not active by default.
	//
	LogMessageFmt(L"ScreenItems::OnPaint. rc_invalid: left=%ld, top=%ld, right=%ld, bottom=%ld.",
					ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
#endif

	if (m_screen_items_area.left != m_screen_items_area.right && m_screen_items_area.top != m_screen_items_area.bottom)
	{
		//
		// Current layout and size of the window allow to draw the screen items.
		//

		// Prepare the clipping rect.
		int dc_inx = ::SaveDC(dc);
		::IntersectClipRect(dc, m_screen_items_area.left, m_screen_items_area.top, m_screen_items_area.right, m_screen_items_area.bottom);

		// Erase the background.
		HBRUSH hBkgr = ::CreateSolidBrush(m_props.bkgr_brush);
		::FillRect(dc, &ps.rcPaint, hBkgr);
		::DeleteObject(hBkgr);

		// Setup the appropriate transform in DC.
		::SetGraphicsMode(dc, GM_ADVANCED);
		::SetWorldTransform(dc, &m_xform);

		// Give an application a chance to paint the background.
		SendNotification(xiv_paint_background, &ps);

		// Iterate the list of objects and paint them.
		if (m_main_layer_screen_items != NULL)
		{
			// Recalculate invalid rect into the appsp coordinates.
			RECT inv_appsp;
			ClientPointToAppSpacePoint((POINT*)&ps.rcPaint.left, (POINT*)&inv_appsp.left);
			ClientPointToAppSpacePoint((POINT*)&ps.rcPaint.right, (POINT*)&inv_appsp.right);

			// Iterate the items of the list.
			for (TListIter<TScreenItem> iter(m_main_layer_screen_items); iter; ++iter)
			{
				TScreenItem *item = &iter.CurrItem();
				RECT *rc_item = item->GetBoundingRect();
				if (item->GetBasicStyle() != NULL && TScreenItem::CheckIntersection(inv_appsp, *rc_item))
				{
					// Current item has intersection with the invalid area.
					item->OnDraw(dc, inv_appsp);
				}
			}
		}

		// Application may wish to paint something above the screen items. Give it a chance.
		SendNotification(xiv_paint_topmost, &ps);

		// Reset the DC back.
		::SetWorldTransform(dc, &null_xform);
		::SetGraphicsMode(dc, GM_COMPATIBLE);

		// Paint the screen items of the front layer.
		if (m_front_layer_screen_items != NULL)
		{
			// Front layer items are painted directly on the client space.
			for (TListIter<TScreenItem> iter(m_front_layer_screen_items); iter; ++iter)
			{
				TScreenItem *item = &iter.CurrItem();
				RECT *rc_item = item->GetBoundingRect();
				if (item->GetBasicStyle() != NULL && TScreenItem::CheckIntersection(ps.rcPaint, *rc_item))
				{
					// Item has intersection with the invalid area.
					item->OnDraw(dc, ps.rcPaint);
				}
			}
		}

		if (dc_inx != 0)
			::RestoreDC(dc, dc_inx);
	}

	// Paint the corner area between the scrollbars.
	if (m_corner_area.left != m_corner_area.right && m_corner_area.top != m_corner_area.bottom)
	{
		HBRUSH hCornerBrush = ::CreateSolidBrush(m_props.corner_brush);
		::FillRect(dc, &m_corner_area, hCornerBrush);
		::DeleteObject(hCornerBrush);

		if (m_props.show_focus_stt && ::GetFocus() == m_hControl)
		{
			// Paint the dotted frame in the corner.
			long sz_x = m_corner_area.right-m_corner_area.left;
			long sz_y = m_corner_area.bottom-m_corner_area.top;
			if (sz_x >= 7 && sz_y >= 7)
			{
				int flip = 0;
				for (int i1=3; i1<sz_y-3; ++i1)
					if (flip ^= 1)
						::SetPixel(dc, m_corner_area.left+3, m_corner_area.top+i1, m_props.focus_stt_dots);
				for (int i2=3; i2<sz_x-3; ++i2)
					if (flip ^= 1)
						::SetPixel(dc, m_corner_area.left+i2, m_corner_area.bottom-3, m_props.focus_stt_dots);
				for (int i3=sz_y-3; i3>3; --i3)
					if (flip ^= 1)
						::SetPixel(dc, m_corner_area.right-3, m_corner_area.top+i3, m_props.focus_stt_dots);
				for (int i4=sz_x-3; i4>3; --i4)
					if (flip ^= 1)
						::SetPixel(dc, m_corner_area.left+i4, m_corner_area.top+3, m_props.focus_stt_dots);
			}
		}
	}

	// Paint the border around the control.
	if (m_props.use_inset_frame == TRUE)
	{
		RECT rc;
		::GetClientRect(m_hControl, &rc);
		TControlHelper::PaintInsetFrame(dc, rc);
	}

	// Release the DC.
	::EndPaint(m_hControl, &ps);
}

void TScreenItemsViewControlState::OnHorzScroll(int scroll_event)
{
	// Prepare the new scroll position candidate.
	int pos = m_space.scroll_pos.x;
	switch (scroll_event)
	{
		case SB_LEFT:
				pos = 0;
				break;

		case SB_RIGHT:
				pos = m_space.app_zm_size.cx-m_space.client_size.cx;
				break;

		case SB_LINELEFT:
				pos -= m_space.client_size.cx/10;
				if (pos == m_space.scroll_pos.x)
					pos--;
				break;

		case SB_LINERIGHT:
				pos += m_space.client_size.cx/10;
				if (pos == m_space.scroll_pos.x)
					pos++;
				break;

		case SB_PAGELEFT:
				pos -= m_space.client_size.cx/2;
				if (pos == m_space.scroll_pos.x)
					pos--;
				break;

		case SB_PAGERIGHT:
				pos += m_space.client_size.cx/2;
				if (pos == m_space.scroll_pos.x)
					pos++;
				break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
				{
					SCROLLINFO si;
					si.cbSize = sizeof(si);
					si.fMask  = SIF_ALL;
					::GetScrollInfo(m_hHorzScroll, SB_CTL, &si);
					pos = si.nTrackPos;
				}
				break;

		case SB_ENDSCROLL:
		default:
			{
				return;
			}
	}

	// Update the scroller and the screen items area.
	if (pos < 0)
		pos = 0;
	VerifyAndSetupScrollPos(pos, -1);
}

void TScreenItemsViewControlState::OnVertScroll(int scroll_event)
{
	// Prepare the new scroll position candidate.
	int pos = m_space.scroll_pos.y;
	switch (scroll_event)
	{
		case SB_LEFT:
				pos = 0;
				break;

		case SB_RIGHT:
				pos = m_space.app_zm_size.cy-m_space.client_size.cy;
				break;

		case SB_LINELEFT:
				pos -= m_space.client_size.cy/10;
				if (pos == m_space.scroll_pos.y)
					pos--;
				break;

		case SB_LINERIGHT:
				pos += m_space.client_size.cy/10;
				if (pos == m_space.scroll_pos.y)
					pos++;
				break;

		case SB_PAGELEFT:
				pos -= m_space.client_size.cy/2;
				if (pos == m_space.scroll_pos.y)
					pos--;
				break;

		case SB_PAGERIGHT:
				pos += m_space.client_size.cy/2;
				if (pos == m_space.scroll_pos.y)
					pos++;
				break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
				{
					SCROLLINFO si;
					si.cbSize = sizeof(si);
					si.fMask  = SIF_ALL;
					::GetScrollInfo(m_hVertScroll, SB_CTL, &si);
					pos = si.nTrackPos;
				}
				break;

		case SB_ENDSCROLL:
		default:
			{
				return;
			}
	}

	// Update the scroller and screen items area.
	if (pos < 0)
		pos = 0;
	VerifyAndSetupScrollPos(-1, pos);
}

void TScreenItemsViewControlState::OnMouseBtnEvent(WPARAM wparam, LPARAM lparam, TScreenItemsViewNtfCode evnt_code)
{
	// Prepare the client coordinates of the mouse click.
	POINT click_place = { LOWORD(lparam), HIWORD(lparam) };

	if (TScreenItem::IsPointInRect(click_place, m_corner_area) == TRUE)
	{
		// User has clicked the right bottom corner.
		TScreenItemsViewNotify view_info;
		memset(&view_info, 0, sizeof(TScreenItemsViewNotify));
		view_info.vkey = (DWORD)wparam;
		view_info.pos_x = click_place.x;
		view_info.pos_y = click_place.y;
		view_info.right_btn = (evnt_code == xiv_right_btn_down) ? TRUE : FALSE;
		SendNotification(xiv_corner_clicked, &view_info);
		return;
	}

	TScreenItemsViewMouseNotify info;
	info.vkey = (DWORD)wparam;

	// Set up the app coordinates in the notification record.
	POINT app_place;
	ClientPointToAppSpacePoint(&click_place, &app_place);
	info.click_place = click_place;
	info.app_x = app_place.x;
	info.app_y = app_place.y;

	// Call method that will scan all available screen items.
	info.object = ProcessScreenItems(click_place, app_place, NULL);

	// Send notification regardless if something was found or not.
	SendNotification(evnt_code, &info);
}

void TScreenItemsViewControlState::InvalidateAppSpaceRect(RECT &app_rect)
{
	// Allow the graph changes to stay on the margins area.
	long zm_horz_mrgn = (m_space.zoom_level != 0) ? (long)(m_props.horz_margin/m_space.zoom_level) : 0;
	long zm_vert_mrgn = (m_space.zoom_level != 0) ? (long)(m_props.vert_margin/m_space.zoom_level) : 0;

	long ref_left = m_space.appsp_rect.left - ((m_props.excl_ltop_empty_space == TRUE) ? zm_horz_mrgn :  0);
	long ref_right = m_space.appsp_rect.top - ((m_props.excl_ltop_empty_space == TRUE) ? zm_vert_mrgn :  0);
	long ref_top = m_space.appsp_rect.right + zm_horz_mrgn;
	long ref_bottom = m_space.appsp_rect.bottom + zm_vert_mrgn;

	if (app_rect.left < ref_left || app_rect.top  < ref_right || app_rect.right  > ref_top || app_rect.bottom > ref_bottom)
	{
		// All or part of the submitted rect stays outside of the current screen items application space area.
		// Major changes on the screen are needed.
		::InvalidateRect(m_hControl, NULL, TRUE);
		m_app_rect_chg = TRUE;
		return;
	}

	// Map this application rect into the client coordinates.
	RECT cln_rect = { 0, 0, 0, 0 };
	AppSpacePointToClientPoint((POINT*)&(app_rect.left),  (POINT*)&(cln_rect.left));
	AppSpacePointToClientPoint((POINT*)&(app_rect.right), (POINT*)&(cln_rect.right));

	// Extend the rect a little bit to fix the rounding errors.
	cln_rect.right += 2;
	cln_rect.bottom++;

	// Invalidate the appropriate part of the control.
	if (TScreenItem::CheckIntersection(cln_rect, m_screen_items_area) == TRUE)
		::InvalidateRect(m_hControl, &cln_rect, TRUE);
}

TScreenItem *TScreenItemsViewControlState::ProcessScreenItems(POINT &click_place, POINT &app_place, TScreenItemSummarizeInfo *sumry_info)
{
	TScreenItem *res = NULL;

	// Check the front layer objects first.
	if (m_front_layer_screen_items != NULL)
	{
		// List of the front layer items is not empty. Iterate items in the reversed order.
		TListIter<TScreenItem> iter(m_front_layer_screen_items);
		for (iter.SeekToLast(); iter; --iter)
		{
			TScreenItem *item = &iter.CurrItem();
			if (item->m_item_owner != NULL)
			{
				res = item->CheckClick(click_place);
				if (res != NULL)
				{
					if (sumry_info == NULL)
						break;

					item->SummarizeScreenItems(*sumry_info, FALSE);
				}
			}
		}
	}

	if (res != NULL && sumry_info == NULL)
		return(res);

	// Object on the front layer was not found or iteration of all screen items was requested.
	// Check the main layer screen items.
	if (TScreenItem::IsPointInRect(click_place, m_screen_items_area) == TRUE)
	{
		// User has clicked area where screen items might sit. Loop over all objects in the data set.
		// Only objects with item_owner not NULL are clickable.
		if (m_main_layer_screen_items != NULL)
		{
			// Determine the size of the client pixel on the appsp plane.
			long app_clx = (long)(1/m_xform.eM11);
			long app_cly = (long)(1/m_xform.eM22);

			// Check, if the graph is zoomed out or not.
			if (app_clx < 4 && app_cly < 4)
			{
				// Use the point based location method. Iterate the list from the last item to first because
				// the list is painted in the forward direction.
				TListIter<TScreenItem> iter(m_main_layer_screen_items);
				for (iter.SeekToLast(); iter; --iter)
				{
					TScreenItem *item = &iter.CurrItem();
					if (item->m_item_owner != NULL)
					{
						res = item->CheckClick(app_place);
						if (res != NULL)
						{
							if (sumry_info == NULL)
								break;

							item->SummarizeScreenItems(*sumry_info, FALSE);
						}
					}
				}
			}
			else
			{
				// Use the bounding rect based location method. It is less accurate, but it allows to find objects when
				// the app space is deeply zoomed out.
				RECT app_rect;
				ClientPointToAppSpacePoint(&click_place, (POINT*)&app_rect);
				app_rect.right = app_rect.left + app_clx;
				app_rect.bottom = app_rect.top + app_cly;

				// Iterate the list from the last item to first because the list is painted in the forward direction.
				TListIter<TScreenItem> iter(m_main_layer_screen_items);
				for (iter.SeekToLast(); iter; --iter)
				{
					TScreenItem *item = &iter.CurrItem();
					RECT *rc_item = item->GetBoundingRect();
					if (item->m_item_owner != NULL && TScreenItem::CheckIntersection(app_rect, *rc_item) == TRUE)
					{
						res = item;
						if (sumry_info == NULL)
							break;

						item->SummarizeScreenItems(*sumry_info, TRUE);
					}
				}
			}
		}
	}

	// Return the latest discovered item or nothing.
	return(res);
}

void TScreenItemsViewControlState::SetupAndInvalidate(bool layout_chg, bool data_chg)
{
	// Prepare the new state of the intermediate data.
	TScreenItemsViewSpaceInfo old_spi = m_space;
	if (layout_chg == TRUE)
		CalcControlLayout();

	CalcDataSetLayout(data_chg);
	int diff_x = m_space.app_zm_size.cx-m_space.client_size.cx;
	int diff_y = m_space.app_zm_size.cy-m_space.client_size.cy;

	// Check if the data set should be centered in the screen items area or not.
	m_cntr_shift.x = m_cntr_shift.y = 0;
	if (m_props.center_small_data_set_x == TRUE)
	{
		if (diff_x < 0)
			m_cntr_shift.x = -diff_x/2;
	}

	if (m_props.center_small_data_set_y == TRUE)
	{
		if (diff_y < 0)
			m_cntr_shift.y = -diff_y/2;
	}

	// Hard point: What the new scroll state should be.
	m_space.scroll_pos.x = m_space.scroll_pos.y = 0;
	if (diff_x > 0 && old_spi.scroll_pos.x > 0 && old_spi.app_zm_size.cx > 0)
	{
		// The prevoius horz scroll state was not zero.
		double old_mpos = old_spi.scroll_pos.x+old_spi.client_size.cx/2;
		double sz_ratio = m_space.app_zm_size.cx/((double)old_spi.app_zm_size.cx);
		m_space.scroll_pos.x = (long)(old_mpos*sz_ratio-m_space.client_size.cx/2);
		VerifyHorzScrollPos(m_space.scroll_pos.x);
	}

	if (diff_y > 0 && old_spi.scroll_pos.y > 0 && old_spi.app_zm_size.cy > 0)
	{
		// The prevoius vert scroll state was not zero.
		double old_mpos = old_spi.scroll_pos.y+old_spi.client_size.cy/2;
		double sz_ratio = m_space.app_zm_size.cy/((double)old_spi.app_zm_size.cy);
		m_space.scroll_pos.y = (long)(old_mpos*sz_ratio-m_space.client_size.cy/2);
		VerifyVertScrollPos(m_space.scroll_pos.y);
	}

	// Fill in the properties of the horz scroller.
	if (m_hHorzScroll != NULL)
	{
		if (m_space.app_zm_size.cx > 0 && diff_x > 0)
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
			si.nMin  = 0;
			si.nMax  = m_space.app_zm_size.cx-1;
			si.nPage = m_space.client_size.cx;
			si.nPos  = m_space.scroll_pos.x;
			::EnableWindow(m_hHorzScroll, TRUE);
			::SetScrollInfo(m_hHorzScroll, SB_CTL, &si, TRUE);
		}
		else
		{
			// Disable the sroll bar.
			::EnableWindow(m_hHorzScroll, FALSE);
		}
	}

	// Fill in the properties of the vert scroller.
	if (m_hVertScroll != NULL)
	{
		if (m_space.app_zm_size.cy > 0 && diff_y > 0)
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
			si.nMin  = 0;
			si.nMax  = m_space.app_zm_size.cy-1;
			si.nPage = m_space.client_size.cy;
			si.nPos  = m_space.scroll_pos.y;
			::EnableWindow(m_hVertScroll, TRUE);
			::SetScrollInfo(m_hVertScroll, SB_CTL, &si, TRUE);
		}
		else
		{
			// Disable the sroll bar.
			::EnableWindow(m_hVertScroll, FALSE);
		}
	}

	// Prepare the conversion structure.
	m_xform.eM11 = m_xform.eM22 = m_space.zoom_level;
	m_xform.eM12 = m_xform.eM21 = 0;
	m_xform.eDx  = (float)(m_app_shift.x + m_cntr_shift.x - m_space.scroll_pos.x);
	m_xform.eDy  = (float)(m_app_shift.y + m_cntr_shift.y - m_space.scroll_pos.y);

	// Send the notification.
	if (m_space.scroll_pos.x != old_spi.scroll_pos.x || m_space.scroll_pos.y != old_spi.scroll_pos.y)
		SendNewScrollPosNotification();

	// Ultimately invalidate the whole control to initiate repainting.
	::InvalidateRect(m_hControl, NULL, TRUE);
	m_app_rect_chg = FALSE;
}

void TScreenItemsViewControlState::VerifyHorzScrollPos(long &new_pos)
{
	int max_scroll = m_space.app_zm_size.cx - m_space.client_size.cx;
	if (new_pos <= 0 || max_scroll <= 0)
		new_pos = 0;
	else if (new_pos > max_scroll)
		new_pos = max_scroll;
}

void TScreenItemsViewControlState::VerifyVertScrollPos(long &new_pos)
{
	int max_scroll = m_space.app_zm_size.cy - m_space.client_size.cy;
	if (new_pos <= 0 || max_scroll <= 0)
		new_pos = 0;
	else if (new_pos > max_scroll)
		new_pos = max_scroll;
}

void TScreenItemsViewControlState::VerifyAndSetupScrollPos(long new_scroll_x, long new_scroll_y)
{
	// Check that the screen items area is not empty.
	if (m_space.client_size.cy <= 0 || m_space.client_size.cy <= 0)
		return;

#ifdef TRACE_SCREEN_ITEMS_SCROLL
	//
	//  This code should be not active by default.
	//
	LogMessageFmt(L"ScreenItems::VerifyAndSetupScrollPos. new_scroll_x=%ld, new_scroll_y=%ld.",
					new_scroll_x, new_scroll_y);
#endif

	// Resolve defaults and verify the new scroll positions.
	if (new_scroll_x < 0)
		new_scroll_x = m_space.scroll_pos.x;
	if (new_scroll_y < 0)
		new_scroll_y = m_space.scroll_pos.y;

	VerifyHorzScrollPos(new_scroll_x);
	VerifyVertScrollPos(new_scroll_y);

	int shift_x = m_space.scroll_pos.x-new_scroll_x;
	if (shift_x != 0)
	{
		m_space.scroll_pos.x = new_scroll_x;
		if (m_hHorzScroll != NULL)
		{
			// Scroll bar requires resetup.
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			si.nPos = new_scroll_x;
			::SetScrollInfo(m_hHorzScroll, SB_CTL, &si, TRUE);
		}
	}

	int shift_y = m_space.scroll_pos.y-new_scroll_y;
	if (shift_y != 0)
	{
		m_space.scroll_pos.y = new_scroll_y;
		if (m_hVertScroll != NULL)
		{
			// Scroll bar requires resetup.
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			si.nPos = new_scroll_y;
			::SetScrollInfo(m_hVertScroll, SB_CTL, &si, TRUE);
		}
	}

	if (shift_x != 0 || shift_y != 0)
	{
		// The screen items area requires scrolling or repainting.
		m_xform.eDx  = (float)(m_app_shift.x + m_cntr_shift.x - m_space.scroll_pos.x);
		m_xform.eDy  = (float)(m_app_shift.y + m_cntr_shift.y - m_space.scroll_pos.y);

		RECT sc_rect = m_screen_items_area;

		if (shift_x > 0)
			sc_rect.right -= shift_x;
		else if (shift_x < 0)
			sc_rect.left -= shift_x;

		if (shift_y > 0)
			sc_rect.bottom -= shift_y;
		else if (shift_y < 0)
			sc_rect.top -= shift_y;

		if (sc_rect.left < sc_rect.right && sc_rect.top < sc_rect.bottom)
		{
			// The shift is not dramaticaly big. Some part of the image have survived. The name of the function below
			// is misleading. Its semantics is more close to "move part of the window to some other place".
			::ScrollWindowEx(m_hControl, shift_x, shift_y, &sc_rect, NULL, NULL, NULL, SW_INVALIDATE);

			// Invalidate everything manually.
			RECT ext = m_screen_items_area;

			if (shift_x > 0)
			{
				ext.right = ext.left+shift_x;
				::InvalidateRect(m_hControl, &ext, TRUE);
				ext.right = m_screen_items_area.right;
			}

			if (shift_x < 0)
			{
				ext.left = ext.right+shift_x;
				::InvalidateRect(m_hControl, &ext, TRUE);
				ext.left = m_screen_items_area.left;
			}

			if (shift_y > 0)
			{
				ext.bottom = ext.top+shift_y;
				::InvalidateRect(m_hControl, &ext, TRUE);
				ext.bottom = m_screen_items_area.bottom;
			}

			if (shift_y < 0)
			{
				ext.top = ext.bottom+shift_y;
				::InvalidateRect(m_hControl, &ext, TRUE);
				ext.top = m_screen_items_area.top;
			}

			// Check the front layer screen items. They have special scroll procedure.
			if (m_front_layer_screen_items != NULL)
			{
				// Invalidate areas that are covered by the front layer screen items that were intersecting with survied area before the shift.
				for (TListIter<TScreenItem> iter1(m_front_layer_screen_items); iter1; ++iter1)
				{
					TScreenItem *item = &iter1.CurrItem();
					RECT *rc_item = item->GetBoundingRect();
					if (item->m_style != NULL && TScreenItem::CheckIntersection(sc_rect, *rc_item) == TRUE)
					{
						// Shift the rect of the item and invalidate it.
						RECT rc = *rc_item;
						ShiftRect(rc, shift_x, shift_y);
						::InvalidateRect(m_hControl, &rc, TRUE);
					}
				}

				// Send an application callback to update the front layer screen items.
				TScreenItemsUpdateFrontLayerNotify ext = { TRUE, shift_x, shift_y };
				SendNotification(xiv_updt_front_scr_items, &ext);

				// Invalidate areas that are covered by front layer screen items that are intersecting with survied area after the shift.
				ShiftRect(sc_rect, shift_x, shift_y);
				for (TListIter<TScreenItem> iter2(m_front_layer_screen_items); iter2; ++iter2)
				{
					TScreenItem *item = &iter2.CurrItem();
					RECT *rc_item = item->GetBoundingRect();
					if (item->m_style != NULL && TScreenItem::CheckIntersection(sc_rect, *rc_item) == TRUE)
					{
						// Invalidate current rect of the screen item.
						::InvalidateRect(m_hControl, rc_item, TRUE);
					}
				}
			}
		}
		else
		{
			// Invalidate the whole control. Completely different part of the graph should be displayed.
			::InvalidateRect(m_hControl, &m_screen_items_area, TRUE);

			// Send an application callback to update the front layer screen items. There is no need for special
			// invalidation procedure because everything will be repainted.
			TScreenItemsUpdateFrontLayerNotify ext = { TRUE, shift_x, shift_y };
			SendNotification(xiv_updt_front_scr_items, &ext);
		}

		// Update the screen immediately.
		::UpdateWindow(m_hControl);
		SendNewScrollPosNotification();
	}
}

void TScreenItemsViewControlState::CalcDataSetLayout(bool full_recalc)
{
	if (full_recalc == TRUE)
	{
		// Calculate the bounding rectangle around all available screen items.
		memset(&m_space.appsp_rect, 0, sizeof(RECT));
		m_space.num_scr_items = CalcBoundingRect(m_main_layer_screen_items, m_space.appsp_rect);
	}

	// Calculate the boundaries of the zoomed app space.
	RECT app_zm_rect;
	app_zm_rect.left = (long)(m_space.appsp_rect.left*m_space.zoom_level);
	app_zm_rect.top  = (long)(m_space.appsp_rect.top*m_space.zoom_level);
	app_zm_rect.right  = (long)(m_space.appsp_rect.right*m_space.zoom_level);
	app_zm_rect.bottom = (long)(m_space.appsp_rect.bottom*m_space.zoom_level);

	// Apply margins according to the current style. Determine shift to the 0 point and the size of the zoomed area.
	if (m_props.excl_ltop_empty_space == TRUE)
	{
		m_app_shift.x = -app_zm_rect.left + m_props.horz_margin;
		m_app_shift.y = -app_zm_rect.top + m_props.vert_margin;
		m_space.app_zm_size.cx = (app_zm_rect.right-app_zm_rect.left) + 2*m_props.horz_margin;
		m_space.app_zm_size.cy = (app_zm_rect.bottom-app_zm_rect.top) + 2*m_props.vert_margin;
	}
	else
	{
		m_app_shift.x = 0;
		m_space.app_zm_size.cx = app_zm_rect.right + m_props.horz_margin;
		if (app_zm_rect.left < 0)
		{
			m_app_shift.x = -app_zm_rect.left;
			m_space.app_zm_size.cx -= app_zm_rect.left;
		}

		m_app_shift.y = 0;
		m_space.app_zm_size.cy = app_zm_rect.bottom + m_props.vert_margin;
		if (app_zm_rect.top < 0)
		{
			m_app_shift.y = -app_zm_rect.top;
			m_space.app_zm_size.cy -= app_zm_rect.top;
		}
	}

	// Check, if extra shift because of the inset frame is needed.
	if (m_props.use_inset_frame == TRUE)
	{
		m_app_shift.x += 2;
		m_app_shift.y += 2;
	}
}

void TScreenItemsViewControlState::CalcControlLayout()
{
	RECT ctrl_rect;
	::GetClientRect(m_hControl, &ctrl_rect);
	int  frame = (m_props.use_inset_frame == TRUE) ? TControlHelper::GetInsetFrameWidth() : 0;
	int  vscroll_dx = 0, hscroll_dy = 0;

	// Retrieve the scroller widths if there is no direct custom request.
	if (m_props.horz_scroll_height <= 0)
		m_props.horz_scroll_height = ::GetSystemMetrics(SM_CXVSCROLL);
	if (m_props.vert_scroll_width <= 0)
		m_props.vert_scroll_width  = ::GetSystemMetrics(SM_CYHSCROLL);

	// Split the available width in horzontal direction.
	int width = ctrl_rect.right-2*frame;
	if (m_props.use_vert_scroll == TRUE)
	{
		if (width > 0)
			vscroll_dx = (width > m_props.vert_scroll_width) ? m_props.vert_scroll_width : width;
		width -= m_props.vert_scroll_width;
	}

	if (width < 0)
		width = 0;

	// Split the available height in vertical direction.
	int height = ctrl_rect.bottom-2*frame;
	if (m_props.use_horz_scroll == TRUE)
	{
		if (height > 0)
			hscroll_dy = (height > m_props.horz_scroll_height) ? m_props.horz_scroll_height : height;
		height -= m_props.horz_scroll_height;
	}

	if (height < 0)
		height = 0;

	// Fill in the info about the screen items area.
	m_screen_items_area.left = frame;
	m_screen_items_area.top  = frame;
	m_screen_items_area.right  = frame+width;
	m_screen_items_area.bottom = frame+height;
	m_space.client_size.cx = width;
	m_space.client_size.cy = height;

	// Create/Move/Destroy the vertical scrollbar.
	if (vscroll_dx > 0 && height > 0)
	{
		// Vert scroll is requested and there is space on the control for it.
		if (m_hVertScroll != NULL)
		{
			::SetWindowPos(m_hVertScroll, NULL,
						frame+width, frame, vscroll_dx, height,
						SWP_NOZORDER);
		}
		else
		{
			HINSTANCE hInst = (HINSTANCE)::GetWindowLong(m_hControl, GWL_HINSTANCE);
			m_hVertScroll = ::CreateWindowW(L"SCROLLBAR",						// Class name.
									NULL,									// Window title.
									WS_CHILD | WS_VISIBLE | WS_DISABLED | SBS_VERT,
									frame+width, frame, vscroll_dx, height,		// Position.
									m_hControl,								// Parent window.
									NULL, hInst, NULL);
		}
	}
	else if (m_hVertScroll != NULL)
	{
		// Get rid of the existing scrollbar.
		::DestroyWindow(m_hVertScroll);
		m_hVertScroll = NULL;
	}

	// Create/Move/Destroy the horizontal scrollbar.
	if (hscroll_dy > 0 && width > 0)
	{
		// Horz scroll is requested and there is a space on the control for this scroll.
		if (m_hHorzScroll != NULL)
		{
			::SetWindowPos(m_hHorzScroll, NULL,
						frame, frame+height, width, hscroll_dy,
						SWP_NOZORDER);
		}
		else
		{
			HINSTANCE hInst = (HINSTANCE)::GetWindowLong(m_hControl, GWL_HINSTANCE);
			m_hHorzScroll = ::CreateWindowW(L"SCROLLBAR",						// Class name.
									NULL,									// Window title.
									WS_CHILD | WS_VISIBLE | WS_DISABLED | SBS_HORZ,
									frame, frame+height, width, hscroll_dy,		// Position.
									m_hControl,								// Parent window.
									NULL, hInst, NULL);
		}
	}
	else if (m_hHorzScroll != NULL)
	{
		// Get rid of the existing scrollbar.
		::DestroyWindow(m_hHorzScroll);
		m_hHorzScroll = NULL;
	}

	// Fill in the info about the right bottom area.
	if (vscroll_dx > 0 && hscroll_dy > 0)
	{
		m_corner_area.left = frame+width;
		m_corner_area.top  = frame+height;
		m_corner_area.right  = frame+width+vscroll_dx;
		m_corner_area.bottom = frame+height+hscroll_dy;
	}
	else
	{
		memset(&m_corner_area, 0, sizeof(RECT));
	}
}

void TScreenItemsViewControlState::AppSpacePointToClientPoint(POINT *app_point, POINT *cln_point)
{
	cln_point->x = (long)(app_point->x*m_xform.eM11+m_xform.eDx);
	cln_point->y = (long)(app_point->y*m_xform.eM22+m_xform.eDy);
}

void TScreenItemsViewControlState::ClientPointToAppSpacePoint(POINT *cln_point, POINT *app_point)
{
	app_point->x = (long)((cln_point->x-m_xform.eDx)/m_xform.eM11);
	app_point->y = (long)((cln_point->y-m_xform.eDy)/m_xform.eM22);
}

void TScreenItemsViewControlState::SendNewScrollPosNotification()
{
	TScreenItemsViewNotify info;
	memset(&info, 0, sizeof(info));
	info.pos_x = m_space.scroll_pos.x;
	info.pos_y = m_space.scroll_pos.y;
	SendNotification(xiv_new_scroll_pos, &info);
}


