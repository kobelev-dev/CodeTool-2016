//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#include  "WinUi/BitmapHelper.H"

//-------------------------------------------------------------------------------
//  =====================  TBitmapPaintHelper  ========================
//-------------------------------------------------------------------------------

void TBitmapPaintHelper::Initialize(HWND hWnd, HBITMAP hBmp, float zoom, bool transp_pixels)
{
	HDC hDC = ::GetDC(hWnd);
	assert(hDC != NULL);

	Initialize(hDC, hBmp, zoom, transp_pixels);
	::DeleteDC(hDC);
}

void TBitmapPaintHelper::Initialize(HDC hDC, HBITMAP hBmp, float zoom, bool transp_pixels)
{
	if (IsInited() == TRUE)
		Release();

	// Set up for drawing.
	m_hBitmap = hBmp;
	BITMAP bmp_info;
	::GetObject(hBmp, sizeof(BITMAP), &bmp_info);
	int bmpWidth  = bmp_info.bmWidth;
	int bmpHeight = bmp_info.bmHeight;

	// Create DCs to hold the temporary data.
	HDC hDCTemp = ::CreateCompatibleDC(hDC);
	HDC hDCBack = ::CreateCompatibleDC(hDC);
	m_hDCObject = ::CreateCompatibleDC(hDC);
	m_hDCMem = ::CreateCompatibleDC(hDC);
	m_hDCCopy = ::CreateCompatibleDC(hDC);

	// Select bitmap with icons to create proper memory bitmaps.
	HBITMAP bmTempOld = (HBITMAP)::SelectObject(hDCTemp, m_hBitmap);
	m_bTranspPixels = transp_pixels;
	m_cTranspColor = ::GetPixel(hDCTemp, 0, 0);

	// Create a bitmap for each DC. DCs are required for some GDI functions.
	HBITMAP bmBack = ::CreateBitmap(bmpWidth, bmpHeight, 1, 1, NULL);
	m_bmObject = ::CreateBitmap(bmpWidth, bmpHeight, 1, 1, NULL);
	m_bmMem = ::CreateCompatibleBitmap(hDCTemp, bmpWidth, bmpHeight);
	m_bmCopy = ::CreateCompatibleBitmap(hDCTemp, bmpWidth, bmpHeight);

	// Each DC must select a bitmap object to store pixel data.
	HBITMAP bmBackOld = (HBITMAP)::SelectObject(hDCBack, bmBack);
	m_bmObjectOld = (HBITMAP)::SelectObject(m_hDCObject, m_bmObject);
	m_bmMemOld = (HBITMAP)::SelectObject(m_hDCMem, m_bmMem);
	m_bmCopyOld = (HBITMAP)::SelectObject(m_hDCCopy, m_bmCopy);

	// Copy the bypassed bitmap, otherwise it will be overwritten.
	::BitBlt(m_hDCCopy, 0, 0, bmpWidth, bmpHeight, hDCTemp, 0, 0, SRCCOPY);

	// Set the background color of the hDCCopy to the color, contained in the parts of the bitmap
	// that should be transparent.
	COLORREF cColor = ::SetBkColor(m_hDCCopy, m_cTranspColor);

	// Create the object mask for the bitmap by performing a BitBlt from the source bitmap to
	// the monochrome bitmap.
	::BitBlt(m_hDCObject, 0, 0, bmpWidth, bmpHeight, m_hDCCopy, 0, 0, SRCCOPY);

	// Set the bkgr color of the hDCCopy back to the original color.
	::SetBkColor(m_hDCCopy, cColor);

	// Create the inverse of the object mask.
	::BitBlt(hDCBack, 0, 0, bmpWidth, bmpHeight, m_hDCObject, 0, 0, NOTSRCCOPY);

	// Mask out the transparent colored pixels on the bitmap.
	::BitBlt(m_hDCCopy, 0, 0, bmpWidth, bmpHeight, hDCBack, 0, 0, SRCAND);

	m_hWhiteBrush = ::CreateSolidBrush(RGB(255, 255, 255));

	// Release all temp GDI objects.
	::SelectObject(hDCTemp, bmTempOld);
	::DeleteDC(hDCTemp);
	::SelectObject(hDCBack, bmBackOld);
	::DeleteObject(bmBack);
	::DeleteDC(hDCBack);
}

void TBitmapPaintHelper::Paint(HDC hDC, int xOnBmp, int yOnBmp, int dxOnBmp, int dyOnBmp, int xPaint, int yPaint)
{
	if (m_hBitmap == NULL)
		return;

	// Copy the background of the destination DC to temp DC.
	::BitBlt(m_hDCMem, 0, 0, dxOnBmp, dyOnBmp, hDC, xPaint, yPaint, SRCCOPY);

	// Mask out pixels where bitmap will be placed.
	::BitBlt(m_hDCMem, 0, 0, dxOnBmp, dyOnBmp, m_hDCObject, xOnBmp, yOnBmp, SRCAND);

	// XOR bitmap with the background on the destination DC.
	::BitBlt(m_hDCMem, 0, 0, dxOnBmp, dyOnBmp, m_hDCCopy, xOnBmp, yOnBmp, SRCPAINT);

	// Copy destination DC to the screen.
	::BitBlt(hDC, xPaint, yPaint, dxOnBmp, dyOnBmp, m_hDCMem, 0, 0, SRCCOPY);
}

void TBitmapPaintHelper::ClearTempDC(int dxToClear, int dyToClear)
{
	if (m_hDCMem == NULL)
		return;

	RECT rc = { 0, 0, dxToClear, dyToClear };
	::FillRect(m_hDCMem, &rc, m_hWhiteBrush);
}

void TBitmapPaintHelper::Release()
{
	if (m_bmObject != NULL)
	{
		::SelectObject(m_hDCObject, m_bmObjectOld);
		::DeleteObject(m_bmObject);
		m_bmObject = NULL;
	}

	if (m_bmMem != NULL)
	{
		::SelectObject(m_hDCMem, m_bmMemOld);
		::DeleteObject(m_bmMem);
		m_bmMem = NULL;
	}

	if (m_bmCopy != NULL)
	{
		::SelectObject(m_hDCCopy, m_bmCopyOld);
		::DeleteObject(m_bmCopy);
		m_bmCopy = NULL;
	}

	if (m_hDCObject != NULL)
	{
		::DeleteDC(m_hDCObject);
		m_hDCObject = NULL;
	}

	if (m_hDCMem != NULL)
	{
		::DeleteDC(m_hDCMem);
		m_hDCMem = NULL;
	}

	if (m_hDCCopy != NULL)
	{
		::DeleteDC(m_hDCCopy);
		m_hDCCopy = NULL;
	}

	if (m_hWhiteBrush != NULL)
	{
		::DeleteObject(m_hWhiteBrush);
		m_hWhiteBrush = NULL;
	}

	m_hBitmap = NULL;
}

//-------------------------------------------------------------------------------
//  ====================  TBitmapClickHelper  =========================
//-------------------------------------------------------------------------------

void TBitmapClickHelper::Initialize(HDC hDC, HBITMAP hBmp, float zoom)
{
	if (IsInited() == TRUE)
		Release();

	// Retrieve the size of the bitmap.
	BITMAP bmp_info;
	::GetObject(hBmp, sizeof(BITMAP), &bmp_info);
	int bmpWidth = bmp_info.bmWidth;
	int bmpHeight = bmp_info.bmHeight;

	// Create DCs to hold the temporary data.
	HDC hDCTemp = ::CreateCompatibleDC(hDC);

	// Select bitmap with icons into the temp DC.
	HBITMAP bmTempOld = (HBITMAP)::SelectObject(hDCTemp, hBmp);
	COLORREF cTranspColor = ::GetPixel(hDCTemp, 0, 0);

	// Prepare helpers.
	m_transpMask.PrepareAsMask(hDC, hDCTemp, bmpWidth, bmpHeight, cTranspColor);
	m_bordersMask.PrepareAsMask(hDC, hDCTemp, bmpWidth, bmpHeight, RGB(0, 0, 0));

	// Release temp GDI objects.
	::SelectObject(hDCTemp, bmTempOld);
	::DeleteDC(hDCTemp);
}

bool TBitmapClickHelper::CheckClick(int xOffsOnBmp, int yOffsOnBmp)
{
	COLORREF cColor = ::GetPixel(m_transpMask.m_hDC, xOffsOnBmp, yOffsOnBmp);
	return((cColor != 0) ? FALSE : TRUE);
}

bool TBitmapClickHelper::CheckBorder(int xOffsOnBmp, int yOffsOnBmp)
{
	// Windows works in a way that the black pixel on the bitmap results in 0xFFFFFF pixel on this monochrome mask
	// and any non black pixel results in a 0x000000 pixel. This function should return TRUE if the src pixel is black.
	COLORREF cColor = ::GetPixel(m_bordersMask.m_hDC, xOffsOnBmp, yOffsOnBmp);
	return((cColor != 0) ? TRUE : FALSE);
}

//-------------------------------------------------------------------------
//  ============  TBitmapClickHelper::TDcBmpGroup  ==================
//-------------------------------------------------------------------------

void TBitmapClickHelper::TDcBmpGroup::PrepareAsMask(HDC hDC, HDC hDCTemp, int bmpWidth, int bmpHeight, COLORREF cColor)
{
	// Create memory DC.
	m_hDC = ::CreateCompatibleDC(hDC);

	// Create a mask bitmap. Select it into the memory DC.
	m_MemoryBitmap = ::CreateBitmap(bmpWidth, bmpHeight, 1, 1, NULL);
	m_OldBitmap = (HBITMAP)::SelectObject(m_hDC, m_MemoryBitmap);

	// Set the background color of the hDCTemp to the passed color.
	COLORREF cOldColor = ::SetBkColor(hDCTemp, cColor);

	// Create the object mask for the bitmap by performing a BitBlt from the source bitmap to a monochrome bitmap.
	::BitBlt(m_hDC, 0, 0, bmpWidth, bmpHeight, hDCTemp, 0, 0, SRCCOPY);

	// Set the bkgr color of the hDCTemp back to the original color.
	::SetBkColor(hDCTemp, cOldColor);
}

void TBitmapClickHelper::TDcBmpGroup::Release()
{
	if (m_MemoryBitmap != NULL)
	{
		::SelectObject(m_hDC, m_OldBitmap);
		::DeleteObject(m_MemoryBitmap);
		m_MemoryBitmap = NULL;
	}

	if (m_hDC != NULL)
	{
		::DeleteDC(m_hDC);
		m_hDC = NULL;
	}
}


