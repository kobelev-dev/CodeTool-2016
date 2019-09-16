//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_CppDefinitionsView_H
#define	LangView_CppDefinitionsView_H

#ifndef   Common_LookAsideTable_H
#include  "Common/LookAsideTable.H"
#endif
#ifndef   TextScan_CppDatabase_H
#include  "TextScan/CppDatabase.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif

class TLangViewCppItemListBoxItem : public TBasicListBoxItem
{
public:

	TLangViewCppItemListBoxItem(TCppItemBase *item = NULL) { m_cpp_item = item; }

	enum { num_name_clmns = 12 };

	void		SetupItem(TCppItemBase *item) { m_cpp_item = item; }
	void		FormatItem(TBasicListBoxObject &owner, int visible_level_delta);

	TCppItemBase	*GetCppItem() { return(m_cpp_item); }

protected:

	int		TypeClmnInx()		{ return(num_name_clmns); }
	int		DtlsClmnInx()		{ return(num_name_clmns+1); }
	int		IdentClmnInx()	{ return(num_name_clmns+2); }

	const  wchar_t		*GetNameColumn(wchar_t *buffer, int buffer_len);
	const  wchar_t		*GetTypeColumn(wchar_t *buffer, int buffer_len);
	const  wchar_t		*GetDetailsColumn(wchar_t *buffer, int buffer_len);

	const wchar_t		*StructItemTypeName(wchar_t *buffer, int buffer_len, const wchar_t *label_format);
	const wchar_t		*ExprBaseItemDetails(wchar_t *buffer, int buffer_len, const TCppExpressionBase *expr);

protected:

	TCppItemBase	*m_cpp_item;
};

class TLangViewCppItemsListBoxTable : public TGenericListBoxTable<TLangViewCppItemListBoxItem>
{
public:

	TLangViewCppItemsListBoxTable() { m_owner = NULL; m_fmt_owner = NULL; m_pos = 0; m_visible_level_delta = 0; }

	void		SetOwners(TBasicDialog *dlg_owner, TBasicListBoxObject *fmt_owner) { m_owner = dlg_owner; m_fmt_owner = fmt_owner; }

	int		NumItems() { return(m_data.NumItems()); }
	int		GetCurrentPosition() { return(m_pos); }
	void		Seek(int inx);

	TLangViewCppItemListBoxItem	*ReadItem(bool read_forward = TRUE);
	void							RecycleItem(TLangViewCppItemListBoxItem *item) { m_lookaside_data.RecycleItem(item); }
									// Note that this is single selection listbox table. This means that table does not know anything about
									// the current selection. The method ReadItem() always returns unselected object. List box code will
									// set the selection flag if this is needed.

	TCppItemBase	*GetCppItem(int inx) { return(m_data[inx]); }

	void		ResetTableContents() { m_data.SetNumItems(0); m_pos = 0; m_lookaside_data.ClearTable(); }

	bool		LoadDefinitions(TCppItemBase &items_parent, int child_items_nest_level, bool natural_sort_order);
				// Second param shows the nesting level of the children of the passed item. For example members
				// of the global namespace have a nesting level 0. The root of the passed tree is NOT displayed.

	int		FindCppItemIndexById(ID cpp_item_id) const;
				// Return value is -1 if a cpp item with the passed id is not present in the table.

protected:

	int		ProcessDefnsSubtree(int ins_before, TCppItemBase &items_parent, bool natural_sort_order);
				// Return value is -1 in the case of failure.

	TBasicDialog					*m_owner;				// The owner is needed only for displaying the error message boxes.
	TBasicListBoxObject			*m_fmt_owner;

	TCppItemPtrsArray				m_data;
	int							m_pos;

	int							m_visible_level_delta;

	typedef  TLookAsideTable<TLangViewCppItemListBoxItem, 100>  TListBoxItemsLookAsideTable;

	TListBoxItemsLookAsideTable	m_lookaside_data;
};

//
//  Template: IDD_LANGV_CPPV_GLOBAL_NAMESP
//
class TLangViewCppDefnsSpaceViewer : public TLangViewChildBrowser
{
public:

	TLangViewCppDefnsSpaceViewer();

	LRESULT		OnInitDialog(HWND hDefFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	// Inherited virtual methods.
	virtual void	ResetContents();
	virtual void	SetSelection(TLangViewClickEvent &event, bool ext_margins);
	virtual void	ResetSelection();

	// New methods.
	void			LoadCppItems(TCppItemBase &items_parent, int child_items_nest_level = 0);

	TCppItemBase	*GetSelectedCppItem();
						// The retrun value is NULL if there is no selection in the listbox.
protected:

	TListBoxObject					m_list_hlp;
	TLangViewCppItemsListBoxTable		m_cpp_items_table;
	int								m_latest_upper_item_inx;
										// The list box is always in the listbox table mode.
};

#endif	// LangView_CppDefinitionsView_H


