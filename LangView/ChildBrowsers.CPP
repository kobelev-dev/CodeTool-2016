//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Common/FormatNumber.H"
#include  "LangView/ChildBrowsers.H"
#include  "LangView/LangViewResources.H"

// ----------------------------------------------------------------------------
//   ===================  TParsingEventListItem  ======================
// ----------------------------------------------------------------------------

bool TParsingEventListItem::FormatItem(TBasicListBoxObject &owner, TMidLevStoreReader *db_link, TMidLevScanner *active_scanner)
{
	// Clear prev contents of the cells if any.
	ClearBkgrAndCells(owner);

	// Overall result.
	bool res = TRUE;

	if (m_event.hdr.event_type == pset_none)
	{
		// This list box item is code location object. It needs special bkgr color.
		SetBkgrStyle(owner, TLangViewParsingEventsListDialog::bkgr_code_loc_normal, TLangViewParsingEventsListDialog::bkgr_code_loc_slct);

		// Draw all cells in special small small font.
		for (int inx=0; inx<owner.GetNumColumns(); ++inx)
		{
			SetFrgrStyleToCell(owner, inx,
						TLangViewParsingEventsListDialog::frgr_code_loc_normal,
						TLangViewParsingEventsListDialog::frgr_code_loc_slct);
		}
	}

	// Format the parsing ID and event ID.
	if (m_event.hdr.event_type != pset_none)
	{
		// This is some parsing event.
		if (SetFmtStrToCell(owner, TLangViewParsingEventsListDialog::lbc_evntid, L"%ld.%03ld", m_event.hdr.parsing_id, m_event.hdr.event_id) == FALSE)
			res = FALSE;
	}
	else
	{
		// This is the code area item. It has "fake" event_id that is used only for sorting. Do not show it.
		SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_evntid, L"n/a");
	}

	// Call the CtvParsingEventInfo class to prepare the event type name.
	SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_evtype, m_event.GetEventTypeNameForListBox());

	// Setup file name, line number and the char position.
	if (m_event.hdr.area.IsNull() == TRUE)
	{
		// This event has no source code location.
		SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_fname, L"n/a");
		SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_lnum, L"n/a");
		SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_chpos, L"n/a");
	}
	else
	{
		TSourceLocation src_loc;
		TSourceAreaBodyInfo body_info;
		if (db_link->GetSourceLocationInfo(src_loc, m_event, active_scanner) == TRUE && db_link->GetSourceAreaBodyInfo(body_info, src_loc.src_area, NULL, TRUE) == TRUE)
		{
			// Source location was successfully retrieved. Display the short file name and the line number.
			SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_fname, TPathHelper::GetShortNamePtr(body_info.area.file_info->info.file_name));

			if (SetFmtStrToCell(owner, TLangViewParsingEventsListDialog::lbc_lnum,  L"%ld", body_info.first_line_num) == FALSE)
				res = FALSE;
			if (SetFmtStrToCell(owner, TLangViewParsingEventsListDialog::lbc_chpos, L"%ld", body_info.first_line_charpos) == FALSE)
				res = FALSE;

			// Save the file offset to use it for sorting in future.
			m_file_offs = body_info.area.area_beg;
		}
		else
		{
			// Error retrieving the source location info.
			SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_fname, L"Error");
			SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_lnum, L"Error");
			SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_chpos, L"Error");
		}
	}

	// Prepare the description field.
	wchar_t buffer[1024];
	const wchar_t *pConstDescr = m_event.GetEventDescriptionForListBox(buffer, 1024);
	if (pConstDescr != NULL)
	{
		// Return value is the RO string that does not need disposal.
		SetExtStringToCell(owner, TLangViewParsingEventsListDialog::lbc_descr, pConstDescr);
	}
	else if (buffer[0] != 0)
	{
		// Return value is generated string that sits in the buffer.
		if (SetStringToCell(owner, TLangViewParsingEventsListDialog::lbc_descr, buffer) == FALSE)
			res = FALSE;
	}

	// Failure can happen only in the case of OOM. Most likely the app will not check it.
	return(res);
}

int TParsingEventListItem::CompareByPidEvid(void *ctx, const TBasicListBoxItem **p1, const TBasicListBoxItem **p2)
{
	// Reveal resl types of params and compare the event fields.
	TParsingEventListItem *i1 = *((TParsingEventListItem**)p1);
	TParsingEventListItem *i2 = *((TParsingEventListItem**)p2);
	return(CtvParsingEventInfo::ComparePidEvidForListBox(&(i1->m_event), &(i2->m_event)));
}

int TParsingEventListItem::CompareByTypeDesc(void *ctx, const TBasicListBoxItem **p1, const TBasicListBoxItem **p2)
{
	TParsingEventListItem *i1 = *((TParsingEventListItem**)p1);
	TParsingEventListItem *i2 = *((TParsingEventListItem**)p2);

	// Compare types and descriptions as they are formatted now.
	int res1 = TStringPtr::Compare(i1->GetStrPtr(TLangViewParsingEventsListDialog::lbc_evtype), i2->GetStrPtr(TLangViewParsingEventsListDialog::lbc_evtype));
	if (res1 != 0)
		return(res1);
	int res2 = TStringPtr::Compare(i1->GetStrPtr(TLangViewParsingEventsListDialog::lbc_descr), i2->GetStrPtr(TLangViewParsingEventsListDialog::lbc_descr));
	if (res2 != 0)
		return(res2);

	// Fall back to the pid-evid comparison.
	return(CtvParsingEventInfo::ComparePidEvidForListBox(&(i1->m_event), &(i2->m_event)));
}

int TParsingEventListItem::CompareByFileLine(void *ctx, const TBasicListBoxItem **p1, const TBasicListBoxItem **p2)
{
	TParsingEventListItem *i1 = *((TParsingEventListItem**)p1);
	TParsingEventListItem *i2 = *((TParsingEventListItem**)p2);

	// Compare the short file names. Contents of these cells are always extern NULL terminated strings.
	int res = wcscmp(i1->GetStrPtr(TLangViewParsingEventsListDialog::lbc_fname).GetBodyPtr(), i2->GetStrPtr(TLangViewParsingEventsListDialog::lbc_fname).GetBodyPtr());
	if (res != 0)
		return(res);

	// Compare the file offsets.
	if (i1->m_file_offs > i2->m_file_offs)
		return(1);
	else if (i1->m_file_offs < i2->m_file_offs)
		return(-1);

	// Event locations are identical. Compare the parsing ids and event ids.
	return(CtvParsingEventInfo::ComparePidEvidForListBox(&(i1->m_event), &(i2->m_event)));
}

// ----------------------------------------------------------------------
//   =============  TParsingEventsListBoxTable  ====================
// ----------------------------------------------------------------------

TParsingEventsListBoxTable::TParsingEventsListBoxTable(TMidLevStoreReader *db)
{
	m_pos = 0;
	m_type_descr_order = FALSE;
	m_foreign_obj_inx = -1;

	m_err_callback = NULL;
	m_fmt_owner = NULL;
	m_db_link = db;
}

void TParsingEventsListBoxTable::Seek(int inx)
{
	if (inx < 0)
		inx = 0;
	if (inx > m_data.NumItems())
		inx = m_data.NumItems();
	m_pos = inx;
}

TParsingEventListItem *TParsingEventsListBoxTable::ReadItem(bool read_forward)
{
	// Find pointer to the object in the data table.
	CtvParsingEventInfo *pev;
	if (read_forward == TRUE)
	{
		if (m_pos >= m_data.NumItems())
			return(NULL);
		pev = m_data[m_pos++];
	}
	else
	{
		if (m_pos <= 0)
			return(NULL);
		pev = m_data[--m_pos];
	}

	// Table contains requested event. Pick up the ListBoxItem.
	TParsingEventListItem *lvi = m_look_aside_data.GetItem();
	if (lvi == NULL)
	{
		m_err_callback->MsgBox(L"Low on memory while allocating new TParsingEventListItem.");
		return(NULL);
	}

	// Allocation succeeded. Prepare the item and return it.
	lvi->Clear();
	lvi->SetEventData(*pev);
	lvi->FormatItem(*m_fmt_owner, m_db_link, NULL);
	return(lvi);
}

bool TParsingEventsListBoxTable::LoadParsingEvents(ID pars_id, PSET_TYPES_MASK evt_mask, ID min_evid, ID max_evid)
{
	assert(m_db_link != NULL);

	// Note that ResetContents() will discard the foreign object element if it is present in the table.
	ResetContents();
	bool res = m_db_link->GetParsingEventsArray(m_data, pars_id, evt_mask, min_evid, max_evid);
	if (res == FALSE)
	{
		ResetContents();
		return(FALSE);
	}

	// The GetParsingEventsArray() method returns data in the "pid-evid" order. Check, if it is necessary to resort the table.
	if (m_type_descr_order == TRUE)
	{
		// Set value of the field to the current order of the data in the table. Otherwise SortItems() will not do anything.
		m_type_descr_order = FALSE;
		SortItems(TRUE);
	}

	// Success.
	return(TRUE);
}

int TParsingEventsListBoxTable::FindEvent(CtvParsingEventInfo &event_info)
{
	if (m_data.NumItems() == 0)
		return(-1);

	if (event_info.hdr.event_type == pset_none)
	{
		// This is code location. Ensure that there is no foreign object in the table.
		assert(m_foreign_obj_inx == -1);
		return(-1);
	}

	// Find location.
	int inx = BinSearchInx(&event_info);

	// Check for exact match of events at the discovered location.
	if (inx == 0 || CtvParsingEventInfo::ComparePidEvidForListBox(&event_info, m_data[inx-1]) != 0)
		return(-1);

	return(inx-1);
}

int TParsingEventsListBoxTable::AddForeignObject(CtvParsingEventInfo &event_info)
{
	// Ensure that there is no foreign object in the table.
	assert(m_foreign_obj_inx < 0);
	assert(event_info.hdr.parsing_id != 0);

	// Copy the passed data inside.
	m_foreign_obj_info = event_info;

	// Find index of the place, where the foreign object should be inserted.
	int inx;
	if (event_info.hdr.event_type == pset_none && m_type_descr_order == TRUE)
	{
		// Current sort order is evnt_type -> descr. The foreign object, that is being inserted, has type pset_none.
		// This is the lowest possible type value and other elements in the table cannot have this type. This means
		// that object should be placed directly on the top of the list. Note that event_id, that is assigned below is
		// temporary. Real value will be assigned if the sort order will be changed.
		m_foreign_obj_info.hdr.event_id = 0;
		inx = 0;
	}
	else
	{
		// Use fast search to get the appropriate index. This call is using smart dependency between the the check
		// above and the check inside the BinSearchInx(). Such combination allows calling BinSearchInx() without
		// setting the fake event id in the foreign object info when it is code area.
		inx = BinSearchInx(&m_foreign_obj_info);
		if (event_info.hdr.event_type == pset_none)
		{
			// Assign the fake event_id.
			if (inx < m_data.NumItems())
				m_foreign_obj_info.hdr.event_id = m_data[inx]->hdr.event_id;
			else m_foreign_obj_info.hdr.event_id = m_data[inx-1]->hdr.event_id+1;
		}
	}

	// Add pointer to the table at this index. Use explicit pointer because compiler does not want to create reference
	// to address that is generated at the point of passing the parameter.
	CtvParsingEventInfo *pfo = &m_foreign_obj_info;
	bool res = (inx == m_data.NumItems()) ? m_data.AppendItem(pfo) : m_data.InsertItem(inx, pfo);
	if (res == FALSE)
		return(-1);

	// Save index of the inserted record.
	m_foreign_obj_inx = inx;
	if (inx < m_pos)
		m_pos++;
	return(inx);
}

bool TParsingEventsListBoxTable::RemoveForeignObject()
{
	if (m_foreign_obj_inx < 0)
		return(FALSE);

	// Foreign object is present. Release it.
	m_data.ReleaseItem(m_foreign_obj_inx);
	if (m_pos > m_foreign_obj_inx)
		m_pos--;

	m_foreign_obj_inx = -1;
	return(TRUE);
}

int TParsingEventsListBoxTable::SortItems(bool new_sort_order)
{
	if (new_sort_order == m_type_descr_order)
		return(m_foreign_obj_inx);

	// Save the new sort order.
	m_type_descr_order = new_sort_order;

	if (m_data.NumItems() > 1)
	{
		// Data array needs non empty sorting.
		if (m_type_descr_order == TRUE)
		{
			// Old sort order is Pid-Evid. Change it to Type-Descr. Simple comparison function can be used because
			// fake event id to the foreign code area item is already assigned.
			m_data.QuickSort(CtvParsingEventPtrsArray::CompareTypeDescr);
		}
		else
		{
			// Old sort order is Type-Descr.
			if (m_foreign_obj_inx >= 0 && m_foreign_obj_info.hdr.event_type == pset_none && m_foreign_obj_info.hdr.event_id == 0)
			{
				// Special case when assigning event_id to the code area was postponed.
				assert(m_foreign_obj_inx == 0);

				// Temporarily remove this foreign code area, sort the list and add it back.
				RemoveForeignObject();
				m_data.QuickSort(CtvParsingEventPtrsArray::ComparePidEvid);
				AddForeignObject(m_foreign_obj_info);
			}
			else
			{
				// Simple case. Resort into the Pid-Evid order.
				m_data.QuickSort(CtvParsingEventPtrsArray::ComparePidEvid);
			}
		}

		if (m_foreign_obj_inx >= 0)
		{
			// Foreign object may have changed its position. Find its new index.
			m_foreign_obj_inx = BinSearchInx(&m_foreign_obj_info)-1;

			// Ensure success of the binary search.
			assert(m_foreign_obj_inx >= 0);
			assert(m_data[m_foreign_obj_inx] == &m_foreign_obj_info);
		}
	}

	// Return index of the foreign object. This allows to search only once if the foreign object is at the same
	// time the selected object.
	return(m_foreign_obj_inx);
}

int TParsingEventsListBoxTable::BinSearchInx(CtvParsingEventInfo *ptr)
{
	// Check the first and the last elements.
	if (m_data.NumItems() <= 0 || CompareWithTableItem(ptr, 0) < 0)
		return(0);
	if (m_data.NumItems() == 1 || CompareWithTableItem(ptr, m_data.NumItems()-1) >= 0)
		return(m_data.NumItems());

	// Search the body of the array.
	int lo_good = 0;
	int hi_bad = m_data.NumItems()-1;
	while ((hi_bad - lo_good) > 1)
	{
		int inx_test = (hi_bad+lo_good)/2;
		if (CompareWithTableItem(ptr, inx_test) >= 0)
			lo_good = inx_test;
		else hi_bad = inx_test;
	}

	//
	// Return value is index of the element, that is:
	//
	//   m_data[hi_bad-1] <= ptr < m_data[hi_bad];
	//
	assert(hi_bad == lo_good+1);
	return(hi_bad);
}

int TParsingEventsListBoxTable::CompareWithTableItem(const CtvParsingEventInfo *p1, int inx_p2)
{
	//
	// Expected return value:
	//
	//    p1 > p2  ==> +1
	//    p1 = p2  ==>  0
	//    p1 < p2  ==> -1
	//
	const CtvParsingEventInfo *p2 = m_data[inx_p2];

	if (m_type_descr_order == TRUE)
	{
		return(CtvParsingEventInfo::CompareTypeDescrForListBox(p1, p2));
	}

	// Table is sorted in the Pid-Evid order.
	if (p1->hdr.event_type == pset_none)
	{
		//
		// This is special case when event_id of the p1 is not known.
		//

		// Compare parsing ids.
		if (p1->hdr.parsing_id > p2->hdr.parsing_id)
			return(1);
		if (p1->hdr.parsing_id < p2->hdr.parsing_id)
			return(-1);

		// Compare linear offsets. Items in array may have no direct relation to the code location. In this case
		// they will have zero in their linear_beg field. Use linear_beg of the previous array item in this case.
		long linear_offs = p2->hdr.area.linear_beg;
		while (linear_offs == 0 && inx_p2 > 0)
			linear_offs = m_data[--inx_p2]->hdr.area.linear_beg;

		if (p1->hdr.area.linear_beg > linear_offs)
			return(1);
		if (p1->hdr.area.linear_beg < linear_offs)
			return(-1);

		// Put code area item in front of all events that start at this location. This will be consistent
		// with sorting events by event type. Code area has smallest event type value.
		return(-1);
	}
	else
	{
		// Use simple Pid-Evid comparison.
		return(CtvParsingEventInfo::ComparePidEvidForListBox(p1, p2));
	}
}

// --------------------------------------------------------------------
//   ============  TLangViewParsingEventsListDialog  ===============
// --------------------------------------------------------------------

static TListBoxColumnProps g_ParsingEventsList_ColumnsInfo[] =
{
	{ align_center,	4,	 64,	   0,	  10,		},		// ParsId.EventId.
	{ align_left,		0,	132,	   0,	    4,		},		// EventType.
	{ align_left,		0,	 84,	   5,	    1,		},		// FileName.
	{ align_right,		0,	 48,	   0,	  20,		},		// LineNumber.
	{ align_right,		0,	 30,	   0,	  20,		},		// CharPosition.
	{ align_left,		0,	 64,	  90,	    4,		},		// Description.
	LBX_CLMNP_EOF
};

static TListBoxColumnTilteProps g_ParsingEventsList_TitleInfo[] =
{
	{ 1,	 FALSE,	L"Event"		},
	{ 1,	 FALSE,	L"Type"		},
	{ 1,	 FALSE,	L"File"		},
	{ 1,	 FALSE,	L"Line"		},
	{ 1,	 FALSE,	L"Pos"		},
	{ 1,	 FALSE,	L"Description"	},
	{ 0,	 FALSE,	NULL		},	// EOF record.
};

static TTextStyleSymbolAdjustInfo g_ParsingEventsList_Adjustments[] =
{
	{ L'.', 0, 1, 1 }, { L':', 0, 1, 1 }, { 0, 0, 0, 0 },
};

static TParsingEventsSortOrder g_ParsingEventsList_ClmnToPesoTableMode[TLangViewParsingEventsListDialog::lbc_num_clmns] =
{
	peso_pid_evid,  peso_type_descr,
	peso_num_types, peso_num_types, peso_num_types,
	peso_type_descr,
};

static TParsingEventsSortOrder g_ParsingEventsList_ClmnToPesoListMode[TLangViewParsingEventsListDialog::lbc_num_clmns] =
{
	peso_pid_evid,  peso_type_descr,
	peso_file_line, peso_file_line, peso_file_line,
	peso_type_descr,
};

#define SORT_ORDER_SUFFIX L" (*)"

static short g_ParsingEventsList_TitleColumnFromSortOrder[peso_num_types] =
{
	TLangViewParsingEventsListDialog::lbc_evntid,
	TLangViewParsingEventsListDialog::lbc_evtype,
	TLangViewParsingEventsListDialog::lbc_fname,
};

TLangViewParsingEventsListDialog::TLangViewParsingEventsListDialog(TMidLevStoreReader *db_link, bool lb_table_mode, bool allow_resorting)
		: TLangViewChildBrowser(db_link, cbrt_events_list, IDD_LANGV_SVF_PARS_EVENTS_LIST),
		m_lb_table(db_link)
{
	if (lb_table_mode == TRUE)
	{
		// List box table is set to the pid-evid sort mode in its ctor. Duplicate the same sort order here.
		m_lb_table.SetOwners(this, &m_list_hlpr);
		m_list_hlpr.SetupListBoxTable((TBasicListBoxTable*)&m_lb_table, FALSE);
		m_sort_order = peso_pid_evid;
	}
	else
	{
		// Put dialog into the unsorted state. This state allows adding new events to the list.
		m_sort_order = peso_num_types;
	}

	m_allow_resorting = allow_resorting;

	// Foreign item member is not a separate heap allocation.
	m_temp_slct_item.SetNotOwned();
}

LRESULT TLangViewParsingEventsListDialog::OnInitDialog(HWND hDefaultFocusControl)
{
	// Prepare resizing.
	AdjustControlRightSide(IDC_LVEVL_TITLE);
	ShiftControl(IDC_LVEVL_NUM_ITEMS, 0, 1);
	AdjustControlRightSide(IDC_LVEVL_LISTBOX);
	SetupControlPositioning(IDC_LVEVL_TITLE, cpm_fix_top_side);
	SetupControlPositioning(IDC_LVEVL_NUM_ITEMS, cpm_fix_right_top);
	SetupControlPositioning(IDC_LVEVL_LISTBOX, cpm_fix_all_corners);

	//
	//  Prepare the ListBox.
	//

	// Step1. Setup windows.
	m_list_hlpr.SetWindow(::GetDlgItem(m_hDlg, IDC_LVEVL_LISTBOX));
	m_list_hlpr.SetTitleWindow(::GetDlgItem(m_hDlg, IDC_LVEVL_TITLE));

	// Step2a. Setup list box props to a non auto select mode.
	TGenericListBoxObjectProps props_data;
	props_data.PrepareForSingleSelect(FALSE);
	m_list_hlpr.SetupProps(props_data, FALSE);

	// Step2b. Set up special bkgr and frgr styles.
	m_list_hlpr.SetupSystemStdStyles(1, 1, g_ParsingEventsList_Adjustments);
	m_list_hlpr.SetupBkgrStyleSlot(bkgr_code_loc_normal, RGB(60, 60, 156));
	m_list_hlpr.SetupBkgrStyleSlot(bkgr_code_loc_slct, RGB(160, 160, 156));
	m_list_hlpr.SetupFrgrStyleSlot(frgr_code_loc_normal, SMALL_FONTS, 6, FALSE, RGB(192, 192, 255), 0, 0, 1);
	m_list_hlpr.SetupFrgrStyleSlot(frgr_code_loc_slct, SMALL_FONTS, 6, FALSE, RGB(232, 232, 230), 0, 0, 1);

	// Step3.  Setup the columns of the listbox.
	m_list_hlpr.SetupColumnProps(g_ParsingEventsList_ColumnsInfo);
	m_list_hlpr.SetupColumnTitles(g_ParsingEventsList_TitleInfo);

	// Step4. Finalize the setup.
	m_list_hlpr.RebuildListBox();
	UpdateNumItemsLabel();

	SetFocusControl(IDC_LVEVL_LISTBOX);
	return(FALSE);
}

void TLangViewParsingEventsListDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_LVEVL_TITLE:
				{
					// User clicked the listbox title.
					if (notificationCode == lbnc_left_btn_down)
					{
						TListBoxMouseEventInfo *mouse_info = (TListBoxMouseEventInfo*)hWndControl;
						ResetUserCancel();

						if (m_allow_resorting == FALSE)
							break;

						// Resorting is currently enabled.
						if (mouse_info->inside_the_column == FALSE || m_sort_order == peso_num_types)
						{
							// Ignore this click.
							break;
						}

						// Pick up the new sort order.
						TParsingEventsSortOrder new_peso = (m_list_hlpr.GetListBoxTableMode() == TRUE) ?
																g_ParsingEventsList_ClmnToPesoTableMode[mouse_info->clmn_index] :
																g_ParsingEventsList_ClmnToPesoListMode[mouse_info->clmn_index];
						if (new_peso != peso_num_types && new_peso != m_sort_order)
						{
							// Do not process resort locally. Ask the owner of the dialog to process the resorting.
							m_owner->ProcessResortEvent(this, 0, new_peso);
						}
					}
					else if (notificationCode == lbnc_right_btn_down)
					{
						TListBoxMouseEventInfo *mouse_info = (TListBoxMouseEventInfo*)hWndControl;
						ResetUserCancel();

						// Pick up the counts of available events and show the context menu.
						int event_type_counts[pset_num_types], error_class_counts[lerrc_num_errorclass_types], other_events;
						CollectEventTypeStatistics(event_type_counts, pset_num_types, error_class_counts, lerrc_num_errorclass_types, &other_events);

						#define GEN_MENU_ITEM(flags, ident, text)	\
								context_menu[cnt_items].item_flags = flags; context_menu[cnt_items].item_id = ident; context_menu[cnt_items].item_text = text; cnt_items++

						// Generate the menu.
						TMenuItemInfo context_menu[pset_num_types+lerrc_num_errorclass_types+6];
						wchar_t buffer[80*(pset_num_types+lerrc_num_errorclass_types+2)];
						int cnt_items = 0;

						// This loop will generate menu items for all regular event types and error classes.
						for (int ievt=pset_none; ievt <= pset_num_types; ++ievt)
						{
							if (ievt == pset_error_warning_event)
							{
								// Generate menu items for different error classes.
								for (int erc=lerrc_message; erc<lerrc_num_errorclass_types; ++erc)
								{
									if (error_class_counts[erc] > 0)
									{
										swprintf(buffer+80*cnt_items, 80, L"%s  \t(%d event%s)", TLexema::GetErrorClassName((TLexErrorClass)erc), error_class_counts[erc], PluralSuffix(error_class_counts[erc]));
										GEN_MENU_ITEM(0, 200+erc, buffer+80*cnt_items);
									}
								}
							}
							else if (event_type_counts[ievt] > 0)
							{
								// Events with the current event type are available.
								swprintf(buffer+80*cnt_items, 80, L"%s  \t(%d event%s)", CtvEventHeader::GetEventTypeName((CtvEventType)ievt), event_type_counts[ievt], PluralSuffix(event_type_counts[ievt]));
								GEN_MENU_ITEM(0, 100+ievt, buffer+80*cnt_items);
							}
						}

						if (other_events > 0)
						{
							// Mysterious other events are present.
							GEN_MENU_ITEM(MF_SEPARATOR, -1, NULL);
							swprintf(buffer+80*cnt_items, 80, L"Other events  \t(%d event%s)", other_events, PluralSuffix(other_events));
							GEN_MENU_ITEM(0, 300, buffer+80*cnt_items);
						}

						// Append the std items and the eof item.
						GEN_MENU_ITEM(MF_SEPARATOR, -1, NULL);
						GEN_MENU_ITEM(0, 301, L"Hide all events");
						GEN_MENU_ITEM(0, 302, L"Show all events");
						memset(context_menu+cnt_items, 0, sizeof(TMenuItemInfo));

						// Track menu and process its result.
						DWORD res = m_list_hlpr.ShowAndTrackPopupMenu(context_menu, mouse_info);
					}
				}
				break;

		case IDC_LVEVL_LISTBOX:
				{
					// User clicked one of events or the area below all events.
					TListBoxNotificationCode ntf_code = (TListBoxNotificationCode)notificationCode;
					if (ntf_code == lbnc_left_btn_down || ntf_code == lbnc_left_btn_dblclk)
					{
						TListBoxMouseEventInfo *mouse_info = (TListBoxMouseEventInfo*)hWndControl;
						ResetUserCancel();

						TParsingEventListItem *sel_item = (TParsingEventListItem*)(mouse_info->clicked_item);
						if (sel_item == NULL)
						{
							// This situation is possible only when the listbox table returns NULL on reading. Do not do anything.
							break;
						}

						if (mouse_info->inside_item_rect == FALSE)
						{
							// User has clicked below all items.
							m_owner->RemoveSelection(this);
							break;
						}

						// Ignore the click if the clicked item is already selected.
						if (sel_item->IsSelected() == TRUE)
							break;

						// Prepare the click event structure. Ensure that clicked item is not the code area. This should not happen
						// because event gets selected not from inside the listbox, but as result of the call to SetSelection() method.
						// This will also guarantee that its event_id is not fake.
						assert(sel_item->m_event.hdr.event_type != pset_none);

						// Fill in the event structure. Click area will be either empty or it will contain what database
						// will create out of the parsing id and the linear area of the event header.
						TLangViewClickEvent info;
						info.SetMidLevEvent(sel_item->m_event);

						// Event structure is prepared. Send it.
						SendForProcessingFromListBox(ntf_code, info);
					}
				}
				break;
	}
}

void TLangViewParsingEventsListDialog::LoadParsingEvents(ID pars_id, PSET_TYPES_MASK event_types_mask, ID min_evid, ID max_evid)
{
	// This method can be called only in the lb_table mode.
	assert(m_db != NULL);
	assert(m_list_hlpr.GetListBoxTableMode() == TRUE);

	// Note that list box table is sorting its elements on load.
	if (m_lb_table.LoadParsingEvents(pars_id, event_types_mask, min_evid, max_evid) == FALSE)
	{
		MsgBox(L"Error loading parsing events into the listbox.");
	}

	m_list_hlpr.RebuildListBox();
	UpdateNumItemsLabel();
}

bool TLangViewParsingEventsListDialog::AddParsingEvent(CtvParsingEventInfo &event)
{
	// This method can be called only in the linked list mode.
	assert(m_db != NULL);
	assert(m_list_hlpr.GetListBoxTableMode() == FALSE);
	assert(event.hdr.event_type != pset_none);

	if (m_sort_order != peso_num_types)
	{
		MsgBox(L"Adding parsing event to sorted list of events is not supported.");
		assert(FALSE);
		return(FALSE);
	}

	// Allocate and format the item.
	TParsingEventListItem *item = new TParsingEventListItem(event);
	if (item == NULL)
		return(FALSE);
	item->FormatItem(m_list_hlpr, m_db, NULL);

	// Add item to the end of the listbox. This is ok because the list is in the unsorted mode.
	m_list_hlpr.AddItem(item);
	UpdateNumItemsLabel();
	return(TRUE);
}

void TLangViewParsingEventsListDialog::ResetContents()
{
	if (m_list_hlpr.GetListBoxTableMode() == TRUE)
	{
		// Kill data in the table and show the new state of the list.
		m_lb_table.ResetContents();
		m_list_hlpr.RebuildListBox();
	}
	else
	{
		// This call will also rebuild the listbox.
		m_list_hlpr.DeleteAllItems();
		m_sort_order = peso_num_types;
	}

	UpdateNumItemsLabel();
}

void TLangViewParsingEventsListDialog::SetSelection(TLangViewClickEvent &event, bool show_margins)
{
	// Reset existing selection if any. This will also remove the foreign object if it was displayed.
	ResetSelection();

	if (m_list_hlpr.GetListBoxTableMode() == TRUE)
	{
		// Convert passed click event either into the mid lev event or into the code area.
		CtvParsingEventInfo event_proto;
		if (event.type == clet_mid_lev_event)
		{
			// Downgrade the TLangViewClickEvent to CtvParsingEvent.
			event_proto = event.mldb_event_data;
		}
		else
		{
			// Try to prepare the pset_none event. In the case of succes the fake event_id will be assigned
			// to this synthetic event later.
			event_proto.Clear();
			TSourceLocation evnt_src_loc;
			if (event.GetSrcLocation(evnt_src_loc) == TRUE && evnt_src_loc.parsing_id != 0)
			{
				// Event belongs to some space of the parsed code.
				if (evnt_src_loc.lin_area.linear_beg > 0)
				{
					event_proto.hdr.Init(evnt_src_loc.parsing_id, evnt_src_loc.lin_area);
				}
				else if (evnt_src_loc.source_ref != 0 && m_db->GetLexSourceEnterEvent(evnt_src_loc.parsing_id, evnt_src_loc.source_ref) == TRUE)
				{
					// Use empty linear area at the beginning of the lex src enclosure inside it.
					TLinearArea lin_beg_inside = { m_db->EventData().src_enter.LinearPosAfterEntering(), 0 };
					event_proto.hdr.Init(m_db->EventData().hdr.parsing_id, lin_beg_inside);
				}
			}

			if (event_proto.hdr.parsing_id == 0 || event_proto.hdr.area.linear_beg == 0)
			{
				// Event does not belong to the space of the parsed code. It is not possible to temporarily
				// add this event to the events table as foreign "code area" object.
				return;
			}
		}

		// First try to find this event in the table.
		int inx_to_sel = m_lb_table.FindEvent(event_proto);
		if (inx_to_sel == -1)
		{
			// Add this event to the table as a foreign object.
			inx_to_sel = m_lb_table.AddForeignObject(event_proto);
			if (inx_to_sel >= 0)
			{
				m_list_hlpr.RebuildListBox();
			}
			else
			{
				MsgBox(L"Error adding foreign object into the list of parsing events");
				return;
			}
		}

		// Scroll listbox to the found or added item and make this item selected.
		m_list_hlpr.SelectItemByIndex(inx_to_sel);
		m_list_hlpr.ScrollItemIntoTheView(inx_to_sel, show_margins);
	}
	else
	{
		// Dialog is in the linked list mode. Check the type of the passed event.
		if (event.type != clet_mid_lev_event)
		{
			// Passed event is not a mid lev parsing event. Ignore this selection request.
			return;
		}

		// Prepare for potentially long lasting processing.
		HCURSOR hPrevCursor = NULL;
		if (m_list_hlpr.GetNumItems() > 50000)
		{
			hPrevCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
		}

		// Iterare existing list of events.
		TParsingEventListItem *item_to_sel = NULL;
		TListIter<TParsingEventListItem> iter = m_list_hlpr.GetAppIter<TParsingEventListItem>();
		if (m_sort_order == peso_num_types)
		{
			// List is in the unsorted mode. Only exact matches are allowed.
			for (iter.SeekToFirst(); iter; ++iter)
			{
				TParsingEventListItem *item = &iter.CurrItem();

				// Compare parsing_id and event_id.
				if (event.mldb_event_data.hdr.Equal(item->m_event.hdr) == TRUE)
				{
					item_to_sel = item;
					break;
				}
			}

			if (item_to_sel == NULL)
			{
				// Passed item is not present in the list. There is nothing to select.
				if (hPrevCursor != NULL)
					::SetCursor(hPrevCursor);
				return;
			}
		}
		else
		{
			// List is sorted. Any mid lev event will be accepted: either as exact match or as temp foreign event.
			// Take in the passed event and format the item. It is necessary to do this before the search because
			// comparison expects to compare the formatted items.
			m_temp_slct_item.SetEventData(event.mldb_event_data);
			m_temp_slct_item.FormatItem(m_list_hlpr, m_db, NULL);

			// Look for exact match or place to insert the foreign item.
			TParsingEventListItem *ins_before_item = NULL;
			for (iter.SeekToFirst(); iter; ++iter)
			{
				TParsingEventListItem *item = &iter.CurrItem();
				int res = CompareParsingEventListItems(&m_temp_slct_item, item);
				if (res == 0)
				{
					item_to_sel = item;
					break;
				}
				else if (res < 0)
				{
					ins_before_item = item;
					break;
				}
			}

			if (item_to_sel == NULL)
			{
				// Insert passed event into the list as a foreign event.
				m_list_hlpr.AddItem(&m_temp_slct_item, ins_before_item);
				item_to_sel = &m_temp_slct_item;
			}
		}

		if (hPrevCursor != NULL)
			::SetCursor(hPrevCursor);

		// Scroll to this item and select it.
		assert(item_to_sel != NULL);
		m_list_hlpr.ScrollItemIntoTheView(item_to_sel, show_margins);
		m_list_hlpr.SelectItem(item_to_sel);
	}
}

void TLangViewParsingEventsListDialog::ResetSelection()
{
	m_list_hlpr.RemoveSelection();
	if (m_list_hlpr.GetListBoxTableMode() == TRUE)
	{
		// Check if it is necessary to modify contents of the table.
		if (m_lb_table.RemoveForeignObject() == TRUE)
		{
			m_list_hlpr.RebuildListBox();
		}
	}
	else
	{
		// Check if foreign item is in the list or not.
		if (m_temp_slct_item.IsInList() == TRUE)
		{
			m_list_hlpr.RemoveItem(&m_temp_slct_item);
		}
	}
}

void TLangViewParsingEventsListDialog::SetSortOrder(int list_id, DWORD new_order)
{
	if (m_allow_resorting == FALSE)
		return;

	assert(list_id == 0);
	if (new_order == m_sort_order)
		return;

	// Remove the old sort order mark from the title.
	m_list_hlpr.SetupColumnTitleSuffix(-1, SORT_ORDER_SUFFIX);

	HCURSOR hPrevCursor = NULL;
	if (m_list_hlpr.GetNumItems() > 50000)
	{
		// Prepare for potentially long lasting processing.
		hPrevCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
	}

	// Resort the list and rebuild the local variables.
	int slct_inx = -1;
	TParsingEventListItem *old_slct_item = NULL;

	if (m_list_hlpr.GetListBoxTableMode() == TRUE)
	{
		// Save information about the old selection.
		int old_slct_inx = m_list_hlpr.GetSelectionIndex();
		CtvParsingEventInfo *old_slct_evt = (old_slct_inx >= 0) ? m_lb_table.GetEvent(old_slct_inx) : NULL;

		// Resort the table itself.
		int new_foreign_inx = m_lb_table.SortItems((new_order == peso_type_descr) ? TRUE : FALSE);
		m_sort_order = (new_order == peso_type_descr) ? peso_type_descr : peso_pid_evid;

		if (old_slct_evt != NULL)
		{
			// Selection was existing before.
			if (m_lb_table.IsForeignObjectPtr(old_slct_evt) == TRUE)
			{
				// Selected event is a foreign object. Its new index is already known.
				slct_inx = new_foreign_inx;
			}
			else
			{
				// Selected item is not a foreign object. FindEvent() finction can find it.
				assert(old_slct_evt->hdr.event_type != pset_none);
				slct_inx = m_lb_table.FindEvent(*old_slct_evt);
				assert(slct_inx >= 0);
			}

			// Reselect this record again because most likely its index changed.
			m_list_hlpr.SelectItemByIndex(slct_inx);
		}
	}
	else
	{
		// Dialog is in the double linked list mode.
		old_slct_item = (TParsingEventListItem*)m_list_hlpr.GetSelectedItem();
		switch (new_order)
		{
			case peso_pid_evid:
					m_list_hlpr.SortLocalList(this, TParsingEventListItem::CompareByPidEvid);
					break;

			case peso_type_descr:
					m_list_hlpr.SortLocalList(this, TParsingEventListItem::CompareByTypeDesc);
					break;

			case peso_file_line:
					m_list_hlpr.SortLocalList(this, TParsingEventListItem::CompareByFileLine);
					break;

			default:
				assert(FALSE);
				break;
		}

		m_sort_order = (TParsingEventsSortOrder)new_order;
	}

	// Rebuild the listbox and scroll selection into the view if it was present before.
	m_list_hlpr.RebuildListBox();
	if (slct_inx >= 0)
	{
		m_list_hlpr.ScrollItemIntoTheView(slct_inx, TRUE);
	}
	else if (old_slct_item != NULL)
	{
		m_list_hlpr.ScrollItemIntoTheView(old_slct_item, TRUE);
	}

	// Change the mouse cursor back.
	if (hPrevCursor != NULL)
		::SetCursor(hPrevCursor);

	// Put the sort order mark back.
	m_list_hlpr.SetupColumnTitleSuffix(g_ParsingEventsList_TitleColumnFromSortOrder[m_sort_order], SORT_ORDER_SUFFIX);
}

void TLangViewParsingEventsListDialog::CollectEventTypeStatistics(int *event_type_counts, int num_etp_counts, int *errwarn_class_counts, int num_ewc_counts, int *other_events)
{
	// Clear the counters.
	memset(event_type_counts, 0, num_etp_counts*sizeof(int));
	memset(errwarn_class_counts, 0, num_ewc_counts*sizeof(int));
	*other_events = 0;

	if (m_list_hlpr.GetListBoxTableMode() == TRUE)
	{
		// Iterate events in the list box table.
		int num_items = m_lb_table.NumItems();
		for (int inx=0; inx<num_items; ++inx)
		{
			SummarizeEvent(m_lb_table.GetEvent(inx), event_type_counts, num_etp_counts, errwarn_class_counts, num_ewc_counts, other_events);
		}
	}
	else
	{
		// Iterate events in the double linked list.
		TListIter<TParsingEventListItem> iter = m_list_hlpr.GetAppIter<TParsingEventListItem>();
		for (iter.SeekToFirst(); iter; ++iter)
		{
			SummarizeEvent(&iter.CurrItem().m_event, event_type_counts, num_etp_counts, errwarn_class_counts, num_ewc_counts, other_events);
		}
	}
}

void TLangViewParsingEventsListDialog::SummarizeEvent(CtvParsingEventInfo *event, int *event_type_counts, int num_etp_counts, int *errwarn_class_counts, int num_ewc_counts, int *other_events)
{
	CtvEventType et = event->hdr.event_type;
	if (et == pset_error_warning_event)
	{
		// Errors/warnings receive special treatment.
		TLexErrorClass erc = event->err_warn.err_class;
		if (erc >= 0 && erc < num_ewc_counts)
			errwarn_class_counts[erc]++;
		else *other_events++;
	}
	else if (et >= 0 && et < num_etp_counts)
	{
		// Event type has correct value.
		event_type_counts[et]++;
	}
	else
	{
		// Bogus event type.
		*other_events++;
	}
}

void TLangViewParsingEventsListDialog::UpdateNumItemsLabel()
{
	if (m_list_hlpr.GetNumItems() > 0)
	{
		wchar_t buff[80];
		SetControlTextFmt(IDC_LVEVL_NUM_ITEMS, L" NumItems: %s ", FormatInt64(m_list_hlpr.GetNumItems(), buff, 80, fnms_dec_signed, (m_list_hlpr.GetNumItems() >= 10000) ? L' ' : 0));
		ShowControl(IDC_LVEVL_NUM_ITEMS, TRUE);
	}
	else
	{
		ShowControl(IDC_LVEVL_NUM_ITEMS, FALSE);
	}
}

int TLangViewParsingEventsListDialog::CompareParsingEventListItems(const TParsingEventListItem *p1, const  TParsingEventListItem*p2)
{
	switch (m_sort_order)
	{
		case peso_pid_evid:
				return(TParsingEventListItem::CompareByPidEvid(this, (const TBasicListBoxItem**)&p2, (const TBasicListBoxItem**)&p2));

		case peso_type_descr:
				return(TParsingEventListItem::CompareByTypeDesc(this, (const TBasicListBoxItem**)&p1, (const TBasicListBoxItem**)&p2));

		case peso_file_line:
				return(TParsingEventListItem::CompareByFileLine(this, (const TBasicListBoxItem**)&p1, (const TBasicListBoxItem**)&p2));
	}

	return(0);
}

// -------------------------------------------------------------------------
//   ===================  Static functions  ========================
// -------------------------------------------------------------------------

static const wchar_t *PrepareCodeLocBrief(wchar_t *buff, TSourceLocation &area, long line_num)
{
	wchar_t file_id_buff[40];
	swprintf(buff, 40, L"%s, L%ld", area.src_area.PrepareFileIdShort(file_id_buff, 40), line_num);
	return(buff);
}

// ----------------------------------------------------------------------------
//    ==============  TLangViewLexSourcesNestingDialog  ==================
// ----------------------------------------------------------------------------

static TListBoxColumnProps g_SourcesNestingListboxColumnsInfo[] =
{
	{ align_center,	4,	24,	   0,	    0,		},		// Nesting level.
	{ align_left,		6,	68,	   3,	    6,		},		// File id and line number.
	{ align_left,		0,	12,	   0,	    0,		},		// Level column (to be duplicated).
	{ align_left,		0,	50,	  90,	    4,		},		// Max level (long column).
	LBX_CLMNP_EOF
};

TLangViewLexSourcesNestingDialog::TLangViewLexSourcesNestingDialog(TMidLevStoreReader *db_link)
		: TLangViewChildBrowser(db_link, cbrt_lex_src_nest, IDD_LANGV_SVF_LEX_SOURCE_NEST)
{
	m_code_loc.Clear();
	m_code_line_num = -1;
}

LRESULT TLangViewLexSourcesNestingDialog::OnInitDialog(HWND hDefaultFocusControl)
{
	// Prepare for resizing. Note that title of the listbox is a static message. This dialog does not have
	// titles for the listbox columns. The user will have to guess.
	AdjustControlRightSide(IDC_LVLSN_TITLE);
	AdjustControlRightSide(IDC_LVLSN_LISTBOX);
	SetupControlPositioning(IDC_LVLSN_TITLE, cpm_fix_top_side);
	SetupControlPositioning(IDC_LVLSN_LISTBOX, cpm_fix_all_corners);

	// Prepare the columns definition script.
	TListBoxColumnProps clmns_info[lbc_num_columns+1];
	clmns_info[lbc_nest] = g_SourcesNestingListboxColumnsInfo[lbc_nest];
	clmns_info[lbc_locat] = g_SourcesNestingListboxColumnsInfo[lbc_locat];
	for (int ic1=0; ic1<num_level_clmns-1; ++ic1)
		clmns_info[lbc_src_lev0+ic1] = g_SourcesNestingListboxColumnsInfo[lbc_src_lev0];
	clmns_info[lbc_src_levmax] = g_SourcesNestingListboxColumnsInfo[lbc_src_lev0+1];
	clmns_info[lbc_src_levmax+1] = g_SourcesNestingListboxColumnsInfo[lbc_src_lev0+2];

	//
	//  Prepare the ListBox.
	//

	// Step1. Setup window.
	m_list_hlpr.SetWindow(::GetDlgItem(m_hDlg, IDC_LVLSN_LISTBOX));

	// Step2. Setup list box props for non auto select mode.
	TGenericListBoxObjectProps props_data;
	props_data.PrepareForSingleSelect(FALSE);
	m_list_hlpr.SetupProps(props_data, FALSE);
	m_list_hlpr.SetupSystemStdStyles(1, 1);

	// Step3. Setup columns in the listbox.
	m_list_hlpr.SetupColumnProps(clmns_info);

	// Step4. Finalize the setup.
	m_list_hlpr.RebuildListBox();

	SetFocusControl(IDC_LVLSN_LISTBOX);
	return(FALSE);
}

void TLangViewLexSourcesNestingDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_LVLSN_LISTBOX:
				{
					// User clicked one of list box items or the area below all items.
					TListBoxNotificationCode ntf_code = (TListBoxNotificationCode)notificationCode;
					if (ntf_code == lbnc_left_btn_down || ntf_code == lbnc_left_btn_dblclk)
					{
						TListBoxMouseEventInfo *mouse_info = (TListBoxMouseEventInfo*)hWndControl;
						ResetUserCancel();

						TNestingLocationListItem *sel_item = (TNestingLocationListItem*)(mouse_info->clicked_item);
						if (sel_item == NULL || mouse_info->inside_item_rect == FALSE)
						{
							m_owner->RemoveSelection(this);
							break;
						}

						// Prepare the event data.
						TLangViewClickEvent info;
						if (sel_item->m_event.hdr.event_type == pset_none)
						{
							// This is tip element of the listbox.
							info.SetSourceLocationEvent(m_code_loc);
						}
						else
						{
							// This is some non tip element.
							assert(sel_item->m_event.hdr.event_type == pset_lex_source_enter);
							info.SetMidLevEvent(sel_item->m_event);
						}

						// Event is prepared. Send it.
						SendForProcessingFromListBox(ntf_code, info);
					}
				}
				break;
	}
}

void TLangViewLexSourcesNestingDialog::LoadContents(CtvEventHeader &new_code_loc)
{
	assert(m_db != NULL);

	// Retrieve the source location.
	TSourceLocation new_src_loc;
	if (m_db->GetSourceLocationInfo(new_src_loc, new_code_loc.parsing_id, new_code_loc.area) == FALSE)
	{
		// Passed header has no location. This is possible when the first lex source is not entered yet.
		ResetContents();
		return;
	}

	// Retrieve the line number for the beginning of the new area.
	assert(new_src_loc.parsing_id != 0 && new_src_loc.lin_area.linear_beg > 0);
	long new_line_num = m_db->GetSourceAreaLineBeg(new_src_loc.src_area);
	if (new_line_num < 0)
	{
		// Error is already reported.
		ResetContents();
		return;
	}

	assert(new_src_loc.source_ref != 0);
	if (new_src_loc.parsing_id == m_code_loc.parsing_id && new_src_loc.source_ref == m_code_loc.source_ref)
	{
		// New code_loc has the same source nesting as the current code_loc. This also means that
		// current location should be not empty. The list box should be non empty also.
		assert(m_list_hlpr.GetNumItems() > 0);

		TNestingLocationListItem *item = (TNestingLocationListItem*)m_list_hlpr.GetItemByIndex(m_list_hlpr.GetNumItems()-1);
		assert(item != NULL);

		m_code_loc = new_src_loc;
		item->m_event.hdr.parsing_id = new_code_loc.parsing_id;
		item->m_event.hdr.area = new_code_loc.area;

		if (new_line_num != m_code_line_num)
		{
			// Update the line number in the tip element.
			m_code_line_num = new_line_num;
			wchar_t code_loc_buff[60];
			item->SetStringToCell(m_list_hlpr, lbc_locat, PrepareCodeLocBrief(code_loc_buff, new_src_loc, new_line_num));
			m_list_hlpr.InvalidateItem(item);
		}

		// Kill the old selection if any.
		ResetSelection();
		return;
	}

	// New location has different stack of sources nesting. Reset the old contents.
	ResetContents();
	m_list_hlpr.StartBulkChanges();
	m_code_loc = new_src_loc;
	m_code_line_num = new_line_num;

	// Prepare data for the tip line.
	CtvLexSourceEnterEvent curr_line_info;
	curr_line_info.Clear();
	curr_line_info.hdr.parsing_id = new_code_loc.parsing_id;
	curr_line_info.hdr.area = new_code_loc.area;
	ID curr_source_ref = new_src_loc.source_ref;

	// Load this location and all its parents into the listbox.
	while (curr_source_ref != 0)
	{
		// Pick up the call event.
		if (m_db->GetLexSourceEnterEvent(new_code_loc.parsing_id, curr_source_ref) == FALSE)
			break;

		// Add event to the top of listbox. Listbox line describes some territory of the file. Variable curr_line_info contains
		// info about the territory itself, like its linear position.
		if (AddLexSourceItem(curr_line_info, m_db->EventData().src_enter) == FALSE)
			break;

		// Make step up to the beginning of the lex sources stack.
		curr_line_info = m_db->EventData().src_enter;
		curr_source_ref = curr_line_info.source_ref;
	}

	m_list_hlpr.RebuildListBox();
}

void TLangViewLexSourcesNestingDialog::ResetContents()
{
	m_list_hlpr.DeleteAllItems();
	m_code_loc.Clear();
	m_code_line_num = -1;
}

void TLangViewLexSourcesNestingDialog::SetSelection(TLangViewClickEvent &event, bool show_margins)
{
	int num_items = m_list_hlpr.GetNumItems();
	if (num_items <= 0)
		return;

	if (event.type == clet_mid_lev_event)
	{
		// Loop over all non tip items in the listbox and search for the passed event.
		for (int inx=0; inx<num_items-1; ++inx)
		{
			TNestingLocationListItem *item = (TNestingLocationListItem*)m_list_hlpr.GetItemByIndex(inx);
			if (item != NULL && event.mldb_event_data.hdr.Equal(item->m_event.hdr) == TRUE)
			{
				// This is exact match with some non tip listbox element.
				m_list_hlpr.ScrollItemIntoTheView(item, show_margins);
				m_list_hlpr.SelectItem(item);
				return;
			}
		}
	}

	ID parsing_id;
	TLinearArea area;
	if (event.GetLinearArea(parsing_id, area) == TRUE)
	{
		// Look for the linear area intersections among all listbox items.
		for (int inx=0; inx<num_items; ++inx)
		{
			TNestingLocationListItem *item = (TNestingLocationListItem*)m_list_hlpr.GetItemByIndex(inx);
			if (item != NULL && item->m_event.hdr.parsing_id == parsing_id && item->m_event.hdr.area.Contains(area.AreaBeg()) == TRUE)
			{
				// This is a match by the parsing id and the linear area.
				m_list_hlpr.ScrollItemIntoTheView(item, show_margins);
				m_list_hlpr.SelectItem(item);
				return;
			}
		}
	}

	// Passed event is not present in the listbox.
	ResetSelection();
}

bool TLangViewLexSourcesNestingDialog::AddLexSourceItem(CtvLexSourceEnterEvent &event, CtvLexSourceEnterEvent &call_evt)
{
	// Both tip and non tip events should have fixed event type.
	assert(event.hdr.parsing_id != 0 && event.hdr.area.linear_beg > 0);
	assert(event.hdr.event_type == pset_none || event.hdr.event_type == pset_lex_source_enter);

	// Retrieve details about the source of this statement.
	TSourceLocation src_loc;
	TSourceAreaBodyInfo body_info;
	if (m_db->GetSourceLocationInfo(src_loc, event.hdr.parsing_id, event.hdr.area) == FALSE || m_db->GetSourceAreaBodyInfo(body_info, src_loc.src_area) == FALSE)
	{
		// Error is already displayed.
		return(FALSE);
	}

	// Create new listbox item.
	TNestingLocationListItem *item = new TNestingLocationListItem((CtvParsingEventInfo&)event);
	if (item == NULL)
	{
		MsgBoxFmt(MB_OK, L"Error creating TNestingLocationListItem. NestingLevel=%d.", call_evt.new_src_level);
		return(FALSE);
	}

	// Figure out the column index for the file name and enlarge the width of this column.
	int clmn_src = lbc_src_lev0 + __min(call_evt.new_src_level-1, num_level_clmns-1);
	item->SetCellWidth(m_list_hlpr, clmn_src, lbc_src_levmax-clmn_src+1);

	// Format the first two fields of the item.
	wchar_t code_loc_buff[60];
	item->SetFmtStrToCell(m_list_hlpr, lbc_nest,  L"%d.", call_evt.new_src_level);
	item->SetStringToCell(m_list_hlpr, lbc_locat, PrepareCodeLocBrief(code_loc_buff, src_loc, body_info.first_line_num));

	// Prepare the name of the source that contains this event.
	if (call_evt.src_type == srct_macro)
	{
		// Display the name of the macro definition.
		bool call_with_prms = (call_evt.hdr.area.linear_len == call_evt.mcall_name_area.linear_len) ? FALSE : TRUE;
		item->SetFmtStrToCell(m_list_hlpr, clmn_src, L"#define %s%s", call_evt.mcall_def_name, (call_with_prms == TRUE) ? L"(...)" : L"");
	}
	else if (call_evt.src_type == srct_mparam)
	{
		// Display the param inx.
		item->SetFmtStrToCell(m_list_hlpr, clmn_src, L"Macro param #%d", call_evt.mcall_param_inx+1);
	}
	else
	{
		// Display the name the file, that contains the passed event.
		item->SetExtStringToCell(m_list_hlpr, clmn_src, body_info.area.file_info->info.file_name);
	}

	// Add this item to the top of listbox.
	m_list_hlpr.AddItemOnTop(item);
	return(TRUE);
}

// ------------------------------------------------------------------------
//    ============  TLangViewCondParseNestingDialog  =================
// ------------------------------------------------------------------------

static TListBoxColumnProps g_CondParseNesting_ListboxColumnsInfo[] =
{
	{ align_center,	4,	24,	   0,	    0,		},		// Nesting level.
	{ align_left,		8,	14,	   0,	    0,		},		// Code status.
	{ align_left,		6,	68,	   3,	    6,		},		// Location as file_id and line number.
	{ align_left,		0,	12,	   0,	    0,		},		// Level column (to be duplicated).
	{ align_left,		0,	50,	  90,	    4,		},		// Max level (long column).
	LBX_CLMNP_EOF
};

static const wchar_t *g_CondParseNesting_CodeSttNames[cexr_num_types] =
{
	L"Passive", L"Active", L"PD", L"AD", L"Conditional",
};

TLangViewCondParseNestingDialog::TLangViewCondParseNestingDialog(TMidLevStoreReader *db_link)
		: TLangViewChildBrowser(db_link, cbrt_cond_pars_nest, IDD_LANGV_SVF_COND_PARSE_NEST)
{
	m_curr_pars_id = 0;
	m_curr_cond_ref = 0;
	m_root_area_item.SetNotOwned();
}

LRESULT TLangViewCondParseNestingDialog::OnInitDialog(HWND hDefaultFocusControl)
{
	// Prepare for resizing. Title of the listbox is a static message. Listbox does not have titles of its columns.
	AdjustControlRightSide(IDC_LVCPN_TITLE);
	AdjustControlRightSide(IDC_LVCPN_LISTBOX);
	SetupControlPositioning(IDC_LVCPN_TITLE, cpm_fix_top_side);
	SetupControlPositioning(IDC_LVCPN_LISTBOX, cpm_fix_all_corners);

	// Prepare the columns definition script.
	TListBoxColumnProps clmns_info[lbc_num_columns+1];
	clmns_info[lbc_nest] = g_CondParseNesting_ListboxColumnsInfo[lbc_nest];
	clmns_info[lbc_cstt] = g_CondParseNesting_ListboxColumnsInfo[lbc_cstt];
	clmns_info[lbc_loc] = g_CondParseNesting_ListboxColumnsInfo[lbc_loc];
	for (int ic1=0; ic1<num_level_clmns-1; ++ic1)
		clmns_info[lbc_stmt_lev0+ic1] = g_CondParseNesting_ListboxColumnsInfo[lbc_stmt_lev0];
	clmns_info[lbc_stmt_levmax] = g_CondParseNesting_ListboxColumnsInfo[lbc_stmt_lev0+1];
	clmns_info[lbc_stmt_levmax+1] = g_CondParseNesting_ListboxColumnsInfo[lbc_stmt_lev0+2];

	//
	//  Prepare ListBox.
	//

	// Step1. Setup windows.
	m_list_hlpr.SetWindow(::GetDlgItem(m_hDlg, IDC_LVCPN_LISTBOX));

	// Step2. Setup list box props for non auto select mode.
	TGenericListBoxObjectProps props_data;
	props_data.PrepareForSingleSelect(FALSE);
	m_list_hlpr.SetupProps(props_data, FALSE);
	m_list_hlpr.SetupSystemStdStyles(1, 1);
	m_list_hlpr.SetupFrgrStyleSlot(lbclr_root_area, RGB(128, 128, 128), 1, 0, 1);
	m_list_hlpr.SetupFrgrStyleSlot(lbclr_code_body, TAHOMA, 8, FALSE, RGB(0, 0, 0), 1, 0, 1);
	m_list_hlpr.SetupFrgrStyleSlot(lbclr_code_body_slct, TAHOMA, 8, FALSE, RGB(255, 255, 255), 1, 0, 1);
	m_list_hlpr.SetupFrgrStyleSlot(lbclr_pseudo_code_stts, VERDANA, 7, FALSE, RGB(80, 80, 96), 0, -1, 0);
	m_list_hlpr.SetupFrgrStyleSlot(lbclr_pseudo_code_body, SMALL_FONTS, 5, FALSE, RGB(196, 196, 196), 1, -1, 1);

	// Step3. Setup columns in the listbox.
	m_list_hlpr.SetupColumnProps(clmns_info);

	// Step4. Finalize the setup.
	m_list_hlpr.RebuildListBox();

	SetFocusControl(IDC_LVCPN_LISTBOX);
	return(FALSE);
}

void TLangViewCondParseNestingDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	switch (controlId)
	{
		case IDC_LVCPN_LISTBOX:
				{
					// User clicked one of list box items or the area below all items.
					TListBoxNotificationCode ntf_code = (TListBoxNotificationCode)notificationCode;
					if (ntf_code == lbnc_left_btn_down || ntf_code == lbnc_left_btn_dblclk)
					{
						assert(m_owner != NULL);
						TListBoxMouseEventInfo *mouse_info = (TListBoxMouseEventInfo*)hWndControl;
						ResetUserCancel();

						TNestingLocationListItem *item = (TNestingLocationListItem*)(mouse_info->clicked_item);
						if (item != NULL && item != &m_root_area_item && item->m_event.hdr.event_id == 0)
						{
							// This is code item. Use the stmt item instead.
							assert(mouse_info->clicked_item_index > 0);
							item = (TNestingLocationListItem*)m_list_hlpr.GetItemByIndex(mouse_info->clicked_item_index-1);
						}

						if (item == NULL || item == &m_root_area_item || mouse_info->inside_item_rect == FALSE)
						{
							m_owner->RemoveSelection(this);
							break;
						}

						// Prepare the mid lev event data.
						TLangViewClickEvent info;
						info.SetMidLevEvent(item->m_event);

						// Event is prepared. Send it.
						SendForProcessingFromListBox(ntf_code, info);
					}
				}
				break;
	}
}

void TLangViewCondParseNestingDialog::LoadContents(CtvEventHeader &new_code_loc)
{
	assert(m_db != NULL);

	// Retrieve the cond ref ID of the passed area.
	ID pars_id = new_code_loc.parsing_id;
	bool cond_ref_retrieved = TRUE;
	if (pars_id == 0)
	{
		// Passed header does not belong to any parsing context. Clear contents of the viewer and exit.
		ResetContents();
		return;
	}
	else if (new_code_loc.event_id != 0)
	{
		// Passed header describes the parsing event.
		if (m_db->GetParsingEvent(pars_id, new_code_loc.event_id) == FALSE)
		{
			cond_ref_retrieved = FALSE;
		}
		else if (m_db->EventData().hdr.area.linear_beg == 0)
		{
			// Passed parsing id and event id are both valid, but this event does not have any code area.
			// Clear contents of the viewer and exit silently.
			ResetContents();
			return;
		}
		else
		{
			cond_ref_retrieved = m_db->GetCondRefEvent(m_db->EventData());
		}
	}
	else if (new_code_loc.area.linear_beg == 0)
	{
		// The event id and the linear offset are both zero while the parsing session is not zero.
		// Clear contents of the viewer and exit silently.
		ResetContents();
		return;
	}
	else
	{
		cond_ref_retrieved = m_db->GetCondRefEvent(pars_id, new_code_loc.area.linear_beg);
	}

	// Check results of retrieving.
	if (cond_ref_retrieved == FALSE)
	{
		// Passed parameter is bogus in this or that way.
		MsgBox(L"Error determining the cond parsing location for the parsing event header.");
		ResetContents();
		return;
	}

	if (pars_id == m_curr_pars_id && m_db->EventData().hdr.event_id == m_curr_cond_ref && m_list_hlpr.GetNumItems() > 0)
	{
		// Current contents of the viewer has the same cond parsing reference as the new area.
		// Do not reset the contents. Only kill the old selection if any.
		ResetSelection();
		return;
	}

	ResetContents();
	m_curr_pars_id = pars_id;
	m_curr_cond_ref = m_db->EventData().hdr.event_id;
	m_list_hlpr.StartBulkChanges();

	if (m_curr_cond_ref == 0)
	{
		// Current location is not inside any conditional area.
		m_root_area_item.SetCellWidth(m_list_hlpr, lbc_cstt, lbc_num_columns-1);
		m_root_area_item.SetExtStringToCell(m_list_hlpr, lbc_cstt, L"=+= Root Area =+=");
		m_root_area_item.SetFrgrStyleToCell(m_list_hlpr, lbc_cstt, lbclr_root_area, lbst_item_selected);
		m_list_hlpr.AddItem(&m_root_area_item);
	}
	else
	{
		// Passed area resides in the non empty cond parsing context. Load the cond parsing stack.
		TMidLevStoreReader *sec_store = NULL;
		for(;;)
		{
			CtvCondParsingChangeEvent cpr_evt = m_db->EventData().cond_pars;

			// Add info about the current code area to the top of the listbox.
			if (AddCodeLocItem(m_db, cpr_evt.new_code_level, cpr_evt.new_code_stt) == FALSE)
				break;

			// Add current event to the top of the listbox.
			if (AddCondParseItem(m_db, cpr_evt) == FALSE)
				break;

			if (cpr_evt.stmt_type == mpr_elif || cpr_evt.stmt_type == mpr_else)
			{
				// This event is not starting the nesting structure. Look for the upper statements from the current nesting.
				if (sec_store == NULL)
				{
					sec_store = m_db->CreateSecondaryStore();
					if (sec_store == NULL)
					{
						MsgBox(L"Unable to allocate secondary mid lev events store reader.");
						break;
					}
				}

				// Ask db to do iteration in the descending order.
				if (m_db->StartCondParsChgCondRefsIteration(pars_id, cpr_evt.cond_ref, cpr_evt.cond_ref, cpr_evt.prev_stmt_id, FALSE) == TRUE)
				{
					while (m_db->NextCondParsChgCondRefEvent() == TRUE)
					{
						if (AddCondParseItem(sec_store, m_db->EventData().cond_pars) == FALSE)
							break;

						if (m_db->EventData().cond_pars.stmt_type <= mpr_if)
						{
							// Iteration came to the beginning of the curr nesting. It is necessary to break the loop
							// now to avoid showing other #ifdef-#else-#endif structures that may be present in
							// the current nesting.
							break;
						}
					}

					m_db->CloseCondParsChgCondRefsIteration();
				}
			}

			// Pick up the cond pars event from the upper layer.
			if (cpr_evt.cond_ref == 0 || m_db->GetCondParsingChangeEvent(pars_id, cpr_evt.cond_ref) == FALSE)
				break;
		}

		if (sec_store != NULL)
			m_db->ReleaseSecondaryStore(sec_store);
	}

	// Finally show all changes that happened in listbox.
	m_list_hlpr.RebuildListBox();
}

void TLangViewCondParseNestingDialog::ResetContents()
{
	m_list_hlpr.DeleteAllItems();
	m_curr_pars_id = 0;
	m_curr_cond_ref = 0;
}

void TLangViewCondParseNestingDialog::SetSelection(TLangViewClickEvent &event, bool show_margins)
{
	if (m_curr_pars_id == 0 || m_curr_cond_ref == 0)
	{
		ResetSelection();
		return;
	}

	// Current cond pars context is not empty.
	int num_items = m_list_hlpr.GetNumItems();
	assert(num_items > 0);

	if (event.type == clet_mid_lev_event)
	{
		// Loop over the listbox items and check for the passed event.
		for (int inx=0; inx<num_items; ++inx)
		{
			TNestingLocationListItem *item = (TNestingLocationListItem*)m_list_hlpr.GetItemByIndex(inx);
			if (item != NULL && event.mldb_event_data.hdr.Equal(item->m_event.hdr) == TRUE)
			{
				m_list_hlpr.ScrollItemIntoTheView(item, show_margins);
				m_list_hlpr.SelectItem(item);
				return;
			}
		}
	}

	ID parsing_id;
	TLinearArea area;
	if (event.GetLinearArea(parsing_id, area) == TRUE && parsing_id != 0 && area.linear_beg > 0)
	{
		// Look for the linear area intersections among all listbox items.
		for (int inx=0; inx<num_items; ++inx)
		{
			TNestingLocationListItem *item = (TNestingLocationListItem*)m_list_hlpr.GetItemByIndex(inx);
			if (item != NULL && item->m_event.hdr.parsing_id == parsing_id && item->m_event.hdr.area.Contains(area.AreaBeg()) == TRUE)
			{
				// Match by the parsing id and the linear area.
				m_list_hlpr.ScrollItemIntoTheView(item, show_margins);
				m_list_hlpr.SelectItem(item);
				return;
			}
		}
	}

	// Passed event is not present in the listbox.
	ResetSelection();
}

bool TLangViewCondParseNestingDialog::AddCodeLocItem(TMidLevStoreReader *sec_store, int code_level, TConditionalStatus code_stts)
{
	// Create the new listbox item.
	TNestingLocationListItem *item = new TNestingLocationListItem();
	if (item == NULL)
	{
		MsgBoxFmt(MB_OK, L"Error creating TNestingLocationListItem. CodeLevel=%d.", code_level);
		return(FALSE);
	}

	// Figure out the column index for displaying the code pseudo body.
	int clmn_stmt = lbc_stmt_lev0 + __min(code_level, num_level_clmns-1);
	item->SetCellWidth(m_list_hlpr, clmn_stmt, lbc_stmt_levmax-clmn_stmt+1);

	// Format the cells.
	item->SetCellWidth(m_list_hlpr, lbc_cstt, 2);
	item->SetExtStringToCell(m_list_hlpr, lbc_cstt, g_CondParseNesting_CodeSttNames[code_stts]);
	item->SetExtStringToCell(m_list_hlpr, clmn_stmt, L"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||");

	// Normal and selected styles should be the same otherwise default selected style will make the item
	// higher because of the size of its stnd font.
	item->SetFrgrStyleToCell(m_list_hlpr, lbc_cstt, lbclr_pseudo_code_stts, lbclr_pseudo_code_stts);
	item->SetFrgrStyleToCell(m_list_hlpr, clmn_stmt, lbclr_pseudo_code_body, lbclr_pseudo_code_body);

	// Add item to the top of the listbox.
	m_list_hlpr.AddItemOnTop(item);
	return(TRUE);
}

bool TLangViewCondParseNestingDialog::AddCondParseItem(TMidLevStoreReader *sec_store, CtvCondParsingChangeEvent &event)
{
	// Retrieve details about the source of this event.
	TSourceLocation src_loc;
	TSourceAreaBodyInfo body_info;
	if (sec_store->GetSourceLocationInfo(src_loc, event.hdr.parsing_id, event.hdr.area) == FALSE || sec_store->GetSourceAreaBodyInfo(body_info, src_loc.src_area) == FALSE)
	{
		// Error is already displayed.
		return(FALSE);
	}

	// Create the new listbox item.
	TNestingLocationListItem *item = new TNestingLocationListItem((CtvParsingEventInfo&)event);
	if (item == NULL)
	{
		MsgBoxFmt(MB_OK, L"Error creating TNestingLocationListItem. CondParsLevel=%d.", event.stmt_code_level);
		return(FALSE);
	}

	// Figure out the column index for displaying the statement body.
	int clmn_stmt = lbc_stmt_lev0 + __min(event.stmt_code_level, num_level_clmns-1);
	item->SetCellWidth(m_list_hlpr, clmn_stmt, lbc_stmt_levmax-clmn_stmt+1);

	// Format fields of the item.
	wchar_t code_loc_buff[60];
	item->SetFmtStrToCell(m_list_hlpr, lbc_nest, L"%d.", event.stmt_code_level+1);
	item->SetFmtStrToCell(m_list_hlpr, lbc_loc, PrepareCodeLocBrief(code_loc_buff, src_loc, body_info.first_line_num));

	// Check if the code line contains unprintable characters or not.
	bool nonpr_chars = FALSE;
	wchar_t loc_code_line[80];
	for (int ich=0; ich<body_info.fline_body_len; ++ich)
	{
		wchar_t ch = body_info.fline_body_ptr[ich];
		if (ch < L' ')
		{
			nonpr_chars = TRUE;
			ch = L' ';
		}

		if (ich < 80)
			loc_code_line[ich] = ch;
	}

	// Fill in either the original line or the patched copy.
	item->SetFrgrStyleToCell(m_list_hlpr, clmn_stmt, lbclr_code_body, lbclr_code_body_slct);
	if (nonpr_chars == FALSE)
		item->SetExtStringToCell(m_list_hlpr, clmn_stmt, body_info.fline_body_ptr, body_info.fline_body_len);
	else item->SetStringToCell(m_list_hlpr, clmn_stmt, loc_code_line, (body_info.fline_body_len < 80) ? body_info.fline_body_len : 80);

	// Add item to the top of the listbox.
	m_list_hlpr.AddItemOnTop(item);
	return(TRUE);
}


