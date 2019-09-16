//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_ScreenItems_H
#define	ScreenItems_ScreenItems_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif
#ifndef   Common_StringPointer_H
#include  "Common/StringPointer.H"
#endif
#ifndef   WinUi_BitmapHelper_H
#include  "WinUi/BitmapHelper.H"
#endif
#ifndef   WinUi_DialogControlHelpers_H
#include  "WinUi/DialogControlHelpers.H"
#endif

//
//     Notes on the screen items styles implementation.
//
//     Instance of the style can be used only with the window or printer context that it was setup with.
//  It is ok to use Windows resources like HFONT, HBRUSH, etc in the instances of the styles.
//
//     Contents of the data fields and actual methods of the screen object type specific styles may vary
//  from style to style and they represent private contract between the styles and their corresponding
//  screen items.
//
//  This header defines the following 4 major styles:
//
//     TBasicStyle			(TRectItem, TCircleItem)
//     TBitmapStyle			(TBitmapItem)
//     TLineStyle			(TLinesChainItem)
//     TTextStyle			(TTextItem)
//

class TScreenItem;

//
//  This value is used in screen items style collections. These collections may include style props that
//  were mentioned above and named objects that contain only one number inside.
//
enum { NumParam_type_ID = 0xA10 };

//
//  Basic style of displaying/printing the screen objects.
//
class TScreenItemStyle : public TListItem
{
public:

	TScreenItemStyle() { m_app_ptr = NULL; }
	virtual ~TScreenItemStyle() { }

	virtual  short	StyleTypeId() const = 0;
					//
					//  Derived class should return unique id of the style type.
					//

	virtual  bool	SetupForDeviceContext(HDC hDC) { return(TRUE); }
					//
					//  This method setups/resetups the internal data members for the passed display or printer device.
					//  If any cleanup is needed after this setup, it should be implemented in the destructor.
					//

	virtual  bool	SetupForDeviceContextEx(HWND m_hDlg);
					//
					//  This is a wrapper around the SetupForDeviceContext(HDC);
					//
	union
	{
		void			*m_app_ptr;
		DWORD		m_app_dword;
	};
};

//
// This structure is used for getting list of screen items that occupy the certain point.
//
struct TScreenItemSummarizeInfo
{
	POINT			click_pt;
	POINT			app_pt;
	int				cnt_found;					// This is the real number of the objects. This number can
												// be bigger than the length of the provided buffer.
	TScreenItem		**screen_items_buff;
	int				len_buff;
	int				len_used;

public:

	inline void AddToSummary(TScreenItem *scr_item)
	{
		assert(scr_item != NULL);
		if (len_used < len_buff)
			screen_items_buff[len_used++] = scr_item;
		cnt_found++;
	}
};

enum TDataBypassMode
{
	dbm_use_only,			// Passed pointer will remain valid throughout the whole lifetime of the object.
	dbm_take_ownsh,		// Data is in the heap, object should release the data when it will be not longer needed.
	dbm_duplicate,			// Allocate memory in the heap and copy the data there.
};

//
//  Base class for all screen objects.
//
class TScreenItem : public TListItem
{
public:

	TScreenItem(TScreenItemStyle *stl = NULL) { m_item_owner = NULL; m_owner_data = 0; m_style = stl; memset(&m_bounding_rect, 0, sizeof(RECT)); }

	virtual short		GetItemTypeId() const = 0;
	virtual void		SetOwner(void *owner, DWORD ext_data = 0) { m_item_owner = owner; m_owner_data = ext_data; }

	void				CopyOwnerInfoFrom(TScreenItem *inst) { SetOwner(inst->m_item_owner, inst->m_owner_data); }

	void				*GetOwner() const { return(m_item_owner); }
	DWORD			GetOwnerData() const { return(m_owner_data); }

	TScreenItemStyle  *GetBasicStyle() const { return(m_style); }
	virtual void		SetStyle(TScreenItemStyle *stl) { m_style = stl; }

	virtual const wchar_t	*GetCssStyleName() const { return(NULL); }

	virtual void		SetBoundingRectPos(long pos_x, long pos_y) = 0;
	virtual void		SetHotSpotPos(long pos_x, long pos_y) = 0;
						// These methods are not expected to be used before contents of the object is set up.

	virtual void		GetHotSpotPos(long &px, long &py) = 0;
						// Derived class should return left-top corner of the object's rect if it has no better hot spot on it.

	virtual void		ShiftItem(long shift_x, long shift_y) = 0;
						// Semantics of this method is "shift". This method is not expected to be used before the contents
						// of the object is set up.

	inline  RECT		*GetBoundingRect()		{ return(&m_bounding_rect); }
	inline  long		GetBoundingRectX()		{ return(m_bounding_rect.left); }
	inline  long		GetBoundingRectY()		{ return(m_bounding_rect.top); }

	inline  long		GetWidth()				{ return(m_bounding_rect.right-m_bounding_rect.left); }
	inline  long		GetHeight()				{ return(m_bounding_rect.bottom-m_bounding_rect.top); }

	virtual void			OnDraw(HDC hDC, RECT &invalid_appsp_rect) = 0;
							//
							// This method implements drawing object in the DC. When this method is called, caller has already
							// checked that invalid rect intersects with screen item. Second param is used for optimizing huge
							// objects painting.
							//

	virtual TScreenItem	*CheckClick(POINT &app_point) = 0;
							//
							// Simple objects like rect or bitmap should return "this" when POINT is on the territory of the object.
							// More complex objects may act as containers and return one of their internal objects.
							//
							// Derived classed should NOT check if the owner field is NULL or not. This is checked before calling this
							// method on the level of the screen items control.
							//

	virtual void			SummarizeScreenItems(TScreenItemSummarizeInfo &info, bool bounding_rect_mode);
							//
							// This method appends list of all screen items that are mapped at the passed point to
							// the summarization buffer. The point itself is one of the fields of the summary struct.
							//
							// The bounding rect mode param is TRUE when the screen items view is deeply zoomed out.
							// In this case derived class can be sure that passed point stays inside of the bounding rect.
							//
							// Typical class should not overwrite this method. Only container classes should do this and only
							// those classes that can have more than one screen item at the same point should overwrite this
							// method. Those classes should rountiney check all objects that mapped at the passed point and
							// return a complete list of these objects.
							//

protected:

	void					*m_item_owner;			// Object can be clicked with the mouse only when the owner
												// field is not NULL.
	DWORD				m_owner_data;			// Any data that should be understood by the owner only.
												// In typical case this will be id of part of a complex object.
	TScreenItemStyle		*m_style;				// Contents and meaning of the style is completely specific
												// to the derived class.
	RECT				m_bounding_rect;		// Rect that covers all parts of the item. This field is used
												// for invalidating part of the screen and making decision
												// if items should be painted or not for some particular
												// invalid screen region.
protected:

	void   ShiftRcItem(long sh_x, long sh_y) { m_bounding_rect.left += sh_x; m_bounding_rect.top += sh_y; m_bounding_rect.right += sh_x; m_bounding_rect.bottom += sh_y; }

public:

	static bool  IsPointInRect(POINT &point, RECT &rect);
	static bool  CheckIntersection(RECT &rect1, RECT &rect2);
	static void  CombineRects(RECT &rect1_and_result_rect, RECT &rect2);

	friend class TScreenItemsViewControlState;
};

class TScreenItemPtrsArray : public TStructsArray<TScreenItem*, 4, 64>
{
public:

	~TScreenItemPtrsArray() { ReleaseItems(); }
	inline  void	Clear() { ReleaseItems(); SetupReadWriteMode(); }

protected:

	inline  void	ReleaseItems() { for (int inx=0; inx<NumItems(); ++inx) if (ItemPtr(inx) != NULL) { delete ItemRef(inx); ItemRef(inx) = NULL; } }
};

//
// - - - - - -  Objects 1, 2. Rect and Circle  - - - - - - - - -
//

//
//  Style of the figure like a rect or a circle.
//
struct TBasicStyleProps
{
	wchar_t				*css_style_name;				// The field should be a pointer to a static string or NULL.

	TColor				background_color;				// The whole shape is filled with this color.

	TColor				shadow_up_color;
	int					shadow_up_width;
	TColor				shadow_down_color;
	int					shadow_down_width;

	TColor				frame_color;
	short				frame_width;					// In pixels. When the value of this field is 0, the object has no frame.
	short				frame_offset;					// In pixels. Offset can be 0 or some positive value.
	int					frame_style;					// This is fnPenStyle for CreatePen(..) function.
														// PS_SOLID = 0, PS_DASH = 1, PS_DOT = 2, PS_DASHDOT = 3.

	TColor				frame_shadow_up_color;
	int					frame_shadow_up_width;		// In pixels.
	TColor				frame_shadow_down_color;
	int					frame_shadow_down_width;		// In pixels.

	short				inner_margin_left;
	short				inner_margin_right;
	short				inner_margin_top;
	short				inner_margin_bottom;
							// For now the inner margins are used only for setting up the inline bkgr decors of the decorated file
							// views. When the value of the field is positive, it defines the width of the left and right extension areas
							// that are covered with the bkgr color. When the value of the field is negative, the bkgr color extends
							// right to the border of the frgr painting list item. In this case it covers the outer paddings and the horz
							// spacings, if any.

	unsigned short		outer_padding_left;
	unsigned short		outer_padding_right;
	unsigned short		outer_padding_top;
	unsigned short		outer_padding_bottom;
							// Note that outer paddings have an unsigned type. They are primarily used for placing the inline
							// bkgr decors in the decorated file views.

	void		Clear() { memset(this, 0, sizeof(TBasicStyleProps)); }
	void		Init(TColor bkgr) { Clear(); background_color = bkgr; }
	void		SetFrameProps(TColor color, short width, int style = 0) { frame_color = color; frame_width = width; frame_style = style; }
};

class TBasicStyle : public TScreenItemStyle
{
public:

	TBasicStyle() { memset(&m_props, 0, sizeof(m_props)); }
	TBasicStyle(TBasicStyleProps &prm_props) { Setup(prm_props); }
	~TBasicStyle() { }

	enum  { type_ID = 0x01020 };

	short	StyleTypeId() const { return(type_ID); }

	bool		Setup(TBasicStyleProps &new_props) { m_props = new_props; return(TRUE); }
				// Implementation copies the data from parameter into the data fields of the object.

	bool		Setup(TColor bkgr_color) { m_props.Init(bkgr_color); return(TRUE); }
	bool		SetupFrameProps(TColor color, short width, int style = 0) { m_props.SetFrameProps(color, width, style); return(TRUE); }

	inline TColor GetBkgrColor() { return(m_props.background_color); }

protected:

	void		BaseRectToBoundingRect(RECT &rc) const;
	void		BoundingRectPosToBaseRectPos(long &px, long &py) const;
	void		PaintSpecialDot(HDC hDC, float dot_x, float dot_y, TColor dot_color, int dot_diameter) const;

	TBasicStyleProps		m_props;

	friend class TRectItem;
	friend class TCircleItem;
	friend class TDecoratedFileViewStyle;
	friend class TDecoratedFileViewLineInfo;
	friend class TLangViewSourceFileStyle;
};

//
//  Rectangular.
//
//  Note that rect screen item can be fully transparent (using appropriate basic style) but clickable (using non NULL owner).
//
class TRectItem : public TScreenItem
{
public:

	TRectItem(TScreenItemStyle *stl = NULL) : TScreenItem(stl) { memset(&m_base_rect, 0, sizeof(RECT)); }
	TRectItem(const RECT &rc) { SetBaseRect(rc); }

	enum { type_ID = 0x010 };

	short	GetItemTypeId() const { return(type_ID); }
	void		SetStyle(TScreenItemStyle *new_style);

	const wchar_t		*GetCssStyleName() const { return((m_style != NULL) ? Style()->m_props.css_style_name : NULL); }
						// When the return value is not NULL, this will be the name of equivalent CSS style.
						// This method is used for generating HTML representation of list boxes and decorated files.

	void		SetBaseRect(const RECT &rc) { m_base_rect = rc; UpdateRcItem(); }
	void		GetBaseRect(RECT &destination) { destination = m_base_rect; }
	void		SetBaseRectSize(long size_x, long size_y) { m_base_rect.right = m_base_rect.left+size_x; m_base_rect.bottom = m_base_rect.top+size_y; UpdateRcItem(); }

	inline  long		GetBaseLeft() { return(m_base_rect.left); }
	inline  long		GetBaseTop()  { return(m_base_rect.top); }

	inline  long		GetBaseWidth() { return(m_base_rect.right-m_base_rect.left); }
	inline  long		GetBaseHeight()  { return(m_base_rect.bottom-m_base_rect.top); }

	void		SetBoundingRectPos(long pos_x, long pos_y);
	void		SetHotSpotPos(long pos_x, long pos_y);
	void		GetHotSpotPos(long &px, long &py) { px = m_base_rect.left; py = m_base_rect.top; }
	void		ShiftItem(long shift_x, long shift_y);

	void					OnDraw(HDC hDC, RECT &rc_invalid);
	TScreenItem			*CheckClick(POINT &app_point) { return((IsPointInRect(app_point, m_base_rect) == TRUE) ? this : NULL); }

	inline const TBasicStyle		*Style() const { return((TBasicStyle*)m_style); }

protected:

	inline void	UpdateRcItem() { m_bounding_rect = m_base_rect; if (m_style != NULL) Style()->BaseRectToBoundingRect(m_bounding_rect); }
	void			PaintSimpleFrame(HDC hDC, RECT &rc, long width, COLORREF color);

	static void	ShiftRect(RECT &rc, long sh) { rc.left += sh; rc.top += sh; rc.right += sh; rc.bottom += sh; }

protected:

	RECT				m_base_rect;

	friend class TDecoratedFileViewLineInfo;
};

//
//  Circle.
//
class TCircleItem : public TScreenItem
{
public:

	TCircleItem(TScreenItemStyle *stl = NULL) : TScreenItem(stl) { m_center_x = m_center_y = m_diameter = 0; }
	TCircleItem(long x, long y, long diam) { SetPosition(x, y, diam); }

	enum { type_ID = 0x011 };

	short	GetItemTypeId() const { return(type_ID); }
	void		SetStyle(TScreenItemStyle *new_style);

	void		SetPosition(long x, long y, long new_diameter);
	void		SetDiameter(long new_diam) { SetPosition(m_center_x, m_center_y, new_diam); }
	long		GetDiameter() { return(m_diameter); }

	void		SetBoundingRectPos(long x, long y);
	void		SetHotSpotPos(long x, long y) { SetPosition(x, y, -1); }
	void		GetHotSpotPos(long &px, long &py) { px = m_center_x; py = m_center_y; }
	void		ShiftItem(long shift_x, long shift_y) { SetPosition(m_center_x+shift_x, m_center_y+shift_y, -1); }

	void					OnDraw(HDC hDC, RECT &rc_invalid);
	TScreenItem			*CheckClick(POINT &app_pt);

	inline TBasicStyle	*Style() { return((TBasicStyle*)m_style); }

protected:

	inline void	SetupRcItem(RECT &rc) { rc.left = m_center_x-m_diameter/2; rc.top = m_center_y-m_diameter/2; rc.right = rc.left+m_diameter; rc.bottom = rc.top+m_diameter; }

protected:

	long			m_center_x, m_center_y;			// Position of the circle center.
	long			m_diameter;
};

//
// - - - - - - -  Object 3. Picture  - - - - - - - - -
//

//
//  Bitmap style is more a bitmap itself than just a style. The props of the bitmap
//  can be defined using the structures:
//
//		struct TBitmapStyleProps;
//		struct TIconsGridStyleProps;
//
//  Look for these definitions in WinUi/BitmapHelper.H.
//
class TBitmapStyle : public TScreenItemStyle
{
public:

	TBitmapStyle() { m_hInst = NULL; m_hBmp = NULL; m_icons_grid_mode = FALSE; m_sb_props.Clear(); m_ic_props.Clear(); m_prev_pos_x = m_prev_pos_y = 0; }
	~TBitmapStyle() { if (m_hBmp != NULL) ::DeleteObject(m_hBmp); }

	enum  { type_ID = 0x01030 };

	short	StyleTypeId() const { return(type_ID); }

	bool		Setup(HINSTANCE hInst, const TBitmapStyleProps &props);
	bool		Setup(HINSTANCE hInst, const TIconsGridStyleProps &props);
				// This setup sets up the object only partially. Setup should be finalized with SetupForDeviceContext().
				// It is ok to pass the NULL HINSTANCE. Instance of the executable will be used in this case. Third optional
				// param describes names of the source files that can be used when HTML <img> elements are rendered.

	bool		SetupForDeviceContext(HDC hDC);
				// Method inits the bmp helpers.

	bool		IsIconsGrid() { return(m_icons_grid_mode); }
	void		GetCellSize(SIZE &cell_size);

	void		PaintBitmap(HDC hDC, POINT &app_item_pos, short icon_cell, POINT &local_pos, SIZE &local_size);
	void		PaintBitmapEx(HDC hDC, POINT &left_top_bitmap_pos, short icon_cell, POINT &local_pos, SIZE &local_size);
	bool		CheckClick(POINT &click_pos, POINT &app_item_pos, short icon_cell, SIZE &local_size);

	void		GetLocalRect(short icon_index, POINT &local_icon_pos, SIZE &local_icon_size);
				// If the bitmap is not an icon grid, the first parameter is ignored. Second param receives coordinates
				// of the left top corner of the bounding rect of the icon inside its cell and the third parameter receives
				// the size of the icon.

	void		PrepareItemRect(RECT &rect, POINT &app_item_pos, POINT &icon_pos, SIZE &icon_size);

protected:

	HINSTANCE					m_hInst;
	HBITMAP						m_hBmp;					// This handle is owned.

	bool							m_icons_grid_mode;
	TBitmapStyleProps				m_sb_props;
	TIconsGridStyleProps			m_ic_props;

	TBitmapPaintHelper			m_paint_hlpr;
	TBitmapClickHelper			m_click_hlpr;

private:

	int							m_prev_pos_x, m_prev_pos_y;
									// These fields are used for creating illusion that printer DC is readable.
	friend class TBitmapItem;
};

//
//  Picture.
//
class TBitmapItem : public TScreenItem
{
public:

	TBitmapItem() { m_base_pos.x = m_base_pos.y = 0; m_icon_index = 0; m_icon_local_pos.x = m_icon_local_pos.y = -1; m_html_class_attr = m_html_src_name = NULL; }
	TBitmapItem(short icn_inx) { m_icon_index = 0; m_icon_local_pos.x = m_icon_local_pos.y = -1; SetPosition(0, 0, icn_inx); m_html_class_attr = m_html_src_name = NULL; }
	TBitmapItem(long x, long y, short icn_inx = -1) { m_icon_index = 0; m_icon_local_pos.x = m_icon_local_pos.y = -1; SetPosition(x, y, icn_inx); m_html_class_attr = m_html_src_name = NULL; }

	enum { type_ID = 0x012 };

	short	GetItemTypeId() const { return(type_ID); }
	void		SetStyle(TScreenItemStyle *new_style);

	void		SetPosition(long x, long y, short icn_inx = -1);
	void		SetLeftAlignedPosition(long x_left, long y_hot_spot, short icn_inx = -1);
	void		SetIconIndex(short icn_inx);

	void		SetBoundingRectPos(long x, long y);
	void		SetHotSpotPos(long x, long y) { SetPosition(x, y, -1); }
	void		GetHotSpotPos(long &px, long &py) { px = m_base_pos.x; py = m_base_pos.y; }
	void		ShiftItem(long shift_x, long shift_y) { SetPosition(m_base_pos.x+shift_x, m_base_pos.y+shift_y, -1); }

	void					OnDraw(HDC hDC, RECT &rc_invalid);
	TScreenItem			*CheckClick(POINT &app_click_point);

	inline TBitmapStyle	*Style() { return((TBitmapStyle*)m_style); }

protected:

	void		UpdateDerivedFields();

protected:

	POINT			m_base_pos;
						// Coordinates of the center of the icon in the icon dir or coordinates of the hot spot
						// if the screen item style contains a singe bitmap.

	short			m_icon_index;
						// Field is used if an underlying bitmap is an icon from the icons directory.

	POINT			m_icon_local_pos;
	SIZE			m_icon_local_size;
						// Bounding rect of the icon in its cell. When the icon_local_pos.x and icon_local_pos.y
						// are both equal to -1, this means that the info is not set.

	wchar_t			*m_html_class_attr;
	wchar_t			*m_html_src_name;
						// These fields are used only when the screen item is emitted as part of the HTML page.
						// Both strings are expected to be const and static.

	friend struct TDFVHtmlGenerationContext;
};

//
// - - - - - - -  Object 4. Lines chain - - - - - - - -
//

enum TLineCornerStyle
{
	lcs_sharp,
	lcs_2width,                // Rounding radius is 2 widths of the line.
	lcs_4width,
	lcs_6width,
	lcs_8width,
};

enum TLineRoundingStyle
{
	lrs_straight,              // Segments are straight lines.
	lrs_spline,
};

enum TLineEndingShape : BYTE
{
	les_none,
	les_arrow,
	les_triangle,
	les_circle,
	les_filled_circle,
	les_rect,
	les_filled_rect,
	les_bitmap,
};

//
//  Arrow has the same color as the line unles it is not a bitmap. Arrow has the same shading
//  as the line that it is starting/terminating.
//
struct TLineEndingStyleProps
{
	TLineEndingShape		arrow_shape;
	char						arrow_length;			// In pixels.
	char						arrow_span;				// In pixels.
	char						arrow_width;			// Percent of the line width.
	short					icon_index;
	bool						rotate_bitmap;			// When rotation of the bitmap is ordered, it is expected that
													// picture on the bitmap is pointing in the up direction.
};

//
//  Style of the line or curve.
//
struct TLineStyleProps
{
	TColor					foreground_color;
	int						line_width;				// It is allowed to put 0 here. In this case the line will
													// be 1 pixel wide regardless of the current zoom.
	TLineCornerStyle			corner_style;
	TLineRoundingStyle		rounding_style;

	TLineEndingStyleProps		beg_pole_style;
	TLineEndingStyleProps		end_pole_style;

	void		Clear() { memset(this, 0, sizeof(TLineStyleProps)); }
	void		Init(TColor frgr, int width = 0) { Clear(); foreground_color = frgr; line_width = width; }
};

class TLineStyle : public TScreenItemStyle
{
public:

	TLineStyle() { memset(&m_props, 0, sizeof(m_props)); }
	TLineStyle(TLineStyleProps &prm_props) { Setup(prm_props); }

	enum { type_ID = 0x01040 };

	short	StyleTypeId() const { return(type_ID); }

	bool		Setup(TLineStyleProps &new_props) { m_props = new_props; return(TRUE); }
	bool		Setup(TColor frgr_color, int width = 0) { m_props.Init(frgr_color, width = 0); return(TRUE); }
				// Implementation copies the data from parameter into the data fields of the object.

	TLineStyleProps			m_props;

	friend class TLinesChainItem;
};

//
//     Lines chain (lomanaya linia). Every lines chain should be either empty or it should consist of at least 2 points of data.
//
class TLinesChainItem : public TScreenItem
{
public:

	TLinesChainItem() { m_num_points = 0; m_points_data = NULL; m_points_data_owned = FALSE; }
	TLinesChainItem(long num_pts, POINT *data, TDataBypassMode bypass_mode = dbm_duplicate) { m_points_data_owned = FALSE; SetPosition(num_pts, data, bypass_mode); }
	~TLinesChainItem() { if (m_points_data_owned == TRUE) free(m_points_data); }

	enum { type_ID = 0x013 };

	short	GetItemTypeId() const { return(type_ID); }
	void		SetStyle(TScreenItemStyle *new_style);

	void		SetPosition(long num_pts, POINT *data, TDataBypassMode bypass_mode);

	void		SetBoundingRectPos(long x, long y);
	void		SetHotSpotPos(long x, long y);
	void		GetHotSpotPos(long &px, long &py);
	void		ShiftItem(long shift_x, long shift_y) { if (m_num_points < 1) return; SetHotSpotPos(m_points_data->x+shift_x, m_points_data->y+shift_y); }

	void				OnDraw(HDC hDC, RECT &rc_invalid);
	TScreenItem		*CheckClick(POINT &app_click_point) { return(NULL); }
						// This object type is not clickable.

	inline TLineStyle	*Style() { return((TLineStyle*)m_style); }

	inline long		JctPosX(int jct_index) const { return(m_points_data[jct_index].x); }
	inline long		JctPosY(int jct_index) const { return(m_points_data[jct_index].y); }
	inline long		NumJctPoints() const { return(m_num_points); }

protected:

	void		SetupRcItem();

protected:

	long				m_num_points;
	POINT			*m_points_data;
	bool				m_points_data_owned;
};

template <int _num_segments>
class TLinesChainItemEx : public TLinesChainItem
{
public:

	TLinesChainItemEx() : TLinesChainItem() { m_points_data = m_points_buff; }

	void SetJctPos(int jct_inx, long pos_x, long pos_y, bool final_point = TRUE)
	{
		m_points_buff[jct_inx].x = pos_x;
		m_points_buff[jct_inx].y = pos_y;

		if (final_point == TRUE)
		{
			m_num_points = jct_inx+1;
			SetupRcItem();
		}
	}

	void SetPosition(long num_pts, POINT *data)
	{
		assert(num_pts <= _num_segments+1);

		// Accept the data.
		m_num_points = num_pts;
		memcpy(m_points_buff, data, m_num_points*sizeof(POINT));

		// Update the bounding rect of the item.
		SetupRcItem();
	}

protected:

	POINT		m_points_buff[_num_segments+1];
};

typedef  TLinesChainItemEx<1>		TLinesChainItem1;
typedef  TLinesChainItemEx<2>		TLinesChainItem2;
typedef  TLinesChainItemEx<3>		TLinesChainItem3;
typedef  TLinesChainItemEx<4>		TLinesChainItem4;
typedef  TLinesChainItemEx<5>		TLinesChainItem5;
typedef  TLinesChainItemEx<6>		TLinesChainItem6;
typedef  TLinesChainItemEx<7>		TLinesChainItem7;
typedef  TLinesChainItemEx<8>		TLinesChainItem8;

typedef  TLinesChainItemEx<10>	TLinesChainItem10;
typedef  TLinesChainItemEx<20>	TLinesChainItem20;
typedef  TLinesChainItemEx<30>	TLinesChainItem30;
typedef  TLinesChainItemEx<40>	TLinesChainItem40;

//
// - - - - - -  Object 5. Single line of text  - - - - - - - -
//

enum TTextFontStyle
{
	fst_none		= 0,
	fst_bold			= 1,
	fst_italic		= 2,
};

enum TFrameCornerStyle
{
	fcs_none,
	fcs_1pix_simple,
	fcs_1pix_excl_tip,
	fcs_1pix_add_inner_dot,
	fcs_1pix_excl_tip_add_inner_dot,
	fcs_num_types,
};

//
//  Style of the single line of text.
//
struct TTextStyleProps
{
	wchar_t				*css_style_name;			// The field should be a pointer to a static string or NULL.

	wchar_t				font_name[80];
	short				font_height;					// Value is expressed in points.
	bool					is_bold;

	TColor				foreground_color;			// Foreground color should stay before the bkgr color to simplify
													// initting structs with the constants.
	TColor				background_color;

	TColor				frame_color;				// The frame is painted when the color is not transp AND frame corner style is not fcs_none.
	TFrameCornerStyle	frame_corner_style;

	TColor				shadow_down_color;			// Shadow is present when the color is not transp AND the shadow width is positive.
	short				shadow_down_width;

	short				up_side_adjust;
	short				baseline_adjust;
	short				down_side_adjust;

	unsigned char			front_side_ext;				// This is the left side inner margin that is covered with the background color if any.
	unsigned char			back_side_ext;				// This is the right side inner margin that is covered with the background color if any.

	unsigned char			outer_front_padding;
	unsigned char			outer_back_padding;
							// Note that outer paddings have an unsigned type. They are primarily used
							// for placing the pieces of text in the decorated file views.
public:

	void		Clear() { memset(this, 0, sizeof(TTextStyleProps)); }

	void		Init(const wchar_t *fnt_name, int fnt_height, bool fnt_bold, TColor frgr_color, TColor bkgr_color = cs_transparent);
				// These initializers fill in all fields of the style. Some fields are filled from parameters,
				// the rest of the fields get default values like transp background, no shadow, etc.

	void		SetFrameProps(TColor color, TFrameCornerStyle style) { frame_color = color; frame_corner_style = style; }

	static wchar_t		*GetFrameCornerStyleText(TFrameCornerStyle style, wchar_t *buff_40_chars);
	static bool		GetFrameCornerStyleFromText(TStringPtr &data, TFrameCornerStyle &style);
};

// Middle of the compact props structure. This definition allows to specify the adjustments.
#define  STD_BKGR		cs_transparent,			/* Transparent background.	*/	\
						cs_transparent, fcs_none,	/* No message frame.		*/	\
						cs_transparent, 0			/* The font has no shadow.	*/

// End of the compact props structure starting from the background color.
#define  STD_TXT			cs_transparent,			/* Transparent background.	*/	\
						cs_transparent, fcs_none,	/* No message frame.		*/	\
						cs_transparent, 0,			/* The font has no shadow.	*/	\
						0, 0, 0,					/* No vertical adjustments.	*/	\
						0, 0,						/* No horz extensions.		*/	\
						0, 0						/* No outer paddings.		*/

// Middle part of the compact props structure starting from the frame props.
#define  STD_SIMPLE		cs_transparent, fcs_none,	/* No message frame.		*/	\
						cs_transparent, 0,			/* The font has no shadow.	*/	\
						0, 0, 0					/* No vertical adjustments.	*/

// Short middle part of the compact props structure starting from the frame props.
#define  STD_NO_FRSH	cs_transparent, fcs_none,	/* No message frame.		*/	\
						cs_transparent, 0			/* The font has no shadow.	*/

struct TTextStyleSymbolAdjustInfo
{
	wchar_t				symbol;
	short				vert_shift;
	short				horz_shift;				// Note: The horz shift is independent from the width delta.
	short				width_delta;
};

class TTextStyle : public TScreenItemStyle
{
public:

	TTextStyle() { memset(&m_props, 0, sizeof(m_props)); ClearFontInfo();  m_symbols_adjust_data = NULL; }
	TTextStyle(TTextStyleProps &prm_props, TTextStyleSymbolAdjustInfo *adjust_data = NULL) { m_props = prm_props; ClearFontInfo(); SetupSymbolsAdjustData(adjust_data); }
	~TTextStyle() { if (m_hFont != NULL) ::DeleteObject(m_hFont); }

	enum { type_ID = 0x01050 };

	short	StyleTypeId() const { return(type_ID); }

	bool		Setup(const TTextStyleProps &new_props, const TTextStyleSymbolAdjustInfo *adjust_data = NULL) { m_props = new_props; if (m_hFont != NULL) ::DeleteObject(m_hFont); ClearFontInfo(); SetupSymbolsAdjustData(adjust_data); return(TRUE); }
				// This setup sets up the object only partially. Setup should be finalized by calling the SetupForDeviceContext().

	bool		SetupFrameProps(TColor color, TFrameCornerStyle fc_style) { m_props.SetFrameProps(color, fc_style); return(TRUE); }
	void		SetupSymbolsAdjustData(const TTextStyleSymbolAdjustInfo *data);

	bool		SetupForDeviceContext(HDC hDC);
				// This method loads the font.

	long		GetStringWidth(HDC hDC, const wchar_t *str, long length = -1, long *string_height = NULL, bool block_front_ext = FALSE, bool block_back_ext = FALSE) const;
				// Note that returned height of the string does not depend on the contents of the passed string.
				// It simply reflects the properties of the used font.

	long		GetFittedLen(HDC hDC, long area_width, const wchar_t *str, long length = -1, bool *add_dots_if_truncating = NULL);

	void		DrawFrgrText(HDC hDC, wchar_t *text_body, long text_length, RECT &rc_text, bool block_front_ext = FALSE);
				// Method paints the passed text with the shadow if this is requested. The background is not painted.

	inline long		GetHeight() const			{ return(m_height_above+m_height_below); }
	inline long		HeightAbove() const		{ return(m_height_above); }
	inline long		HeightBelow() const		{ return(m_height_below); }
	inline long		GetBasicWidth() const		{ return(m_basic_char_width); }
	inline HFONT		GetFont() const				{ return(m_hFont); }

	inline TColor		GetFrgrColor() const		{ return(m_props.foreground_color); }
	inline TColor		GetBkgrColor() const		{ return(m_props.background_color); }

protected:

	inline void	ClearFontInfo() { m_hFont = NULL; m_height_above = m_height_below = m_basic_char_width = 0; }
	int			GetSimplePrefix(const wchar_t *text_body, long text_length, TTextStyleSymbolAdjustInfo *&sa_data) const;
	void			DrawTextWithAdjustments(HDC hDC, wchar_t *text_body, long text_length, RECT &rc_text);

	TTextStyleProps					m_props;

	HFONT							m_hFont;
	long								m_height_above;
	long								m_height_below;
	long								m_basic_char_width;

	TTextStyleSymbolAdjustInfo			*m_symbols_adjust_data;				// Only static pointers are allowed. This object is not owning
																			// and it is not releasing this data.
	short							m_symbols_adjust_lookup[128];
										// Array of indexes into the array of symbol adjust records. This array contains fast access
										// data for the first 128 symbols of the UNICODE character set. Unoccupied cell contains zero.
										// Otherwise the cell contains index into symbols adjust data and its MSB bit is set.
	friend class TTextItem;
	friend class TDecoratedFileViewStyle;
	friend class TDFVPrepareScreenItemsContext;
	friend class TDecoratedFileViewLineInfo;
	friend class TDecoratedFileViewScreenItem;
};

//
//     Text string. Adding data items to the this class should be done with great care.
//  Objects of this class will tend to be instantiated more times than other screen item objects.
//  Typical length of the string will be 1-10 chars.
//
//  NB: The hot spot of the screen item is located at the left top corner of the text.
//
class TTextItem : public TScreenItem
{
public:

	TTextItem(TScreenItemStyle *stl = NULL) : TScreenItem(stl) { m_text_body = NULL; m_text_length = 0; m_text_owned = FALSE; m_block_front_ext_and_frame = m_block_back_ext_and_frame = FALSE; }
	TTextItem(const wchar_t *body, long length = -1, TDataBypassMode bp_mode = dbm_use_only) { m_text_owned = FALSE; SetContents(body, length, bp_mode); }
	~TTextItem() { if (m_text_owned == TRUE) free(m_text_body); }

	enum  { type_ID = 0x014 };

	short	GetItemTypeId() const { return(type_ID); }

	const wchar_t		*GetCssStyleName() const { return((m_style != NULL) ? Style()->m_props.css_style_name : NULL); }
						// When the return value is not NULL, this will be the name of equivalent CSS style.
						// This method is used for generating HTML representation of list boxes and decorated files.

	void		SetStyle(TScreenItemStyle *stl);
				// Updatting the style is not modifying the item rect because when the style is changed,
				// it is better to decide on the app level how to update the position/size of the item's rect.

	bool		SetContents(const wchar_t *body, long length = -1, TDataBypassMode bypass_mode = dbm_use_only);
				// Method is just replacing the contents. Item's rect should be set/updated by the app level separately.
				// Note that body with the zero length is a valid data. In some cases this simplifies the code.

	void		SetFrameBlockingFlags(bool block_front, bool block_back) { m_block_front_ext_and_frame = block_front; m_block_back_ext_and_frame = block_back; }

	long		PlaceObject(HWND hCtrl, long pos_x, long baseline_y);
	long		PlaceObject(HDC hDC, long pos_x, long baseline_y);
	void		PlaceAroundCenter(HDC hDC, long pos_center_x, long pos_center_y);
	void		PlaceHorzCentered(HDC hDC, long pos_center_x, long baseline_y);
				// These methods expect that the style and data are already set in the object. First two methods evaluate
				// the width of the object's rect, place the object and return the horz position after the object. Other two methods
				// simply center the text around the given point.

	void		SetVerifiedPosition(const RECT &already_verified_rect) { m_bounding_rect = already_verified_rect; }
				// Method expects that bypassed rect is already verified to conform to the existing/future style and contents.

	void		UpdateSize(HWND hCtrl)	{ PlaceObject(hCtrl, m_bounding_rect.left, m_bounding_rect.top+Style()->HeightAbove()); }
	void		UpdateSize(HDC hDC)		{ PlaceObject(hDC, m_bounding_rect.left, m_bounding_rect.top+Style()->HeightAbove()); }
				// Methods update the size and m_bounding_rect according to the current style and contents.

	inline long		HeightAbove() const	{ return((m_style != NULL) ? Style()->HeightAbove() : 0); }
	inline long		HeightBelow() const	{ return((m_style != NULL) ? Style()->HeightBelow() : 0); }
	inline long		GetBaseline() const		{ return(m_bounding_rect.top+Style()->HeightAbove()); }

	inline wchar_t		*GetBody() const		{ return(m_text_body); }
	inline long		GetLength() const		{ return(m_text_length); }

	void		SetBoundingRectPos(long x, long y) { SetHotSpotPos(x, y); }
	void		SetHotSpotPos(long new_x, long new_y);
	void		GetHotSpotPos(long &px, long &py) { px = m_bounding_rect.left; py = m_bounding_rect.top; }
	void		ShiftItem(long shift_x, long shift_y) { m_bounding_rect.left += shift_x; m_bounding_rect.right += shift_x; m_bounding_rect.top += shift_y; m_bounding_rect.bottom += shift_y; }
				// If bounding rect was fine before the shift, it will remain fine after the shift also.

	void						OnDraw(HDC hDC, RECT &rc_invalid);
	TScreenItem				*CheckClick(POINT &app_point) { return((IsPointInRect(app_point, m_bounding_rect) == TRUE) ? this : NULL); }

	inline const TTextStyle	*Style() const { return((TTextStyle*)m_style); }

protected:

	wchar_t			*m_text_body;
	long				m_text_length;
						// The text is always stored with its length. This allows faster painting and this allows
						// not to keep 0 at the end of the text. The length of the text can be zero.

	bool				m_text_owned;

	bool				m_block_front_ext_and_frame;
	bool				m_block_back_ext_and_frame;

	friend class TDFVPrepareScreenItemsContext;
	friend class TDecoratedFileViewLineInfo;
};

class TTextItemExProto : public TTextItem
{
public:

	TTextItemExProto(TScreenItemStyle *stl = NULL) : TTextItem(stl) { }

	virtual void SetText(const wchar_t *text) = 0;
	virtual void SetText(const wchar_t *text, int text_len) = 0;
	virtual void AppendText(const wchar_t *text, int text_len = -1) = 0;
};

template <int _buffer_len>
class TTextItemEx : public TTextItemExProto
{
public:

	TTextItemEx(TScreenItemStyle *stl = NULL) : TTextItemExProto(stl) { m_text_buff[0] = 0; }

	virtual void SetText(const wchar_t *text)
	{
		if (text != NULL)
		{
			wcsncpy(m_text_buff, text, _buffer_len);
			m_text_buff[_buffer_len-1] = 0;
		}
		else
		{
			m_text_buff[0] = 0;
		}

		SetContents(m_text_buff);
	}

	virtual void SetText(const wchar_t *text, int text_len)
	{
		if (text != NULL)
		{
			if (text_len >= _buffer_len)
				text_len = _buffer_len-1;

			wcsncpy(m_text_buff, text, text_len);
			m_text_buff[text_len] = 0;
		}
		else
		{
			m_text_buff[0] = 0;
		}

		SetContents(m_text_buff);
	}

	virtual void AppendText(const wchar_t *text, int text_len = -1)
	{
		assert(text != NULL);

		if (text_len < 0)
			text_len = (int)wcslen(text);
		if (text_len > _buffer_len-m_text_length-1)
			text_len = _buffer_len-m_text_length-1;

		if (text_len > 0)
		{
			wcsncpy(m_text_buff+m_text_length, text, text_len);
			m_text_buff[m_text_length+text_len] = 0;
			SetContents(m_text_buff);
		}
	}

	void SetTextFmt(const wchar_t *format, ...)
	{
		va_list vargs;
		va_start(vargs, format);
		vswprintf(m_text_buff, _buffer_len, format, vargs);
		va_end(vargs);

		SetContents(m_text_buff);
	}

	void SetTextFmtVargs(const wchar_t *format, va_list vargs)
	{
		vswprintf(m_text_buff, _buffer_len, format, vargs);
		SetContents(m_text_buff);
	}

	const wchar_t		*GetText() const { return((m_text_buff[0] != 0) ? m_text_buff : NULL); }
	int				GetBufferLen() { return(_buffer_len); }

protected:

	wchar_t		m_text_buff[_buffer_len];

};

typedef  TTextItemEx<20>		TTextItem20;
typedef  TTextItemEx<40>		TTextItem40;
typedef  TTextItemEx<60>		TTextItem60;
typedef  TTextItemEx<80>		TTextItem80;

typedef  TTextItemEx<256>	TTextItem256;
typedef  TTextItemEx<1024>	TTextItem1024;
typedef  TTextItemEx<4096>	TTextItem4096;

//
// - - - - - -  Named style props  - - - - - - - -
//

struct TNamedScreenItemStyleProps : public TListItem
{
	TNamedScreenItemStyleProps() { m_style_name[0] = 0; m_style_type_id = 0; m_src_line_num = -1; m_cnt_use = 0; }

	enum { MAX_NAME_LEN = 40 };

	wchar_t		m_style_name[MAX_NAME_LEN];
	short		m_style_type_id;

	union
	{
		__int64				m_num_value;
		TBasicStyleProps		m_basic_props;
		TLineStyleProps		m_line_props;
		TTextStyleProps		m_text_props;
	};

	int			m_src_line_num;				// Line number of the <Name> tag in the XML file.
	int			m_cnt_use;
};

#endif	// ScreenItems_ScreenItems_H


