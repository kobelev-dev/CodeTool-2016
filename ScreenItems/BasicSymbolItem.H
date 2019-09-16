//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_BasicSymbolItem_H
#define	ScreenItems_BasicSymbolItem_H

#ifndef   ScreenItems_ScreenItems_H
#include  "ScreenItems/ScreenItems.H"
#endif

//
//  Instance of the structure never owns styles, that it points to.
//
struct TBasicSymbolStyleInfo
{
	TBitmapStyle			*app_icons_style;
	short				app_cnct_icon;							// When the icon id is zero, no icon is displayed.
	short				app_pcb_icon;							// When the icon id is zero, no icon is displayed.

	TBasicStyle			*main_bkgr_style;
	TBasicStyle			*pcb_lower_bkgr_style;

	TTextStyle			*main_label_style;
	TTextStyle			*symbol_label_style;

	short				cnct_circle_diameter;
	char					cnct_main_label_shift_x;
	char					cnct_main_label_shift_y;
	char					cnct_symbol_label_shift_x;
	char					cnct_symbol_label_shift_y;

	short				pcb_max_rect_width;					// Note that this is a max width, not the width itself.
	char					pcb_main_label_margins;
	char					pcb_main_label_down_shift;
	char					pcb_main_label_lo_space;
	char					pcb_symlab_left_margin;
	char					pcb_symlab_right_margin;
	char					pcb_lower_rect_height;

	void Clear() { memset(this, 0, sizeof(TBasicSymbolStyleInfo)); }
};

//
//  This style has a limited use. It was defined primarily for using it in the test code.
//
class TBasicSymbolStyle : public TScreenItemStyle
{
public:

	TBasicSymbolStyle();

	enum { type_ID = 0x0105 };

	virtual  short	StyleTypeId() const { return(type_ID); }
	virtual  bool	SetupForDeviceContext(HDC hDC);

	TBasicSymbolStyleInfo &GetBasicStyleInfo() { return(m_style_info); }

protected:

	TBasicSymbolStyleInfo		m_style_info;

	TBasicStyle				m_main_bkgr_style;
	TBasicStyle				m_pcb_lower_bkgr_style;

	TTextStyle				m_main_label_style;
	TTextStyle				m_symbol_label_style;
};

//
//  Generic class for creating various objects. Applications can either use this class directly,
//  or create derived classes.
//
class TBasicSymbolItem : public TScreenItem
{
public:

	TBasicSymbolItem(TScreenItemStyle *stl = NULL) : TScreenItem(stl) { }

	//
	//  Methods, inherited from TScreenItem.
	//

	enum { type_ID = 0x310 };

	virtual short	GetItemTypeId() const { return(type_ID); }

	virtual void	SetOwner(void *owner, DWORD ext_data = 0);
					// Set owner method sets passed params into all internal screen items.

	virtual void	SetBoundingRectPos(long pos_x, long pos_y);
	virtual void	SetHotSpotPos(long pos_x, long pos_y);
	virtual void	GetHotSpotPos(long &px, long &py);
	virtual void	ShiftItem(long shift_x, long shift_y);

	virtual void			OnDraw(HDC hDC, RECT &rc_invalid);
	virtual TScreenItem	*CheckClick(POINT &app_point);

	//
	//  New methods.
	//

	void		ResetScreenItems(TList &screen_items_data_set);

	void		PrepareConcentricModeItems(HDC hDC, TBasicSymbolStyleInfo &style_info, int circle_center_pos_x, int circle_center_pos_y,
											const wchar_t *main_label, int main_label_len, const wchar_t *symbol_label, int symbol_label_len,
											short app_icon_id = 0);
	void		PrepareRectLinesModeItems(HDC hDC, TBasicSymbolStyleInfo &style_info, int pos_x, int pos_y, int children_based_min_width_x,
											const wchar_t *main_label, int main_label_len, const wchar_t *symbol_label, int symbol_label_len,
											short app_icon_id = 0);

	int		GetRectLinesModeRectWidth(HDC hDC, TBasicSymbolStyleInfo &style_info, int min_width_x,
											const wchar_t *main_label, int main_label_len, const wchar_t *symbol_label, int symbol_label_len);
	int		GetRectLinesModeRectHeight(HDC hDC, TBasicSymbolStyleInfo &style_info);

protected:

	void		PrepareConcentricModeTextItem(HDC hDC, TBasicSymbolStyleInfo &style_info, int center_pos_x, int center_pos_y,
											TTextItemExProto &src_item, TTextStyle *text_style, const wchar_t *text_body, int text_len, int offs_x, int offs_y);
	void		PrepareRectLinesModeTextItem(HDC hDC, TBasicSymbolStyleInfo &style_info, int pos_x, int pos_y,
											TTextItemExProto &src_item, TTextStyle *text_style, const wchar_t *text_body, int text_len,
											bool add_dots_if_truncating, int left_margin, int right_margin);
public:

	TBitmapItem			m_app_icon;

	TTextItem40			m_main_label;
	TTextItem20			m_symbol_label;

	TCircleItem			m_def_circle;			// Concentric placing mode only.
	TRectItem			m_main_rect;			// RectLines placing mode only.
	TRectItem			m_lower_rect;			// RectLines placing mode only.
};

#endif	// ScreenItems_BasicSymbolItem_H


