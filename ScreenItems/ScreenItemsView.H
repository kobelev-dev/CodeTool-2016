//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_ScreenItemsView_H
#define	ScreenItems_ScreenItemsView_H

#ifndef   ScreenItems_ScreenItems_H
#include  "ScreenItems/ScreenItems.H"
#endif

//
//  This function registers window class for the custom control.
//  Function will use HINSTANCE of the excecutable if this function is called with default parameter.
//
void  ScreenItemsRegisterWindowClasses(HINSTANCE hInst = NULL);

//  -------------- Custom control -----------------

#define  SCREEN_ITEMS_VIEW_CTRL_CLASS   L"SCREEN_ITEMS_VIEW_CONTROL"

//
// Scroll bars on the sides of the container are either permanently existing or permanently not existing.
// It is not possible to make them auto-hiding.
//
struct  TScreenItemsViewFrameProps
{
	bool				use_inset_frame;
	bool				use_horz_scroll;
	bool				use_vert_scroll;
	bool				show_focus_stt;					// This flag instructs to display state of the input focus in the lower-right corner.

	TColor			bkgr_brush;						// Backgound color should not be transparent.
	TColor			corner_brush;					// Color of the corner that is visible when the horz and vert scrollbars are both present.
	TColor			focus_stt_dots;

	bool				center_small_data_set_x;
	bool				center_small_data_set_y;
	bool				excl_ltop_empty_space;			// This field instructs to omit the space if the topmost and/or leftmost objects
													// do not begin from the coordinate value 0.
	short			horz_margin;					// Empty space to the left and right of the dataset in screen pixels.
	short			vert_margin;					// Empty space to the top and bottom of the dataset in screen pixels.
	short			horz_scroll_height;				// The value 0 defaults the to the current system metrics.
	short			vert_scroll_width;				// The value 0 defaults the to the current system metrics.

	void		SetStndScrollableProps(TColor bkgr_color = RGB(255, 255, 255), int horz_marg = 10, int vert_marg = 10);
	void		SetStndDocStyleProps(bool show_scrollers_horz, bool show_scrollers_vert, TColor bkgr_color = RGB(255, 255, 255));
};

//-----------------------------------------------------------------------
//
//  Notes on positioning the data set on the surface of the control. The data set is positioned based
//  on the following algorithm:
//
//   1. Determine the bouding rect around all visible ScreenItems. This will   be called "appsp_rect".
//  This rect can be returned to the user on request.
//
//   2. ZOOM conversion is applied to the "appsp_rect" at this point. Zoomed rect will be
//   called "appsp_zoomed".
//
//   3. Determine the "displayable rect" (dr).
//
//           if (excl_ltop_empty_space == TRUE)
//           {
//               dr.left   = appsp_zoomed.left-horz_margin;
//               dr.top    = appsp_zoomed.top-vert_margin;
//               dr.right  = appsp_zoomed.right+horz_margin;
//               dr.bottom = appsp_zoomed.bottom+vert_margin;
//           }
//           else
//           {
//               dr.left   = 0;
//               dt.top    = 0;
//               dr.right  = appsp_zoomed.right+horz_margin;
//               dr.bottom = appsp_zoomed.bottom+vert_margin;
//           }
//
//   This rect is never returned to the app level.
//
//   Note: Some of the fields can be negative even if all screen items have positive coordinates
//   because of the margins.
//
//   4a. When size of the "displayable rect" is bigger than the size of the control, it is scrolled in
//   the control from its smalest pixel to its biggest pixel.
//
//   4b. When size of the "displayable rect" is smaller than the size of the control, data set is either
//   placed at the left/top side or it is centered in the client area of the control.
//
//   Note: Considerations of the point 4 are applied to horz/vert directions independently, i.e. the size
//   of "displayable rect" can be bigger than the control in one direction and smaller in another direction.
//

struct TScreenItemsViewSpaceInfo
{
	RECT			appsp_rect;				// Rect that is covered with ScreenItems. The app units are used.
											// It is most likely that the left and top members of this field are not 0.
	SIZE			app_zm_size;			// Size of the ScreenItems covered area after applying zoom and adding
											// the requested margins. This size is expressed in the screen pixels.
	SIZE			client_size;				// Current size of the control area that can be used for drawing screen
											// items. This size is close to the client size of the control but it excludes
											// the scrollers and the inset frame if any.
	float				zoom_level;				// Current zoom level. Note that screen resolution is fixed at 75 dpi.
											// Zoom level equal to 1.0 will still zoom out.
	POINT			scroll_pos;				// Current scroll position in the screen pixels. This is position of the smaller
											// rect that has the client_sizes on the bigger rect that has the app_zm_size.
											// The value of this field for "small-centered" data sets is zero.
	DWORD			num_scr_items;			// Number of screen items in the curr data set that have non empty
											// (not zero filled) item rect.

	void Clear() { memset(this, 0, sizeof(TScreenItemsViewSpaceInfo)); }
};

// Control notification codes sent as loword of WPARAM.
enum TScreenItemsViewNtfCode
{
	xiv_none,
	xiv_new_focus_state,
	xiv_paint_background,				// lParam - PAINTSTRUCT*
	xiv_paint_topmost,					// lParam - PAINTSTRUCT*
	xiv_new_scroll_pos,					// lParam - TScreenItemsViewNotify*
	xiv_new_zoom_level,				// lParam - new ZOOM level (float)
	xiv_left_btn_down,					// lParam - TScreenItemsViewMouseNotify*
	xiv_right_btn_down,					// lParam - TScreenItemsViewMouseNotify*
	xiv_left_btn_dblclk,					// lParam - TScreenItemsViewMouseNotify*
	xiv_corner_clicked,					// lParam - TScreenItemsViewNotify*
	xiv_wm_char,						// lParam - TScreenItemsViewNotify*
	xiv_updt_front_scr_items,			// lParam - TScreenItemsUpdateFrontLayerNotify*
};

// General notification structure.
struct TScreenItemsViewNotify
{
	DWORD			vkey;				// Shift keys state.
	long				pos_x;				// Meaning of the field depends on the message.
	long				pos_y;				// Meaning of the field depends on the message.
	bool				right_btn;
	WPARAM			wparam;			// Meaning of the field depends on the message.
	LPARAM			lparam;				// Meaning of the field depends on the message.
};

// Mouse notification structure.
struct TScreenItemsViewMouseNotify
{
	DWORD			vkey;				// Shift keys state.
	POINT			click_place;			// Client coordinates.
	long				app_x;				// Mouse x coordinate.
	long				app_y;				// Mouse y coordinate.
	TScreenItem		*object;				// Clicked object or NULL.
};

// Details for xiv_updt_front_scr_items.
struct TScreenItemsUpdateFrontLayerNotify
{
	bool				update_reason;		// TRUE - scroll.
	long				shift_x, shift_y;
};

//
//           Notes
//   -------------------
//
//     1. Applications should provide reasonably small positions to avoid overflows in coordinate calculations.
//   Positions are in device pixels. Screen resolution is fixed as 75 dpi.
//     2. Items of the list are not expected to have any geometrical relation. Order of the items in the list
//  affects only the painting order.
//     3. Items have bounding rectangulars. They are not expected to cover them fully. Bounding rects
//  of different items may overlap.
//
class TScreenItemsViewObject
{
public:

	TScreenItemsViewObject() { m_control = NULL; }
	~TScreenItemsViewObject() { m_main_layer_owned_screen_items.DeleteAll(); m_front_layer_owned_screen_items.DeleteAll(); }
				// Destructor deletes objects only from the lists of the owned objects.

	//
	//  These two methods establish bidirectional communication between the helper object and  the control.
	//
	void		SetWindow(HWND hCtrl) { m_control = hCtrl; }
				// This method should be called only once while processing WM_INITDIALOG.

	HWND	GetControl() { return(m_control); }
				// Main purpose of this method - allow creation the control window DC.

	//
	//  Set properties for the control.
	//

	void		SetProperties(TScreenItemsViewFrameProps &props, bool paint_now = TRUE);
	void		SetStndScrollableProps(TColor bkgr_color = RGB(255, 255, 255), int horz_marg = 10, int vert_marg = 10, bool paint_now = TRUE);
	void		SetStndDocStyleProps(bool horz_scroller, bool vert_scroller, TColor bkgr_color = RGB(255, 255, 255), bool paint_now = TRUE);

	void		SetScrollPos(long pos_x, long pos_y,  bool paint_now = TRUE);
				// Passing -1 as new scroll position means keeping existing scroll pos.

	void		SetZoomLevel(float new_zoom_level, bool paint_now = TRUE);

	//
	// Group of data set related actions.
	//

	void		ShowDataSet(TList *new_data_set, bool paint_now = TRUE);
	void		ShowFrontLayerDataSet(TList *new_data_set, bool paint_now = TRUE);
				// When second param is TRUE, control is invalidated and repainted.

	TList		*CurrDataSet();
	TList		*CurrFrontLayerDataSet();

	void		ClearInternalDataSet() { m_main_layer_screen_items.Clear();  }
	void		ClearInternalFrontLayerDataSet() { m_front_layer_screen_items.Clear(); }
	void		AddToInternalDataSet(TScreenItem &scr_item) { scr_item.Clear(); m_main_layer_screen_items.AppendItem(scr_item); }
	void		AddToInternalFrontLayerDataSet(TScreenItem &scr_item) { scr_item.Clear(); m_front_layer_screen_items.AppendItem(scr_item); }
	void		RemoveFromInternalDataSet(TScreenItem &scr_item) { m_main_layer_screen_items.RemoveItem(scr_item); }
	void		RemoveFromInternalFrontLayerDataSet(TScreenItem &scr_item) { m_front_layer_screen_items.RemoveItem(scr_item); }
	void		ShowInternalDataSet(bool paint_now = TRUE) { ShowDataSet(&m_main_layer_screen_items, paint_now);  }
	void		ShowInternalFrontLayerDataSet(bool paint_now = TRUE) { ShowDataSet(&m_front_layer_screen_items, paint_now); }

	bool		AddToInternalDataSetBeforeItem(TScreenItem &scr_item, TScreenItem *ins_before);
	bool		AddToInternalDataSetAfterItem(TScreenItem &scr_item, TScreenItem *ins_after);
				// Return value shows if the passed ins_before/ins_after item is present in the painting list or not.
				// If the passed ins_before/ins_after item is not present, the item is not inserted at all.

	TList		&InternalDataSet() { return(m_main_layer_screen_items);  }
	TList		&InternalFrontLayerDataSet() { return(m_front_layer_screen_items); }
	TList		&InternalOwnedDataSet() { return(m_main_layer_owned_screen_items);  }
	TList		&InternalOwnedFrontLayerDataSet() { return(m_front_layer_owned_screen_items); }

	void		ScrollDataSetIntoCenter(bool center_x = TRUE, bool center_y = TRUE);

	bool		CheckFullOrPartialScrollOut(TScreenItem &scr_item);
	void		ScrollScreenItemIntoTheView(TScreenItem &scr_item, long horz_marg = 0, long vert_marg = 0);

	void		GetSpaceInfo(TScreenItemsViewSpaceInfo &buffer);
	void		GetCurrentFrameProps(TScreenItemsViewFrameProps &props);

	void		GetClientSpacePos(POINT &app_point, POINT &client_point);
	void		GetAppSpacePos(POINT &client_point, POINT &app_point);
					// Note that client space pos is position of the point on the window control. This is not the same to offset
					// from the sides of the area for drawing screen items. The difference is the width of the inset frame.

	DWORD		CreateScreenShotProto(RECT *margins_rect, HDC &hMemoryDC, HBITMAP &hBitmapObject, HBITMAP &hBitmapOld, SIZE &bitmap_size);
					// Method calculates intersection between the current window and the data set, adds to this intersection
					// requested margins, if they are passed, creates bitmap of this size, copies visible part of the data set into
					// the bitmap. Margins area is filled with the current bkgr color. Return value is the Win32 eror if any.

	void		ScreenShotProtoCleanup(HDC &hMemoryDC, HBITMAP &hBitmapOld);
					// This method should be called if the CreateScreenShotProto returned success after doing ALL extra
					// painting on the main bitmap.

	int		GetScreenItemsFromPoint(long client_x, long client_y, TScreenItem **buffer, int len_buff);
					// Method fills the buffer with pointers to screen items up to its length. Return value is real number
					// of screen items under the passed point that can be bigger than the length of the buffer.

	void		InvalidateControl() { if (m_control != NULL) ::InvalidateRect(m_control, NULL, TRUE); }
	void		InvalidateAppSpaceRect(RECT &appsp_rect);
	void		InvalidateClientSpaceRect(RECT &client_rect) { if (m_control != NULL) ::InvalidateRect(m_control, &client_rect, TRUE); }
	void		InvalidateScreenItemRect(TScreenItem *item) { InvalidateAppSpaceRect(*(item->GetBoundingRect())); }
	void		InvalidateFrontLayerScreenItemRect(TScreenItem *item) { InvalidateClientSpaceRect(*(item->GetBoundingRect())); }
	void		UpdateControl() { if (m_control != NULL) ::UpdateWindow(m_control); }
	void		RepaintControl() { InvalidateControl(); UpdateControl(); }

	void			SaveClickedPointColor(TScreenItemsViewMouseNotify *ev_info);
					// Method saves color of the clicked point into the source/result color of the TControlHelper::ShowColorSelectDialog() method.

	DWORD		ShowAndTrackPopupMenu(const TMenuItemInfo *menu_info, long click_x, long click_y);
					// Return value is menu id that was selected by the user or 0 if Esc was pressed.
					// Params click_x, click_y are client coordinates.

	static  bool	IsItemsNotifyEvent(TScreenItemsViewNtfCode evt) { return((evt == xiv_new_scroll_pos || evt == xiv_corner_clicked || evt == xiv_wm_char) ? TRUE : FALSE); }
	static  bool	IsMouseNotifyEvent(TScreenItemsViewNtfCode evt) { return((evt >= xiv_left_btn_down && evt <= xiv_left_btn_dblclk) ? TRUE : FALSE); }
	static  bool	IsUserActivityEvent(TScreenItemsViewNtfCode evt) { return((evt >= xiv_left_btn_down && evt <= xiv_wm_char) ? TRUE : FALSE); }

protected:

	bool		PrepareScrollCalculations(TScreenItem &scr_item, RECT &rc_src_item_client, long &inset_frame_width);

protected:

	HWND		m_control;

	TList			m_main_layer_screen_items;				// Items in this list are not owned by the control.
	TList			m_front_layer_screen_items;				// Items in this list are not owned by the control.

	TList			m_main_layer_owned_screen_items;
	TList			m_front_layer_owned_screen_items;
};

#endif	// ScreenItems_ScreenItemsView_H


