//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>
#include  <math.h>

#pragma hdrstop

#include  "ScreenItems/BasicSymbolItem.H"

//----------------------------------------------------------------
//  ===============  TBasicSymbolStyle  ===================
//----------------------------------------------------------------

TBasicSymbolStyle::TBasicSymbolStyle()
{
	m_style_info.Clear();
	m_style_info.main_bkgr_style = &m_main_bkgr_style;
	m_style_info.pcb_lower_bkgr_style = &m_pcb_lower_bkgr_style;
	m_style_info.main_label_style = &m_main_label_style;
	m_style_info.symbol_label_style = &m_symbol_label_style;

	// Place reasonable values into some of the fields.
	m_style_info.cnct_circle_diameter = 40;
	m_style_info.pcb_lower_rect_height = 8;
}

bool TBasicSymbolStyle::SetupForDeviceContext(HDC hDC)
{
	bool overallSuccess = TRUE;
	overallSuccess &= TScreenItemStyle::SetupForDeviceContext(hDC);
	overallSuccess &= m_main_bkgr_style.SetupForDeviceContext(hDC);
	overallSuccess &= m_pcb_lower_bkgr_style.SetupForDeviceContext(hDC);
	overallSuccess &= m_main_label_style.SetupForDeviceContext(hDC);
	overallSuccess &= m_symbol_label_style.SetupForDeviceContext(hDC);
	return(overallSuccess);
}

//----------------------------------------------------------------
//  ===============  TBasicSymbolItem  ===================
//----------------------------------------------------------------

void TBasicSymbolItem::SetOwner(void *owner, DWORD ext_data)
{
	// Simply pass this data to all member items.
	TScreenItem::SetOwner(owner, ext_data);
	m_app_icon.SetOwner(owner, ext_data);
	m_main_label.SetOwner(owner, ext_data);
	m_symbol_label.SetOwner(owner, ext_data);
	m_def_circle.SetOwner(owner, ext_data);
	m_main_rect.SetOwner(owner, ext_data);
	m_lower_rect.SetOwner(owner, ext_data);
}

void TBasicSymbolItem::SetBoundingRectPos(long pos_x, long pos_y)
{
	if (m_bounding_rect.left != pos_x || m_bounding_rect.top != pos_y)
		ShiftItem(pos_x-m_bounding_rect.left, pos_y-m_bounding_rect.top);
}

void TBasicSymbolItem::SetHotSpotPos(long pos_x, long pos_y)
{
	long px, py;
	if (m_def_circle.GetBasicStyle() != NULL)
	{
		// Current mode is concentric.
		m_def_circle.GetHotSpotPos(px, py);
	}
	else if (m_main_rect.GetBasicStyle() != NULL)
	{
		// Current mode is rect lines.
		m_main_rect.GetHotSpotPos(px, py);
	}
	else
	{
		// The object is not prepared yet.
		return;
	}

	if (px != pos_x || py != pos_y)
		ShiftItem(pos_x-px, pos_y-py);
}

void TBasicSymbolItem::GetHotSpotPos(long &px, long &py)
{
	px = py = 0;
	if (m_def_circle.GetBasicStyle() != NULL)
		m_def_circle.GetHotSpotPos(px, py);
	else if (m_main_rect.GetBasicStyle() != NULL)
		m_main_rect.GetHotSpotPos(px, py);
}

void TBasicSymbolItem::ShiftItem(long shift_x, long shift_y)
{
	if (m_app_icon.GetBasicStyle() != NULL)
		m_app_icon.ShiftItem(shift_x, shift_y);

	if (m_main_label.GetBasicStyle() != NULL)
		m_main_label.ShiftItem(shift_x, shift_y);
	if (m_symbol_label.GetBasicStyle() != NULL)
		m_symbol_label.ShiftItem(shift_x, shift_y);

	if (m_def_circle.GetBasicStyle() != NULL)
		m_def_circle.ShiftItem(shift_x, shift_y);
	if (m_main_rect.GetBasicStyle() != NULL)
		m_main_rect.ShiftItem(shift_x, shift_y);
	if (m_lower_rect.GetBasicStyle() != NULL)
		m_lower_rect.ShiftItem(shift_x, shift_y);

	ShiftRcItem(shift_x, shift_y);
}

void TBasicSymbolItem::OnDraw(HDC hDC, RECT &rc_invalid)
{
	//
	// Draw items from bottom to top.
	//

	if (m_def_circle.GetBasicStyle() != NULL)
		m_def_circle.OnDraw(hDC, rc_invalid);
	if (m_main_rect.GetBasicStyle() != NULL)
		m_main_rect.OnDraw(hDC, rc_invalid);
	if (m_lower_rect.GetBasicStyle() != NULL)
		m_lower_rect.OnDraw(hDC, rc_invalid);

	if (m_main_label.GetBasicStyle() != NULL)
		m_main_label.OnDraw(hDC, rc_invalid);
	if (m_symbol_label.GetBasicStyle() != NULL)
		m_symbol_label.OnDraw(hDC, rc_invalid);

	if (m_app_icon.GetBasicStyle() != NULL)
		m_app_icon.OnDraw(hDC, rc_invalid);
}

TScreenItem *TBasicSymbolItem::CheckClick(POINT &app_point)
{
	//
	// Check items from top to bottom.
	//
	TScreenItem *clicked_item = NULL;

	if (m_app_icon.GetBasicStyle() != NULL && m_app_icon.CheckClick(app_point) != NULL)
		clicked_item = this;
	else if (m_symbol_label.GetBasicStyle() != NULL && m_symbol_label.CheckClick(app_point) != NULL)
		clicked_item = this;
	else if (m_main_label.GetBasicStyle() != NULL && m_main_label.CheckClick(app_point) != NULL)
		clicked_item = this;
	else if (m_lower_rect.GetBasicStyle() != NULL && m_lower_rect.CheckClick(app_point) != NULL)
		clicked_item = this;
	else if (m_main_rect.GetBasicStyle() != NULL && m_main_rect.CheckClick(app_point) != NULL)
		clicked_item = this;
	else if (m_def_circle.GetBasicStyle() != NULL && m_def_circle.CheckClick(app_point) != NULL)
		clicked_item = this;

	return(clicked_item);
}

void TBasicSymbolItem::ResetScreenItems(TList &screen_items_data_set)
{
	if (m_app_icon.IsInList() == TRUE)
		screen_items_data_set.RemoveItem(m_app_icon);
	if (m_main_label.IsInList() == TRUE)
		screen_items_data_set.RemoveItem(m_main_label);
	if (m_symbol_label.IsInList() == TRUE)
		screen_items_data_set.RemoveItem(m_symbol_label);

	if (m_def_circle.IsInList() == TRUE)
		screen_items_data_set.RemoveItem(m_def_circle);
	if (m_main_rect.IsInList() == TRUE)
		screen_items_data_set.RemoveItem(m_main_rect);
	if (m_lower_rect.IsInList() == TRUE)
		screen_items_data_set.RemoveItem(m_lower_rect);
}

void TBasicSymbolItem::PrepareConcentricModeItems(HDC hDC, TBasicSymbolStyleInfo &style_info, int pos_x, int pos_y,
											const wchar_t *main_label, int main_label_len, const wchar_t *symbol_label, int symbol_label_len,
											short app_icon_id)
{
	// Prepare the circle item.
	m_def_circle.SetStyle(style_info.main_bkgr_style);
	m_def_circle.SetPosition(pos_x, pos_y, style_info.cnct_circle_diameter);

	// Prepare the text labels.
	PrepareConcentricModeTextItem(hDC, style_info, pos_x, pos_y, m_main_label, style_info.main_label_style,
								main_label, main_label_len, style_info.cnct_main_label_shift_x, style_info.cnct_main_label_shift_y);
	PrepareConcentricModeTextItem(hDC, style_info, pos_x, pos_y, m_symbol_label, style_info.symbol_label_style,
								symbol_label, symbol_label_len, style_info.cnct_symbol_label_shift_x, style_info.cnct_symbol_label_shift_y);

	// Prepare the bounding rect. Without the proper bounding rect painting of the object may fail.
	m_bounding_rect = *m_def_circle.GetBoundingRect();
	if (m_main_label.GetBasicStyle() != NULL)
		CombineRects(m_bounding_rect, *m_main_label.GetBoundingRect());
	if (m_symbol_label.GetBasicStyle() != NULL)
		CombineRects(m_bounding_rect, *m_symbol_label.GetBoundingRect());

	// Prepare the application icon.
	m_app_icon.SetStyle(NULL);
	if (style_info.app_icons_style != NULL)
	{
		// The style is present. Now check for the icon id.
		if (app_icon_id == 0)
			app_icon_id = style_info.app_cnct_icon;

		if (app_icon_id != 0)
		{
			// Place the icon into the left-top corner.
			long icon_offs = style_info.cnct_circle_diameter/2-3;
			m_app_icon.SetStyle(style_info.app_icons_style);
			m_app_icon.SetPosition(pos_x-icon_offs, pos_y-icon_offs, app_icon_id);
			CombineRects(m_bounding_rect, *m_app_icon.GetBoundingRect());
		}
	}
}

void TBasicSymbolItem::PrepareRectLinesModeItems(HDC hDC, TBasicSymbolStyleInfo &style_info, int pos_x, int pos_y, int min_width_x,
											const wchar_t *main_lab, int main_lab_len, const wchar_t *symbol_lab, int symbol_lab_len,
											short app_icon_id)
{
	// Reset diameter of the main circle to indicate that current prepared mode is rect lines.
	m_def_circle.SetPosition(0, 0, 0);

	// Setup the main rect of the object.
	long rect_width = GetRectLinesModeRectWidth(hDC, style_info, min_width_x, main_lab, main_lab_len, symbol_lab, symbol_lab_len);
	RECT rct =
	{
		pos_x, pos_y, pos_x+rect_width, pos_y+GetRectLinesModeRectHeight(hDC, style_info),
	};

	m_main_rect.SetStyle(style_info.main_bkgr_style);
	m_main_rect.SetBaseRect(rct);

	// Prepare the text labels.
	PrepareRectLinesModeTextItem(hDC, style_info, pos_x, pos_y, m_main_label, style_info.main_label_style, main_lab, main_lab_len,
								TRUE, style_info.pcb_main_label_margins, style_info.pcb_main_label_margins);
	PrepareRectLinesModeTextItem(hDC, style_info, pos_x, pos_y, m_symbol_label, style_info.symbol_label_style, symbol_lab, symbol_lab_len,
								FALSE, style_info.pcb_symlab_left_margin, style_info.pcb_symlab_right_margin);

	// Setup the secondary lower rect.
	m_lower_rect.SetStyle(style_info.pcb_lower_bkgr_style);
	if (style_info.pcb_lower_bkgr_style != NULL && style_info.pcb_lower_rect_height > 0)
	{
		// Align this rect to the bottom of the main rect.
		rct.top = rct.bottom-style_info.pcb_lower_rect_height;
		m_lower_rect.SetBaseRect(rct);
	}

	// Prepare the bounding rect. Without proper bounding rect painting of the object may fail.
	m_bounding_rect = *m_main_rect.GetBoundingRect();
	if (m_symbol_label.GetBasicStyle() != NULL)
		CombineRects(m_bounding_rect, *m_symbol_label.GetBoundingRect());

	// Prepare the application icon.
	m_app_icon.SetStyle(NULL);
	if (style_info.app_icons_style != NULL)
	{
		// The style is present. Now check for the icon id.
		if (app_icon_id == 0)
			app_icon_id = style_info.app_pcb_icon;

		if (app_icon_id != 0)
		{
			// Place the icon into the left-top corner of the rect.
			m_app_icon.SetStyle(style_info.app_icons_style);
			m_app_icon.SetPosition(pos_x, pos_y, app_icon_id);
			CombineRects(m_bounding_rect, *m_app_icon.GetBoundingRect());
		}
	}
}

int TBasicSymbolItem::GetRectLinesModeRectWidth(HDC hDC, TBasicSymbolStyleInfo &style_info, int min_width_x,
											const wchar_t *main_label, int main_label_len, const wchar_t *symbol_label, int symbol_label_len)
{
	int main_label_width = 0;
	if (main_label != NULL && main_label_len != 0)
		main_label_width = style_info.main_label_style->GetStringWidth(hDC, main_label, main_label_len);

	int symbol_label_width = 0;
	if (symbol_label != NULL && symbol_label_len != 0)
		symbol_label_width = style_info.symbol_label_style->GetStringWidth(hDC, symbol_label, symbol_label_len);

	// Calc the unrestricted widths for both lines of text with margins.
	int line1_len = main_label_width + 2*style_info.pcb_main_label_margins;
	int line2_len = style_info.pcb_symlab_left_margin + symbol_label_width + style_info.pcb_symlab_right_margin;

	// Step 1. Pick up the max from both lines.
	int len = __max(line1_len, line2_len);

	// Step 2. Check the max width restriction from the style props.
	if (style_info.pcb_max_rect_width > 0)
		len = __min(len, style_info.pcb_max_rect_width);

	// Step 3. Check the min width restriction that is based on the number of children.
	len = __max(len, min_width_x);
	return(len);
}

int TBasicSymbolItem::GetRectLinesModeRectHeight(HDC hDC, TBasicSymbolStyleInfo &style_info)
{
	return(style_info.pcb_main_label_down_shift+style_info.main_label_style->GetHeight()+style_info.pcb_main_label_lo_space+style_info.pcb_lower_rect_height);
}

void TBasicSymbolItem::PrepareConcentricModeTextItem(HDC hDC, TBasicSymbolStyleInfo &style_info, int center_pos_x, int center_pos_y,
											TTextItemExProto &scr_item, TTextStyle *text_style, const wchar_t *text_body, int text_len, int text_x_offs, int text_y_offs)
{
	// Ensure that all necessary parts are present.
	if (text_style == NULL || text_body == NULL || text_len == 0)
	{
		scr_item.SetStyle(NULL);
		return;
	}

	// Find out the width of the available area.
	long radius = style_info.cnct_circle_diameter/2;
	long y_offs = text_style->GetHeight() + ((&scr_item == &m_symbol_label) ? text_y_offs : -text_y_offs);
	long width_avail = 20;
	if (radius > y_offs)
	{
		// The center of the outer horz border of the label belongs to the circle.
		width_avail = 3+2*(long)sqrt((double)(radius*radius-y_offs*y_offs));
	}

	// Find out the length of the text that will be displayed.
	text_len = text_style->GetFittedLen(hDC, width_avail, text_body, text_len);
	if (text_len > 0)
	{
		// Prepare the center position on the baseline.
		long pos_x = center_pos_x + text_x_offs;
		long pos_y = center_pos_y + ((&scr_item == &m_symbol_label) ? text_style->HeightAbove() : -text_style->HeightBelow()) + text_y_offs;

		// Prepare the text screen item.
		scr_item.SetStyle(text_style);
		scr_item.SetText(text_body, text_len);
		scr_item.PlaceHorzCentered(hDC, pos_x, pos_y);
	}
	else
	{
		// There is no space even for one character.
		scr_item.SetStyle(NULL);
		return;
	}
}

void TBasicSymbolItem::PrepareRectLinesModeTextItem(HDC hDC, TBasicSymbolStyleInfo &style_info, int pos_x, int pos_y,
														TTextItemExProto &scr_item, TTextStyle *text_style, const wchar_t *text_body, int text_len,
														bool add_dots_if_truncating, int left_margin, int right_margin)
{
	// Ensure that all necessary parts are present.
	if (text_style == NULL || text_body == NULL || text_len == 0)
	{
		scr_item.SetStyle(NULL);
		return;
	}

	// Find out the width of available area.
	if (left_margin < 0)
		left_margin = text_style->GetBasicWidth();
	if (right_margin < 0)
		right_margin = text_style->GetBasicWidth();

	long width_avail = m_main_rect.GetBaseWidth()-left_margin-right_margin;
	if (width_avail <= 0)
	{
		// The margins are too wide. Nothing is visible.
		scr_item.SetStyle(NULL);
		return;
	}

	// Determine the part of the string that will be displayed.
	text_len = text_style->GetFittedLen(hDC, width_avail, text_body, text_len, &add_dots_if_truncating);
	if (text_len <= 0)
	{
		// The font is too big. Nothing is visible.
		scr_item.SetStyle(NULL);
		return;
	}

	// Setup the item style.
	scr_item.SetStyle(text_style);

	// Prepare the lev_y as baseline of the font.
	if (&scr_item == &m_symbol_label)
	{
		// Place the outer line right on top of the border.
		pos_x += m_main_rect.GetBaseWidth()-right_margin-text_style->GetStringWidth(hDC, text_body, text_len);
		pos_y -= text_style->HeightBelow();
		scr_item.SetText(text_body, text_len);
	}
	else
	{
		// Place the inner line right inside the main rect.
		pos_x += left_margin;
		pos_y += text_style->HeightAbove()+style_info.pcb_main_label_down_shift;
		scr_item.SetText(text_body, text_len);

		if (add_dots_if_truncating == TRUE)
		{
			// Fitting procedure decided to add dots. Add them to the text.
			scr_item.AppendText(L"...", 3);
		}
	}

	// Finally place the object.
	scr_item.PlaceObject(hDC, pos_x, pos_y);
}


