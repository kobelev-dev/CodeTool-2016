//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "WinUi/ListBoxHelper.H"

//----------------------------------------------------------------------------
//  ===================  TListBoxHorzHelper  ========================
//----------------------------------------------------------------------------

void TListBoxHorzHelper::SetColumnProps(TListBoxColumnProps *records)
{
	assert(records != NULL);

	m_num_columns = 0;
	for (int inx=0; inx <LBX_MAX_COLUMNS; ++inx)
	{
		if (records->clmn_min_width <= 0)
			break;

		// Make some basic checks of the record data.
		assert(records->clmn_alignment >= align_left && records->clmn_alignment <= align_right);
		assert(records->clmn_front_delim >= 0);
		assert(records->clmn_ext_width >= 0);
		assert(records->clmn_back_delim >= 0);

		// Accept the record.
		m_props[inx] = *records;
		m_num_columns = inx+1;
		records++;
	}

	UpdateColumnWidths();
}

void TListBoxHorzHelper::UpdateColumnWidths(int new_area_width)
{
	if (new_area_width >= 0)
		m_visible_area_width = new_area_width;

	if (m_num_columns <= 0)
		return;

	// Find out the minimum required width and the sum of all requested extras.
	int min_required = 0;
	int ext_required = 0;
	for (int inx1=0; inx1<m_num_columns; ++inx1)
	{
		TListBoxColumnProps &pr = m_props[inx1];
		min_required += pr.clmn_front_delim;
		min_required += pr.clmn_min_width;
		ext_required += pr.clmn_ext_width;
		min_required += pr.clmn_back_delim;
	}

	int ext_to_distribute = 0;
	if (min_required < m_visible_area_width)
	{
		// There is some extra space to distribute.
		ext_to_distribute = m_visible_area_width-min_required;
	}

	// Rebuild the columns state.
	int offs_x = 0;
	int ext_remaining = ext_to_distribute;
	int max_offset = m_visible_area_width-m_props[m_num_columns-1].clmn_back_delim;
	for (int inx2=0; inx2<m_num_columns; ++inx2)
	{
		TListBoxColumnProps &pr = m_props[inx2];

		offs_x += pr.clmn_front_delim;
		m_state[inx2].column_align = pr.clmn_alignment;
		m_state[inx2].column_offset = offs_x;
		offs_x += pr.clmn_min_width;

		if (pr.clmn_ext_width > 0 && ext_remaining > 0)
		{
			assert(ext_required > 0);
			int extra = (ext_to_distribute*pr.clmn_ext_width)/ext_required;
			offs_x += extra;
			ext_remaining -= extra;
		}

		if (ext_required <= 0 || inx2 != m_num_columns-1 || offs_x >= max_offset)
		{
			// All columns have fixed width or this is not the final column or the width overflow happened.
			// In all these cases the straightforward assignment should be used.
			m_state[inx2].column_width = offs_x-m_state[inx2].column_offset;
		}
		else
		{
			// Align the right side of the column to the right side of the visible area.
			m_state[inx2].column_width = max_offset - m_state[inx2].column_offset;
		}

		offs_x += pr.clmn_back_delim;
	}
}

int TListBoxHorzHelper::GetColumnFromOffset(TListBoxItemCore *item, int offs_x, bool &offs_inside_the_column)
{
	offs_inside_the_column = FALSE;

	if (m_num_columns <= 0)
		return(-1);

	if (offs_x >= m_visible_area_width)
	{
		// Position stays outside of the columns area. Return like nothing was clicked.
		return(m_num_columns);
	}

	for (int inx=0; inx<m_num_columns; ++inx)
	{
		if (offs_x < m_state[inx].column_offset)
		{
			// Postion stays to the left of the current column.
			return(inx-1);
		}

		// Position belongs either to the current column or to the right space after the current column.
		int ret_inx = inx;

		int ovlp = (item != NULL) ? item->GetCellWidth(inx) : 1;
		if (ovlp > 1)
		{
			// The cell occupies more than one column.
			inx += ovlp-1;
		}

		if (inx < m_num_columns && offs_x < m_state[inx].column_offset+m_state[inx].column_width)
		{
			// The pos is inside of the current column.
			offs_inside_the_column = TRUE;
			return(ret_inx);
		}
	}

	// Position stays after the last column.
	return(m_num_columns-1);
}

//---------------------------------------------------------------------------
//  ===============  TGenericListBoxObjectProps  =====================
//---------------------------------------------------------------------------

void TGenericListBoxObjectProps::PrepareForSingleSelect(bool auto_sel)
{
	memset(this, 0, sizeof(TGenericListBoxObjectProps));
	use_inset_frame = TRUE;
	auto_select = auto_sel;
	upper_scroll_offset = 2;
	lower_scroll_offset = 3;
}

void TGenericListBoxObjectProps::PrepareForMultiSelect(bool auto_sel)
{
	PrepareForSingleSelect(auto_sel);
	multi_select = TRUE;
}


