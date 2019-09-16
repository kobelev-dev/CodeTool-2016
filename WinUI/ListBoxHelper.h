//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Single direction abstract list box.
//

#ifndef	WinUi_ListBoxHelper_H
#define	WinUi_ListBoxHelper_H

#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif
#ifndef   WinUi_DialogControlHelpers_H
#include  "WinUi/DialogControlHelpers.H"
#endif

class TListBoxObjectCore;

class TListBoxItemCore : public TListItem
{
public:

	virtual	int		GetCellWidth(int iclmn) const { return(1); }
	virtual	void		SetCellWidth(TListBoxObjectCore &owner, int iclmn, int width_in_columns) { }
						//
						//  The cell of the list box item may occupy several adjacent columns.
						//
						//		value == 0	-- column is obscured by one of the previous columns.
						//		value > 0		-- value shows the number of occupied columns.
						//
						//  Parameter owner is needed because changing the width of the cell may obscure or reveal
						//  other cells that may change the height of the listbox item. Implementation should report
						//  these changes to the owning list box object appropriately.
						//
};

class TListBoxObjectCore : public TListItem
{
public:

	virtual	int		GetNumColumns() const = 0;
	virtual	int		GetNumItems() const = 0;
						//
						//  Both methods should return the current values, not the max possible number of columns or items.
						//
};

struct TListBoxColumnProps
{
	TObjectAlignment	clmn_alignment;
	short				clmn_front_delim;			// In pixels.
	short				clmn_min_width;				// In pixels. Min width cannot be zero, except for the EOF record.
	short				clmn_ext_width;				// In pseudo percents;
	short				clmn_back_delim;			// In pixels.

};

#define	LBX_CLMNP_EOF	   { align_left, 0, 0, 0, 0, },

struct TListBoxColumnState
{
	TObjectAlignment	column_align;
	int					column_offset;				// In pixels.
	int					column_width;				// In pixels.
};

class TListBoxHorzHelper
{
public:

	TListBoxHorzHelper() { m_num_columns = m_visible_area_width = 0; }

	void		SetColumnProps(TListBoxColumnProps *array_of_records);
				// If passed array is longer than LBX_MAX_COLUMNS, then only the first LBX_MAX_COLUMNS records will be processed.

	void		SetColumnAlignment(int iclmn, TObjectAlignment new_alignment) { assert(iclmn >= 0 && iclmn < m_num_columns); m_props[iclmn].clmn_alignment = new_alignment; UpdateColumnWidths(); }

	void		UpdateColumnWidths(int new_area_width = -1);

	int					NumColumns() const { return(m_num_columns); }

	TListBoxColumnState	&ClmnInfo(int iclmn) { assert(iclmn >= 0 && iclmn < m_num_columns); return(m_state[iclmn]); }

	int		GetColumnFromOffset(TListBoxItemCore *item, int offs_x, bool &offs_inside_the_column);
				//
				//  The first param is needed to figure out what columns are extended and what columns are not.
				//  It is ok to pass NULL. This is the same to passing item that has no overlapped columns.
				//
				//  When the columns are wider than the visible area, click outside of the visible area it is treated
				//  as click out of any column. This means that only the visible parts of the coumns are clickable.
				//
				//  This is what the method will return:
				//
				//	  ///////]		   [Column_1]				...	[Column_N-1]			   [//////////////
				//				^		^			      ^		     ^			 ^		   ^
				//				|		|			      |		     |				 |		   |
				//				|	retValue = 0		      |	   retValue = N-1		 |	   retValue = N
				//				|	insideClmn = TRUE	      |	   insideClmn = TRUE	 |	   insideClmn = FALSE
				//				|					      |						 |
				//			retValue = -1			      retValue = 0				 retValue = N-1
				//			insideClmn = FALSE	      insideClmn = FALSE			 insideClmn = FALSE
				//

	enum { LBX_MAX_COLUMNS = 50 };

protected:

	TListBoxColumnProps		m_props[LBX_MAX_COLUMNS];
	TListBoxColumnState		m_state[LBX_MAX_COLUMNS];

	int						m_num_columns;

	int						m_visible_area_width;
								// When the width of the screen area is smaller than the min required width, object distributes
								// the remaining width between the columns. In the opposite case columns are simply placed
								// outside of the visible area. Upper layer should handle this case.
};

//
//  All list box items are visible. Hidden/visible concept is not applicable to this implementation of the list boxes.
//
template <class ListBoxObjectType>
class TGenericListBoxItem : public TListBoxItemCore
{
public:

	TGenericListBoxItem(BYTE ident = 0) { m_listbox_index = -1; m_item_flags = 0; m_app_ident = ident; m_cached_height  = -1; }

	inline long	ListBoxIndex() const { return(m_listbox_index); }
					// Return value is zero based index or -1 if item does not belong to the listbox.

	inline bool	IsSelected() const { return((m_item_flags & item_selected) != 0); }
					// This method and corresponding flag are used only when object is used in multiple selection listbox.

	inline bool	IsOwned() const { return((m_item_flags & item_not_owned) == 0); }
					// Item is owned is when it is owned by the listbox. Item can be not owned ONLY when list box is in local items list mode.

	inline int		GetItemHeight(ListBoxObjectType *ptr_owner)
							{ if (m_cached_height <= 0) m_cached_height = MeasureItemHeight(ptr_owner); return(m_cached_height); }
					// Latest implementation uses only the variable items height mode.

	virtual  void	PaintItem(HDC hDC, ListBoxObjectType *ptr_owner, RECT &item_rect) = 0;
					// This method should paint the listbox item at the specific position on the screen.
					// Object should not record the rect where it was painted.

	virtual  int	MeasureItemHeight(ListBoxObjectType *ptr_owner) = 0;
					// This method should always return some positive number. Otherwise this will break the code that fills
					// the client area of the listbox with items. App level should simply overwrite this method and return
					// appropriave value. This is enough for the rest of the code to work. App level should not check/modify
					// GetItemHeight() method and/or the m_cached_height field.

	void		SetListBoxIndex(int val) { m_listbox_index = val; }
	void		SetSelection(bool val) { if (val == TRUE) m_item_flags |= item_selected; else m_item_flags &= (~item_selected); }
	void		SetNotOwned() { m_item_flags |= item_not_owned; }

	short	GetCachedHeight() const { return(m_cached_height); }
	void		SetCachedHeight(short val) { m_cached_height  = val; }

protected:

	enum { item_selected = 1, item_not_owned = 2, };

	long				m_listbox_index;			// Index of the item in the lsitbox or -1 if item does not belong to
												// any listbox.
	BYTE			m_item_flags;				// All bits of this field should be used by the lisbox code only.
	BYTE			m_app_ident;				// The list box code is not using this field. App level can place any
												// data here.
	short			m_cached_height;			// Latest height of the item. Application code should never change this field.
												// Changes should happen only ctor, GetItemHeight() and UpdateItemHeight().
};

//
//  Generic ListBox table. When application adds/removes items to the table, it should notify the list box
//  object about these chages with its own code.
//
template <class TableItemType>
class TGenericListBoxTable : public TListItem
{
public:

	virtual int		NumItems() = 0;
					//
					// The number of items in the table. The table can be empty. Note that this method cannot fail.
					//

	virtual int		GetCurrentPosition() = 0;
					//
					// Implementation should return the current position. Position is considered to be in front of the item. Position can also stay
					// after the last element of the table. Note that this method cannot fail. The current position is always known.
					//

	virtual void	Seek(int record_index) = 0;
					//
					// After the call to this function the reading position stays in front of the item with passed zero based record index.
					// Position can also stay after the end of all records. ListBox class does not allow seek operation to fail. Id caller passes
					// bogus record index, implementation should place position at the appropriate side of the table.
					//

	virtual TableItemType *ReadItem(bool read_forward = TRUE) = 0;
					//
					//   1. When the reading is successful, method should return object that is completely ready for painting.
					//   2. In case of failure implementation should return NULL and position should be still incremented/decremented.
					//   3. When caller tries to read outside of the table, return value should be NULL and current position should
					// remain unchanged.
					//   4. Single selection listboxes should always return the selection flag cleared. Listbox code will set this flag itself.
					//   5. Multi select listbox tables are responsible for retaining and setting selection flag in the returned object appropriately.
					//

	virtual void	RecycleItem(TableItemType *item) = 0;
					//
					// Derived classes may organize some sort of lookaside table to reuse the TableItemType instances in future calls
					// to ReadItem(). Multi selection listbox tables should save selection field of the passed object before releasing
					// the TableItemType object.
					//

	virtual bool	IsMultiSelSupported() { return(FALSE); }
	virtual void	ResetSelectionMarks() { }
	virtual void	SelectItemsRange(int record_inx_beg, int num_records, bool new_slct_stt = TRUE) { }
	virtual bool	GetSlctFlag(int record_inx) { return(FALSE); }
	virtual bool	GetNextSelectedRange(int seek_pos, int &range_beg_inx, int &range_len, bool direction_forward = TRUE) { return(FALSE); }
	virtual int		GetSelectionSummary(int &slct_beg_inx, int &slct_end_inx) { slct_beg_inx = slct_end_inx = -1; return(0); }
					//
					// These methods implement the selected state for individual records of the table. They are needed ONLY if the owning list box
					// object is in multiselect mode. For single selection listboxes they are not needed because single selection is implemented in
					// the listbox object itself by storing the index of the fist selected item.
					// Once implemented, these methods should support "reliable" selection, i.e. return values should be always consistent even
					// if ReadItem() method returns NULL.
					//
};

struct TGenericListBoxObjectProps
{
	bool				use_inset_frame;

	bool				multi_select;
	bool				auto_select;						// When value of this field is FALSE, list box is never changing the selected
													// state of its item(s) on click. It simply sends notification to the app level.
	short			upper_scroll_offset;
	short			lower_scroll_offset;

	bool				vert_scroll_always_visib;
	short			vert_scroller_width;				// The value 0 defaults the to current system metrics.

	void		PrepareForSingleSelect(bool auto_sel = TRUE);
	void		PrepareForMultiSelect(bool auto_sel = TRUE);
};

template <class ListBoxItemType, class ListBoxObjectType>
class TGenericListBoxObject : public TListBoxObjectCore
{
public:

	TGenericListBoxObject() { m_items_table = NULL; ResetListBox(); }
	~TGenericListBoxObject() { ResetListBox(); }
		// The ctor and the dector are very similar. This is not a mistake.

	void		ResetListBox();
				// This method releases local list and brings all local vars to the state of the empty list box.

	void		SetupProps(TGenericListBoxObjectProps &new_props, bool rebuild_listbox_now);

	//
	//   Manipuliations with items.
	//

	typedef TGenericListBoxTable<ListBoxItemType> ListBoxTableType;

	void		SetupListBoxTable(ListBoxTableType *table, bool paint_now = TRUE);
				// Method can setup either non NULL or null table. When the new table is set, all existing items are either discarded
				// or recycled into the existing listbox table. ListBox switches to the new table and resets its scrolling pos to zero.

	void		SetupNewItemsList(TList &list_to_bypass, bool paint_now = TRUE);
				// Existing items list is destroyed and passed list is bypassed into listbox as new list of items.
				// Note that ownership on all items that are not marked with xtrn_owned flag is passed to
				// the listbox.

	void		StartBulkChanges() {  m_bulk_changes_flag = TRUE; }
				// After calling this method application should call RebuildListBox() when it finishes changes in the items list
				// and and props of the object.

	void		RebuildListBox(int upper_item_inx_proto = -1, bool paint_now = TRUE);
				// The method is similar to setting the new listbox table or the new items list, only the items are not recycled
				// and the scroll pos is preserved if possible.

	bool		GetListBoxTableMode() { return((m_items_table != NULL) ? TRUE : FALSE); }
				// This method should be used to fugure out the current list box mode.

		//
		//	All data manipulation methods, including the DeleteAllItems() method cannot be used in the list
		//	box table mode.
		//

	void		AddItem(ListBoxItemType *item, ListBoxItemType *ins_before = NULL);
	void		AddItemOnTop(ListBoxItemType *item) { AddItem(item, (ListBoxItemType*)m_items_list.GetFirst()); }
				// These methods add item to the internal items list and display the changes if build changes state is not set.
				// Note that if m_bulk_changes_flag is not set, renumbering of all appropriate items will immediately happen.
				// When ins_before param is NULL, passed item will be added to the end of the list. Selection is the passed
				// item is killed if any.

	void		RemoveItem(ListBoxItemType *item);
				// Method removes item from internal list and from the listbox. Note that method is not destroying the item,
				// caller gains ownership on it.

	void		DeleteAllItems();
				// Method deletes everything from the listbox and destroyes the internal list.

	void		InvalidateItem(ListBoxItemType *item, int num_items_to_invalidate = 1);
	void		InvalidateItemByIndex(int listbox_index, int num_items_to_invalidate = 1);
				// Methods invalidate screen rect of the item if it is visible in the current state scrolling. It is ok to pass indexes,
				// that stay outside of the visible area. Method will agjust indexes and invalidate visible part of requested items.

	void		UpdateItemHeight(ListBoxItemType *item, int new_height = -1);
	void		UpdateItemHeightByIndex(int listbox_index, int new_height = -1);
				// When listbox is in the "same items height" mode, first parameter of the call is ignored.
				// Method invalidates the screen arrea only.

	void		SelectItem(ListBoxItemType *item, bool new_selection_state = TRUE);
	void		SelectItemByIndex(int listbox_index, bool new_selection_state = TRUE);
				// These methods can select/deselect only single item. Only valid item pointer or listbox index
				// should be passed.

	void		SelectItemsRange(int inx_beg, int num_items, bool new_slct_stt = TRUE);
				// This method should be called on multiselection listboxes only.

	void		RemoveSelection();
				// Method removes the selection if any. This works both for single and multiple selection objects.

	int					GetSelectionIndex();
	ListBoxItemType		*GetSelectedItem();
							// These methods are applicable only for single selection listboxes. Second method cannot
							// be used in list box table mode because selected item can be scrolled out of the view.

	bool		GetItemMultiSelectState(int listbox_index);
	int		GetSelectionSummary(int &slct_beg_inx, int &slct_end_inx);
	bool		GetNextSelectedRange(int seek_pos, int &range_beg_inx, int &range_len, bool direction_forward = TRUE);

	int		GetNumItems() const { return(m_num_items); }
	int		GetNumOwnedItems();
	int		GetUpperItemIndex() const { return(m_upper_item_inx); }

	ListBoxItemType		*GetItemByIndex(int listbox_index);
							// This method cannot be used in listbox table mode.

	void		SetTopIndex(int new_upper_item_inx);
	void		ScrollItemIntoTheView(ListBoxItemType *item, bool enforce_margins);
	void		ScrollItemIntoTheView(int listbox_index, bool enforce_margins);
				// The numbers if lines are the numbers of items that are completely visible.

	bool		SortLocalList(void *ctx, int (__cdecl *compare_function)(void *ctx, const ListBoxItemType **item1, const ListBoxItemType **item2))
						{ return(m_items_list.QuickSort(ctx, (int (__cdecl*)(void *ctx, const TListItem**, const TListItem**))compare_function)); }
				// This method only sorts items in the m_items_list. It is not doing anything with the listbox itself.

	TListIter<ListBoxItemType>	GetLocalItemsListIter() { TListIter<ListBoxItemType> iter(m_items_list); return(iter); }

	template <class AppListBoxItemType>
	TListIter<AppListBoxItemType>	GetAppIter() { TListIter<AppListBoxItemType> iter(m_items_list); return(iter); }

	int		GetItemFromPoint(int offs_y, ListBoxItemType **item_ptr, bool *inside_item_rect = NULL, bool *item_fully_visible = NULL);
	bool		IsItemIndexVisible(int inx) { return(inx >= m_upper_item_inx && inx < m_upper_item_inx+m_num_visible_items); }

	enum { TLB_DEFAULT_ITEM_HEIGHT = 6 };
				// This value is used when it is not possible to determine the real height of the item.

protected:

	//
	//  This iterator should be used to iterate visible items in the local items list. It should be used ONLY when ListBox
	//  is in the local items list mode.
	//
	class TListBoxLocalItemsListIter
	{
	public:

		inline	TListBoxLocalItemsListIter(TGenericListBoxObject *inst);
					// After this ctor current pos is in front of the first item in the list if any and the current item
					// is the first item in the local items list if any.

		inline	TListBoxLocalItemsListIter(TGenericListBoxObject *inst, ListBoxItemType *initial_item, bool make_step_up = FALSE);

		inline	void		Seek(ListBoxItemType *item);
							// Seek should be done only to visible non NULL items.

		inline	operator bool() const	{ return((bool)iter); }
		inline	void		operator ++()	{ if (iter == TRUE) ++iter; latest_shift_down_flag = TRUE; }
		inline	void		operator --()	{ if (iter == TRUE) --iter; latest_shift_down_flag = FALSE; }

		inline	int		CurrPos()		{ return((iter == TRUE) ? iter.CurrItem().ListBoxIndex() : ((latest_shift_down_flag == TRUE) ? num_items : -1)); }
							// Current pos returns the current reading position "between" the items.

		inline	ListBoxItemType	*CurrItem() const { return((iter == TRUE) ? &iter.CurrItem() : NULL); }
		inline	int		CurrItemHeight() { return((iter == TRUE) ? iter.CurrItem().GetItemHeight(owner) : TLB_DEFAULT_ITEM_HEIGHT); }

	private:

		ListBoxObjectType				*owner;
		TListIter<ListBoxItemType>		iter;
		int								num_items;
		bool								latest_shift_down_flag;
	};

	//
	// This iterator reads items from the listbox table or iterates the local list. This iterator is intended from iterating
	// areas that are not part of the foreground list. This iterator should be used only when list box is not empty.
	//
	class TListBoxListBoxDataIter
	{
	public:

		inline	TListBoxListBoxDataIter(TGenericListBoxObject *inst, int seek_pos);
					// After ctor this iterator is not functional yet. Second stage of initialization
					// should be call to MoveUp() or MoveDown().

		inline	~TListBoxListBoxDataIter();

		inline	bool		MoveUp();
		inline	bool		MoveDown();

		inline	ListBoxItemType *InitialItem() const { return(initial_item); }

		inline	int		CurrPos() const { return(curr_pos); }
							// Current pos shows current reading position "between" the items.
							// This method can be called at any time.

		inline	ListBoxItemType	*CurrItem() const { return(curr_item); }
		inline	int		CurrItemHeight() { return((curr_item != NULL) ? curr_item->GetItemHeight(owner) : TLB_DEFAULT_ITEM_HEIGHT); }
							// Current item and its height are valid _ONLY_ after successful call
							// to MoveDown/MoveUp. CurrItem() return value can be NULL.

		inline	void		PrependToLocalList() { owner->m_items_list.PrependList(local_list); }
		inline	void		AppendToLocalList() { owner->m_items_list.AppendList(local_list); }

	private:

		ListBoxObjectType				*owner;
		TListIter<ListBoxItemType>		iter;

		ListBoxItemType					*initial_item;		// First item that was given out. It has seek_pos index if the first shift
															// was down and seek_pos-1 if the first shift was up.
		int								initial_pos;			// Copy of the seek_pos ctor param.

		ListBoxItemType					*curr_item;
		int								curr_pos;			// It will be more correct to say that curr_pos always
															// stays "between" the items.
		TList								local_list;
	};

	//
	// This iterator iterates items in the local list of items. It is able to process lists in the local list mode
	// and in the ListBoxTable mode. In list box table mode local list may contain gaps that originate from
	// failures while reading items from the ListBoxTable.
	//
	class TListBoxFrgrItemsListIter
	{
	public:

		TListBoxFrgrItemsListIter(TGenericListBoxObject *inst);
			// Right after the ctor the top visible item becoomes the current item.

		//
		// This class allows to check/use current list item right after instantiating the iterator.
		// When iteration comes to the end of the list, CurrItem becomes NULL and CurrPos()
		// gains next value after the index of the last item in the list.
		//

		inline	int					CurrPos() const { return(curr_item_inx); }
		inline	ListBoxItemType		*CurrItem() const { return(curr_item); }
		inline	int					CurrItemHeight() { return((curr_item != NULL) ? curr_item->GetItemHeight(owner) : TLB_DEFAULT_ITEM_HEIGHT); }

		bool		StepIterUp();
		bool		StepIterDown();
					// Return value shows if the step was done or not.

	private:

		ListBoxObjectType				*owner;
		bool								moving_down_flag;

		ListBoxItemType					*curr_item;
		int								curr_item_inx;
		ListBoxItemType					*list_item_ptr;
		bool								list_ovfl_flag;
	};

	//
	// This intermediate structure is instantiated inside the methods only.
	// It temporarily stores the current state of the visible items.
	//
	struct TListBoxScrollState
	{
		TListBoxScrollState(TGenericListBoxObject *inst)
		{
			m_num_items = inst->m_num_items;
			m_upper_inx = inst->m_upper_item_inx;

			m_visib_items = inst->m_num_visible_items;
			m_vitms_height = inst->m_visible_items_height;
			m_area_height = inst->m_curr_list_area_height;
		}

		int  NumItems() { return(m_num_items); }

		bool IsBottomAligned()
		{
			return(m_upper_inx > 0 && m_upper_inx+m_visib_items >= m_num_items);
		}

		int CalcUpperInx(int new_num_items)
		{
			int upper_inx = 0;

			if (new_num_items > 0 && m_num_items > 0)
				upper_inx = (int)((m_upper_inx*new_num_items)/m_num_items);
			if (upper_inx >= new_num_items)
				upper_inx = new_num_items-1;

			return(upper_inx);
		}

	private:

		int			m_num_items;
		__int64		m_upper_inx;

		int			m_visib_items;
		int			m_vitms_height;
		int			m_area_height;
	};

	struct TListBoxSlctChangeInfoInternal
	{
		bool						new_selection;
		ListBoxItemType			*clicked_item;
		int						clicked_item_inx;
									// In single selection listboxes the clicked item is all the time selected.
									// In multi selection objects ckicked item can gain and loose selection.

		int						slct_chg_beg;
		int						slct_chg_len;
									// This is range of indexes that contains all items that became selected
									// or deselected as result of this change.
	};

	void		SetupListAreaHeight(int new_height);
	void		UpdateScroller();
	void		CheckInvisibleChangesAbove();

	bool		ProcessSlctChangeClick(int offs_y, bool shift_pressed, bool control_pressed, TListBoxSlctChangeInfoInternal &info);
				// Return value is TRUE when it is necessary to send notification to the parent window.

	void		RemoveSingleSelection();
	void		RemoveMultipleSelection();
	void		SetupSingleSelection(ListBoxItemType *item, int item_inx);
	void		ModifyMultipleSelection(ListBoxItemType *beg_item, int beg_item_inx, int cnt_items, bool new_slct_state);
	void		SetupItemsRangeSelection(ListBoxItemType *beg_item, int beg_item_inx, int cnt_items, bool new_slct_state);

	bool		GetItemSelection(ListBoxItemType *item, int item_inx);
	void		AddToMultiSelectionSumry(int item_inx);
	void		SetupListItemFromListBoxTable(ListBoxItemType *item);

	void		BuildItemIndexesAndSelectionVars();
	void		BuildListStateVars(int new_upper_item_inx, bool can_use_upper_item_ptr, TListBoxScrollState &prev_scroll_state);

	ListBoxItemType		*LocalListSeek(int item_inx, bool can_use_upper_item_ptr);
	ListBoxItemType		*FindFrgrItem(int item_inx);

	void		DisposeLocalItems();

	virtual bool		IsTitleItem(TListBoxItemCore *item) { return(FALSE); }

	virtual void		ScrollListArea(int area_beg, int area_len, int shift_val, bool update_now = TRUE) = 0;
						// Positive shift_val moves the passed area down.

	virtual void		InvalidateListArea(int area_beg = 0, int area_len = -1) = 0;
	virtual void		UpdateWindows() = 0;

	virtual void		SetupVertScroller(int nMax, int nPage, int nPos) = 0;
						// When param nMax <= 0, this means that scroller should be disabled.

	virtual void		DisposeVertScroller() = 0;

protected:

	ListBoxTableType				*m_items_table;					// This data field shows if the object is currently in
																// the "ListBoxTable" mode or not.
	TList							m_items_list;
	int							m_num_items;					// Current number of items in the listbox both for the items list mode and
																// for the listbox table mode.
	bool							m_bulk_changes_flag;

	TGenericListBoxObjectProps		m_props;						// By default the props are set to the single auto select mode.

	ListBoxItemType				*m_upper_item;
	int							m_upper_item_inx;				// At any point the upper side of some listbox item is aligned with
																// the upper side of the listbox items area. Current implementation
																// does not allow to scroll long listbox items in the short client area.

	int							m_num_visible_items;			// All visible items. Bottom item may be visible only partly. This height
	int							m_visible_items_height;			// can be smaller and bigger than the height of the items area.

	int							m_curr_list_area_height;

	ListBoxItemType				*m_slct_item;
	int							m_slct_item_inx;
									// Selection state for single selection listboxes.

	int							m_slct_item_beg_inx, m_slct_item_end_inx;
	int							m_slct_num_slct;
									// Selection state for multiple selection listboxes.
};

//-----------------------------------------------------------------------
//  ================  TListBoxLocalItemsListIter  ===================
//-----------------------------------------------------------------------

template <class ListBoxItemType, class ListBoxObjectType>
inline TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxLocalItemsListIter::TListBoxLocalItemsListIter(TGenericListBoxObject *inst)
		: iter(inst->m_items_list)
{
	// This iterator should be used when list box is in the local items list mode.
	assert(inst->m_items_table == NULL);

	owner = (ListBoxObjectType*)inst;
	num_items = inst->m_num_items;
	latest_shift_down_flag = TRUE;
}

template <class ListBoxItemType, class ListBoxObjectType>
inline TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxLocalItemsListIter::TListBoxLocalItemsListIter(TGenericListBoxObject *inst, ListBoxItemType *initial_item, bool make_step_up)
		: iter(inst->m_items_list, initial_item)
{
	// This iterator should be used when list box is in the local items list mode.
	assert(inst->m_items_table == NULL);

	if (make_step_up == FALSE)
	{
		// Initial item should belong to some listbox.
		assert(initial_item->ListBoxIndex() >= 0);
	}

	owner = (ListBoxObjectType*)inst;
	num_items = inst->m_num_items;
	latest_shift_down_flag = TRUE;

	if (make_step_up == TRUE)
		--iter;
};

template <class ListBoxItemType, class ListBoxObjectType>
inline void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxLocalItemsListIter::Seek(ListBoxItemType *item)
{
	assert(item != NULL);
	assert(item->ListBoxIndex() >= 0);

	iter.SeekToItem(item);
}

//-----------------------------------------------------------------------
//  =================  TListBoxListBoxDataIter  ===================
//-----------------------------------------------------------------------

template <class ListBoxItemType, class ListBoxObjectType>
TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxListBoxDataIter::TListBoxListBoxDataIter(TGenericListBoxObject *inst, int seek_pos)
		: iter(inst->m_items_list)
{
	assert(inst->m_num_items > 0);
	owner = (ListBoxObjectType*)inst;

	if (owner->m_items_table != NULL)
	{
		owner->m_items_table->Seek(seek_pos);
	}
	else
	{
		if (seek_pos < owner->m_num_items)
		{
			if (seek_pos < 0 )
				seek_pos = 0;

			ListBoxItemType *item = owner->LocalListSeek(seek_pos, TRUE);
			assert(item != NULL);

			iter.SeekToItem(item);
		}
		else
		{
			// Place the curr_pos exactly after the last item.
			seek_pos = owner->m_num_items;
		}
	}

	initial_item = curr_item = NULL;
	initial_pos = curr_pos = seek_pos;
}

template <class ListBoxItemType, class ListBoxObjectType>
TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxListBoxDataIter::~TListBoxListBoxDataIter()
{
	// Note: Local list can contain only items that were retrieved from the list box table.
	while (local_list.IsEmpty() == FALSE)
	{
		ListBoxItemType *item = (ListBoxItemType*)local_list.GetFirst();
		local_list.RemoveItem(item);
		owner->m_items_table->RecycleItem(item);
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
bool TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxListBoxDataIter::MoveUp()
{
	if (curr_pos <= 0)
		return(FALSE);

	if (owner->m_items_table != NULL)
	{
		curr_item = owner->m_items_table->ReadItem(FALSE);
		if (curr_item != NULL)
		{
			curr_item->SetListBoxIndex(curr_pos-1);
			owner->SetupListItemFromListBoxTable(curr_item);
			local_list.PrependItem(curr_item);
		}
	}
	else
	{
		if (curr_pos < owner->m_num_items)
		{
			assert(iter == TRUE);
			--iter;
		}
		else
		{
			iter.SeekToLast();
		}

		curr_item = (iter == TRUE) ? &iter.CurrItem() : NULL;
	}

	if (curr_pos == initial_pos)
		initial_item = curr_item;

	curr_pos--;
	return(TRUE);
}

template <class ListBoxItemType, class ListBoxObjectType>
bool TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxListBoxDataIter::MoveDown()
{
	if (curr_pos >= owner->m_num_items)
		return(FALSE);

	if (owner->m_items_table != NULL)
	{
		curr_item = owner->m_items_table->ReadItem(TRUE);
		if (curr_item != NULL)
		{
			curr_item->SetListBoxIndex(curr_pos);
			owner->SetupListItemFromListBoxTable(curr_item);
			local_list.AppendItem(curr_item);
		}
	}
	else
	{
		assert(iter == TRUE);
		curr_item = &iter.CurrItem();
		++iter;
	}

	if (curr_pos == initial_pos)
		initial_item = curr_item;

	curr_pos++;
	return(TRUE);
}

//---------------------------------------------------------------------
//  ================  TListBoxFrgrItemsListIter  ==================
//---------------------------------------------------------------------

template <class ListBoxItemType, class ListBoxObjectType>
TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxFrgrItemsListIter::TListBoxFrgrItemsListIter(TGenericListBoxObject *inst)
{
	assert(inst->m_num_items > 0);
	assert(inst->m_num_visible_items > 0);

	owner = (ListBoxObjectType*)inst;
	moving_down_flag = TRUE;

	curr_item_inx = owner->m_upper_item_inx;

	if (owner->m_upper_item != NULL)
	{
		list_item_ptr = owner->m_upper_item;
	}
	else
	{
		list_item_ptr = (ListBoxItemType*)owner->m_items_list.GetFirst();
	}

	if (list_item_ptr != NULL)
	{
		curr_item = (list_item_ptr->ListBoxIndex() == curr_item_inx) ? list_item_ptr : NULL;
	}
	else
	{
		curr_item = NULL;
	}

	list_ovfl_flag = FALSE;
}

template <class ListBoxItemType, class ListBoxObjectType>
bool TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxFrgrItemsListIter::StepIterUp()
{
	if (curr_item_inx <= owner->m_upper_item_inx)
	{
		// It is not possible to do the step.
		return(FALSE);
	}

	// Shift index to the required value.
	curr_item_inx--;

	if (moving_down_flag == TRUE)
	{
		moving_down_flag = FALSE;
		list_ovfl_flag = FALSE;
	}

	// Consider all different cases that may take place.
	if (list_item_ptr != NULL && list_ovfl_flag == FALSE)
	{
		if (curr_item_inx > list_item_ptr->ListBoxIndex())
		{
			curr_item = NULL;
		}
		else if (curr_item_inx == list_item_ptr->ListBoxIndex())
		{
			curr_item = list_item_ptr;
		}
		else
		{
			ListBoxItemType *item = (ListBoxItemType*)owner->m_items_list.GetPrev(list_item_ptr);

			if (item != NULL && item->ListBoxIndex() >= owner->m_upper_item_inx)
			{
				assert(item->ListBoxIndex() <= curr_item_inx);
				curr_item = (item->ListBoxIndex() == curr_item_inx) ? item : NULL;
				list_item_ptr = item;
			}
			else
			{
				curr_item = NULL;
				list_ovfl_flag = TRUE;
			}
		}
	}

	return(TRUE);
}

template <class ListBoxItemType, class ListBoxObjectType>
bool TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::TListBoxFrgrItemsListIter::StepIterDown()
{
	if (curr_item_inx >= owner->m_upper_item_inx+owner->m_num_visible_items-1)
	{
		// It is not possible to do the step.
		return(FALSE);
	}

	// Shift index to the required value.
	curr_item_inx++;

	if (moving_down_flag == FALSE)
	{
		moving_down_flag = TRUE;
		list_ovfl_flag = FALSE;
	}

	// Consider all different cases that may take place.
	if (list_item_ptr != NULL && list_ovfl_flag == FALSE)
	{
		if (curr_item_inx < list_item_ptr->ListBoxIndex())
		{
			curr_item = NULL;
		}
		else if (curr_item_inx == list_item_ptr->ListBoxIndex())
		{
			curr_item = list_item_ptr;
		}
		else
		{
			ListBoxItemType *item = (ListBoxItemType*)owner->m_items_list.GetNext(list_item_ptr);

			if (item != NULL && item->ListBoxIndex() < owner->m_upper_item_inx+owner->m_num_visible_items)
			{
				assert(item->ListBoxIndex() >= curr_item_inx);
				curr_item = (item->ListBoxIndex() == curr_item_inx) ? item : NULL;
				list_item_ptr = item;
			}
			else
			{
				curr_item = NULL;
				list_ovfl_flag = TRUE;
			}
		}
	}

	return(TRUE);
}

// Include bodies of the methods of the primary template.
#define	_GenericListBox_Implementation_
#include  "WinUi/GenericListBoxObject.inc"

#endif	// WinUi_ListBoxHelper_H


