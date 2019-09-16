//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Annotated files display.
//

#ifndef	ScreenItems_DecoratedFileView_H
#define	ScreenItems_DecoratedFileView_H

#ifndef   Xml_HtmlGenDecors_H
#include  "Xml/HtmlGenDecors.H"
#endif
#ifndef   WinUi_ControlsPositioningHelpers_H
#include  "WinUi/ControlsPositioningHelpers.H"
#endif
#ifndef   ScreenItems_ScreenItemsView_H
#include  "ScreenItems/ScreenItemsView.H"
#endif

class TDecoratedFileViewLineInfo;
class TDecoratedFileViewScreenItem;

struct TDecoratedFileViewStyleProps
{
	wchar_t				*css_style_name;				// Name of the style for the "<table>... </table>" object.
														// Once this field not NULL, it is expected to be a pointer to a static string.
	short				line_nums_width;
	short				marks_area_width;

	short				text_left_offs;
	short				text_right_offs;
	short				text_top_offs;
	short				text_bottom_offs;

	wchar_t				line_nums_format[10];
	TObjectAlignment	line_nums_alignment;
	short				line_nums_offs;					// For left and right alignment types this offset is applied to the left or
														// right side of the nums area respectively. For the centered alignment
														// this is a shift from the middle of the line nums area.
	SIZE				text_area_max;					// Max allowed width/height of the text area in pixels. This area contains
														// the frgr painting list objects. If the actual file data has longer lines,
														// then the horz truncation will happen. If the file data has too many lines,
														// then some of the lines will not be displayed.
	SIZE				bounding_rect_min;				// This is min size of the whole object bounding rect. If this field contains
														// non zero values, the values should be reasonable to allow displaying
														// at least one line of 10 chars in the default text font. Negative values
														// specify the width and height of the area in charplaces and lines.
	TColor				line_nums_bkgr_color;
	TColor				marks_area_bkgr_color;
	TColor				text_area_bkgr_color;

	wchar_t				*line_nums_css_style;
	wchar_t				*marks_area_css_style;
	wchar_t				*text_area_css_style;
							// These styles are used as "class" attributes for the <colgroup> elements of the HTML table. This means
							// that they will be used for painting line numbers, text in the marks area and undecorated parts of the file
							// lines. Name of the CSS style in line nums style and default text style that are passed to Setup() method
							// will not be used. Code expects only static strings in these fields.

	int					shadow_down_width;
	TColor				shadow_down_color;

	TColor				mouse_selection_bkgr_color;
	TColor				mouse_selection_frgr_color;
	short				mouse_selection_horz_ext;

	inline void Clear() { memset(this, 0, sizeof(TDecoratedFileViewStyleProps)); }
};

//
// This structure describes handling of code points in the range 0x0..0x1F.
// Vertical center of the icon is placed on the baseline.
//
struct TNonAsciiSubstsInfo
{
	wchar_t		code_from;				// Starting from.
	wchar_t		code_to;				// Up to and including this value.

	union
	{
		struct
		{
			short	inx_nonsel;			// Index of the icon on the bitmap.
			short	inx_slct;			// Index of the icon on the bitmap.
		};

		DWORD		eof_marker;			// Zero value of the field indicates an end of array.
	};
};

struct TDecoratedFileViewTextProps
{
	TTextStyleProps		nums_props;
	TTextStyleProps		text_props;
	TTextStyleProps		ovfl_props;
};

struct TTabsSpacesViewMode
{
	bool					visible_tabs;
	bool					visible_spaces;

	bool					implicit_multisp_tabs;
							// When this mode is turned ON, every sequence of more than one space is treated as
							// implicit tab. Horz position of the next non space character is calculated as multiplication
							// of the zero based char position inside the line and the char_place_width field.

							//
							//  Note that when the implicit multisp tab is turned ON, every seq of spaces causes the frgr
							//  painting list break regardless of the state of the visible spaces mode flag. Later on based
							//  on this flag the code decides if this seq should generate visible or invisible frgr painting
							//  list object.
							//

	short				tab_width_in_chars;
							// Number of char places that the single tab spacing area occupies.

	short				tab_symbol_width_koef;
	short				char_place_width_koef;
	short				chars_seq_min_width_koef;
							// These koefficients are percents of the basic font width of the default text style. They are
							// used to prepare the tab_symbol_width, char_place_width and chars_seq_min_width fields
							// if those fields are zero. These fields are not used for anything else.
public:

	//
	// In the typical scenario the fields below are not set by the user directly. They are calculated
	// by the code based on the koef fields above.
	//

	short				tab_symbol_width;
							// This is the width of the tab symbol itself. This means that once the tab is present in
							// the file, the current position will be shifted forward at least for this number of pixels.

	short				char_place_width;
							// Widths of the char place in pixels for the tab postion calculations.

	short				chars_seq_min_width;
							// Min allowed width in pixels of the implicit multispace tab.

	inline void Clear() { memset(this, 0, sizeof(TTabsSpacesViewMode)); }

protected:

	void PrepareTabSymbolWidth(TTextStyle *default_text_style)
	{
		short koef = (tab_symbol_width_koef != 0) ? tab_symbol_width_koef : 100;
		long bsw = default_text_style->GetBasicWidth();
		tab_symbol_width = (short)((bsw*koef)/100);
	}

	void PrepareCharPlaceWidth(TTextStyle *default_text_style)
	{
		short koef = (char_place_width_koef != 0) ? char_place_width_koef : 100;
		long bsw = default_text_style->GetBasicWidth();
		char_place_width = (short)((bsw*koef)/100);
	}

	void PrepareCharSeqsMinWidth(TTextStyle *default_text_style)
	{
		short koef = (chars_seq_min_width_koef != 0) ? chars_seq_min_width_koef : 100;
		long bsw = default_text_style->GetBasicWidth();
		chars_seq_min_width = (short)((bsw*koef)/100);
	}

	long		GetCharPlaceIndex(const wchar_t *line, int line_len);
				// This method returns the length of the string in charplaces. While processing the string
				// it makes the tab expansions if needed.

	long		GetExplicitTabWidth(long char_place_index_of_the_tab_symbol_itself, long curr_pix_offset);
				// First parameter is the charplace index, not an index of the shar inside the string.
				// Return value is recommended width of the tab symbol foreground.

	long		GetImplicitTabWidth(long char_place_index_of_the_sym_after_the_seq_of_spaces, long curr_pix_offset);
				// Return value is the recommended width of the TAB symbol foreground.

	friend class TDecoratedFileViewStyle;
	friend class TDecoratedFileViewLineInfo;
};

class TDecoratedFileViewStyle : public TScreenItemStyle
{
public:
			TDecoratedFileViewStyle();
			~TDecoratedFileViewStyle();

	enum { type_ID = 0x02000 };

	short	StyleTypeId() const { return(type_ID); }

	bool		Setup(const TDecoratedFileViewStyleProps &new_props, const TTabsSpacesViewMode &new_tabs_mode, bool xml_escaped_data,
						TTextStyle *nums_style, TTextStyle *default_text_style, TTextStyle *line_horz_ovfl_and_oom_marker_style,
						TBitmapStyle *non_ascii_picts = NULL, const TNonAsciiSubstsInfo *non_ascii_substs_info = NULL);
				// Ownership on all pointers is NOT bypassed. It is ok to pass styles that are not prepared for the device
				// context. Note that horz overflow style should be passed even if the width of the object is not constrained
				// because this style is also used for painting the out of mem marker. Inner and outer margins in the horz
				// overflow style are ignored.

	bool		SetupForDeviceContext(HDC hDC);
				// This method also updates all owned and referenced child styles.

	void		SetTabsSpacesVisibMode(bool visib_tabs, bool visib_spaces) { m_tabs_mode.visible_tabs = visib_tabs; m_tabs_mode.visible_spaces = visib_spaces; }

	void		PrepareTabsSpacesWidthMode(short tab_width_in_chars, short tab_symbol_width_koef = 0, short char_place_width_koef = 0);
				// This method can be called only when the style is already prepared for the device context.

	TDecoratedFileViewStyleProps	&Props()						{ return(m_props); }
	TTabsSpacesViewMode			&TabsMode()					{ return(m_tabs_mode); }

	bool							AssembleXmlEscapes()		{ return(m_xml_escaped_data); }

	const TBasicStyle				*LineNumsBkgr() const			{ return(&m_line_nums_bkgr_style); }
	const TBasicStyle				*MarksAreaBkgr() const		{ return(&m_marks_area_bkgr_style); }
	const TBasicStyle				*TextAreaBkgr() const			{ return(&m_text_area_bkgr_style); }

	const TTextStyle				*LineNumsFrgr() const			{ return(m_line_nums_style); }
	const TTextStyle				*DefaultTextFrgr() const		{ return(m_default_text_style); }
	const TTextStyle				*LineOverflowFrgr() const		{ return(m_line_overflow_style); }

protected:

	bool				CheckForNonAsciiSubsts(wchar_t ch, bool slct_area, short &icon_index);

	TBasicStyle		*LookupOrCreateBkgrStyle(HDC hDC, TColor color, TColor frame_color = cs_transparent);
	TTextStyle		*LookupOrCreateTextStyle(HDC hDC, const TTextStyle *proto_style, TColor color, bool keep_shadow_color, bool ignore_css_name);
	TLineStyle		*LookupOrCreateLineStyle(HDC hDC, TColor color);

protected:

	TDecoratedFileViewStyleProps	m_props;
	TTabsSpacesViewMode			m_tabs_mode;

	bool							m_xml_escaped_data;
									// This flag tells that the file data is expected to be XML escaped. This means that decorated file
									// viewer will seach for these escape sequencies and display the original symbols instead.

	TBasicStyle					m_line_nums_bkgr_style;
	TBasicStyle					m_marks_area_bkgr_style;
	TBasicStyle					m_text_area_bkgr_style;

	TTextStyle					*m_line_nums_style;
	TTextStyle					*m_default_text_style;
	TTextStyle					*m_line_overflow_style;

	TBitmapStyle					*m_non_ascii_subst_picts;
	TNonAsciiSubstsInfo			*m_non_ascii_substs_table;

	TBasicStyle					m_mouse_selection_bkgr_style;

private:

	TList				m_derived_bkgr_styles_list;				// Instances of TBasicStyle.
	TList				m_derived_frgr_styles_list;				// Instances of TTextStyle
	TList				m_derived_tbsp_styles_list;				// Instances of TLineStyle.

	friend class TLangViewSourceFileStyle;
	friend class TDecoratedFileViewLineInfo;
	friend class TDecoratedFileViewScreenItem;
	friend class TDFVAddRemoveDecorsContext;
	friend class TDFVPrepareScreenItemsContext;
};

// - - - - - - - - - TDFV support structures - - - - - - - - -

enum TDecoratedFileWlbkAreaFlags
{
	wlbk_none		= 0,
	wlbk_nums		= 0x0020,
	wlbk_marks		= 0x0010,
	wlbk_ltext		= 0x0004,
	wlbk_mtext		= 0x0002,
	wlbk_rtext		= 0x0001,
	wlbk_text		= 0x0007,
	wlbk_whole_line	= 0xFFFF,
};

#define ANY_DECOR_ZORDER  (short)0x8000

//
//  Internal structure. This stucture is not visible to the application. This class can be used only as data member
//  in the structures that can added to the frgr painting list. There are no strong requirements on these classes.
//
//  The screen_item data field is always non NULL. It always points to the containing object. It should be always
//  possible to determine the type of containing object by calling the screen_item->GetItemTypeId() method.
//  In the moment:
//
//		-- TextItem			->  TDFVFrgrDecoration
//
//		-- BitmapItem		->  TDFVNonAsciiSubstItem
//									- Substitution of the single character.
//
//		-- LinesChainItem		->  TDFVTabOrSpaceItem
//									- Visible tab character.
//									- Visible single space.
//									- Visible multispace tab.
//
class TDFVFrgrPaintListItem : public TListItem
{
public:

	TScreenItem			*screen_item;

	long					pix_len;				// Length of the object in pixels. This value has nothing to do with the width of any screen item
											// that is going to be painted. Frgr painting list item represents one or more characters on the line
											// of the decorated file. These characters of the file occupy "foreground space". This field shows
											// width of this occupied space. All "normal" objects have nonzero value in this field. Only "special"
											// objects, i.e. the horz overflow message and the OOM location marker have zero in this field.
	short				frgr_front_ext;
	short				frgr_back_ext;
							// Every frgr painting list item (except for special ones that can be present only at the end)
							// consists of the frgr area and possible extensions on both sides. Storing these extensions
							// is needed for making conversions from the char_index to horz location and back. In rare
							// cases these extensions can be negative. Location of the frgr object should be considered
							// to be independent from the location of the charplace area that the frgr painting list item
							// represents.
public:

	int		GetNumCoveredChars() const;
				// Method returns how many chars on the line the current frgr item covers.
};

// This is an internal structure. It is not visible to the application.
class TDFVRectDecoration : public TRectItem
{
public:
		TDFVRectDecoration(long ch_beg, long num_ch, short ue, short le) { char_beg = ch_beg; num_chars = num_ch; upper_ext = ue; lower_ext = le; }
		TDFVRectDecoration(WORD mask = 0) { cover_mask = mask; }

	void SetupSpecial(void *dcr_owner, DWORD owner_data, short zorder_code, TBasicStyle *style)
	{
		// This method is used only for processing the upper and lower non file line area decorations.
		SetOwner(dcr_owner, owner_data);
		zorder = zorder_code;
		SetStyle(style);
	}

	void CheckCancelStaticDecor(void *dcr_owner, short zorder_code)
	{
		// This method is used only for processing the upper and lower non file line area decorations.
		if ((dcr_owner == NULL || GetOwner() == dcr_owner) && (zorder_code == ANY_DECOR_ZORDER || zorder == zorder_code))
		{
			// Decoration should be discharged.
			SetOwner(NULL);
			SetStyle(NULL);
		}
	}

public:

	short				zorder;

	// Looking at the instance of the struct it is not possible to tell if this is an inline or a whole line object.
	// Objects of different types are stored in different lists.
	union
	{
		// This struct is relevant for inline decors only.
		struct
		{
			long			char_beg;						// Index of the char inside the line.
			long			num_chars;						// Number of chars is always positive. It cannot be zero.

			short		upper_ext,	lower_ext;
							// Extents can be negative. This means that upper/lower side of the decor is equal to the max
							// upper/lower vert spacing decor that accepts the wlbk color. Effectively in the majority of cases
							// this is the same to the vert border of the whole line. If there are no vert spacing decors at all
							// or none of them is accepting wlbk color, the extents will be equal to the max across all other
							// bkgr decors that are present on the line.
		};

		// This field is relevant for the whole line decors only.
		WORD			cover_mask;					// Set of TDecoratedFileWlbkAreaFlags.
	};
};

// This is an internal structure. It is not visible to the application.
class TDFVFrgrDecoration : public TTextItem
{
public:		//
			// Painting the foreground decoration is ALWAYS split into painting the non transparent background
			// as a rect item regardless if this decor is the topmost or not and painting the topmost fragments of
			// the decor using elements of the frgr painting list. These elements always have a transp bkgr.
			//

		TDFVFrgrDecoration() { paint_link.screen_item = this; }
		TDFVFrgrDecoration(long ch_beg, long num_ch) { char_beg = ch_beg; num_chars = num_ch; paint_link.screen_item = this; }

	TRectItem				bkgr_item;					// The rect of this item is always mapped. The style of this rect item is not NULL
														// if the original decor style has non transparent bkgr or it has frame.
	short					zorder;

	long						char_beg;					// Index of the char inside the line.
	long						num_chars;					// The number of chars is always positive. It cannot be zero.

	TDFVFrgrPaintListItem		paint_link;
};

// This is an internal structure. It is not visible to the application.
class TDFVNonAsciiSubstItem : public TBitmapItem
{
public:		//
			// Non ASCII icon occupies space on the line according to its actual width. The vertical center
			// of the icon cell is placed on the baseline. There is no way to modify this placement.
			//

		TDFVNonAsciiSubstItem() { paint_link.screen_item = this; }

	long						char_inx;					// Zero based char index inside the line.

	TDFVFrgrPaintListItem		paint_link;
};

// This is an internal structure. It is not visible to the application.
class TDFVTabOrSpaceItem : public TLinesChainItem
{
public:
		TDFVTabOrSpaceItem() { paint_link.screen_item = this; num_characters = 1; }

	long						char_inx;					// Zero based char index inside the line.
	long						num_characters;				// The number of chars is not equal to 1 only when object represents an implicit
														// multispace tab. In all other cases the value of this field is one.
	POINT					line_data[10];

	TDFVFrgrPaintListItem		paint_link;

	void GetBoundingRectEx(RECT &rc)
	{
		// This method returns a rect that matches the horz directions of the lines chain and it rezembles
		// the height of some generic font in the vertical direction.
		rc = m_bounding_rect;
		if (NumJctPoints() > 4)
		{
			// This is a tab object.
			rc.top -= 6;
			rc.bottom += 3;
		}
		else
		{
			// This is either a simgle space or a multi space object.
			rc.top -= 10;
			rc.bottom += 2;
		}
	}
};

// This is an internal structure. It is not visible to the application.
struct TDFVSlctDecoration
{
	TRectItem				slct_bkgr_item;				// This field is used for storing the owner pointer and the owner data.
														// When the style of this field is not NULL, it always has non transparent bkgr color.
														// It also cannot have shade, frame, When the caller requests selection with transp
														// bkgr, the style of this field is NULL.
	short					zorder;

	long						char_beg;					// Zero based char index inside the line.
	long						num_chars;					// Number of chars is always positive. It cannot be zero.

	TColor					frgr_color;
	short					beg_end_line_ext;			// Value of the background extensions at the beginning and at at the end of the line
														// if selection starts from the beg of the line or ends at the end of the line.
};

// This is an internal structure. It is not visible to application.
struct TDFVInlineImposedDecoration
{
	TScreenItem				*scr_item;					// This field is used for storing the owner pointer and the owner data.

	short					zorder;						// The zorder is needed only for replacing and for removing the decoration.
	long						char_inx;					// Zero based char index inside the line.

	bool						insert_decor;				// This field indicates if curr decoration describes an imposed decoration (FALSE)
														// or an inserted screen item (TRUE).
	union
	{
		// Imposed decor group of fields.
		struct
		{
			long				sym_border_horz_offs;		// Horz shift of the screen item hot spot from the vertical border between the symbols.
			long				baseline_vert_offs;
		};

		// Inserted decor group of fields.
		struct
		{
			short			insert_horz_order;
			long				insert_vert_offs;				// When the vert offset is zero, the bottom of the passed screen item is placed on
														// the baseline.
		};
	};
};

// This is an internal structure. It is not visible to the application.
// This structure defines extra spacing decors both for horizontal and vertical extra spacing.
struct TDFVExtraSpcDecoration
{
	void						*spc_owner;					// This type of decoration is not clickable. Nevertheless the owner is
	DWORD					spc_owner_data;			// needed for replacing and for removing the decoration.

	short					spc_zorder;

	// Structures with different fields of this union are ALWAYS stored in differrent arrays. There is no way
	// to determine looking at the structure itself if this particular struct defines horz or vertical decoration.
	union
	{
		// These fields are relevant for horz spacing decors and spec tab decors.
		struct
		{
			long				spc_char_inx;				// Zero based char index inside the line.

			bool				spc_spec_tab;

			union
			{
				struct
				{
					long			spc_horz_value_left;				// This field is relevant only for the horz spacing decorations.
					long			spc_horz_value_right;			// This field is relevant only for the horz spacing decorations.
				};

				struct
				{
					short		spc_spec_tab_horz_order;		// This field is relevant only for the spec tab decorations.
				};
			};
		};

		// These fields are relevant for vert decors. Vert space decor can defines an extra spacing above or
		// below the line. In both cases the structures are the same, only they are stored in different arrays.
		struct
		{
			bool				spc_wlbk_use_wlbk_color;
			short			spc_wlbk_value;
		};
	};
};

// This is an internal structure. It is not visible to the application.
struct TDFVLineNumFrgrDecoration
{
	void						*dcr_owner;
	DWORD					dcr_owner_data;
	short					zorder;

	TTextStyle				*lnum_frgr_style;
};

// This is an internal structure. It is not visible to the application.
struct TDFVWholeLineImposedDecoration
{
	short					zorder;
	TScreenItem				*wlimp_scr_item;
};

typedef THeapBasedArray<TDFVSlctDecoration, 2> TDFVSlctDecorsArray;
typedef THeapBasedArray<TDFVInlineImposedDecoration, 2> TDFVInlineImposedDecorsArray;
typedef THeapBasedArray<TDFVExtraSpcDecoration, 2> TExtraSpcDecorsArray;
typedef THeapBasedArray<TDFVLineNumFrgrDecoration, 2> TLineNumFrgrDecorsArray;
typedef THeapBasedArray<TDFVWholeLineImposedDecoration, 2> TWholeLineImposedDecorsArray;

// - - - - - - - - -  Single line of text  - - - - - - - - -

// This class is not visible to the upper layers. This light context is used for adding/removing decorations.
class TDFVAddRemoveDecorsContext
{
public:
			TDFVAddRemoveDecorsContext(TDecoratedFileViewScreenItem *dfv);
			~TDFVAddRemoveDecorsContext();

	HDC GetControlDC();

	TDecoratedFileViewScreenItem		*owner;
	HDC								hDC;
	long								text_x_pos;				// First pixel of the area for painting the text.
	long								line_num;
};

// This class is not visible to the upper layers. This intermediate context is used for rendering screen item positions.
class TDFVPrepareScreenItemsContext : public TDFVAddRemoveDecorsContext
{
public:
			TDFVPrepareScreenItemsContext(TDecoratedFileViewScreenItem *dfv);
			~TDFVPrepareScreenItemsContext() { extra_frgr_lookaside.DeleteAll(); non_ascii_lookaside.DeleteAll(); visib_spc_lookaside.DeleteAll(); }

	long						horz_overflow_msg_width;
								// This value is prepared in the ctor and it is stored as a data member
								// because it is used for evaluation of the line length on each line.

	long						x_limit;							// Max width of the text area or zero if the width of the text
															// is not constrained.
	long						curr_pix;						// Current pixel in the horizontal direction. This is an offset from
															// the beginning of the line.
	bool						horz_overflow;

	TDFVFrgrDecoration		*frgr_area;
	TDFVSlctDecoration		*slct_area;

	int						ctx_ibeg;
	int						ctx_iend;
								// Char place index of the beginning of the current frgr painting list item and index of the char
								// place after its end. Frgr painting list item ALWAYS covers at least one char place on the line.

	long						ctx_frgr_front_ext;
	long						ctx_frgr_back_ext;
								// Note that these extensions can be positive and in rare cases even negative.

	long						ctx_min_extents;
								// This field contains sum of all front extensions from the starting char place and back extensions
								// from the final charplace. This field was added because it is used in horz overflow calculations.
protected:

	bool		VisibleTabs();
	bool		VisibleSpaces();

	bool		SameFrgrDecorBefore(TDFVFrgrDecoration &decor) const;
	bool		SameFrgrDecorAfter(TDFVFrgrDecoration &decor) const;

	bool		SameFrgrDecorBeforeCurrArea() const;
	bool		SameFrgrDecorAfterCurrArea() const;

	void		ClearExtentsInfo() { ctx_frgr_front_ext = ctx_frgr_back_ext = 0; ctx_min_extents = 0; }
	long		GetFrgrHorzBeg() const { return(text_x_pos+curr_pix+ctx_frgr_front_ext); }

	TScreenItem	*GetOwnerItem(TDecoratedFileViewLineInfo *inst) const;

	void		PrepareFrgrItemPlacing(TDecoratedFileViewLineInfo *inst, int ibeg, int iend);
	void		FinalizeFrgrItemPlacing(TDecoratedFileViewLineInfo *inst, TDFVFrgrPaintListItem &pi, long frgr_width);

	TDFVFrgrDecoration		*GetExtraFrgrObject()	{ if (extra_frgr_lookaside.IsEmpty() == FALSE) { TDFVFrgrDecoration *item = (TDFVFrgrDecoration*)extra_frgr_lookaside.GetFirst(); extra_frgr_lookaside.RemoveItem(item); item->paint_link.Clear(); return(item); } return(new TDFVFrgrDecoration()); }
	TDFVNonAsciiSubstItem		*GetNonAsciiObject()		{ if (non_ascii_lookaside.IsEmpty() == FALSE) { TDFVNonAsciiSubstItem *item = (TDFVNonAsciiSubstItem*)non_ascii_lookaside.GetFirst(); non_ascii_lookaside.RemoveItem(item); item->paint_link.Clear(); return(item); } return(new TDFVNonAsciiSubstItem()); }
	TDFVTabOrSpaceItem		*GetVisibSpcObject()		{ if (visib_spc_lookaside.IsEmpty() == FALSE) { TDFVTabOrSpaceItem *item = (TDFVTabOrSpaceItem*)visib_spc_lookaside.GetFirst(); visib_spc_lookaside.RemoveItem(item); item->paint_link.Clear(); return(item); }  return(new TDFVTabOrSpaceItem()); }

	void		RecycleExtraFrgrOblects(TList &list)		{ extra_frgr_lookaside.AppendList(list); }
	void		RecycleNonAsciiOblects(TList &list)		{ non_ascii_lookaside.AppendList(list); }
	void		RecycleVisibSpcOblects(TList &list)		{ visib_spc_lookaside.AppendList(list); }

protected:

	TList				extra_frgr_lookaside;
	TList				non_ascii_lookaside;
	TList				visib_spc_lookaside;
						// When frgr painting lists are being rebuilt, all their extra frgr objects are passed to
						// these lookaside lists in this context object. Later on they can be consumed either
						// in the current line line or maybe in subsequent lines.

	friend class TDecoratedFileViewLineInfo;
};

// This struct is not visible to the upper layers. This context is used during the file setup procedure.
class TDFVDecoratedLineSetupContext : public TDFVPrepareScreenItemsContext
{
public:
			TDFVDecoratedLineSetupContext(TDecoratedFileViewScreenItem *dfv) : TDFVPrepareScreenItemsContext(dfv) { }

	long						line_top;				// This field is updated after each call to the TDecoratedFileViewLineInfo::Setup().
	long						nums_x_pos;			// Left side of the line nums area.
};

// This struct is not visible to the upper layers. This structure is used in an array that holds temporary data
// while rendering the horz structure of the line.
struct TDFVLineRenderCharPlaceInfo
{
	TDFVFrgrDecoration		*m_frgr_overlap_state;
	TDFVSlctDecoration		*m_slct_overlap_state;
								// Information about the topmost frgr/slct decor for the current charplace or NULL
								// if the charplace is not decorated with any frgr/slct decor.

	short					m_frgr_max_inner_front_ext;
	short					m_bkgr_max_inner_front_ext;
								// Max of the inner margins in front of the curr charplace across all bkgr/frgr decors.

	short					m_max_outer_front_ext;
								// Max of the outer paddings in front of the curr charplace across all bkgr AND frgr decors.

	short					m_frgr_max_inner_back_ext;
	short					m_bkgr_max_inner_back_ext;
								// Max of the inner margins after the curr charplace across all bkgr/frgr decors.

	short					m_max_outer_back_ext;
								// Max of the outer paddings after the curr charplace across all bkgr AND frgr decors.

	long						m_horz_spc_front_ext;
	long						m_horz_spc_back_ext;
								// The values of these fields are calculated in 2 steps.
								// On the first step these values include space for inline horz spacings and space for inserted
								// decors. During the second step these values are updated (complex procedure) by the outer
								// paddings of the bkgr/frgr decors. After this merge the outer bkgr/frgr paddings are not
								// more used.

	bool						m_spec_tab_flag;
								// This flag indicates that a special tab is present in front of the current charplace. Note that
								// that upper layer may specify several spec tabs on the same charplace. All these tabs will
								// be silently merged into this flag and they will be rendered as one tab.

	bool						m_frgr_plist_break;

	long						m_painting_list_item_offs;
								// This field is set only in those char places, that start the frgr painting list items.
								// In debug mode all unassigned and intermediate elements contain -1 in this field.

	inline void Clear() { memset(this, 0, sizeof(TDFVLineRenderCharPlaceInfo)); }

	inline long FullFrontExts() const { return(m_frgr_max_inner_front_ext+m_bkgr_max_inner_front_ext+m_horz_spc_front_ext); }
	inline long FullBackExts() const { return(m_frgr_max_inner_back_ext+m_bkgr_max_inner_back_ext+m_horz_spc_back_ext); }
};

// Typedef for rect decor ptr is needed because of the template problems in the MS compiler.
typedef TDFVRectDecoration *TDFVRectDecorationPtr;
typedef TStructsArray<TDFVRectDecorationPtr, 32, 64> TDFVRectDecorPtrsArray;

// This struct is not visible to the upper layers.
struct TDFVHtmlGenerationContext
{
	TDFVHtmlGenerationContext(TDecoratedFileViewScreenItem *dfv, THtmlDocument &rprt)
		: m_owns_ctx(m_low_level_owner_alloc_errors), m_emt_hlpr(rprt) { m_dfv = dfv; m_low_level_owner_alloc_errors = 0; }

	TRawHtmlElement		*FindOrCreateLowLevelOwner(TScreenItem &decoration);

public:

	TDecoratedFileViewScreenItem		*m_dfv;

	THtmlGenDecorOwnersContext		m_owns_ctx;
	THtmlObjectEmittingHelper			m_emt_hlpr;
	THtmlDecoratedLineInfo				m_line_gen;

	int								m_low_level_owner_alloc_errors;
										// The number of owner allocation errors or the number of times when applying
										// decoration failed is not not displayed to the user. Nevertheless all possible points
										// of failure are properly checked and any failure affects the overall boolean result.
};

// This struct is not visible to the upper layers. Instances of this class are stored in the arrays only.
class TDecoratedFileViewLineInfo
{
protected:

	TDecoratedFileViewLineInfo();
	~TDecoratedFileViewLineInfo();

	bool		Setup(TDFVDecoratedLineSetupContext &ctx, const wchar_t *line_beg, long line_len);
				// This method is very close to the constructor. It can be called only once during the life of the object.
				// The return value is FALSE when the system is low on memory.

	void		ShiftY(TDecoratedFileViewScreenItem *owner, long shift_value);

	void		SetPosition(TDecoratedFileViewScreenItem *owner, long new_pos_x, long new_pos_y);
				// Position of the line is the beginning of the first item of the frgr painting list on the baseline level.
				// This position is expressed directly in the appspace coordinates.

	void		PaintBkgrFrgrObjects(HDC hDC, RECT &rc_invalid);
	void		PaintImposedObjects(HDC hDC, RECT &rc_invalid);

	TScreenItem	*CheckClickOrSummarize(TDecoratedFileViewScreenItem *owner, POINT &app_pt, TScreenItemSummarizeInfo *sumry);

	//
	//  All char_beg and char_inx parameters are passed relatively to the beginning of the line.
	//

	bool		AddBkgrDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data,
							short zorder_code, long char_beg, long num_chars,
							TBasicStyle *style, short upper_ext, short lower_ext);
	bool		AddFrgrDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data,
							short zorder_code, long char_beg, long num_chars,
							TTextStyle *style, TBasicStyle *bkgr_style = NULL);
	bool		AddSlctDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data,
							short zorder_code, long char_beg, long num_chars,
							TBasicStyle *bkgr_style, TColor frgr_color, short beg_end_line_ext);
	bool		AddInsertedDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data,
							short zorder_code, long char_inx, TScreenItem *scr_item, short decor_horz_order, long baseline_vert_offs);
	bool		AddImposedDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data,
							short zorder_code, long char_inx, TScreenItem *scr_item, long horz_offs, long baseline_vert_offs);
	bool		AddHorzSpcDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data,
							short zorder_code, long char_inx, long spc_val_left, long spc_val_right);
	bool		AddSpecTabDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data,
							short zorder_code, long char_inx, short decor_horz_order);

	bool		AddWholeLineBkgrDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder_code,
							TBasicStyle *style, WORD cover_mask);
	bool		AddLineNumFrgrDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder_code,
							TTextStyle *style);
	bool		AddWholeLineImposedDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder_code,
							TScreenItem *scr_item, long horz_offs, long baseline_vert_offs);
	bool		AddWholeLineSpacingDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder_code,
							bool above_spacing, short spacing_value, bool accept_whole_line_bkgr_color);

	void		RemoveDecor(TDecoratedFileViewScreenItem *dfv, void *decor_owner, short zorder_code);

	bool		PrepareScreenItems(TDFVPrepareScreenItemsContext &ctx);

	bool		GenerateHtmlLine(TDFVHtmlGenerationContext &gen_ctx, int line_number);

private:

	void		ResetFrgrPaintingList(TDFVPrepareScreenItemsContext &ctx);
	bool		PrepareScreenItemsWithNoDecors(TDFVPrepareScreenItemsContext &ctx);
	bool		PrepareScreenItemsInternal(TDFVPrepareScreenItemsContext &ctx);

	bool		AddSimplePaintingListItem(TDFVPrepareScreenItemsContext &ctx, wchar_t *data, long data_len);
	bool		AddTabOrSpacePaintingListItem(TDFVPrepareScreenItemsContext &ctx, bool tab_object, long char_inx, long object_width_in_chars);
	bool		AddBitmapPaintingListItem(TDFVPrepareScreenItemsContext &ctx, long char_inx, short icon_index);

	bool		AppendHorzOverflowObject(TDFVPrepareScreenItemsContext &ctx);
	bool		AppendOutOfMemFrgrObject(TDFVPrepareScreenItemsContext &ctx);

	void		UpdateInlineBkgrDecors(TDFVAddRemoveDecorsContext &ctx, bool update_both_directions);
	void		PositionImposedDecor(TDFVAddRemoveDecorsContext &ctx, TDFVInlineImposedDecoration &decor);
	void		UpdateWholeLineBkgrDecors(TDFVAddRemoveDecorsContext &ctx);
	void		PositionWholeLineBkgrDecor(TDFVAddRemoveDecorsContext &ctx, TDFVRectDecoration &decor);

	void		GetHorzLocationInfo(TDFVAddRemoveDecorsContext &ctx, long char_beg, long num_chars, RECT &dest_rect);
	bool		ProcessTextStyleVertImpact(TDFVAddRemoveDecorsContext &ctx, TTextStyle *style);
	void		SetLineNumScreenItemStyleAndPosition(TDFVAddRemoveDecorsContext &ctx, long nums_x_pos, TTextStyle *style);
	void		CalcVerticalTextExtents(TDecoratedFileViewScreenItem *dfv);
	bool		ProcessTextExtentsVertImpact(TDFVAddRemoveDecorsContext &ctx);

	void		EvaluateTextStyleExtents(TTextStyle *stl);
	void		EvaluateImposedExtents(TDecoratedFileViewScreenItem *dfv, TScreenItem *imposed_item);

	void		ReportTextAreaInvalid(TDecoratedFileViewScreenItem *owner);
	void		ReportFullAreaInvalid(TDecoratedFileViewScreenItem *owner);

	bool		RemoveVertSpacingDecors(TExtraSpcDecorsArray &info, void *decor_owner, short zorder_code, short &new_ext, short &new_wlbk_ext);
	long		GetFrgrPaintingListItemCharPos(TDecoratedFileViewScreenItem *dfv, TScreenItem *frgr_list_scr_item, long app_pt_x);

	bool			IsSimpleData(TDecoratedFileViewScreenItem *dfv);
	wchar_t		CheckAssembleXmlSeq(const wchar_t *pdata, long data_len, long &seq_len);

	inline long GetFrgrPaintingListWidth()
	{
		long width = 0;
		for (TListIter<TDFVFrgrPaintListItem> iter(m_frgr_painting_list); iter; ++iter)
			width += iter.CurrItem().pix_len;
		return(width);
	}

	inline long	GetWlbkHeightAbove() const	{ return(__max(m_line_num_scr_item.HeightAbove(), m_text_above+m_ext_bkgr_above+m_upper_wlbk_ext)); }
	inline long	GetWlbkHeightBelow() const	{ return(__max(m_line_num_scr_item.HeightBelow(), m_text_below+m_ext_bkgr_below+m_lower_wlbk_ext)); }
					// These methods should be used only for positioning the whole line bkgr decorations.
					// They should not be used for positioning lines on the whole file view.

	inline long	GetFullHeightAbove() const	{ return(__max(m_line_num_scr_item.HeightAbove(), m_text_above+m_ext_bkgr_above+m_upper_extent)); }
	inline long	GetFullHeightBelow() const	{ return(__max(m_line_num_scr_item.HeightBelow(), m_text_below+m_ext_bkgr_below+m_lower_extent)); }
	inline long	GetFullHeight() const			{ return(GetFullHeightAbove()+GetFullHeightBelow()); }
					// Full height methods should be primarily used for arranging lines of the file view.

	inline long	GetUpperLineOverlap() const	{ return(__max(0, m_imposed_above-GetFullHeightAbove())); }
	inline long	GetLowerLineOverlap() const	{ return(__max(0, m_imposed_below-GetFullHeightBelow())); }
					// These method should be used in painting to detect if imposed decorations extend
					// out of the line stripe or not.
private:

	bool					m_tabs_or_substs;			// Tabs, bitmap substs or XML escape sequencies.
	bool					m_space_chars;
	bool					m_space_seqs;
							// These 3 boolean fields depend on the contents of the line data.
							// They are set during the setup process and they never change later.

	long					m_baseline;									// Vertical position of the line's baseline.

	short				m_text_above,		m_text_below;			// Max values across all fonts that are present on the line.
	short				m_ext_bkgr_above,	m_ext_bkgr_below;		// Max values of exts of across all fixed ext background decorations.
	short				m_upper_wlbk_ext,	m_lower_wlbk_ext;		// Max values among whole line vert spacing decors that accept
																	// the whole line bkgr decors color.
	short				m_upper_extent,	m_lower_extent;			// Max values across all whole line extra vert spacing decors.

	short				m_imposed_above,	m_imposed_below;
							// Max values across boundaries of all available inline and whole line imposed decorations
							// on the line if any. Imposed decorations may stick out of the borders of the line.

	bool					m_indirect_hgt_bkgrs;		// This flag indicates presence of the bkgr decors that have the same height
													// as the whole line bkgrs and hence depend on the vert spacing decors that
													// accept the whole line bkgr color.
	bool					m_frgrw_sens_wlbks;		// This flag is TRUE when line contains whole line bkgr decors that depend on
													// the horz position of the end of the frgr painting list.
	bool					m_invalid_scr_items;		// This flag is used to avoid multiple recalculations of visible objects when
													// several decors are added/removed.

	TTextItem20			m_line_num_scr_item;		// The line number should be painted when its style is not NULL.
													// This screen item contains info about the current topmost line number decoration.

	TDFVFrgrDecoration	m_default_text;				// This field contains correct data ptr and the length of the line regardless of
													// the state of decors. This object can also be part of the frgr painting list.

	TList								m_bkgr_decors;					// TDFVRectDecoration instances.
	TList								m_frgr_decors;					// TDFVFrgrDecoration instances.
	TDFVSlctDecorsArray				m_slct_decors;					// TDFVSlctDecoration instances.
	TDFVInlineImposedDecorsArray		m_imposed_decors;				// TDFVInlineImposedDecoration instances.
	TExtraSpcDecorsArray				m_spacing_decors;				// TDFVExtraSpcDecoration instances.

	int								m_spec_tabs_area_index;		// Zero based index into the array of spec tab areas on the file.
																	// The value of the field is -1 if line does not belong to any spec tab area.

	TList								m_whole_line_bkgrs;			// TDFVRectDecoration instances.
	TLineNumFrgrDecorsArray			m_whole_line_lnum_frgrs;		// TDFVLineNumFrgrDecoration instances.
	TWholeLineImposedDecorsArray		m_whole_line_imposed;			// TDFVWholeLineImposedDecoration instances.
	TExtraSpcDecorsArray				m_whole_line_upper_exts;		// TDFVExtraSpcDecoration instances.
	TExtraSpcDecorsArray				m_whole_line_lower_exts;		// TDFVExtraSpcDecoration instances.

	TList								m_extra_frgr_objects;			// TDFVFrgrDecoration instances.
	TList								m_non_ascii_objects;			// TDFVNonAsciiSubstItem instances.
	TList								m_visib_spc_objects;			// TDFVTabOrSpaceItem instances.

	TList								m_frgr_painting_list;				// TDFVFrgrPaintListItem instances. This list contains all frgr objects
																	// that should be painted. The list is not owning its members.
	friend class TDFVPrepareScreenItemsContext;
	friend class TDecoratedFileViewScreenItem;
};

// - - - - - - - - - - - - Major object - - - - - - - - - - - - -

//
// Types of decorations. This enum is not used on the decorated file viewer layer itself.
// Allpications need this enum and it is placed here to centralize these names.
//
enum TDecoratedFileViewDecorType
{
	dfv_no_decor,

	// Inline decorations.
	dfv_bkgr_decor,
	dfv_frgr_decor,
	dfv_slct_decor,
	dfv_inserted_decor,
	dfv_imposed_decor,
	dfv_horz_spc_decor,
	dfv_spec_tab_decor,

	// Whole line decorations.
	dfv_wlbk_decor,
	dfv_line_num_decor,
	dfv_spec_tabs_area,
	dfv_wl_imposed_decor,
	dfv_vert_spc_decor,

	// Whole file decorations.
	dfv_top_bottom_decor,

	dfv_decor_num_types,
};

// This struct is not visible to the upper layers.
struct TDFVSpecTabsArea
{
	void							*m_spt_area_owner;
	DWORD						m_spt_area_odata;
	short						m_spt_area_zorder;

	int							m_line_num_start;
	int							m_num_lines;

	enum { MAX_SUBCOLUMNS = 4, };
			// Current implementation uses simple approach with the fixed upper limit
			// on the max possible number of subcolumns in the file view.

	long							m_spec_tab_offsets[MAX_SUBCOLUMNS-1];

public:

	void ClearLayout()
	{
		for (int isp=0; isp<MAX_SUBCOLUMNS-1; ++isp)
			m_spec_tab_offsets[isp] = -1;
	}

	int GetNumSubColumns() const
	{
		for (int isp=0; isp<MAX_SUBCOLUMNS-1; ++isp)
		{
			if (m_spec_tab_offsets[isp] < 0)
				return(isp+1);
		}

		// All spec tab slots are used.
		return(MAX_SUBCOLUMNS);
	}

	long GetSpecTabOffset(TDFVPrepareScreenItemsContext &ctx, int ispec, long proposed_offs);
};

typedef TStructsArray<TDFVSpecTabsArea, 64, 256> TDFVSpecTabsAreasArray;
			// Line ranges of different spec tab areas cannot overlap.

//
//  Major object.
//
//  Note that right after creation or after calling the method Reset() object is not functional. It cannot be added
//  to any list of screen items. It is necessary to call the method Setup() to make the object functional.
//
class TDecoratedFileViewScreenItem : public TScreenItem
{
public:
			TDecoratedFileViewScreenItem();
			~TDecoratedFileViewScreenItem() { Reset(); }
				// The object is not really operational until its Setup() method is called.

	void		Reset();

	enum { type_ID = 0x050 };

	virtual short		GetItemTypeId() const { return(type_ID); }

	virtual void		SetStyle(TScreenItemStyle *stl) { assert(FALSE); }
						// The style should be never set using this method. This object can be prepared only with
						// the method Setup(). The style and the file data are passed to this method at the same time.

	virtual void		SetBoundingRectPos(long pos_x, long pos_y) { SetHotSpotPos(pos_x, pos_y); }
	virtual void		SetHotSpotPos(long pos_x, long pos_y);
	virtual void		GetHotSpotPos(long &px, long &py) { px = m_bounding_rect.left; py = m_bounding_rect.top; }
	virtual void		ShiftItem(long shift_x, long shift_y) { SetHotSpotPos(m_bounding_rect.left+shift_x, m_bounding_rect.top+shift_y); }

	virtual void				OnDraw(HDC hDC, RECT &rc_invalid_appsp);
	virtual TScreenItem		*CheckClick(POINT &app_pt);
	virtual void				SummarizeScreenItems(TScreenItemSummarizeInfo &info, bool bounding_rect_mode);

	TDecoratedFileViewStyle	*Style() const { return(m_dfv_style); }

	HWND					GetCtrl() const { return(m_view->GetControl()); }
	TScreenItemsViewObject	*GetViewObject() const { return(m_view); }

	inline  bool		IsInited() const		{ return(m_view != NULL && m_dfv_style != NULL); }
	inline  bool		IsUpdated() const	{ return(m_frgr_update_wanted == FALSE && m_vert_delta_pix_value == 0  && m_smth_invalid == FALSE); }

		//
		//  Setup() can be called more than once. Every call to the method Setup() completely destroys the internal state
		//  of the object. All decorations should be reapplied after calling the method Setup().
		//
		//  These are the following major different params cases:
		//
		//		fdata		fdata_len
		//	--------------------------------------------------------
		//		NULL		     x				Missing file. See the note below.
		//          Value		   < 0			Non empty file. Length of the file data is determined using wcslen().
		//          Value		     0				Empty file.
		//          Value		   > 0			Non empty file. Length of the file data is passed explicitly. Passed data
		//									may contain NULL chars inside. This is allowed.
		//
		//    Missing file. In this case params like fdata_first_line, first_visib_line, num_visib_lines are ignored. The data field
		//    m_display_lines is not allocated at all. Visible representation in this case is empty bkgr rects. Line numbers are
		//    not displayed. There is special form of Setup() method with smaller number of params for this case. Setup for
		//    the missing file cannot fail.
		//
		//    Passed file. This means that the file is not missing. At least one line number will be displayed on the bkgr rects.
		//
		//    NB:		All linear file offsets are measured from the beginning of the passed file data regardless of the value
		//			of the fdata_first_line param.
		//

	bool		Setup(TScreenItemsViewObject *view_object, TDecoratedFileViewStyle *style_object, long position_x = 0, long position_y = 0)
							{ return(Setup(view_object, style_object, NULL, 0, dbm_use_only, 0, 1, NULL, 0, dbm_use_only, 1, 0, position_x, position_y)); }

	bool		Setup(TScreenItemsViewObject *view_object,					// This object is needed for obtaining the HDC of the control's
																		// window. Only non NULL value should be passed.
							TDecoratedFileViewStyle *style_object,			// Style object contains the tabs spaces mode info. This param cannot
																		// be NULL.
							const wchar_t *fdata, long fdata_len_in_wchars, TDataBypassMode fd_bp_mode,
																		// The file data can be NULL.
							long		fdata_src_offs = 0,					// Source offset of the first byte of the passed file data.
																		// Passed value is not changed by the Setup() method in any way.
							long		fdata_first_line = 1,					// Line number of the first line of the passed file data.
																		// Passed value is not changed by the Setup() method in any way.
							TSimpleLineInfo *linfo = NULL, long linfo_len = 0, TDataBypassMode li_bp_mode = dbm_use_only,
																		// Lines info should be either missing or it should describe the file data.
							long		first_visib_line = 1,					// Param value can be bigger and smaller than the fdata_first_line.
																		// The passed value is not changed by the Setup() method in any way.
							long		num_visib_lines = 0,					// Param can be 0. This means the file should be displayed
																		// up to its end. In any case at least one line will be displayed.
							long		position_x = 0, long position_y = 0);	// Absolute position of the top left corner of the object.

	bool		SetupTabsSpacesViewMode(bool visib_tabs, bool visib_spaces);
				// Only this part of setup is allowed to be changed "on the fly".

	bool		SetupMinBoundingRectSize(long new_min_size_x, long new_min_size_y);
				// Method updates the bkgr rects if this is needed. Passing negative value in this or that
				// direction preserves the current state of the object in the appropriate dir.

	void		AllowUnupdatedPainting(bool new_value) { m_allow_unupdated_painting = new_value; }
				// This method should be primarily used in the debug environment.

	//
	//  Adding/removing the INLINE decorations.
	//

	bool		AddBkgrDecoration(void *decoration_owner, DWORD owner_data, short zorder_code,
								long area_beg, long area_len, TBasicStyle *style, short upper_ext = 0, short lower_ext = 0, bool *smth_applied = NULL);
	bool		AddFrgrDecoration(void *decoration_owner, DWORD owner_data, short zorder_code,
								long area_beg, long area_len, TTextStyle *style, bool *smth_applied = NULL);
	bool		AddSelectionDecoration(void *decoration_owner, DWORD owner_data, short zorder_code,
								long area_beg, long area_len, TColor bkgr_color, TColor frgr_color, short beg_end_line_ext, bool *smth_applied = NULL);
	bool		AddInsertedDecoration(void *decoration_owner, DWORD owner_data, short zorder_code,
								long file_offset, TScreenItem *scr_item, short decor_horz_order, long baseline_vert_offs, bool *smth_applied = NULL);
	bool		AddImposedDecoration(void *decoration_owner, DWORD owner_data, short zorder_code,
								long file_offset, TScreenItem *scr_item, long horz_offs, long baseline_vert_offs, bool *smth_applied = NULL);
	bool		AddHorzSpacingDecoration(void *decoration_owner, DWORD owner_data, short zorder_code,
								long file_offset, long spacing_value_left, long spacing_value_right, bool *smth_applied = NULL);
	bool		AddSpecTabDecoration(void *decoration_owner, DWORD owner_data, short zorder_code,
								long file_offset, short decor_horz_order, bool *smth_applied = NULL);
					//
					// The vert extents in the bkgr decor can be negative. This means that the upper/lower side of the decor will be equal to
					// the max upper/lower vert spacing decor that accepts the wlbk color. In the majority of cases this is the same to the vert
					// border of the whole line. If there are no vert spacing decors at all or none of them is accepting the wlbk color, the extents
					// will be equal to the max height across all other bkgr decors that are present on the line.
					//
					// Note that inline bkgr decorations can be fully invisible but they can be clickabe.
					// The horz spacing decors are not clickable by their nature.
					//
	//
	//	Adding/removing the WHOLE LINE decorations.
	//
	//	Cover mask below is a set of TDecoratedFileWlbkAreaFlags enum members.
	//

	bool		AddWholeLineBkgrDecoration(void *decoration_owner, DWORD owner_data, short zorder_code, long line_beg, long num_lines,
								TBasicStyle *style, WORD cover_mask, bool *smth_applied = NULL);
	bool		AddLineNumFrgrDecoration(void *decoration_owner, DWORD owner_data, short zorder_code, long line_beg, long num_lines,
								TTextStyle *style, bool *smth_applied = NULL);
	bool		AddSpecTabsAreaDecoration(void *decoration_owner, DWORD owner_data, short zorder_code, long line_beg, long num_lines,
								bool *smth_applied = NULL);
	bool		AddWholeLineImposedDecoration(void *decoration_owner, DWORD owner_data, short zorder_code, long line_number,
								TScreenItem *scr_item, long horz_offs, long baseline_offs, bool *smth_applied = NULL);
	bool		AddWholeLineSpacingDecoration(void *decoration_owner, DWORD owner_data, short zorder_code, long line_number,
								bool above_spacing, short new_spacing_value, bool accept_wlbk_color, bool *smth_applied = NULL);
					//
					// Note that whole line bkgr decorations can be fully invisible but still clickabe.
					//

	//
	//	Adding/removing the WHOLE FILE decorations.
	//

	bool		AddTopBottomAreaBkgrDecoration(void *decoration_owner, DWORD owner_data, short zorder_code,
								bool cover_top_area, bool cover_bottom_area,
								TBasicStyle *style, WORD cover_mask, bool *smth_applied = NULL);
				// The top and bottom areas are the area above the first line and the below the last line of the file. This area is
				// present when the text_top_offs and/or text_bottom_offs fields of the decorated file style props have positive value.

	//
	//	Misc handling.
	//

	void		RemoveDecoration(void *decor_owner, short zorder_code = ANY_DECOR_ZORDER);
				// This method allows removing either some or all existing decorations. When the passed decor_owner is NULL,
				// this means that decors with any owner with the specified zorder will be removed. When the zorder code is
				// ANY_DECOR_ZORDER, this means that no zorder filtering will take place.
				// The call RemoveDecoration(NULL) will remove all decorations.

	bool		UpdateControl();
				// It is unlikely that decors will be added/removed one by one. It is more lilely that groups of hundreds
				// or even thousands decorations will be added/removed as one action. So, it makes sence to postpone some
				// work that can be done for many decors at once.

	bool		GetSelectionPosition(TColor bkgr_color, TColor frgr_color, long &sel_beg, long &sel_len);
				// Return value is TRUE when selection with passed pair of colors is non empty.
				// It is possible to retrieve only the first segment of selection if selection has multiple segments.

	bool		GetLineNumberFromFileOffs(long file_offs, long &line_num, long &char_pos);
				// Method will return FALSE if bypassed offset does not belong to any line. Character postions start from 1.
				// This is position, not an offset.

	long		GetLineNumberFromPoint(POINT &app_pt);
				// Method returns either the line number or -1.

	bool		GetFileAreaFromPoint(POINT &app_pt, long &area_beg, long &area_len);
				// When the clicked point is to the left or to the right of the text itself, the return value is TRUE and the beg offset
				// or the end offset of the line is returned. When clicked point is inside the line, the return value is again TRUE and
				// method can return either empty of non empty (typically 1 char) area. The return value is FALSE only when
				// the clicked point is above or below the visible lines.

	TDecoratedFileWlbkAreaFlags	GetHorzAreaTypeFromPoint(POINT &app_pt);

	// This enum is used only as parameter to TDecoratedFileViewScreenItem::GetLineHeightInfo() method.
	enum TLineHeightType
	{
		dfvlh_frgr_exts,				// Extents reflect the biggest fonts that are used on the line.
		dfvlh_bkgr_exts,				// Background decors may be higher/lower than fonts on the line.
		dfvlh_wlbk_colored_exts,	// These extents include extra spacing that use whole line bkgr decors color.
		dfvlh_full_exts,				// Bkgr decors and all extra spacings. These extents constitute the line height.
		dfvlh_complete_exts,		// Everything including sticking out of the line imposed decors.
	};

	bool		GetLineHeightInfo(long line_number, long &baseline, long &hi_ext, long &lo_ext, TLineHeightType line_measure_type = dfvlh_full_exts);

	long		GetNumFileDataLines() { return(m_lines_info_len); }
				// This is number of lines in available file data regardless if this file data is completely visible
				// or it is visible only partly or it is not visible at all.

	long		GetHorzScrollForFirstLineOfFileArea(long area_beg, long area_len, long curr_scroll, long window_width,
											long show_at_least_pixels_left, long show_at_least_pixels_right);
	long		GetScrollPosForLinesRange(long line_beg, long line_end, long curr_scroll, long window_height,
											long show_at_least_pixels_above, long show_at_least_pixels_below,
											bool inititial_frame_placement);
				//
				//  Return value is the scroll pos in pixels of the whole scene that is displayed inside the ScreenItemsView control.
				//  Current scroll and window height tell current vertical scroll situation in the whole screen items view object.
				//  Method returns original curr_scroll in case of bogus parameters or empty file.
				//
				//  Initial placement:	When it is on, requested area will be displayed at the top of the window
				//					even if it is located somewhere in the middle. This can be called "hard" scrolling.
				//					When initial placement is off, the scrolling is more "soft". This means that window
				//					is scrolled only when part of requested area is not visible.
				//

	bool		IntersectWithVisibleFileArea(long &area_beg, long &area_len, bool allow_zero_length_area);
	bool		IntersectWithVisibleLinesRange(long &line_beg, long &num_lines);
				// Methods adjust passed data to the visible area of the file. Return value shows if there is non empty intersection or not.

	//
	//	HTML generation.
	//

	virtual THtmlGenAppDelegate *GetHtmlOwnerDelegate(TScreenItem *decoration) { return(NULL); }
				//
				// Returned object is responsible for emitting the application layer attributes. All other aspects, like the name
				// of the Html element, its CSS style, contents, etc is handled by the current layer. Ownership on the object
				// remains at the application layer.
				//
				// When application returns a NULL pointer but the decoration has non NULL CSS style in its props,
				// an appropriate html element is still generated.
				//

	bool		GenerateBasicHtmlPage(THtmlDocument &rprt, const wchar_t *page_title = NULL, const wchar_t *css_dir_or_url = NULL, const wchar_t **css_file_names = NULL, int line_from = 1, int num_lines = 0);
	bool		GenerateHtmlTable(THtmlDocument &rprt, int line_from = 1, int num_lines = 0);

				//
				// When the name of the CSS files directory (or URL) and the name of the CSS file are both NULLs, method
				// generates only the "<div>...</div>" part of the page. Otherwise it generates a complete HTML page.
				//
				//	For now only the following info is important:
				//
				//		--	Names of the CSS styles for the whole table and its three major columns.
				//		--	CSS styles of the inline background decorations.
				//		--	CSS styles of the inline foreground decorations.
				//
				// The return value is FALSE when at least one of internal allocations fail.
				//
protected:

	static long	GetScrollPos(long req_beg, long req_end, long curr_scroll, long window_height,
								long pixels_above, long pixels_below, long obj_top, long obj_height,
								bool inititial_placement, bool use_smaller_shift);

	void		SetupMaxTextAreaSize(long &def_font_heigth);
	void		SetupMinTextAreaSize(long bounding_rect_min_x, long bounding_rect_min_y);

	void		RequestFrgrUpdateForLinesRange(long line_num_start, long num_lines);
	void		ReportFrgrUpdateWanted(TDecoratedFileViewLineInfo *dfv_line);
	void		SetupFrgrUpdate(TDecoratedFileViewLineInfo *dfv_line);

	bool		PrepareForegroundScreenItems();
	void		UpdateObjectWidth();

	void		ReportVertDelta(long new_delta, long line_num);
	void		CheckAndApplyVertDelta(long line_num);
	void		ApplyAndDismissVertDelta();
	void		UpdateObjectHeight();

	void		AddToInvalidRect(RECT &rc);
	void		ReportInvalidRect();

	TDecoratedFileViewLineInfo	*IterateVisibleFileSpace(TFileSpaceIterationInfo &info);
	TDecoratedFileViewLineInfo	*IterateVisibleLinesSpace(long &line_beg, long &num_lines);

	TDecoratedFileViewLineInfo	*GetDFVLineInfo(long line_number);
	TDecoratedFileViewLineInfo	*GetDFVLineInfoFromVertPos(long app_pos_y);
								// These methods can return only lines that belong to the file. They cannot return the line info
								// objects that describe the lines range overflow lines.

	long		GetLineNumFromLineInfo(TDecoratedFileViewLineInfo *dfv_line);

	int		SplitWlbkCoverMask(WORD cover_mask, WORD mask_fragments[3]);
				// Return value is the number of discovered fragments.

protected:

	TScreenItemsViewObject			*m_view;
										// Typical screen item does not have the link to its view object. This field is needed here because
										// adding/removing decors requires acces to the DC of the current window. This is why this data
										// field is present here. This field should be used to detect if object contains some file or not.

	TDecoratedFileViewStyle			*m_dfv_style;
										// This class is not using the TScreenItem::m_style field at all. This allows using the m_style field
										// in the core object by the derived classes.

	SIZE							m_max_text_size;
	SIZE							m_min_text_size;

	//
	// Information about the data in the file.
	//

	wchar_t							*m_file_data;					// Can be NULL.
	long								m_file_data_len;					// Length of the file data in wchars.
	bool								m_file_data_owned;
										//
										//  The major data member in this group is the m_file_data.
										//
										//	  When the m_file_data is not NULL, the length of the file data is stored in m_file_data_len.
										//	  It can be still 0. The file data may contain one or more zero characters inside the data or
										//	  at the end of the data.
										//
										//    If the passed file is missing, the value of this field is NULL. In this case the rest of the fields
										//    in this group and many other fields are ignored. In this case the object still displays bkgr rects
										//    with the height of one line and without any line number.
										//

	long								m_file_data_src_offs;			// The source offset of the first byte in the file data. This field is used
																	// for processing offsets of the areas that are occupied with decorations.
																	// In a typical case the value of this field is zero.
	long								m_file_data_first_line;			// Line number of the first byte of the file data. Even when the length
																	// of the file data is zero, this file consists of one line and this line has
																	// a line number.
	//
	// Information about the line structure of the file data.
	//

	TSimpleLineInfo					*m_lines_info;					// The value is NULL when the file data is NULL.
	long								m_lines_info_len;				// Number of lines in the file data. Can be zero.
	bool								m_lines_info_owned;
										//
										// If the source file is not missing, i.e. the m_file_data != NULL, then m_lines_info != NULL and
										// m_lines_info_len > 0. The lines info describe the file data as it is.
										//
	//
	// Information about the displayable lines.
	//

	TDecoratedFileViewLineInfo			*m_display_lines;				// The value of this field can NOT be NULL in the inited object.
	long								m_display_lines_len;				// Number of elements in the m_display_lines array.
	long								m_display_lines_first_line;		// Line number of the first displayable line. The value of this field is always
																	// exactly equal to the value that was passed to the Setup() method.
										//
										// Typically the file will have data for all displayable lines. But this may not always be the case.
										// This class displays special message when displayable line is not existing in the file.
										//
										//   The number of display lines is ALWAYS positive (if the setup went fine).
										//

	long								m_first_isect_line;
	long								m_num_isect_lines;
										// Information about the intersection between the file data and the requested range of lines
										// to display. This intersection can be empty. In this case the m_num_isect_lines is equal to 0.

	SIZE							m_text_size;
										// Current width and height of all lines in the m_display_lines. Note that it may happen that some
										// lines from m_display_lines are not visible because of the height constrains. But even in this case
										// this data field stores the total height of all m_display_lines lines.

	long								m_num_visible_lines;
	long								m_visible_lines_height;
										// Number of lines that participate in the painting process. This value can be less than m_display_lines_len
										// only when the m_max_text_size.cy != 0.

	TDFVSpecTabsAreasArray			m_spec_tab_areas;
										// Information about the ranges of lines that share the horz positions of their spec tabs.

	TDFVRectDecoration				m_top_area_wlbk_decors[3];
	TDFVRectDecoration				m_bottom_area_wlbk_decors[3];
										// Whole line decorations for the area above and below the visible lines. Each array has
										// elements for line nums area, marks area and the text area. Three elements in total.

	TRectItem						m_nums_rect_scr_item;
	TRectItem						m_marks_rect_scr_item;
	TRectItem						m_text_rect_scr_item;
										// Screen items for painting the background of the object if the background of the corresponding
										// area is not transparent.

	bool								m_frgr_update_wanted;
	TDecoratedFileViewLineInfo			*m_min_invalid_frgr_line;
	TDecoratedFileViewLineInfo			*m_max_invalid_frgr_line;
										// These data fields sum up the need for updating the screen items on their lines.

	long								m_vert_delta_pix_value;
	long								m_vert_delta_line_num;
										// These data fields store the state of the pending vertical lines shift. The first field is the pending
										// shift value in pixels. If it is zero, this means that shift is not pending. The second field contains
										// the line number of the last accurate line. When the pending shift is zero, the second field is
										// never checked.

	bool								m_allow_unupdated_painting;
										// This flag prevents assert when painting happens in an unupdated state.

	bool								m_smth_invalid;
	RECT							m_rc_invalid;
	RECT							m_old_bounding_rect;
										// When some decoration is added or view mode is changed, object is not initiating repaint itself.
										// It only sums up the bounding rect of the invalid area. It is the responcibility of the application
										// to force repainting by calling the UpdateControl() method.

	TDFVFrgrDecoration				m_out_of_mem_message;
	TDecoratedFileViewLineInfo			*m_out_of_mem_msg_line;
										// This is a foreground message that points to the place in the file where out of memory situation
										// happened.

	TDFVLineRenderCharPlaceInfo		*m_line_rendering_info;
										// This array is allocated with the length of the longest line in the file plus one element. It is used
										// as temporary buffer when the foreground painting lists of the lines are prepared and when
										// the HTML representation of the lines is rendered.

	long								m_line_render_info_len;
										// This is length of the current line that is being rendered. This not the length of the whole array
										// of the rendering info structs as it was allocated.
private:

	TBasicStyle						m_default_bkgr_style;
										// This default bkgr is transparent and it is not visible from outside. It is needed to make
										// the object paintable if there is no derived class that wants to use the m_style field.

	friend class TDecoratedFileViewLineInfo;
	friend class TDFVAddRemoveDecorsContext;
	friend class TDFVPrepareScreenItemsContext;

	friend struct TDFVSpecTabsArea;
};

// - - - - - - - - - - Inline methods - - - - - - - - - - - - -

inline int TDFVFrgrPaintListItem::GetNumCoveredChars() const
{
	int nch = 1;
	if (screen_item->GetItemTypeId() == TTextItem::type_ID)
		nch = ((TDFVFrgrDecoration*)screen_item)->num_chars;
	else if (screen_item->GetItemTypeId() == TLinesChainItem::type_ID)
		nch = ((TDFVTabOrSpaceItem*)screen_item)->num_characters;

	assert(nch > 0);
	return(nch);
}

inline TDFVAddRemoveDecorsContext::TDFVAddRemoveDecorsContext(TDecoratedFileViewScreenItem *dfv)
{
	owner = dfv;
	hDC = NULL;
	text_x_pos = dfv->m_text_rect_scr_item.GetBaseLeft()+dfv->m_dfv_style->m_props.text_left_offs;
	line_num = -1;
}

inline TDFVAddRemoveDecorsContext::~TDFVAddRemoveDecorsContext()
{
	if (hDC != NULL)
		::ReleaseDC(owner->GetCtrl(), hDC);
}

inline HDC TDFVAddRemoveDecorsContext::GetControlDC()
{
	if (hDC != NULL)
		return(hDC);

	hDC = ::GetDC(owner->GetCtrl());
	assert(hDC != NULL);
	return(hDC);
}

inline TDFVPrepareScreenItemsContext::TDFVPrepareScreenItemsContext(TDecoratedFileViewScreenItem *dfv)
		: TDFVAddRemoveDecorsContext(dfv)
{
	hDC = ::GetDC(owner->GetCtrl());
	assert(hDC != NULL);

	if (owner->m_max_text_size.cx > 0)
		horz_overflow_msg_width = dfv->m_dfv_style->m_line_overflow_style->GetStringWidth(hDC, L"...", 3)+3;
}

inline bool TDFVPrepareScreenItemsContext::VisibleTabs()
{
	return(owner->m_dfv_style->m_tabs_mode.visible_tabs);
}

inline bool TDFVPrepareScreenItemsContext::VisibleSpaces()
{
	return(owner->m_dfv_style->m_tabs_mode.visible_spaces);
}

inline bool TDFVPrepareScreenItemsContext::SameFrgrDecorBefore(TDFVFrgrDecoration &decor) const
{
	if (decor.char_beg <= 0)
		return(FALSE);

	TDFVFrgrDecoration *prev_frgr = owner->m_line_rendering_info[decor.char_beg-1].m_frgr_overlap_state;
	if (prev_frgr == NULL)
		return(FALSE);

	// Decors are considered equal if their frgr and bkgr styles both match.
	return(prev_frgr->Style() == decor.Style() && prev_frgr->bkgr_item.Style() == decor.bkgr_item.Style());
}

inline bool TDFVPrepareScreenItemsContext::SameFrgrDecorAfter(TDFVFrgrDecoration &decor) const
{
	if (decor.char_beg+decor.num_chars >= owner->m_line_render_info_len)
		return(FALSE);

	TDFVFrgrDecoration *next_frgr = owner->m_line_rendering_info[decor.char_beg+decor.num_chars].m_frgr_overlap_state;
	if (next_frgr == NULL)
		return(FALSE);

	// Decors are considered equal if their frgr and bkgr styles both match.
	return(next_frgr->Style() == decor.Style() && next_frgr->bkgr_item.Style() == decor.bkgr_item.Style());
}

inline bool TDFVPrepareScreenItemsContext::SameFrgrDecorBeforeCurrArea() const
{
	if (ctx_ibeg <= 0)
		return(FALSE);

	TDFVFrgrDecoration *prev_frgr = owner->m_line_rendering_info[ctx_ibeg-1].m_frgr_overlap_state;
	if (prev_frgr == NULL && frgr_area == NULL)
	{
		// Both char places are not decorated.
		return(TRUE);
	}
	else if (prev_frgr != NULL && frgr_area != NULL)
	{
		// Decors are considered equal if their frgr and bkgr styles both match.
		return(prev_frgr->Style() == frgr_area->Style() && prev_frgr->bkgr_item.Style() == frgr_area->bkgr_item.Style());
	}

	// The state of decors is different.
	return(FALSE);
}

inline bool TDFVPrepareScreenItemsContext::SameFrgrDecorAfterCurrArea() const
{
	if (ctx_iend >= owner->m_line_render_info_len)
		return(FALSE);

	TDFVFrgrDecoration *next_frgr = owner->m_line_rendering_info[ctx_iend].m_frgr_overlap_state;
	if (next_frgr == NULL && frgr_area == NULL)
	{
		// Both char places are not decorated.
		return(TRUE);
	}
	else if (next_frgr != NULL && frgr_area != NULL)
	{
		// Decors are considered equal if their frgr and bkgr styles both match.
		return(next_frgr->Style() == frgr_area->Style() && next_frgr->bkgr_item.Style() == frgr_area->bkgr_item.Style());
	}

	// The state of decors is different.
	return(FALSE);
}

inline TScreenItem *TDFVPrepareScreenItemsContext::GetOwnerItem(TDecoratedFileViewLineInfo *inst) const
{
	if (slct_area != NULL)
		return(&(slct_area->slct_bkgr_item));
	else if (frgr_area != NULL)
		return(frgr_area);

	// Current char place is not decorated.
	return(&(inst->m_default_text));
}

#endif	// ScreenItems_DecoratedFileView_H


