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
#include  "ScreenItems/ScreenItemsGrid.H"

// -------------------------------------------------------------------------
//  =================  TScreenItemsGridItem  ======================
// -------------------------------------------------------------------------

TScreenItemsGridItem::TScreenItemsGridItem(TScreenItemStyle *bkgr_style)
{
	// Clear the invalid area.
	m_min_invalid_row = m_max_invalid_row_plus1 = -1;
	m_min_invalid_clmn = m_max_invalid_clmn_plus1 = -1;

	// Setup non NULL style to the m_style field of the base class to make the object paintable.
	m_default_bkgr_style.Setup(cs_transparent);
	m_style = &m_default_bkgr_style;
	m_clickable_bkgr = TRUE;

	// Setup the backgould style of the whole grid.
	m_bkgr_scr_item.SetStyle(bkgr_style);
}

void TScreenItemsGridItem::SetBoundingRectPos(long pos_x, long pos_y)
{
	// Convert new bounding rect pos into new hot spot pos.
	long hot_x = m_bkgr_scr_item.GetBaseLeft();
	long hot_y = m_bkgr_scr_item.GetBaseTop();
	SetHotSpotPos(pos_x+(hot_x-m_bounding_rect.left), pos_y+(hot_y-m_bounding_rect.top));
}

void TScreenItemsGridItem::SetHotSpotPos(long pos_x, long pos_y)
{
	assert(m_min_invalid_row == -1);
	assert(m_min_invalid_clmn == -1);

	// Iterate all rows.
	long offs_x = pos_x - m_bkgr_scr_item.GetBaseLeft();
	long offs_y = pos_y - m_bkgr_scr_item.GetBaseTop();
	for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
	{
		TScreenItemsGridRow &row = iter.CurrItem();
		if (row.m_scr_items_present == FALSE)
			continue;

		for (int iclmn=0; iclmn<NumColumns(); ++iclmn)
		{
			TScreenItem **pxi = row.m_cells[iclmn].m_scr_item;
			for (int il=0; il<SCREEN_ITEMS_GRID_NUM_LAYERS; ++il, ++pxi)
			{
				if (*pxi != NULL)
				{
					// Current layer of the cell needs resetup.
					long px, py;
					(*pxi)->GetHotSpotPos(px, py);
					(*pxi)->SetHotSpotPos(px+offs_x, py+offs_y);
				}
			}
		}

		ShiftRect(row.m_scr_items_rect, offs_x, offs_y);
	}

	// Update the overall rects.
	m_bkgr_scr_item.SetHotSpotPos(pos_x, pos_y);
	ShiftRcItem(offs_x, offs_y);
}

void TScreenItemsGridItem::ShiftItem(long shift_x, long shift_y)
{
	// Simply shift all non NULL screen items.
	for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
	{
		TScreenItemsGridRow &row = iter.CurrItem();
		if (row.m_scr_items_present == TRUE)
		{
			ShiftRect(row.m_scr_items_rect, shift_x, shift_y);
			for (int i=0; i<NumColumns(); ++i)
			{
				row.m_cells[i].ShiftScreenItems(shift_x, shift_y);
			}
		}
	}

	// Update the bkgr and the bounding rect.
	m_bkgr_scr_item.ShiftItem(shift_x, shift_y);
	ShiftRcItem(shift_x, shift_y);
}

void TScreenItemsGridItem::OnDraw(HDC hDC, RECT &invalid_appsp_rect)
{
	// Grid should be not empty.
	assert(NumRows() > 0);
	assert(NumColumns() > 0);

	//
	// Phase 1: Background painting.
	//
	RECT rc;
	TRectItem worker_rect;
	TScreenItemsGridFullClmnInfo *clmn_info;
	long ref_x = m_bkgr_scr_item.GetBaseLeft();
	long ref_y = m_bkgr_scr_item.GetBaseTop();
	int num_rows = NumRows();

	// (1.1) Paint whole object background.
	if (m_bkgr_scr_item.GetBasicStyle() != NULL)
	{
		// Non empty background is present.
		m_bkgr_scr_item.OnDraw(hDC, invalid_appsp_rect);
	}

	// (1.2) Paint the column backgrounds if any. This will also determine the range of invalid columns.
	m_bkgr_scr_item.GetBaseRect(rc);
	clmn_info = m_clmns_info.DataPtr();
	long invalid_clmn_min = -1, invalid_clmn_max = -1;
	for (int clmn1=0; clmn1<NumColumns(); ++clmn1, ++clmn_info)
	{
		rc.left = ref_x + clmn_info->m_clmn_offs;
		rc.right = rc.left + clmn_info->m_props.m_width;

		if (rc.right <= invalid_appsp_rect.left)
			continue;

		if (invalid_clmn_min == -1)
			invalid_clmn_min = clmn1;

		if (clmn_info->m_props.m_width > 0 && clmn_info->m_props.m_bkgr_style != NULL)
		{
			// Paint the column background.
			worker_rect.SetStyle(clmn_info->m_props.m_bkgr_style);
			worker_rect.SetBaseRect(rc);
			worker_rect.OnDraw(hDC, invalid_appsp_rect);
		}

		invalid_clmn_max = clmn1;
		if (rc.right >= invalid_appsp_rect.right)
			break;
	}

	// (1.3) Paint the row and cell backgrounds. Paint the row delimiters. This will also determine the range
	// of invalid rows.
	m_bkgr_scr_item.GetBaseRect(rc);
	long invalid_row_min = -1, invalid_row_max = -1;
	for (int row1=0; row1<num_rows; ++row1)
	{
		TScreenItemsGridRow &row = *m_row_ptrs[row1];

		// Set up rc to the rect of the row without delim.
		rc.top = ref_y + row.m_row_offs;
		rc.bottom = rc.top + row.m_props.m_height;

		if (rc.bottom+row.m_props.m_delim_height <= invalid_appsp_rect.top)
			continue;

		if (invalid_row_min == -1)
			invalid_row_min = row1;

		// Paint the row background.
		if (row.m_props.m_height > 0 && row.m_props.m_bkgr_style != NULL)
		{
			// Current row has background.
			worker_rect.SetStyle(row.m_props.m_bkgr_style);
			worker_rect.SetBaseRect(rc);
			worker_rect.OnDraw(hDC, invalid_appsp_rect);
		}

#ifdef SCREEN_ITEMS_GRID_WANT_CELLS_BKGR
		// Paint the cell backgrounds.
		if (invalid_clmn_min != -1)
		{
			// Invalid columns are present. It is ok to use min/max invalid clmn indexes because
			// cell backgrounds have the same horizontals positions to the column backgrounds.
			RECT rc_cell = rc;
			clmn_info = m_clmns_info.DataPtr()+invalid_clmn_min;
			for (int clmn2=invalid_clmn_min; clmn2 <= invalid_clmn_max; ++clmn2, ++clmn_info)
			{
				if (clmn_info->m_props.m_width > 0 && row.m_cells[clmn2].m_bkgr_style != NULL)
				{
					// Curr cell has background.
					rc_cell.left = ref_x + clmn_info->m_clmn_offs;
					rc_cell.right = rc_cell.left + clmn_info->m_props.m_width;
					worker_rect.SetStyle(row.m_cells[clmn2].m_bkgr_style);
					worker_rect.SetBaseRect(rc_cell);
					worker_rect.OnDraw(hDC, invalid_appsp_rect);
				}
			}
		}
#endif

		// Paint the row delimiter.
		rc.top = rc.bottom;
		rc.bottom = rc.top + row.m_props.m_delim_height;
		if (row.m_props.m_delim_height > 0 && row.m_props.m_bkgr_delim_style != NULL)
		{
			// Current row has delimiter.
			worker_rect.SetStyle(row.m_props.m_bkgr_delim_style);
			worker_rect.SetBaseRect(rc);
			worker_rect.OnDraw(hDC, invalid_appsp_rect);
		}

		invalid_row_max = row1;
		if (rc.bottom >= invalid_appsp_rect.bottom)
			break;
	}

	// (1.4) Paint the column delimiters.
	if (invalid_clmn_min != -1)
	{
		// Column delimiters should be painted on top of the row delimiters. Note that invalid_clmn_min is
		// not used because it is based on the column area only without considering the clmn delimiter area.
		m_bkgr_scr_item.GetBaseRect(rc);
		clmn_info = m_clmns_info.DataPtr();
		for (int clmn3=0; clmn3 <= invalid_clmn_max; ++clmn3, ++clmn_info)
		{
			rc.left = ref_x + clmn_info->m_clmn_offs + clmn_info->m_props.m_width;
			rc.right = rc.left + clmn_info->m_props.m_delim_width;

			if (rc.right <= invalid_appsp_rect.left)
				continue;

			if (clmn_info->m_props.m_delim_width > 0 && clmn_info->m_props.m_bkgr_delim_style != NULL)
			{
				// Current column has delimiter.
				worker_rect.SetStyle(clmn_info->m_props.m_bkgr_delim_style);
				worker_rect.SetBaseRect(rc);
				worker_rect.OnDraw(hDC, invalid_appsp_rect);
			}
		}
	}

	//
	// Phase 2: Screen items painting.
	//

	// (2.1) Construct affected rect based on the range of rows whoes backgrounds were repainted.
	rc = invalid_appsp_rect;
	if (invalid_row_min != -1)
	{
		// Add rows with repainted background to the affected area.
		for (int row2=invalid_row_min; row2 <= invalid_row_max; ++row2)
		{
			TScreenItemsGridRow &row = *m_row_ptrs[row2];
			if (row.m_scr_items_present == TRUE)
			{
				CombineRects(rc, row.m_scr_items_rect);
			}
		}
	}

	// (2.2) Finalize the range of rows whoes screen items should be painted.
	for(;;)
	{
		bool smth_added = FALSE;
		if (invalid_row_min == -1)
		{
			for (int row3=0; row3<num_rows; ++row3)
			{
				TScreenItemsGridRow &row = *m_row_ptrs[row3];
				if (row.m_scr_items_present == TRUE && CheckIntersection(rc, row.m_scr_items_rect) == TRUE)
				{
					// Current row has screen items that intersect with invalid area. Start the range
					// of affected rows from the rect of this row.
					invalid_row_min = invalid_row_max = row3;
					CombineRects(rc, row.m_scr_items_rect);
					smth_added = TRUE;
					break;
				}
			}
		}
		else
		{
			// Range of affected rows is already present. Test those lines that stay outside of this range.
			for (int row4=0; row4<invalid_row_min; ++row4)
			{
				TScreenItemsGridRow &row = *m_row_ptrs[row4];
				if (row.m_scr_items_present == TRUE && CheckIntersection(rc, row.m_scr_items_rect) == TRUE)
				{
					// Add whole range of rows starting from the current row up to the lower border of affected
					// rows to the range of affected rows.
					for (int row5=row4; row5<invalid_row_min; ++row5)
					{
						TScreenItemsGridRow &inner_row = *m_row_ptrs[row5];
						if (inner_row.m_scr_items_present == TRUE)
						{
							CombineRects(rc, inner_row.m_scr_items_rect);
						}
					}

					invalid_row_min = row4;
					smth_added = TRUE;
					break;
				}
			}

			for (int row6=invalid_row_max+1; row6<num_rows; ++row6)
			{
				TScreenItemsGridRow &row = *m_row_ptrs[row6];
				if (row.m_scr_items_present == TRUE && CheckIntersection(rc, row.m_scr_items_rect) == TRUE)
				{
					// Add range of rows starting from the higher border of affected rows range up to
					// the current row to the range of affected rows.
					for (int row7=invalid_row_max+1; row7 <= row6; ++row7)
					{
						TScreenItemsGridRow &inner_row = *m_row_ptrs[row7];
						if (inner_row.m_scr_items_present == TRUE)
						{
							CombineRects(rc, inner_row.m_scr_items_rect);
						}
					}

					invalid_row_max = row6;
					smth_added = TRUE;
				}
			}
		}

		if (smth_added == FALSE)
			break;
	}

	// (2.3) Check if there are screen items to paint or not.
	if (invalid_row_min != -1)
	{
		// Range of rows to paint is present. Paint layers from bottom to top.
		for (int il=SCREEN_ITEMS_GRID_NUM_LAYERS-1; il >= 0; --il)
		{
			for (int row8=invalid_row_min; row8 <= invalid_row_max; ++row8)
			{
				TScreenItemsGridRow *row = m_row_ptrs[row8];
				if (row->m_scr_items_present == FALSE)
					continue;

				for (int clmn4=0; clmn4<NumColumns(); ++clmn4)
				{
					TScreenItem *scr_item = row->m_cells[clmn4].m_scr_item[il];
					if (scr_item != NULL)
					{
						// Paint the screen item.
						scr_item->OnDraw(hDC, invalid_appsp_rect);
					}
				}
			}
		}
	}
}

TScreenItem *TScreenItemsGridItem::CheckClick(POINT &app_click_point)
{
	// Grid should be not empty.
	assert(NumRows() > 0);
	assert(NumColumns() > 0);

	// Scan the whole grid row by row. Check foreground layers from top to bottom.
	for (int il=0; il<SCREEN_ITEMS_GRID_NUM_LAYERS; ++il)
	{
		for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
		{
			TScreenItemsGridRow &row = iter.CurrItem();
			if (row.m_scr_items_present == FALSE || IsPointInRect(app_click_point, row.m_scr_items_rect) == FALSE)
				continue;

			// Current row has screen items and bounding rect of these screen items intersects with the passed point.
			for (int i=0; i<NumColumns(); ++i)
			{
				TScreenItem *scr_item = row.m_cells[i].m_scr_item[il];
				if (scr_item != NULL && scr_item->GetOwner() != NULL && IsPointInRect(app_click_point, *scr_item->GetBoundingRect()) == TRUE)
				{
					TScreenItem *clicked_scr_item = scr_item->CheckClick(app_click_point);
					if (clicked_scr_item != NULL)
					{
						// Clicked item is found.
						return(clicked_scr_item);
					}
				}
			}
		}
	}

	if (m_clickable_bkgr == TRUE)
	{
		RECT rc_bkgr;
		m_bkgr_scr_item.GetBaseRect(rc_bkgr);

		if (IsPointInRect(app_click_point, rc_bkgr) == TRUE)
			return(this);
	}

	// Nothing was found.
	return(NULL);
}

void TScreenItemsGridItem::Reset()
{
	// Cancel rows and columns info.
	m_clmns_info.SetNumItems(0);
	m_row_ptrs.SetNumItems(0);
	ReleaseRows();

	// Clear the invalid area.
	m_min_invalid_row = m_max_invalid_row_plus1 = -1;
	m_min_invalid_clmn = m_max_invalid_clmn_plus1 = -1;

	// Reset the overall rects.
	RECT rc = { 0, 0, 0, 0 };
	m_bkgr_scr_item.SetBaseRect(rc);
	m_bounding_rect = *m_bkgr_scr_item.GetBoundingRect();
}

bool TScreenItemsGridItem::AddColumn(int inx_before, TScreenItemsGridColumnInfo &props)
{
	if (inx_before >= 0)
		assert(inx_before <= NumColumns());

	// Allocate space for new column in the array of column infos and in the arrays of cells in all existing rows.
	if (m_clmns_info.ReserveSpace() == FALSE)
		return(FALSE);

	for (TListIter<TScreenItemsGridRow> iter1(m_rows); iter1; ++iter1)
	{
		if (iter1.CurrItem().m_cells.ReserveSpace() == FALSE)
			return(FALSE);
	}

	// Reserving space in all affected arrays succeeded. Adding elements to arrays below cannot fail now.
	TScreenItemsGridFullClmnInfo clmn_info;
	clmn_info.m_props = props;
	clmn_info.m_clmn_offs = 0;
	clmn_info.m_clmn_ready = FALSE;
	TScreenItemsGridCellData cell_data;
	cell_data.Clear();

	if (inx_before < 0 || inx_before >= NumColumns())
	{
		// Append new column to the right side of the table.
		m_clmns_info.AppendItem(clmn_info);
		for (TListIter<TScreenItemsGridRow> iter2(m_rows); iter2; ++iter2)
			iter2.CurrItem().m_cells.AppendItem(cell_data);

		inx_before = NumColumns()-1;
	}
	else
	{
		// Insert new column into the beginning or into the middle of the table.
		m_clmns_info.InsertItem(inx_before, clmn_info);
		for (TListIter<TScreenItemsGridRow> iter3(m_rows); iter3; ++iter3)
			iter3.CurrItem().m_cells.InsertItem(inx_before, cell_data);
	}

	// Add this column to the range of invalid columns.
	DeclareInvalidColumn(inx_before, TRUE);
	return(TRUE);
}

void TScreenItemsGridItem::DeleteColumn(int clmn_inx)
{
	// Ensure that index of the column is valid.
	assert(clmn_inx >= 0);
	assert(clmn_inx < NumColumns());

	// Update the data structures.
	m_clmns_info.ReleaseItem(clmn_inx);
	for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
	{
		TScreenItemsGridRow &row = iter.CurrItem();
		row.m_cells[clmn_inx].ReleaseScreenItems();
		row.m_cells.ReleaseItem(clmn_inx);
	}

	// Update range of invalid columns.
	if (m_min_invalid_clmn != -1)
	{
		// Update existing range.
		if (clmn_inx < m_min_invalid_clmn)
			m_min_invalid_clmn--;
		if (clmn_inx < m_max_invalid_clmn_plus1)
			m_max_invalid_clmn_plus1--;
	}
	else if (clmn_inx < NumColumns())
	{
		// Create new empty range.
		m_min_invalid_clmn = m_max_invalid_clmn_plus1 = clmn_inx;
	}
}

bool TScreenItemsGridItem::AddRow(int inx_before, TScreenItemsGridRowInfo &props, TScreenItemsGridBuffsArray *external_rows_info_storage)
{
	if (inx_before >= 0)
		assert(inx_before <= NumRows());

	// Allocate new row object, cells in it and ensure that there is free space in the array of row pointers.
	TScreenItemsGridRow *new_row = NULL;
	if (external_rows_info_storage == NULL)
	{
		// Use regular heap to allocate the object.
		new_row = new TScreenItemsGridRow(props);
	}
	else
	{
		// Caller wants special procedure.
		TScreenItemsGridRow row_proto(props);
		new_row = (TScreenItemsGridRow*)external_rows_info_storage->AddDwordAlignedData(&row_proto, sizeof(TScreenItemsGridRow));
		if (new_row != NULL)
		{
			// This object needs fixing and additional init.
			new_row->Clear();
			new_row->m_cells.FixObjectMove();
			new_row->m_xtrn_allocated_row = TRUE;
		}
	}

	if (new_row == NULL)
		return(FALSE);
	if (new_row->m_cells.ReserveSpace(NumColumns()) == FALSE)
		return(FALSE);
	if (m_row_ptrs.ReserveSpace() == FALSE)
		return(FALSE);

	// Clear all cells in the new row.
	new_row->m_cells.SetNumItems(NumColumns());
	for (int i=0; i<NumColumns(); ++i)
		new_row->m_cells[i].Clear();

	if (inx_before < 0 || inx_before >= NumRows())
	{
		// Append new row to the bottom of the table. There is no need to shift anything.
		m_rows.AppendItem(new_row);
		m_row_ptrs.AppendItem(new_row);
		inx_before = NumRows()-1;
	}
	else
	{
		// Insert new row at the top or into the middle of the table.
		m_rows.InsertBefore(new_row, m_row_ptrs[inx_before]);
		m_row_ptrs.InsertItem(inx_before, new_row);
	}

	// Add this row to the range of invalid rows.
	DeclareInvalidRow(inx_before, TRUE);
	return(TRUE);
}

void TScreenItemsGridItem::DeleteRow(int row_inx)
{
	// Ensure that passed index of the row is valid.
	assert(row_inx >= 0);
	assert(row_inx < NumRows());

	// Update data structures.
	TScreenItemsGridRow *row = m_row_ptrs[row_inx];
	m_row_ptrs.ReleaseItem(row_inx);
	m_rows.RemoveItem(row);
	if (row->m_xtrn_allocated_row == FALSE)
		delete row;
	else row->~TScreenItemsGridRow();

	// Update the range of invalid rows.
	if (m_min_invalid_row != -1)
	{
		// Update existing range.
		if (row_inx < m_min_invalid_row)
			m_min_invalid_row--;
		if (row_inx < m_max_invalid_row_plus1)
			m_max_invalid_row_plus1--;
	}
	else if (row_inx < NumRows())
	{
		// Create new empty range.
		m_min_invalid_row = m_max_invalid_row_plus1 = row_inx;
	}
}

int TScreenItemsGridItem::GetColumnFromPosX(long pos_x, bool *position_in_delim)
{
	assert(m_min_invalid_clmn == -1);

	long ref_x = m_bkgr_scr_item.GetBaseLeft();
	TScreenItemsGridFullClmnInfo *clmn_info = m_clmns_info.DataPtr();
	for (int clmn=0; clmn<NumColumns(); ++clmn, ++clmn_info)
	{
		long left = ref_x + clmn_info->m_clmn_offs;
		long right = left + clmn_info->ColumnWidth();

		if (right <= pos_x)
			continue;
		if (left > pos_x)
			break;

		// Column is found.
		if (position_in_delim != NULL)
			*position_in_delim = (pos_x >= left+clmn_info->m_props.m_width);
		return(clmn);
	}

	// Passed point does not belong to any column.
	if (position_in_delim != NULL)
		*position_in_delim = FALSE;
	return(-1);
}

int TScreenItemsGridItem::GetRowFromPosY(long pos_y, bool *position_in_delim)
{
	assert(m_min_invalid_row == -1);

	long ref_y = m_bkgr_scr_item.GetBaseTop();
	TListIter<TScreenItemsGridRow> iter(m_rows);
	for (int irow=0; iter; ++iter, ++irow)
	{
		TScreenItemsGridRow &row = iter.CurrItem();
		long top = ref_y + row.m_row_offs;
		long bottom = top + row.RowHeight();

		if (bottom <= pos_y)
			continue;
		if (top > pos_y)
			break;

		// Row is found.
		if (position_in_delim != NULL)
			*position_in_delim = (pos_y >= top+row.m_props.m_height);
		return(irow);
	}

	if (position_in_delim != NULL)
		*position_in_delim = FALSE;

	// Passed point does not belong to any row.
	return(-1);
}

void TScreenItemsGridItem::UpdateColumnInfo(int clmn_inx, TScreenItemsGridColumnInfo &clmn_info)
{
	// Save the props and mark this column as invalid.
	m_clmns_info[clmn_inx].m_props = clmn_info;
	m_clmns_info[clmn_inx].m_clmn_ready = FALSE;
	DeclareInvalidColumn(clmn_inx, FALSE);
}

void TScreenItemsGridItem::UpdateRowInfo(int row_inx, TScreenItemsGridRowInfo &row_info)
{
	// Save the props and mark this row as invalid.
	TScreenItemsGridRow *row = m_row_ptrs[row_inx];
	row->m_props = row_info;
	row->m_row_ready = FALSE;
	DeclareInvalidRow(row_inx, FALSE);
}

int  TScreenItemsGridItem::GetColumnOffsetExact(int clmn_inx)
{
	if (clmn_inx < 0)
		clmn_inx = m_clmns_info.NumItems();

	int offs_exact = 0;
	for (int iclmn=0; iclmn<clmn_inx; ++iclmn)
		offs_exact += m_clmns_info[iclmn].ColumnWidth();

	return(offs_exact);
}

void TScreenItemsGridItem::GetCellPosition(int row_inx, int clmn_inx, long &cell_pos_x, long &cell_pos_y)
{
	TScreenItemsGridRow &row = *m_row_ptrs[row_inx];
	TScreenItemsGridCellData &data = row.m_cells[clmn_inx];
	cell_pos_x = m_bkgr_scr_item.GetBaseLeft()+m_clmns_info[clmn_inx].m_clmn_offs;
	cell_pos_y = m_bkgr_scr_item.GetBaseTop()+row.m_row_offs;
}

void TScreenItemsGridItem::SetCellData(int row_inx, int clmn_inx, TScreenItemsGridCellData &data, bool update_cell_now)
{
	// Accept the passed data.
	TScreenItemsGridRow *row = m_row_ptrs[row_inx];
	row->m_cells[clmn_inx] = data;

	// Check for immediate update.
	if (update_cell_now == TRUE && m_min_invalid_row == -1 && m_min_invalid_clmn == -1)
	{
		// Immediate update is reasonable.
		RefreshScreenItemsPositions(*row, clmn_inx);
		row->RebuildScreenItemsRect();
		RecalcBoundingRect();
	}
	else
	{
		// Shedule postponed update.
		row->m_row_ready = FALSE;
		DeclareInvalidRow(row_inx, FALSE);
	}
}

void TScreenItemsGridItem::SetCellDataNoInvalidate(int row_inx, int clmn_inx, TScreenItemsGridCellData &data)
{
	// Accept the passed data.
	TScreenItemsGridRow *row = m_row_ptrs[row_inx];
	row->m_cells[clmn_inx] = data;
}

void TScreenItemsGridItem::SetPositionedScreenItemAtLayerNoInvalidate(int row_inx, int clmn_inx, int ilayer, TScreenItem *scr_item, bool bypass_ownership)
{
	// Pick up the current positions of the passed screen item.
	long curr_pos_x = scr_item->GetBoundingRect()->left;
	long curr_pos_y = scr_item->GetBoundingRect()->top;

	// Pick up the expected positions of the passed screen item.
	TScreenItemsGridRow &row = *m_row_ptrs[row_inx];
	TScreenItemsGridCellData &data = row.m_cells[clmn_inx];
	long cell_x = m_bkgr_scr_item.GetBaseLeft()+m_clmns_info[clmn_inx].m_clmn_offs;
	long cell_y = m_bkgr_scr_item.GetBaseTop()+row.m_row_offs;
	long xpct_pos_x = CalcPosX(clmn_inx, cell_x, data, scr_item);
	long xpct_pos_y = CalcPosY(row, cell_y, data, scr_item);

	// Set the passed screen item into the cell.
	data.PlaceAtLayer(ilayer, scr_item, bypass_ownership, (short)(curr_pos_x-xpct_pos_x), (short)(curr_pos_y-xpct_pos_y));
}

void TScreenItemsGridItem::UpdateGrid()
{
	// The grid should be not empty.
	assert(NumRows() > 0);
	assert(NumColumns() > 0);

	// Assume that update is not needed.
	bool recalc_bounding_rect = FALSE;

	// Fix columns info first.
	if (m_min_invalid_clmn != -1)
	{
		// Invalid columns are present. Determine the offset to the first invalid column.
		long offs_x = 0;
		if (m_min_invalid_clmn > 0)
		{
			TScreenItemsGridFullClmnInfo &prev_clmn = m_clmns_info[m_min_invalid_clmn-1];
			offs_x = prev_clmn.m_clmn_offs+prev_clmn.ColumnWidth();
		}

		// Iterate all columns starting from the first invalid.
		TScreenItemsGridFullClmnInfo *clmn_info = m_clmns_info.DataPtr()+m_min_invalid_clmn;
		for (int iclmn=m_min_invalid_clmn; iclmn<NumColumns(); ++iclmn, ++clmn_info)
		{
			// Check state of the column.
			bool clmn_ready = clmn_info->m_clmn_ready;
			long old_clmn_offs = clmn_info->m_clmn_offs;
			if (clmn_ready == FALSE || old_clmn_offs != offs_x)
			{
				// Update the column header.
				clmn_info->m_clmn_offs = offs_x;
				clmn_info->m_clmn_ready = TRUE;

				if (clmn_ready == FALSE)
				{
					// Refresh screen items in this column. Process only those rows, that will not be processed
					// later during the rows update.
					for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
					{
						TScreenItemsGridRow &row = iter.CurrItem();
						if (row.m_row_ready == TRUE)
							RefreshScreenItemsPositions(row, iclmn);
					}
				}
				else
				{
					// Just shift the screen items in this column.
					long delta_x = offs_x-old_clmn_offs;
					for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
					{
						TScreenItemsGridRow &row = iter.CurrItem();
						if (row.m_row_ready == TRUE)
							row.m_cells[iclmn].ShiftScreenItems(delta_x, 0);
					}
				}
			}
			else if (iclmn >= m_max_invalid_clmn_plus1)
			{
				// Changes in this and later columns are not possible.
				break;
			}

			// Move curr offset to the next column.
			offs_x += clmn_info->ColumnWidth();
		}

		// Update bounding rects of the row screen items.
		for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
		{
			TScreenItemsGridRow &row = iter.CurrItem();
			if (row.m_row_ready == TRUE)
				row.RebuildScreenItemsRect();
		}

		// Overall bounding rect should be updated also.
		recalc_bounding_rect = TRUE;

		// Check if bkgr rect should be updated.
		TScreenItemsGridFullClmnInfo &last_clmn = m_clmns_info[NumColumns()-1];
		long new_base_width = last_clmn.m_clmn_offs+last_clmn.ColumnWidth();

		RECT base_rect;
		m_bkgr_scr_item.GetBaseRect(base_rect);
		if (base_rect.right-base_rect.left != new_base_width)
		{
			base_rect.right = base_rect.left+new_base_width;
			m_bkgr_scr_item.SetBaseRect(base_rect);
		}

		// Horizontal update is done.
		m_min_invalid_clmn = m_max_invalid_clmn_plus1 = -1;
	}

	// Fix rows.
	if (m_min_invalid_row != -1)
	{
		// Invalid rows are present. Determine the offset to the first invalid row.
		long offs_y = 0;
		if (m_min_invalid_row > 0)
		{
			TScreenItemsGridRow *prev_row = m_row_ptrs[m_min_invalid_row-1];
			offs_y = prev_row->m_row_offs+prev_row->RowHeight();
		}

		// Iterate all rows starting from the first invalid.
		int irow = m_min_invalid_row;
		TScreenItemsGridRow *first_row = m_row_ptrs[m_min_invalid_row];
		for (TListIter<TScreenItemsGridRow> iter(m_rows, first_row); iter; ++iter, ++irow)
		{
			// Check the state of the row.
			TScreenItemsGridRow &row = iter.CurrItem();
			bool row_ready = row.m_row_ready;
			long old_row_offs = row.m_row_offs;
			if (row_ready == FALSE || old_row_offs != offs_y)
			{
				// Update the row header.
				row.m_row_offs = offs_y;
				row.m_row_ready = TRUE;

				if (row_ready == FALSE)
				{
					// Refresh all screen items in this row.
					for (int clmn_inx=0; clmn_inx<NumColumns(); ++clmn_inx)
					{
						RefreshScreenItemsPositions(row, clmn_inx);
					}

					row.RebuildScreenItemsRect();
				}
				else if (row.m_scr_items_present == TRUE)
				{
					// Just shift the screen items in this column.
					long delta_y = offs_y-old_row_offs;
					for (int clmn_inx=0; clmn_inx<NumColumns(); ++clmn_inx)
					{
						row.m_cells[clmn_inx].ShiftScreenItems(0, delta_y);
					}

					ShiftRect(row.m_scr_items_rect, 0, delta_y);
				}
			}
			else if (irow >= m_max_invalid_row_plus1)
			{
				// Changes in this and subsequent rows are not possible.
				break;
			}

			// Move curr offset to the next row.
			offs_y += row.RowHeight();
		}

		// Overall bounding rect should be updated.
		recalc_bounding_rect = TRUE;

		// Check if bkgr rect should be updated.
		TScreenItemsGridRow *last_row = m_row_ptrs[NumRows()-1];
		long new_base_height = last_row->m_row_offs+last_row->RowHeight();

		RECT base_rect;
		m_bkgr_scr_item.GetBaseRect(base_rect);
		if (base_rect.bottom-base_rect.top != new_base_height)
		{
			base_rect.bottom = base_rect.top+new_base_height;
			m_bkgr_scr_item.SetBaseRect(base_rect);
		}

		// Vertical update is done.
		m_min_invalid_row = m_max_invalid_row_plus1 = -1;
	}

	// Fix the overall bounding rect.
	if (recalc_bounding_rect == TRUE)
		RecalcBoundingRect();
}

void TScreenItemsGridItem::RebuildRects()
{
	for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
		iter.CurrItem().RebuildScreenItemsRect();
	RecalcBoundingRect();
}

void TScreenItemsGridItem::RefreshScreenItemsPositions(TScreenItemsGridRow &row, int clmn_inx)
{
	TScreenItemsGridCellData &data = row.m_cells[clmn_inx];
	long cell_x = m_bkgr_scr_item.GetBaseLeft()+m_clmns_info[clmn_inx].m_clmn_offs;
	long cell_y = m_bkgr_scr_item.GetBaseTop()+row.m_row_offs;

	// Check all screen item layers in this cell.
	for (int il=0; il<SCREEN_ITEMS_GRID_NUM_LAYERS; ++il)
	{
		TScreenItem *scr_item = data.m_scr_item[il];
		if (scr_item != NULL)
		{
			// Figure out the new position.
			long pos_x = CalcPosX(clmn_inx, cell_x, data, scr_item);
			long pos_y = CalcPosY(row, cell_y, data, scr_item);

			// Pass new position to screen item with appropriate shift.
			scr_item->SetBoundingRectPos(pos_x+data.m_scr_item_shift_x[il], pos_y+data.m_scr_item_shift_y[il]);
		}
	}
}

long TScreenItemsGridItem::CalcPosX(int clmn_inx, long cell_x, TScreenItemsGridCellData &cell_data, TScreenItem *scr_item)
{
	// Pick up size of the object.
	long scr_item_width = scr_item->GetWidth();

	// Figure out the horizontal position of the object.
	long pos_x = 0, active_horz;
	switch (cell_data.m_horz_align)
	{
		case align_left:
				pos_x = cell_x+cell_data.m_off_left;
				break;

		case align_center:
				active_horz = ColumnWidthWithoutDelim(clmn_inx)-cell_data.m_off_left-cell_data.m_off_right;
				pos_x = cell_x+cell_data.m_off_left+active_horz/2-scr_item_width/2;
				break;

		case align_right:
				pos_x = cell_x+ColumnWidthWithoutDelim(clmn_inx)-cell_data.m_off_right-scr_item_width;
				break;
	}

	return(pos_x);
}

long TScreenItemsGridItem::CalcPosY(TScreenItemsGridRow &row, long cell_y, TScreenItemsGridCellData &cell_data, TScreenItem *scr_item)
{
	// Pick up size of the object.
	long scr_item_height = scr_item->GetHeight();

	// Figure out the vertical position of the object.
	long pos_y = 0, active_vert;
	switch (cell_data.m_vert_align)
	{
		case align_left:
				pos_y = cell_y+cell_data.m_off_top;
				break;

		case align_center:
				active_vert = row.RowHeightWithoutDelim()-cell_data.m_off_top-cell_data.m_off_bottom;
				pos_y = cell_y+cell_data.m_off_top+active_vert/2-scr_item_height/2;
				break;

		case align_right:
				pos_y = cell_y+row.RowHeightWithoutDelim()-cell_data.m_off_bottom-scr_item_height;
				break;
	}

	return(pos_y);
}

void TScreenItemsGridItem::RecalcBoundingRect()
{
	// Calc sum of background rect of the whole object and bounding rects of screen items in each row.
	RECT new_bounding_rect = *(m_bkgr_scr_item.GetBoundingRect());
	for (TListIter<TScreenItemsGridRow> iter(m_rows); iter; ++iter)
	{
		TScreenItemsGridRow &row = iter.CurrItem();
		if (row.m_scr_items_present == TRUE)
			CombineRects(new_bounding_rect, row.m_scr_items_rect);
	}

	// Finally assign the resulting rect.
	m_bounding_rect = new_bounding_rect;
}

void TScreenItemsGridItem::DeclareInvalidRow(int row_inx, bool new_row)
{
	if (m_min_invalid_row < 0)
	{
		// Start new invalid area. It is not important if this row is already existing or new one.
		m_min_invalid_row = row_inx;
		m_max_invalid_row_plus1 = row_inx+1;
	}
	else
	{
		// Update existing invalid area.
		if (new_row == TRUE)
		{
			if (row_inx <= m_min_invalid_row)
				m_min_invalid_row++;
			if (row_inx < m_max_invalid_row_plus1)
				m_max_invalid_row_plus1++;
		}

		if (row_inx < m_min_invalid_row)
			m_min_invalid_row = row_inx;
		if (row_inx >= m_max_invalid_row_plus1)
			m_max_invalid_row_plus1 = row_inx+1;
	}
}

void TScreenItemsGridItem::DeclareInvalidColumn(int clmn_inx, bool new_clmn)
{
	if (m_min_invalid_clmn < 0)
	{
		// Start new invalid area. It is not important if this column is already existing or new one.
		m_min_invalid_clmn = clmn_inx;
		m_max_invalid_clmn_plus1 = clmn_inx+1;
	}
	else
	{
		// Update existing invalid area.
		if (new_clmn == TRUE)
		{
			if (clmn_inx <= m_min_invalid_clmn)
				m_min_invalid_clmn++;
			if (clmn_inx < m_max_invalid_clmn_plus1)
				m_max_invalid_clmn_plus1++;
		}

		if (clmn_inx < m_min_invalid_clmn)
			m_min_invalid_clmn = clmn_inx;
		if (clmn_inx >= m_max_invalid_clmn_plus1)
			m_max_invalid_clmn_plus1 = clmn_inx+1;
	}
}


