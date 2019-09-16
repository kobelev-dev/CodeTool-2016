//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	WinUi_BitmapHelper_H
#define	WinUi_BitmapHelper_H

//
// Props for the bitmap that is a simple bitmap, not an icons grid bitmap.
//
struct TBitmapStyleProps
{
	wchar_t		*bitmap_css_style;			// The name of the style is expected to be a static const string.

	short		resource_id;					// Id of the bitmap in resources.
	bool			transp_pixels;				// This field shows if bitmap has transparent pixels or not.
	short		resolution;					// Resolution of the bitmap in dpi. All normal bitmaps that are inteneded
											// to be displayed on the screen should have the value 75 in this field.
											// The value 75 is fixed as a screen resolution.
	short		hot_spot_x, hot_spot_y;

	inline void Clear() { memset(this, 0, sizeof(TBitmapStyleProps)); }
};

//
//  The icons grid is a collection of icons. This collection starts from x_base, y_base pixel of the bitmap.
//  Step between the cells is specified separately from the size of the cell. This allows to have space on
//  the bitmap that does not belong to any icon and may contain delimiting lines. Each cell is identified
//  with an index. Left top cell has an index 100. First cell on the second row has index 200.
//
//  Hot spot of the icon is in the center of the icon cell. There is no way to change the hot spot position.
//
struct TIconsGridStyleProps
{
	wchar_t		*icons_grid_css_style;		// Name of the style is expected to be a const static string. This style should
											// contain properties that are common to all icons of this bitmap. Props of
											// individual icons should be passed in the m_html_class_attr field.
	short		resource_id;					// Id of the bitmap in resources.
	bool			transp_pixels;				// This field shows if bitmap has transparent pixels or not. Index
											// of the transp color is the color index of the pixel (0,0).
	bool			frame_exts_avail;			// Props of the bitmap allows to have individual bounding rects for each icon.
											// In other words, the icon can be smaller than the size of the icon cell. This is
											// implemented using special color coding of the left and top sides of the cell.
											// Note, these are lines that are not part of the cell, but stay right outside it.
											// When pixel on the icon frame is not black, this means that corresponding
											// row/colunm is not part of the icon.
	short		resolution;					// Resolution of the bitmap in dpi. All normal bitmaps that are inteneded to be
											// displayed on the computer screen should have the value 75 in this field.
	short		x_base, y_base;				// Left top pixel of the left top cell of the grid.
	short		x_step, y_step;				// Step between the cells in the grid in pixels.
	short		x_cell, y_cell;				// Size of the grid cell in pixels.

	inline void Clear() { memset(this, 0, sizeof(TIconsGridStyleProps)); }
};

class TBitmapPaintHelper
{
public:
			TBitmapPaintHelper() { memset(this, 0, sizeof(TBitmapPaintHelper)); }
			~TBitmapPaintHelper() { Release(); }

	bool		IsInited() { return((m_hBitmap != NULL) ? TRUE : FALSE); }
	void		Initialize(HWND hWnd, HBITMAP hBmp, float zoom, bool transp_pixels);
	void		Initialize(HDC hDC, HBITMAP hBmp, float zoom, bool transp_pixels);
	void		Paint(HDC hDC, int xOnBmp, int yOnBmp, int dxOnBmp, int dyOnBmp, int xPaint, int yPaint);
	void		ClearTempDC(int dxToClear, int dyToClear);
	void		Release();

protected:

	HBITMAP			m_hBitmap;				// This bitmap is not owned by the instance of the object.
	bool				m_bTranspPixels;
	COLORREF		m_cTranspColor;

	HDC				m_hDCObject;
	HDC				m_hDCMem;
	HDC				m_hDCCopy;

	HBITMAP			m_bmObject;
	HBITMAP			m_bmMem;
	HBITMAP			m_bmCopy;

	HBITMAP			m_bmObjectOld;
	HBITMAP			m_bmMemOld;
	HBITMAP			m_bmCopyOld;

	HBRUSH			m_hWhiteBrush;
};

//
//  This helper is needed only for bitmaps with transparent pixels.
//
class TBitmapClickHelper
{
public:
			TBitmapClickHelper() { memset(this, 0, sizeof(TBitmapClickHelper)); }
			~TBitmapClickHelper() { Release(); }

	bool		IsInited() { return((m_transpMask.m_hDC != NULL) ? TRUE : FALSE); }
	void		Initialize(HDC hDC, HBITMAP hBmp, float zoom);

	bool		CheckClick(int xOffsOnBmp, int yOffsOnBmp);
				// Returned value is TRUE if passed pixel is not trasparent.

	bool		CheckBorder(int xOffsOnBmp, int yOffsOnBmp);
				// Returned value is TRUE if pixel with the passed coordinates is black,
				// in other words the corresponding row/column belongs to the icon.

	void		Release() { m_transpMask.Release(); m_bordersMask.Release(); }

protected:

	struct TDcBmpGroup
	{
		HDC				m_hDC;					// Memory DC.
		HBITMAP			m_MemoryBitmap;		// Bitmap selected into memory DC.
		HBITMAP			m_OldBitmap;			// Old bitmap from created DC.

		void		PrepareAsMask(HDC hDC, HDC hDCTemp, int bmpWidth, int bmpHeight, COLORREF cColor);
		void		Release();
	};

	TDcBmpGroup		m_transpMask;			// Transparency mask info.
	TDcBmpGroup		m_bordersMask;			// Info about icon borders.
};

#endif	// WinUi_BitmapHelper_H


