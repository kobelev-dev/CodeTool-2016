//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_ScreenItemsCore_H
#define	ScreenItems_ScreenItemsCore_H

//
// Messages for communication between the control state and view classes.
// All these messages return 0 on success and -1 otherwise.
//

#define SM_SET_PROPERTIES					WM_USER + 1
			// Set the properties of the control.
			// wParam - should be 0, reserved for future use.
			// lParam - pointer to TScreenItemsViewFrameProps structure

#define SM_SET_SCROLL_POS					WM_USER + 2
			// Set the scroll position.
			// wParam - new horz scroll position (long)
			// lParam - new vert scroll position (long)

#define SM_SET_ZOOM_LEVEL					WM_USER + 3
			// Set new zoom level.
			// wParam - should be 0, reserved for future use.
			// lParam - new zoom level (float).

#define SM_SHOW_MAIN_LAYER_DATA_SET		WM_USER + 4
			// Set the data set to display.
			// wParam - should be 0, reserved for future use.
			// lParam - TList*, pointer to the new data set to display.

#define SM_SHOW_FRONT_LAYER_DATA_SET	WM_USER + 5
			// Set the data set to display.
			// wParam - should be 0, reserved for future use.
			// lParam - TList*, pointer to the new data set to display.

#define SM_INVALIDATE_APPSP_RECT			WM_USER + 6
			// Set the data set to display.
			// wParam - should be 0, reserved for future use.
			// lParam - RECT*, in the app space coordinates.

#define SM_GET_SPACE_INFO					WM_USER + 100
			// Retrieve the information about the size of the areas and scroll pos.
			// wParam - should be 0, reserved for future use.
			// lParam - TScreenItemsViewSpaceInfo*, pointer to the buffer to fill in.

#define SM_GET_CURRENT_PROPS				WM_USER + 101
			// Retrieve the information about the size of the areas and scroll pos.
			// wParam - should be 0, reserved for future use.
			// lParam - TScreenItemsViewFrameProps*, pointer to the buffer to fill in.

#define SM_GET_CLIENT_POS					WM_USER + 102
			// Convert app space pos into client pos (click pos).
			// wParam - POINT*, app_space point, input.
			// lParam - POINT*, buffer for placing client space point data.

#define SM_GET_APPSPACE_POS				WM_USER + 103
			// Convert client pos (click pos) into app space pos.
			// wParam - POINT*, client space point, input.
			// lParam - POINT*, buffer for placing  app_space point data.

#define SM_GET_MAIN_LAYER_DATA_SET		WM_USER + 104
#define SM_GET_FRONT_LAYER_DATA_SET		WM_USER + 105
			// Retrieve information about the current data sets.
			// wParam - should be 0, reserved for future use.
			// lParam - should be 0, reserved for future use.

#define SM_GET_SCRITEMS_FROM_POINT		WM_USER + 106
			// Retrieve information about the current data sets.
			// wParam - should be 0, reserved for future use.
			// lParam - TScreenItemSummarizeInfo*, params and buff for results.

//
// Internal state of the custom control.
//
class TScreenItemsViewControlState
{
public:

	TScreenItemsViewControlState(HWND hCtrl);
				// Object is created inside the WM_CREATE message handler.

	virtual ~TScreenItemsViewControlState() { }
				// Object is deleted on WM_DESTROY message.

	static LRESULT CALLBACK ScreenItemsViewWndProc(HWND hWnd, DWORD msgid, WPARAM wParam, LPARAM lParam);

	static int  CalcBoundingRect(TList *data, RECT &rc_max);
				// Return value is the number of ScreenItems that participated in forming the bounding rect.

protected:

	void   OnPaint();
	void   OnHorzScroll(int event_type);
	void   OnVertScroll(int event_type);
	void   OnMouseBtnEvent(WPARAM wparam, LPARAM lparam, TScreenItemsViewNtfCode);
	void   InvalidateAppSpaceRect(RECT &app_rect);

private:

	TScreenItem	*ProcessScreenItems(POINT &click_pt, POINT &app_pt, TScreenItemSummarizeInfo *sumry_info);
					// This method ignores screen items with NULL item owner.

	void   SetupAndInvalidate(bool layout_chg, bool data_chg);
	void   VerifyHorzScrollPos(long &new_pos);
	void   VerifyVertScrollPos(long &new_pos);
	void   VerifyAndSetupScrollPos(long new_scroll_x, long new_scroll_y);
	void   CalcDataSetLayout(bool full_recalc);
	void   CalcControlLayout();

	void   AppSpacePointToClientPoint(POINT *app_point, POINT *cln_point);
	void   ClientPointToAppSpacePoint(POINT *cln_point, POINT *app_point);
	void   SendNewScrollPosNotification();
	void   SendNotification(TScreenItemsViewNtfCode ntf, void *ext_param) { ::SendMessage(m_hParent, WM_COMMAND, MAKEWPARAM(m_ctrl_id, ntf), (LPARAM)ext_param); }

	static void ShiftRect(RECT &rc, long sh_x, long sh_y) { rc.left += sh_x; rc.top += sh_y; rc.right += sh_x; rc.bottom += sh_y; }

protected:

	HWND						m_hControl;							// Handle of the control.
	DWORD						m_ctrl_id;							// Control identifier.
	HWND						m_hParent;							// Parent window handle.

	TScreenItemsViewFrameProps	m_props;							// Properties of the control.

	HWND						m_hHorzScroll;						// Horizontal scrollbar handler
	HWND						m_hVertScroll;						// Vertical scrollbar handler

	TList							*m_main_layer_screen_items;		// Data for display, can be NULL.
	TList							*m_front_layer_screen_items;		// Data for display, can be NULL.
										//
										// Control is not owning the lists themselves and it is also not owning
										// screen items that are members of these lists.
										//

	bool							m_app_rect_chg;					// AppSpace info is invalid because of the changes in the data set.

	TScreenItemsViewSpaceInfo		m_space;
	POINT						m_app_shift;						// This shift brings the left top corner of the data set to the
																	// left top corner of the control.
	POINT						m_cntr_shift;						// This shift is non zero only when centering of the small data set
																	// is requested and needed.
	RECT						m_screen_items_area;				// This rect is close to the rect of the whole control. It is not
																	// including only the frame and the scroll bars if any.
	RECT						m_corner_area;

	XFORM						m_xform;
};

#endif	// ScreenItems_ScreenItemsCore_H


