//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_ScreenItemsGrid_H
#define	ScreenItems_ScreenItemsGrid_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   WinUi_DialogControlHelpers_H
#include  "WinUi/DialogControlHelpers.H"
#endif
#ifndef   ScreenItems_ScreenItems_H
#include  "ScreenItems/ScreenItems.H"
#endif

#define	SCREEN_ITEMS_GRID_NUM_LAYERS	3

struct TScreenItemsGridCellData
{
	char					m_off_left,		m_off_right;
	char					m_off_top,		m_off_bottom;
	TObjectAlignment	m_horz_align,	m_vert_align;

	BYTE				m_scr_items_owned_mask;

#ifdef SCREEN_ITEMS_GRID_WANT_CELLS_BKGR
	TBasicStyle			*m_cell_bkgr_style;
#endif

	TScreenItem			*m_scr_item[SCREEN_ITEMS_GRID_NUM_LAYERS];

	short				m_scr_item_shift_x[SCREEN_ITEMS_GRID_NUM_LAYERS];
	short				m_scr_item_shift_y[SCREEN_ITEMS_GRID_NUM_LAYERS];
							// Note: Layer [0] is the topmost layer.

	//
	//  NB: Fields of the cell data structure are always in the correct state. Only positions of the screen items
	//  can be not updated. It is not possible to understand from the cell if screen items are updated or not.
	//

	inline void  Clear() { memset(this, 0, sizeof(TScreenItemsGridCellData)); }
				// This method will also align all screen items in the cell to the left-top corner.

	void  ReleaseScreenItems() { for (int il=0; il<SCREEN_ITEMS_GRID_NUM_LAYERS; ++il) { if (m_scr_item[il] != NULL) { if (IsItemOwned(il)) delete m_scr_item[il]; m_scr_item[il] = NULL; } } }

	void  PlaceInTheCenter(TScreenItem *scr_item, bool bypass_ownership)
				{ m_off_left = m_off_right = m_off_top = m_off_bottom = 0; m_horz_align = m_vert_align = align_center; m_scr_item[0] = scr_item; SetMainLayerItemOwned(bypass_ownership); }
	void  PlaceCenterBottom(TScreenItem *scr_item, bool bypass_ownership, char off_bottom = 0)
				{ m_off_left = m_off_right = m_off_top = 0; m_off_bottom = off_bottom; m_horz_align = align_center; m_vert_align = align_right; m_scr_item[0] = scr_item; SetMainLayerItemOwned(bypass_ownership); }

	void  PlaceLeftTop(TScreenItem *scr_item, bool bypass_ownership, char off_left = 0, char off_top = 0)
				{ m_off_left = off_left; m_off_right = 0; m_off_top = off_top; m_off_bottom = 0; m_horz_align = m_vert_align = align_left; m_scr_item[0] = scr_item; SetMainLayerItemOwned(bypass_ownership); }
	void  PlaceLeftCenter(TScreenItem *scr_item, bool bypass_ownership, char off_left = 0)
				{ m_off_left = off_left; m_off_right = m_off_top = m_off_bottom = 0; m_horz_align = align_left; m_vert_align = align_center; m_scr_item[0] = scr_item; SetMainLayerItemOwned(bypass_ownership); }
	void  PlaceRightTop(TScreenItem *scr_item, bool bypass_ownership, char off_right = 0, char off_top = 0)
				{ m_off_left = 0; m_off_right = off_right; m_off_top = off_top; m_off_bottom = 0; m_horz_align = align_right; m_vert_align = align_left; m_scr_item[0] = scr_item; SetMainLayerItemOwned(bypass_ownership); }
	void  PlaceRightCenter(TScreenItem *scr_item, bool bypass_ownership, char off_right = 0)
				{ m_off_left = 0; m_off_right = off_right; m_off_top = m_off_bottom = 0; m_horz_align = align_right; m_vert_align = align_center; m_scr_item[0] = scr_item; SetMainLayerItemOwned(bypass_ownership); }

	void  PlaceAtLayer(int ilayer, TScreenItem *scr_item, bool bypass_ownership, short shift_x = 0, short shift_y = 0)
	{
		assert(ilayer >= 0 && ilayer < SCREEN_ITEMS_GRID_NUM_LAYERS);
		m_scr_item[ilayer] = scr_item; SetItemOwned(ilayer, bypass_ownership);
		m_scr_item_shift_x[ilayer] = shift_x; m_scr_item_shift_y[ilayer] = shift_y;
	}

protected:

	bool IsItemOwned(int ilayer)	 const { return((m_scr_items_owned_mask >> ilayer) & 1); }
	void SetMainLayerItemOwned(bool value) { if (value == TRUE) m_scr_items_owned_mask |= 1; else m_scr_items_owned_mask &= 0xFE; }
	void SetItemOwned(int ilayer, bool value) { if (value == TRUE) m_scr_items_owned_mask |= (1 << ilayer); else m_scr_items_owned_mask &= ~(1 << ilayer); }

	void ShiftScreenItems(long shift_x, long shift_y)
	{
		TScreenItem **pxi = m_scr_item;
		for (int il=0; il<SCREEN_ITEMS_GRID_NUM_LAYERS; ++il, ++pxi)
		{
			if (*pxi != NULL)
				(*pxi)->ShiftItem(shift_x, shift_y);
		}
	}

	friend class TScreenItemsGridItem;
};

typedef TStructsArray<TScreenItemsGridCellData, 8, 16> TScreenItemsGridCellsArray;

struct TScreenItemsGridRowInfo
{
	long					m_height;				// Height of the row without delim.
	long					m_delim_height;			// Delim height can be 0.

	TBasicStyle			*m_bkgr_style;
	TBasicStyle			*m_bkgr_delim_style;

	DWORD				m_row_app_data;
	UINT64				m_row_app_data_ex;

	inline void Clear() { memset(this, 0, sizeof(TScreenItemsGridRowInfo)); }

	void  Setup(long height, TBasicStyle *bkgr_style = NULL) { m_height = height; m_delim_height = 0; m_bkgr_style = bkgr_style; m_bkgr_delim_style = NULL; m_row_app_data = 0; m_row_app_data_ex = 0; }
	void  SetupDelim(long height, TBasicStyle *bkgr_style) { m_delim_height = height; m_bkgr_delim_style = bkgr_style; }
};

class TScreenItemsGridRow : public TListItem
{
public:

	TScreenItemsGridRow() { m_props.Clear(); m_xtrn_allocated_row = m_row_ready = FALSE; }
	TScreenItemsGridRow(TScreenItemsGridRowInfo &props) { m_props = props; m_xtrn_allocated_row = m_row_ready = FALSE; }
	~TScreenItemsGridRow() { int nCells = m_cells.NumItems(); for (int i=0; i<nCells; ++i) m_cells[i].ReleaseScreenItems(); }

	inline long RowHeight() const { return(m_props.m_height+m_props.m_delim_height); }
	inline long RowHeightWithoutDelim() const { return(m_props.m_height); }

protected:

	void  RebuildScreenItemsRect()
	{
		m_scr_items_present = FALSE;
		int nCells = m_cells.NumItems();
		for (int i=0; i<nCells; ++i)
		{
			TScreenItem **pxi = m_cells[i].m_scr_item;
			for (int il=0; il<SCREEN_ITEMS_GRID_NUM_LAYERS; ++il, ++pxi)
			{
				if (*pxi != NULL)
				{
					// ScreenItem is present. Process its bounding rect.
					if (m_scr_items_present == FALSE)
					{
						m_scr_items_rect = *((*pxi)->GetBoundingRect());
						m_scr_items_present = TRUE;
					}
					else
					{
						TScreenItem::CombineRects(m_scr_items_rect, *((*pxi)->GetBoundingRect()));
					}
				}
			}
		}
	}

	TScreenItemsGridRowInfo		m_props;					// Properties of the row are always "correct".

	TScreenItemsGridCellsArray		m_cells;

	bool							m_xtrn_allocated_row;

	bool							m_row_ready;				// This flag shows if m_row_offs, m_scr_items_present, m_scr_items_rect,
															// and screen item positions are in correct state or not.
	bool							m_scr_items_present;		// This flag is TRUE if at least one cell contains screen item in at least one
															// of the layers.
	long							m_row_offs;					// Vertical offset of the row from the top of the base rect of the m_bkgr_scr_item.
	RECT						m_scr_items_rect;			// Bounding rect of all screen items in the row.

	friend class TScreenItemsGridItem;
};

typedef TStructsArray<TScreenItemsGridRow*, 32, 1024> TScreenItemsGridRowPtrsArray;

struct TScreenItemsGridColumnInfo
{
	long							m_width;					// Width of the column without delim.
	long							m_delim_width;				// Delimiter width can be 0.

	TBasicStyle					*m_bkgr_style;
	TBasicStyle					*m_bkgr_delim_style;

	inline void Clear() { memset(this, 0, sizeof(TScreenItemsGridColumnInfo)); }

	void  Setup(long width, TBasicStyle *bkgr_style = NULL) { m_width = width; m_delim_width = 0; m_bkgr_style = bkgr_style; m_bkgr_delim_style = NULL; }
	void  SetupDelim(long width, TBasicStyle *bkgr_style) { m_delim_width = width; m_bkgr_delim_style = bkgr_style; }
};

struct TScreenItemsGridFullClmnInfo
{
	TScreenItemsGridColumnInfo	m_props;					// Props of the column are always "correct".

	bool							m_clmn_ready;				// This flag shows if m_clmn_offs and positions of all screen items in the column
															// are in the correct state or not.
	long							m_clmn_offs;

	inline long ColumnWidth() const { return(m_props.m_width+m_props.m_delim_width); }
	inline long ColumnWidthWithoutDelim() const { return(m_props.m_width); }
};

typedef TStructsArray<TScreenItemsGridFullClmnInfo, 32, 256> TScreenItemsGridFullClmnInfosArray;

#define SCRITEMS_GRID_APPEND -1
#define SCRITEMS_GRID_NUM_COLUMNS -1

// Array of 256kb buffers with initial capacity of 128 pointers.
typedef TDataBuffersArray<128, 128> TScreenItemsGridBuffsArray;

class TScreenItemsGridItem : public TScreenItem
{
public:

	TScreenItemsGridItem(TScreenItemStyle *bkgr_style = NULL);
	~TScreenItemsGridItem() { ReleaseRows(); }

	//
	// Methods, inherited from the TScreenItem class.
	//

	enum { type_ID = 0x054 };

	short	GetItemTypeId() const { return(type_ID); }

	void		SetStyle(TScreenItemStyle *bkgr_style) { m_bkgr_scr_item.SetStyle(bkgr_style); }
				// Passed style becomes the background color of the grid. Note that passed style is not becoming the value
				// of the m_style field of the grid object itself.

	void		SetBoundingRectPos(long pos_x, long pos_y);
	void		SetHotSpotPos(long pos_x, long pos_y);
	void		GetHotSpotPos(long &px, long &py) { px = m_bkgr_scr_item.GetBaseLeft(); py = m_bkgr_scr_item.GetBaseTop(); }
	void		ShiftItem(long shift_x, long shift_y);

	void			OnDraw(HDC hDC, RECT &invalid_appsp_rect);
	TScreenItem	*CheckClick(POINT &app_click_point);

	//
	//  New class specific methods.
	//

	int		GetNumInitialColumnSlots() const { return(m_clmns_info.NumInitialItems()); }

	void		SetClickableBkgrProp(bool new_value) { m_clickable_bkgr = new_value; }

	void		Reset();
	bool		AddColumn(int clmn_inx_before, TScreenItemsGridColumnInfo &props);
	void		DeleteColumn(int clmn_inx);
	bool		AddRow(int row_inx_before, TScreenItemsGridRowInfo &props, TScreenItemsGridBuffsArray *external_rows_info_storage = NULL);
	void		DeleteRow(int row_inx);

	int		NumRows() const { return(m_row_ptrs.NumItems()); }
	int		NumColumns() const { return(m_clmns_info.NumItems()); }

	long		ColumnWidth(int clmn_inx) const { return(m_clmns_info[clmn_inx].ColumnWidth()); }
	long		ColumnWidthWithoutDelim(int clmn_inx) const { return(m_clmns_info[clmn_inx].ColumnWidthWithoutDelim()); }
	long		RowHeight(int row_inx) const { return(m_row_ptrs[row_inx]->RowHeight()); }
	long		RowHeightWithoutDelim(int row_inx) const { return(m_row_ptrs[row_inx]->RowHeightWithoutDelim()); }

	int		GetColumnFromPosX(long pos_x, bool *position_in_delim = NULL);
	int		GetRowFromPosY(long pos_y, bool *position_in_delim = NULL);

	void		GetColumnInfo(int clmn_inx, TScreenItemsGridColumnInfo &clmn_info) { clmn_info = m_clmns_info[clmn_inx].m_props; }
	void		UpdateColumnInfo(int clmn_inx, TScreenItemsGridColumnInfo &clmn_info);
	void		GetRowInfo(int row_inx, TScreenItemsGridRowInfo &row_info) { row_info = m_row_ptrs[row_inx]->m_props; }
	void		UpdateRowInfo(int row_inx, TScreenItemsGridRowInfo &row_info);

	int		GetColumnOffset(int clmn_inx) const { return(m_clmns_info[clmn_inx].m_clmn_offs); }
	int		GetRowOffset(int row_inx) const { return(m_row_ptrs[row_inx]->m_row_offs); }

	int		GetColumnOffsetExact(int clmn_inx);
				// Method sums up the current widths of the columns.
				// It is ok to pass SCRITEMS_GRID_NUM_COLUMNS as parameter to get width of the whole grid.

	void		GetCellPosition(int row_inx, int clmn_inx, long &cell_pos_x, long &cell_pos_y);

	void		GetCellData(int row_inx, int clmn_inx, TScreenItemsGridCellData &data) { data = m_row_ptrs[row_inx]->m_cells[clmn_inx]; }
	void		SetCellData(int row_inx, int clmn_inx, TScreenItemsGridCellData &data, bool update_cell_now = FALSE);

	TObjectAlignment	GetCellHorzAlignment(int row_inx, int clmn_inx) { return(m_row_ptrs[row_inx]->m_cells[clmn_inx].m_horz_align); }
	TObjectAlignment	GetCellVertAlignment(int row_inx, int clmn_inx) { return(m_row_ptrs[row_inx]->m_cells[clmn_inx].m_vert_align); }

	TScreenItem			*GetScreenItem(int row_inx, int clmn_inx, int ilayer) { return(m_row_ptrs[row_inx]->m_cells[clmn_inx].m_scr_item[ilayer]); }
	TScreenItem			*GetMainLayerScreenItem(int row_inx, int clmn_inx) { return(m_row_ptrs[row_inx]->m_cells[clmn_inx].m_scr_item[0]); }

	void		SetCellDataNoInvalidate(int row_inx, int clmn_inx, TScreenItemsGridCellData &data);
				// This method sets/updates cell when repositioning of the screen items is not needed. Row rect is not rebuilt also.
				// If it might change, application is responsible for calling RebuildRects(). App should not change m_scr_items_present
				// in the row by calling this method if it is not calling RebuildRects().

	void		SetPositionedScreenItemAtLayerNoInvalidate(int row_inx, int clmn_inx, int ilayer, TScreenItem *scr_item, bool bypass_ownership);

	DWORD	GetRowAppData(int row_inx) { return(m_row_ptrs[row_inx]->m_props.m_row_app_data); }
	void		SetRowAppData(int row_inx, DWORD data) { m_row_ptrs[row_inx]->m_props.m_row_app_data = data; }
	UINT64	GetRowAppDataEx(int row_inx) { return(m_row_ptrs[row_inx]->m_props.m_row_app_data_ex); }
	void		SetRowAppDataEx(int row_inx, UINT64 data) { m_row_ptrs[row_inx]->m_props.m_row_app_data_ex = data; }

	void		RefreshScreenItemsPositions(int row_inx, int clmn_inx) { RefreshScreenItemsPositions(*m_row_ptrs[row_inx], clmn_inx); }

	void		UpdateGrid();
	void		RebuildRects();

protected:

	void ReleaseRows()
	{
		while (m_rows.IsEmpty() == FALSE)
		{
			TScreenItemsGridRow *row = (TScreenItemsGridRow*)m_rows.GetFirst();
			m_rows.RemoveItem(row);

			if (row->m_xtrn_allocated_row == FALSE)
				delete row;
			else row->~TScreenItemsGridRow();
		}
	}

	void		RefreshScreenItemsPositions(TScreenItemsGridRow &row, int clmn_inx);
	long		CalcPosX(int clmn_inx, long cell_x, TScreenItemsGridCellData &cell_data, TScreenItem *scr_item);
	long		CalcPosY(TScreenItemsGridRow &row, long cell_y, TScreenItemsGridCellData &cell_data, TScreenItem *scr_item);
	void		RecalcBoundingRect();

	void		DeclareInvalidRow(int row_inx, bool new_row);
	void		DeclareInvalidColumn(int clmn_inx, bool new_clmn);

	inline static void ShiftRect(RECT &rc, long shift_x, long shift_y) { rc.left += shift_x; rc.right += shift_x; rc.top += shift_y; rc.bottom += shift_y; }

protected:

	TScreenItemsGridFullClmnInfosArray		m_clmns_info;
	TScreenItemsGridRowPtrsArray			m_row_ptrs;
	TList									m_rows;

	int									m_min_invalid_row,  m_max_invalid_row_plus1;
	int									m_min_invalid_clmn, m_max_invalid_clmn_plus1;
											// When all rows/columns are valid, m_min_invalid_row and/or m_min_invalid_clmn
											// are equal to -1. Note that range of invalid rows/columns can be empty.

	TRectItem							m_bkgr_scr_item;
											// This data field is the storage of the hot spot position.

	bool									m_clickable_bkgr;

private:

	TBasicStyle							m_default_bkgr_style;
											// Default bkgr is transparent and it is not visible from outside. It is needed to make
											// the object paintable if there is no derived class that wants to use the m_style field.
};

#endif	// ScreenItems_ScreenItemsGrid_H


