//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   The List Box Custom Control.
//

#ifndef	WinUi_ListBoxObject_H
#define	WinUi_ListBoxObject_H

#ifndef   Common_StringPointer_H
#include  "Common/StringPointer.H"
#endif
#ifndef   ScreenItems_ScreenItems_H
#include  "ScreenItems/ScreenItems.H"
#endif
#ifndef   WinUi_ListBoxHelper_H
#include  "WinUi/ListBoxHelper.H"
#endif

// The names of the custom controls. These names should be used in the dialog templates.
#define	DLG_CTRL_LIST_BOX_TITLE	L"DLG_CONTROL_LIST_BOX_TITLE"
#define	DLG_CTRL_LIST_BOX_BODY	L"DLG_CONTROL_LIST_BOX_BODY"

class TBasicListBoxObject;

enum TBasicListBoxCellFlags :  unsigned char
{
	lbcf_empty,				// The cell is empty.
	lbcf_local_text,			// Simple local text.
	lbcf_local_script,			// Scripted local text.
	lbcf_extern_text,			// Simple external text.
	lbcf_extern_script,			// Scripted externall text.
};

struct TBasicListBoxCellInfo
{
	TBasicListBoxCellFlags		cell_flags;
	BYTE					cell_width;

	BYTE					norm_text_style_inx;		// Style index for the non selected item.
	BYTE					slct_text_style_inx;			// Style index for the SELECTED item.

	TStringPtr				text_data;
};

class TBasicListBoxItem : public TGenericListBoxItem<TBasicListBoxObject>
{
public:

	TBasicListBoxItem(BYTE ident = 0);

	//
	//  Implementation of the inherited virtual methods.
	//

	virtual	int		GetCellWidth(int iclmn) const { return(m_cells_info[iclmn].cell_width); }
	virtual	void		SetCellWidth(TListBoxObjectCore &owner, int iclmn, int width_in_columns);
						// Passed width of the cell should be positive number: value one means the simple cell.

	virtual	void		PaintItem(HDC hDC, TBasicListBoxObject *ptr_owner, RECT &item_rect);
	virtual	int		MeasureItemHeight(TBasicListBoxObject *ptr_owner);
						//
						//   Height of the item is:  max(HeightAbove-of-all_used_fonts) + max(HeightBelow-of-all_used_fonts).
						//
						//   When all cells of the item are empty, height of the item is still not zero and it is based on
						//   the height of the the slot [0] font. If at least one cell is not empty, then only non empty cells
						//   are used for calculating the height of the item.
						//
						//   Height of the item is not changing when item is changing its selection state.
						//
	//
	//  New methods.
	//

	void		ClearCell(TBasicListBoxObject &owner, int iclmn);
	void		ClearBkgrAndCells(TBasicListBoxObject &owner);

	void		SetBkgrStyle(TBasicListBoxObject &owner, BYTE normal_style_inx, BYTE selected_style_inx);
	void		SetFrgrStyleToCell(TBasicListBoxObject &owner, int iclmn, BYTE normal_inx, BYTE selected_inx);

	bool		SetStringToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *string, int len = -1);
	bool		SetFmtStrToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *format, ...);
	bool		SetScriptedStringToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *string, int len = -1);

	void		SetExtStringToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *string, int len = -1);
	void		SetExtScriptedStringToCell(TBasicListBoxObject &owner, int iclmn, const wchar_t *string, int len = -1);

	//
	//  Possible elements of the scripted text:
	//
	//		1. Char @ escape:		@@
	//		2. Icon object:		@i107,i109; @I405,I407; @I200;
	//		3. New text style:		@S2,S3; @S3; @s4;
	//		4. Reset text style:		@RS; @rs;
	//
	//  Icons are aligned to the top of the list box item rect.
	//

	enum TScriptedTextCtrlType
	{
		ic_none, ic_draw_icon, ic_change_style, ic_reset_style,
	};

	TStringPtr	&GetStrPtr(int iclmn) { return(m_cells_info[iclmn].text_data); }

protected:

	BYTE					m_norm_bkgr_style_inx;
	BYTE					m_slct_bkgr_style_inx;
								// Background style is common for all columns.

	WORD					m_baseline_offs;	// Offset to the baseline from the top of the item.

	TBasicListBoxCellInfo		m_cells_info[TListBoxHorzHelper::LBX_MAX_COLUMNS];

	TTextBuffer80				m_local_buffer;
								// Note that this buffer contains several non NULL terminated strings. Plus there is no NULL character
								// at the end of the buffer.
private:

	void		ClearFieldsInternal(int num_columns);
	void		CheckUpdateHeight(TBasicListBoxObject &owner);
	bool		SetLocalStringInternal(TBasicListBoxObject &owner, int iclmn, TBasicListBoxCellFlags cf, const wchar_t *string, int len);

	bool			IsLocalStringCell(int iclmn);
	wchar_t		*ReserveLocalSpace(TBasicListBoxObject &owner, int len_requested, int &len_allocated);
	void			RemoveLocalString(TBasicListBoxObject &owner, int iclmn);

	void					CheckStyleExts(TBasicListBoxObject *ptr_owner, int style_inx, int &max_above, int &max_below);
	bool					ScanShortInt(const wchar_t *src_str, int src_len, int &fragm_len, short &result);
	TScriptedTextCtrlType	ScanScriptedTextCtrlInfo(const wchar_t *src_str, int src_len, int &fragm_len, short &norm_value, short &slct_value);
	long					ProcessScriptedText(HDC hDC, TBasicListBoxObject *ptr_owner, bool mode_paint, RECT &rct, int initial_frgr_style_inx,
												bool selected_state, TStringPtr &text_ptr);

};

typedef  TGenericListBoxTable<TBasicListBoxItem>  TBasicListBoxTable;

//
// Notifications are sent as the WM_COMMAND events. Event code is sent in the loword of WPARAM.
//
enum TListBoxNotificationCode
{
	lbnc_none,
	lbnc_left_btn_down,			// lParam - TListBoxMouseEventInfo*
	lbnc_right_btn_down,		// lParam - TListBoxMouseEventInfo*
	lbnc_left_btn_dblclk,			// lParam - TListBoxMouseEventInfo*
	lbnc_wm_char,				// lParam - TListBoxKeyboardEventInfo*
	lbnc_slct_change,			// lParam - TListBoxSlctChangeInfo*
};

//
// Mouse event cannot bring info about the click on the frame or scroller. Item index is always
// in the range 0...num_items-1. It is always 0 when the list is empty. Item field can be NULL
// only in case of list box table reading error or when the listbox title is clicked.
//
struct TListBoxMouseEventInfo
{
	bool						main_list_event;					// FALSE - list box title, TRUE - list box body.
	TListBoxNotificationCode	event_code;
	TListBoxObjectCore		*list_box_inst;

	DWORD					vKey;
	long						pos_x, pos_y;					// Custom control client coordinates.

	TListBoxItemCore			*clicked_item;					// Area of this item can be visible only partly.
	int						clicked_item_index;
	bool						inside_item_rect;

	int						clmn_index;
	bool						inside_the_column;
};

struct TListBoxKeyboardEventInfo
{
	TListBoxNotificationCode	event_code;
	TListBoxObjectCore		*list_box_inst;

	WPARAM					wParam;
	LPARAM					lParam;
};

struct TListBoxSelectionChangeInfo
{
	TListBoxNotificationCode	event_code;
	TListBoxObjectCore		*list_box_inst;

	bool						new_selection;
	TListBoxItemCore			*clicked_item;
	int						clicked_item_inx;
								// In the single selection listboxes the clicked item is all the time selected.
								// In multi select objects ckicked item can gain and loose selection.

	int						slct_chg_beg;
	int						slct_chg_len;
								// This is range of indexes that contains all items that became selected
								// or deselected as result of this change.
};

enum TListBoxObjectStdStyleIndexes
{
	lbst_item_normal,
	lbst_item_selected,
	lbst_title_item,
	lbst_app_indexes_base,
};

struct TListBoxColumnTilteProps
{
	short					title_clmn_width;				// Record with zero column width marks the end of array.
	bool						title_scripted_text;
	wchar_t					*title_clmn_text;					// The text is expected to be a static string.
};

class TBasicListBoxObject : public TGenericListBoxObject<TBasicListBoxItem, TBasicListBoxObject>
{
public:

			TBasicListBoxObject();
			~TBasicListBoxObject();

	void		SetWindow(HWND hListBodyControl);
	void		SetTitleWindow(HWND hTitleControl);

	void		SetupSystemStdStyles(short ext_up = 1, short ext_dn = 1, TTextStyleSymbolAdjustInfo *adjust_data = NULL);
				// Method inits the first 3 indexes of the background and foreground props and empty space color with
				// the current font of the dialog and with current Windows colors. This method assumes that control window
				// is already set.

	bool		SetupFrgrStyleSlot(int istyle, const wchar_t *fnt_name, int fnt_height, bool fnt_bold, TColor frgr_color, short ext_up = 0, short baseline_shift = 0, short ext_dn = 0);
	bool		SetupFrgrStyleSlot(int istyle, TColor frgr_color, short ext_up = 0, short baseline_shift = 0, short ext_dn = 0);
	bool		SetupFrgrStyleSlot(int istyle, TTextStyleProps &props, TTextStyleSymbolAdjustInfo *adjust_data = NULL);
	void		SetupBkgrStyleSlot(int istyle, TColor kbgr_color);
	void		SetupBkgrStyleSlot(int istyle, TBasicStyleProps &props);
	bool		SetupIconsStyle(HINSTANCE hInst, TIconsGridStyleProps &props);
	void		SetupNoItemsSpaceColor(TColor no_items_color);
	void		SetupNoItemsSpaceColor(TBasicStyleProps &props);

	void		SetupColumnProps(TListBoxColumnProps *array_of_records);
	void		SetColumnAlignment(int iclmn, TObjectAlignment new_alignment);
	void		SetupColumnTitles(TListBoxColumnTilteProps *array_of_records);

	void		SetupColumnTitleSuffix(int iclmn, const wchar_t *suffix);
				// The suffix is expected to be the scripted string. When this method changes the column title, it converts
				// it into local scripted string. It is possible to remove the suffix by passing iclmn equal to -1.

	TBasicListBoxItem &Title() { return(m_title_item); }

public:

	static void	RegisterWindowClasses(HINSTANCE hInst = NULL);
					// If instance handle is omitted, instance of the executable will be used.

	virtual int		GetNumColumns() const { return(m_horz_helper.NumColumns()); }
	virtual bool	IsTitleItem(TListBoxItemCore *item) { return(item == &m_title_item); }

	virtual void	ScrollListArea(int area_beg, int area_len, int shift_val, bool update_now = TRUE);
	virtual void	InvalidateListArea(int area_beg = 0, int area_len = -1);
	virtual void	UpdateWindows();

	virtual void	SetupVertScroller(int nMax, int nPage, int nPos);
	virtual void	DisposeVertScroller();

	DWORD		ShowAndTrackPopupMenu(TMenuItemInfo *menu_info, TListBoxMouseEventInfo *click_event_mouse_info);
					// Return value is menu id that was selected by the user or 0 if ESC was pressed or Windows failed
					// to create the menu.
protected:

	HDC		GetControlDC() { assert(m_hListBodyControl != NULL); return(::GetDC(m_hListBodyControl)); }
	void		ReleaseControlDC(HDC hDC) { assert(m_hListBodyControl != NULL); ::ReleaseDC(m_hListBodyControl, hDC); }
	void		GetListRect(RECT &rc, bool include_scroller_area);

	void		PaintTitleControl(HDC hDC, RECT &invalidRect);
	void		PaintListBodyControl(HDC hDC, RECT &invalidRect);

	void		ProcessVertScroll(int windows_scroll_event);
	void		ProcessResizeEvent();


	void		ProcessTitleMouseClick(TListBoxNotificationCode evt, WPARAM wParam, LPARAM lParam);
	void		ProcessListBodyMouseClick(TListBoxNotificationCode evt, WPARAM wParam, LPARAM lParam);
	void		ProcessMouseSelectionChangeClick(WPARAM wParam, LPARAM lParam);

	TColor	GetMissingItemColor(bool slct) { return((slct == TRUE) ? RGB(101, 145, 248) : RGB(252, 245, 116)); }
				// This method is used when listbox table returns the NULL pointer. This method is
				// not applicable for local list mode because items in the local list cannot be missing.

	void		RetrieveSystemFontProps(TTextStyleProps &props);

protected:

	bool					m_title_msg_active;
	bool					m_list_msg_active;
							// In fact, Windows can call window procedure recursively. And this really happens, and not just only
							// for "read only" messages like WM_PAINT. This flag allows detection of these nasty recursive calls.

	HWND				m_hTitleControl;
	HWND				m_hListBodyControl;
	HWND				m_hVertScroll;

	TBasicListBoxItem		m_title_item;
	TListBoxHorzHelper		m_horz_helper;

	enum { TLB_MAX_STYLES = 20 };

	TTextStyle			m_frgr_styles[TLB_MAX_STYLES];
	TBasicStyle			m_bkgr_styles[TLB_MAX_STYLES];
	TBitmapStyle			m_icons_style;
	TBasicStyle			m_no_items_space_style;

	static bool			m_control_classes_registered;

protected:

	bool		CheckTitleWindowRecursion();
	bool		CheckListWindowRecursion();

	//
	//  Window LONG memory use:
	//
	//		DWORD[0] - TListBoxObject*;
	//		DWORD[1] - hFont, provided by dialog.
	//

	static LRESULT CALLBACK ListTitleWindowProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK ListBodyWindowProc(HWND, UINT, WPARAM, LPARAM);

	friend class TBasicListBoxItem;
};

typedef TBasicListBoxObject TListBoxObject;

#endif	// WinUi_ListBoxObject_H


