//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	WinUi_DialogControlHelpers_H
#define	WinUi_DialogControlHelpers_H

#ifndef   Common_Buffers_H
#include  "Common/Buffers.H"
#endif
#ifndef   Common_FileDataHelper_H
#include  "Common/FileDataHelper.H"
#endif
#ifndef   WinUi_BitmapHelper_H
#include  "WinUi/BitmapHelper.H"
#endif

//
// Color support definitions.
//
typedef DWORD TColor;
#define cs_transparent 0xFFFFFFFF
#define cs_color_type_mask 0xFF000000

//
// Alignment of the arbitrary object like the text string in the column.
//
enum TObjectAlignment : unsigned char
{
	align_left,
	align_center,
	align_right,
};

struct TMenuItemInfo
{
	DWORD				item_flags;		// Menu item flags like: MF_CHECKED, MF_GRAYED, MF_SEPARATOR, etc.

	union
	{
		DWORD			item_id;			// Simple menu item. There should be no MF_POPUP flag in the item flags.
		HMENU			hSubMenu;		// Submenu. MF_POPUP should be present in the flags.
	};

	wchar_t				*item_text;		// The text of the menu item.

	void Setup(DWORD id, wchar_t *msg, DWORD flags = 0) { item_flags = flags; item_id = id; item_text = msg; }
	void SetEndOfMenu() { item_flags = item_id = 0; item_text = NULL; }
};

class TBasicDialog;

class TControlHelper
{
public:

	static void	PaintInsetFrame(HDC hDC, RECT &rect);
	static int		GetInsetFrameWidth() { return(2); }

	static void	CenterOverTheWindow(HWND hWndToCenter, HWND hWndToCenterOver, long offs_x = 0, long offs_y = 0);
	static void	CenterOverTheScreenRect(HWND hWndToCenter, RECT &rcScreenRect);

	static HMENU	CreatePopupMenuHandle(const TMenuItemInfo *info);
					// Passed array or descriptors should be terminated with an element that has NULL value
					// in its item_text field and this item should not be a menu separator.

	static TColor	m_color_select_result_color;
	static TColor	m_color_select_custom_colors[16];
					// Application can pick up the result of the color selection from these fields.

	static bool	ShowColorSelectDialog(HWND hParent, const wchar_t *window_title);
					// Method shows the stnd Windows color selection dialog.

	static void	ShowColorSelectDialogEx(TBasicDialog *parent_dialog, const wchar_t *window_title);
					// Method shows stnd Windows color selection dialog. If user presses OK, the dialog
					// places selected color into the clipboard using the "%d, %d, %d" format.

	static UINT_PTR CALLBACK ColorSelectDialogBoxHook(HWND, UINT, WPARAM, LPARAM);

	static void	PlaceColorToClipboardAsRgbString(TBasicDialog *dialog, TColor color);
	static void	PlaceMessageBufferToClipboard(TBasicDialog *dialog, bool gen_res, TMessageBuffer &buffer);
	static void	PlaceDestinationFileToClipboard(TBasicDialog *dialog, bool gen_res, TDestinationFile &in_memory_buffer, bool show_success_message);

	static DWORD		PutStringToClipboard(const wchar_t *message);
						// Method deletes the previous contents of the clipboard. After that it copies contents of the string
						// into the heap and passes the handle of this memory block to clipboard. Clipboard will take care
						// of this memory. The return value is either ERROR_SUCCESS (0) if everything went fine or
						// a Win32 error in the case of failure.

	static DWORD		PutBitmapToClipboard(HANDLE hBitmap);
						// Method takkes and consumes the passed bitmap regardless of the returned error code.
};

//
// Aliases for some of the well-known Windows fonts.
//
#define   ARIAL				L"Arial"				// 6, 8, 10, 12, 14, 18, 24, 36.
#define   ARIAL_BLACK		L"Arial Black"
#define   COMIC_SANS_MS		L"Comic Sans MS"
#define   COURIER			L"Courier"			// 10, 12, 15.
#define   COURIER_NEW		L"Courier New"		// 6, 8, 9, 10, 11, 12, 14, 16, 18, 20 ...
#define   FIXEDSYS			L"Fixedsys"
#define   HELVETICA			L"Helvetica"
#define   MS_SANS_SERIF		L"MS Sans Serif"		// 8, 10, 12, 14, 18, 24.
#define   MS_SERIF			L"MS Serif"			// 6, 7, 8, 10, 12, 14, 18, 24.
#define   SMALL_FONTS		L"Small Fonts"			// 2, 3, 4, 5, 6, 7.
#define   TAHOMA				L"Tahoma"
#define   TERMINAL			L"Terminal"			// 5, 6, 9, 12, 14.
#define   VERDANA			L"Verdana"

class TFontsListHelper
{
public:

	~TFontsListHelper();

	HFONT	Lookup(HWND hWnd, const wchar_t *fontFamilyName, int fontSize, bool fontBold = FALSE, bool fontItalic = FALSE);

protected:

	struct TFontInfo
	{
		HFONT		m_fontHandle;

		wchar_t		*m_fontName;				// The name of the font is expected to be static.
		int			m_fontSize;
		bool			m_fontBold;
		bool			m_fontItalic;
	};

	TStructsArray<TFontInfo, 2, 16>		m_data;
};

class TControlColorHelper
{
public:

	struct TControlColorInfo
	{
		short		m_ctrl_id;
		TColor		m_frgr_color;
		HBRUSH		m_bkgr_brush;
		bool			m_custom_bkgr_brush;
	};

public:

	~TControlColorHelper();

	TControlColorInfo	*FindControlInfo(short ctrlId);

	bool		AddControlInfo(TControlColorInfo &info);
	bool		DeleteControlInfo(short ctrlId);

protected:

	int		GetIndex(short ctrlId);

	TStructsArray<TControlColorInfo, 2, 16>		m_data;
};

struct THueSatLumColorHelper
{
	double		hue_comp;		// Hue.
	double		sat_comp;		// Saturation.
	double		lum_comp;		// Luminosity.

	TColor		original_color;

	THueSatLumColorHelper() { hue_comp = sat_comp = lum_comp = 0; original_color = 0; }
	THueSatLumColorHelper(TColor color) { Setup(color); }

	void		Setup(TColor color);
	TColor	GetRgb();

	void		AdjustLuminosity(double middle_level, double compressionFactor);
				// For reducing the luminocity of the current color it is necessary to specify
				// the compression factor in the range 0 ... 1.
private:

	double	HueToRgb(double v1, double v2, double vH);
};

class TFileOpenHelper
{
public:

	TFileOpenHelper(HINSTANCE hInst, const wchar_t *files_filter_spec_only_static, const wchar_t *default_extension_only_static = NULL,
					const wchar_t *file_open_title_only_static = NULL, const wchar_t *file_save_title_only_static = NULL);
				//
				//  All params of the ctor should be constant strings. It is ok to pass the hInst as NULL. In this case HISTANCE
				//  of the exe file will be used.
				//
				//  Files filtering specification is list of the NULL terminated string pairs that is followed by one more NULL.
				//  It should look like:
				//
				//		L"Source files (*.cpp)\0*.cpp\0All Files (*.*)\0*.*\0"
				//
				//  OS provides initial directory using its own unclear rules. Because of that it is recommended to setup the initial
				//  directory before using the object. After the first use where the user has presses the ok button, value of
				//  the initial directory is not more used.
				//
				//  File name inside the object is a full file name.
				//

	enum { FILE_OPEN_MAX_PATH = 2*MAX_PATH };

	void		SetInitialDirectory(const wchar_t *path, BOOL path_is_file_name = TRUE);
				// When the value of the second parameter is TRUE, then only the directory part of the passed path is used.

	void		SetFileOpenTitle(const wchar_t *file_open_title_only_static) { m_get_open_file_dialog_title = (wchar_t*)file_open_title_only_static; }

	bool		GetOpenFileName(HWND hParent);
	bool		GetSaveFileName(HWND hParent, bool want_file_overwrite_warning = FALSE);

	const wchar_t		*GetCurrFileName() const { return(m_target_file_name); }
						// The name of the file is empty after the ctor.

	void				SetCurrFileName(const wchar_t *path);
						// It is ok to call this method with the param value NULL to clear the name.

	bool				IsFileNameEmpty() const { return((m_target_file_name[0] != 0) ? FALSE : TRUE); }
	const wchar_t		*GetFileNameExtension() const { return(wcsrchr(m_target_file_name, L'.')); }

protected:

	OPENFILENAMEW		m_ofn_data;
	wchar_t				m_initial_directory[FILE_OPEN_MAX_PATH];

	wchar_t				m_target_file_name[FILE_OPEN_MAX_PATH];
							// This buffer changes either after the call to SetFileName or after a successful call to
							// GetOpenFileName() or GetSaveFileName methods.

	wchar_t				m_file_name_buffer[FILE_OPEN_MAX_PATH];
							// Temp buffer that is passed to Windows.

	bool					m_file_open_request;			// When value of this field is FALSE, this means that this is FileSave request.
	bool					m_file_overwrite_warn;			// This variable is used in GetSaveFileName method only.

	wchar_t				*m_get_open_file_dialog_title;
	wchar_t				*m_get_save_file_dialog_title;

	static int				g_latest_dialog_width;
	static int				g_latest_dialog_height;

	static UINT_PTR CALLBACK FileOpenHelperDialogHook(HWND, UINT, WPARAM, LPARAM);
};

class TGetDirectoryNameHelper
{
public:

	TGetDirectoryNameHelper(HINSTANCE hInst);

	bool		GetDirectoryName(HWND hParent, const wchar_t *instructions_string = NULL);
				// When the return value is TRUE, the dir name buffer contains the full name of the selected directory
				// with backslashes in the middle and without the trailing backslash.

	const wchar_t		*GetCurrDirName() const { return(m_target_dir_name); }
						// The name of the directory is empty after the ctor.

protected:

	static int CALLBACK		BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
	static LRESULT CALLBACK	BrowseFolderSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	bool			m_opened_at_least_once;
	HWND		m_curr_parent;

	wchar_t		m_target_dir_name[TFileOpenHelper::FILE_OPEN_MAX_PATH];
	POINT		m_latest_dialog_pos;
};

// ---------------------- Custom controls  ----------------------------

void		DialogControlsRegisterWindowClasses(HINSTANCE hInst = NULL);
			// Registering windows classes for all custom controls of this file. If instance handle is omitted,
			// instance of the executable will be used.

//
//		Splitter custom control
//  ---------------------------------
//
//    Messages:
//  ------------
//
//  DLG_CTRL_SET_SPLITTER_COLOR:
//
//		wParam	--	ace color of the splitter when it is not dragged.
//					Use cs_transparent value to make the control transparent.
//		lParam	--	color adjustment params.
//
//  DLG_CTRL_SET_SPLITTER_LIMITS:
//
//		wParam	--	lower moving limit, in pixels from either left or top of the dialog.
//		lParam	--	higher moving limit, in pixels from either right or bottom of the dialog.
//
//    Events:
//  ---------------
//
//  WM_COMMAND:
//
//		wParam	--	LO WORD - id of the control.
//					HI WORD - signed difference between the new and the old control positions.
//		lParam	--	window handle of the control.
//

#define	DLG_CTRL_HORZ_SPLITTER		L"DLG_CONTROL_HORZ_SPLITTER"
#define	DLG_CTRL_VERT_SPLITTER		L"DLG_CONTROL_VERT_SPLITTER"

#define	DLG_CTRL_SET_SPLITTER_COLOR		(WM_USER+0)
#define	DLG_CTRL_SET_SPLITTER_LIMITS		(WM_USER+1)

//
//		Checkbox custom control
//  ---------------------------------
//
//   Frame of the checkbox covers whole client area of the control. Icon is painted right from the code. No external bitmap
//   is needed.
//
//   When the control is created, its normal bkgr color and disabled bkgr color are taken from the current Windows scheme.
//   Later they can be modified by sending messages to the control. In any case, the colors that are stored in the variables
//   of the contol are used to paint background all the time.
//
//     Messages:
//  -------------
//
//  DLG_CTRL_SET_CHKBOX_BKGR_COLOR:
//
//		wParam	--	Unused, should be 0.
//		lParam	--	TColor.
//
//  DLG_CTRL_SET_CHKBOX_DISABLED_BKGR_COLOR:
//
//		wParam	--	Unused, should be 0.
//		lParam	--	TColor.
//
//     Events:
//  ---------------
//
//  WM_COMMAND:
//
//		wParam	--	id of the control.
//		lParam	--	window handle of the control.
//

#define   DLG_CTRL_CHKBOX_CONTROL   L"DLG_CONTROL_CHECKBOX"

#define	DLG_CTRL_SET_CHKBOX_BKGR_COLOR				WM_USER
#define	DLG_CTRL_SET_CHKBOX_DISABLED_BKGR_COLOR	WM_USER+1

//
//		Bitmap custom control
//  ---------------------------------
//
//   This control displays one of the icons from the iocns grid. Title of the control may contain ID of the icon.
//
//     Messages:
//  -------------
//
//  DLG_CTRL_SET_BITMAP_PROPS:
//
//		wParam	--	The hInst of the module. It can be NULL if bitmap is stored in resources of the executable.
//		lParam	--	TIconsGridStyleProps*.
//
//  DLG_CTRL_SET_BITMAP_ICON:
//
//		wParam	--	Unused, should be 0.
//		lParam	--	id of the icon on the grid.
//
//   No events.
//

#define   DLG_CTRL_BITMAP_CONTROL   L"DLG_CONTROL_BITMAP"

#define	DLG_CTRL_SET_BITMAP_PROPS		WM_USER
#define	DLG_CTRL_SET_BITMAP_ICON			WM_USER+1

//
//		Brushed static custom control
//  ---------------------------------
//
//   The  face of the control is filled with horizontal lines. Color of the lines is taken from the control title,
//   like "255, 48, 93". If there is no color in the title, the value RGB(128, 128, 128) is used.
//
//   No messages, no events.
//

#define   DLG_CTRL_BRUSHED_STATIC   L"DLG_CONTROL_BRUSHED_STATIC"

#endif	// WinUi_DialogControlHelpers_H


