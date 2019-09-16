//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_ChildBrowsers_H
#define	LangView_ChildBrowsers_H

#ifndef   Common_LookAsideTable_H
#include  "Common/LookAsideTable.H"
#endif
#ifndef   WinUi_ListBoxObject_H
#include  "WinUi/ListBoxObject.H"
#endif
#ifndef   TextScan_MidStore_H
#include  "TextScan/MidStore.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif

//
// Parsing events ListBox has 6 columns but there are only 3 different sort orders. When  listbox is using
// listbox table mode (typical situation) then only 2 sort orders are possible (pars_id-event_id and type-descr).
//
enum TParsingEventsSortOrder
{
	peso_pid_evid,			// Default: (pars_id, event_id).
	peso_type_descr,		// Sort by: (type, description) -> (pars_id, event_id).
	peso_file_line,			// Sort by: (file_name, line_num) -> (pars_id, event_id).
							// Note that short names of the files are compared.
	peso_num_types,		// Unsorted.
};

//
//  This object may represent either the MidLev event or the code location during some parsing process.
//  In the second case event_id field should be set to appropriate "fake" value. This "fake event_id" will not
//  be displayed but it will be used for sorting.
//
class TParsingEventListItem : public TBasicListBoxItem
{
public:

	TParsingEventListItem() { m_event.Clear(); m_file_offs = -1; }
	TParsingEventListItem(CtvParsingEventInfo &evt_data) { SetEventData(evt_data); }

	void		SetEventData(CtvParsingEventInfo &evt_data) { m_event = evt_data; m_file_offs = -1; }
	bool		FormatItem(TBasicListBoxObject &owner, TMidLevStoreReader *db_link, TMidLevScanner *active_scanner = NULL);
				// Scanner should be passed only when parsing event belongs to the unfinished parsing session.

protected:

	CtvParsingEventInfo		m_event;

	long						m_file_offs;			// This field gets its value when the listbox item is formatted.
												// File offset is used only in CompareByFileLine() method.

	static int  __cdecl	CompareByPidEvid(void *ctx, const TBasicListBoxItem **p1, const TBasicListBoxItem **p2);
	static int  __cdecl	CompareByTypeDesc(void *ctx, const TBasicListBoxItem **p1, const TBasicListBoxItem **p2);
	static int  __cdecl	CompareByFileLine(void *ctx, const TBasicListBoxItem **p1, const TBasicListBoxItem **p2);
						// These comparison functions are used only when parsing events list dialog is not using
						// the listbox table. Both listbox items should be formatted. Comparison functions are not using
						// linear offsets.

	friend class TLangViewParsingEventsListDialog;
};

class TParsingEventsListBoxTable : public TGenericListBoxTable<TParsingEventListItem>
{
public:

	TParsingEventsListBoxTable(TMidLevStoreReader *db);

	void			SetOwners(TBasicDialog *cbk, TBasicListBoxObject *fmt_owner) { m_err_callback = cbk; m_fmt_owner = fmt_owner; }
	void			SetDataBase(TMidLevStoreReader *db_link) { m_db_link = db_link; }

	int			NumItems() { return(m_data.NumItems()); }
	int			GetCurrentPosition() { return(m_pos); }
	void			Seek(int inx);

	TParsingEventListItem		*ReadItem(bool read_forward = TRUE);
	void						RecycleItem(TParsingEventListItem *item) { m_look_aside_data.RecycleItem(item); }

	void			ResetContents() { m_data.SetNumItems(0); m_pos = 0; m_look_aside_data.ClearTable(); m_foreign_obj_inx = -1; }
	bool			LoadParsingEvents(ID pars_id, PSET_TYPES_MASK evt_mask, ID min_evid = 0, ID max_evid = 0);

	CtvParsingEventInfo		*GetEvent(int inx) { return(m_data[inx]); }
	bool						IsForeignObjectPtr(CtvParsingEventInfo *ptr) const { return(ptr == &m_foreign_obj_info); }

	int			FindEvent(CtvParsingEventInfo &event_info);
					// For "code_area" argument the return value is always -1. This is done so because more complex
					// equality comparison is needed to support this while the current application layer does not needed it.
					// Method is doing real search only for real mid_lev events using the binary search.

	//
	//  Only one foreign object can be present at a time. It can be either the mid lev event or the code location.
	//

	int			GetForeignObjectInx() { return(m_foreign_obj_inx); }
	int			AddForeignObject(CtvParsingEventInfo &event_info);
					// Return value from these methods is either the index of the foreign object in the table
					// or -1 in case of failure.

	bool			RemoveForeignObject();

	int			SortItems(bool type_descr_order);
					// Return value is new value of the foreign object index after resorting the table.

private:

	int		BinSearchInx(CtvParsingEventInfo *ptr);
	int		CompareWithTableItem(const CtvParsingEventInfo *ptr_event1, int inx_event2);
				// This binary search method returns the hi_bad index, ie: m_data[hi_bad-1] <= ptr < m_data[hi_bad].
				// Return value is in the range 0...NumItems(). Comparison function is used only for binary search.
protected:

	CtvParsingEventPtrsArray			m_data;
	int								m_pos;

	typedef  TLookAsideTable<TParsingEventListItem, 100>  TListBoxItemsLookAsideTable;

	TListBoxItemsLookAsideTable		m_look_aside_data;

	bool								m_type_descr_order;
										// This field stores the current sort order. Items in the table are always sorted.

	int								m_foreign_obj_inx;
										// NB: When m_foreign_obj_inx is -1, this means that table does not contain the foreign object.

	CtvParsingEventInfo				m_foreign_obj_info;
										// When foreign object is code area, its event_id field is assigned based on the linear offset
										// comparisons. Assigned event_id value is event_id of one of the existing events in the table
										// if there is an event where table_event.linear_beg >= foreign_code_area.linear_beg.

	TBasicDialog						*m_err_callback;
	TBasicListBoxObject				*m_fmt_owner;
	TMidLevStoreReader				*m_db_link;					// Link to the database object is needed for loading events plus it is used for
																// picking up the file name and line number for formatting the list box items.
};

//
//  Template: IDD_LANGV_SVF_PARS_EVENTS_LIST
//
class TLangViewParsingEventsListDialog : public TLangViewChildBrowser
{
public:

	TLangViewParsingEventsListDialog(TMidLevStoreReader *db_link, bool lb_table_mode, bool allow_resorting = FALSE);
		//
		//  When this dialog is in lb_table mode, its list is always sorted and there are only 2 possible sort orders:
		//
		//			pid-edid and type-descr.
		//
		//  When dialog is in local items list mode (second param of the ctor is FALSE), it can be either in unsorted
		//  state (right after creation) or in sorted state (after the first call to SetSortOrder() method).
		//

	~TLangViewParsingEventsListDialog() { m_list_hlpr.ResetListBox(); }

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			SetDataBase(TMidLevStoreReader *db_link) { ResetContents(); m_lb_table.SetDataBase(db_link); TLangViewChildBrowser::SetDataBase(db_link); }

	void			LoadParsingEvents(ID pars_id, PSET_TYPES_MASK event_types_mask, ID min_evid = 0, ID max_evid = 0);
					// Method can be called only in lb_table mode. This method discards prev contents of the table completely.

	bool			AddParsingEvent(CtvParsingEventInfo &event);
					// This method can be called only when the dialog is in the local items list mode. Local items list mode
					// is set when the dialog is created and this mode cannot be changed. Adding new events is allowed only
					// when the local list is not sorted.

	//
	// Inherited virtual methods.
	//

	virtual void	ResetContents();

	virtual void	SetSelection(TLangViewClickEvent &event, bool show_margins);
					//
					// This child browser can contain only clet_mid_lev_event events as its own items.
					// What can be selected depends on the mode and the curr state of the viewer:
					//
					//	lb_table:		Event should be either the mid lev event or event that has code area and
					//				belongs to some parsing. In this case it will be temp added as the "code area"
					//				object.
					//
					//	local_list:		Event should be only the mid_lev_object. All other event types are ignored.
					//				Behavior depends on the state of sorting:
					//
					//					unsorted:		only exact matches are allowed.
					//					sorted:		passed event can be added as foreign event.
					//

	virtual void	ResetSelection();

	virtual void	SetSortOrder(int list_id, DWORD new_sort_order);
					// Note that once the order was changed from unsorted to sorted by calling this method,
					// it cannot be changed back to unsorted.

	// Enum values are the listbox column indexes. This enum should be public because it is used
	// for defining static arrays that are not defined as members of this dialog.
	enum LbColumns
	{
		lbc_evntid,	lbc_evtype,				// 0, 1.
		lbc_fname,	lbc_lnum,	lbc_chpos,	// 2, 3, 4.
		lbc_descr,							// 5.
		lbc_num_clmns
	};

protected:

	enum LbFrgrStyles { frgr_code_loc_normal = lbst_app_indexes_base, frgr_code_loc_slct = lbst_app_indexes_base+1, };
	enum LbBkgrStyles { bkgr_code_loc_normal = lbst_app_indexes_base, bkgr_code_loc_slct = lbst_app_indexes_base+1, };
			// Define app specific style indexes right after the stnd styles that are present in all listobxes.

	void		CollectEventTypeStatistics(int *event_type_counts, int num_etp_counts, int *errwarn_class_counts, int num_ewc_counts, int *other_events);
	void		SummarizeEvent(CtvParsingEventInfo *event, int *event_type_counts, int num_etp_counts, int *errwarn_class_counts, int num_ewc_counts, int *other_events);
	void		UpdateNumItemsLabel();

	int		CompareParsingEventListItems(const TParsingEventListItem *p1, const TParsingEventListItem*p2);
				// This method is used only in the linked list mode to find appropriate place for adding foreign
				// parsing event. Note that in the linked list mode the foreign object cannot be a code location
				// object.
protected:

	TListBoxObject				m_list_hlpr;
									// To figure out if the dialog is in the list box table mode or not, it is necessary
									// to call the m_list_hlpr.GetLbTableMode().

	TParsingEventsListBoxTable		m_lb_table;
	bool							m_allow_resorting;
	TParsingEventsSortOrder		m_sort_order;

	TParsingEventListItem			m_temp_slct_item;
									// This item is used when it is necessary to show the "foreign" selected event
									// or the code area place. This item is used ONLY in local items list mode.

	friend class TParsingEventListItem;
};

//
//  This list box item class is used for lex sources nesting and cond parsing nesting child viewers.
//
class TNestingLocationListItem : public TBasicListBoxItem
{
public:

	TNestingLocationListItem() { m_event.Clear(); }
	TNestingLocationListItem(CtvParsingEventInfo &evt) { m_event = evt; }

public:

	CtvParsingEventInfo		m_event;
								//
								//  This event can contain:
								//
								//		pset_none				--	Tip element in the lex sources nesting dialog or the code areas
								//									in the cond parsing nesting dialog.
								//		pset_lex_source_enter		--	Any non tip element in the list box of the sources nesting dialog.
								//		pset_cond_parsing_change	--	Any element in the list box of the cond parse nesting when the cond
								//									parsing is not empty.
								//
};

//
//  Template: IDD_LANGV_SVF_LEX_SOURCE_NEST
//
class TLangViewLexSourcesNestingDialog : public TLangViewChildBrowser
{
public:

	TLangViewLexSourcesNestingDialog(TMidLevStoreReader *db_link);

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			LoadContents(CtvEventHeader &new_code_loc);
					// Only the parsing_id and beginning of the linear area are important in the passed structure.

	virtual void	ResetContents();

	virtual void	SetSelection(TLangViewClickEvent &event, bool show_margins);
					// Primary purpose of selection in this child browser is playing back and forth the browser history.
					// This browser can accept the code area event (for tip element) and the LexSourceEnter events
					// (for any non tip element).

	virtual void	ResetSelection() { m_list_hlpr.RemoveSelection(); }

protected:

	enum { num_level_clmns = 16 };

	enum LbColumns { lbc_nest, lbc_locat, lbc_src_lev0, lbc_src_levmax = lbc_src_lev0+num_level_clmns-1, lbc_num_columns = lbc_src_levmax+1 };
				// Enum values are indexes of the colums in the listbox. Column with the index lbc_srcn_levmax
				// is a real column. This column is a wide column that shows the major part of the name.

	bool		AddLexSourceItem(CtvLexSourceEnterEvent &event, CtvLexSourceEnterEvent &call_evt);
				// First param can only be either pset_none (for tip list box item) or lex src enter (for all non tip items).
				// Item is always placed on the top of the list box.

	TListBoxObject				m_list_hlpr;				// List box is in non auto select mode. It just sends the mouse
														// button down notifications to its owner.

	TSourceLocation				m_code_loc;				// This field is not emty when the dialog is not empty.
	long							m_code_line_num;		// Line number of the beginning of the current code area.
};

//
//  Template: IDD_LANGV_SVF_COND_PARSE_NEST
//
class TLangViewCondParseNestingDialog : public TLangViewChildBrowser
{
public:

	TLangViewCondParseNestingDialog(TMidLevStoreReader *db_link);

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void			LoadContents(CtvEventHeader &new_code_loc);
					// Only the parsing_id, source_ref, cond_ref and event_area are important in this structure.

	virtual void	ResetContents();

	virtual void	SetSelection(TLangViewClickEvent &event, bool show_margins);
					// This browser can accept code area event (for tip element) and the CondParsingChange events
					// (for any non tip element).

	virtual void	ResetSelection() { m_list_hlpr.RemoveSelection(); }

protected:

	bool			AddCodeLocItem(TMidLevStoreReader *sec_store, int code_level, TConditionalStatus code_stts);
	bool			AddCondParseItem(TMidLevStoreReader *sec_store, CtvCondParsingChangeEvent &event);
					// Items are always placed on the top of the list box.

	enum LbFrgrColorStyleInx { lbclr_root_area = lbst_app_indexes_base, lbclr_code_body, lbclr_code_body_slct, lbclr_pseudo_code_stts, lbclr_pseudo_code_body };

	enum { num_level_clmns = 16 };

	enum LbColumns { lbc_nest, lbc_cstt, lbc_loc, lbc_stmt_lev0, lbc_stmt_levmax = lbc_stmt_lev0+num_level_clmns-1, lbc_num_columns = lbc_stmt_levmax+1  };
			// Enum values are indexes of the colums in the listbox. Column with the index lbc_srcn_levmax
			// is a real column. This column is a wide column that keeps major part of the code line.

	TListBoxObject				m_list_hlpr;				// This list is in non auto slct mode. It sends the mouse button down notifications
														// to its owner.
	ID							m_curr_pars_id;
	ID							m_curr_cond_ref;

	TNestingLocationListItem		m_root_area_item;		// This item is used only for display purposes. Its app level fields are all the time
														// empty. Root area item is not clickable.
};

#endif	// LangView_ChildBrowsers_H


