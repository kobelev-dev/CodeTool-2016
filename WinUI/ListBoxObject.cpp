//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   List Box Custom Control.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "WinUi/ListBoxObject.H"

#define DEEP_LIST_BOX_DEBUG

//---------------------------------------------------------------------------
//  ===================  TBasicListBoxItem  ========================
//---------------------------------------------------------------------------

TBasicListBoxItem::TBasicListBoxItem(BYTE ident) : TGenericListBoxItem<TBasicListBoxObject>(ident)
{
	ClearFieldsInternal(TListBoxHorzHelper::LBX_MAX_COLUMNS);
}

void TBasicListBoxItem::SetCellWidth(TListBoxObjectCore &owner, int iclmn, int width_in_columns)
{
	assert(iclmn >= 0 && iclmn < owner.GetNumColumns());
	TBasicListBoxObject &castedOwner = (TBasicListBoxObject&)owner;

	// Normalize the new width.
	if (width_in_columns < 1)
		width_in_columns = 1;
	if (iclmn+width_in_columns > owner.GetNumColumns())
		width_in_columns = owner.GetNumColumns()-iclmn;

	// Accept the data.
	m_cells_info[iclmn].cell_width = (BYTE)width_in_columns;
	for (iclmn++; width_in_columns>1; --width_in_columns, ++iclmn)
		m_cells_info[iclmn].cell_width = 0;

	// Check, if the height has changed or not.
	CheckUpdateHeight(castedOwner);
}

void TBasicListBoxItem::PaintItem(HDC hDC, TBasicListBoxObject *ptr_owner, RECT &item_rect)
{
	HDC hLocalDC = NULL;
	if (hDC == NULL)
	{
		// Caller has not passed the DC. Use the slow procedure.
		hLocalDC = ptr_owner->GetControlDC();
		assert(hLocalDC != NULL);
		hDC = hLocalDC;
	}

	// Paint the background.
	TColor bkgr_color = ptr_owner->m_bkgr_styles[(IsSelected() == TRUE) ? m_slct_bkgr_style_inx : m_norm_bkgr_style_inx].GetBkgrColor();
	HBRUSH hBkgr = ::CreateSolidBrush(bkgr_color);
	::FillRect(hDC, &item_rect, hBkgr);
	::DeleteObject(hBkgr);

	// Paint contents of the cells.
	TBasicListBoxCellInfo *cell_info = m_cells_info;
	TListBoxColumnState *clmn_info = &ptr_owner->m_horz_helper.ClmnInfo(0);
	for (int iclmn=0; iclmn<ptr_owner->GetNumColumns(); ++iclmn, ++cell_info, ++clmn_info)
	{
		if (cell_info->cell_width == 0)
			continue;

		// Prepare the rect of the cell.
		RECT rct = item_rect;
		rct.left += clmn_info->column_offset;
		if (cell_info->cell_width == 1)
		{
			rct.right = rct.left+clmn_info->column_width;
		}
		else
		{
			TListBoxColumnState &last_clmn = ptr_owner->m_horz_helper.ClmnInfo(iclmn+cell_info->cell_width-1);
			rct.right = item_rect.left+last_clmn.column_offset+last_clmn.column_width;
		}

		switch (cell_info->cell_flags)
		{
			case lbcf_local_text:
			case lbcf_extern_text:
					{
						BYTE style_inx = (IsSelected() == TRUE) ? cell_info->slct_text_style_inx : cell_info->norm_text_style_inx;
						int dc_inx = ::SaveDC(hDC);
						::IntersectClipRect(hDC, rct.left, rct.top, rct.right, rct.bottom);

						// Do the horz adjustment.
						RECT rc_cell_body = rct;
						if (clmn_info->column_align != align_left)
						{
							// Measure the width of the simple text.
							long  total_width = ptr_owner->m_frgr_styles[style_inx].GetStringWidth(hDC, cell_info->text_data.GetBodyPtr(), cell_info->text_data.GetLength());
							if (clmn_info->column_align == align_center)
							{
								rc_cell_body.left += (rc_cell_body.right-rc_cell_body.left-total_width)/2;
							}
							else if (clmn_info->column_align == align_right)
							{
								rc_cell_body.left = rc_cell_body.right-total_width;
							}
						}

						// Paint the simple text.
						long delta = m_baseline_offs-ptr_owner->m_frgr_styles[style_inx].HeightAbove();
						rc_cell_body.top += delta;
						rc_cell_body.bottom += delta;
						ptr_owner->m_frgr_styles[style_inx].DrawFrgrText(hDC, cell_info->text_data.GetBodyPtr(), cell_info->text_data.GetLength(), rc_cell_body);

						// Restore the clipping back.
						::RestoreDC(hDC, dc_inx);
					}
					break;

			case lbcf_local_script:
			case lbcf_extern_script:
					{
						BYTE style_inx = (IsSelected() == TRUE) ? cell_info->slct_text_style_inx : cell_info->norm_text_style_inx;
						int dc_inx = ::SaveDC(hDC);
						::IntersectClipRect(hDC, rct.left, rct.top, rct.right, rct.bottom);

						// Do the horz adjustment.
						RECT rc_cell_body = rct;
						if (clmn_info->column_align != align_left)
						{
							// Measure the width of the scripted text.
							long  total_width = ProcessScriptedText(hDC, ptr_owner, FALSE, rc_cell_body, style_inx, IsSelected(), cell_info->text_data);
							if (clmn_info->column_align == align_center)
							{
								rc_cell_body.left += (rc_cell_body.right-rc_cell_body.left-total_width)/2;
							}
							else if (clmn_info->column_align == align_right)
							{
								rc_cell_body.left = rc_cell_body.right-total_width;
							}
						}

						// Paint the script in the updated rect.
						ProcessScriptedText(hDC, ptr_owner, TRUE, rc_cell_body, style_inx, IsSelected(), cell_info->text_data);

						// Restore the clipping back.
						::RestoreDC(hDC, dc_inx);
					}
					break;
		}
	}

	if (hLocalDC != NULL)
		ptr_owner->ReleaseControlDC(hLocalDC);
}

int TBasicListBoxItem::MeasureItemHeight(TBasicListBoxObject *ptr_owner)
{
	int max_above = 0, max_below = 0;
	TBasicListBoxCellInfo *cell_info = m_cells_info;
	for (int iclmn=0; iclmn<ptr_owner->GetNumColumns(); ++iclmn, ++cell_info)
	{
		if (cell_info->cell_width == 0)
			continue;

		switch (cell_info->cell_flags)
		{
			case lbcf_local_text:
			case lbcf_extern_text:
					{
						// A simple text is painted using just one pair of styles.
						CheckStyleExts(ptr_owner, cell_info->norm_text_style_inx, max_above, max_below);
						CheckStyleExts(ptr_owner, cell_info->slct_text_style_inx, max_above, max_below);
					}
					break;

			case lbcf_local_script:
			case lbcf_extern_script:
					{
						// Process initial style indexes.
						CheckStyleExts(ptr_owner, cell_info->norm_text_style_inx, max_above, max_below);
						CheckStyleExts(ptr_owner, cell_info->slct_text_style_inx, max_above, max_below);

						wchar_t *src_str = cell_info->text_data.GetBodyPtr();
						int src_len = cell_info->text_data.GetLength();
						while (src_len > 0)
						{
							if (src_str[0] != L'@')
							{
								src_str++;
								src_len--;
								continue;
							}
							else if (src_len >= 2 && src_str[1] == L'@')
							{
								src_str += 2;
								src_len -= 2;
								continue;
							}

							// This seems to be a beginning of the inline control.
							int fragm_len = 0;
							short norm_value, slct_value;
							if (ScanScriptedTextCtrlInfo(src_str, src_len, fragm_len, norm_value, slct_value) == ic_change_style)
							{
								// Process the modified styles.
								CheckStyleExts(ptr_owner, norm_value, max_above, max_below);
								CheckStyleExts(ptr_owner, slct_value, max_above, max_below);
							}

							// Take out the fragment from the source string.
							src_str += fragm_len;
							src_len -= fragm_len;
						}
					}
					break;
		}
	}

	if (max_above == 0 && max_below == 0)
	{
		// All cells are empty. Use the style[0] to assign the height of the item.
		max_above = ptr_owner->m_frgr_styles[0].HeightAbove();
		max_below = ptr_owner->m_frgr_styles[0].HeightBelow();
	}

	m_baseline_offs = (WORD)max_above;
	return(max_above + max_below);
}

void TBasicListBoxItem::ClearCell(TBasicListBoxObject &owner, int iclmn)
{
	assert(iclmn >= 0 && iclmn < owner.GetNumColumns());
	RemoveLocalString(owner, iclmn);

	TBasicListBoxCellInfo &cell = m_cells_info[iclmn];
	BYTE old_width = cell.cell_width;

	cell.cell_flags = lbcf_empty;
	cell.cell_width = 1;
	cell.norm_text_style_inx = 0;
	cell.slct_text_style_inx = 1;

	if (old_width > 1)
	{
		// This should not happen in a good code. Nevertheless, reveal the cells.
		for (iclmn++; old_width>1; --old_width, ++iclmn)
			m_cells_info[iclmn].cell_width = 1;
	}

	// Check, if the height has changed or not.
	CheckUpdateHeight(owner);
}

void TBasicListBoxItem::ClearBkgrAndCells(TBasicListBoxObject &owner)
{
	ClearFieldsInternal(owner.GetNumColumns());

	// Check, if the height has changed or not.
	CheckUpdateHeight(owner);
}

void TBasicListBoxItem::SetBkgrStyle(TBasicListBoxObject &owner, BYTE normal_style_inx, BYTE selected_style_inx)
{
	// This call cannot change any width or height. At least for now.
	m_norm_bkgr_style_inx = normal_style_inx;
	m_slct_bkgr_style_inx = selected_style_inx;
}

void TBasicListBoxItem::SetFrgrStyleToCell(TBasicListBoxObject &owner, int iclmn, BYTE normal_inx, BYTE selected_inx)
{
	assert(iclmn >= 0 && iclmn < owner.GetNumColumns());
	m_cells_info[iclmn].norm_text_style_inx = normal_inx;
	m_cells_info[iclmn].slct_text_style_inx = selected_inx;

	// Check, if the height has changed or not.
	CheckUpdateHeight(owner);
}

bool TBasicListBoxItem::SetStringToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *string, int len)
{
	return(SetLocalStringInternal(owner, iclmn, lbcf_local_text, string, len));
}

bool TBasicListBoxItem::SetFmtStrToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *format, ...)
{
	assert(iclmn >= 0 && iclmn < owner.GetNumColumns());
	RemoveLocalString(owner, iclmn);

	// Estimate the size of the output string.
	assert(format != NULL);
	int len_needed = wcslen(format)+20;
	for(;;)
	{
		// Find some place in the local buffer.
		int len_allocated = 0;
		wchar_t *space = ReserveLocalSpace(owner, len_needed, len_allocated);
		if (space == NULL)
		{
			// Leave the cell in its old state except maybe without the string.
			return(FALSE);
		}

		// Make the formatting attempt.
		va_list vaList;
		va_start(vaList, format);
		int len_written = vswprintf(space, len_allocated, format, vaList);
		va_end(vaList);

		// Check the results, if there was enough space or not.
		if (len_written == 0)
		{
			// Writing succeeded with empty string.
			m_cells_info[iclmn].text_data.Clear();
			break;
		}
		else if (len_written > 0)
		{
			// Writing succeeded with non empty string.
			m_cells_info[iclmn].text_data.SetData(space, len_written);
			m_local_buffer.IncNumItems(len_written);
			break;
		}

		// The buffer turned out to be not long enough.
		len_needed = len_allocated+80;
	}

	// Finalize the cell.
	m_cells_info[iclmn].cell_flags = lbcf_local_text;
	CheckUpdateHeight(owner);
	return(TRUE);
}

bool TBasicListBoxItem::SetScriptedStringToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *string, int len)
{
	return(SetLocalStringInternal(owner, iclmn, lbcf_local_script, string, len));
}

void TBasicListBoxItem::SetExtStringToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *string, int len)
{
	assert(iclmn >= 0 && iclmn < owner.GetNumColumns());
	RemoveLocalString(owner, iclmn);

	TBasicListBoxCellInfo &cell = m_cells_info[iclmn];
	cell.cell_flags = lbcf_extern_text;
	cell.text_data.SetData(string, len);

	// Check, if the height has changed or not.
	CheckUpdateHeight(owner);
}

void TBasicListBoxItem::SetExtScriptedStringToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *string, int len)
{
	assert(iclmn >= 0 && iclmn < owner.GetNumColumns());
	RemoveLocalString(owner, iclmn);

	TBasicListBoxCellInfo &cell = m_cells_info[iclmn];
	cell.cell_flags = lbcf_extern_script;
	cell.text_data.SetData(string, len);

	// Check, if the height has changed or not.
	CheckUpdateHeight(owner);
}

void TBasicListBoxItem::ClearFieldsInternal(int num_columns)
{
	// Reset the bkgr style indexes.
	m_norm_bkgr_style_inx = 0;
	m_slct_bkgr_style_inx = 1;
	m_baseline_offs = 0;

	// Clear the frgr cell by cell.
	for (int iclmn=0; iclmn<num_columns; ++iclmn)
	{
		TBasicListBoxCellInfo &cell = m_cells_info[iclmn];
		cell.cell_flags = lbcf_empty;
		cell.cell_width = 1;
		cell.norm_text_style_inx = 0;
		cell.slct_text_style_inx = 1;
	}

	// Reset the local heap.
	m_local_buffer.ClearBuffer();
}

void TBasicListBoxItem::CheckUpdateHeight(TBasicListBoxObject &owner)
{
	// It makes sense to report the height changes only when the item is in the list.
	if (IsInList() == TRUE)
	{
		short new_height = MeasureItemHeight(&owner);
		if (new_height != m_cached_height)
		{
			owner.UpdateItemHeight(this, new_height);
		}
	}
}

bool TBasicListBoxItem::SetLocalStringInternal(TBasicListBoxObject &owner, int iclmn, TBasicListBoxCellFlags cf, const wchar_t *string, int len)
{
	assert(iclmn >= 0 && iclmn < owner.GetNumColumns());
	RemoveLocalString(owner, iclmn);

	TBasicListBoxCellInfo &cell = m_cells_info[iclmn];
	if (string == NULL || len == 0 || string[0] == 0)
	{
		// The passed string is empty.
		cell.text_data.Clear();
	}
	else
	{
		// Alocate the space for the string.
		if (len < 0)
			len = wcslen(string);

		int len_allocated = 0;
		wchar_t *space = ReserveLocalSpace(owner, len, len_allocated);
		if (space == NULL)
		{
			// Leave the cell in its old state except maybe without the string.
			return(FALSE);
		}

		// Move the data to the buffer.
		wcsncpy(space, string, len);
		m_local_buffer.IncNumItems(len);
		cell.text_data.SetData(space, len);
	}

	// Finalize the cell.
	cell.cell_flags = cf;
	CheckUpdateHeight(owner);
	return(TRUE);
}

bool TBasicListBoxItem::IsLocalStringCell(int iclmn)
{
	TBasicListBoxCellFlags cf = m_cells_info[iclmn].cell_flags;
	return((cf == lbcf_local_text || cf == lbcf_local_script) && m_cells_info[iclmn].text_data.GetLength() > 0);
}

wchar_t *TBasicListBoxItem::ReserveLocalSpace(TBasicListBoxObject &owner, int len_requested, int &len_allocated)
{
	// Store the current state of the buffer.
	wchar_t *curr_data = m_local_buffer.DataPtr();
	int curr_len = m_local_buffer.NumItems();

	if (m_local_buffer.ReserveSpace(len_requested) == FALSE)
		return(NULL);

	// Appending succeeded.
	len_allocated = m_local_buffer.NumAllocedItems()-m_local_buffer.NumItems();
	if (m_local_buffer.DataPtr() != curr_data)
	{
		// Buffer reallocation happened. Fix all existing pointers to the local strings.
		for (int ic=0; ic<owner.GetNumColumns(); ++ic)
		{
			if (IsLocalStringCell(ic) == TRUE)
			{
				TStringPtr &cell_text = m_cells_info[ic].text_data;
				INT_PTR offs = (INT_PTR)(cell_text.GetBodyPtr()-curr_data);
				cell_text.SetData(m_local_buffer.DataPtr()+offs, cell_text.GetLength());
			}
		}
	}

	// Success.
	return(m_local_buffer.DataPtr()+curr_len);
}

void TBasicListBoxItem::RemoveLocalString(TBasicListBoxObject &owner, int iclmn)
{
	if (IsLocalStringCell(iclmn) == TRUE)
	{
		// Passed cell owns a local string.
		TStringPtr &ptr = m_cells_info[iclmn].text_data;
		wchar_t *ptr_ptr = ptr.GetBodyPtr();
		INT_PTR ptr_offs = (INT_PTR)(ptr_ptr-m_local_buffer.DataPtr());
		int ptr_len = ptr.GetLength();

		// Clear the string.
		ptr.Clear();

		if (ptr_offs+ptr_len < m_local_buffer.NumItems())
		{
			// The string, that is being removed, is not the last one in the buffer.
			memmove(ptr_ptr, ptr_ptr+ptr_len, (m_local_buffer.NumItems()-(ptr_offs+ptr_len))*sizeof(wchar_t));
			for (int ic=0; ic<owner.GetNumColumns(); ++ic)
			{
				TStringPtr &cell_text = m_cells_info[ic].text_data;
				if (IsLocalStringCell(ic) == TRUE && cell_text.GetBodyPtr() > ptr_ptr)
				{
					// Current cell owns the local string that stays after the string, that is being removed.
					cell_text.SetData(cell_text.GetBodyPtr()-ptr_len, cell_text.GetLength());
				}
			}
		}

		m_local_buffer.IncNumItems(-ptr_len);
	}
}

void TBasicListBoxItem::CheckStyleExts(TBasicListBoxObject *ptr_owner, int style_inx, int &max_above, int &max_below)
{
	if (ptr_owner->m_frgr_styles[style_inx].HeightAbove() > max_above)
		max_above = ptr_owner->m_frgr_styles[style_inx].HeightAbove();
	if (ptr_owner->m_frgr_styles[style_inx].HeightBelow() > max_below)
		max_below = ptr_owner->m_frgr_styles[style_inx].HeightBelow();
}

bool TBasicListBoxItem::ScanShortInt(const wchar_t *src_str, int src_len, int &fragm_len, short &result)
{
	result = 0;
	bool some_digits_found = FALSE;
	while (fragm_len < src_len)
	{
		wchar_t ch = src_str[fragm_len];
		if (ch < L'0' || ch > L'9')
			break;

		result = result*10 + (ch - L'0');
		fragm_len++;
		some_digits_found = TRUE;
	}

	return(some_digits_found);
}

TBasicListBoxItem::TScriptedTextCtrlType TBasicListBoxItem::ScanScriptedTextCtrlInfo(const wchar_t *src_str, int src_len, int &fragm_len, short &norm_value, short &slct_value)
{
	assert(src_str != NULL && src_len > 0 && src_str[0] == L'@');

	// At least the passed string begins with the control key.
	fragm_len = 1;
	if (fragm_len >= src_len)
		return(ic_none);

	switch (src_str[fragm_len])
	{
		case L'I': case L'i':
				{
					fragm_len++;
					if (ScanShortInt(src_str, src_len, fragm_len, norm_value) == FALSE)
						return(ic_none);
					if (fragm_len >= src_len)
						return(ic_none);

					switch (src_str[fragm_len])
					{
						case L',':
								{
									// This is a long form of the draw icon description.
									fragm_len++;
									if (fragm_len >= src_len || (src_str[fragm_len+1] != L'I' && src_str[fragm_len+1] != L'i'))
										return(ic_none);

									fragm_len++;
									if (ScanShortInt(src_str, src_len, fragm_len, slct_value) == FALSE)
										return(ic_none);

									if (fragm_len < src_len && src_str[fragm_len] == L';')
									{
										fragm_len++;
										return(ic_draw_icon);
									}
								}
								break;

						case L';':
								{
									// This is a short form with the same icon for normal/selected cases.
									fragm_len++;
									slct_value = norm_value;
									return(ic_draw_icon);
								}
								break;
					}
				}
				break;

		case L'S': case L's':
				{
					fragm_len++;
					if (ScanShortInt(src_str, src_len, fragm_len, norm_value) == FALSE)
						return(ic_none);

					if (fragm_len >= src_len)
						return(ic_none);

					switch (src_str[fragm_len])
					{
						case L',':
								{
									// This is a long form of the change style description.
									if (fragm_len >= src_len || (src_str[fragm_len+1] != L'S' && src_str[fragm_len+1] != L's'))
										return(ic_none);

									fragm_len++;
									if (ScanShortInt(src_str, src_len, fragm_len, slct_value) == FALSE)
										return(ic_none);

									if (fragm_len < src_len && src_str[fragm_len] == L';')
									{
										fragm_len++;
										return(ic_change_style);
									}
								}
								break;

						case L';':
								{
									// This is a short form with the same style for normal/selected cases.
									fragm_len++;
									slct_value = norm_value;
									return(ic_change_style);
								}
								break;
					}
				}
				break;

		case L'R': case L'r':
				{
					fragm_len++;
					if (fragm_len >= src_len || (src_str[fragm_len+1] != L'S' && src_str[fragm_len+1] != L's'))
						return(ic_none);

					if (fragm_len < src_len && src_str[fragm_len] == L';')
					{
						fragm_len++;
						return(ic_reset_style);
					}
				}
				break;
	}

	return(ic_none);
}

long TBasicListBoxItem::ProcessScriptedText(HDC hDC, TBasicListBoxObject *ptr_owner, bool proc_mode_paint, RECT &rct, int initial_frgr_style_inx,
											bool selected_state, TStringPtr &text_ptr)
{
	wchar_t *src_str = text_ptr.GetBodyPtr();
	int src_len = text_ptr.GetLength();

	int curr_style = initial_frgr_style_inx;
	long total_width = 0;

	while (src_len > 0 && rct.left < rct.right)
	{
		// Find the length of the next fragment and try to process it inline.
		int fragm_len = 0;
		bool	fragm_processed = FALSE;

		if (src_str[0] != L'@')
		{
			// This is a simple piece of text.
			fragm_len = 1;
			while (fragm_len < src_len)
			{
				if (src_str[fragm_len] == L'@')
					break;
				fragm_len++;
			}
		}
		else if (src_len >= 2 && src_str[1] == L'@')
		{
			// This is a double at, paint it as a single at.
			src_str++;
			src_len--;
			fragm_len = 1;
		}
		else
		{
			// This seems to be a beginning of the inline control.
			short norm_value, slct_value;
			switch (ScanScriptedTextCtrlInfo(src_str, src_len, fragm_len, norm_value, slct_value))
			{
				case ic_draw_icon:
						{
							short icon_index = (selected_state == TRUE) ? slct_value : norm_value;

							// Repicking the width info might be slow, but this feature should not be heavily used.
							POINT icon_pos; SIZE icon_size;
							ptr_owner->m_icons_style.GetLocalRect(icon_index, icon_pos, icon_size);

							if (proc_mode_paint == FALSE)
							{
								total_width += icon_size.cx;
							}
							else
							{
								ptr_owner->m_icons_style.PaintBitmapEx(hDC, (POINT&)rct, icon_index, icon_pos, icon_size);
								rct.left += icon_size.cx;
							}
							fragm_processed = TRUE;
						}
						break;

				case ic_change_style:
						{
							curr_style = (selected_state == TRUE) ? slct_value : norm_value;
							if (curr_style < 0)
								curr_style = 0;
							if (curr_style >= TBasicListBoxObject::TLB_MAX_STYLES)
								curr_style = TBasicListBoxObject::TLB_MAX_STYLES-1;
							fragm_processed = TRUE;
						}
						break;

				case ic_reset_style:
						{
							curr_style  = initial_frgr_style_inx;
							fragm_processed = TRUE;
						}
						break;
			}
		}

		// Paint the fragment if it is not processed yet.
		if (fragm_processed == FALSE)
		{
			long fragm_width = ptr_owner->m_frgr_styles[curr_style].GetStringWidth(hDC, src_str, fragm_len);
			if (proc_mode_paint == FALSE)
			{
				total_width += fragm_width;
			}
			else
			{
				long delta = m_baseline_offs-ptr_owner->m_frgr_styles[curr_style].HeightAbove();
				rct.top += delta;
				rct.bottom += delta;
				ptr_owner->m_frgr_styles[curr_style].DrawFrgrText(hDC, src_str, fragm_len, rct);
				rct.top -= delta;
				rct.bottom -= delta;
				rct.left += fragm_width;
			}
		}

		// Take out the fragment from the source string.
		src_str += fragm_len;
		src_len -= fragm_len;
	}

	return(total_width);
}

//-----------------------------------------------------------------------------
//  ===================  TBasicListBoxObject  ========================
//-----------------------------------------------------------------------------

bool TBasicListBoxObject::m_control_classes_registered = FALSE;

TBasicListBoxObject::TBasicListBoxObject()
{
	m_title_msg_active = FALSE;
	m_list_msg_active = FALSE;

	// Only the window handles need cleaning. All other fields have default ctors.
	m_hTitleControl = NULL;
	m_hListBodyControl = NULL;
	m_hVertScroll = NULL;
}

TBasicListBoxObject::~TBasicListBoxObject()
{
	// Break the link with the control windows if they were present. All style objects have dectors.
	// They will take care of themselves.
	SetWindow(NULL);
	SetTitleWindow(NULL);
}

void TBasicListBoxObject::SetWindow(HWND hListBodyControl)
{
	if (hListBodyControl != NULL)
	{
		#ifdef _DEBUG
			// Ensure that passed window has the right name of its window class.
			wchar_t class_name_buff[80];
			assert(::GetClassNameW(hListBodyControl, class_name_buff, 80) == wcslen(DLG_CTRL_LIST_BOX_BODY));
			assert(wcscmp(class_name_buff, DLG_CTRL_LIST_BOX_BODY) == 0);
		#endif

		// Accept the window handle.
		m_hListBodyControl = hListBodyControl;
		::SendMessage(m_hListBodyControl, WM_USER, 0, (LPARAM)this);
		ProcessResizeEvent();
	}
	else
	{
		// Reset the link between the object and the custom control window.
		if (m_hListBodyControl != NULL)
		{
			::SendMessage(m_hListBodyControl, WM_USER, 0, NULL);
			m_hListBodyControl = NULL;
		}

		// Change list vars to the state without window. Selection keeps its state because data is still existing.
		SetupListAreaHeight(0);
	}
}

void TBasicListBoxObject::SetTitleWindow(HWND hTitleControl)
{
	if (hTitleControl != NULL)
	{
		#ifdef _DEBUG
			// Ensure that passed window has the right name of its window class.
			wchar_t class_name_buff[80];
			assert(::GetClassNameW(hTitleControl, class_name_buff, 80) == wcslen(DLG_CTRL_LIST_BOX_TITLE));
			assert(wcscmp(class_name_buff, DLG_CTRL_LIST_BOX_TITLE) == 0);
		#endif

		// Accept the window handle.
		m_hTitleControl = hTitleControl;
		::SendMessage(m_hTitleControl, WM_USER, 0, (LPARAM)this);
		::InvalidateRect(m_hTitleControl, NULL, TRUE);
	}
	else
	{
		// Reset the link to the listbox header window.
		if (m_hTitleControl != NULL)
		{
			::SendMessage(m_hTitleControl, WM_USER, 0, NULL);
			m_hTitleControl = NULL;
		}
	}
}

void TBasicListBoxObject::SetupSystemStdStyles(short ext_up, short ext_dn, TTextStyleSymbolAdjustInfo *adjust_data)
{
	// Pick up the font of the list control.
	TTextStyleProps frgr_props;
	RetrieveSystemFontProps(frgr_props);

	// Apply the extensions.
	frgr_props.up_side_adjust = ext_up;
	frgr_props.down_side_adjust = ext_dn;

	// Setup the first 3 std foreground styles.
	frgr_props.foreground_color = ::GetSysColor(COLOR_WINDOWTEXT);
	SetupFrgrStyleSlot(lbst_item_normal, frgr_props, adjust_data);
	frgr_props.foreground_color = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	SetupFrgrStyleSlot(lbst_item_selected, frgr_props, adjust_data);
	frgr_props.foreground_color = ::GetSysColor(COLOR_WINDOWTEXT);
	SetupFrgrStyleSlot(lbst_title_item, frgr_props, adjust_data);

	// Setup the first 3 std background styles.
	SetupBkgrStyleSlot(lbst_item_normal, ::GetSysColor(COLOR_WINDOW));
	SetupBkgrStyleSlot(lbst_item_selected, ::GetSysColor(COLOR_HIGHLIGHT));
	SetupBkgrStyleSlot(lbst_title_item, ::GetSysColor(COLOR_3DFACE));

	// Setup the color of the empty space.
	SetupNoItemsSpaceColor(::GetSysColor(COLOR_WINDOW));
}

bool TBasicListBoxObject::SetupFrgrStyleSlot(int istyle, const wchar_t *fnt_name, int fnt_height, bool fnt_bold, TColor frgr_color, short ext_up, short baseline_shift, short ext_dn)
{
	assert(istyle >= 0 && istyle < TLB_MAX_STYLES);
	TTextStyleProps props;
	props.Init(fnt_name, fnt_height, fnt_bold, frgr_color);
	props.up_side_adjust = ext_up;
	props.baseline_adjust = baseline_shift;
	props.down_side_adjust = ext_dn;
	return(SetupFrgrStyleSlot(istyle, props));
}

bool TBasicListBoxObject::SetupFrgrStyleSlot(int istyle, TColor frgr_color, short ext_up, short baseline_shift, short ext_dn)
{
	assert(istyle >= 0 && istyle < TLB_MAX_STYLES);
	TTextStyleProps props;
	RetrieveSystemFontProps(props);
	props.foreground_color = frgr_color;
	props.up_side_adjust = ext_up;
	props.baseline_adjust = baseline_shift;
	props.down_side_adjust = ext_dn;
	return(SetupFrgrStyleSlot(istyle, props));
}

bool TBasicListBoxObject::SetupFrgrStyleSlot(int istyle, TTextStyleProps &props, TTextStyleSymbolAdjustInfo *adjust_data)
{
	assert(istyle >= 0 && istyle < TLB_MAX_STYLES);
	if (m_frgr_styles[istyle].Setup(props, adjust_data) == FALSE)
		return(FALSE);

	return(m_frgr_styles[istyle].SetupForDeviceContextEx(m_hListBodyControl));
}

void TBasicListBoxObject::SetupBkgrStyleSlot(int istyle, TColor bkgr_color)
{
	assert(istyle >= 0 && istyle < TLB_MAX_STYLES);
	TBasicStyleProps props;
	props.Init(bkgr_color);
	m_bkgr_styles[istyle].Setup(props);
	m_bkgr_styles[istyle].SetupForDeviceContextEx(m_hListBodyControl);
}

void TBasicListBoxObject::SetupBkgrStyleSlot(int istyle, TBasicStyleProps &props)
{
	assert(istyle >= 0 && istyle < TLB_MAX_STYLES);
	m_bkgr_styles[istyle].Setup(props);
	m_bkgr_styles[istyle].SetupForDeviceContextEx(m_hListBodyControl);
}

bool TBasicListBoxObject::SetupIconsStyle(HINSTANCE hInst, TIconsGridStyleProps &props)
{
	if (m_icons_style.Setup(hInst, props) == FALSE)
		return(FALSE);

	return(m_icons_style.SetupForDeviceContextEx(m_hListBodyControl));
}

void TBasicListBoxObject::SetupNoItemsSpaceColor(TColor no_items_color)
{
	TBasicStyleProps props;
	props.Init(no_items_color);
	m_no_items_space_style.Setup(props);
	m_no_items_space_style.SetupForDeviceContextEx(m_hListBodyControl);
}

void TBasicListBoxObject::SetupNoItemsSpaceColor(TBasicStyleProps &props)
{
	m_no_items_space_style.Setup(props);
	m_no_items_space_style.SetupForDeviceContextEx(m_hListBodyControl);
}

void TBasicListBoxObject::SetupColumnProps(TListBoxColumnProps *records)
{
	m_horz_helper.SetColumnProps(records);
	if (m_hListBodyControl != NULL)
	{
		RECT rcListArea;
		GetListRect(rcListArea, FALSE);
		m_horz_helper.UpdateColumnWidths(rcListArea.right-rcListArea.left);
	}
}

void TBasicListBoxObject::SetColumnAlignment(int iclmn, TObjectAlignment new_alignment)
{
	m_horz_helper.SetColumnAlignment(iclmn, new_alignment);

	// The layout has changed, repaint the whole window.
	if (m_hListBodyControl != NULL)
		::InvalidateRect(m_hListBodyControl, NULL, TRUE);
	if (m_hTitleControl != NULL)
		::InvalidateRect(m_hTitleControl, NULL, TRUE);
}

void TBasicListBoxObject::SetupColumnTitles(TListBoxColumnTilteProps *records)
{
	// Reset the title.
	m_title_item.ClearBkgrAndCells(*this);

	// Setup the background.
	m_title_item.SetBkgrStyle(*this, lbst_title_item, lbst_title_item);

	// Setup the foregrounds.
	for (int iclmn=0; iclmn<TListBoxHorzHelper::LBX_MAX_COLUMNS; ++iclmn, ++records)
	{
		if (records->title_clmn_width <= 0)
			break;

		m_title_item.SetFrgrStyleToCell(*this, iclmn, lbst_title_item, lbst_title_item);

		if (records->title_scripted_text == FALSE)
			m_title_item.SetExtStringToCell(*this, iclmn, records->title_clmn_text);
		else m_title_item.SetExtScriptedStringToCell(*this, iclmn, records->title_clmn_text);

		if (records->title_clmn_width > 1)
		{
			m_title_item.SetCellWidth(*this, iclmn, records->title_clmn_width);
			iclmn += records->title_clmn_width-1;
		}
	}

	// It is necessary to refresh the baseline value in the item.
	m_title_item.MeasureItemHeight(this);

	if (m_hTitleControl != NULL)
		::InvalidateRect(m_hTitleControl, NULL, TRUE);
}

void TBasicListBoxObject::SetupColumnTitleSuffix(int iclmn, const wchar_t *suffix)
{
	assert(suffix != NULL && suffix[0] != 0);

	int len_suff = wcslen(suffix);
	assert(len_suff <= 80);

	bool smth_changed = FALSE;
	for (int ic = 0; ic < GetNumColumns(); ++ic)
	{
		// Pick up the current title of the column.
		wchar_t buffer[128];
		m_title_item.GetStrPtr(ic).CopyWithTruncationTo(buffer, 128-len_suff-1);

		// Look for the suffix in this title.
		wchar_t *ptr_existing_suffix = NULL;
		wchar_t *ptr_suffix = wcsstr(buffer, suffix);
		while (ptr_suffix != NULL)
		{
			ptr_existing_suffix = ptr_suffix;
			ptr_suffix = wcsstr(ptr_suffix+1, suffix);
		}

		if (ic == iclmn && ptr_existing_suffix == NULL)
		{
			// Append the suffix that is not there.
			wcscat(buffer, suffix);
			m_title_item.SetScriptedStringToCell(*this, ic, buffer);
			smth_changed = TRUE;
		}
		else if (ic != iclmn && ptr_existing_suffix != NULL)
		{
			// Remove the existing suffix.
			*ptr_existing_suffix = 0;
			m_title_item.SetScriptedStringToCell(*this, ic, buffer);
			smth_changed = TRUE;
		}
	}

	if (smth_changed == TRUE)
	{
		m_title_item.MeasureItemHeight(this);
		::InvalidateRect(m_hTitleControl, NULL, TRUE);
		::UpdateWindow(m_hTitleControl);
	}
}

void TBasicListBoxObject::RegisterWindowClasses(HINSTANCE hInst)
{
	if (m_control_classes_registered == TRUE)
		return;

	// Fill in the common props.
	WNDCLASSW wcInfo;
	wcInfo.style = CS_DBLCLKS;
	wcInfo.cbClsExtra = 0;
	wcInfo.cbWndExtra = 16;
	wcInfo.hInstance = (hInst != NULL) ? hInst : ::GetModuleHandle(NULL);
	wcInfo.hIcon = NULL;
	wcInfo.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcInfo.lpszMenuName = NULL;

	// Register window class for the list box title.
	wcInfo.lpfnWndProc = ListTitleWindowProc;
	wcInfo.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
	wcInfo.lpszClassName = DLG_CTRL_LIST_BOX_TITLE;
	::RegisterClassW(&wcInfo);

	// Register window class for the list box body.
	wcInfo.lpfnWndProc   = ListBodyWindowProc;
	wcInfo.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
	wcInfo.lpszClassName = DLG_CTRL_LIST_BOX_BODY;
	::RegisterClassW(&wcInfo);

	m_control_classes_registered = TRUE;
}

void TBasicListBoxObject::ScrollListArea(int area_beg, int area_len, int shift_val, bool update_now)
{
	assert(m_hListBodyControl != NULL);
	assert(area_len > 0 && shift_val != 0);

	if (m_curr_list_area_height <= 0)
		return;

	if (area_beg+area_len > m_curr_list_area_height)
	{
		assert(area_beg >= 0 && area_beg < m_curr_list_area_height);
		area_len = m_curr_list_area_height-area_beg;
	}

	RECT list_rect;
	GetListRect(list_rect, FALSE);

	// Move window pixels. Positive shift_val moves the passed area down.
	RECT scroll_rect = list_rect;
	scroll_rect.top += area_beg;
	scroll_rect.bottom = scroll_rect.top+area_len;
	::ScrollWindowEx(m_hListBodyControl, 0, shift_val, &scroll_rect, NULL, NULL, NULL, SW_INVALIDATE);

	// Invalidate the revealed areas manually.
	if (shift_val > 0)
	{
		scroll_rect.bottom = scroll_rect.top+shift_val;
		::InvalidateRect(m_hListBodyControl, &scroll_rect, TRUE);
	}

	if (shift_val < 0)
	{
		scroll_rect.top = scroll_rect.bottom+shift_val;
		::InvalidateRect(m_hListBodyControl, &scroll_rect, TRUE);
	}

	if (update_now == TRUE)
		::UpdateWindow(m_hListBodyControl);
}

void TBasicListBoxObject::InvalidateListArea(int area_beg, int area_len)
{
	assert(m_hListBodyControl != NULL);

	RECT list_rect;
	GetListRect(list_rect, FALSE);

	if (area_beg > 0)
		list_rect.top += area_beg;
	if (area_len > 0)
		list_rect.bottom = list_rect.top+area_len;

	::InvalidateRect(m_hListBodyControl, &list_rect, TRUE);
}

void TBasicListBoxObject::UpdateWindows()
{
	if (m_hTitleControl != NULL)
		::UpdateWindow(m_hTitleControl);

	if (m_hListBodyControl != NULL)
		::UpdateWindow(m_hListBodyControl);
}

void TBasicListBoxObject::SetupVertScroller(int nMax, int nPage, int nPos)
{
	assert(m_hListBodyControl != NULL);
	assert(m_curr_list_area_height > 0);

	RECT rcListArea;
	GetListRect(rcListArea, TRUE);

	int scroller_width = m_props.vert_scroller_width;
	if (scroller_width <= 0)
		scroller_width = ::GetSystemMetrics(SM_CXVSCROLL);
	scroller_width = __min(scroller_width, rcListArea.right-rcListArea.left);

	if (m_hVertScroll == NULL)
	{
		// Base class wants a non NULL scroller.
		m_hVertScroll = ::CreateWindowW(L"SCROLLBAR",							// Class name.
								NULL,										// Window title.
								WS_CHILD | WS_VISIBLE | WS_DISABLED | SBS_VERT,
								rcListArea.right-scroller_width, rcListArea.top,		// Position.
								scroller_width, m_curr_list_area_height,			// Size.
								m_hListBodyControl,							// Parent window.
								NULL, (HINSTANCE)::GetWindowLongPtr(m_hListBodyControl, GWLP_HINSTANCE), NULL);

		if (m_hVertScroll != NULL)
		{
			::InvalidateRect(m_hListBodyControl, NULL, TRUE);
			if (m_hTitleControl != NULL)
				::InvalidateRect(m_hTitleControl, NULL, TRUE);
		}
	}
	else
	{
		// Pick up the latest scroller rect.
		RECT rc_scroll;
		::GetClientRect(m_hVertScroll, &rc_scroll);
		if (rc_scroll.right -rc_scroll.left != scroller_width)
		{
			// Move/resize the scroller in the horz direction.
			::SetWindowPos(m_hVertScroll, NULL,
						rcListArea.right-scroller_width, rcListArea.top,
						scroller_width, m_curr_list_area_height,
						SWP_NOZORDER);
		}
	}

	if (m_hVertScroll != NULL)
	{
		if (nMax > 0)
		{
			// Enable the scroller and set all its props.
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
			si.nMin = 0;
			si.nMax = nMax;
			si.nPage = nPage;
			si.nPos = nPos;
			::EnableWindow(m_hVertScroll, TRUE);
			::SetScrollInfo(m_hVertScroll, SB_CTL, &si, TRUE);
		}
		else
		{
			// Disable the sroll bar.
			::EnableWindow(m_hVertScroll, FALSE);
		}
	}
}

void TBasicListBoxObject::DisposeVertScroller()
{
	if (m_hVertScroll != NULL)
	{
		// Get rid of the scroller.
		::DestroyWindow(m_hVertScroll);
		m_hVertScroll = NULL;

		if (m_hListBodyControl != NULL)
			::InvalidateRect(m_hListBodyControl, NULL, TRUE);
		if (m_hTitleControl != NULL)
			::InvalidateRect(m_hTitleControl, NULL, TRUE);
	}
}

DWORD TBasicListBoxObject::ShowAndTrackPopupMenu(TMenuItemInfo *menu_info, TListBoxMouseEventInfo *click_event_mouse_info)
{
	HMENU hMenu = TControlHelper::CreatePopupMenuHandle(menu_info);
	if (hMenu ==  NULL)
		return(0);

	RECT rc_ctrl;
	HWND hControl = (click_event_mouse_info->main_list_event == FALSE) ? m_hTitleControl : m_hListBodyControl;
	::GetWindowRect(hControl, &rc_ctrl);

	//
	//  Do not step over this system function in the debugger.
	//  Testing shows that this will prevent menu from opening.
	//
	DWORD res = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_BOTTOMALIGN,
								rc_ctrl.left+click_event_mouse_info->pos_x, rc_ctrl.top+click_event_mouse_info->pos_y, 0, hControl, NULL);

	::DestroyMenu(hMenu);
	return(res);
}

void TBasicListBoxObject::GetListRect(RECT &rc, bool include_scroller_area)
{
	// Pick up the client rect first.
	assert(m_hListBodyControl != NULL);
	::GetClientRect(m_hListBodyControl, &rc);

	if (m_props.use_inset_frame == TRUE)
	{
		int wf = TControlHelper::GetInsetFrameWidth();
		rc.left += wf;
		rc.right -= wf;
		rc.top += wf;
		rc.bottom -= wf;
	}

	if (include_scroller_area == FALSE && m_hVertScroll != NULL)
	{
		RECT scroller_rect;
		::GetClientRect(m_hVertScroll, &scroller_rect);
		rc.right -= (scroller_rect.right-scroller_rect.left);
	}

	if (rc.right < rc.left)
		rc.right = rc.left;
	if (rc.bottom < rc.top)
		rc.bottom = rc.top;
}

void TBasicListBoxObject::PaintTitleControl(HDC hDC, RECT &invalidRect)
{
	RECT rc;
	::GetClientRect(m_hTitleControl, &rc);

	if (m_props.use_inset_frame == TRUE)
	{
		rc.left += TControlHelper::GetInsetFrameWidth();
		rc.right -= TControlHelper::GetInsetFrameWidth();
	}

	if (m_hVertScroll != NULL)
	{
		RECT scroller_rect;
		::GetClientRect(m_hVertScroll, &scroller_rect);
		rc.right -= (scroller_rect.right-scroller_rect.left);
	}

	m_title_item.PaintItem(hDC, this, rc);
}

void TBasicListBoxObject::PaintListBodyControl(HDC hDC, RECT &invalidRect)
{
	assert(m_hListBodyControl != NULL);

	// Check, if the inset frame needs paining.
	if (m_props.use_inset_frame == TRUE)
	{
		RECT client_rect;
		::GetClientRect(m_hListBodyControl, &client_rect);
		int wf = TControlHelper::GetInsetFrameWidth();
		if (invalidRect.left < wf || invalidRect.top < wf || invalidRect.right > client_rect.right-wf || invalidRect.bottom > client_rect.bottom-wf)
		{
			// The invalid rect intersects with the inset frame.
			TControlHelper::PaintInsetFrame(hDC, client_rect);
		}
	}

	RECT list_rect;
	GetListRect(list_rect, FALSE);
	if (list_rect.right > list_rect.left && list_rect.bottom > list_rect.top)
	{
		// Prepare the clipping rect.
		int dc_inx = ::SaveDC(hDC);
		::IntersectClipRect(hDC, list_rect.left, list_rect.top, list_rect.right, list_rect.bottom);

		// Paint visible items.
		RECT item_rect = list_rect;
		item_rect.bottom = item_rect.top;
		if (m_num_visible_items > 0)
		{
			TListBoxFrgrItemsListIter iter(this);
			do
			{
				TBasicListBoxItem *item_to_paint = iter.CurrItem();
				int item_hgt = iter.CurrItemHeight();
				assert(item_hgt > 0);

				// Process current item.
				item_rect.bottom = item_rect.top+item_hgt;
				if (item_rect.bottom > invalidRect.top)
				{
					if (item_to_paint != NULL)
					{
						// Paint this item.
						item_to_paint->PaintItem(hDC, this, item_rect);
					}
					else
					{
						// Fill this rect with the dummy color.
						HBRUSH hBkgr = ::CreateSolidBrush(GetMissingItemColor(GetItemSelection(NULL, iter.CurrPos())));
						::FillRect(hDC, &item_rect, hBkgr);
						::DeleteObject(hBkgr);
					}
				}

				if (item_rect.bottom >= invalidRect.bottom)
					break;

				// Step to the next item.
				item_rect.top = item_rect.bottom;
			}
			while (iter.StepIterDown() == TRUE);
		}

		// Check, if the bottom part of the list area should be painted.
		if (item_rect.bottom < invalidRect.bottom)
		{
			item_rect.top = item_rect.bottom;
			item_rect.bottom = list_rect.bottom;
			HBRUSH hBkgr = ::CreateSolidBrush(m_no_items_space_style.GetBkgrColor());
			::FillRect(hDC, &item_rect, hBkgr);
			::DeleteObject(hBkgr);
		}

		// Restore the clipping.
		if (dc_inx != 0)
			::RestoreDC(hDC, dc_inx);
	}
}

void TBasicListBoxObject::ProcessVertScroll(int windows_scroll_event_wparam)
{
	if (m_num_items <= 0 || m_curr_list_area_height <= 0)
		return;

	// Prepare the new scroll position candidate.
	int pos = m_upper_item_inx;

	// The number of fully visible items is the same to the height of the window as the scroller
	// should understand it.
	int num_fully_visib = m_num_visible_items;
	if (m_visible_items_height > m_curr_list_area_height)
		num_fully_visib--;
	if (num_fully_visib == 0)
		num_fully_visib = 1;

	switch (windows_scroll_event_wparam)
	{
		case SB_LEFT:
				pos = 0;
				break;

		case SB_RIGHT:
				pos = m_num_items-num_fully_visib;
				break;

		case SB_LINELEFT:
				pos -= num_fully_visib/20;
				if (pos == m_upper_item_inx)
					pos--;
				break;

		case SB_LINERIGHT:
				pos += num_fully_visib/20;
				if (pos == m_upper_item_inx)
					pos++;
				break;

		case SB_PAGELEFT:
				pos -= (19*num_fully_visib)/20;
				if (pos == m_upper_item_inx)
					pos--;
				break;

		case SB_PAGERIGHT:
				pos += (19*num_fully_visib)/20;
				if (pos == m_upper_item_inx)
					pos++;
				break;

		case SB_THUMBTRACK:		// wparam == 5.
		case SB_THUMBPOSITION:		// wparam == 4.
				{
					SCROLLINFO si;
					si.cbSize = sizeof(si);
					si.fMask  = SIF_ALL;
					::GetScrollInfo(m_hVertScroll, SB_CTL, &si);
					pos = si.nTrackPos;
				}
				break;

		case SB_ENDSCROLL:			// wparam == 8.
		default:
			return;
	}

	// Setup the new scrolling position. This method will also fix the situation if the passed new position
	// will be too big or too small.
	SetTopIndex(pos);
	assert(m_upper_item == NULL || m_upper_item->ListBoxIndex() == m_upper_item_inx);

#ifdef DEEP_LIST_BOX_DEBUG
	if (m_upper_item != NULL && m_items_table == NULL)
	{
		int visib_hgt = 0;
		TListBoxLocalItemsListIter iter(this, m_upper_item);
		for (int inx =0; inx<m_num_visible_items; ++inx, ++iter)
			visib_hgt += iter.CurrItemHeight();

		assert(visib_hgt == m_visible_items_height);
	}
#endif
}

void TBasicListBoxObject::ProcessResizeEvent()
{
	// Check if window is available or not.
	if (m_hListBodyControl == NULL)
		return;

	// Prepare rects for the curr state of the window.
	RECT rcListArea;
	GetListRect(rcListArea, TRUE);

	// Let the base class to setup its variables and the scroller.
	SetupListAreaHeight(rcListArea.bottom-rcListArea.top);

	int new_width = rcListArea.right-rcListArea.left;
	if (m_hVertScroll != NULL)
	{
		// Check, if the width of the scroller should be changed.
		if (new_width == 0)
		{
			// Get rid of the scroller.
			DisposeVertScroller();
		}
		else
		{
			int scroller_width = m_props.vert_scroller_width;
			if (scroller_width <= 0)
				scroller_width = ::GetSystemMetrics(SM_CXVSCROLL);
			scroller_width = __min(scroller_width, new_width);
			new_width -= scroller_width;

			// Move/resize the scroller in the horz direction.
			assert(scroller_width > 0);
			::SetWindowPos(m_hVertScroll, NULL,
						rcListArea.right-scroller_width, rcListArea.top,
						scroller_width, m_curr_list_area_height,
						SWP_NOZORDER);
		}
	}

	// Horz layout of the columns has changed.
	m_horz_helper.UpdateColumnWidths(new_width);

	// Layout has changed, repaint the whole window.
	::InvalidateRect(m_hListBodyControl, NULL, TRUE);
	if (m_hTitleControl != NULL)
		::InvalidateRect(m_hTitleControl, NULL, TRUE);
}

void TBasicListBoxObject::ProcessTitleMouseClick(TListBoxNotificationCode evt, WPARAM wParam, LPARAM lParam)
{
	HWND hParent = ::GetParent(m_hTitleControl);
	if (hParent == NULL)
		return;

	// Convert passed mouse pos into the client coordinates.
	POINT mouse_pos = { LOWORD(lParam), HIWORD(lParam) };
	TListBoxMouseEventInfo info = { FALSE, evt, this, (DWORD)wParam, mouse_pos.x, mouse_pos.y };
	info.clicked_item = &m_title_item;
	info.clicked_item_index = m_title_item.ListBoxIndex();
	info.inside_item_rect = TRUE;

	// Figure out the situation with the clicked column.
	int offs_x = (m_props.use_inset_frame == TRUE) ? mouse_pos.x-TControlHelper::GetInsetFrameWidth() : mouse_pos.x;
	info.clmn_index = m_horz_helper.GetColumnFromOffset(info.clicked_item, offs_x, info.inside_the_column);

	// Send notification to the parent window.
	long wndID = ::GetWindowLong(m_hTitleControl, GWL_ID);
	::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(wndID, evt), (LPARAM)&info);
}

void TBasicListBoxObject::ProcessListBodyMouseClick(TListBoxNotificationCode evt, WPARAM wParam, LPARAM lParam)
{
	HWND hParent = ::GetParent(m_hListBodyControl);
	if (hParent == NULL)
		return;

	RECT list_rect;
	GetListRect(list_rect, FALSE);
	POINT mouse_pos = { LOWORD(lParam), HIWORD(lParam) };
	if (mouse_pos.x < list_rect.left || mouse_pos.x >= list_rect.right || mouse_pos.y < list_rect.top || mouse_pos.y >= list_rect.bottom)
	{
		// The click does not belong to the list area.
		return;
	}

	// Look for the item with the passed vertical position.
	TListBoxMouseEventInfo info = { TRUE, evt, this, (DWORD)wParam, mouse_pos.x, mouse_pos.y };
	info.clicked_item_index = GetItemFromPoint(mouse_pos.y-list_rect.top, (TBasicListBoxItem**)&info.clicked_item, &info.inside_item_rect);
	if (info.clicked_item_index < 0)
	{
		// The clicked item is missing.
		return;
	}

	// Figure out the situation with the clicked column.
	info.clmn_index = m_horz_helper.GetColumnFromOffset(info.clicked_item, mouse_pos.x-list_rect.left, info.inside_the_column);

	// Send notification to the parent window.
	long wndID = ::GetWindowLong(m_hListBodyControl, GWL_ID);
	::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(wndID, evt), (LPARAM)&info);
}

void TBasicListBoxObject::ProcessMouseSelectionChangeClick(WPARAM wParam, LPARAM lParam)
{
	RECT list_rect;
	GetListRect(list_rect, FALSE);
	POINT mouse_pos = { LOWORD(lParam), HIWORD(lParam) };
	if (mouse_pos.x < list_rect.left || mouse_pos.x >= list_rect.right || mouse_pos.y < list_rect.top || mouse_pos.y >= list_rect.bottom)
	{
		// The click does not belong to the list area.
		return;
	}

	TListBoxSlctChangeInfoInternal info;
	if (ProcessSlctChangeClick(mouse_pos.y-list_rect.top, ((wParam & MK_SHIFT) != 0), ((wParam & MK_CONTROL) != 0), info) == TRUE)
	{
		// Base class has asked to send the notification.
		HWND hParent = ::GetParent(m_hListBodyControl);
		if (hParent != NULL)
		{
			// Allocate bigger structure and copy in the fields.
			TListBoxSelectionChangeInfo info_ex = { lbnc_slct_change, this };
			info_ex.new_selection = info.new_selection;
			info_ex.clicked_item = info.clicked_item;
			info_ex.clicked_item_inx = info.clicked_item_inx;
			info_ex.slct_chg_beg = info.slct_chg_beg;
			info_ex.slct_chg_len = info.slct_chg_len;

			WPARAM wParamVal = MAKEWPARAM(::GetWindowLong(m_hListBodyControl, GWL_ID), lbnc_slct_change);
			::SendMessage(hParent, WM_COMMAND, wParamVal, (LPARAM)&info_ex);
		}
	}
}

void TBasicListBoxObject::RetrieveSystemFontProps(TTextStyleProps &props)
{
	assert(m_hListBodyControl != NULL);
	HFONT hFont = (HFONT)::SendMessage(m_hListBodyControl, WM_GETFONT, 0, 0);
	assert(hFont != NULL);

	// Pick up the props of the font. It is not good to use the handle of the font directly because
	// DGI does not provide means for duplicating the font handles.
	HDC hDC = GetControlDC();
	assert(hDC != NULL);

	HFONT hOldFont = (HFONT)::SelectObject(hDC, hFont);
	wchar_t control_font_name[80];
	::GetTextFaceW(hDC, 80, control_font_name);
	TEXTMETRIC control_font_tm;
	::GetTextMetrics(hDC, &control_font_tm);

	// Dispose the resources.
	::SelectObject(hDC, hOldFont);
	ReleaseControlDC(hDC);

	// Prepare the props using the retrieved font info.
	int height = ::MulDiv((control_font_tm.tmHeight-control_font_tm.tmInternalLeading), 72, ::GetDeviceCaps(hDC, LOGPIXELSY));
	props.Init(control_font_name, height, (control_font_tm.tmWeight == FW_BOLD) ? TRUE : FALSE, RGB(0, 0, 0));
}

bool TBasicListBoxObject::CheckTitleWindowRecursion()
{
	if (m_title_msg_active == FALSE)
	{
		// The situation is fine.
		return(TRUE);
	}

#ifdef ASSERT_ON_WNDPROC_RECURSION
	assert(FALSE);
#endif

	// Avoid the recursive processing.
	return(FALSE);
}

bool TBasicListBoxObject::CheckListWindowRecursion()
{
	if (m_list_msg_active == FALSE)
	{
		// The situation is fine.
		return(TRUE);
	}

#ifdef ASSERT_ON_WNDPROC_RECURSION
	assert(FALSE);
#endif

	// Avoid the recursive processing.
	return(FALSE);
}

LRESULT CALLBACK TBasicListBoxObject::ListTitleWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TBasicListBoxObject *control_state = (TBasicListBoxObject*)(LONG_PTR)::GetWindowLongPtr(hWnd, 0);

	switch (msg)
	{
		case WM_CREATE:
				{
					// Reset the window long fields.
					::SetWindowLong(hWnd, 0, 0);
					::SetWindowLong(hWnd, 4, 0);
				}
				return(0);

		case WM_DESTROY:
				{
					if (control_state != NULL)
					{
						control_state->SetTitleWindow(NULL);
					}
				}

		case WM_SETFONT:
				{
					::SetWindowLong(hWnd, 4, (LONG)wParam);
					if (lParam != 0)
					{
						::InvalidateRect(hWnd, NULL, TRUE);
						::UpdateWindow(hWnd);
					}
				}
				break;

		case WM_GETFONT:
				{
					return(::GetWindowLong(hWnd, 4));
				}
				break;

		case WM_USER:
				{
					// App level decided to set or reset the control object pointer.
					::SetWindowLong(hWnd, 0, (LONG)lParam);
				}
				break;

		case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC   hDC = ::BeginPaint(hWnd, &ps);
					HFONT font = (HFONT)(LONG_PTR)::GetWindowLong(hWnd, 4);
					HFONT old_font = NULL;
					if (font != NULL)
						old_font = (HFONT)::SelectObject(hDC, font);

					if (control_state != NULL)
					{
						control_state->PaintTitleControl(hDC, ps.rcPaint);
					}
					else
					{
						::FillRect(hDC, &(ps.rcPaint), ::GetSysColorBrush(COLOR_WINDOW));

						RECT rc;
						::GetClientRect(hWnd, &rc);
						rc.bottom--;
						::DrawTextW(hDC, L" -- Helper is not set -- ", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
					}

					if (font != NULL)
						::SelectObject(hDC, old_font);

					::EndPaint(hWnd, &ps);
				}
				return(0);

		case WM_LBUTTONDOWN:
				{
					if (control_state != NULL && control_state->CheckTitleWindowRecursion() == TRUE)
					{
						control_state->m_title_msg_active = TRUE;
						control_state->ProcessTitleMouseClick(lbnc_left_btn_down, wParam, lParam);
						control_state->m_title_msg_active = FALSE;
					}
				}
				return(0);

		case WM_RBUTTONDOWN:
				{
					if (control_state != NULL && control_state->CheckTitleWindowRecursion() == TRUE)
					{
						control_state->m_title_msg_active = TRUE;
						control_state->ProcessTitleMouseClick(lbnc_right_btn_down, wParam, lParam);
						control_state->m_title_msg_active = FALSE;
					}
				}
				return(0);

		case WM_LBUTTONDBLCLK:
				{
					if (control_state != NULL && control_state->CheckTitleWindowRecursion() == TRUE)
					{
						control_state->m_title_msg_active = TRUE;
						control_state->ProcessTitleMouseClick(lbnc_left_btn_dblclk, wParam, lParam);
						control_state->m_title_msg_active = FALSE;
					}
				}
				return(0);
	}

	return(::DefWindowProc(hWnd, msg, wParam, lParam));
}

LRESULT CALLBACK TBasicListBoxObject::ListBodyWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TBasicListBoxObject *control_state = (TBasicListBoxObject*)(LONG_PTR)::GetWindowLongPtr(hWnd, 0);

	switch (msg)
	{
		case WM_CREATE:
				{
					// Reset the window long fields.
					::SetWindowLong(hWnd, 0, 0);
					::SetWindowLong(hWnd, 4, 0);
				}
				return(0);

		case WM_DESTROY:
				{
					if (control_state != NULL)
					{
						control_state->SetWindow(NULL);
					}
				}

		case WM_SETFONT:
				{
					::SetWindowLong(hWnd, 4, (LONG)wParam);
					if (lParam != 0)
					{
						::InvalidateRect(hWnd, NULL, TRUE);
						::UpdateWindow(hWnd);
					}
				}
				break;

		case WM_GETFONT:
				{
					return(::GetWindowLong(hWnd, 4));
				}
				break;

		case WM_USER:
				{
					// App level decided to set or reset the control object pointer.
					::SetWindowLong(hWnd, 0, (LONG)lParam);
				}
				break;

		case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC   hDC = ::BeginPaint(hWnd, &ps);
					HFONT font = (HFONT)(LONG_PTR)::GetWindowLong(hWnd, 4);
					HFONT old_font = NULL;
					if (font != NULL)
						old_font = (HFONT)::SelectObject(hDC, font);

					if (control_state != NULL)
					{
						control_state->PaintListBodyControl(hDC, ps.rcPaint);
					}
					else
					{
						::FillRect(hDC, &(ps.rcPaint), ::GetSysColorBrush(COLOR_WINDOW));

						RECT rc;
						::GetClientRect(hWnd, &rc);
						rc.top += 2;
						::DrawTextW(hDC, L" -- Helper is not set -- ", -1, &rc, DT_CENTER | DT_NOPREFIX);
					}

					if (font != NULL)
						::SelectObject(hDC, old_font);

					::EndPaint(hWnd, &ps);
				}
				return(0);

		case WM_VSCROLL:
				{
					::SetFocus(hWnd);
					if (control_state != NULL && lParam == (LPARAM)control_state->m_hVertScroll && control_state->CheckListWindowRecursion() == TRUE)
					{
						control_state->m_list_msg_active = TRUE;
						control_state->ProcessVertScroll(LOWORD(wParam));
						control_state->m_list_msg_active = FALSE;
					}
				}
				return(0);

		case WM_SIZE:
				{
					if (control_state != NULL && control_state->CheckListWindowRecursion() == TRUE)
					{
						control_state->m_list_msg_active = TRUE;
						control_state->ProcessResizeEvent();
						control_state->m_list_msg_active = FALSE;
					}
				}
				break;

		case WM_CHAR:
				{
					HWND hParent = ::GetParent(hWnd);
					if (hParent != NULL && control_state != NULL && control_state->CheckListWindowRecursion() == TRUE)
					{
						control_state->m_list_msg_active = TRUE;

						TListBoxKeyboardEventInfo info = { lbnc_wm_char, control_state, wParam, lParam };
						long wndID = ::GetWindowLong(hWnd, GWL_ID);
						::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(wndID, lbnc_wm_char), (LPARAM)&info);

						control_state->m_list_msg_active = FALSE;
					}
				}
				return(0);

		case WM_MOUSEMOVE:
				{
					if (::GetFocus() != hWnd)
						::SetFocus(hWnd);
				}
				break;

		case WM_MOUSEWHEEL:
				{
					// Use simple implementation for low resolution mouses as MSDN suggests.
					short zDeltaRaw = GET_WHEEL_DELTA_WPARAM(wParam);
					if (control_state != NULL && zDeltaRaw != 0 && control_state->CheckListWindowRecursion() == TRUE)
					{
						control_state->m_list_msg_active = TRUE;
						control_state->ProcessVertScroll((zDeltaRaw > 0) ? SB_LINELEFT : SB_LINERIGHT);
						control_state->m_list_msg_active = FALSE;
					}
				}
				return(0);

		case WM_LBUTTONDOWN:
				{
					::SetFocus(hWnd);
					if (control_state != NULL && control_state->CheckListWindowRecursion() == TRUE)
					{
						control_state->m_list_msg_active = TRUE;

						if (control_state->m_props.auto_select == TRUE)
							control_state->ProcessMouseSelectionChangeClick(wParam, lParam);
						else control_state->ProcessListBodyMouseClick(lbnc_left_btn_down, wParam, lParam);

						control_state->m_list_msg_active = FALSE;
					}
				}
				return(0);

		case WM_RBUTTONDOWN:
				{
					::SetFocus(hWnd);
					if (control_state != NULL && control_state->CheckListWindowRecursion() == TRUE)
					{
						control_state->m_list_msg_active = TRUE;
						control_state->ProcessListBodyMouseClick(lbnc_right_btn_down, wParam, lParam);
						control_state->m_list_msg_active = FALSE;
					}
				}
				return(0);

		case WM_LBUTTONDBLCLK:
				{
					::SetFocus(hWnd);
					if (control_state != NULL && control_state->CheckListWindowRecursion() == TRUE)
					{
						control_state->m_list_msg_active = TRUE;
						control_state->ProcessListBodyMouseClick(lbnc_left_btn_dblclk, wParam, lParam);
						control_state->m_list_msg_active = FALSE;
					}
				}
				return(0);
	}

	return(::DefWindowProc(hWnd, msg, wParam, lParam));
}


