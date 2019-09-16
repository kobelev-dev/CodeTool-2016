//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <math.h>
#include  <windows.h>
#include  <gdiplus.h>
#include  <assert.h>

#pragma hdrstop

#include  "WinUi/BitmapHelper.H"
#include  "ScreenItems/ScreenItems.H"

// ------------------------------------------------------------------------
//  ==================  TScreenItemStyle  =======================
// ------------------------------------------------------------------------

bool TScreenItemStyle::SetupForDeviceContextEx(HWND hWnd)
{
	HDC hDC = ::GetDC(hWnd);
	if (hDC == NULL)
		return(FALSE);

	// Call the virtual function. It will do the job.
	SetupForDeviceContext(hDC);

	::ReleaseDC(hWnd, hDC);
	return(TRUE);
}

// --------------------------------------------------------------------------
//  ====================  TScreenItem  ==========================
// --------------------------------------------------------------------------

void TScreenItem::SummarizeScreenItems(TScreenItemSummarizeInfo &info, bool bounding_rect_mode)
{
	// This method is called when the clicked point is inside the bounding rect of the screen item.
	// In the less accurate bounding rect mode, the item itself should be added to the summary
	// without additional checks. Otherwise ensure that the passed point does not belong to
	// the transparent part of the screen item.
	TScreenItem *scr_item = (bounding_rect_mode == TRUE) ? this : CheckClick(info.app_pt);

	if (scr_item != NULL)
		info.AddToSummary(scr_item);
}

bool TScreenItem::IsPointInRect(POINT &point, RECT &rect)
{
	if (rect.left == rect.right || rect.top == rect.bottom)
		return(FALSE);

	if (point.x < rect.left || point.x >= rect.right)
		return(FALSE);
	if (point.y < rect.top || point.y >= rect.bottom)
		return(FALSE);

	return(TRUE);
}

bool TScreenItem::CheckIntersection(RECT &rect1, RECT &rect2)
{
	// Check the vert direction first. Veritical direction tends to be bigger.
	long dpy = rect1.top-rect2.top;
	if (dpy < 0)
	{
		if (rect1.bottom-rect1.top <= -dpy)
			return(FALSE);
	}
	else
	{
		if (rect2.bottom-rect2.top <= dpy)
			return(FALSE);
	}

	// Vertical intersection is present. Check the horizontal direction.
	long dpx = rect1.left-rect2.left;
	if (dpx < 0)
	{
		if (rect1.right-rect1.left <= -dpx)
			return(FALSE);
	}
	else
	{
		if (rect2.right-rect2.left <= dpx)
			return(FALSE);
	}

	// Intersection is present in both directions.
	return(TRUE);
}

void TScreenItem::CombineRects(RECT &rect1, RECT &rect2)
{
	if (rect2.left < rect1.left)
		rect1.left = rect2.left;
	if (rect2.right > rect1.right)
		rect1.right = rect2.right;

	if (rect2.top < rect1.top)
		rect1.top = rect2.top;
	if (rect2.bottom > rect1.bottom)
		rect1.bottom = rect2.bottom;
}

// -----------------------------------------------------------------
//   =================  TBasicStyle  ======================
// -----------------------------------------------------------------

void TBasicStyle::BaseRectToBoundingRect(RECT &rc) const
{
	if (m_props.shadow_up_width > 0 || m_props.frame_offset < 0)
	{
		long lt_offs = __max(m_props.shadow_up_width, -m_props.frame_offset);
		rc.left -= lt_offs;
		rc.top  -= lt_offs;
	}

	if (m_props.shadow_down_width > 0 || m_props.frame_offset < 0)
	{
		long rb_offs = __max(m_props.shadow_down_width, -m_props.frame_offset);
		rc.right  += rb_offs;
		rc.bottom += rb_offs;
	}
}

void TBasicStyle::BoundingRectPosToBaseRectPos(long &px, long &py) const
{
	if (m_props.shadow_up_width > 0 || m_props.frame_offset < 0)
	{
		long lt_offs = __max(m_props.shadow_up_width, -m_props.frame_offset);
		px += lt_offs;
		py += lt_offs;
	}
}

void TBasicStyle::PaintSpecialDot(HDC hDC, float dot_x, float dot_y, TColor dot_color, int dot_diameter) const
{
	if (dot_diameter == 1)
	{
		// Single pixel dot without dithering.
		::SetPixel(hDC, (long)dot_x, (long)dot_y, dot_color);
	}
	else
	{
		// GdiPlus procedure.
		::Gdiplus::Graphics graphics(hDC);
		graphics.SetSmoothingMode(::Gdiplus::SmoothingModeAntiAlias);
		::Gdiplus::Color frameColor;
		frameColor.SetFromCOLORREF(dot_color);
		::Gdiplus::SolidBrush frameBrush(frameColor);

		float offs = (float)((dot_diameter+1)/2);
		graphics.FillEllipse(&frameBrush, dot_x-offs, dot_y-offs, 2*offs, 2*offs);
	}
}

// ---------------------------------------------------------------------
//   ====================  TRectItem  =======================
// ---------------------------------------------------------------------

void TRectItem::SetStyle(TScreenItemStyle *new_style)
{
	if (new_style != NULL)
	{
		assert(new_style->StyleTypeId() == TBasicStyle::type_ID);
	}

	m_style = new_style;
	if (m_style != NULL)
	{
		UpdateRcItem();
	}
}

void TRectItem::SetBoundingRectPos(long pos_x, long pos_y)
{
	if (m_style != NULL)
		Style()->BoundingRectPosToBaseRectPos(pos_x, pos_y);
	SetHotSpotPos(pos_x, pos_y);
}

void TRectItem::SetHotSpotPos(long new_x, long new_y)
{
	RECT new_rc = {  new_x, new_y, new_x+(m_base_rect.right-m_base_rect.left), new_y+(m_base_rect.bottom-m_base_rect.top) };
	SetBaseRect(new_rc);
}

void TRectItem::ShiftItem(long shift_x, long shift_y)
{
	m_base_rect.left += shift_x;
	m_base_rect.right += shift_x;
	m_base_rect.top += shift_y;
	m_base_rect.bottom += shift_y;
	UpdateRcItem();
}

void TRectItem::OnDraw(HDC hDC, RECT &rc_invalid)
{
	assert(m_style != NULL);
	const TBasicStyleProps &props = Style()->m_props;
	RECT rcp = m_base_rect;

	if (props.shadow_up_width > 0 && props.shadow_up_color != cs_transparent)
	{
		// Properties require the upper shadow.
		ShiftRect(rcp, -props.shadow_up_width);
		HBRUSH shadowUpBrush = ::CreateSolidBrush(props.shadow_up_color);
		RECT rc1 = { rcp.left, rcp.top, rcp.right, rcp.top+props.shadow_up_width };
		::FillRect(hDC, &rc1, shadowUpBrush);
		RECT rc2 = { rcp.left, rcp.top, rcp.left+props.shadow_up_width, rcp.bottom };
		::FillRect(hDC, &rc2, shadowUpBrush);
		::DeleteObject(shadowUpBrush);
		ShiftRect(rcp, props.shadow_up_width);
	}

	if (props.shadow_down_width > 0 && props.shadow_down_color != cs_transparent)
	{
		// Properties require the lower shadow.
		ShiftRect(rcp, props.shadow_down_width);
		HBRUSH shadowDownBrush = ::CreateSolidBrush(props.shadow_down_color);
		RECT rc1 = { rcp.right-props.shadow_down_width, rcp.top, rcp.right, rcp.bottom };
		::FillRect(hDC, &rc1, shadowDownBrush);
		RECT rc2 = { rcp.left, rcp.bottom-props.shadow_down_width, rcp.right, rcp.bottom };
		::FillRect(hDC, &rc2, shadowDownBrush);
		::DeleteObject(shadowDownBrush);
		ShiftRect(rcp, -props.shadow_down_width);
	}

	if (props.background_color != cs_transparent)
	{
		// Draw the body of the rect.
		HBRUSH backBrush = ::CreateSolidBrush(props.background_color);
		::FillRect(hDC, &rcp, backBrush);
		::DeleteObject(backBrush);
	}

	// Draw the frame inside of the rect.
	if (props.frame_width > 0)
	{
		// Style requests a non empty frame.
		RECT rcFrame =
		{
			rcp.left + props.frame_offset, rcp.top + props.frame_offset,
			rcp.right - props.frame_offset, rcp.bottom - props.frame_offset
		};

		if (props.frame_shadow_up_width > 0)
		{
			ShiftRect(rcFrame, -props.frame_shadow_up_width);
			PaintSimpleFrame(hDC, rcFrame, props.frame_shadow_up_width,
			props.frame_shadow_up_color);
			ShiftRect(rcFrame, props.frame_shadow_up_width);
		}

		if (props.frame_shadow_down_width > 0)
		{
			ShiftRect(rcFrame, props.frame_shadow_down_width);
			PaintSimpleFrame(hDC, rcFrame, props.frame_shadow_down_width,
			props.frame_shadow_down_color);
			ShiftRect(rcFrame, -props.frame_shadow_down_width);
		}

		// Paint the frame itself.
		if (props.frame_style == PS_DOT)
		{
			// Paint the special dotted frame.
			float frame_width = (float)props.frame_width;

			long horz_span = rcFrame.right-rcFrame.left-props.frame_width/2;
			int num_horz_dots = horz_span/3;
			if (num_horz_dots <= 0)
				num_horz_dots = 1;

			long vert_span = rcFrame.bottom-rcFrame.top-props.frame_width/2;
			int num_vert_dots = vert_span/3;
			if (num_vert_dots <= 0)
				num_vert_dots = 1;

			float horz_step = ((float)horz_span)/num_horz_dots;
			float horz_pos1 = rcFrame.left+frame_width/4;
			float horz_pos2 = rcFrame.right-1-frame_width/4;
			for (int ihorz=0; ihorz<num_horz_dots; ++ihorz, horz_pos1 += horz_step, horz_pos2 -= horz_step)
			{
				Style()->PaintSpecialDot(hDC, horz_pos1, rcFrame.top+frame_width/4, props.frame_color, props.frame_width);
				Style()->PaintSpecialDot(hDC, horz_pos2, rcFrame.bottom-1-frame_width/4, props.frame_color, props.frame_width);
			}

			float vert_step = ((float)vert_span)/num_vert_dots;
			float vert_pos1 = rcFrame.top+frame_width/4;
			float vert_pos2 = rcFrame.bottom-1-frame_width/4;
			for (int ivert=0; ivert<num_vert_dots; ++ivert, vert_pos1 += vert_step, vert_pos2 -= vert_step)
			{
				Style()->PaintSpecialDot(hDC, rcFrame.right-1-frame_width/4, vert_pos1, props.frame_color, props.frame_width);
				Style()->PaintSpecialDot(hDC, rcFrame.left+frame_width/4, vert_pos2, props.frame_color, props.frame_width);
			}
		}
		else
		{
			// Paint the regular frame.
			PaintSimpleFrame(hDC, rcFrame, props.frame_width, props.frame_color);
		}
	}
}

void TRectItem::PaintSimpleFrame(HDC hDC, RECT &rc, long width, COLORREF color)
{
	HBRUSH frmBrush = ::CreateSolidBrush(color);

	RECT rc1 = { rc.left, rc.top, rc.right, rc.top+width };
	RECT rc2 = { rc.left, rc.top, rc.left+width, rc.bottom };
	RECT rc3 = { rc.left, rc.bottom-width, rc.right, rc.bottom };
	RECT rc4 = { rc.right-width, rc.top, rc.right, rc.bottom };

	::FillRect(hDC, &rc1, frmBrush);
	::FillRect(hDC, &rc2, frmBrush);
	::FillRect(hDC, &rc3, frmBrush);
	::FillRect(hDC, &rc4, frmBrush);
	::DeleteObject(frmBrush);
}

// -------------------------------------------------------------------
//   ===================  TCircleItem  ======================
// -------------------------------------------------------------------

static LOGBRUSH g_nullLogBrush = { BS_NULL, 0, 0 };

void TCircleItem::SetStyle(TScreenItemStyle* new_style)
{
	if (new_style != NULL)
	{
		assert(new_style->StyleTypeId() == TBasicStyle::type_ID);
	}

	m_style = new_style;
	if (m_style != NULL)
	{
		SetupRcItem(m_bounding_rect);
		Style()->BaseRectToBoundingRect(m_bounding_rect);
		m_bounding_rect.right++;
		m_bounding_rect.bottom++;
	}
}

void TCircleItem::SetPosition(long x, long y, long new_diam)
{
	m_center_x = x;
	m_center_y = y;
	if (new_diam >= 0)
		m_diameter = new_diam;

	SetupRcItem(m_bounding_rect);
	if (m_style != NULL)
	{
		Style()->BaseRectToBoundingRect(m_bounding_rect);
		m_bounding_rect.right++;
		m_bounding_rect.bottom++;
	}
}

void TCircleItem::SetBoundingRectPos(long x, long y)
{
	if (m_style != NULL)
		Style()->BoundingRectPosToBaseRectPos(x, y);
	SetPosition(x+m_diameter/2, y+m_diameter/2, -1);
}

void TCircleItem::OnDraw(HDC hDC, RECT &rc_invalid)
{
	assert(m_style != NULL);
	TBasicStyleProps &props = Style()->m_props;
	HPEN nullPen = ::CreatePen(PS_NULL, 0, 0);
	HBRUSH nullBrush = ::CreateBrushIndirect(&g_nullLogBrush);
	HPEN oldPen;
	HBRUSH oldBrush;
	RECT rc_paint;
	SetupRcItem(rc_paint);

	if (props.shadow_up_width > 0 && props.shadow_up_color != cs_transparent)
	{
		// Properties require the upper shadow.
		HBRUSH shadowUpBrush = ::CreateSolidBrush(props.shadow_up_color);
		oldPen = (HPEN)::SelectObject(hDC, nullPen);
		oldBrush = (HBRUSH)::SelectObject(hDC, shadowUpBrush);
		::Ellipse(hDC, rc_paint.left - props.shadow_up_width,
				rc_paint.top - props.shadow_up_width,
				rc_paint.right - props.shadow_up_width,
				rc_paint.bottom - props.shadow_up_width);

		::SelectObject(hDC, oldBrush);
		::SelectObject(hDC, oldPen);
		::DeleteObject(shadowUpBrush);
	}

	if (props.shadow_down_width > 0 && props.shadow_down_color != cs_transparent)
	{
		// Properties require the lower shadow.
		HBRUSH shadowDownBrush = ::CreateSolidBrush(props.shadow_down_color);
		oldPen = (HPEN)::SelectObject(hDC, nullPen);
		oldBrush = (HBRUSH)::SelectObject(hDC, shadowDownBrush);
		::Ellipse(hDC, rc_paint.left + props.shadow_down_width,
				rc_paint.top + props.shadow_down_width,
				rc_paint.right + props.shadow_down_width,
				rc_paint.bottom + props.shadow_down_width);

		::SelectObject(hDC, oldBrush);
		::SelectObject(hDC, oldPen);
		::DeleteObject(shadowDownBrush);
	}

	// Draw the body of the circle only when the color is not transparent.
	if (props.background_color != cs_transparent)
	{
		HBRUSH backBrush = ::CreateSolidBrush(props.background_color);
		oldPen = (HPEN)::SelectObject(hDC, nullPen);
		oldBrush = (HBRUSH)::SelectObject(hDC, backBrush);
		::Ellipse(hDC, rc_paint.left, rc_paint.top, rc_paint.right, rc_paint.bottom);

		::SelectObject(hDC, oldBrush);
		::SelectObject(hDC, oldPen);
		::DeleteObject(backBrush);
	}

	// Draw the frame.
	if (props.frame_width > 0)
	{
		//
		//  Style asks for the non empty frame.
		//

		// Frame can be dashed, dotted, etc, according to the props.
		if (props.frame_style == PS_DOT)
		{
			// Special painting procedure. Place centers of the dots in 3 pixels from one another.
			int num_beams = (int)(3.1415*m_diameter/3);
			num_beams &= ~3;
			double curr_angle = 6.2832/4;

			for (int bi=0; bi<num_beams; ++bi, curr_angle += 6.2832/num_beams)
			{
				float dot_x = m_center_x + (float)(m_diameter*cos(curr_angle)/2);
				float dot_y = m_center_y + (float)(m_diameter*sin(curr_angle)/2);
				Style()->PaintSpecialDot(hDC, dot_x, dot_y, props.frame_color, props.frame_width);
			}
		}
		else
		{
			// Simple frame using the GdiPlus procedure.
			::Gdiplus::Graphics graphics(hDC);
			graphics.SetSmoothingMode(::Gdiplus::SmoothingModeAntiAlias);
			::Gdiplus::Color frameColor;
			frameColor.SetFromCOLORREF(props.frame_color);
			::Gdiplus::Pen framePen(frameColor, props.frame_width-(float)0.4);
			graphics.DrawEllipse(&framePen, rc_paint.left, rc_paint.top, rc_paint.right-rc_paint.left, rc_paint.bottom-rc_paint.top);

#define WANT_DOUBLE_CIRC_FRAME
#ifdef WANT_DOUBLE_CIRC_FRAME
			if (props.frame_style == PS_DASH)
			{
				frameColor.SetFromCOLORREF(RGB(64, 64, 64));
				::Gdiplus::Pen framePen(frameColor, 1);
				graphics.DrawEllipse(&framePen, rc_paint.left-1, rc_paint.top-1, rc_paint.right-rc_paint.left+2, rc_paint.bottom-rc_paint.top+2);
			}
#endif
		}
	}

	::DeleteObject(nullPen);
	::DeleteObject(nullBrush);
}

TScreenItem *TCircleItem::CheckClick(POINT &app_pt)
{
	// Execute fast check based on the bounding rect first.
	if (IsPointInRect(app_pt, m_bounding_rect) == FALSE)
		return(NULL);

	// Check the distance from the center.
	long dx = app_pt.x-m_center_x;
	long dy = app_pt.y-m_center_y;
	if (dx*dx+dy*dy <= m_diameter*m_diameter/4+1)
		return(this);

	// Passed point is outside of the circle.
	return(NULL);
}

// --------------------------------------------------------------------
//   ===================  TBitmapStyle  ======================
// --------------------------------------------------------------------

bool TBitmapStyle::Setup(HINSTANCE hInst, const TBitmapStyleProps &props)
{
	m_paint_hlpr.Release();
	m_click_hlpr.Release();
	if (m_hBmp != NULL)
		::DeleteObject(m_hBmp);

	m_hInst = (hInst != NULL) ? hInst : (::GetModuleHandle(NULL));
	m_icons_grid_mode = FALSE;
	m_sb_props = props;

	m_hBmp = ::LoadBitmap(m_hInst, MAKEINTRESOURCE(props.resource_id));
	if (m_hBmp == NULL)
	{
		// Bitmap is missing in the resources.
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TBitmapStyle::Setup(HINSTANCE hInst, const TIconsGridStyleProps &props)
{
	m_paint_hlpr.Release();
	m_click_hlpr.Release();
	if (m_hBmp != NULL)
		::DeleteObject(m_hBmp);

	m_hInst = (hInst != NULL) ? hInst : (::GetModuleHandle(NULL));
	m_icons_grid_mode = TRUE;
	m_ic_props = props;

	m_hBmp = ::LoadBitmap(m_hInst, MAKEINTRESOURCE(props.resource_id));
	if (m_hBmp == NULL)
	{
		// Bitmap is missing in the resources.
		return(FALSE);
	}

	// Success.
	return(TRUE);
}

bool TBitmapStyle::SetupForDeviceContext(HDC hDC)
{
	assert(m_hBmp != NULL);

	TScreenItemStyle::SetupForDeviceContext(hDC);
	m_paint_hlpr.Initialize(hDC, m_hBmp, 1.0, (IsIconsGrid() == FALSE) ? m_sb_props.transp_pixels :  m_ic_props.transp_pixels);
	m_paint_hlpr.ClearTempDC(m_ic_props.x_cell, m_ic_props.y_cell);
	m_click_hlpr.Initialize(hDC, m_hBmp, 1.0);

	m_prev_pos_x = m_prev_pos_y = 0;
	return(TRUE);
}

void TBitmapStyle::GetCellSize(SIZE &cell_size)
{
	if (IsIconsGrid() == FALSE)
	{
		// Return the size of the whole bitmap.
		BITMAP bmp_info;
		::GetObject(m_hBmp, sizeof(BITMAP), &bmp_info);
		cell_size.cx = bmp_info.bmWidth;
		cell_size.cy = bmp_info.bmHeight;
	}
	else
	{
		// Return the cell size.
		cell_size.cx = m_ic_props.x_cell;
		cell_size.cy = m_ic_props.y_cell;
	}
}

void TBitmapStyle::PaintBitmap(HDC hDC, POINT &app_item_pos, short icn_inx, POINT &local_pos, SIZE &local_size)
{
	assert(m_paint_hlpr.IsInited() == TRUE);

	// This is a trick to fix the printing temorarily. This creates an illusion that printer DC is readable.
	if (app_item_pos.x != m_prev_pos_x || app_item_pos.y != m_prev_pos_y)
	{
		m_paint_hlpr.ClearTempDC(m_ic_props.x_cell, m_ic_props.y_cell);
		m_prev_pos_x = app_item_pos.x;
		m_prev_pos_y = app_item_pos.y;
	}

	if (IsIconsGrid() == FALSE)
	{
		// Single bitmap case.
		m_paint_hlpr.Paint(hDC, local_pos.x, local_pos.y,					// Pos on the bitmap.
						local_size.cx, local_size.cy,					// Size of the bitmap.
						app_item_pos.x-m_sb_props.hot_spot_x,			// x-pos on the screen.
						app_item_pos.y-m_sb_props.hot_spot_y);		// y-pos on the screen.
	}
	else
	{
		// Calculate position of the icon on the bitmap.
		int bmp_x = (icn_inx%100)*(m_ic_props.x_step)+m_ic_props.x_base+local_pos.x;
		int bmp_y = (icn_inx/100-1)*(m_ic_props.y_step)+m_ic_props.y_base+local_pos.y;

		// Do the painting.
		m_paint_hlpr.Paint(hDC, bmp_x, bmp_y,									// Pos on the bitmap.
						local_size.cx, local_size.cy,							// Size on the bitmap.
						app_item_pos.x-m_ic_props.x_cell/2+local_pos.x,		// x-pos on the screen.
						app_item_pos.y-m_ic_props.y_cell/2+local_pos.y);		// y-pos on the screen.
	}
}

void TBitmapStyle::PaintBitmapEx(HDC hDC, POINT &left_top_bitmap_pos, short icn_inx, POINT &local_pos, SIZE &local_size)
{
	assert(m_paint_hlpr.IsInited() == TRUE);

	if (IsIconsGrid() == FALSE)
	{
		// Single bitmap case.
		m_paint_hlpr.Paint(hDC, local_pos.x, local_pos.y,		// Pos on the bitmap.
						local_size.cx, local_size.cy,		// Size of the bitmap.
						left_top_bitmap_pos.x,			// x-pos on the screen.
						left_top_bitmap_pos.y);			// y-pos on the screen.
	}
	else
	{
		// Calculate position of the icon on the bitmap.
		int bmp_x = (icn_inx%100)*(m_ic_props.x_step)+m_ic_props.x_base+local_pos.x;
		int bmp_y = (icn_inx/100-1)*(m_ic_props.y_step)+m_ic_props.y_base+local_pos.y;

		// Do the painting.
		m_paint_hlpr.Paint(hDC, bmp_x, bmp_y,				// Pos on the bitmap.
						local_size.cx, local_size.cy,		// Size on the bitmap.
						left_top_bitmap_pos.x,			// x-pos on the screen.
						left_top_bitmap_pos.y);			// y-pos on the screen.
	}
}

bool TBitmapStyle::CheckClick(POINT &click_pos, POINT &app_item_pos, short icn_inx, SIZE &local_size)
{
	assert(m_click_hlpr.IsInited() == TRUE);

	if (IsIconsGrid() == FALSE)
	{
		// Single bitmap case.
		int off_x = click_pos.x-app_item_pos.x+m_sb_props.hot_spot_x;
		if (off_x < 0 || off_x >= local_size.cx)
			return(FALSE);

		int off_y = click_pos.y-app_item_pos.y+m_sb_props.hot_spot_y;
		if (off_y < 0 || off_y >= local_size.cy)
			return(FALSE);

		return(m_click_hlpr.CheckClick(off_x, off_y));
	}
	else
	{
		assert(icn_inx > 0);
		int cell_app_pos_x = app_item_pos.x-m_ic_props.x_cell/2;
		int cell_app_pos_y = app_item_pos.y-m_ic_props.y_cell/2;

		// Check that passed point fits into the size of the cell.
		if (click_pos.x < cell_app_pos_x || click_pos.x >= cell_app_pos_x+m_ic_props.x_cell)
			return(FALSE);
		if (click_pos.y < cell_app_pos_y || click_pos.y >= cell_app_pos_y+m_ic_props.y_cell)
			return(FALSE);

		// Calculate the position of the cell on the bitmap.
		int bmp_x = (icn_inx%100)*(m_ic_props.x_step)+m_ic_props.x_base;
		int bmp_y = (icn_inx/100-1)*(m_ic_props.y_step)+m_ic_props.y_base;

		// Check the value of one bitmap pixel. Click hepler does not need to know the size of the icon
		// becase this was already checked above.
		return(m_click_hlpr.CheckClick(bmp_x+(click_pos.x-cell_app_pos_x), bmp_y+(click_pos.y-cell_app_pos_y)));
	}
}

void TBitmapStyle::GetLocalRect(short icon_index, POINT &icon_pos, SIZE &icon_size)
{
	assert(m_hBmp != NULL);

	if (IsIconsGrid() == FALSE)
	{
		// Return rect of the whole bitmap.
		BITMAP bmp_info;
		::GetObject(m_hBmp, sizeof(BITMAP), &bmp_info);
		icon_pos.x = icon_pos.y = 0;
		icon_size.cx = bmp_info.bmWidth;
		icon_size.cy = bmp_info.bmHeight;
	}
	else if (m_ic_props.frame_exts_avail == FALSE)
	{
		// Return the rect for the whole cell because info for individual icons is not available.
		icon_pos.x = icon_pos.y = 0;
		icon_size.cx = m_ic_props.x_cell;
		icon_size.cy = m_ic_props.y_cell;
	}
	else
	{
		// Look for the bounding rect on the frame of the cell. Non black pixels on the icon border
		// show that area that does not belong to the icon.
		assert(icon_index > 0);
		int bmp_x = (icon_index%100)*(m_ic_props.x_step)+m_ic_props.x_base;
		int bmp_y = (icon_index/100-1)*(m_ic_props.y_step)+m_ic_props.y_base;

		int xOn = -1, xOff = -1;
		for (int x=0; x<m_ic_props.x_cell; ++x)
		{
			bool isBlack = m_click_hlpr.CheckBorder(bmp_x+x, bmp_y-1);
			if (xOn == -1 && isBlack == TRUE)
				xOn = x;
			if (xOn != -1 && isBlack != TRUE)
			{
				xOff = x;
				break;
			}
		}

		int yOn = -1, yOff = -1;
		for (int y=0; y<m_ic_props.y_cell; ++y)
		{
			bool isBlack = m_click_hlpr.CheckBorder(bmp_x-1, bmp_y+y);
			if (yOn == -1 && isBlack == TRUE)
				yOn = y;
			if (yOn != -1 && isBlack != TRUE)
			{
				yOff = y;
				break;
			}
		}

		// Sum up the picked info.
		icon_pos.x = (xOn < 0) ? 0 : xOn;
		icon_pos.y = (yOn < 0) ? 0 : yOn;
		icon_size.cx = ((xOff < 0) ? m_ic_props.x_cell : xOff)-icon_pos.x;
		icon_size.cy = ((yOff < 0) ? m_ic_props.y_cell : yOff)-icon_pos.y;
	}
}

void TBitmapStyle::PrepareItemRect(RECT &rect, POINT &app_item_pos, POINT &icon_pos, SIZE &icon_size)
{
	if (IsIconsGrid() == FALSE)
	{
		rect.left = app_item_pos.x-m_sb_props.hot_spot_y;
		rect.top = app_item_pos.y-m_sb_props.hot_spot_y;
	}
	else
	{
		rect.left = app_item_pos.x-m_ic_props.x_cell/2+icon_pos.x;
		rect.top = app_item_pos.y-m_ic_props.y_cell/2+icon_pos.y;
	}

	rect.right  = rect.left+icon_size.cx;
	rect.bottom = rect.top+icon_size.cy;
}

// --------------------------------------------------------------------
//   ===================  TBitmapItem  ======================
// --------------------------------------------------------------------

void TBitmapItem::SetStyle(TScreenItemStyle* new_style)
{
	if (new_style != NULL)
	{
		// Ensure that passed style has correct type.
		assert(new_style->StyleTypeId() == TBitmapStyle::type_ID);
	}

	m_style = new_style;
	UpdateDerivedFields();
}

void TBitmapItem::SetPosition(long x, long y, short icn_inx)
{
	m_base_pos.x = x;
	m_base_pos.y = y;

	if (icn_inx >= 0)
	{
		SetIconIndex(icn_inx);
	}
	else if (m_style != NULL && m_icon_local_pos.x != -1 && m_icon_local_pos.y != -1)
	{
		// Style and icon index (if needed) are available. Update the bounding rect.
		Style()->PrepareItemRect(m_bounding_rect, m_base_pos, m_icon_local_pos, m_icon_local_size);
	}
}

void TBitmapItem::SetLeftAlignedPosition(long x_left, long y_hot_spot, short icn_inx)
{
	assert(m_style != NULL);

	POINT local_icon_pos;
	SIZE cell_size, local_icon_size;
	Style()->GetCellSize(cell_size);
	Style()->GetLocalRect(icn_inx, local_icon_pos, local_icon_size);
	SetPosition(x_left+(cell_size.cx/2-local_icon_pos.x)+1, y_hot_spot, icn_inx);
}

void TBitmapItem::SetIconIndex(short icn_inx)
{
	if (icn_inx < 0)
		return;

	m_icon_index = icn_inx;
	UpdateDerivedFields();
}

void TBitmapItem::SetBoundingRectPos(long x, long y)
{
	assert(m_style != NULL);
	assert(m_icon_local_pos.x != -1 && m_icon_local_pos.y != -1);

	m_base_pos.x = x-m_icon_local_pos.x+Style()->m_ic_props.x_cell/2;
	m_base_pos.y = y-m_icon_local_pos.y+Style()->m_ic_props.y_cell/2;
	Style()->PrepareItemRect(m_bounding_rect, m_base_pos, m_icon_local_pos, m_icon_local_size);
}

void TBitmapItem::OnDraw(HDC hDC, RECT &rc_invalid)
{
	assert(m_style != NULL);
	assert(m_icon_local_pos.x != -1 && m_icon_local_pos.y != -1);

	Style()->PaintBitmap(hDC, m_base_pos, m_icon_index, m_icon_local_pos, m_icon_local_size);
}

TScreenItem *TBitmapItem::CheckClick(POINT &app_point)
{
	assert(m_style != NULL);
	assert(m_icon_local_pos.x != -1 && m_icon_local_pos.y != -1);

	return((Style()->CheckClick(app_point, m_base_pos, m_icon_index, m_icon_local_size) == TRUE) ? this : NULL);
}

void TBitmapItem::UpdateDerivedFields()
{
	// This method expects that style or the icon index has really changed.
	if (m_style == NULL || Style()->IsIconsGrid() == TRUE && m_icon_index == 0)
	{
		// There is no style or the icon index is missing. Clear the secondary fields.
		m_icon_local_pos.x = m_icon_local_pos.y = -1;
		memset(&m_bounding_rect, 0, sizeof(RECT));
	}
	else
	{
		// Retrieve boundaries of the icon inside its cell or boundaries of the whole bitmap.
		Style()->GetLocalRect(m_icon_index, m_icon_local_pos, m_icon_local_size);

		// Fill in the bounding app space rect.
		Style()->PrepareItemRect(m_bounding_rect, m_base_pos, m_icon_local_pos, m_icon_local_size);
	}
}

// ----------------------------------------------------------------------
//   ==================  TLinesChainItem  ======================
// ----------------------------------------------------------------------

void TLinesChainItem::SetStyle(TScreenItemStyle* new_style)
{
	if (new_style != NULL)
	{
		// Ensure that passed style has correct type.
		assert(new_style->StyleTypeId() == TLineStyle::type_ID);
	}

	m_style = new_style;
	SetupRcItem();
}

void TLinesChainItem::SetPosition(long num_pts, POINT *data, TDataBypassMode bypass_mode)
{
	// Release the prev data set if any.
	if (m_points_data_owned == TRUE)
	{
		free(m_points_data);
		m_points_data_owned = FALSE;
	}

	m_num_points = 0;

	// Process the bypassed data set.
	if (num_pts > 1)
	{
		m_num_points = num_pts;
		switch (bypass_mode)
		{
			case dbm_use_only:
					{
						m_points_data = data;
					}
					break;

			case dbm_take_ownsh:
					{
						m_points_data = data;
						m_points_data_owned = TRUE;
					}
					break;

			case dbm_duplicate:
					{
						m_points_data = (POINT*)malloc(m_num_points*sizeof(POINT));
						if (m_points_data == NULL)
						{
							m_num_points = 0;
						}
						else
						{
							memcpy(m_points_data, data, m_num_points*sizeof(POINT));
							m_points_data_owned = TRUE;
						}
					}
					break;
		}
	}

	// Update the bounding rect of the item.
	SetupRcItem();
}

void TLinesChainItem::SetBoundingRectPos(long x, long y)
{
	if (m_num_points < 1)
		return;

	long new_x = x + (m_points_data->x - m_bounding_rect.left);
	long new_y = y + (m_points_data->y - m_bounding_rect.top);
	SetHotSpotPos(new_x, new_y);
}

void TLinesChainItem::SetHotSpotPos(long x, long y)
{
	if (m_num_points < 1)
		return;

	long sh_x = x - m_points_data->x;
	long sh_y = y - m_points_data->y;

	for (int inx = 0; inx < m_num_points; ++inx)
	{
		m_points_data[inx].x += sh_x;
		m_points_data[inx].y += sh_y;
	}

	ShiftRcItem(sh_x, sh_y);
}

void TLinesChainItem::GetHotSpotPos(long &px, long &py)
{
	px = (m_num_points > 0) ? m_points_data->x : 0;
	py = (m_num_points > 0) ? m_points_data->y : 0;
}

void TLinesChainItem::OnDraw(HDC hDC, RECT &rc_invalid)
{
	if (m_num_points < 2)
		return;

	assert(m_style != NULL);
	TLineStyleProps &props = Style()->m_props;

	if (props.line_width <= 1)
	{
		// Unfortunately GDI+ procedute is not nice for thin lines.
		HPEN pen = ::CreatePen(PS_SOLID, props.line_width, props.foreground_color);
		HPEN oldPen = (HPEN)::SelectObject(hDC, pen);

		::MoveToEx(hDC, m_points_data[0].x, m_points_data[0].y, NULL);

		for (int inx = 1; inx < m_num_points; ++inx)
			::LineTo(hDC, m_points_data[inx].x, m_points_data[inx].y);

		::SelectObject(hDC, oldPen);
		::DeleteObject(pen);
	}
	else
	{
		// Use GdiPlus procedure.
		::Gdiplus::Graphics graphics(hDC);
		graphics.SetSmoothingMode(::Gdiplus::SmoothingModeAntiAlias);
		::Gdiplus::Color lineColor;
		lineColor.SetFromCOLORREF(props.foreground_color);
		::Gdiplus::Pen linePen(lineColor, (float)props.line_width-(float)0.2);

		for (int inx = 0; inx < m_num_points-1; ++inx)
			graphics.DrawLine(&linePen, m_points_data[inx].x, m_points_data[inx].y, m_points_data[inx+1].x, m_points_data[inx+1].y);
	}
}

void TLinesChainItem::SetupRcItem()
{
	long offs = (m_style != NULL) ? (Style()->m_props.line_width/2) : 0;

	if (m_num_points > 0)
	{
		// There should be some non empty rect.
		m_bounding_rect.left = m_points_data[0].x-offs;
		m_bounding_rect.right = m_points_data[0].x+offs+1;
		m_bounding_rect.top = m_points_data[0].y-offs;
		m_bounding_rect.bottom = m_points_data[0].y+offs+1;

		for (int i = 1; i < m_num_points; i++)
		{
			long xt = m_points_data[i].x;
			long yt = m_points_data[i].y;

			if (xt-offs < m_bounding_rect.left)
				m_bounding_rect.left = xt-offs;
			if (yt-offs < m_bounding_rect.top)
				m_bounding_rect.top = yt-offs;
			if (xt+offs >= m_bounding_rect.right)
				m_bounding_rect.right = xt+offs+1;
			if (yt+offs >= m_bounding_rect.bottom)
				m_bounding_rect.bottom = yt+offs+1;
		}
	}
	else
	{
		// Clear the rect. Item is currently not placed anywhere.
		memset(&m_bounding_rect, 0, sizeof(RECT));
	}
}

// -----------------------------------------------------------------------
//   ===================  TTextStyleProps  ======================
// -----------------------------------------------------------------------

static const wchar_t *g_FrameCornerStyleText[fcs_num_types] =
{
	L"fcs_none",
	L"fcs_1pix_simple",
	L"fcs_1pix_excl_tip",
	L"fcs_1pix_add_inner_dot",
	L"fcs_1pix_excl_tip_add_inner_dot",
};

void TTextStyleProps::Init(const wchar_t *fn, int fh, bool ib, TColor frgr_color, TColor bkgr_color)
{
	Clear();
	frame_color = cs_transparent;

	wcscpy(font_name, fn);
	font_height = fh;
	is_bold = ib;

	background_color = bkgr_color;
	foreground_color = frgr_color;
}

wchar_t *TTextStyleProps::GetFrameCornerStyleText(TFrameCornerStyle style, wchar_t *buff_40_chars)
{
	if (style >= 0 && style < fcs_num_types)
		return((wchar_t*)g_FrameCornerStyleText[style]);

	swprintf(buff_40_chars, 40, L"BogusVal%d", style);
	return(buff_40_chars);
}

bool TTextStyleProps::GetFrameCornerStyleFromText(TStringPtr &data, TFrameCornerStyle &style)
{
	for (int inx = 0; inx < fcs_num_types; ++inx)
	{
		if (data == g_FrameCornerStyleText[inx])
		{
			style = (TFrameCornerStyle)inx;
			return(TRUE);
		}
	}

	return(FALSE);
}

// --------------------------------------------------------------------
//   ===================  TTextStyle  =======================
// --------------------------------------------------------------------

void TTextStyle::SetupSymbolsAdjustData(const TTextStyleSymbolAdjustInfo *data)
{
	// The adjustment data is never owned. Simply overwrite the old value.
	m_symbols_adjust_data = (TTextStyleSymbolAdjustInfo*)data;

	if (data != NULL)
	{
		// The new adjustment data is not empty.
		memset(m_symbols_adjust_lookup, 0, sizeof(m_symbols_adjust_lookup));
		while (data->symbol != 0)
		{
			if (data->symbol > 0 && data->symbol < 128)
				m_symbols_adjust_lookup[data->symbol] = (short)(data-m_symbols_adjust_data)+0x8000;
			data++;
		}
	}
}

bool TTextStyle::SetupForDeviceContext(HDC hDC)
{
	assert(hDC != NULL);
	assert(m_props.font_name[0] != 0 && m_props.font_height != 0);
	TScreenItemStyle::SetupForDeviceContext(hDC);

	if (m_hFont != NULL)
		::DeleteObject(m_hFont);

	int log_height = ::MulDiv(m_props.font_height, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
	m_hFont = ::CreateFontW(
					-log_height,						// nHeight
					0,								// nWidth
					0,								// nEscapament
					0,								// nOrientation
					(m_props.is_bold == TRUE) ? FW_BOLD : FW_NORMAL,    // fnWeight
					FALSE,							// fdwItalic
					FALSE,							// fdwUnderline
					FALSE,							// fdwStrikeOut
					DEFAULT_CHARSET,
					OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY,
					FF_DONTCARE | DEFAULT_PITCH,
					m_props.font_name);
	if (m_hFont == NULL)
		return(FALSE);

	// Retrieve the props of the created font.
	TEXTMETRIC font_metr;
	HFONT oldFont = (HFONT)::SelectObject(hDC, m_hFont);
	::GetTextMetrics(hDC, &font_metr);
	::SelectObject(hDC, oldFont);

	// Step1. Figure out the params of the font.
	m_height_above = font_metr.tmAscent;
	m_height_below = font_metr.tmHeight - m_height_above;
	m_basic_char_width = font_metr.tmAveCharWidth;

	// Step 2. Apply the baseline and the vert adjustments.
	m_height_above += m_props.up_side_adjust;
	m_height_below += m_props.down_side_adjust;

	// Step 3. Apply the shadow.
	m_height_below += m_props.shadow_down_width;

	return(TRUE);
}

long TTextStyle::GetStringWidth(HDC hDC, const wchar_t *str, long length, long *string_height, bool block_front_ext, bool block_back_ext) const
{
	if (string_height != NULL)
	{
		// Return the font info. Height of the string does not depend on the string contents.
		*string_height = m_height_above + m_height_below;
	}

	char fr_side_ext = (block_front_ext == FALSE) ? m_props.front_side_ext : 0;
	char bk_side_ext = (block_back_ext == FALSE) ? m_props.back_side_ext : 0;

	if (length == 0)
		return(fr_side_ext+bk_side_ext);
	if (length < 0)
		length = (long)wcslen(str);

	SIZE text_size;
	HFONT oldFont = (HFONT)::SelectObject(hDC, m_hFont);
	long string_width = 0;
	if (m_symbols_adjust_data == NULL)
	{
		// Simple case.
		::GetTextExtentPoint32W(hDC, str, length, &text_size);
		string_width = text_size.cx;
	}
	else
	{
		while (length > 0)
		{
			// Search for the perfix that does not need adjustments.
			TTextStyleSymbolAdjustInfo *sa_data;
			int len = GetSimplePrefix(str, length, sa_data);

			if (len > 0)
			{
				::GetTextExtentPoint32W(hDC, str, len, &text_size);
				string_width += text_size.cx;

				// Remove the prefix from the source string.
				str += len;
				length -= len;
			}

			if (sa_data != NULL)
			{
				::GetTextExtentPoint32W(hDC, str, 1, &text_size);
				string_width += text_size.cx;
				string_width += sa_data->width_delta;

				// Remove curr char from the source string.
				str += 1;
				length -= 1;
			}
		}
	}

	::SelectObject(hDC, oldFont);
	return(fr_side_ext + string_width + m_props.shadow_down_width + bk_side_ext);
}

long TTextStyle::GetFittedLen(HDC hDC, long width_avail, const wchar_t *str, long length, bool *add_dots_if_truncating)
{
	if (length < 0)
		length = (long)wcslen(str);

	if (width_avail <= m_props.front_side_ext + m_props.back_side_ext)
		return(0);

	// Get full width of the passed text.
	long width_req = GetStringWidth(hDC, str, length);
	if (width_req <= width_avail)
	{
		// No truncation. Reset the dots flag if needed.
		if (add_dots_if_truncating != NULL)
			*add_dots_if_truncating = FALSE;
		return(length);
	}

	// Truncation happened.
	if (add_dots_if_truncating != NULL && *add_dots_if_truncating == TRUE)
	{
		// Adjust the available width.
		width_avail -= GetStringWidth(hDC, L"...", 3, NULL, TRUE, TRUE);
		if (width_avail <= 0)
			return(0);
	}

	// Find appropriate length that still fits into the area.
	while (width_req > width_avail && length > 0)
	{
		if (length > 10)
		{
			long new_len = length/2;
			width_req = GetStringWidth(hDC, str, new_len);
			length = (width_req >= width_avail) ? new_len : length-1;
		}
		else
		{
			length--;
		}

		width_req = GetStringWidth(hDC, str, length);
	}

	// Return remaining length, maybe zero.
	return(length);
}

void TTextStyle::DrawFrgrText(HDC hDC, wchar_t *text_body, long text_length, RECT &rc_text_rect, bool block_front_ext)
{
	if (text_length == 0)
		return;

	HFONT  oldFont = (HFONT)::SelectObject(hDC, m_hFont);
	COLORREF oldFrColor = ::SetTextColor(hDC, m_props.foreground_color);
	int oldBkMode = ::SetBkMode(hDC, TRANSPARENT);

	// Copy the rect and adjust its sides. Do not adjust the right side and even add small extra room there.
	// In fact, with some fonts, and especially when the view is zoomed in, Windows is not evaluating the width
	// of the text properly, and this results in clipping the tail of the text. Adding extra space to the right
	// mitigates this problem.
	RECT rc_text = rc_text_rect;
	rc_text.left += (block_front_ext == FALSE) ? m_props.front_side_ext : 0;
	rc_text.right++;
	rc_text.top += m_props.baseline_adjust + m_props.up_side_adjust;
	rc_text.bottom += m_props.baseline_adjust - m_props.down_side_adjust;

	if (m_props.shadow_down_width > 0)
	{
		// Paint lower shadow and shift the rect back.
		rc_text.left += m_props.shadow_down_width;
		rc_text.top += m_props.shadow_down_width;
		::SetTextColor(hDC, m_props.shadow_down_color);

		if (m_symbols_adjust_data == NULL)
			::DrawTextW(hDC, text_body, text_length, &rc_text, DT_LEFT | DT_NOPREFIX);
		else DrawTextWithAdjustments(hDC, text_body, text_length, rc_text);

		rc_text.left -= m_props.shadow_down_width;
		rc_text.top -= m_props.shadow_down_width;
	}

	// Paint the main message on top of the shadow if any.
	::SetTextColor(hDC, m_props.foreground_color);
	if (m_symbols_adjust_data == NULL)
		::DrawTextW(hDC, text_body, text_length, &rc_text, DT_LEFT | DT_NOPREFIX);
	else DrawTextWithAdjustments(hDC, text_body, text_length, rc_text);

	// Restore the props in DC.
	::SelectObject(hDC, oldFont);
	::SetTextColor(hDC, oldFrColor);
	::SetBkMode(hDC, oldBkMode);
}

int TTextStyle::GetSimplePrefix(const wchar_t *text_body, long text_length, TTextStyleSymbolAdjustInfo *&sa_data) const
{
	int len;
	sa_data = NULL;
	for (len=0; len<text_length; ++len)
	{
		wchar_t ch = text_body[len];
		if (ch > 0 && ch < 128)
		{
			// Use the fast lookup.
			short sa_inx = m_symbols_adjust_lookup[ch];
			if (sa_inx != 0)
			{
				sa_data = m_symbols_adjust_data+(sa_inx & 0x7FFF);
				break;
			}
		}
		else
		{
			// Use the slow search.
			TTextStyleSymbolAdjustInfo *data = m_symbols_adjust_data;
			while (data->symbol != 0)
			{
				if (data->symbol == ch)
				{
					sa_data = data;
					break;
				}
				data++;
			}
		}
	}

	return(len);
}

void TTextStyle::DrawTextWithAdjustments(HDC hDC, wchar_t *text_body, long text_length, RECT &rc_text)
{
	assert(m_symbols_adjust_data != NULL);

	RECT rc = rc_text;
	while (text_length > 0)
	{
		// Search for perfix that does not need adjustments.
		TTextStyleSymbolAdjustInfo *sa_data;
		int len = GetSimplePrefix(text_body, text_length, sa_data);

		// Check for extriemly weird case when adjusted symbol is preceded with a space.
		// For some unclear reson Windows is processing this case badly with some fonts.
		int len_seq_to_adjust = 1;
		if (sa_data != NULL && len > 0 && text_body[len-1] == L' ')
		{
			len_seq_to_adjust++;
			len--;
		}

		if (len > 0)
		{
			// Paint simple text prefix.
			::DrawTextW(hDC, text_body, len, &rc, DT_LEFT | DT_NOPREFIX);

			// Remove prefix from the source string.
			text_body += len;
			text_length -= len;

			if (text_length > 0)
			{
				// Fugure out width of the painted prefix.
				SIZE text_size;
				::GetTextExtentPoint32W(hDC, text_body-len, len, &text_size);
				rc.left += text_size.cx;
			}
		}

		if (sa_data != NULL)
		{
			assert(text_length > 0);

			rc.left += sa_data->horz_shift;
			rc.right += sa_data->horz_shift;
			rc.top += sa_data->vert_shift;
			rc.bottom += sa_data->vert_shift;

			// Paint the single adjusted symbol.
			::DrawTextW(hDC, text_body, len_seq_to_adjust, &rc, DT_LEFT | DT_NOPREFIX);

			rc.left -= sa_data->horz_shift;
			rc.right -= sa_data->horz_shift;
			rc.top -= sa_data->vert_shift;
			rc.bottom -= sa_data->vert_shift;

			// Remove char from the source string.
			text_body += len_seq_to_adjust;
			text_length -= len_seq_to_adjust;

			if (text_length > 0)
			{
				// Fugure out the width of the painted char.
				SIZE text_size;
				::GetTextExtentPoint32W(hDC, text_body-len_seq_to_adjust, len_seq_to_adjust, &text_size);
				rc.left += text_size.cx;
				rc.left += sa_data->width_delta;
			}
		}
	}
}

// ----------------------------------------------------------------------
//   ===================  TTextItem  =========================
// ----------------------------------------------------------------------

void TTextItem::SetStyle(TScreenItemStyle* new_style)
{
	if (new_style != NULL)
		assert(new_style->StyleTypeId() == TTextStyle::type_ID);

	m_style = new_style;
}

bool TTextItem::SetContents(const wchar_t *body, long length, TDataBypassMode bypass_mode)
{
	//
	//  It is expected that item's rect will be updated by the app level. At this point the style may
	//  be still missing. Note that zero length data is needed and it is used in some cases.
	//

	// Release the prev data if any.
	if (m_text_owned == TRUE)
	{
		free(m_text_body);
		m_text_owned = FALSE;
	}

	m_text_body = NULL;
	m_text_length = 0;

	// Verify/fix the input params.
	if (body == NULL)
		return(FALSE);
	if (length < 0)
		length = (long)wcslen(body);

	// Duplicate the data if this is requested.
	if (bypass_mode == dbm_duplicate)
	{
		// Plus one in the length is needed when in rare cases the length is zero.
		wchar_t *buffer = (wchar_t*)malloc((length+1)*sizeof(wchar_t));
		if (buffer == NULL)
			return(FALSE);

		if (length > 0)
			wcsncpy(buffer, body, length);

		body = buffer;
		bypass_mode = dbm_take_ownsh;
	}

	// Accept the data set.
	m_text_body = (wchar_t*)body;
	m_text_length = length;
	m_text_owned = (bypass_mode == dbm_take_ownsh) ? TRUE : FALSE;

	// Reset the blocking flags because they are not set in the typical case.
	m_block_front_ext_and_frame = FALSE;
	m_block_back_ext_and_frame = FALSE;
	return(TRUE);
}

long TTextItem::PlaceObject(HWND hCtrl, long pos_x, long baseline_y)
{
	HDC hDC = ::GetDC(hCtrl);
	if (hDC != NULL)
	{
		// In a typical case getting the DC should succeed.
		long ret_val = PlaceObject(hDC, pos_x, baseline_y);
		::ReleaseDC(hCtrl, hDC);
		return(ret_val);
	}
	else
	{
		// This is rare Windows failure. Implement pseudo procedure.
		RECT new_rc = { pos_x, baseline_y-10, pos_x+10, baseline_y };
		SetVerifiedPosition(new_rc);
		return(pos_x+10);
	}
}

long TTextItem::PlaceObject(HDC hDC, long pos_x, long baseline_y)
{
	long szh;
	long szw = Style()->GetStringWidth(hDC, m_text_body, m_text_length, &szh, m_block_front_ext_and_frame, m_block_back_ext_and_frame);
	RECT new_rc = { pos_x, baseline_y-Style()->HeightAbove(), 0, 0 };
	new_rc.right = new_rc.left + szw;
	new_rc.bottom = new_rc.top + szh;
	SetVerifiedPosition(new_rc);
	return(new_rc.right);
}

void TTextItem::PlaceAroundCenter(HDC hDC, long pos_center_x, long pos_center_y)
{
	long szh;
	long szw = Style()->GetStringWidth(hDC, m_text_body, m_text_length, &szh, m_block_front_ext_and_frame, m_block_back_ext_and_frame);
	RECT new_rc = { pos_center_x-szw/2, pos_center_y-szh/2, 0, 0 };
	new_rc.right = new_rc.left + szw;
	new_rc.bottom = new_rc.top + szh;
	SetVerifiedPosition(new_rc);
}

void TTextItem::PlaceHorzCentered(HDC hDC, long pos_center_x, long baseline_y)
{
	long szh;
	long szw = Style()->GetStringWidth(hDC, m_text_body, m_text_length, &szh, m_block_front_ext_and_frame, m_block_back_ext_and_frame);
	RECT new_rc = { pos_center_x-szw/2, baseline_y-Style()->HeightAbove(), 0, 0 };
	new_rc.right = new_rc.left + szw;
	new_rc.bottom = new_rc.top + szh;
	SetVerifiedPosition(new_rc);
}

void TTextItem::SetHotSpotPos(long new_x, long new_y)
{
	RECT new_rc = { new_x, new_y, new_x+(m_bounding_rect.right-m_bounding_rect.left), new_y+(m_bounding_rect.bottom-m_bounding_rect.top) };
	SetVerifiedPosition(new_rc);
}

void TTextItem::OnDraw(HDC hDC, RECT &rc_invalid)
{
	if (m_text_body == NULL)
		return;

	assert(m_style != NULL);
	assert(Style()->m_hFont != NULL);
	const TTextStyleProps &props = Style()->m_props;

	if (props.background_color != cs_transparent)
	{
		// Clear the background.
		HBRUSH bkgrBrush = ::CreateSolidBrush(props.background_color);
		::FillRect(hDC, &m_bounding_rect, bkgrBrush);
		::DeleteObject(bkgrBrush);
	}

	TFrameCornerStyle fcs = props.frame_corner_style;
	if (props.frame_color != cs_transparent && fcs != fcs_none)
	{
		// Paint the frame.
		HPEN hPen = ::CreatePen(PS_SOLID, 1, props.frame_color);
		HPEN oldPen = (HPEN)::SelectObject(hDC, hPen);

		long ZZ = 0;
		if (fcs == fcs_1pix_excl_tip || fcs == fcs_1pix_excl_tip_add_inner_dot)
			ZZ = 1;

		::MoveToEx(hDC, m_bounding_rect.left+ZZ, m_bounding_rect.top, NULL);
		::LineTo(hDC, m_bounding_rect.right-1, m_bounding_rect.top);
		::MoveToEx(hDC, m_bounding_rect.right-1-ZZ, m_bounding_rect.bottom-1, NULL);
		::LineTo(hDC, m_bounding_rect.left, m_bounding_rect.bottom-1);

		::MoveToEx(hDC, m_bounding_rect.left, m_bounding_rect.bottom-1-ZZ, NULL);
		::LineTo(hDC, m_bounding_rect.left, m_bounding_rect.top);
		::MoveToEx(hDC, m_bounding_rect.right-1, m_bounding_rect.top+ZZ, NULL);
		::LineTo(hDC, m_bounding_rect.right-1, m_bounding_rect.bottom-1);

		::SelectObject(hDC, oldPen);
		::DeleteObject(hPen);

		if (fcs == fcs_1pix_add_inner_dot || fcs == fcs_1pix_excl_tip_add_inner_dot)
		{
			// Add 4 pixems in the corners.
			::SetPixel(hDC, m_bounding_rect.left+1, m_bounding_rect.top+1, props.frame_color);
			::SetPixel(hDC, m_bounding_rect.left+1, m_bounding_rect.bottom-2, props.frame_color);
			::SetPixel(hDC, m_bounding_rect.right-2, m_bounding_rect.top+1, props.frame_color);
			::SetPixel(hDC, m_bounding_rect.right-2, m_bounding_rect.bottom-2, props.frame_color);
		}
	}

	// Paint the message itself.
	((TTextStyle*)Style())->DrawFrgrText(hDC, m_text_body, m_text_length, m_bounding_rect, m_block_front_ext_and_frame);
}


