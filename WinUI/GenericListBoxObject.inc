//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   The List Box Custom Control.
//

#ifndef	_GenericListBox_Implementation_
#error	This file should be included only from the ListBoxHelper.h.
#endif

//-------------------------------------------------------------------------------
//  ===================  TGenericListBoxObject  ========================
//-------------------------------------------------------------------------------

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::ResetListBox()
{
	// Dispose items before resetting the pointer to the list box table.
	DisposeLocalItems();

	m_items_table = NULL;
	m_num_items = 0;

	m_bulk_changes_flag = FALSE;
	m_props.PrepareForSingleSelect();

	m_upper_item = NULL;
	m_upper_item_inx = 0;

	m_num_visible_items = 0;
	m_visible_items_height = 0;
	m_curr_list_area_height = 0;

	m_slct_item = NULL;
	m_slct_item_inx = -1;

	m_slct_item_beg_inx = -1;
	m_slct_item_end_inx = -1;
	m_slct_num_slct = 0;
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SetupProps(TGenericListBoxObjectProps &new_props, bool rebuild_listbox_now)
{
	m_props = new_props;

	// It might make sense to delay rebuilding the listbox if list of items is already present and these items
	// are expecting certain columns schema. At the same time this columns might be not set yet.
	if (rebuild_listbox_now == TRUE)
		RebuildListBox();
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SetupListBoxTable(ListBoxTableType *table, bool paint_now)
{
	// Get rid of the current local items if any.
	DisposeLocalItems();

	// Switch to the list box table mode.
	m_items_table = table;
	RebuildListBox(-1, paint_now);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SetupNewItemsList(TList &list_to_bypass, bool paint_now)
{
	// Get rid of the current local items if any.
	DisposeLocalItems();

	// Switch to the local items list mode.
	m_items_table = NULL;
	m_items_list.TakeDataFrom(list_to_bypass);
	RebuildListBox(-1, paint_now);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::RebuildListBox(int upper_item_inx_proto, bool paint_now)
{
	// Update the list and number of items all the time.
	m_bulk_changes_flag = FALSE;

	BuildItemIndexesAndSelectionVars();

	// Update visible part only if there is place to show something.
	if (m_curr_list_area_height > 0)
	{
		TListBoxScrollState scroll_state(this);
		BuildListStateVars(upper_item_inx_proto, FALSE, scroll_state);
		InvalidateListArea();
		UpdateScroller();

		if (paint_now == TRUE)
		{
			UpdateWindows();
		}
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::AddItem(ListBoxItemType *item, ListBoxItemType *ins_before)
{
	// Data modification methods cannot be used in the listbox table mode.
	assert(m_items_table == NULL);
	assert(item != NULL);

	// Add item to the list.
	item->Clear();
	if (ins_before != NULL)
	{
		m_items_list.InsertBefore(item, ins_before);
	}
	else
	{
		m_items_list.AppendItem(item);
	}

	// Find visible item above the passed item if any. Use its index for the new item and for all subsequent items.
	TListBoxLocalItemsListIter iter0(this, item, TRUE);
	int inx = (iter0 == TRUE) ? iter0.CurrItem()->ListBoxIndex()+1 : 0;
	item->SetSelection(FALSE);

	if (m_bulk_changes_flag == TRUE)
	{
		// Rebuilding the ListBox will fix the rest.
		item->SetListBoxIndex(inx);
		return;
	}

	// Walk the list starting from the passed item up to its end. Update undexes and selection variables.
	for (TListIter<ListBoxItemType> iter(m_items_list, item); iter; ++iter)
	{
		ListBoxItemType &curr_item = iter.CurrItem();

		// Reassign the index.
		curr_item.SetListBoxIndex(inx);
		if (curr_item.IsSelected() == TRUE)
		{
			// Selection state has not changed. Update vars that are describing this selection state.
			if (m_props.multi_select == TRUE)
			{
				if (inx-1 == m_slct_item_beg_inx)
					m_slct_item_beg_inx = inx;
				if (inx-1 == m_slct_item_end_inx)
					m_slct_item_end_inx = inx;
			}
			else
			{
				m_slct_item_inx = inx;
			}
		}

		inx++;
	}

	// Save existing state of the list vars and update the num items.
	TListBoxScrollState scroll_state(this);
	assert(inx > m_num_items);
	m_num_items = inx;

	if (m_curr_list_area_height <= 0)
	{
		// There is no space to display anything.
		assert(m_num_visible_items == 0);
		return;
	}

	if (scroll_state.NumItems() == 0)
	{
		// Nothing was visible in the list before. Show new items starting from the top of the box.
		BuildListStateVars(0, FALSE, scroll_state);
		InvalidateListArea();
	}
	else
	{
		// Check correlation between the revealed items and the ones that were already visible.
		assert(m_upper_item != NULL);
		int major_item_height = item->GetItemHeight((ListBoxObjectType*)this);

		if (item->ListBoxIndex() < m_upper_item->ListBoxIndex())
		{
			// Revealed item is above items that are currently visible. Visible items should be left as-is if already
			// visible items are completely covering the window and bottom align rule is not violated.
			CheckInvisibleChangesAbove();
		}
		else if (item->ListBoxIndex() >= m_upper_item->ListBoxIndex()+m_num_visible_items)
		{
			// Revealed item is below the items that are currently visible.
			if (m_visible_items_height < m_curr_list_area_height)
			{
				// Major item should stay right after the last item.
				assert(item->ListBoxIndex() == m_upper_item_inx+m_num_visible_items);
				if (m_visible_items_height+major_item_height > m_curr_list_area_height)
				{
					// Major item is not fully visible.
					int num_to_conseal = 0;
					int height_to_conseal = 0;
					for (TListBoxLocalItemsListIter iter2(this, m_upper_item); iter2; --iter2)
					{
						num_to_conseal++;
						height_to_conseal += iter2.CurrItemHeight();

						if (m_visible_items_height-height_to_conseal+major_item_height <= m_curr_list_area_height)
							break;
					}

					BuildListStateVars(m_upper_item_inx-num_to_conseal, FALSE, scroll_state);
					InvalidateListArea();
				}
				else
				{
					// Figure out how many items should be painted below existing set of foreground items.
					int items_below = 0;
					int pixels_below = 0;
					for (TListBoxLocalItemsListIter iter3(this, item); iter3; ++iter3)
					{
						items_below++;
						pixels_below += iter3.CurrItemHeight();

						if (m_visible_items_height+pixels_below >= m_curr_list_area_height)
							break;
					}

					int old_visib_height = m_visible_items_height;
					m_num_visible_items += items_below;
					m_visible_items_height += pixels_below;
					InvalidateListArea(old_visib_height, (m_visible_items_height >= m_curr_list_area_height) ? -1 : pixels_below);
				}
			}
		}
		else
		{
			// Revealed item is somewhere in between of the visible items.
			int num_above_major = 0;
			int height_above_major = 0;
			int scroll_up_items_count = 0;

			// The major item is at least partly visible. Figure out the screen index and screen position of this item.
			for (TListBoxLocalItemsListIter iter4(this, m_upper_item); iter4; ++iter4)
			{
				if (item == iter4.CurrItem())
					break;

				num_above_major++;
				height_above_major += iter4.CurrItemHeight();
			}

			// Check the results of the loop.
			assert(m_upper_item_inx+num_above_major == item->ListBoxIndex());
			assert(height_above_major < m_curr_list_area_height);

			if (height_above_major+major_item_height > m_curr_list_area_height)
			{
				// Scroll the list up to make major item fully visible. The loop is similar to the one above.
				int hgt_with_major = height_above_major+major_item_height;
				for (TListBoxLocalItemsListIter iter5(this, m_upper_item); iter5; ++iter5)
				{
					if (item == iter5.CurrItem())
						break;

					hgt_with_major -= iter5.CurrItemHeight();
					scroll_up_items_count++;

					if (hgt_with_major <= m_curr_list_area_height)
						break;
				}

				// Check result of the loop.
				assert(scroll_up_items_count > 0);
				assert(scroll_up_items_count <= num_above_major);
			}

			if (scroll_up_items_count > 0)
			{
				// Rework the list state vars from scratch.
				BuildListStateVars(m_upper_item_inx+scroll_up_items_count, FALSE, scroll_state);
				InvalidateListArea();
			}
			else
			{
				// The changes in the visible area are not that complex. Figure out the total height of the revealed items.
				int num_new_visib = 0;
				int height_new_visib = 0;
				for (TListBoxLocalItemsListIter iter6(this, item); iter6; ++iter6)
				{
					num_new_visib++;
					height_new_visib += iter6.CurrItemHeight();

					if (height_above_major+height_new_visib >= m_curr_list_area_height)
						break;
					if (iter6.CurrItem() == item)
						break;
				}

				// Interpret the results of the latest loop.
				if (height_above_major+height_new_visib >= m_curr_list_area_height)
				{
					// The bottom part is fully covered with new items. Update the list state vars and invalidate this area.
					m_num_visible_items = num_above_major+num_new_visib;
					m_visible_items_height = height_above_major+height_new_visib;
					InvalidateListArea(height_above_major);
				}
				else
				{
					// Find out how many old items will be still visible below the new items that became visible.
					int num_old_survived = 0;
					int height_old_survived = 0;
					int old_surv_new_beg = height_above_major+height_new_visib;

					TListBoxLocalItemsListIter iter7(this, item);
					for (++iter7; iter7; ++iter7)
					{
						num_old_survived++;
						height_old_survived += iter7.CurrItemHeight();

						if (old_surv_new_beg+height_old_survived >= m_curr_list_area_height)
							break;
					}

					// Fill in the list state variables.
					m_num_visible_items = num_above_major+num_new_visib+num_old_survived;
					m_visible_items_height = old_surv_new_beg+height_old_survived;

					// Process the screen according to the results above.
					if (num_old_survived > 0)
					{
						// Survived area of the window should be scrolled down.
						ScrollListArea(height_above_major, m_curr_list_area_height-old_surv_new_beg, height_new_visib);
					}
					else
					{
						// There is nothing to scroll. Simply invalidate the area that contains the new items.
						InvalidateListArea(height_above_major, height_new_visib);
					}
				}
			}
		}

	}

	// Finally reflect these changes on the scroller.
	UpdateScroller();
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::RemoveItem(ListBoxItemType *item)
{
	// Data modification methods cannot be used in the listbox table mode.
	assert(m_items_table == NULL);
	assert(item != NULL);

	if (item->ListBoxIndex() < 0)
	{
		// Passed item is not part of the listbox.
		return;
	}

	assert(m_num_items > 0);

	// Note that ownership on the item is transfered to the caller.
	ListBoxItemType *prev_item = (ListBoxItemType*)m_items_list.GetPrev(item);
	ListBoxItemType *next_item = (ListBoxItemType*)m_items_list.GetNext(item);
	m_items_list.RemoveItem(item);

	if (m_bulk_changes_flag == TRUE)
	{
		item->SetListBoxIndex(-1);
		return;
	}

	int hidden_inx = item->ListBoxIndex();
	int height_visib_hidden = item->GetItemHeight((ListBoxObjectType*)this);
	item->SetListBoxIndex(-1);

	if (hidden_inx == m_slct_item_inx && m_props.multi_select == FALSE)
	{
		// A single selected item gets removed.
		m_slct_item = NULL;
		m_slct_item_inx = -1;
	}

	int inx = hidden_inx;
	if (next_item != NULL)
	{
		// Walk the list below the passed item. Update undexes and single selection variables.
		for (TListBoxLocalItemsListIter iter2(this, next_item); iter2; ++iter2)
		{
			ListBoxItemType &curr_item = *iter2.CurrItem();

			// This is some item below. Reassign the index.
			if (curr_item.IsSelected() == TRUE && m_props.multi_select == FALSE)
			{
				if (curr_item.ListBoxIndex() == m_slct_item_inx)
					m_slct_item_inx = inx;
			}

			curr_item.SetListBoxIndex(inx++);
		}
	}

	// Update the number of items.
	assert(inx < m_num_items);
	m_num_items = inx;

	if (m_props.multi_select == TRUE &&  item->IsSelected() == TRUE)
	{
		// The item, that is being deleted, belongs to the multiselection. Update the variables.
		assert(m_slct_num_slct > 0);

		if (--m_slct_num_slct == 0)
		{
			// Selection disappears.
			m_slct_item_beg_inx = m_slct_item_end_inx = -1;
		}
		else
		{
			if (hidden_inx == m_slct_item_beg_inx)
			{
				// Fix the min index of the selection boundary.
				assert(next_item != NULL);
				m_slct_item_beg_inx = -1;
				for (TListBoxLocalItemsListIter iter3(this, next_item); iter3; ++iter3)
				{
					if (iter3.CurrItem()->IsSelected() == TRUE)
					{
						m_slct_item_beg_inx = iter3.CurrPos();
						break;
					}
				}

				assert(m_slct_item_beg_inx >= 0);
			}

			if (hidden_inx == m_slct_item_end_inx)
			{
				// Fix the max index of the selection boundary.
				assert(prev_item != NULL);
				m_slct_item_end_inx = -1;
				for (TListBoxLocalItemsListIter iter4(this, prev_item, TRUE); iter4; --iter4)
				{
					if (iter4.CurrItem()->IsSelected() == TRUE)
					{
						m_slct_item_end_inx = iter4.CurrPos();
						break;
					}
				}

				assert(m_slct_item_end_inx >= 0);
			}
		}
	}

	if (m_curr_list_area_height <= 0)
	{
		// There is no space to display anything.
		assert(m_num_visible_items == 0);
		return;
	}

	TListBoxScrollState scroll_state(this);
	assert(m_num_visible_items > 0);
	if (m_num_items == 0)
	{
		// The last item was removed. There are no items in the list now.
		BuildListStateVars(0, FALSE, scroll_state);
		InvalidateListArea();
	}
	else
	{
		if (hidden_inx < m_upper_item_inx)
		{
			// All hidden items were above items that are currently visible. Visible part should be left as-is
			// if already visible items were completely covering the window and the bottom align rule has not
			// got violated.
			CheckInvisibleChangesAbove();
		}
		else if (hidden_inx < m_upper_item_inx+m_num_visible_items)
		{
			// Items that were just hidden were at least partly visible.
			bool rebuild_whole_screen = FALSE;
			if (hidden_inx < m_upper_item_inx)
			{
				// The top item was invisible, while some other items were in the curr window. This situation has
				// many posiblitites and it does not seem to happen often. Simply rebuild the state and repaint
				// the whole window.
				rebuild_whole_screen = TRUE;
			}
			else
			{
				// Top item in the set of items that were just hidden was visible on the screen, at least partly.
				ListBoxItemType *new_upper_item = NULL;

				// Get info about of the area that survived above the gap.
				int num_above_the_gap = 0;
				int height_above_the_gap = 0;
				TListBoxLocalItemsListIter iter5(this);
				if (hidden_inx > m_upper_item_inx)
				{
					new_upper_item = m_upper_item;
					for (iter5.Seek(m_upper_item); iter5; ++iter5)
					{
						if (iter5.CurrPos() >= hidden_inx)
							break;

						num_above_the_gap++;
						height_above_the_gap += iter5.CurrItemHeight();
					}
				}
				else
				{
					// Items, that were hidden, start right from the top.
					if (next_item != NULL)
					{
						new_upper_item = next_item;
						iter5.Seek(next_item);
					}
				}

				// Get info about the area below the gap.
				int num_below_the_gap = 0;
				int height_avail = height_above_the_gap;
				if (new_upper_item != NULL)
				{
					for (; iter5; ++iter5)
					{
						num_below_the_gap++;
						height_avail += iter5.CurrItemHeight();

						if (height_avail >= m_curr_list_area_height)
							break;
					}
				}

				// Check the overall results.
				if (new_upper_item == NULL || (height_avail < m_curr_list_area_height && new_upper_item->ListBoxIndex() > 0))
				{
					// Conversion of the curr screen image is too complex. Redo everything from scratch.
					rebuild_whole_screen = TRUE;
				}
				else
				{
					// New list state vars are already clear.
					m_upper_item = new_upper_item;
					m_upper_item_inx = new_upper_item->ListBoxIndex();
					m_num_visible_items = num_below_the_gap+num_above_the_gap;
					m_visible_items_height = height_avail;

					if (height_above_the_gap+height_visib_hidden < m_curr_list_area_height)
					{
						// Some stripe on the bottom sirvived.
						ScrollListArea(height_above_the_gap+height_visib_hidden,
									m_curr_list_area_height-height_above_the_gap+height_visib_hidden,
									-height_visib_hidden);
					}
					else
					{
						// The bottom part shouls be simply repainted.
						InvalidateListArea(height_above_the_gap);
					}
				}
			}

			if (rebuild_whole_screen == TRUE)
			{
				int new_upper_inx = (next_item != NULL) ? next_item->ListBoxIndex() : m_num_items-1;
				BuildListStateVars(new_upper_inx, FALSE, scroll_state);
				InvalidateListArea();
			}
		}
	}

	// Finally reflect these changes in the scroller.
	UpdateScroller();
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::DeleteAllItems()
{
	assert(m_items_table == NULL);
	DisposeLocalItems();
	RebuildListBox();
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::InvalidateItem(ListBoxItemType *item, int num_items_to_invalidate)
{
	// Check, if caller has passed something to invalidate or not.
	if (item != NULL && num_items_to_invalidate > 0)
	{
		InvalidateItemByIndex(item->ListBoxIndex(), num_items_to_invalidate);
	}

}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::InvalidateItemByIndex(int index, int num_items_to_invalidate)
{
	if (m_num_items <= 0 || m_curr_list_area_height <= 0 || num_items_to_invalidate <= 0)
		return;

	// Check, if this item is visible or not.
	if (index+num_items_to_invalidate <= m_upper_item_inx || index >= m_upper_item_inx+m_num_visible_items)
	{
		return;
	}

	// If the upper part of items is not visible, adjust the requested range.
	if (index < m_upper_item_inx)
	{
		num_items_to_invalidate -= m_upper_item_inx-index;
		index = m_upper_item_inx;

		if (num_items_to_invalidate <= 0)
			return;
	}

	int area_top = 0;
	TListBoxFrgrItemsListIter iter(this);
	while (iter.CurrPos() < index)
	{
		area_top += iter.CurrItemHeight();
		iter.StepIterDown();
	}

	int area_bottom = area_top;
	while (num_items_to_invalidate-- > 0)
	{
		area_bottom += iter.CurrItemHeight();
		if (iter.StepIterDown() == FALSE)
			break;
	}

	if (area_bottom > m_curr_list_area_height)
		area_bottom = m_curr_list_area_height;

	if (area_bottom > area_top)
		InvalidateListArea(area_top, area_bottom-area_top);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::UpdateItemHeight(ListBoxItemType *item, int new_height)
{
	int old_item_height = item->GetCachedHeight();
	if (new_height <= 0)
	{
		new_height = item->GetItemHeight((ListBoxObjectType*)this);
	}
	else
	{
		item->SetCachedHeight((short)new_height);
	}

	// Check, if there are any visible changes or not.
	if (new_height == old_item_height)
		return;

	if (IsTitleItem(item) == TRUE || m_items_table != NULL)
		return;

	if (m_curr_list_area_height <= 0)
		return;

	TListBoxScrollState scroll_state(this);
	int inx = item->ListBoxIndex();

	// Process the "out of window" situation.
	if (inx < m_upper_item_inx || inx >= m_upper_item_inx+m_num_visible_items)
	{
		if (inx == m_upper_item_inx-1 && m_visible_items_height+new_height <= m_curr_list_area_height)
		{
			// The bottom alignment rule got violated. Use a generic procedure because this sitiation should be very rare.
			BuildListStateVars(-1, TRUE, scroll_state);
			InvalidateListArea();
			UpdateScroller();
		}

		return;
	}

	// The item is currenlty visible at least partly. Figure out the height of items above this one.
	int hgt_above = 0;
	TListBoxLocalItemsListIter iter1(this, m_upper_item);
	for (; iter1.CurrPos() < inx; ++iter1)
	{
		hgt_above += iter1.CurrItemHeight();
	}

	// Invalidate the area of the passed item itself.
	InvalidateListArea(hgt_above, new_height);

	// Figure out situation below the item.
	int new_hgt_visib = hgt_above+new_height;
	for (++iter1; iter1; ++iter1)
	{
		if (new_hgt_visib >= m_curr_list_area_height)
			break;

		new_hgt_visib += iter1.CurrItemHeight();
	}

	if (new_height > old_item_height)
	{
		// The item got bigger. The upper item cannot change. Some of the items may go out of sight.
		m_num_visible_items = iter1.CurrPos()-m_upper_item_inx;
		m_visible_items_height = new_hgt_visib;
		int bottom_surv_beg = hgt_above+new_height;
		if (bottom_surv_beg < m_curr_list_area_height)
		{
			// Some bottom part survived. Scroll it down.
			int shift = new_height-old_item_height;
			int len = m_curr_list_area_height-bottom_surv_beg-shift;
			ScrollListArea(bottom_surv_beg, len, shift);
		}
	}
	else
	{
		// The item height got smaller. Check the bottom aligment rule.
		if (new_hgt_visib < m_curr_list_area_height && m_upper_item_inx > 0)
		{
			BuildListStateVars(-1, TRUE, scroll_state);
			InvalidateListArea();
			UpdateScroller();
			return;
		}

		// The sitation is still not bottom aligned. Some items may come into the sight.
		m_num_visible_items = iter1.CurrPos()-m_upper_item_inx;
		m_visible_items_height = new_hgt_visib;
		int bottom_surv_beg = hgt_above+old_item_height;
		if (bottom_surv_beg < m_curr_list_area_height)
		{
			// Some bottom part survived. Scroll it up.
			int len = m_curr_list_area_height-bottom_surv_beg;
			ScrollListArea(bottom_surv_beg, len, new_height-old_item_height);
		}
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::UpdateItemHeightByIndex(int listbox_index, int new_height)
{
	assert(m_items_table == NULL);

	if (listbox_index < 0 || listbox_index >= m_num_items)
		return;

	// Figure out item with the passed index.
	ListBoxItemType *item = LocalListSeek(listbox_index, TRUE);
	assert(item != NULL);

	// Call a more generic function.
	UpdateItemHeight(item, new_height);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SelectItem(ListBoxItemType *item, bool new_slct_stt)
{
	if (new_slct_stt == FALSE && m_props.multi_select == FALSE)
	{
		// Do not check the item at all.
		RemoveSingleSelection();
		return;
	}

	if (item != NULL)
	{
		if (item->ListBoxIndex() >= 0)
		{
			assert(item->ListBoxIndex() < m_num_items);
			if (m_props.multi_select == TRUE)
			{
				ModifyMultipleSelection(item, item->ListBoxIndex(), 1, new_slct_stt);
			}
			else
			{
				SetupSingleSelection(item, item->ListBoxIndex());
			}
		}
		else
		{
			assert(m_items_table == NULL);

			// Just modify the slct flag of the invisible item.
			item->SetSelection(new_slct_stt);
		}
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SelectItemByIndex(int listbox_index, bool new_slct_stt)
{
	if (new_slct_stt == FALSE && m_props.multi_select == FALSE)
	{
		// Do not check the value of the item index at all.
		RemoveSingleSelection();
		return;
	}

	if (listbox_index < 0 || listbox_index >= m_num_items)
	{
		// It is not clear what the upper layer wants.
		return;
	}

	if (m_props.multi_select == TRUE)
	{
		ModifyMultipleSelection(NULL, listbox_index, 1, new_slct_stt);
	}
	else
	{
		SetupSingleSelection(NULL, listbox_index);
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SelectItemsRange(int inx_beg, int range_len, bool new_slct_state)
{
	// Check the input params.
	assert(inx_beg >= 0 && range_len >= 0 && inx_beg+range_len <= m_num_items);

	if (m_num_items <= 0 || range_len <= 0)
		return;

	if (m_props.multi_select == FALSE)
	{
		// Process simple single selection case.
		assert(range_len == 1);
		if (new_slct_state == TRUE)
		{
			SetupSingleSelection(NULL, inx_beg);
		}
		else
		{
			RemoveSingleSelection();
		}
	}
	else
	{
		ModifyMultipleSelection(NULL, inx_beg, range_len, new_slct_state);
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::RemoveSelection()
{
	if (m_props.multi_select == TRUE)
	{
		RemoveMultipleSelection();
	}
	else
	{
		RemoveSingleSelection();
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
int TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetSelectionIndex()
{
	if (m_props.multi_select == TRUE)
		return(-1);
	return(m_slct_item_inx);
}

template <class ListBoxItemType, class ListBoxObjectType>
ListBoxItemType *TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetSelectedItem()
{
	if (m_items_table != NULL || m_props.multi_select == TRUE)
		return(NULL);
	return(m_slct_item);
}

template <class ListBoxItemType, class ListBoxObjectType>
bool  TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetItemMultiSelectState(int listbox_index)
{
	if (m_props.multi_select == FALSE)
		return(FALSE);
	return(GetItemSelection(NULL, listbox_index));
}

template <class ListBoxItemType, class ListBoxObjectType>
int TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetSelectionSummary(int &slct_beg, int &slct_end)
{
	if (m_props.multi_select == FALSE)
	{
		// Simple single selection case.
		if (m_slct_item_inx < 0)
		{
			slct_beg = slct_end = -1;
			return(0);
		}
		else
		{
			slct_beg = slct_end = m_slct_item_inx;
			return(1);
		}
	}

	if (m_items_table != NULL && m_items_table->IsMultiSelSupported() == TRUE)
	{
		// Let table handle this request.
		return(m_items_table->GetSelectionSummary(slct_beg, slct_end));
	}

	// Give out the current state of the vars.
	slct_beg = m_slct_item_beg_inx;
	slct_end = m_slct_item_end_inx;
	return(m_slct_num_slct);
}

template <class ListBoxItemType, class ListBoxObjectType>
bool TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetNextSelectedRange(int seek_pos, int &range_beg_inx, int &range_len, bool dir_forward)
{
	if (m_props.multi_select == FALSE)
	{
		// Bogus call.
		assert(FALSE);
		return(FALSE);
	}

	range_len = 0;
	range_beg_inx = -1;
	if (m_items_table != NULL && m_items_table->IsMultiSelSupported() == TRUE)
	{
		// ListBox table should handle this request.
		return(m_items_table->GetNextSelectedRange(seek_pos, range_beg_inx, range_len, dir_forward));
	}

	// Check, if it is necessary to give out any range or not.
	if (m_slct_num_slct <= 0)
		return(FALSE);
	if (dir_forward == FALSE && seek_pos <= m_slct_item_beg_inx)
		return(FALSE);
	if (dir_forward == TRUE && seek_pos > m_slct_item_end_inx)
		return(FALSE);

	assert(m_num_items > 0);
	if (m_slct_item_end_inx-m_slct_item_beg_inx+1 == m_slct_num_slct)
	{
		// The selection is a single contiguous range.
		range_beg_inx = m_slct_item_beg_inx;
		range_len = m_slct_num_slct;
	}
	else
	{
		// This should be the listbox with local items.
		assert(m_items_table == NULL);

		// Check state of the local items list.
		ListBoxItemType *range_beg_item = NULL, *range_end_item = NULL;
		if (seek_pos <= m_slct_item_beg_inx)
		{
			// Single down procedure.
			assert(dir_forward == TRUE);
			range_beg_item = LocalListSeek(m_slct_item_beg_inx, TRUE);
			TListBoxLocalItemsListIter iter1(this, range_beg_item);
			while (iter1.CurrItem()->IsSelected() == TRUE)
			{
				range_end_item = iter1.CurrItem();
				++iter1;
			}
		}
		else if (seek_pos >= m_slct_item_end_inx)
		{
			// Single up procedure.
			range_end_item = LocalListSeek(m_slct_item_end_inx, TRUE);
			TListBoxLocalItemsListIter iter2(this, range_end_item);
			while (iter2.CurrItem()->IsSelected() == TRUE)
			{
				range_end_item = iter2.CurrItem();
				--iter2;
			}
		}
		else
		{
			ListBoxItemType *item = LocalListSeek(seek_pos, TRUE);
			TListBoxLocalItemsListIter iter3(this, item);
			bool mid_range_proc = FALSE;
			if (item->IsSelected() == TRUE)
			{
				if (dir_forward == TRUE)
				{
					mid_range_proc = TRUE;
				}
				else
				{
					--iter3;
					assert(iter3 == TRUE);
					if (iter3.CurrItem()->IsSelected() == TRUE)
						mid_range_proc = TRUE;
				}
			}

			if (mid_range_proc == TRUE)
			{
				// Up-Down procedure.
				for (; iter3; --iter3)
				{
					range_beg_item = iter3.CurrItem();
					if (iter3.CurrItem()->IsSelected() == FALSE)
						break;
				}

				for (iter3.Seek(item); iter3; ++iter3)
				{
					range_end_item = iter3.CurrItem();
					if (iter3.CurrItem()->IsSelected() == FALSE)
						break;
				}
			}
			else if (dir_forward == TRUE)
			{
				// Down-Down procedure.
				while (iter3.CurrItem()->IsSelected() == FALSE)
					++iter3;

				assert(iter3 == TRUE);
				range_beg_item = iter3.CurrItem();
				while (iter3.CurrItem()->IsSelected() == TRUE)
				{
					range_end_item = iter3.CurrItem();
					++iter3;
				}
			}
			else
			{
				// Up-Up procedure.
				while (iter3.CurrItem()->IsSelected() == FALSE)
					--iter3;

				assert(iter3 == TRUE);
				range_end_item = iter3.CurrItem();
				while (iter3.CurrItem()->IsSelected() == TRUE)
				{
					range_beg_item = iter3.CurrItem();
					--iter3;
				}
			}
		}

		// Check the results of the search.
		assert(range_beg_item != NULL);
		assert(range_end_item != NULL);
		range_beg_inx = range_beg_item->ListBoxIndex();
		range_len = range_end_item->ListBoxIndex()-range_beg_inx+1;
	}

	assert(range_len > 0);
	return(TRUE);
}

template <class ListBoxItemType, class ListBoxObjectType>
int TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetNumOwnedItems()
{
	assert(m_items_table == NULL);
	int cnt_owned_items = 0;
	for (TListIter<ListBoxItemType> iter(m_items_list); iter; ++iter)
	{
		if (iter.CurrItem().IsOwned() == TRUE)
			cnt_owned_items++;
	}

	return(cnt_owned_items);
}

template <class ListBoxItemType, class ListBoxObjectType>
ListBoxItemType *TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetItemByIndex(int index)
{
	assert(m_items_table == NULL);
	if (index < 0 || index >= m_num_items)
		return(NULL);
	return(LocalListSeek(index, TRUE));
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SetTopIndex(int pos)
{
	if (m_num_items == 0 || m_curr_list_area_height == 0)
	{
		// There is no list or there is no screen space to display it.
		assert(m_upper_item_inx == 0);
		return;
	}

	// Fix the passed index.
	if (pos < 0)
		pos = 0;
	if (pos >= m_num_items)
		pos = m_num_items-1;

	if (pos == m_upper_item_inx)
	{
		// Requested position is the same to the current one.
		return;
	}

	assert(m_num_visible_items > 0);
	assert(m_visible_items_height > 0);
	if (pos < m_upper_item_inx)
	{
		// Iterate new items until the upper visible item will show up or iteration will find enough items
		// to cover the whole window.
		TListBoxListBoxDataIter iter1(this, pos);
		int new_items_hgt = 0;
		while (iter1.CurrPos() < m_upper_item_inx)
		{
			iter1.MoveDown();
			new_items_hgt += iter1.CurrItemHeight();
			if (new_items_hgt >= m_curr_list_area_height)
				break;
		}

		if (new_items_hgt >= m_curr_list_area_height)
		{
			// New items have nothing to do with the old ones.
			m_upper_item = iter1.InitialItem();
			m_upper_item_inx = pos;
			m_num_visible_items = iter1.CurrPos()-pos;
			m_visible_items_height = new_items_hgt;
			if (m_items_table != NULL)
			{
				DisposeLocalItems();
				iter1.AppendToLocalList();
			}

			InvalidateListArea();
		}
		else
		{
			// Some amount of the old items survied.
			assert(iter1.CurrPos() == m_upper_item_inx);

			// Figure out how many old items should be still visible.
			int survived_hgt = 0;
			TListBoxFrgrItemsListIter iter2(this);
			do
			{
				survived_hgt += iter2.CurrItemHeight();
				if (new_items_hgt+survived_hgt >= m_curr_list_area_height)
					break;
			}
			while (iter2.StepIterDown() == TRUE);

			// Modify the list state vars and scroll the area.
			m_upper_item = iter1.InitialItem();
			m_upper_item_inx = pos;
			m_num_visible_items = iter2.CurrPos()+1-pos;
			m_visible_items_height = new_items_hgt+survived_hgt;
			ScrollListArea(0, m_curr_list_area_height-new_items_hgt, new_items_hgt, FALSE);

			if (m_items_table != NULL)
			{
				// Move items from the helper to the beg of the list.
				iter1.PrependToLocalList();

				// Recycle items that are going out of sight.
				for(;;)
				{
					ListBoxItemType *item = (ListBoxItemType*)m_items_list.GetLast();
					if (item == NULL || item->ListBoxIndex() <= iter2.CurrPos())
						break;

					m_items_list.RemoveItem(item);
					m_items_table->RecycleItem(item);
				}
			}
		}
	}
	else
	{
		//
		// This is an attempt to scroll the whole list down.
		//
		if (m_num_items == 1 || m_num_visible_items == m_num_items && m_visible_items_height <= m_curr_list_area_height)
		{
			// List can be only in the one possible state.
			assert(m_upper_item_inx == 0);
			return;
		}

		if (pos < m_upper_item_inx+m_num_visible_items)
		{
			// Some of the old items will be still visible. Figure out the total height of the items
			// that are about to go away.
			int go_away_height = 0;
			TListBoxFrgrItemsListIter iter3(this);
			while (iter3.CurrPos() < pos)
			{
				go_away_height += iter3.CurrItemHeight();
				iter3.StepIterDown();
			}

			int survived_height = m_visible_items_height-go_away_height;

			// Reveal invisible items from the bottom part of the list.
			int revealed_items_height = 0;
			TListBoxListBoxDataIter iter4(this, m_upper_item_inx+m_num_visible_items);
			while (iter4.MoveDown() == TRUE)
			{
				revealed_items_height += iter4.CurrItemHeight();
				if (survived_height+revealed_items_height >= m_curr_list_area_height)
					break;
			}

			if (m_items_table != NULL)
				iter4.AppendToLocalList();

			// Figure out if the revealed items managed to cover the window or not.
			if (survived_height+revealed_items_height >= m_curr_list_area_height)
			{
				// It is ok to use the passed pos as a new top item of the window.
				m_upper_item = iter3.CurrItem();
				m_upper_item_inx = pos;
				m_num_visible_items = iter4.CurrPos()-pos;
				m_visible_items_height = survived_height+revealed_items_height;
				ScrollListArea(go_away_height, m_curr_list_area_height-go_away_height, -go_away_height, FALSE);

				if (m_items_table != NULL)
				{
					// Recycle items that are going out of sight.
					for(;;)
					{
						ListBoxItemType *item = (ListBoxItemType*)m_items_list.GetFirst();
						if (item == NULL || item->ListBoxIndex() >= pos)
							break;

						m_items_list.RemoveItem(item);
						m_items_table->RecycleItem(item);
					}
				}
			}
			else
			{
				// The position value is too big. Make one more loop. This loop should stop when the height of the list will be
				// still less or equal to the height of the window and adding one more item should make this height bigger than
				// the height of the window.
				int hgt_avail = survived_height+revealed_items_height;
				while (iter3.CurrPos() > m_upper_item_inx)
				{
					iter3.StepIterUp();
					int hgt_item = iter3.CurrItemHeight();
					if (hgt_avail+hgt_item > m_curr_list_area_height)
					{
						// The height became big enough. Do not accept this item.
						iter3.StepIterDown();
						break;
					}

					hgt_avail += hgt_item;
					go_away_height -= hgt_item;
				}

				if (iter3.CurrPos() > m_upper_item_inx)
				{
					// Position was shifted at least for one item.
					m_upper_item = iter3.CurrItem();
					m_upper_item_inx = iter3.CurrPos();
					m_num_visible_items = iter4.CurrPos()-iter3.CurrPos();
					m_visible_items_height = hgt_avail;
					ScrollListArea(go_away_height, m_curr_list_area_height-go_away_height, -go_away_height, FALSE);

					if (m_items_table != NULL)
					{
						// Recycle items that are going out of sight.
						for(;;)
						{
							ListBoxItemType *item = (ListBoxItemType*)m_items_list.GetFirst();
							if (item == NULL || item->ListBoxIndex() >= m_upper_item_inx)
								break;

							m_items_list.RemoveItem(item);
							m_items_table->RecycleItem(item);
						}
					}
				}
				else
				{
					// The list is already at its bottom position.
					assert(revealed_items_height == 0);
					return;
				}
			}
		}
		else
		{
			// New area stays compleltely below the current visible area.
			TListBoxScrollState scroll_state(this);
			BuildListStateVars(pos, TRUE, scroll_state);
			InvalidateListArea();
		}
	}

	// The list vars are ok and the proper invalidation is done.
	UpdateScroller();
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::ScrollItemIntoTheView(ListBoxItemType *item, bool enforce_margins)
{
	if (item == NULL || item->ListBoxIndex() < 0)
		return;
	ScrollItemIntoTheView(item->ListBoxIndex(), enforce_margins);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::ScrollItemIntoTheView(int inx_to_show, bool enforce_margins)
{
	if (m_num_items <= 1 || m_curr_list_area_height <= 0)
	{
		// There is no list (one item list is not scrollable) or there is no space to display it.
		assert(m_upper_item_inx == 0);
		return;
	}

	if (m_num_visible_items == m_num_items && m_visible_items_height <= m_curr_list_area_height)
	{
		// The list is not scrollable.
		assert(m_upper_item_inx == 0);
		return;
	}

	// Fix the passed index.
	if (inx_to_show < 0)
		inx_to_show = 0;
	if (inx_to_show >= m_num_items)
		inx_to_show = m_num_items-1;

	// Pick up the proper margin values and fix them.
	int num_upper_lines = (enforce_margins == TRUE) ? m_props.upper_scroll_offset : 0;
	int num_lower_lines = (enforce_margins == TRUE) ? m_props.lower_scroll_offset : 0;

	if (num_upper_lines > inx_to_show)
		num_upper_lines = inx_to_show;
	if (num_lower_lines > m_num_items-1-inx_to_show)
		num_lower_lines = m_num_items-1-inx_to_show;

	// Evaluate the current scroll position.
	int bottom_inx = m_upper_item_inx+m_num_visible_items-1;
	if (inx_to_show-num_upper_lines >= m_upper_item_inx && inx_to_show+num_lower_lines <= bottom_inx)
	{
		// Check the case with partial visibility of the bottom_inx item.
		if (inx_to_show+num_lower_lines < bottom_inx || m_visible_items_height <= m_curr_list_area_height)
		{
			// Existing state of the list is ok.
			return;
		}
	}

	//
	// Either the current scrolling state needs to be changed or it could be that the existing window is
	// not high enough to show the requested margins. At this point items height needs evaluation.
	//

	// Pick up the height of the target item and lines around it.
	int main_item_hgt, upper_item_heights[100], lower_item_heights[100];
	{
		// Place iterator into the block to reduce the listbox items lifetime.
		TListBoxListBoxDataIter iter1(this, inx_to_show);
		iter1.MoveDown();
		main_item_hgt = iter1.CurrItemHeight();
		if (main_item_hgt >= m_curr_list_area_height)
		{
			SetTopIndex(inx_to_show);
			return;
		}

		num_lower_lines = __min(num_lower_lines, 100);
		int cnt = 0;
		while (cnt < num_lower_lines)
		{
			iter1.MoveDown();
			lower_item_heights[cnt++] = iter1.CurrItemHeight();
		}
	}

	if (num_upper_lines > 0)
	{
		TListBoxListBoxDataIter iter2(this, inx_to_show);
		num_upper_lines = __min(num_upper_lines, 100);
		int cnt = 0;

		while (cnt < num_upper_lines)
		{
			iter2.MoveUp();
			upper_item_heights[cnt++] = iter2.CurrItemHeight();
		}
	}

	// Find out how many items above and below will fit in the existing window.
	int upper_items = 0, lower_items = 0;
	int visible_hgt = main_item_hgt;
	while (upper_items < num_upper_lines || lower_items < num_lower_lines)
	{
		if (upper_items < num_upper_lines)
		{
			// Try to add one more upper items.
			int item_hgt = upper_item_heights[upper_items];
			if (visible_hgt+item_hgt <= m_curr_list_area_height)
			{
				visible_hgt += item_hgt;
				upper_items++;
			}
			else
			{
				break;
			}
		}

		if (lower_items < num_lower_lines)
		{
			// Try to add one more lower item.
			int item_hgt = lower_item_heights[lower_items];
			if (visible_hgt+item_hgt <= m_curr_list_area_height)
			{
				visible_hgt += item_hgt;
				lower_items++;
			}
			else
			{
				break;
			}
		}
	}

	// Check, if the desired item is too high or it is not visible.
	if (inx_to_show-upper_items < m_upper_item_inx)
	{
		SetTopIndex(inx_to_show-upper_items);
		return;
	}

	// Check, if the desired item is too low or not.
	if (inx_to_show+lower_items > bottom_inx || inx_to_show+lower_items == bottom_inx && m_visible_items_height > m_curr_list_area_height)
	{
		// Find sutable top item based on the desireable bottom item.
		int new_top_inx = inx_to_show-upper_items;
		TListBoxListBoxDataIter iter3(this, new_top_inx);
		while (iter3.MoveUp() == TRUE)
		{
			int item_hgt = iter3.CurrItemHeight();
			if (visible_hgt+item_hgt > m_curr_list_area_height)
				break;

			visible_hgt += item_hgt;
			new_top_inx--;
		}

		// Navigate to the discovered index by scrolling the list down.
		SetTopIndex(new_top_inx);
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
int TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetItemFromPoint(int offs_y, ListBoxItemType **item_ptr, bool *inside_item_rect, bool *item_fully_visib)
{
	*item_ptr = NULL;
	if (inside_item_rect != NULL)
		*inside_item_rect = FALSE;
	if (item_fully_visib != NULL)
		*item_fully_visib = FALSE;

	if (m_num_items == 0 || m_curr_list_area_height <= 0)
	{
		// The list is empty or there is no list area.
		return(-1);
	}

	if (offs_y < 0 || offs_y >= m_curr_list_area_height)
	{
		// Passed offset stays outside of the list area.
		return(-1);
	}

	int pos_y = 0;
	TListBoxFrgrItemsListIter iter(this);
	do
	{
		pos_y += iter.CurrItemHeight();
		if (offs_y < pos_y)
		{
			// Passed point belongs to the area of the current item.
			if (inside_item_rect != NULL)
				*inside_item_rect = TRUE;
			if (offs_y <= m_curr_list_area_height && item_fully_visib != NULL)
				*item_fully_visib = TRUE;

			// Return the item and its index.
			*item_ptr = iter.CurrItem();
			return(iter.CurrPos());
		}
	}
	while (iter.StepIterDown() == TRUE);

	// Passed point belongs to the free space below all items.
	assert(pos_y == m_visible_items_height);
	if (m_visible_items_height <= m_curr_list_area_height && item_fully_visib != NULL)
		*item_fully_visib = TRUE;
	*item_ptr = iter.CurrItem();
	return(iter.CurrPos());
}

// ---------------------------- Protected methods -----------------------------------

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SetupListAreaHeight(int new_height)
{
	if (new_height < 0)
		new_height = 0;

	// Build the state vars.
	bool need_scroller = FALSE;

	if (new_height <= 0)
	{
		// There is no way to show anything in the window.
		m_upper_item = NULL;
		m_upper_item_inx = 0;
		m_num_visible_items = m_visible_items_height = 0;
		m_curr_list_area_height = 0;

		DisposeVertScroller();

		if (m_items_table != NULL)
			DisposeLocalItems();
	}
	else
	{
		// Use a simple approach with plain rebuiling the state vars.
		m_curr_list_area_height = new_height;
		TListBoxScrollState scroll_state(this);
		BuildListStateVars(m_upper_item_inx, TRUE, scroll_state);
		UpdateScroller();
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::UpdateScroller()
{
	//
	// When this method is called, the m_curr_list_area_height should contain correct and POSITIVE height
	// of the list area plus ALL list state vars should be are already set. This is needed because scroller shows
	// correlation between these values. If they are not set, the scroller cannot be updated.
	//

	// Figure out if the scroller is needed or not.
	bool bNeedScroller = FALSE;
	if (m_props.vert_scroll_always_visib == TRUE)
	{
		bNeedScroller = TRUE;
	}
	else if (m_num_visible_items < m_num_items || m_visible_items_height > m_curr_list_area_height)
	{
		bNeedScroller = TRUE;
	}

	if (bNeedScroller == TRUE)
	{
		int num_fully_visib = m_num_visible_items;
		if (m_visible_items_height > m_curr_list_area_height)
			num_fully_visib--;
		if (num_fully_visib == 0 && m_num_items > 0)
			num_fully_visib = 1;

		if (num_fully_visib < m_num_items)
		{
			// Enable the scroller and set up all its props.
			SetupVertScroller(m_num_items-1, num_fully_visib, m_upper_item_inx);
		}
		else
		{
			// Disable the sroll bar.
			SetupVertScroller(0, 0, 0);
		}
	}
	else
	{
		// Get rid of the scroller.
		DisposeVertScroller();
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::CheckInvisibleChangesAbove()
{
	int scroll_items_cnt = 0;
	int scroll_pix_value = 0;
	ListBoxItemType *new_upper_item = NULL;
	if (m_visible_items_height < m_curr_list_area_height)
	{
		// Check if the existing list needs to be scrolled down or not.
		TListBoxLocalItemsListIter iter1(this, m_upper_item);
		for (--iter1; iter1; --iter1)
		{
			int item_hgt = iter1.CurrItemHeight();
			if (m_visible_items_height+scroll_pix_value+item_hgt > m_curr_list_area_height)
				break;

			// Reveal this item.
			scroll_items_cnt++;
			scroll_pix_value += item_hgt;
			new_upper_item = iter1.CurrItem();
		}
	}

	if (scroll_items_cnt > 0)
	{
		// Scroll the existing non empty set of visible items down.
		m_upper_item = new_upper_item;
		m_upper_item_inx = m_upper_item->ListBoxIndex();
		m_num_visible_items += scroll_items_cnt;
		m_visible_items_height += scroll_pix_value;
		ScrollListArea(0, m_visible_items_height, scroll_pix_value);
	}
	else
	{
		// The upper item has not changed, while its index did changed.
		m_upper_item_inx = m_upper_item->ListBoxIndex();
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
bool TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::ProcessSlctChangeClick(int offs_y, bool mk_shift, bool mk_control, TListBoxSlctChangeInfoInternal &info)
{
	if (m_num_items <= 0)
	{
		// Selection is not possible on an empty list.
		assert(m_slct_item_inx == -1);
		assert(m_slct_num_slct == 0);
		return(FALSE);
	}
	else if (m_curr_list_area_height <= 0)
	{
		// The list area cannot be clicked.
		assert(m_upper_item_inx == 0);
		return(FALSE);
	}

	assert(m_num_visible_items > 0);
	ListBoxItemType *clicked_item;
	bool inside_item_rect, item_fully_visib;
	int clk_inx = GetItemFromPoint(offs_y, &clicked_item, &inside_item_rect, &item_fully_visib);
	if (clk_inx < 0)
	{
		// User clicked place outside of the list area.
		return(FALSE);
	}

	if (inside_item_rect == FALSE && m_props.multi_select == FALSE)
	{
		// User clicked the place outside the item rect. Kill the selection if any.
		RemoveSingleSelection();
		return(FALSE);
	}

	if (item_fully_visib == FALSE)
	{
		// Scroll this item into the view.
		ScrollItemIntoTheView(clk_inx, FALSE);
	}

	// Prefill notification structure with the most likely to happen values.
	info.clicked_item = clicked_item;
	info.clicked_item_inx = info.slct_chg_beg = clk_inx;
	info.slct_chg_len = 1;

	if (m_props.multi_select == FALSE)
	{
		// This is a single selection case. The keyboard modifiers are not important.
		if (clk_inx == m_slct_item_inx)
		{
			// 1. This is a click on the same item. Do not do anything.
			return(FALSE);
		}
		else
		{
			// 2. Move selection to the different item.
			SetupSingleSelection(clicked_item, clk_inx);
		}
	}
	else
	{
		//
		//  Multi selection case.
		//

		if (m_slct_num_slct == 1 && m_slct_item_beg_inx == clk_inx)
		{
			// User has clicked again on the only one selected item.
			if (mk_control == FALSE || mk_shift == TRUE)
			{
				// 3. This is a click or shift+click or shift+ctrl+click.
				// Keep existing selection as it is now.
				return(FALSE);
			}
			else
			{
				// 4. This is a ctrl+click.
				// The selection should disappear completely.
				RemoveSelection();
			}
		}
		else
		{
			// Big set of different cases.
			bool clicked_item_slct = GetItemSelection(clicked_item, clk_inx);
			int slct_area_len = m_slct_item_end_inx-m_slct_item_beg_inx+1;
			if (m_slct_num_slct == 0 || (mk_control == FALSE && mk_shift == FALSE))
			{
				// 5. Switch to new one item selection.
				RemoveSelection();
				ModifyMultipleSelection(clicked_item, clk_inx, 1, TRUE);
			}
			else if (m_slct_num_slct == slct_area_len)
			{
				// Selection contains only one contiguous range of items.
				if (mk_shift == FALSE)
				{
					// This is a ctrl+click:
					if (m_items_table == NULL || m_items_table->IsMultiSelSupported() == TRUE)
					{
						// 6. The listbox allows creation of patterns.
						// Toggle selection of one item only.
						info.new_selection = FALSE;
						ModifyMultipleSelection(clicked_item, clk_inx, 1, !clicked_item_slct);
					}
					else
					{
						if (clk_inx == m_slct_item_beg_inx-1 || clk_inx == m_slct_item_beg_inx+slct_area_len)
						{
							// 7. Enlarge the area for one item.
							assert(clicked_item_slct == FALSE);
							info.new_selection = FALSE;
							ModifyMultipleSelection(clicked_item, clk_inx, 1, TRUE);
						}
						else if (clk_inx == m_slct_item_beg_inx || clk_inx == m_slct_item_beg_inx+slct_area_len-1)
						{
							// 8. Reduce the area for one line.
							assert(clicked_item_slct == TRUE);
							info.new_selection = FALSE;
							ModifyMultipleSelection(clicked_item, clk_inx, 1, FALSE);
						}
						else
						{
							// 9. Switch to the new one line selection.
							RemoveSelection();
							ModifyMultipleSelection(clicked_item, clk_inx, 1, TRUE);
						}
					}
				}
				else
				{
					// This is a shift+click:
					info.new_selection = FALSE;
					if (clk_inx < m_slct_item_beg_inx)
					{
						// 10. Add more selected items on top.
						ModifyMultipleSelection(clicked_item, clk_inx, m_slct_item_beg_inx-clk_inx, TRUE);
						info.slct_chg_beg = clk_inx;
						info.slct_chg_len = m_slct_item_beg_inx-clk_inx;
					}
					else if (clk_inx < m_slct_item_beg_inx+slct_area_len)
					{
						if (clk_inx == m_slct_item_beg_inx+slct_area_len-1)
						{
							// 11.  This is a special case when the last item of the range was clicked.
							return(FALSE);
						}

						// 12. Adjust length of the selected range.
						int len_to_adj = (m_slct_item_beg_inx+slct_area_len)-(clk_inx+1);
						assert(len_to_adj > 0);
						ModifyMultipleSelection(NULL, clk_inx+1, len_to_adj, FALSE);
						info.slct_chg_beg = clk_inx+1;
						info.slct_chg_len = len_to_adj;
					}
					else
					{
						// 13. Add more selected items at the bottom.
						int len_to_adj = (clk_inx+1)-(m_slct_item_beg_inx+slct_area_len);
						ModifyMultipleSelection(NULL, m_slct_item_beg_inx+slct_area_len, len_to_adj, TRUE);
						info.slct_chg_beg = m_slct_item_beg_inx+slct_area_len;
						info.slct_chg_len = len_to_adj;
					}
				}
			}
			else
			{
				// Selection is a pattern of non contiguous ranges. Verify if the current props allow to have patterns.
				assert(m_items_table == NULL || m_items_table->IsMultiSelSupported() == TRUE);
				int range_beg, range_len;
				if (mk_shift == FALSE)
				{
					//  This is a ctrl+click.
					// 14. Toggle selection state for one item only.
					info.new_selection = FALSE;
					ModifyMultipleSelection(clicked_item, clk_inx, 1, !clicked_item_slct);
				}
				else if (clicked_item_slct == FALSE)
				{
					if (clk_inx < m_slct_item_beg_inx)
					{
						// 15. Pick up first selection range.
						// Cancel the old pattern and enlarge the upper selection.
						GetNextSelectedRange(clk_inx, range_beg, range_len);
						RemoveSelection();
						int new_range_len = range_beg+range_len-clk_inx;
						ModifyMultipleSelection(clicked_item, clk_inx, new_range_len, TRUE);
						info.slct_chg_beg = clk_inx;
						info.slct_chg_len = new_range_len;
					}
					else if (clk_inx > m_slct_item_end_inx)
					{
						// 16. Pick up last selection range.
						// Cancel the old pattern and enlarge the upper selection.
						GetNextSelectedRange(clk_inx, range_beg, range_len, FALSE);
						RemoveSelection();
						int new_range_len = clk_inx-range_beg+1;
						ModifyMultipleSelection(NULL, range_beg, new_range_len, TRUE);
						info.slct_chg_beg = range_beg;
						info.slct_chg_len = new_range_len;
					}
					else
					{
						// 17. Click is in between of selected areas.
						// Switch to new selection that contains just one item.
						RemoveSelection();
						ModifyMultipleSelection(clicked_item, clk_inx, 1, TRUE);
					}
				}
				else
				{
					// 18. Pick up info about the current range.
					// Switch to the pattern that contains the modified current range.
					GetNextSelectedRange(clk_inx, range_beg, range_len, FALSE);
					RemoveSelection();
					int new_range_len = clk_inx-range_beg+1;
					ModifyMultipleSelection(NULL, range_beg, new_range_len, TRUE);
					info.slct_chg_beg = range_beg;
					info.slct_chg_len = new_range_len;
				}
			}
		}
	}

	return(TRUE);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::RemoveSingleSelection()
{
	assert(m_props.multi_select == FALSE);
	if (m_slct_item_inx < 0 || m_num_items <= 0)
	{
		m_slct_item = NULL;
		return;
	}

	assert(m_slct_item_inx < m_num_items);
	int old_slct_inx = m_slct_item_inx;

	if (m_items_table == NULL)
	{
		// In the local list mode the selected item should be simply known.
		assert(m_slct_item != NULL);
		m_slct_item->SetSelection(FALSE);
	}
	else
	{
		// Try to find this item in the frgr painting list. In any case table knows hothing about selection here.
		ListBoxItemType *old_slct_item = FindFrgrItem(old_slct_inx);
		if (old_slct_item != NULL)
			old_slct_item->SetSelection(FALSE);
	}

	m_slct_item = NULL;
	m_slct_item_inx = -1;
	if (IsItemIndexVisible(old_slct_inx) == TRUE)
		InvalidateItemByIndex(old_slct_inx);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::RemoveMultipleSelection()
{
	assert(m_props.multi_select == TRUE);
	if (m_slct_num_slct <= 0 || m_num_items <= 0)
		return;

	assert(m_slct_item_beg_inx >= 0 && m_slct_item_end_inx >= m_slct_item_beg_inx);
	if (m_items_table == NULL)
	{
		// Iterate items in the local list and set/reset slct flags there.
		TListBoxLocalItemsListIter iter(this, LocalListSeek(m_slct_item_beg_inx, TRUE));
		for (; iter && iter.CurrPos() <= m_slct_item_end_inx; ++iter)
		{
			if (iter.CurrItem()->IsSelected() == TRUE)
				iter.CurrItem()->SetSelection(FALSE);
		}
	}
	else
	{
		// Update selection flags in the frgr list items.
		if (m_num_visible_items > 0)
		{
			// Loop will modify something or it will stop at the very beginning.
			TListBoxFrgrItemsListIter iter(this);
			do
			{
				if (iter.CurrItem() != NULL && iter.CurrItem()->IsSelected() == TRUE)
					iter.CurrItem()->SetSelection(FALSE);
			}
			while (iter.StepIterDown() == TRUE);
		}

		// Update selection state in the table.
		if (m_items_table->IsMultiSelSupported() == TRUE)
		{
			m_items_table->ResetSelectionMarks();
		}
	}

	// Invalidate this range or items on the screen.
	InvalidateItemByIndex(m_slct_item_beg_inx, m_slct_item_end_inx-m_slct_item_beg_inx+1);
	m_slct_item_beg_inx = m_slct_item_end_inx = -1;
	m_slct_num_slct = 0;
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SetupSingleSelection(ListBoxItemType *item, int item_inx)
{
	if (item_inx == m_slct_item_inx)
	{
		// There is no need to do anything.
		return;
	}

	if (m_slct_item_inx >= 0)
		RemoveSingleSelection();

	// Update the flag in the item.
	if (item != NULL)
	{
		// Put mark on the known item.
		assert(item->ListBoxIndex() == item_inx);
		item->SetSelection(TRUE);
	}
	else if (m_items_table == NULL)
	{
		// Find item in the local items list.
		item = LocalListSeek(item_inx, TRUE);
		assert(item != NULL);
		item->SetSelection(TRUE);
	}
	else
	{
		// Look for the item in the frgr list.
		item = FindFrgrItem(item_inx);
		if (item != NULL)
			item->SetSelection(TRUE);
	}

	// Update slct vars and invalidate the screen.
	m_slct_item = (m_items_table != NULL) ? NULL : item;
	m_slct_item_inx = item_inx;
	if (IsItemIndexVisible(item_inx) == TRUE)
		InvalidateItemByIndex(item_inx);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::ModifyMultipleSelection(ListBoxItemType *beg_item_ptr, int beg_item_inx, int cnt_items, bool new_slct_stt)
{
	assert(cnt_items > 0);
	assert(beg_item_ptr == NULL || beg_item_ptr->ListBoxIndex() == beg_item_inx);
	int last_item_inx = beg_item_inx+cnt_items-1;

	if (m_slct_num_slct <= 0)
	{
		// There is no selection in the moment.
		if (new_slct_stt == FALSE)
			return;

		// Start the new selection from scratch.
		SetupItemsRangeSelection(beg_item_ptr, beg_item_inx, cnt_items, TRUE);
	}
	else
	{
		// Some selection is already present.
		if (new_slct_stt == FALSE && beg_item_inx <= m_slct_item_beg_inx && last_item_inx >= m_slct_item_end_inx)
		{
			// The existing selection goes away completely.
			RemoveMultipleSelection();
			return;
		}

		int slct_area_len = m_slct_item_end_inx-m_slct_item_beg_inx+1;
		if (slct_area_len == m_slct_num_slct && m_items_table != NULL && m_items_table->IsMultiSelSupported() == FALSE)
		{
			// The existing selection is pure range of items and is should be changed only to some other single contiguous range of items.
			if (new_slct_stt == TRUE && (last_item_inx < m_slct_item_beg_inx-1 || beg_item_inx > m_slct_item_end_inx+1))
			{
				// Kill the old selection and setup new pure range.
				RemoveMultipleSelection();
				SetupItemsRangeSelection(beg_item_ptr, beg_item_inx, cnt_items, TRUE);
			}
			else if (new_slct_stt == FALSE && beg_item_inx > m_slct_item_beg_inx && last_item_inx < m_slct_item_end_inx)
			{
				// This is an attempt to make a hole inside the contiguous selection. Keep the upper part of the selection and get rid of its lower part.
				SetupItemsRangeSelection(beg_item_ptr, beg_item_inx, m_slct_item_end_inx+1-beg_item_inx, FALSE);
			}
			else
			{
				// New range is not breaking the old selected range, it maybe just extendings/reducing it.
				if (beg_item_inx < m_slct_item_beg_inx)
				{
					if (new_slct_stt == TRUE)
					{
						SetupItemsRangeSelection(beg_item_ptr, beg_item_inx, m_slct_item_beg_inx-beg_item_inx, TRUE);
					}
					else if (last_item_inx >= m_slct_item_beg_inx)
					{
						SetupItemsRangeSelection(NULL, m_slct_item_beg_inx, last_item_inx-m_slct_item_beg_inx+1, FALSE);
					}
				}

				if (last_item_inx > m_slct_item_end_inx)
				{
					if (new_slct_stt == TRUE)
					{
						SetupItemsRangeSelection(NULL, m_slct_item_end_inx+1, last_item_inx-m_slct_item_end_inx, TRUE);
					}
					else if (beg_item_inx <= m_slct_item_end_inx)
					{
						SetupItemsRangeSelection(beg_item_ptr, beg_item_inx, m_slct_item_end_inx+1-beg_item_inx, FALSE);
					}
				}
			}
		}
		else
		{
			// The existing selection either is not a pure range of items or it is ok to change it to a pattern of contiguous ranges.
			SetupItemsRangeSelection(beg_item_ptr, beg_item_inx, cnt_items, new_slct_stt);
		}
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SetupItemsRangeSelection(ListBoxItemType *beg_item, int beg_item_inx, int cnt_items, bool new_slct_stt)
{
	// Method expects that the passed range of indexes stays inside the range of available items.
	assert(m_num_items > 0 && cnt_items > 0);
	int last_item_inx = beg_item_inx+cnt_items-1;
	int old_num_slct = m_slct_num_slct;

	if (m_items_table == NULL)
	{
		//
		// Local list case.
		//

		// Fix the beginning item if it was not passed.
		if (beg_item == NULL)
			beg_item = LocalListSeek(beg_item_inx, TRUE);
		assert(beg_item != NULL && beg_item->ListBoxIndex() == beg_item_inx);

		// Iterate items in the local list and set/reset slct flags there.
		TListBoxLocalItemsListIter iter(this, beg_item);
		for (; iter && iter.CurrPos() <= last_item_inx; ++iter)
		{
			if (iter.CurrItem()->IsSelected() != new_slct_stt)
			{
				iter.CurrItem()->SetSelection(new_slct_stt);
				if (new_slct_stt == TRUE)
					m_slct_num_slct++;
				else m_slct_num_slct--;
			}
		}

		// Update the rest of selection state flags.
		if (new_slct_stt == TRUE)
		{
			// Setting more selection is rather simple.
			if (old_num_slct <= 0)
			{
				m_slct_item_beg_inx = beg_item_inx;
				m_slct_item_end_inx = last_item_inx;
			}
			else
			{
				if (beg_item_inx < m_slct_item_beg_inx)
					m_slct_item_beg_inx = beg_item_inx;
				if (last_item_inx > m_slct_item_end_inx)
					m_slct_item_end_inx = last_item_inx;
			}
		}
		else
		{
			// Look for the new side items using loops. Situation when selection is removed completely
			// is processed by the upper layer using different code.
			if (beg_item_inx <= m_slct_item_beg_inx)
			{
				// Iterator stays after the last modified item.
				m_slct_item_beg_inx = -1;
				for (; iter; ++iter)
				{
					if (iter.CurrItem()->IsSelected() == TRUE)
					{
						m_slct_item_beg_inx = iter.CurrPos();
						break;
					}
				}

				assert(m_slct_item_beg_inx != -1);
			}
			if (last_item_inx >= m_slct_item_end_inx)
			{
				// Iterator should be seeked back to the beg of the range.
				m_slct_item_end_inx = -1;
				for (iter.Seek(beg_item), --iter; iter; --iter)
				{
					if (iter.CurrItem()->IsSelected() == TRUE)
					{
						m_slct_item_end_inx = iter.CurrPos();
						break;
					}
				}

				assert(m_slct_item_end_inx != -1);
			}
		}
	}
	else
	{
		//
		// External table with listbox items.
		//

		// Update the selection flags in the frgr items list.
		if (m_num_visible_items > 0 && beg_item_inx < m_upper_item_inx+m_num_visible_items)
		{
			// Loop will modify something or it will stop right at the beginning.
			TListBoxFrgrItemsListIter iter(this);
			while (iter.CurrPos() <= last_item_inx)
			{
				if (iter.CurrPos() >= beg_item_inx && iter.CurrItem() != NULL)
				{
					if (iter.CurrItem()->IsSelected() != new_slct_stt)
					{
						iter.CurrItem()->SetSelection(new_slct_stt);
					}
				}

				if (iter.StepIterDown() == FALSE)
					break;
			}
		}

		// Update the selection summary vars.
		if (m_items_table->IsMultiSelSupported() == TRUE)
		{
			// Inform the table about these changes and get the summary back.
			m_items_table->SelectItemsRange(beg_item_inx, cnt_items, new_slct_stt);
			m_slct_num_slct = m_items_table->GetSelectionSummary(m_slct_item_beg_inx, m_slct_item_end_inx);
		}
		else
		{
			// Current table supports only one range of selected items. This integrity should be verified by the upper layer.
			if (new_slct_stt == TRUE)
			{
				if (old_num_slct <= 0)
				{
					m_slct_item_beg_inx = beg_item_inx;
					m_slct_item_end_inx = last_item_inx;
					m_slct_num_slct = cnt_items;
				}
				else
				{
					if (beg_item_inx < m_slct_item_beg_inx)
					{
						// Shift the top side of the range up.
						m_slct_num_slct += m_slct_item_beg_inx-beg_item_inx;
						m_slct_item_beg_inx = beg_item_inx;
					}

					if (last_item_inx > m_slct_item_end_inx)
					{
						// Shift the bottom side of the range down.
						m_slct_num_slct += last_item_inx-m_slct_item_end_inx;
						m_slct_item_end_inx = last_item_inx;
					}
				}
			}
			else
			{
				if (beg_item_inx <= m_slct_item_beg_inx)
				{
					// Use the bottom end of the passed range as new selection top.
					m_slct_num_slct -= last_item_inx+1-m_slct_item_beg_inx;
					m_slct_item_beg_inx = last_item_inx+1;
				}

				if (last_item_inx >= m_slct_item_end_inx)
				{
					// Use the top end of the passed range as new selection bottom.
					m_slct_num_slct -= m_slct_item_end_inx+1-beg_item_inx;
					m_slct_item_end_inx = beg_item_inx-1;
				}
			}

			// Reverify the single range integrity with assert.
			int slct_area_len = m_slct_item_end_inx-m_slct_item_beg_inx+1;
			assert(slct_area_len == m_slct_num_slct);
		}
	}

	// Invalidate this range of items on the screen.
	InvalidateItemByIndex(beg_item_inx, cnt_items);

	// After applying this method the selection should be non empty.
	assert(m_slct_num_slct > 0);
}

template <class ListBoxItemType, class ListBoxObjectType>
bool TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::GetItemSelection(ListBoxItemType *item, int item_inx)
{
	if (item != NULL)
	{
		return(item->IsSelected());
	}
	else if (item_inx < 0 || item_inx >= m_num_items)
	{
		// Items that stay outside of the list are treated as not selected.
		return(FALSE);
	}

	if (m_items_table == NULL)
	{
		if (m_slct_num_slct < 0 || item_inx < m_slct_item_beg_inx || item_inx > m_slct_item_end_inx)
			return(FALSE);

		item = LocalListSeek(item_inx, TRUE);
		return(item->IsSelected());
	}
	else if (m_items_table->IsMultiSelSupported() == TRUE)
	{
		return(m_items_table->GetSlctFlag(item_inx));
	}

	// This is multi selection with the table that cannot keep the multiselection state.
	// Selection can only be a single consecutive range.
	return(m_slct_num_slct > 0 && item_inx >= m_slct_item_beg_inx && item_inx <= m_slct_item_end_inx);
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::AddToMultiSelectionSumry(int item_inx)
{
	// This should be the multi selection listbox.
	assert(m_props.multi_select == TRUE);

	if (m_slct_num_slct == 0)
	{
		m_slct_item_beg_inx = m_slct_item_end_inx = item_inx;
		m_slct_num_slct = 1;
	}
	else
	{
		if (item_inx < m_slct_item_beg_inx)
			m_slct_item_beg_inx = item_inx;
		if (item_inx > m_slct_item_end_inx)
			m_slct_item_end_inx = item_inx;
		m_slct_num_slct++;
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::SetupListItemFromListBoxTable(ListBoxItemType *item)
{
	item->SetSelection(FALSE);
	item->SetCachedHeight(-1);

	if (m_props.multi_select == FALSE)
	{
		if (item->ListBoxIndex() == m_slct_item_inx)
			item->SetSelection(TRUE);
	}
	else if (m_slct_num_slct > 0 && m_items_table->IsMultiSelSupported() == FALSE)
	{
		if (item->ListBoxIndex() >= m_slct_item_beg_inx && item->ListBoxIndex() <= m_slct_item_end_inx)
			item->SetSelection(TRUE);
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::BuildItemIndexesAndSelectionVars()
{
	if (m_items_table != NULL)
	{
		// Listbox table case.
		DisposeLocalItems();
		m_num_items = m_items_table->NumItems();

		// Pick up the current selection state from the table. Default implementation should work fine
		// with tables that do not support selection.
		m_slct_item = NULL;
		m_slct_item_inx = -1;
		m_slct_num_slct = m_items_table->GetSelectionSummary(m_slct_item_beg_inx, m_slct_item_end_inx);
		if (m_slct_num_slct == 1)
		{
			assert(m_slct_item_beg_inx == m_slct_item_end_inx);
			m_slct_item_inx = m_slct_item_beg_inx;
		}
	}
	else
	{
		// Local items list case.
		m_slct_item = NULL;
		m_slct_item_inx = m_slct_item_beg_inx = m_slct_item_end_inx = -1;
		m_slct_num_slct = 0;

		// Iterate the whole list and reassign the listbox index values. At the same time reset
		// the knowledge on the items height and figure out the state of selection in the list.
		m_num_items = 0;
		for (TListIter<ListBoxItemType> iter(m_items_list); iter; ++iter)
		{
			ListBoxItemType &item = iter.CurrItem();

			// Assign the index and force repicking the item height.
			item.SetListBoxIndex(m_num_items++);
			item.SetCachedHeight(-1);

			if (item.IsSelected() == TRUE)
			{
				if (m_props.multi_select == TRUE)
				{
					// This is a multi selection listbox.
					AddToMultiSelectionSumry(item.ListBoxIndex());
				}
				else
				{
					// This is a single selection listbox.
					if (m_slct_item == NULL)
					{
						// The selected item is found.
						m_slct_item = &item;
						m_slct_item_inx = item.ListBoxIndex();
					}
					else
					{
						// Listbox has more than one selected item. Reset the selection mark on the current item.
						item.SetSelection(FALSE);
					}
				}
			}
		}
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::BuildListStateVars(int new_upper_item_inx, bool can_use_upper_item_ptr, TListBoxScrollState &prev_scroll_state)
{
	//
	//  This procedure expects that after calling this method the whole list area will be invalidated and repainted.
	//

	if (m_num_items <= 0 || m_curr_list_area_height <= 0)
	{
		// The list is empty or there is no place to display it.
		m_upper_item = NULL;
		m_upper_item_inx = 0;
		m_num_visible_items = m_visible_items_height = 0;
		return;
	}

	// Attempt 1. Top or middle positioned list.
	bool bottom_align = prev_scroll_state.IsBottomAligned();
	if (new_upper_item_inx >= 0 || bottom_align == FALSE)
	{
		// Reset the flag even if list was bottom aligned before.
		bottom_align = FALSE;

		// Figure out new reasonable value for upper item index.
		if (new_upper_item_inx < 0)
		{
			// Use old scroll ratio that was present in the list before.
			new_upper_item_inx = prev_scroll_state.CalcUpperInx(m_num_items);
		}

		// Fill the list area with items starting from the new m_upper_item_inx.
		if (m_items_table != NULL)
		{
			// The ListBox table case. Seek() to the recommended position. Design of the TListBoxTable class
			// does not allow the Seek() operation to fail.
			m_items_table->Seek(new_upper_item_inx);
			m_upper_item_inx = new_upper_item_inx;

			// Get rid of the old list. Build visible items from scratch.
			DisposeLocalItems();

			m_upper_item = NULL;
			m_num_visible_items = m_visible_items_height = 0;
			for (int inx=m_upper_item_inx; inx<m_num_items; ++inx)
			{
				ListBoxItemType *item = m_items_table->ReadItem(TRUE);
				if (inx == m_upper_item_inx)
					m_upper_item = item;

				if (item != NULL)
				{
					// Ensure the correct value of the listbox index.
					item->SetListBoxIndex(inx);
					SetupListItemFromListBoxTable(item);
					m_visible_items_height += item->GetItemHeight((ListBoxObjectType*)this);
					m_items_list.AppendItem(item);
				}
				else
				{
					// This situation is not nice but still may happen.
					m_visible_items_height += TLB_DEFAULT_ITEM_HEIGHT;
				}

				m_num_visible_items++;
				if (m_visible_items_height >= m_curr_list_area_height)
					break;
			}
		}
		else
		{
			// It is necessary to find item with the requested index in the local list.
			m_upper_item = LocalListSeek(new_upper_item_inx, can_use_upper_item_ptr);
			assert(m_upper_item != NULL);
			m_upper_item_inx = new_upper_item_inx;

			m_num_visible_items = m_visible_items_height = 0;
			for (TListBoxLocalItemsListIter iter(this, m_upper_item); iter; ++iter)
			{
				m_visible_items_height += iter.CurrItemHeight();
				m_num_visible_items++;
				if (m_visible_items_height >= m_curr_list_area_height)
					break;
			}
		}

		// It may happen that recommened/calculated scroll pos index was so big that listbox area
		// was not fully covered with listbox items. Check for this situation.
		if (m_upper_item_inx > 0 && m_upper_item_inx+m_num_visible_items >= m_num_items && m_visible_items_height < m_curr_list_area_height)
		{
			// Check if item that is above the current upper item can be scrolled into the view
			// without making the bottom item partially invisible.
			int item_above_hgt = 0;
			if (m_items_table != NULL)
			{
				m_items_table->Seek(m_upper_item_inx);
				ListBoxItemType *item = m_items_table->ReadItem(FALSE);
				item_above_hgt = TLB_DEFAULT_ITEM_HEIGHT;
				if (item != NULL)
				{
					item->SetListBoxIndex(m_upper_item_inx-1);
					SetupListItemFromListBoxTable(item);
					item_above_hgt = item->GetItemHeight((ListBoxObjectType*)this);
					m_items_table->RecycleItem(item);
				}
			}
			else
			{
				TListBoxLocalItemsListIter iter(this, m_upper_item, TRUE);
				item_above_hgt = (iter == TRUE) ? iter.CurrItem()->GetItemHeight((ListBoxObjectType*)this) : 0;
			}

			if (m_visible_items_height+item_above_hgt <= m_curr_list_area_height)
			{
				// This item can and should be scrolled in.
				bottom_align = TRUE;
			}
		}
	}

	// Attempt 2. Bottom aligned list.
	if (bottom_align == TRUE)
	{
		// This is a special procedure that starts from the bottom of the list. Find out how many listbox items are needed
		// to cover all availaible client space. Funny aspect of this block is that if there are no enough items to cover
		// the window completely, the loop will end and m_upper_item_inx will be zero. The variables will be in the top
		// aligned list state.
		m_upper_item = NULL;
		m_upper_item_inx = -1;
		m_num_visible_items = m_visible_items_height = 0;
		if (m_items_table != NULL)
		{
			// The ListBox table case. Visible items are built from scratch.
			DisposeLocalItems();

			// Seek to the end of the table.
			m_items_table->Seek(m_num_items);
			for (int inx=m_num_items-1; inx >= 0; --inx)
			{
				ListBoxItemType *item = m_items_table->ReadItem(FALSE);
				int item_hgt = TLB_DEFAULT_ITEM_HEIGHT;
				if (item != NULL)
				{
					item->SetListBoxIndex(inx);
					SetupListItemFromListBoxTable(item);
					item_hgt = item->GetItemHeight((ListBoxObjectType*)this);
				}

				if (m_visible_items_height+item_hgt > m_curr_list_area_height)
				{
					if (item != NULL)
						m_items_table->RecycleItem(item);
					break;
				}

				// Accept the current item as fully visible.
				m_visible_items_height += item_hgt;
				m_num_visible_items++;
				m_upper_item = item;
				m_upper_item_inx = inx;

				if (item != NULL)
					m_items_list.PrependItem(item);
			}
		}
		else
		{
			// Local list of items.
			TListIter<ListBoxItemType> iter(m_items_list);
			for (iter.SeekToLast(); iter; --iter)
			{
				ListBoxItemType &item = iter.CurrItem();

				int item_hgt = item.GetItemHeight((ListBoxObjectType*)this);
				if (m_visible_items_height+item_hgt > m_curr_list_area_height)
					break;

				// Accept the current item as fully visible.
				m_visible_items_height += item_hgt;
				m_num_visible_items++;
				m_upper_item = &item;
				m_upper_item_inx = item.ListBoxIndex();
			}
		}
	}
}

template <class ListBoxItemType, class ListBoxObjectType>
ListBoxItemType *TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::LocalListSeek(int item_inx, bool can_use_upper_item_ptr)
{
	// The list is expected to have correct indexes without gaps.
	assert(m_items_table == NULL);
	if (item_inx < 0)
		item_inx = 0;
	if (item_inx >= m_num_items)
		item_inx = m_num_items-1;

	// Check for trivial cases first.
	if (m_num_items == 0)
		return(NULL);

	if (can_use_upper_item_ptr && item_inx == m_upper_item_inx && m_upper_item != NULL)
	{
		assert(m_upper_item->ListBoxIndex() == item_inx);
		return(m_upper_item);
	}

	// Detect the best starting point and the direction of iteration.
	ListBoxItemType *item;
	bool search_forward;
	if (m_upper_item == NULL || can_use_upper_item_ptr == FALSE)
	{
		// Item should be searched either from the beginning or or from the end of the whole list.
		if (item_inx < m_num_items-item_inx)
		{
			item = (ListBoxItemType*)m_items_list.GetFirst();
			search_forward = TRUE;
		}
		else
		{
			item = (ListBoxItemType*)m_items_list.GetLast();
			search_forward = FALSE;
		}
	}
	else
	{
		// The situation is more complex. It is necessary to select from the four possible cases.
		assert(m_upper_item_inx >= 0 && m_upper_item_inx < m_num_items);
		if (item_inx < m_upper_item_inx)
		{
			// The item to pick is in the lower part of the list.
			if (m_upper_item_inx-item_inx < item_inx)
			{
				item = m_upper_item;
				search_forward = FALSE;
			}
			else
			{
				item = (ListBoxItemType*)m_items_list.GetFirst();
				search_forward = TRUE;
			}
		}
		else
		{
			// The item to pick is in the upper part of the list.
			if (item_inx-m_upper_item_inx < m_num_items-item_inx)
			{
				item = m_upper_item;
				search_forward = TRUE;
			}
			else
			{
				item = (ListBoxItemType*)m_items_list.GetLast();
				search_forward = FALSE;
			}
		}
	}

	// Do the search itself with a simple loop.
	TListIter<ListBoxItemType> iter(m_items_list);
	if (search_forward == TRUE)
	{
		for (iter.SeekToItem(item); iter; ++iter)
		{
			item = &iter.CurrItem();
			if (item->ListBoxIndex() >= item_inx)
				break;
		}
	}
	else
	{
		for (iter.SeekToItem(item); iter; --iter)
		{
			item = &iter.CurrItem();
			if (item->ListBoxIndex() <= item_inx)
				break;
		}
	}

	// Check that the search has found the requested item.
	assert(item != NULL && item->ListBoxIndex() == item_inx);
	return(item);
}

template <class ListBoxItemType, class ListBoxObjectType>
ListBoxItemType *TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::FindFrgrItem(int item_inx)
{
	if (IsItemIndexVisible(item_inx) == FALSE)
		return(NULL);

	TListBoxFrgrItemsListIter iter(this);
	while (iter.CurrPos() < item_inx)
		iter.StepIterDown();

	return(iter.CurrItem());
}

template <class ListBoxItemType, class ListBoxObjectType>
void TGenericListBoxObject<ListBoxItemType, ListBoxObjectType>::DisposeLocalItems()
{
	while (m_items_list.IsEmpty() == FALSE)
	{
		ListBoxItemType *item = (ListBoxItemType*)m_items_list.GetFirst();
		m_items_list.RemoveItem(item);
		if (m_items_table != NULL)
		{
			// Return item to the list box table.
			m_items_table->RecycleItem(item);
		}
		else
		{
			if (item->IsOwned() == TRUE)
			{
				// The item is owned by the listbox.
				delete item;
			}
		}
	}

	if (m_items_table == NULL)
		m_num_items = 0;
}


