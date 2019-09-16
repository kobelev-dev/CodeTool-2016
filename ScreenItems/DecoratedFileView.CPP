//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//   Annotated files display. July 2011 - considerable rework of the horz foreground.
//

#define    STRICT
#include  <windows.h>
#include  <assert.h>
#include  <stdio.h>
#include  <math.h>

#include  "Common/HashTable.H"
#include  "ScreenItems/DecoratedFileView.H"

// -------------------------------------------------------------------------------
//	==================  TTabsSpacesViewMode  ========================
// -------------------------------------------------------------------------------

long TTabsSpacesViewMode::GetCharPlaceIndex(const wchar_t *line, int line_len)
{
	int ch_inx = 0;
	while (line_len-- > 0)
	{
		wchar_t ch = *line++;
		if (ch == L'\t')
		{
			// Move current char position to the next tab position.
			ch_inx = (ch_inx/tab_width_in_chars+1)*tab_width_in_chars;
		}
		else
		{
			// This is a simple char. Just increment the char position.
			ch_inx++;
		}
	}

	return(ch_inx);
}

long TTabsSpacesViewMode::GetExplicitTabWidth(long char_inx_of_tab_symbol, long curr_pix_offset)
{
	// Round the char index to the nearest upper tab position.
	long char_inx = (char_inx_of_tab_symbol/tab_width_in_chars+1)*tab_width_in_chars;

	// Find out the pixel offset that corresponds to the rounded char position.
	long recommended_offs = char_inx*char_place_width;

	// Adjust the offset if the current position is too big.
	while (curr_pix_offset+tab_symbol_width > recommended_offs)
	{
		// Already accumulated length of the string exceeds the offset of the desired tabstop.
		// Shift the offset to the next tabstop position.
		recommended_offs += tab_width_in_chars*char_place_width;
	}

	return(recommended_offs-curr_pix_offset);
}

long TTabsSpacesViewMode::GetImplicitTabWidth(long char_inx_after_spaces_seq, long curr_pix_offset)
{
	// Find out the pixel offset that corresponds to the passed char position.
	long recommended_offs = char_inx_after_spaces_seq*char_place_width;

	// Adjust the offset if the current pixel position is too big.
	while (curr_pix_offset+chars_seq_min_width > recommended_offs)
	{
		// Shift offset to the next char position.
		recommended_offs += char_place_width;
	}

	return(recommended_offs-curr_pix_offset);
}

// ----------------------------------------------------------------------------
//  =================  TDecoratedFileViewStyle  =======================
// ----------------------------------------------------------------------------

TDecoratedFileViewStyle::TDecoratedFileViewStyle()
{
	m_props.Clear();
	m_tabs_mode.Clear();
	m_xml_escaped_data = FALSE;

	m_line_nums_style = m_default_text_style = m_line_overflow_style = NULL;
	m_non_ascii_subst_picts = NULL;
	m_non_ascii_substs_table = NULL;
}

TDecoratedFileViewStyle::~TDecoratedFileViewStyle()
{
	m_derived_bkgr_styles_list.DeleteAll();
	m_derived_frgr_styles_list.DeleteAll();
	m_derived_tbsp_styles_list.DeleteAll();
}

bool TDecoratedFileViewStyle::Setup(const TDecoratedFileViewStyleProps &new_props, const TTabsSpacesViewMode &new_tabs_mode, bool xml_escaped_data,
									TTextStyle *nums_style, TTextStyle *default_style, TTextStyle *line_ovfl_style,
									TBitmapStyle *non_ascii_picts, const TNonAsciiSubstsInfo *substs_info)
{
	// Save the props and the tabs data.
	m_props = new_props;
	m_tabs_mode = new_tabs_mode;
	m_xml_escaped_data = xml_escaped_data;

	//
	// Prepare the bkgr styles: line nums, marks and text.
	//

	TBasicStyleProps props1;
	props1.Init(m_props.line_nums_bkgr_color);
	props1.shadow_down_width = m_props.shadow_down_width;
	props1.shadow_down_color = m_props.shadow_down_color;
	m_line_nums_bkgr_style.Setup(props1);

	TBasicStyleProps props2;
	props2.Init(m_props.marks_area_bkgr_color);
	props2.shadow_down_width = m_props.shadow_down_width;
	props2.shadow_down_color = m_props.shadow_down_color;
	m_marks_area_bkgr_style.Setup(props2);

	TBasicStyleProps props3;
	props3.Init(m_props.text_area_bkgr_color);
	props3.shadow_down_width = m_props.shadow_down_width;
	props3.shadow_down_color = m_props.shadow_down_color;
	m_text_area_bkgr_style.Setup(props3);

	// Verify correspondance between the props and the passed styles.
	if (m_props.line_nums_width > 0)
	{
		assert(nums_style != NULL);
	}

	// Note that the horz overflow style is used for an out of memory marker.
	assert(default_style != NULL);
	assert(line_ovfl_style != NULL);

	if (substs_info != NULL)
	{
		assert(non_ascii_picts != NULL);
	}

	// Accept pointers to the bypassed objects.
	m_line_nums_style = nums_style;
	m_default_text_style = default_style;
	m_line_overflow_style = line_ovfl_style;
	m_non_ascii_subst_picts = non_ascii_picts;
	m_non_ascii_substs_table = (TNonAsciiSubstsInfo*)substs_info;

	// Prepare the mouse selection bkgr style.
	m_mouse_selection_bkgr_style.Setup(m_props.mouse_selection_bkgr_color);

	// Get rid of all available derived styles if any.
	m_derived_bkgr_styles_list.DeleteAll();
	m_derived_frgr_styles_list.DeleteAll();
	m_derived_tbsp_styles_list.DeleteAll();

	// Full success.
	return(TRUE);
}

bool TDecoratedFileViewStyle::SetupForDeviceContext(HDC hDC)
{
	bool full_success = TRUE;

	// Update the parent class.
	TScreenItemStyle::SetupForDeviceContext(hDC);

	// Update all owned and referenced styles.
	full_success &= m_line_nums_bkgr_style.SetupForDeviceContext(hDC);
	full_success &= m_marks_area_bkgr_style.SetupForDeviceContext(hDC);
	full_success &= m_text_area_bkgr_style.SetupForDeviceContext(hDC);

	if (m_line_nums_style != NULL)
		full_success &= m_line_nums_style->SetupForDeviceContext(hDC);
	if (m_default_text_style != NULL)
		full_success &= m_default_text_style->SetupForDeviceContext(hDC);
	if (m_line_overflow_style != NULL)
		full_success &= m_line_overflow_style->SetupForDeviceContext(hDC);
	if (m_non_ascii_subst_picts != NULL)
		full_success &= m_non_ascii_subst_picts->SetupForDeviceContext(hDC);

	full_success &= m_mouse_selection_bkgr_style.SetupForDeviceContext(hDC);

	//
	// Update all created and derived styles.
	//

	for (TListIter<TScreenItemStyle> iter1(m_derived_bkgr_styles_list); iter1; ++iter1)
	{
		full_success &= iter1.CurrItem().SetupForDeviceContext(hDC);
	}

	for (TListIter<TScreenItemStyle> iter2(m_derived_frgr_styles_list); iter2; ++iter2)
	{
		full_success &= iter2.CurrItem().SetupForDeviceContext(hDC);
	}

	for (TListIter<TScreenItemStyle> iter3(m_derived_tbsp_styles_list); iter3; ++iter3)
	{
		full_success &= iter3.CurrItem().SetupForDeviceContext(hDC);
	}

	if (m_tabs_mode.tab_symbol_width == 0)
	{
		// Use default text style for preparing the tab symbol width.
		m_tabs_mode.PrepareTabSymbolWidth(m_default_text_style);
	}

	if (m_tabs_mode.char_place_width == 0)
	{
		// Use default text style for preparing the char place width.
		m_tabs_mode.PrepareCharPlaceWidth(m_default_text_style);
	}

	if (m_tabs_mode.chars_seq_min_width == 0)
	{
		// Use default text style for preparing the min char seqs width.
		m_tabs_mode.PrepareCharSeqsMinWidth(m_default_text_style);
	}

	return(full_success);
}

void TDecoratedFileViewStyle::PrepareTabsSpacesWidthMode(short new_tab_width_in_chars, short new_tab_symbol_width_koef, short new_char_place_width_koef)
{
	m_tabs_mode.tab_width_in_chars = new_tab_width_in_chars;

	if (new_tab_symbol_width_koef != 0)
	{
		m_tabs_mode.tab_symbol_width_koef = new_tab_symbol_width_koef;
		m_tabs_mode.PrepareTabSymbolWidth(m_default_text_style);
	}

	if (new_char_place_width_koef != 0)
	{
		m_tabs_mode.char_place_width_koef = new_char_place_width_koef;
		m_tabs_mode.PrepareCharPlaceWidth(m_default_text_style);
	}
}

bool TDecoratedFileViewStyle::CheckForNonAsciiSubsts(wchar_t ch, bool slct_area, short &icon_index)
{
	// Method looks if the code point should be replaced with the bitmap icon or not. This replacement
	// is needed mainly because many fonts do not contain picture for FF (form feed) code point.
	if (m_non_ascii_substs_table == NULL)
		return(FALSE);

	for (TNonAsciiSubstsInfo *info=m_non_ascii_substs_table; info->inx_nonsel != 0; ++info)
	{
		if (ch >= info->code_from && ch <= info->code_to)
		{
			icon_index = (slct_area == TRUE) ? info->inx_slct : info->inx_nonsel;
			return(TRUE);
		}
	}

	// The substitution was not found.
	return(FALSE);
}

TBasicStyle *TDecoratedFileViewStyle::LookupOrCreateBkgrStyle(HDC hDC, TColor color, TColor frame_color)
{
	// Check, if an appropriate style is already existing or not.
	for (TListIter<TBasicStyle> iter(m_derived_bkgr_styles_list); iter; ++iter)
	{
		TBasicStyle &style = iter.CurrItem();
		if (style.m_props.background_color != color)
			continue;

		// Main bkgr color is the same. Check the requested frame.
		TBasicStyleProps &prps = style.m_props;
		if (frame_color == cs_transparent && prps.frame_width == 0)
			return(&style);
		else if (frame_color != cs_transparent && prps.frame_width == 1 && prps.frame_color == frame_color)
			return(&style);
	}

	// Create new basic shape style for the passed color and frame.
	TBasicStyleProps props;
	props.Init(color);
	if (frame_color != cs_transparent)
		props.SetFrameProps(frame_color, 1);

	TBasicStyle *new_style = new TBasicStyle(props);
	if (new_style == NULL)
		return(NULL);

	// Creation succeeded. Finalize and save this style.
	new_style->SetupForDeviceContext(hDC);
	m_derived_bkgr_styles_list.AppendItem(new_style);
	return(new_style);
}

TTextStyle *TDecoratedFileViewStyle::LookupOrCreateTextStyle(HDC hDC, const TTextStyle *proto_style, TColor color, bool keep_shadow_color, bool ignore_css_name)
{
	const TTextStyleProps &pr_pr = proto_style->m_props;
	TColor shadow_color = pr_pr.shadow_down_color;
	if (keep_shadow_color == FALSE)
	{
		// Prepare color for the shadow based on the requested foreground color.
		THueSatLumColorHelper color_hlpr(color);
		color_hlpr.AdjustLuminosity(0.65, 0.10);
		shadow_color = color_hlpr.GetRgb();
	}

	// Check, if an appropriate style is already existing or not. Note that the list to search contains not
	// the arbitrary text styles, but only those styles, that were created in this method. This means that
	// it is ok to check only some of the props, not all of them.
	for (TListIter<TTextStyle> iter(m_derived_frgr_styles_list); iter; ++iter)
	{
		TTextStyleProps &cpr = iter.CurrItem().m_props;

		if (ignore_css_name == FALSE)
		{
			// CSS names should be both missing or they should be the same.
			if (cpr.css_style_name == NULL && pr_pr.css_style_name != NULL || cpr.css_style_name != NULL && pr_pr.css_style_name == NULL)
				continue;

			if (cpr.css_style_name != NULL && pr_pr.css_style_name != NULL && wcscmp(cpr.css_style_name, pr_pr.css_style_name) != 0)
				continue;
		}

		// Props of the core font should match exactly.
		if (cpr.font_height != pr_pr.font_height || cpr.is_bold != pr_pr.is_bold || wcscmp(cpr.font_name, pr_pr.font_name) != 0)
			continue;

		// The main front color should match.
		if (cpr.foreground_color != color)
			continue;

		// Shadows should either be both missing or the modified props of the shadows should match.
		if (pr_pr.shadow_down_color != cs_transparent && pr_pr.shadow_down_width > 0)
		{
			// Shadow in the proto style is present.
			if (cpr.shadow_down_color != shadow_color || cpr.shadow_down_width != pr_pr.shadow_down_width)
				continue;
		}
		else
		{
			// The proto style has no shadow.
			if (cpr.shadow_down_color != cs_transparent && cpr.shadow_down_width != 0)
				continue;
		}

		// All types of adjustments should be identical.
		if (cpr.up_side_adjust != pr_pr.up_side_adjust || cpr.baseline_adjust != pr_pr.baseline_adjust || cpr.down_side_adjust != pr_pr.down_side_adjust)
			continue;
		else if (cpr.front_side_ext != pr_pr.front_side_ext || cpr.back_side_ext != pr_pr.back_side_ext)
			continue;
		else if (cpr.outer_front_padding != pr_pr.outer_front_padding || cpr.outer_back_padding != pr_pr.outer_back_padding)
			continue;
		else if (iter.CurrItem().m_symbols_adjust_data != proto_style->m_symbols_adjust_data)
			continue;

		// All reqiured props matched.
		return(&(iter.CurrItem()));
	}

	// Create new text style based on the proto style.
	TTextStyleProps props = pr_pr;

	if (ignore_css_name == TRUE)
		props.css_style_name = NULL;

	// Replace the frgr color with the requested color.
	props.foreground_color = color;

	// Derived style should have no background and no frame.
	props.background_color = cs_transparent;
	props.frame_color = cs_transparent;
	props.frame_corner_style = fcs_none;

	// Shadow color may not be the same.
	if (pr_pr.shadow_down_color != cs_transparent && pr_pr.shadow_down_width > 0)
		props.shadow_down_color = shadow_color;

	TTextStyle *new_style = new TTextStyle(props, proto_style->m_symbols_adjust_data);
	if (new_style == NULL)
		return(NULL);

	// Creation succeeded.
	if (new_style->SetupForDeviceContext(hDC) == FALSE)
	{
		// Setting up for DC failed.
		delete new_style;
		return(NULL);
	}

	// Save the pointer and return the style.
	m_derived_frgr_styles_list.AppendItem(new_style);
	return(new_style);
}

TLineStyle *TDecoratedFileViewStyle::LookupOrCreateLineStyle(HDC hDC, TColor color)
{
	// Check, if the appropriate style is already existing or not.
	for (TListIter<TLineStyle> iter(m_derived_tbsp_styles_list); iter; ++iter)
	{
		TLineStyle &style = iter.CurrItem();
		if (style.m_props.foreground_color == color)
			return(&style);
	}

	// Create new line style with this color.
	TLineStyleProps props;
	props.Init(color);
	TLineStyle *new_style = new TLineStyle(props);
	if (new_style == NULL)
		return(NULL);

	// Creation succeeded. Finalize and save this style.
	new_style->SetupForDeviceContext(hDC);
	m_derived_tbsp_styles_list.AppendItem(new_style);
	return(new_style);
}

// -----------------------------------------------------------------------------
//  ================  TDFVPrepareScreenItemsContext  ===================
// -----------------------------------------------------------------------------

inline void TDFVPrepareScreenItemsContext::PrepareFrgrItemPlacing(TDecoratedFileViewLineInfo *inst, int ibeg, int iend)
{
	// The passed line fragment should not be empty. Ensure this.
	assert(iend > ibeg);

	ctx_ibeg = ibeg;
	ctx_iend = iend;

	ctx_frgr_front_ext = owner->m_line_rendering_info[ibeg].FullFrontExts();
	ctx_frgr_back_ext = owner->m_line_rendering_info[iend-1].FullBackExts();
	ctx_min_extents = ctx_frgr_front_ext+ctx_frgr_back_ext;

	// Offsets are set up based on the props of the frgr style only. If the area carries a selection,
	// it will be painted using a different style, but its inner margin/outer padding props will be the same.
	const TTextStyle *frgr_style = (frgr_area != NULL) ? frgr_area->Style() : inst->m_default_text.Style();
	assert(frgr_style != NULL);

	short inner_front_ext = (SameFrgrDecorBeforeCurrArea() == FALSE) ? frgr_style->m_props.front_side_ext : 0;
	if (inner_front_ext < 0)
		ctx_frgr_front_ext += inner_front_ext;

	short inner_back_ext = (SameFrgrDecorAfterCurrArea() == FALSE) ? frgr_style->m_props.back_side_ext : 0;
	if (inner_back_ext < 0)
		ctx_frgr_back_ext += inner_back_ext;
}

inline void TDFVPrepareScreenItemsContext::FinalizeFrgrItemPlacing(TDecoratedFileViewLineInfo *inst, TDFVFrgrPaintListItem &pi, long frgr_width)
{
	if (frgr_width >= 0)
	{
		// This is a regular frgr item. Assign the pixel length.
		pi.pix_len = ctx_frgr_front_ext+frgr_width+ctx_frgr_back_ext;
		if (pi.pix_len <= 0)
		{
			// In reality this is almost impossible to happen. Nevertheless allow even this.
			pi.pix_len = 1;
		}

		// Extents should be set exactly as they were cooked in PrepareFrgrItemPlacing().
		pi.frgr_front_ext = (short)ctx_frgr_front_ext;
		pi.frgr_back_ext = (short)ctx_frgr_back_ext;
	}
	else
	{
		// This is either a horz overflow item or an out of memory marker.
		pi.pix_len = pi.frgr_front_ext = pi.frgr_back_ext = 0;

		// Clear the exts info in all charplaces starting from the first char of the area and fill all
		// remaining char places with the current pixel offset.
		long line_len = inst->m_default_text.num_chars;
		for (int ich=ctx_ibeg; ich <= line_len; ++ich)
		{
			TDFVLineRenderCharPlaceInfo &cpi = owner->m_line_rendering_info[ich];
			cpi.Clear();
			cpi.m_painting_list_item_offs = curr_pix;
		}
	}

	inst->m_frgr_painting_list.AppendItem(pi);
	curr_pix += pi.pix_len;
	owner->m_line_rendering_info[ctx_iend].m_painting_list_item_offs = curr_pix;
}

// --------------------------------------------------------------------------
//  =============  TDFVHtmlGenerationContext  =====================
// --------------------------------------------------------------------------

TRawHtmlElement *TDFVHtmlGenerationContext::FindOrCreateLowLevelOwner(TScreenItem &decoration)
{
	// Prepare the additional properties.
	const wchar_t *css_class = decoration.GetCssStyleName();
	const wchar_t *css_style = (decoration.GetItemTypeId() == TBitmapItem::type_ID) ? ((TBitmapItem&)decoration).m_html_class_attr : NULL;
	const wchar_t *img_src = (decoration.GetItemTypeId() == TBitmapItem::type_ID) ? ((TBitmapItem&)decoration).m_html_src_name : NULL;

	// Pick up the app layer delegate if any.
	THtmlGenAppDelegate *app_delegate = m_dfv->GetHtmlOwnerDelegate(&decoration);

	// Call the worker functions.
	if (app_delegate == NULL)
		return(m_owns_ctx.FindOrCreateSimpleOwner(css_class, css_style, img_src));
	else return(m_owns_ctx.CreateAppDelegateProxy(app_delegate, css_class, css_style, img_src));
}

// --------------------------------------------------------------------------
//  ==============  TDecoratedFileViewLineInfo  =======================
// --------------------------------------------------------------------------

static wchar_t g_DFV_OutOfRangeLine[] = L"The line number is out of range ...";

TDecoratedFileViewLineInfo::TDecoratedFileViewLineInfo()
{
	// Init only those fields that are not set up in the Setup() method.
	m_text_above = m_text_below = 0;
	m_ext_bkgr_above = m_ext_bkgr_below = 0;
	m_upper_wlbk_ext = m_lower_wlbk_ext = 0;
	m_upper_extent = m_lower_extent = 0;

	m_imposed_above = m_imposed_below = 0;

	m_frgrw_sens_wlbks = FALSE;
	m_indirect_hgt_bkgrs = FALSE;

	m_spec_tabs_area_index = -1;
}

TDecoratedFileViewLineInfo::~TDecoratedFileViewLineInfo()
{
	m_bkgr_decors.DeleteAll();
	m_frgr_decors.DeleteAll();
	m_whole_line_bkgrs.DeleteAll();

	// Line is owning its imposed decors. Release these objects.
	for (int inx1=0; inx1<m_imposed_decors.NumItems(); ++inx1)
		delete m_imposed_decors[inx1].scr_item;
	for (int inx2=0; inx2<m_whole_line_imposed.NumItems(); ++inx2)
		delete m_whole_line_imposed[inx2].wlimp_scr_item;

	m_extra_frgr_objects.DeleteAll();
	m_non_ascii_objects.DeleteAll();
	m_visib_spc_objects.DeleteAll();
}

bool TDecoratedFileViewLineInfo::Setup(TDFVDecoratedLineSetupContext &ctx, const wchar_t *line_beg, long line_len)
{
	//
	//  This method should be called exactly after the object construction. Parameters of this method
	//  are not placed into the ctor because this object is used in arrays and arrays are not allowing
	//  different ctor params for different members of the array.
	//
	assert(line_len >= 0);

	// Initially assume that the passed data is simple.
	m_tabs_or_substs = m_space_chars = m_space_seqs = FALSE;

	TDecoratedFileViewStyle *dfv_style = ctx.owner->m_dfv_style;
	m_baseline = ctx.line_top;
	m_invalid_scr_items = FALSE;

	// Prepare the line number object.
	if (dfv_style->m_props.line_nums_width > 0)
	{
		m_line_num_scr_item.SetStyle(dfv_style->m_line_nums_style);
		m_line_num_scr_item.SetTextFmt(dfv_style->m_props.line_nums_format, ctx.line_num);
	}

	// Setup the default text using the bypassed contents of the line. Note that this setup is not placing
	// anything into the frgr painting list.
	m_default_text.zorder = 0;
	m_default_text.char_beg = 0;
	m_default_text.num_chars = line_len;
	if (line_beg == NULL)
	{
		// This is an "out of the lines range" dummy line. Setup the constant text with the line overflow style.
		assert(line_len == 0);
		m_default_text.SetStyle(dfv_style->m_line_overflow_style);
		m_default_text.SetContents(g_DFV_OutOfRangeLine);
	}
	else if (line_len > 0)
	{
		// This is a non empty line from the file data. Fill in the default style into the default line.
		m_default_text.SetStyle(dfv_style->m_default_text_style);
		m_default_text.SetContents(line_beg, line_len);

		// Figure out the props of the line data.
		wchar_t *p = (wchar_t*)line_beg;
		bool prev_space = FALSE;
		for (int i=0; i<line_len; ++i, ++p)
		{
			short icon_index;
			if (*p == L'\t' || (*p < 0x20 && dfv_style->CheckForNonAsciiSubsts(*p, FALSE, icon_index) == TRUE))
			{
				// This is a tab symbol or this char requires a bitmap substitute.
				m_tabs_or_substs = TRUE;
			}
			else if (*p == L'&' && dfv_style->AssembleXmlEscapes() == TRUE)
			{
				// Check if current ampersand starts an XML esc seq or not.
				long seq_len;
				if (CheckAssembleXmlSeq(p, line_len-i, seq_len) != 0)
					m_tabs_or_substs = TRUE;
			}

			if (*p == L' ')
			{
				// At least one space character is found.
				m_space_chars = TRUE;
				if (prev_space == TRUE)
				{
					// This is a sequence of at least two consecutive spaces.
					m_space_seqs = TRUE;
				}

				prev_space = TRUE;
			}
			else
			{
				prev_space = FALSE;
			}
		}
	}
	else
	{
		// This is an empty line. Nevertheless the pointer to the beginning of this line is still needed.
		m_default_text.SetStyle(NULL);
		m_default_text.SetContents(line_beg, 0);
	}

	// Figure out the height params of the font and shift the screen items.
	if (line_beg != NULL)
		EvaluateTextStyleExtents(dfv_style->m_default_text_style);
	else EvaluateTextStyleExtents(dfv_style->m_line_overflow_style);

	m_baseline += GetFullHeightAbove();
	if (m_line_num_scr_item.GetBasicStyle() != NULL)
		SetLineNumScreenItemStyleAndPosition(ctx, ctx.nums_x_pos, dfv_style->m_line_nums_style);
	ctx.line_top += GetFullHeight();

	// Finally prepare the frgr painting list if any.
	if (line_beg == NULL)
	{
		// This is a lines range overflow case.
		m_default_text.PlaceObject(ctx.GetControlDC(), ctx.text_x_pos, m_baseline);
		return(TRUE);
	}

	// Note that at this point there are no decorations on the file view. They are not applied yet.
	// This means that this preparation cannot make other lines of the file invalid.
	return(PrepareScreenItems(ctx));
}

void TDecoratedFileViewLineInfo::ShiftY(TDecoratedFileViewScreenItem *owner, long shift_value)
{
	// Background decorations.
	for (TListIter<TDFVRectDecoration> iter1(m_bkgr_decors); iter1; ++iter1)
	{
		iter1.CurrItem().ShiftItem(0, shift_value);
	}

	// Backgrounds of the foreground decorations. Note that the foreground part should not be shifted
	// in this loop. It will be shifted (if needed) when the frgr painting list will be processed.
	for (TListIter<TDFVFrgrDecoration> iter2(m_frgr_decors); iter2; ++iter2)
	{
		iter2.CurrItem().bkgr_item.ShiftItem(0, shift_value);
	}

	// Backgrounds of the selection decorations.
	for (int inx1=0; inx1<m_slct_decors.NumItems(); ++inx1)
	{
		m_slct_decors[inx1].slct_bkgr_item.ShiftItem(0, shift_value);
	}

	// Process the frgr painting list.
	if (m_invalid_scr_items == FALSE)
	{
		for (TListIter<TDFVFrgrPaintListItem> iter3(m_frgr_painting_list); iter3; ++iter3)
		{
			iter3.CurrItem().screen_item->ShiftItem(0, shift_value);
		}
	}

	// Text fragment imposed decorations.
	for (int inx2=0; inx2<m_imposed_decors.NumItems(); ++inx2)
	{
		m_imposed_decors[inx2].scr_item->ShiftItem(0, shift_value);
	}

	// Whole line background decorations.
	for (TListIter<TDFVRectDecoration> iter4(m_whole_line_bkgrs); iter4; ++iter4)
	{
		iter4.CurrItem().ShiftItem(0, shift_value);
	}

	// Shift the line number of the line.
	if (m_line_num_scr_item.GetBasicStyle() != NULL)
	{
		m_line_num_scr_item.ShiftItem(0, shift_value);
	}

	// Whole line imposed decorations.
	for (int inx3=0; inx3<m_whole_line_imposed.NumItems(); ++inx3)
	{
		m_whole_line_imposed[inx3].wlimp_scr_item->ShiftItem(0, shift_value);
	}

	// Update the baseline and report new invalid areas.
	ReportFullAreaInvalid(owner);
	m_baseline += shift_value;
	ReportFullAreaInvalid(owner);
}

void TDecoratedFileViewLineInfo::SetPosition(TDecoratedFileViewScreenItem *owner, long new_pos_x, long new_pos_y)
{
	//
	//  Invalid areas are not reported here because this method is called only when the whole decorated file is moved.
	//  In this case invalid areas are reported on the whole file view level.
	//

	long sh_x = new_pos_x - (owner->m_text_rect_scr_item.GetBaseLeft()+owner->m_dfv_style->m_props.text_left_offs);
	long sh_y = new_pos_y - m_baseline;
	if (sh_x == 0)
	{
		// The horz shift is missing. Apply just the vert shift.
		ShiftY(owner, sh_y);
		return;
	}

	// Background decorations.
	for (TListIter<TDFVRectDecoration> iter1(m_bkgr_decors); iter1; ++iter1)
	{
		iter1.CurrItem().ShiftItem(sh_x, sh_y);
	}

	// Backgrounds of the foreground decorations. Note that foreground part should not be shifted
	// in this loop. It will be shifted (if needed) when the frgr painting list will be processed.
	for (TListIter<TDFVFrgrDecoration> iter2(m_frgr_decors); iter2; ++iter2)
	{
		iter2.CurrItem().bkgr_item.ShiftItem(sh_x, sh_y);
	}

	// Backgrounds of the selection decorations.
	for (int inx1=0; inx1<m_slct_decors.NumItems(); ++inx1)
	{
		m_slct_decors[inx1].slct_bkgr_item.ShiftItem(sh_x, sh_y);
	}

	// Process the frgr painting list.
	if (m_invalid_scr_items == FALSE)
	{
		for (TListIter<TDFVFrgrPaintListItem> iter3(m_frgr_painting_list); iter3; ++iter3)
		{
			iter3.CurrItem().screen_item->ShiftItem(sh_x, sh_y);
		}
	}

	// Inline imposed decorations.
	for (int inx2=0; inx2<m_imposed_decors.NumItems(); ++inx2)
	{
		m_imposed_decors[inx2].scr_item->ShiftItem(sh_x, sh_y);
	}

	// Whole line background decorations.
	for (TListIter<TDFVRectDecoration> iter4(m_whole_line_bkgrs); iter4; ++iter4)
	{
		iter4.CurrItem().ShiftItem(sh_x, sh_y);
	}

	// Shift the line number of the line.
	if (m_line_num_scr_item.GetBasicStyle() != NULL)
	{
		m_line_num_scr_item.ShiftItem(sh_x, sh_y);
	}

	// Whole line imposed decorations.
	for (int inx3=0; inx3<m_whole_line_imposed.NumItems(); ++inx3)
	{
		m_whole_line_imposed[inx3].wlimp_scr_item->ShiftItem(sh_x, sh_y);
	}

	// Save the new baseline value.
	m_baseline = new_pos_y;
}

void TDecoratedFileViewLineInfo::PaintBkgrFrgrObjects(HDC hDC, RECT &rc_invalid)
{
	assert(m_invalid_scr_items == FALSE);

	// Paint all whole line background decorations.
	for (TListIter<TDFVRectDecoration> iter1(m_whole_line_bkgrs); iter1; ++iter1)
	{
		if (iter1.CurrItem().GetBasicStyle() != NULL)
			iter1.CurrItem().OnDraw(hDC, rc_invalid);
	}

	// Paint the line number.
	if (m_line_num_scr_item.GetBasicStyle() != NULL)
	{
		m_line_num_scr_item.OnDraw(hDC, rc_invalid);
	}

	// Paint all inline background decorations.
	for (TListIter<TDFVRectDecoration> iter2(m_bkgr_decors); iter2; ++iter2)
	{
		if (iter2.CurrItem().GetBasicStyle() != NULL)
			iter2.CurrItem().OnDraw(hDC, rc_invalid);
	}

	// Paint the backgrounds of the foreground decorations.
	for (TListIter<TDFVFrgrDecoration> iter3(m_frgr_decors); iter3; ++iter3)
	{
		if (iter3.CurrItem().bkgr_item.GetBasicStyle() != NULL)
			iter3.CurrItem().bkgr_item.OnDraw(hDC, rc_invalid);
	}

	// Paint the backgrounds of the selection decorations.
	for (int inx1=0; inx1<m_slct_decors.NumItems(); ++inx1)
	{
		if (m_slct_decors[inx1].slct_bkgr_item.GetBasicStyle() != NULL)
			m_slct_decors[inx1].slct_bkgr_item.OnDraw(hDC, rc_invalid);
	}

	// Process the frgr painting list.
	for (TListIter<TDFVFrgrPaintListItem> iter4(m_frgr_painting_list); iter4; ++iter4)
	{
		if (iter4.CurrItem().screen_item->GetBasicStyle() != NULL)
			iter4.CurrItem().screen_item->OnDraw(hDC, rc_invalid);
	}

	// Paint the text fragment inserted decorations. Do this in a separate loop before painting
	// the imposed decors to be sure that all inserted decors are placed under the imposed decors.
	for (int inx1=0; inx1<m_imposed_decors.NumItems(); ++inx1)
	{
		if (m_imposed_decors[inx1].insert_decor == TRUE)
			m_imposed_decors[inx1].scr_item->OnDraw(hDC, rc_invalid);
	}
}

void TDecoratedFileViewLineInfo::PaintImposedObjects(HDC hDC, RECT &rc_invalid)
{
	assert(m_invalid_scr_items == FALSE);

	// Paint the text fragment imposed decorations.
	for (int inx2=0; inx2<m_imposed_decors.NumItems(); ++inx2)
	{
		if (m_imposed_decors[inx2].insert_decor == FALSE)
			m_imposed_decors[inx2].scr_item->OnDraw(hDC, rc_invalid);
	}

	// Paint the whole line imposed decorations.
	for (int inx3=0; inx3<m_whole_line_imposed.NumItems(); ++inx3)
	{
		m_whole_line_imposed[inx3].wlimp_scr_item->OnDraw(hDC, rc_invalid);
	}
}

TScreenItem *TDecoratedFileViewLineInfo::CheckClickOrSummarize(TDecoratedFileViewScreenItem *owner, POINT &app_pt, TScreenItemSummarizeInfo *sumry)
{
	assert(m_invalid_scr_items == FALSE);

	// Check the text fragment imposed decorations.
	for (int inx1=m_imposed_decors.NumItems()-1; inx1 >= 0; --inx1)
	{
		TScreenItem *lc_item = m_imposed_decors[inx1].scr_item->CheckClick(app_pt);
		if (lc_item != NULL && lc_item->GetOwner() != NULL)
		{
			if (sumry == NULL)
				return(lc_item);

			sumry->AddToSummary(lc_item);
		}
	}

	// Check the whole line imposed decorations.
	for (int inx2=m_whole_line_imposed.NumItems()-1; inx2 >= 0; --inx2)
	{
		TScreenItem *lc_item = m_whole_line_imposed[inx2].wlimp_scr_item->CheckClick(app_pt);
		if (lc_item != NULL && lc_item->GetOwner() != NULL)
		{
			if (sumry == NULL)
				return(lc_item);

			sumry->AddToSummary(lc_item);
		}
	}

	// Check the line number decorations.
	if (m_line_num_scr_item.GetBasicStyle() != NULL)
	{
		TScreenItem *lc_item = m_line_num_scr_item.CheckClick(app_pt);
		if (lc_item != NULL && lc_item->GetOwner() != NULL)
		{
			if (sumry == NULL)
				return(lc_item);

			sumry->AddToSummary(lc_item);
		}
	}

	// Check the frgr painting list. This procedure is the most complex. During this initial loop detect the frgr
	// painting list screen item that owns the clicked point even if this screen item is click transparent.
	TScreenItem *frgr_list_scr_item = NULL;
	TListIter<TDFVFrgrPaintListItem> paint_list_iter(m_frgr_painting_list);
	for (paint_list_iter.SeekToLast(); paint_list_iter; --paint_list_iter)
	{
		TDFVFrgrPaintListItem &item = paint_list_iter.CurrItem();
		if (item.pix_len == 0 || item.screen_item == &m_default_text)
		{
			// These frgr painting list objects are non clickable.
			continue;
		}

		TScreenItem *lc_item = item.screen_item->CheckClick(app_pt);
		if (lc_item != NULL)
		{
			if (sumry == NULL && lc_item->GetOwner() != NULL)
				return(lc_item);

			frgr_list_scr_item = item.screen_item;
			break;
		}

		if (item.screen_item->GetItemTypeId() == TLinesChainItem::type_ID)
		{
			// This is a visible space or a visible tab object. It cannot be clicked using the regular
			// procedure. Check if the passed app_point is in its rect or not.
			RECT rc_lines_chain;
			((TDFVTabOrSpaceItem*)item.screen_item)->GetBoundingRectEx(rc_lines_chain);
			if (TScreenItem::IsPointInRect(app_pt, rc_lines_chain) == TRUE)
			{
				if (sumry == NULL && item.screen_item->GetOwner() != NULL)
					return(item.screen_item);

				frgr_list_scr_item = item.screen_item;
				break;
			}
		}
	}

	if (frgr_list_scr_item != NULL)
	{
		// User clicked on the area where the text is present. At the same time this is either the summarization
		// procedure or user clicked on a click transparent frgr item. In both cases it is necessary to figure out
		// the char place that the current clicked point belongs to.
		long char_num = GetFrgrPaintingListItemCharPos(owner, frgr_list_scr_item, app_pt.x);

		// Loop over the slct decorations using their zorder.
		for (int inx3=m_slct_decors.NumItems()-1; inx3 >= 0; --inx3)
		{
			TDFVSlctDecoration &decor = m_slct_decors[inx3];
			if (decor.slct_bkgr_item.GetOwner() == NULL)
				continue;

			if (char_num >= decor.char_beg && char_num < decor.char_beg+decor.num_chars)
			{
				// Char place belongs to the current decoration.
				if (sumry == NULL)
					return(&(decor.slct_bkgr_item));

				sumry->AddToSummary(&(decor.slct_bkgr_item));
			}
		}

		// Loop over the frgr decorations using their zorder.
		TListIter<TDFVFrgrDecoration> frgr_iter1(m_frgr_decors);
		for (frgr_iter1.SeekToLast(); frgr_iter1; --frgr_iter1)
		{
			TDFVFrgrDecoration *decor = &frgr_iter1.CurrItem();
			if (decor->GetOwner() == NULL)
				continue;

			if (char_num >= decor->char_beg && char_num < decor->char_beg+decor->num_chars)
			{
				// Char place belongs to the current decoration.
				if (sumry == NULL)
					return(decor);

				sumry->AddToSummary(decor);
			}
		}
	}
	else
	{
		//
		// No frgr object was clicked. Nevertheless it is necessary to carry out almost the same procedure as above.
		//

		// Loop over the slct decorations using their zorder.
		for (int inx4=m_slct_decors.NumItems()-1; inx4 >= 0; --inx4)
		{
			TDFVSlctDecoration &decor = m_slct_decors[inx4];
			TScreenItem *lc_item = decor.slct_bkgr_item.CheckClick(app_pt);
			if (lc_item != NULL && lc_item->GetOwner() != NULL)
			{
				if (sumry == NULL)
					return(lc_item);

				sumry->AddToSummary(lc_item);
			}

		}

		// Loop over the frgr decorations using their zorder.
		TListIter<TDFVFrgrDecoration> frgr_iter2(m_frgr_decors);
		for (frgr_iter2.SeekToLast(); frgr_iter2; --frgr_iter2)
		{
			TDFVFrgrDecoration *decor = &frgr_iter2.CurrItem();
			TScreenItem *lc_item = decor->bkgr_item.CheckClick(app_pt);
			if (lc_item != NULL && lc_item->GetOwner() != NULL)
			{
				if (sumry == NULL)
					return(lc_item);

				sumry->AddToSummary(lc_item);
			}
		}
	}

	// Check the text fragment background decorations.
	TListIter<TDFVRectDecoration> bkgr_iter(m_bkgr_decors);
	for (bkgr_iter.SeekToLast(); bkgr_iter; --bkgr_iter)
	{
		TScreenItem *lc_item = bkgr_iter.CurrItem().CheckClick(app_pt);
		if (lc_item != NULL && lc_item->GetOwner() != NULL)
		{
			if (sumry == NULL)
				return(lc_item);

			sumry->AddToSummary(lc_item);
		}
	}

	// Check the whole line background decorations.
	TListIter<TDFVRectDecoration> wlbk_iter(m_whole_line_bkgrs);
	for (wlbk_iter.SeekToLast(); wlbk_iter; --wlbk_iter)
	{
		TScreenItem *lc_item = wlbk_iter.CurrItem().CheckClick(app_pt);
		if (lc_item != NULL && lc_item->GetOwner() != NULL)
		{
			if (sumry == NULL)
				return(lc_item);

			sumry->AddToSummary(lc_item);
		}
	}

	// Nothing succeeded or this was the summarization procedure.
	return(NULL);
}

// /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
// -----------------  Decoration manipulations ---------------------
// \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

bool TDecoratedFileViewLineInfo::AddBkgrDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
											long char_beg, long num_chars, TBasicStyle *style, short upper_ext, short lower_ext)
{
	// Verify the passed char indexes.
	assert(char_beg >= 0 && char_beg <= m_default_text.num_chars);
	assert(num_chars > 0 && char_beg+num_chars <= m_default_text.num_chars);

	// This type of decors is not merged with existing decorations. Allocate object for this new
	// decor without checking state of other decors.
	TDFVRectDecoration *new_decor = new TDFVRectDecoration(char_beg, num_chars, upper_ext, lower_ext);
	if (new_decor == NULL)
		return(FALSE);

	new_decor->SetStyle(style);
	new_decor->SetOwner(decor_owner, owner_data);
	new_decor->zorder = zorder;

	// Add object to the right place into the list of bkgr decors. Decorations are stored in the order
	// of increasing of their zorder. Position and length of the decor is not important.
	TDFVRectDecoration *ib = NULL;
	for (TListIter<TDFVRectDecoration> iter(m_bkgr_decors); iter; ++iter)
	{
		TDFVRectDecoration *decor = &iter.CurrItem();
		if (decor->zorder > zorder)
		{
			ib = decor;
			break;
		}
	}

	if (ib != NULL)
		m_bkgr_decors.InsertBefore(new_decor, ib);
	else m_bkgr_decors.AppendItem(new_decor);

	// Process possible changes in the vertical direction.
	bool line_height_chg = FALSE;
	if (upper_ext > m_ext_bkgr_above)
	{
		long baseline_shift = upper_ext-m_ext_bkgr_above;
		ShiftY(ctx.owner, baseline_shift);
		ctx.owner->ReportVertDelta(baseline_shift, ctx.line_num);
		m_ext_bkgr_above = upper_ext;
		line_height_chg = TRUE;
	}

	if (lower_ext > m_ext_bkgr_below)
	{
		ctx.owner->ReportVertDelta(lower_ext-m_ext_bkgr_below, ctx.line_num);
		m_ext_bkgr_below = lower_ext;
		line_height_chg = TRUE;
	}

	if (line_height_chg == TRUE)
		UpdateWholeLineBkgrDecors(ctx);

	// Check, if this decor depends on the total line height or not.
	if (upper_ext < 0 || lower_ext < 0)
	{
		// At least one of the vert decor borders depend on the vert spacing decors that accept color from the wlbk decors.
		m_indirect_hgt_bkgrs = TRUE;
	}

	// Horz changes will be figured out later.
	ctx.owner->ReportFrgrUpdateWanted(this);
	m_invalid_scr_items = TRUE;
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddFrgrDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
											long char_beg, long num_chars, TTextStyle *frgr_style, TBasicStyle *bkgr_style)
{
	// Make stnd char indexes verification. Note that foreground decors are allowed to have zero width.
	assert(char_beg >= 0 && char_beg <= m_default_text.num_chars);
	assert(num_chars >= 0 && char_beg+num_chars <= m_default_text.num_chars);

	// This type of decors is not merged with existing decors. Allocate object for this new decor without
	// checking the state of other decors.
	TDFVFrgrDecoration *new_decor = new TDFVFrgrDecoration(char_beg, num_chars);
	if (new_decor == NULL)
		return(FALSE);

	// The background style is not NULL only when it is not transp, does not contain frame, etc.
	new_decor->SetStyle(frgr_style);
	new_decor->SetOwner(decor_owner, owner_data);
	new_decor->bkgr_item.SetStyle(bkgr_style);
	new_decor->bkgr_item.SetOwner(decor_owner, owner_data);
	new_decor->zorder = zorder;

	// Add object to the right place into the list of frgr decors.
	TDFVFrgrDecoration *ib = NULL;
	for (TListIter<TDFVFrgrDecoration> iter(m_frgr_decors); iter; ++iter)
	{
		TDFVFrgrDecoration *decor = &iter.CurrItem();
		if (decor->zorder > zorder)
		{
			ib = decor;
			break;
		}
	}

	if (ib != NULL)
		m_frgr_decors.InsertBefore(new_decor, ib);
	else m_frgr_decors.AppendItem(new_decor);

	// Figure out the changes in the vertical direction.
	if (ProcessTextExtentsVertImpact(ctx) == TRUE)
	{
		// Height of the line has increased or decreased.
		UpdateWholeLineBkgrDecors(ctx);
	}

	// Horz changes will be figured out later.
	ctx.owner->ReportFrgrUpdateWanted(this);
	m_invalid_scr_items = TRUE;
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddSlctDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
											long char_beg, long num_chars, TBasicStyle *bkgr_style, TColor frgr_color, short beg_end_line_ext)
{
	// Verify the passed char indexes.
	assert(char_beg >= 0 && char_beg <= m_default_text.num_chars);
	assert(num_chars > 0 && char_beg+num_chars <= m_default_text.num_chars);

	// This type of decors is not merged with existing decors. Allocate object for this new decor without
	// checking the state of other decors.
	TDFVSlctDecoration new_decor;
	new_decor.slct_bkgr_item.SetOwner(decor_owner, owner_data);
	new_decor.slct_bkgr_item.SetStyle(bkgr_style);
	new_decor.zorder = zorder;
	new_decor.char_beg = char_beg;
	new_decor.num_chars = num_chars;
	new_decor.frgr_color = frgr_color;
	new_decor.beg_end_line_ext = beg_end_line_ext;

	// Add object to the right place into the list of slct decors.
	int inx_ib = m_slct_decors.NumItems();
	for (int i=0; i<m_slct_decors.NumItems(); ++i)
	{
		if (m_slct_decors[i].zorder >= zorder)
		{
			inx_ib = i;
			break;
		}
	}

	if (m_slct_decors.InsertItem(inx_ib, new_decor) == FALSE)
		return(FALSE);

	// The vert changes are not possible. Horz changes will be figured out later.
	ctx.owner->ReportFrgrUpdateWanted(this);
	m_invalid_scr_items = TRUE;
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddInsertedDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
												long char_inx, TScreenItem *scr_item, short decor_horz_order, long baseline_vert_offs)
{
	// Verify the passed char index.
	assert(char_inx >= 0 && char_inx <= m_default_text.num_chars);

	assert(scr_item != NULL);
	scr_item->SetOwner(decor_owner, owner_data);

	// This type of decorations cannot be merged with existing decors.
	TDFVInlineImposedDecoration new_decor = { scr_item, zorder, char_inx, TRUE };
	new_decor.insert_horz_order = decor_horz_order;
	new_decor.insert_vert_offs = baseline_vert_offs;

	// Add object to the right place into the list of imposed decors.
	int inx_ib = m_imposed_decors.NumItems();
	for (int i=0; i<m_imposed_decors.NumItems(); ++i)
	{
		TDFVInlineImposedDecoration &dcr = m_imposed_decors[i];
		if (dcr.insert_decor == TRUE && (dcr.char_inx > char_inx || dcr.char_inx == char_inx && dcr.insert_horz_order >= decor_horz_order))
		{
			inx_ib = i;
			break;
		}
	}

	if (m_imposed_decors.InsertItem(inx_ib, new_decor) == FALSE)
		return(FALSE);

	// Make some temp fake placing.
	scr_item->SetHotSpotPos(0, m_baseline);
	EvaluateImposedExtents(ctx.owner, scr_item);

	// Make the line invalid and return success.
	ctx.owner->ReportFrgrUpdateWanted(this);
	m_invalid_scr_items = TRUE;
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddImposedDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
												long char_inx, TScreenItem *scr_item, long horz_offs, long baseline_offs)
{
	// Verify the passed char index.
	assert(char_inx >= 0 && char_inx <= m_default_text.num_chars);

	assert(scr_item != NULL);
	scr_item->SetOwner(decor_owner, owner_data);

	// This type of decorations cannot be merged with existing decors.
	TDFVInlineImposedDecoration new_decor = { scr_item, zorder, char_inx, FALSE };
	new_decor.sym_border_horz_offs = horz_offs;
	new_decor.baseline_vert_offs = baseline_offs;

	// Add object to the right place into the list of imposed decors.
	int inx_ib = m_imposed_decors.NumItems();
	for (int i=0; i<m_imposed_decors.NumItems(); ++i)
	{
		if (m_imposed_decors[i].insert_decor == FALSE && m_imposed_decors[i].zorder >= zorder)
		{
			inx_ib = i;
			break;
		}
	}

	if (m_imposed_decors.InsertItem(inx_ib, new_decor) == FALSE)
		return(FALSE);

	if (m_invalid_scr_items == FALSE)
	{
		// It is possible to set the decor position right now.
		PositionImposedDecor(ctx, new_decor);
	}
	else
	{
		// Make some temp fake placing.
		scr_item->SetHotSpotPos(horz_offs, m_baseline+baseline_offs);
	}

	// Success.
	EvaluateImposedExtents(ctx.owner, scr_item);
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddHorzSpcDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
													long char_inx, long spc_val_left, long spc_val_right)
{
	// Verify the passed char index.
	assert(char_inx >= 0 && char_inx <= m_default_text.num_chars);

	// This type of decor _is_ merged with existing decors. Check if the passed location is already
	// decorated using the passed props or not.
	int decor_inx = -1;
	for (int i=0; i<m_spacing_decors.NumItems(); ++i)
	{
		TDFVExtraSpcDecoration &dcr = m_spacing_decors[i];
		if (dcr.spc_spec_tab == FALSE && dcr.spc_char_inx == char_inx && dcr.spc_owner == decor_owner && dcr.spc_owner_data == owner_data && dcr.spc_zorder == zorder)
		{
			decor_inx = i;
			break;
		}
	}

	if (decor_inx >= 0)
	{
		// Update the existing decoration record.
		m_spacing_decors[decor_inx].spc_horz_value_left = spc_val_left;
		m_spacing_decors[decor_inx].spc_horz_value_right = spc_val_right;
	}
	else
	{
		// Add new decor record to the end of the array.
		TDFVExtraSpcDecoration decor = { decor_owner, owner_data, zorder };
		decor.spc_char_inx = char_inx;
		decor.spc_spec_tab = FALSE;
		decor.spc_horz_value_left = spc_val_left;
		decor.spc_horz_value_right = spc_val_right;

		if (m_spacing_decors.AppendItem(decor) == FALSE)
			return(FALSE);
	}

	// Vert changes are not possible. Horz changes will be figured out later.
	ctx.owner->ReportFrgrUpdateWanted(this);
	m_invalid_scr_items = TRUE;
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddSpecTabDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
													long char_inx, short decor_horz_order)
{
	// Verify the passed char index.
	assert(char_inx >= 0 && char_inx <= m_default_text.num_chars);

	// This type of decor _is_ merged with existing decors. Check if the passed location is already
	// decorated using the passed props or not.
	int decor_inx = m_spacing_decors.NumItems();
	for (int i=0; i<m_spacing_decors.NumItems(); ++i)
	{
		TDFVExtraSpcDecoration &dcr = m_spacing_decors[i];
		if (dcr.spc_spec_tab == TRUE && dcr.spc_char_inx == char_inx && dcr.spc_owner == decor_owner && dcr.spc_owner_data == owner_data)
		{
			if (dcr.spc_spec_tab_horz_order == decor_horz_order)
			{
				// The requested decor is already present. There is no need to modify anything.
				return(TRUE);
			}
			else if (dcr.spc_spec_tab_horz_order > decor_horz_order)
			{
				decor_inx = i;
				break;
			}
		}
	}

	// Add new decor record to the array.
	TDFVExtraSpcDecoration decor = { decor_owner, owner_data, zorder };
	decor.spc_char_inx = char_inx;
	decor.spc_spec_tab = TRUE;
	decor.spc_spec_tab_horz_order = decor_horz_order;

	if (m_spacing_decors.InsertItem(decor_inx, decor) == FALSE)
		return(FALSE);

	// Vert changes are not possible. Horz changes will be figured out later.
	ctx.owner->ReportFrgrUpdateWanted(this);
	m_invalid_scr_items = TRUE;
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddWholeLineBkgrDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
														TBasicStyle *style, WORD cover_mask)
{
	// At least one of the areas should be covered.
	assert(style != NULL && cover_mask != 0);

	// Look for identical record and at the same time for the place to insert the new record.
	TDFVRectDecoration *ib = NULL;
	for (TListIter<TDFVRectDecoration> iter(m_whole_line_bkgrs); iter; ++iter)
	{
		TDFVRectDecoration *dcr = &iter.CurrItem();
		if (dcr->zorder > zorder)
		{
			ib = dcr;
			break;
		}

		if (dcr->GetOwner() == decor_owner && dcr->GetOwnerData() == owner_data && dcr->zorder == zorder)
		{
			if (dcr->cover_mask == cover_mask && dcr->GetBasicStyle() == style)
			{
				// This is a completely identical decoration. View of the file is not changing.
				return(TRUE);
			}
		}
	}

	// Create a new decoration.
	TDFVRectDecoration *new_decor = new TDFVRectDecoration(cover_mask);
	if (new_decor == NULL)
		return(FALSE);

	new_decor->SetStyle(style);
	new_decor->SetOwner(decor_owner, owner_data);
	new_decor->zorder = zorder;

	// Add decoration to the right place in the list.
	if (ib != NULL)
		m_whole_line_bkgrs.InsertBefore(new_decor, ib);
	else m_whole_line_bkgrs.AppendItem(new_decor);

	// Check if this decor depends on the contents of the line.
	WORD fple_dep_flags = cover_mask & (wlbk_mtext | wlbk_rtext);
	if (fple_dep_flags == wlbk_mtext || fple_dep_flags == wlbk_rtext)
	{
		// Middle and right flags have different value. This means that decor depends on
		// the horizontal position of the end of the line.
		m_frgrw_sens_wlbks = TRUE;
	}

	// Position decor right now even if the horz update is still pending.
	PositionWholeLineBkgrDecor(ctx, *new_decor);

	// Success.
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddLineNumFrgrDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
													TTextStyle *style)
{
	if (ctx.owner->m_dfv_style->m_props.line_nums_width <= 0)
	{
		// Props of the whole file are saying that the line numbers area is not visible.
		return(TRUE);
	}

	// This type of decor _is_ merged with existing line num decors. Check if this location is already decorated with the passed props.
	int inx_ib = -1;
	int decor_inx = -1;
	for (int i=0; i<m_whole_line_lnum_frgrs.NumItems(); ++i)
	{
		TDFVLineNumFrgrDecoration &decor = m_whole_line_lnum_frgrs[i];

		if (decor.zorder > zorder)
		{
			inx_ib = i;
			break;
		}

		if (decor.zorder == zorder && decor.dcr_owner == decor_owner && decor.dcr_owner_data == owner_data)
		{
			decor_inx = i;
			break;
		}
	}

	if (decor_inx >= 0)
	{
		// Simply replace the style pointer.
		m_whole_line_lnum_frgrs[decor_inx].lnum_frgr_style = style;
	}
	else
	{
		// Add new record to the array.
		TDFVLineNumFrgrDecoration decor = { decor_owner, owner_data, zorder, style };
		if (inx_ib >= 0)
		{
			if (m_whole_line_lnum_frgrs.InsertItem(inx_ib, decor) == FALSE)
				return(FALSE);

			decor_inx = inx_ib;
		}
		else
		{
			if (m_whole_line_lnum_frgrs.AppendItem(decor) == FALSE)
				return(FALSE);

			decor_inx = m_whole_line_lnum_frgrs.NumItems()-1;
		}
	}

	if (decor_inx == m_whole_line_lnum_frgrs.NumItems()-1)
	{
		// The bypassed text style became the topmost one. Show it.
		m_line_num_scr_item.SetOwner(decor_owner, owner_data);
		SetLineNumScreenItemStyleAndPosition(ctx, ctx.owner->m_nums_rect_scr_item.GetBaseLeft(), style);

		if (ProcessTextExtentsVertImpact(ctx) == TRUE)
		{
			// Height of the line has increased or decreased.
			UpdateWholeLineBkgrDecors(ctx);
		}

		ReportFullAreaInvalid(ctx.owner);
	}

	// Full success.
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddWholeLineImposedDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
															TScreenItem *scr_item, long horz_offs, long baseline_offs)
{
	//
	//  This type of decor cannot be merged with existing decors.
	//

	// Look for the place in the list where the new object should be inserted.
	int inx_ib = m_whole_line_imposed.NumItems();
	for (int i=0; i<m_whole_line_imposed.NumItems(); ++i)
	{
		if (m_whole_line_imposed[i].zorder >= zorder)
		{
			inx_ib = i;
			break;
		}
	}

	// Insert the object into the list.
	TDFVWholeLineImposedDecoration decor = { zorder, scr_item };
	if (m_whole_line_imposed.InsertItem(inx_ib, decor) == FALSE)
		return(FALSE);

	// Insertion succeeded. Prepare the decoration.
	scr_item->SetOwner(decor_owner, owner_data);
	scr_item->SetHotSpotPos(ctx.owner->m_text_rect_scr_item.GetBaseLeft()+horz_offs, m_baseline+baseline_offs);

	// Process the impact of the new decoration on the line.
	EvaluateImposedExtents(ctx.owner, scr_item);
	ctx.owner->AddToInvalidRect(*(scr_item->GetBoundingRect()));
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddWholeLineSpacingDecor(TDFVAddRemoveDecorsContext &ctx, void *decor_owner, DWORD owner_data, short zorder,
															bool above_spacing, short new_spacing_value, bool accept_wlbk_color)
{
	// Find compatible decor. This type of decor does not have visible zorder relation.
	// New records will be added always to the end.
	TExtraSpcDecorsArray &dci = (above_spacing == TRUE) ? m_whole_line_upper_exts : m_whole_line_lower_exts;
	int decor_inx = -1;
	for (int i=0; i<dci.NumItems(); ++i)
	{
		if (dci[i].spc_owner == decor_owner && dci[i].spc_owner_data == owner_data && dci[i].spc_zorder == zorder && dci[i].spc_wlbk_use_wlbk_color == accept_wlbk_color)
		{
			decor_inx = i;
			break;
		}
	}

	if (decor_inx >= 0)
	{
		// Update the existing record.
		dci[decor_inx].spc_wlbk_value = new_spacing_value;
	}
	else
	{
		// Add new record to the end of array.
		TDFVExtraSpcDecoration decor = { decor_owner, owner_data, zorder };
		decor.spc_wlbk_use_wlbk_color = accept_wlbk_color;
		decor.spc_wlbk_value = new_spacing_value;

		if (dci.AppendItem(decor) == FALSE)
			return(FALSE);
	}

	// Check the vertical consequencies of applying this decor.
	if (above_spacing == TRUE)
	{
		if (new_spacing_value > m_upper_extent)
		{
			long baseline_shift = new_spacing_value-m_upper_extent;
			ctx.owner->ReportVertDelta(baseline_shift, ctx.line_num);
			m_upper_extent = new_spacing_value;
			ShiftY(ctx.owner, baseline_shift);

			if (accept_wlbk_color == TRUE && new_spacing_value > m_upper_wlbk_ext)
				m_upper_wlbk_ext = new_spacing_value;

			UpdateWholeLineBkgrDecors(ctx);
		}
		else if (accept_wlbk_color == TRUE && new_spacing_value > m_upper_wlbk_ext)
		{
			m_upper_wlbk_ext = new_spacing_value;
			UpdateWholeLineBkgrDecors(ctx);
		}
	}
	else
	{
		if (new_spacing_value > m_lower_extent)
		{
			ctx.owner->ReportVertDelta(new_spacing_value-m_lower_extent, ctx.line_num);
			m_lower_extent = new_spacing_value;

			if (accept_wlbk_color == TRUE && new_spacing_value > m_lower_wlbk_ext)
				m_lower_wlbk_ext = new_spacing_value;

			UpdateWholeLineBkgrDecors(ctx);
		}
		else if (accept_wlbk_color == TRUE && new_spacing_value > m_lower_wlbk_ext)
		{
			m_lower_wlbk_ext = new_spacing_value;
			UpdateWholeLineBkgrDecors(ctx);
		}
	}

	// Decor is applied. State of decors may be not final because of the delayed horz actions.
	return(TRUE);
}

void TDecoratedFileViewLineInfo::RemoveDecor(TDecoratedFileViewScreenItem *dfv, void *decor_owner, short zorder_code)
{
	// Save the initial line height.
	long old_line_upper = GetFullHeightAbove();
	long old_full_height = GetFullHeight();

	//
	//  Search the foreground decors group first.
	//
	bool frgr_removed = FALSE;
	int  last_ln_num_removed = -1;

	// (1) Foreground text decorations.
	TListIter<TDFVFrgrDecoration> frgr_iter1(m_frgr_decors);
	while (frgr_iter1)
	{
		TDFVFrgrDecoration *decor = &frgr_iter1.CurrItem();

		// Shift the iterator now because the current item can be deleted.
		++frgr_iter1;

		if ((decor_owner == NULL || decor->GetOwner() == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || decor->zorder == zorder_code))
		{
			m_frgr_decors.RemoveItem(decor);
			delete decor;

			frgr_removed = TRUE;
		}
	}

	// (2) Text selection decorations.
	for (int i1=0; i1<m_slct_decors.NumItems(); ++i1)
	{
		TDFVSlctDecoration &decor = m_slct_decors[i1];
		if ((decor_owner == NULL || decor.slct_bkgr_item.GetOwner() == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || decor.zorder == zorder_code))
		{
			m_slct_decors.ReleaseItem(i1--);
			frgr_removed = TRUE;
		}
	}

	// (3) Horz spacing decorations and spec tabs.
	for (int i2=0; i2<m_spacing_decors.NumItems(); ++i2)
	{
		TDFVExtraSpcDecoration &decor = m_spacing_decors[i2];
		if ((decor_owner == NULL || decor.spc_owner == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || decor.spc_zorder == zorder_code))
		{
			m_spacing_decors.ReleaseItem(i2--);
			frgr_removed = TRUE;
		}
	}

	// (4) Line number decors are not part of the frgr painting list, but they have similar impact on the line height.
	// Because of this process them in the frgr decors group.
	for (int i3=0; i3<m_whole_line_lnum_frgrs.NumItems(); ++i3)
	{
		TDFVLineNumFrgrDecoration &decor = m_whole_line_lnum_frgrs[i3];
		if ((decor_owner == NULL || decor.dcr_owner == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || decor.zorder == zorder_code))
		{
			last_ln_num_removed = i3;
			m_whole_line_lnum_frgrs.ReleaseItem(i3--);
		}
	}

	// Check results of the line number decors removal.
	if (last_ln_num_removed >= 0)
	{
		// Some line number decors were removed.
		TDFVAddRemoveDecorsContext ctx(dfv);
		long nums_x_pos = dfv->m_nums_rect_scr_item.GetBaseLeft();
		if (m_whole_line_lnum_frgrs.NumItems() == 0)
		{
			// All line number decors are gone. Switch back to the default line number style.
			m_line_num_scr_item.SetOwner(NULL);
			SetLineNumScreenItemStyleAndPosition(ctx, nums_x_pos, dfv->m_dfv_style->m_line_nums_style);
		}
		else if (last_ln_num_removed == m_whole_line_lnum_frgrs.NumItems())
		{
			// The topmost line number decor has changed.
			TDFVLineNumFrgrDecoration &new_topmost_decor = m_whole_line_lnum_frgrs[last_ln_num_removed-1];
			m_line_num_scr_item.SetOwner(new_topmost_decor.dcr_owner, new_topmost_decor.dcr_owner_data);
			SetLineNumScreenItemStyleAndPosition(ctx, nums_x_pos, new_topmost_decor.lnum_frgr_style);
		}

		ReportFullAreaInvalid(dfv);
	}

	// Announce the need to update the frgr list and reveal the text exts.
	if (frgr_removed == TRUE)
	{
		dfv->ReportFrgrUpdateWanted(this);
		m_invalid_scr_items = TRUE;
	}

	if (frgr_removed == TRUE || last_ln_num_removed >= 0)
	{
		CalcVerticalTextExtents(dfv);
	}

	//
	//  Search the background group.
	//
	bool wl_bkgr_update_req = FALSE;

	// (5) Search among background decorations and rebuild the bkgr_exts.
	m_ext_bkgr_above = m_ext_bkgr_below = 0;
	m_indirect_hgt_bkgrs = FALSE;
	TListIter<TDFVRectDecoration> bkgr_iter1(m_bkgr_decors);
	while (bkgr_iter1 == TRUE)
	{
		TDFVRectDecoration *decor = &bkgr_iter1.CurrItem();

		// Shift the iterator now because the current item can be deleted.
		++bkgr_iter1;

		if ((decor_owner == NULL || decor->GetOwner() == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || decor->zorder == zorder_code))
		{
			m_bkgr_decors.RemoveItem(decor);
			dfv->AddToInvalidRect(*(decor->GetBoundingRect()));
			delete decor;

			wl_bkgr_update_req = TRUE;
		}
		else
		{
			// Decor survived.
			if (decor->upper_ext > m_ext_bkgr_above)
				m_ext_bkgr_above = decor->upper_ext;
			if (decor->lower_ext > m_ext_bkgr_below)
				m_ext_bkgr_below = decor->lower_ext;
			if (decor->upper_ext < 0 || decor->lower_ext < 0)
				m_indirect_hgt_bkgrs = TRUE;
		}
	}

	// (6) Search among the whole line spacings and reevaluate the extensions.
	if (RemoveVertSpacingDecors(m_whole_line_upper_exts, decor_owner, zorder_code, m_upper_extent, m_upper_wlbk_ext) == TRUE)
		wl_bkgr_update_req = TRUE;
	if (RemoveVertSpacingDecors(m_whole_line_lower_exts, decor_owner, zorder_code, m_lower_extent, m_lower_wlbk_ext) == TRUE)
		wl_bkgr_update_req = TRUE;

	// (7) Search among the whole line background decorations. Reset the flag that marks dependency
	// on the line contents and rebuild it from the survived decors only.
	m_frgrw_sens_wlbks = FALSE;
	TListIter<TDFVRectDecoration> bkgr_iter2(m_whole_line_bkgrs);
	while (bkgr_iter2 == TRUE)
	{
		TDFVRectDecoration *decor = &bkgr_iter2.CurrItem();

		// Shift the iterator now because the current item can be deleted.
		++bkgr_iter2;

		if ((decor_owner == NULL || decor->GetOwner() == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || decor->zorder == zorder_code))
		{
			// Decor dies.
			m_whole_line_bkgrs.RemoveItem(decor);
			dfv->AddToInvalidRect(*(decor->GetBoundingRect()));
			delete decor;

			wl_bkgr_update_req = TRUE;
		}
		else
		{
			// Decor survived. Check if it depends on the right edge of the frgr.
			WORD fple_flags = decor->cover_mask & (wlbk_mtext | wlbk_rtext);
			if (fple_flags == wlbk_mtext || fple_flags == wlbk_rtext)
				m_frgrw_sens_wlbks = TRUE;
		}
	}

	// Check the vertical consequences of all removal steps above.
	if (frgr_removed == TRUE || last_ln_num_removed >= 0 || wl_bkgr_update_req == TRUE)
	{
		// All exts should be already reevaluated. Check if the baseline or the full height of the line has changed.
		long line_upper_delta  = GetFullHeightAbove()-old_line_upper;
		if (line_upper_delta != 0)
			ShiftY(dfv, line_upper_delta);

		TDFVAddRemoveDecorsContext ctx(dfv);
		UpdateWholeLineBkgrDecors(ctx);
		dfv->m_vert_delta_pix_value += GetFullHeight()-old_full_height;
	}

	//
	// Imposed decorations (except for inserted decors) do not affect other decorations. Process them during the last step.
	//
	bool imposed_removed = FALSE;

	// (8) Search among the text fragment imposed decorations. Imposed screen items are owned by
	// the decorated file object. This means that they should be released here.
	for (int i4=0; i4<m_imposed_decors.NumItems(); ++i4)
	{
		TDFVInlineImposedDecoration &decor = m_imposed_decors[i4];
		assert(decor.scr_item != NULL);

		if ((decor_owner == NULL || decor.scr_item->GetOwner() == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || decor.zorder == zorder_code))
		{
			dfv->AddToInvalidRect(*(decor.scr_item->GetBoundingRect()));
			delete decor.scr_item;

			m_imposed_decors.ReleaseItem(i4--);
			imposed_removed = TRUE;
		}
	}

	// (9) Search among the whole line imposed decorations. Imposed screen items are owned by
	// the decorated file object. This means that they should be released here.
	for (int i5=0; i5<m_whole_line_imposed.NumItems(); ++i5)
	{
		TDFVWholeLineImposedDecoration &decor = m_whole_line_imposed[i5];
		assert(decor.wlimp_scr_item != NULL);

		if ((decor_owner == NULL || decor.wlimp_scr_item->GetOwner() == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || decor.zorder == zorder_code))
		{
			dfv->AddToInvalidRect(*(decor.wlimp_scr_item->GetBoundingRect()));
			delete decor.wlimp_scr_item;

			m_whole_line_imposed.ReleaseItem(i5--);
			imposed_removed = TRUE;
		}
	}

	// Check results of imposed decors removal.
	if (imposed_removed == TRUE)
	{
		// It is necessary to recalculate the imposed decors extents.
		m_imposed_above = m_imposed_below = 0;

		for (int inx1=0; inx1<m_imposed_decors.NumItems(); ++inx1)
		{
			EvaluateImposedExtents(dfv, m_imposed_decors[inx1].scr_item);
		}

		for (int inx2=0; inx2<m_whole_line_imposed.NumItems(); ++inx2)
		{
			EvaluateImposedExtents(dfv, m_whole_line_imposed[inx2].wlimp_scr_item);
		}
	}
}

// /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
//    --------------------  Suport methods ---------------------
// \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

bool TDecoratedFileViewLineInfo::PrepareScreenItems(TDFVPrepareScreenItemsContext &ctx)
{
	// Check for the first trivial case when the line is empty.
	if (m_default_text.GetBasicStyle() == NULL)
	{
		// The line itself is empty. Check for selection decors with horz extensions.
		for (int i1=0; i1<m_slct_decors.NumItems(); ++i1)
		{
			TDFVSlctDecoration &decor = m_slct_decors[i1];
			if (decor.slct_bkgr_item.GetBasicStyle() == NULL || decor.beg_end_line_ext <= 0)
				continue;

			// Prepare and set a small rect near the beg of the line.
			RECT bkgr_rect;
			bkgr_rect.left = ctx.text_x_pos-decor.beg_end_line_ext;
			bkgr_rect.right = ctx.text_x_pos+decor.beg_end_line_ext;
			bkgr_rect.top = m_baseline - m_text_above;
			bkgr_rect.bottom = m_baseline + m_text_below;
			decor.slct_bkgr_item.SetBaseRect(bkgr_rect);
		}

		// Check for imposed decors.
		for (int i2=0; i2<m_imposed_decors.NumItems(); ++i2)
		{
			TDFVInlineImposedDecoration &decor = m_imposed_decors[i2];
			decor.scr_item->SetHotSpotPos(ctx.text_x_pos+decor.sym_border_horz_offs, m_baseline+decor.baseline_vert_offs);
			ctx.owner->AddToInvalidRect(*(decor.scr_item->GetBoundingRect()));
		}

		m_invalid_scr_items = FALSE;
		return(TRUE);
	}

	// Check for another trivial case when there are no decors that make the horz structure non trivial.
	if (IsSimpleData(ctx.owner) == TRUE && m_frgr_decors.IsEmpty() == TRUE && m_slct_decors.NumItems() == 0 && m_spacing_decors.NumItems() == 0)
	{
		// The last condition that may prevent mapping only default_text is the horz overflow.
		// It will be checked inside the final function.
		return(PrepareScreenItemsWithNoDecors(ctx));
	}

	// Run the general procedure.
	ctx.x_limit = ctx.owner->m_max_text_size.cx;
	return(PrepareScreenItemsInternal(ctx));
}

bool TDecoratedFileViewLineInfo::GenerateHtmlLine(TDFVHtmlGenerationContext &gen_ctx, int line_number)
{
	bool overall_success = TRUE;
	gen_ctx.m_low_level_owner_alloc_errors = 0;

	// Select default css styles for the columns of the table row.
	const wchar_t *nums_css_style = gen_ctx.m_dfv->m_dfv_style->Props().line_nums_css_style;
	const wchar_t *marks_css_style = gen_ctx.m_dfv->m_dfv_style->Props().marks_area_css_style;
	const wchar_t *data_css_style = gen_ctx.m_dfv->m_dfv_style->Props().text_area_css_style;

	TListIter<TDFVRectDecoration> iter1(m_whole_line_bkgrs);
	for (iter1.SeekToLast(); iter1; --iter1)
	{
		if ((iter1.CurrItem().cover_mask & wlbk_nums) != 0 && iter1.CurrItem().GetCssStyleName() != NULL)
		{
			// This is the topmost wlbk decor that covers the line nums column.
			nums_css_style = iter1.CurrItem().GetCssStyleName();
			break;
		}
	}

	TListIter<TDFVRectDecoration> iter2(m_whole_line_bkgrs);
	for (iter2.SeekToLast(); iter2; --iter2)
	{
		if ((iter2.CurrItem().cover_mask & wlbk_marks) != 0 && iter2.CurrItem().GetCssStyleName() != NULL)
		{
			// This is the topmost wlbk decor that covers the marks column.
			marks_css_style = iter2.CurrItem().GetCssStyleName();
			break;
		}
	}

	TListIter<TDFVRectDecoration> iter3(m_whole_line_bkgrs);
	for (iter3.SeekToLast(); iter3; --iter3)
	{
		if ((iter3.CurrItem().cover_mask & wlbk_text) == wlbk_text && iter3.CurrItem().GetCssStyleName() != NULL)
		{
			// This is the topmost wlbk decor that covers the line data column.
			data_css_style = iter3.CurrItem().GetCssStyleName();
			break;
		}
	}

	// Apply the selected whole line styles.
	gen_ctx.m_line_gen.SetWholeLineStyles(nums_css_style, marks_css_style, data_css_style);

	// Setup the line column.
	if (m_line_num_scr_item.GetCssStyleName() != NULL)
	{
		TRawHtmlElement *low_level_owner = gen_ctx.FindOrCreateLowLevelOwner(m_line_num_scr_item);
		if (low_level_owner != NULL)
			gen_ctx.m_line_gen.SetLineNumberStyle(low_level_owner);
	}

	// Kill decors on the line body from the prev line if any.
	gen_ctx.m_line_gen.LineDecors().ResetDecorations();

	// Apply the background decorations.
	for (TListIter<TDFVRectDecoration> iter4(m_bkgr_decors); iter4; ++iter4)
	{
		TDFVRectDecoration &bkgr_decor = iter4.CurrItem();
		if (bkgr_decor.GetCssStyleName() != NULL && bkgr_decor.num_chars > 0)
		{
			TRawHtmlElement *low_level_owner = gen_ctx.FindOrCreateLowLevelOwner(bkgr_decor);
			if (low_level_owner != NULL)
				overall_success &= gen_ctx.m_line_gen.LineDecors().SetBkgrDecor(low_level_owner, bkgr_decor.zorder, bkgr_decor.char_beg, bkgr_decor.num_chars);
		}
	}

	// Apply the foreground decorations.
	for (TListIter<TDFVFrgrDecoration> iter5(m_frgr_decors); iter5; ++iter5)
	{
		TDFVFrgrDecoration &frgr_decor = iter5.CurrItem();
		if (frgr_decor.GetCssStyleName() != NULL && frgr_decor.num_chars > 0)
		{
			TRawHtmlElement *low_level_owner = gen_ctx.FindOrCreateLowLevelOwner(frgr_decor);
			if (low_level_owner != NULL)
				overall_success &= gen_ctx.m_line_gen.LineDecors().SetFrgrDecor(low_level_owner, frgr_decor.zorder, frgr_decor.char_beg, frgr_decor.num_chars);
		}
	}

	if (gen_ctx.m_low_level_owner_alloc_errors > 0)
		overall_success = FALSE;

	// Emit the line.
	long line_len = m_default_text.num_chars;
	overall_success &= gen_ctx.m_line_gen.EmitLine(gen_ctx.m_emt_hlpr, line_number, m_default_text.GetBody(), line_len);
	return(overall_success);
}

// /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
//   --------------------  Private methods ---------------------
// \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

void TDecoratedFileViewLineInfo::ResetFrgrPaintingList(TDFVPrepareScreenItemsContext &ctx)
{
	// This list is not owning its items. Simply clear it.
	m_frgr_painting_list.Clear();

	// Return additional objects into the context.
	ctx.RecycleExtraFrgrOblects(m_extra_frgr_objects);
	ctx.RecycleNonAsciiOblects(m_non_ascii_objects);
	ctx.RecycleVisibSpcOblects(m_visib_spc_objects);

	// Default text and foreground decors might be used in the list.
	m_default_text.paint_link.Clear();
	for (TListIter<TDFVFrgrDecoration> iter(m_frgr_decors); iter; ++iter)
	{
		iter.CurrItem().paint_link.Clear();
	}

	// Check if the OutOfMem message is displayed on the curr line or not.
	if (ctx.owner->m_out_of_mem_msg_line == this)
	{
		ctx.owner->m_out_of_mem_message.paint_link.Clear();
		ctx.owner->m_out_of_mem_msg_line = NULL;
	}
}

bool TDecoratedFileViewLineInfo::PrepareScreenItemsWithNoDecors(TDFVPrepareScreenItemsContext &ctx)
{
	// This function can be called only for lines with simple data and when visible spaces, visible tabs and
	// implicit multispace tabs are all switched OFF.
	ResetFrgrPaintingList(ctx);
	m_invalid_scr_items = FALSE;

	// Check if the object has restriction on the horz width or not.
	if (ctx.owner->m_max_text_size.cx != 0)
	{
		// Try the full width of the line. If the data will not fit, fail back to the generic procedure.
		long req_length = m_default_text.Style()->GetStringWidth(ctx.hDC, m_default_text.GetBody(), m_default_text.num_chars);
		if (req_length > ctx.owner->m_max_text_size.cx)
		{
			return(PrepareScreenItemsInternal(ctx));
		}
	}

	//
	//  The simple mapping procedure is possible. Only the m_default_text should be present in the frgr painting list.
	//  It is ok not to check anything about the bkgr item because the horz spacing cannot be present in this case.
	//

	ctx.ctx_iend = 0;
	ctx.ClearExtentsInfo();
	long width = m_default_text.PlaceObject(ctx.hDC, ctx.text_x_pos, m_baseline)-ctx.text_x_pos;
	ctx.FinalizeFrgrItemPlacing(this, m_default_text.paint_link, width);

	// Report the text area enlargement if any and the invalid area.
	if (width > ctx.owner->m_text_size.cx)
	{
		ctx.owner->m_text_size.cx = width;
	}

	ReportTextAreaInvalid(ctx.owner);
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::PrepareScreenItemsInternal(TDFVPrepareScreenItemsContext &ctx)
{
	// Set up the process.
	ctx.curr_pix = 0;
	ctx.horz_overflow = FALSE;
	ResetFrgrPaintingList(ctx);
	m_invalid_scr_items = FALSE;

	// This method should be called only for non empty lines. Ensure this.
	long line_len = m_default_text.num_chars;
	assert(line_len > 0);

	//
	// Step 1. Prepare the inner/outer extents for all char places that require this info.
	//

	// Clear the horz rendering info array. This array is allocated at the beginning according to the length
	// of the longest line in the file plus one element. Because of that the length of this array is always
	// big enough to process any line of the file.
	TDFVLineRenderCharPlaceInfo *horz_info = ctx.owner->m_line_rendering_info;
	memset(horz_info, 0, (line_len+1)*sizeof(TDFVLineRenderCharPlaceInfo));
	ctx.owner->m_line_render_info_len = line_len;

#ifdef _DEBUG
	// Fill in the frgr painting list item offsets with the negative values starting from the first char place.
	// Finalize procedure will fill offset for the next painting list item. The first item should have a zero offset.
	for (int ich=1; ich <= line_len; ++ich)
		horz_info[ich].m_painting_list_item_offs = -1;
#endif

	// 1.1. Find out the integral state of the inner/outer extents of the inline bkgr decors.
	for (TListIter<TDFVRectDecoration> iter1(m_bkgr_decors); iter1; ++iter1)
	{
		TDFVRectDecoration &decor = iter1.CurrItem();
		const TBasicStyleProps &decor_props = decor.Style()->m_props;
		assert(decor.num_chars > 0);

		// Process the inner/outer magrins on the left side.
		TDFVLineRenderCharPlaceInfo &bkgr_area_beg = horz_info[decor.char_beg];

		if (decor_props.inner_margin_left > bkgr_area_beg.m_bkgr_max_inner_front_ext)
			bkgr_area_beg.m_bkgr_max_inner_front_ext = decor_props.inner_margin_left;
		if (decor_props.outer_padding_left > bkgr_area_beg.m_max_outer_front_ext)
			bkgr_area_beg.m_max_outer_front_ext = decor_props.outer_padding_left;

		// Process the inner/outer magrins on the right side.
		TDFVLineRenderCharPlaceInfo &bkgr_area_end = horz_info[decor.char_beg+decor.num_chars-1];

		if (decor_props.inner_margin_right > bkgr_area_end.m_bkgr_max_inner_back_ext)
			bkgr_area_end.m_bkgr_max_inner_back_ext = decor_props.inner_margin_right;
		if (decor_props.outer_padding_right > bkgr_area_end.m_max_outer_back_ext)
			bkgr_area_end.m_max_outer_back_ext = decor_props.outer_padding_right;

		// Put marks telling that new frgr painting list items should be started on the boundaries.
		bkgr_area_beg.m_frgr_plist_break = TRUE;
		horz_info[decor.char_beg+decor.num_chars].m_frgr_plist_break = TRUE;
	}

	// 1.2. Apply foreground decorations in their existing order. This will assign the topmost frgr decoration
	// if any to each character position because decors are stored in the order of increasing their zorder.
	for (TListIter<TDFVFrgrDecoration> iter2(m_frgr_decors); iter2; ++iter2)
	{
		TDFVFrgrDecoration &decor = iter2.CurrItem();
		if (decor.GetBasicStyle() == NULL)
			continue;

		TDFVLineRenderCharPlaceInfo *frgr_info = horz_info+decor.char_beg;
		for (int i2=0; i2<decor.num_chars; ++i2, ++frgr_info)
			frgr_info->m_frgr_overlap_state = &decor;
	}

	// Process the extents on the beginnings and ends of all topmost frgr decorations and of all non
	// topmost decors that have non transp background or frame.
	for (TListIter<TDFVFrgrDecoration> iter3(m_frgr_decors); iter3; ++iter3)
	{
		TDFVFrgrDecoration &decor = iter3.CurrItem();
		if (decor.GetBasicStyle() == NULL)
			continue;

		const TTextStyleProps &decor_props = decor.Style()->m_props;
		TDFVLineRenderCharPlaceInfo &frgr_area_beg = horz_info[decor.char_beg];
		TDFVLineRenderCharPlaceInfo &frgr_area_end = horz_info[decor.char_beg+decor.num_chars-1];
		TDFVFrgrDecoration *edcr_beg = frgr_area_beg.m_frgr_overlap_state;
		TDFVFrgrDecoration *edcr_end = frgr_area_end.m_frgr_overlap_state;

		// Process the left side magrins.
		if (ctx.SameFrgrDecorBefore(decor) == FALSE && (&decor == edcr_beg || decor.bkgr_item.GetBasicStyle() != NULL))
		{
			// Beginning of the current decoration should be processed.
			if (decor_props.front_side_ext > frgr_area_beg.m_frgr_max_inner_front_ext)
				frgr_area_beg.m_frgr_max_inner_front_ext = decor_props.front_side_ext;
			if (decor_props.outer_front_padding > frgr_area_beg.m_max_outer_front_ext)
				frgr_area_beg.m_max_outer_front_ext = decor_props.outer_front_padding;

			if (&decor != edcr_beg)
				frgr_area_beg.m_frgr_plist_break = TRUE;
		}

		// Process the right side magrins.
		if (ctx.SameFrgrDecorAfter(decor) == FALSE && (&decor == edcr_end || decor.bkgr_item.GetBasicStyle() != NULL))
		{
			// Ending of the current decoration should be processed.
			if (decor_props.back_side_ext > frgr_area_end.m_frgr_max_inner_back_ext)
				frgr_area_end.m_frgr_max_inner_back_ext = decor_props.back_side_ext;
			if (decor_props.outer_back_padding > frgr_area_end.m_max_outer_back_ext)
				frgr_area_end.m_max_outer_back_ext = decor_props.outer_back_padding;

			if (&decor != edcr_end)
				horz_info[decor.char_beg+decor.num_chars].m_frgr_plist_break = TRUE;
		}
	}

	// 1.3. Apply selection decorations in their existing order.
	for (int i4=0; i4<m_slct_decors.NumItems(); ++i4)
	{
		TDFVSlctDecoration &decor = m_slct_decors[i4];
		TDFVLineRenderCharPlaceInfo *slct_info = horz_info+decor.char_beg;
		for (int i4=0; i4<decor.num_chars; ++i4, ++slct_info)
			slct_info->m_slct_overlap_state = &decor;
	}

	// 1.4. Find out the integral state of the inline ext spacing decors and spec tabs.
	int spec_tabs_noticed = 0;
	for (int i5=0; i5<m_spacing_decors.NumItems(); ++i5)
	{
		TDFVExtraSpcDecoration &decor = m_spacing_decors[i5];
		if (decor.spc_spec_tab == FALSE)
		{
			// This is a regular horz spacing decor.
			if (decor.spc_char_inx > 0)
			{
				TDFVLineRenderCharPlaceInfo &espc_left = horz_info[decor.spc_char_inx-1];
				if (decor.spc_horz_value_left > espc_left.m_horz_spc_back_ext)
					espc_left.m_horz_spc_back_ext = decor.spc_horz_value_left;
			}

			TDFVLineRenderCharPlaceInfo &espc_right = horz_info[decor.spc_char_inx];
			if (decor.spc_horz_value_right > espc_right.m_horz_spc_front_ext)
				espc_right.m_horz_spc_front_ext = decor.spc_horz_value_right;

			espc_right.m_frgr_plist_break = TRUE;
		}
		else if (m_spec_tabs_area_index >= 0 && spec_tabs_noticed < TDFVSpecTabsArea::MAX_SUBCOLUMNS)
		{
			// Current line belongs to the range of lines where the spec tabs are allowed
			// and the number of spec tabs on the line does not exceed the allowed maximum.
			TDFVLineRenderCharPlaceInfo &spct_chplc = horz_info[decor.spc_char_inx];
			if (spct_chplc.m_spec_tab_flag == FALSE)
			{
				spct_chplc.m_spec_tab_flag = TRUE;
				spct_chplc.m_frgr_plist_break = TRUE;
				spec_tabs_noticed++;
			}
		}
	}

	// 1.5. Add extra spacing, that is needed for placing the inserted decors to the spacings of the horz spc decors.
	// This procedure should be carried out after processing the spacings because that proc is finding max values
	// and this procedure is adding surplus to what was discovered at that time.
	for (int i6=0; i6<m_imposed_decors.NumItems(); ++i6)
	{
		TDFVInlineImposedDecoration &decor = m_imposed_decors[i6];
		if (decor.insert_decor == TRUE)
		{
			// Current record describes the inserted decoration.
			long decor_width = decor.scr_item->GetWidth();

			if (decor.char_inx == 0)
			{
				// Add the width of this decor to the left side of the line.
				horz_info[0].m_horz_spc_front_ext += decor_width;
			}
			else if (decor.char_inx < line_len)
			{
				// The decor is inserted into the middle of the line. Check the horz order.
				if (decor.insert_horz_order < 0)
				{
					// Decor is considered to be part of the previous character.
					horz_info[decor.char_inx-1].m_horz_spc_back_ext += decor_width;
				}
				else
				{
					// Decor is considered to be part of the next character.
					horz_info[decor.char_inx].m_horz_spc_front_ext += decor_width;
				}
			}
			else
			{
				// Add the width of this decor to the left side of the line.
				horz_info[line_len-1].m_horz_spc_back_ext += decor_width;
			}

			horz_info[decor.char_inx].m_frgr_plist_break = TRUE;
		}
	}

	//
	// 1.6. Merge the outer paddings of the bkgr/frgr decorations into the horz spacings.
	// After this merge these outer padding will be no longer needed.
	//

	// Normalize the outer spacing on the left side of the first character.
	if (horz_info->m_max_outer_front_ext > horz_info->m_horz_spc_front_ext)
		horz_info->m_horz_spc_front_ext = horz_info->m_max_outer_front_ext;

	// Normalize the outer spacings on the left side of all char places except for the first char.
	// This will also preprare the right spacing of the char that stays in front of the curr char.
	TDFVLineRenderCharPlaceInfo *prev_info = horz_info, *curr_info = horz_info+1;
	for (int i7=1; i7<line_len; ++i7, ++prev_info, ++curr_info)
	{
		long front_outer_margin = ___max(prev_info->m_max_outer_back_ext, curr_info->m_max_outer_front_ext);
		long front_horz_extspc = prev_info->m_horz_spc_back_ext+curr_info->m_horz_spc_front_ext;

		if (front_outer_margin > front_horz_extspc)
		{
			// It is necessary to enlarge the horz spacings to satisfy the outer margin conditions.
			long diff = front_outer_margin-front_horz_extspc;

			if (prev_info->m_horz_spc_back_ext == curr_info->m_horz_spc_front_ext)
			{
				// Both horz spacings are the same (maybe zero). Split the diff equaly.
				curr_info->m_horz_spc_front_ext = diff/2;
				prev_info->m_horz_spc_back_ext = diff-curr_info->m_horz_spc_front_ext;
			}
			else if (prev_info->m_horz_spc_back_ext == 0)
			{
				curr_info->m_horz_spc_front_ext += diff;
			}
			else if (curr_info->m_horz_spc_front_ext == 0)
			{
				prev_info->m_horz_spc_back_ext += diff;
			}
			else
			{
				// Both horz spacings are not zero and they even are not the same.
				long delta1 = (curr_info->m_horz_spc_front_ext*diff)/front_horz_extspc;
				curr_info->m_horz_spc_front_ext += delta1;
				prev_info->m_horz_spc_back_ext += diff-delta1;
			}

			// Ensure that the difference was properly distributed.
			assert(prev_info->m_horz_spc_back_ext+curr_info->m_horz_spc_front_ext == front_outer_margin);
		}
	}

	// Normalize the outer spacing on the right side of the last character on the line.
	TDFVLineRenderCharPlaceInfo &last_char_info = horz_info[line_len-1];
	if (last_char_info.m_max_outer_back_ext > last_char_info.m_horz_spc_back_ext)
		last_char_info.m_horz_spc_back_ext = last_char_info.m_max_outer_back_ext;

	//
	// Step 2. Prepare the foreground painting list.
	//
	bool res = TRUE;
	wchar_t *line_data = m_default_text.GetBody();
	assert(line_data >= ctx.owner->m_file_data && line_data+line_len <= ctx.owner->m_file_data+ctx.owner->m_file_data_len);
	int num_spec_tabs_processed = 0;

	int  ib = 0;
	while (ib < line_len)
	{
		// Find consequtive area that belongs to the same frgr and slct decorations.
		TDFVLineRenderCharPlaceInfo *outer_loop_info = horz_info+ib;
		TDFVFrgrDecoration *frgr_area = outer_loop_info->m_frgr_overlap_state;
		TDFVSlctDecoration *slct_area = outer_loop_info->m_slct_overlap_state;

		int ic = ib+1;
		outer_loop_info++;
		while (ic < line_len)
		{
			if (outer_loop_info->m_frgr_overlap_state != frgr_area)
				break;
			else if (outer_loop_info->m_slct_overlap_state != slct_area)
				break;
			else if (outer_loop_info->m_frgr_plist_break == TRUE)
				break;

			ic++;
			outer_loop_info++;
		}

		// The area, starting from ib up to ic and not including it, has identical background, foreground and
		// selection styles. Plus it does not contain, inserted decors, horz spacings and spec tabs in the middle
		// and it is not empty.
		ctx.frgr_area = frgr_area;
		ctx.slct_area = slct_area;

		// This area may need further subdivision in case if it contains visible spacings, non ASCII chars or
		// XML escape sequencies that require substitutions.
		int ig = ib;
		while (ig < ic)
		{
			short icon_index;
			wchar_t xml_char;
			long xml_seq_len;
			int break_reason = 0;					// End of the sequence or some extra horz spacing.

			// Find consequtive area that can be decribed with one text screen item.
			int ik = ig;
			while (ik < ic)
			{
				wchar_t ch = line_data[ik];
				if (ch == L'\t')
				{
					break_reason = 1;				// Tab char.
					break;
				}
				else if (ch == L' ')
				{
					if (ctx.owner->m_dfv_style->m_tabs_mode.implicit_multisp_tabs == TRUE)
					{
						if (ik < ic-1 && line_data[ik+1] == L' ')
						{
							break_reason = 2;		// This is more than one space in a row. This is an implicit tab.
							break;				// Note that the number of consequtive spaces is not known yet.
						}
					}

					if (ctx.VisibleSpaces() == TRUE)
					{
						break_reason = 3;			// Visible space.
						break;
					}
				}
				else if (ch < 0x20)
				{
					if (ctx.owner->m_dfv_style->CheckForNonAsciiSubsts(ch, (slct_area != NULL) ? TRUE : FALSE, icon_index) == TRUE)
					{
						break_reason = 4;				// Non ASCII subst.
						break;
					}
				}
				else if (ch == L'&' && ctx.owner->m_dfv_style->AssembleXmlEscapes() == TRUE)
				{
					xml_char = CheckAssembleXmlSeq(line_data+ik, line_len-ik, xml_seq_len);
					if (xml_char != 0)
					{
						break_reason = 5;			// This is an XML escape sequence.
						break;
					}
				}

				// Current character is an "ordinary char".
				ik++;
			}

			if (ik != ig)
			{
				// Characters srtarting from ig up to ik and not including it are "ordinary" characters. Process them.
				ctx.PrepareFrgrItemPlacing(this, ig, ik);
				res = AddSimplePaintingListItem(ctx, line_data+ig, ik-ig);
				if (res == FALSE || ctx.horz_overflow == TRUE)
				{
					// Preparation of the frgr painting list is aborted.
					goto Step3;
				}
			}

			// Adding ordinary characters succeeded or this was not needed.
			if (break_reason != 0)
			{
				// Character that needs special processing is present.
				assert(ik < ic);

				// Find the length of the spec seq. In typical case this will be just one character.
				int iq = ik+1;
				if (break_reason == 2)
				{
					// Figure out the length of the sequence of spaces.
					while (iq < ic)
					{
						if (line_data[iq] != L' ')
							break;
						iq++;
					}
				}
				else if (break_reason == 5)
				{
					// The length of the XML escape seq is already known.
					iq += xml_seq_len-1;
				}

				ctx.PrepareFrgrItemPlacing(this, ik, iq);
				switch (break_reason)
				{
					case 1:	// Special processing for tab character. Painting list item will be created even for invisible tab.
							res = AddTabOrSpacePaintingListItem(ctx, TRUE, ik, 1);
							break;

					case 2:	// Special processing for implicit tab positioning.
							res = AddTabOrSpacePaintingListItem(ctx, FALSE, ik, iq-ik);
							break;

					case 3:	// Special processing for visible space character.
							res = AddTabOrSpacePaintingListItem(ctx, FALSE, ik, 1);
							break;

					case 4:	// Special processing for non ASCII subst character.
							res = AddBitmapPaintingListItem(ctx, ik, icon_index);
							break;

					case 5:	// Painting the escaped XML character.
							break;
				}

				ik = iq;
				if (res == FALSE || ctx.horz_overflow == TRUE)
				{
					// Preparation of the frgr chain is aborted.
					goto Step3;
				}
			}

			// Simple chars and possible special char(s) that stay after the simple chars were processed.
			// Set the beg of the sequence to the first unprocessed char.
			ig = ik;
		}

		// Current bkgr/frgr/slct area is processed. Shift the outer loop.
		ib = ic;
	}

	//
	// Step 3. Update the bkgr rects of the frgr painting list items, background decors, inserted and imposed
	// decors according to the new state of the foreground painting list.
	//
	Step3:

	// Inline backgrounds.
	UpdateInlineBkgrDecors(ctx, TRUE);

	for (TListIter<TDFVFrgrDecoration> iter7(m_frgr_decors); iter7; ++iter7)
	{
		TDFVFrgrDecoration &decor = iter7.CurrItem();
		if (decor.GetBasicStyle() == NULL || decor.bkgr_item.GetBasicStyle() == NULL)
			continue;

		// Set left and right sides of the rect to the borders of the frgr painting list item.
		const TTextStyleProps &props = decor.Style()->m_props;
		RECT bkgr_rect;

		TDFVLineRenderCharPlaceInfo &info_beg = horz_info[decor.char_beg];
		TDFVLineRenderCharPlaceInfo *ptr_info_end = &(horz_info[decor.char_beg+decor.num_chars-1]);
		long left_pitem_offs = info_beg.m_painting_list_item_offs;
		long right_pitem_offs = ptr_info_end[1].m_painting_list_item_offs;
		assert(left_pitem_offs >= 0 && right_pitem_offs >= 0);

		bkgr_rect.left = ctx.text_x_pos+left_pitem_offs;
		bkgr_rect.right = ctx.text_x_pos+right_pitem_offs;

		if (&decor != info_beg.m_frgr_overlap_state || ctx.SameFrgrDecorBefore(decor) == FALSE)
		{
			// First char of the current decor is not the topmost or this is not the coninuation of the same decor.
			bkgr_rect.left += info_beg.FullFrontExts();
			if (props.front_side_ext > 0)
				bkgr_rect.left -= props.front_side_ext;
		}

		if (&decor != ptr_info_end->m_frgr_overlap_state || ctx.SameFrgrDecorAfter(decor) == FALSE)
		{
			// Last char of the current decor is not the topmost or there is no coninuation of the same decor.
			bkgr_rect.right -= ptr_info_end->FullBackExts();
			if (props.back_side_ext > 0)
				bkgr_rect.right += props.back_side_ext;
		}

		// The height of the background rect should be the height of the decoration font.
		bkgr_rect.top = m_baseline - decor.Style()->HeightAbove();
		bkgr_rect.bottom = m_baseline + decor.Style()->HeightBelow();

		// Setup the prepared rect.
		decor.bkgr_item.SetBaseRect(bkgr_rect);
	}

	for (int i8=0; i8<m_slct_decors.NumItems(); ++i8)
	{
		TDFVSlctDecoration &decor = m_slct_decors[i8];
		if (decor.slct_bkgr_item.GetBasicStyle() == NULL)
			continue;

		// Set left and right sides of the rect to the borders of the frgr area of the frgr painting list item.
		// This is a bit simplified approach. Nevertheless leave it so for now.
		RECT bkgr_rect;

		TDFVLineRenderCharPlaceInfo &info_beg = horz_info[decor.char_beg];
		TDFVLineRenderCharPlaceInfo *ptr_info_end = &(horz_info[decor.char_beg+decor.num_chars-1]);
		long left_pitem_offs = info_beg.m_painting_list_item_offs;
		long right_pitem_offs = ptr_info_end[1].m_painting_list_item_offs;
		assert(left_pitem_offs >= 0 && right_pitem_offs >= 0);

		bkgr_rect.left = ctx.text_x_pos+left_pitem_offs+info_beg.FullFrontExts();
		bkgr_rect.right = ctx.text_x_pos+right_pitem_offs-ptr_info_end->FullBackExts();

		// The height of the bkgr rect should be the max height across all fonts that are present on the line.
		bkgr_rect.top = m_baseline - m_text_above;
		bkgr_rect.bottom = m_baseline + m_text_below;

		// Setup the prepared rect.
		decor.slct_bkgr_item.SetBaseRect(bkgr_rect);
	}

	// Inserted and imposed decorations.
	long insert_char_inx = -1;
	long insert_pixel_pos;
	for (int i9=0; i9<m_imposed_decors.NumItems(); ++i9)
	{
		TDFVInlineImposedDecoration &decor = m_imposed_decors[i9];
		if (decor.insert_decor == FALSE)
		{
			// Position the independent object.
			PositionImposedDecor(ctx, decor);
		}
		else
		{
			if (decor.char_inx != insert_char_inx)
			{
				// This is one or more inserts in a different place of the line.
				insert_char_inx = decor.char_inx;
				insert_pixel_pos = horz_info[insert_char_inx].m_painting_list_item_offs;

				if (decor.insert_horz_order < 0 && decor.char_inx != 0 && decor.char_inx != line_len)
				{
					// Subtract the widths of all inserts at this charplace with negative horz order
					// from the pixel position of the border between the charplaces.
					for (int i10 = i9; i10<m_imposed_decors.NumItems(); ++i10)
					{
						TDFVInlineImposedDecoration &dcrx = m_imposed_decors[i10];
						if (dcrx.insert_decor == FALSE)
							continue;
						else if (dcrx.char_inx != insert_char_inx || decor.insert_horz_order >= 0)
							break;

						insert_pixel_pos -= decor.scr_item->GetWidth();
					}
				}
			}

			// Current insert is located at the same charplace or this info was set up above.
			decor.scr_item->SetBoundingRectPos(insert_pixel_pos, m_baseline-decor.scr_item->GetHeight()+decor.insert_vert_offs);
			ctx.owner->AddToInvalidRect(*(decor.scr_item->GetBoundingRect()));
			insert_pixel_pos += decor.scr_item->GetWidth();
		}
	}

	//
	// Step 4. Final actions.
	//

	// Report possible text area enlargement and the invalid area.
	if (ctx.curr_pix > ctx.owner->m_text_size.cx)
	{
		ctx.owner->m_text_size.cx = ctx.curr_pix;
	}

	ReportTextAreaInvalid(ctx.owner);
	return(res);
}

bool TDecoratedFileViewLineInfo::AddSimplePaintingListItem(TDFVPrepareScreenItemsContext &ctx, wchar_t *data, long data_len)
{
	assert(data_len > 0);

	// Prepare the text style that will be used for painting this object.
	const TTextStyle *frgr_style = (ctx.frgr_area != NULL) ? ctx.frgr_area->Style() : m_default_text.Style();
	if (ctx.slct_area != NULL)
	{
		// Create derived style with the same font but with a different frgr color.
		frgr_style = ctx.owner->m_dfv_style->LookupOrCreateTextStyle(ctx.hDC, frgr_style, ctx.slct_area->frgr_color, FALSE, TRUE);
		if (frgr_style == NULL)
			return(AppendOutOfMemFrgrObject(ctx));
	}

	// Check, if the horz overflow is happening or not.
	bool horz_overlow_happened = FALSE;
	if (ctx.x_limit > 0)
	{
		while (data_len > 0)
		{
			long req_length = ctx.ctx_min_extents+frgr_style->GetStringWidth(ctx.hDC, data, data_len, NULL, TRUE, TRUE);
			if (ctx.curr_pix+req_length <= ctx.x_limit)
				break;

			// The horz overflow is happening. Reduce the length of the string.
			data_len--;
			TDFVLineRenderCharPlaceInfo &lchi = ctx.owner->m_line_rendering_info[ctx.ctx_iend-1];
			TDFVLineRenderCharPlaceInfo &alchi = ctx.owner->m_line_rendering_info[ctx.ctx_iend];
			lchi.m_frgr_max_inner_back_ext = alchi.m_frgr_max_inner_back_ext;
			lchi.m_bkgr_max_inner_back_ext = alchi.m_bkgr_max_inner_back_ext;
			lchi.m_horz_spc_back_ext = alchi.m_horz_spc_back_ext;
			alchi.Clear();
			ctx.ctx_iend--;
		}
	}

	// Recheck the data length again. Horz overflow handling procedure above may have reduced the length.
	assert(horz_overlow_happened == FALSE || data_len > 0);
	if (data_len > 0)
	{
		// Retrieve and prepare the foreground object.
		wchar_t *line_data = m_default_text.GetBody();
		TDFVFrgrDecoration *frgr_obj = NULL;

		if (ctx.slct_area != NULL)
		{
			// Item will use combination of the slct bkgr rect and frgr object with modified text style where
			// the font will be taken from the frgr decor and the color will be taken from the selection props.
			frgr_obj = ctx.GetExtraFrgrObject();
			if (frgr_obj == NULL)
				return(AppendOutOfMemFrgrObject(ctx));

			frgr_obj->char_beg = (long)(data-line_data);
			frgr_obj->num_chars = data_len;
			frgr_obj->CopyOwnerInfoFrom(&(ctx.slct_area->slct_bkgr_item));
			frgr_obj->SetStyle((TTextStyle*)frgr_style);

			m_extra_frgr_objects.AppendItem(frgr_obj);
		}
		else if (ctx.frgr_area != NULL && data == line_data+ctx.frgr_area->char_beg && data_len == ctx.frgr_area->num_chars)
		{
			// Current part of the line has foreground decor and this decor is not splitted by anything. This means
			// that decoration object itself can be used in the painting list. Note that several props of this object like
			// covered chars, owner and style are already set.
			frgr_obj = ctx.frgr_area;
			assert(frgr_obj->paint_link.IsInList() == FALSE);
		}
		else
		{
			// New frgr object is needed.
			frgr_obj = ctx.GetExtraFrgrObject();
			if (frgr_obj == NULL)
				return(AppendOutOfMemFrgrObject(ctx));

			frgr_obj->char_beg = (long)(data-line_data);
			frgr_obj->num_chars = data_len;
			frgr_obj->CopyOwnerInfoFrom((ctx.frgr_area != NULL) ? ctx.frgr_area : &m_default_text);
			frgr_obj->SetStyle((TTextStyle*)frgr_style);

			m_extra_frgr_objects.AppendItem(frgr_obj);
		}

		// Setup contents of the text item and set its position.
		frgr_obj->SetContents(data, data_len);
		frgr_obj->SetFrameBlockingFlags(TRUE, TRUE);
		long x_beg_frgr = ctx.GetFrgrHorzBeg();
		long x_end_frgr = frgr_obj->PlaceObject(ctx.hDC, x_beg_frgr, m_baseline);

		// Finally add this object into the painting list.
		ctx.FinalizeFrgrItemPlacing(this, frgr_obj->paint_link, (x_end_frgr-x_beg_frgr));
	}

	if (ctx.horz_overflow == TRUE)
	{
		// Add the trailer object without checking how wide is it.
		return(AppendHorzOverflowObject(ctx));
	}

	// Full success.
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddTabOrSpacePaintingListItem(TDFVPrepareScreenItemsContext &ctx, bool tab_object, long char_inx, long object_width_in_chars)
{
	// Pick up the style that has the right font. Note that the frgr color in this style may be not correct.
	// At this point it is not important. The right color will be picked up later.
	const TTextStyle *frgr_style = ((ctx.frgr_area != NULL) ? ctx.frgr_area->Style() : m_default_text.Style());

	// Determine the width of the object in pixels.
	long frgr_width;
	TTabsSpacesViewMode &tbsp_mode = ctx.owner->m_dfv_style->m_tabs_mode;
	if (tab_object == TRUE)
	{
		// This is the tab symbol. If it should be visible or not, this will be decided later.
		assert(object_width_in_chars == 1);
		long tab_symbol_char_place_index = tbsp_mode.GetCharPlaceIndex(m_default_text.GetBody(), char_inx);
		frgr_width = tbsp_mode.GetExplicitTabWidth(tab_symbol_char_place_index, ctx.curr_pix+ctx.ctx_frgr_front_ext);
	}
	else
	{
		if (object_width_in_chars > 1)
		{
			// This is implicit tab. If it should be visible or not, this will be decided later.
			long char_place_index_after = tbsp_mode.GetCharPlaceIndex(m_default_text.GetBody(), char_inx+object_width_in_chars);
			frgr_width = tbsp_mode.GetImplicitTabWidth(char_place_index_after, ctx.curr_pix+ctx.ctx_frgr_front_ext);
		}
		else
		{
			// This situation should happen only when visible spaces mode is switched ON.
			assert(ctx.VisibleSpaces() == TRUE);
			assert(object_width_in_chars == 1);

			// Unfortunately it is necessary to reeveluate the width of the space many times because it
			// is different for different fonts. Caching this width in the style structure complicates the code.
			frgr_width = frgr_style->GetStringWidth(ctx.hDC, L" ", 1, NULL, TRUE, TRUE);
		}
	}

	assert(frgr_width > 0);

	// Check, if the horz overflow is happening or not.
	if (ctx.x_limit > 0 && ctx.curr_pix+ctx.ctx_min_extents+frgr_width > ctx.x_limit)
	{
		// The horz overflow is happening. Abandon the current tab or space object.
		return(AppendHorzOverflowObject(ctx));
	}

	// Retrieve the style for the line object. Later on the style pointer will tell if the object is visible or not.
	TLineStyle *lch_style = NULL;
	if (tab_object == TRUE && ctx.VisibleTabs() == TRUE || tab_object == FALSE && ctx.VisibleSpaces() == TRUE)
	{
		// This is either a visible space or a visible tab.
		THueSatLumColorHelper color_hlpr((ctx.slct_area != NULL) ? ctx.slct_area->frgr_color : frgr_style->GetFrgrColor());
		color_hlpr.AdjustLuminosity(0.65, 0.15);

		lch_style = ctx.owner->m_dfv_style->LookupOrCreateLineStyle(ctx.hDC, color_hlpr.GetRgb());
		if (lch_style == NULL)
			return(AppendOutOfMemFrgrObject(ctx));
	}

	// Allocate an object for this tab or space. Note that object is allocated even if it will be invisible.
	// This is necessary for correct processing of the mouse clicks.
	TDFVTabOrSpaceItem *visib_spc = ctx.GetVisibSpcObject();
	if (visib_spc == NULL)
		return(AppendOutOfMemFrgrObject(ctx));

	// Save pointer to this new object in the data members of the DFV line object.
	m_visib_spc_objects.AppendItem(visib_spc);

	// Fill in the index of the char that this object describes.
	visib_spc->char_inx = char_inx;
	visib_spc->num_characters = object_width_in_chars;

	// Prepare the node coordinates for the chain of lines. Prepare these coords even if the object will
	// be invisible. This simplifies the click detection procedure.
	long num_nodes = 0;
	POINT *pd = visib_spc->line_data;
	long l_pos = ctx.GetFrgrHorzBeg();

	if (tab_object == TRUE)
	{
		// Form an arrow, pointing to the right.
		long lev_y = m_baseline-3;
		long r_pos = l_pos+frgr_width-2;
		pd->x = l_pos+1;	pd->y = lev_y;	pd++;	// 0
		pd->x = r_pos;	pd->y = lev_y;	pd++;	// 1
		pd->x = r_pos;	pd->y = lev_y-2;	pd++;	// 2
		pd->x = r_pos;	pd->y = lev_y+2;	pd++;	// 3
		pd->x = r_pos;	pd->y = lev_y;	pd++;	// 4
		pd->x = r_pos-1;	pd->y = lev_y;	pd++;	// 5
		pd->x = r_pos-3;	pd->y = lev_y-2;	pd++;	// 6
		pd->x = r_pos-1;	pd->y = lev_y;	pd++;	// 7
		pd->x = r_pos-3;	pd->y = lev_y+2;	pd++;	// 8
		pd->x = r_pos-1;	pd->y = lev_y;	pd++;	// 9
		num_nodes = 10;
	}
	else if (object_width_in_chars <= 1)
	{
		// Form a small busket at the bottom of the char place.
		if (frgr_width <= 3)
		{
			long h_offs = (frgr_width <= 2) ? 0 : 1;
			pd[0].x = l_pos+h_offs;		pd[0].y = m_baseline-1;
			pd[1].x = l_pos+h_offs;		pd[1].y = m_baseline;
			num_nodes = 2;
		}
		else
		{
			pd[0].x = l_pos+1;				pd[0].y = m_baseline-1;
			pd[1].x = l_pos+1;				pd[1].y = m_baseline;
			pd[2].x = l_pos+frgr_width-2;	pd[2].y = m_baseline;
			pd[3].x = l_pos+frgr_width-2;	pd[3].y = m_baseline-2;
			num_nodes = 4;
		}
	}
	else
	{
		// Form an upside down busket at the bottom of the char place.
		if (frgr_width <= 3)
		{
			long h_offs = (frgr_width <= 2) ? 0 : 1;
			pd[0].x = l_pos+h_offs;		pd[0].y = m_baseline-1;
			pd[1].x = l_pos+h_offs;		pd[1].y = m_baseline;
			num_nodes = 2;
		}
		else
		{
			pd[0].x = l_pos+1;				pd[0].y = m_baseline;
			pd[1].x = l_pos+1;				pd[1].y = m_baseline-1;
			pd[2].x = l_pos+frgr_width-2;	pd[2].y = m_baseline-1;
			pd[3].x = l_pos+frgr_width-2;	pd[3].y = m_baseline+1;
			num_nodes = 4;
		}
	}

	// Setup the foreground object.
	visib_spc->CopyOwnerInfoFrom(ctx.GetOwnerItem(this));
	visib_spc->SetStyle(lch_style);
	visib_spc->SetPosition(num_nodes, visib_spc->line_data, dbm_use_only);

	// Finally add this object to the painting list.
	ctx.FinalizeFrgrItemPlacing(this, visib_spc->paint_link, frgr_width);

	// Full success.
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AddBitmapPaintingListItem(TDFVPrepareScreenItemsContext &ctx, long char_inx, short icon_index)
{
	TBitmapStyle *non_ascii_substs_style = ctx.owner->m_dfv_style->m_non_ascii_subst_picts;

	// Presence of the bitmap style is checked in the setup. Nevertheless recheck this once again here.
	assert(non_ascii_substs_style != NULL);

	// Pick up the props of the icon. The icon should have a non empty width.
	POINT icon_pos;
	SIZE icon_size;
	non_ascii_substs_style->GetLocalRect(icon_index, icon_pos, icon_size);
	assert(icon_size.cx > 0);

	// Check, if the horz overflow is happening or not.
	if (ctx.x_limit > 0 && ctx.curr_pix+ctx.ctx_min_extents+icon_size.cx > ctx.x_limit)
	{
		// The horz overflow is happening. Abandon the current object.
		return(AppendHorzOverflowObject(ctx));
	}

	// The horz space for the icon is available. Allocate an object for this non ASCII subst.
	TDFVNonAsciiSubstItem *non_ascii_subst = ctx.GetNonAsciiObject();
	if (non_ascii_subst == NULL)
		return(AppendOutOfMemFrgrObject(ctx));

	// Store the pointer and setup the app level props.
	m_non_ascii_objects.AppendItem(non_ascii_subst);
	non_ascii_subst->char_inx = char_inx;

	// Setup the foreground object. Horz location is simply based on the current pos and the width of the icon.
	// In the vert direction the hot spot of the icon, that is located in the middle of the cell, is placed on the baseline.
	non_ascii_subst->CopyOwnerInfoFrom(ctx.GetOwnerItem(this));
	non_ascii_subst->SetStyle(non_ascii_substs_style);
	non_ascii_subst->SetLeftAlignedPosition(ctx.GetFrgrHorzBeg(), m_baseline, icon_index);
	ctx.FinalizeFrgrItemPlacing(this, non_ascii_subst->paint_link, icon_size.cx);

	// Full success.
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AppendHorzOverflowObject(TDFVPrepareScreenItemsContext &ctx)
{
	assert(ctx.horz_overflow_msg_width > 0 && ctx.x_limit >= ctx.horz_overflow_msg_width);
	if (ctx.curr_pix+ctx.horz_overflow_msg_width > ctx.x_limit)
	{
		// There is no space to add the horz overflow marker. Take some of the frgr painting list items out.
		long curr_pix_limit = ctx.x_limit-ctx.horz_overflow_msg_width;
		while (m_frgr_painting_list.IsEmpty() == FALSE && ctx.curr_pix > curr_pix_limit)
		{
			TDFVFrgrPaintListItem *pitem = (TDFVFrgrPaintListItem*)m_frgr_painting_list.GetLast();
			m_frgr_painting_list.RemoveItem(pitem);
		}
	}

	// Start with preparation for selection.
	TTextStyle *frgr_style = ctx.owner->m_dfv_style->m_line_overflow_style;
	if (ctx.slct_area != NULL)
	{
		// Prepare text style with the horz overflow font and the selection color.
		frgr_style = ctx.owner->m_dfv_style->LookupOrCreateTextStyle(ctx.hDC, frgr_style, ctx.slct_area->frgr_color, FALSE, TRUE);
		if (frgr_style == NULL)
			return(AppendOutOfMemFrgrObject(ctx));
	}

	// Pick up the frgr object.
	TDFVFrgrDecoration *horz_ovfl = ctx.GetExtraFrgrObject();
	if (horz_ovfl == NULL)
		return(AppendOutOfMemFrgrObject(ctx));

	m_extra_frgr_objects.AppendItem(horz_ovfl);
	ctx.ClearExtentsInfo();

	// Set the props.
	horz_ovfl->SetOwner(NULL);
	horz_ovfl->SetStyle(frgr_style);
	horz_ovfl->SetContents(L"...", 3);
	horz_ovfl->PlaceObject(ctx.hDC, ctx.GetFrgrHorzBeg()+3, m_baseline);

	// Finally add this object to the painting list and return success.
	ctx.FinalizeFrgrItemPlacing(this, horz_ovfl->paint_link, -1);
	return(TRUE);
}

bool TDecoratedFileViewLineInfo::AppendOutOfMemFrgrObject(TDFVPrepareScreenItemsContext &ctx)
{
	// This object should not be created. Use the preallocated one.
	TDFVFrgrDecoration &oom_msg = ctx.owner->m_out_of_mem_message;
	if (ctx.owner->m_out_of_mem_msg_line != NULL)
	{
		// Out of memory message is used in some other line. Take it out from there.
		ctx.owner->m_out_of_mem_msg_line->m_frgr_painting_list.RemoveItem(oom_msg);
		ctx.owner->m_out_of_mem_msg_line = NULL;
	}

	ctx.ClearExtentsInfo();

	// Resetup all props in the oom marker if any.
	oom_msg.SetOwner(NULL);
	oom_msg.SetStyle(ctx.owner->m_dfv_style->m_line_overflow_style);
	oom_msg.SetContents(L"<!> out-of-mem <!>");
	oom_msg.PlaceObject(ctx.hDC, ctx.GetFrgrHorzBeg()+7, m_baseline);

	// Finally add this object to the painting list and return failure.
	ctx.FinalizeFrgrItemPlacing(this, oom_msg.paint_link, -1);
	return(FALSE);
}

void TDecoratedFileViewLineInfo::UpdateInlineBkgrDecors(TDFVAddRemoveDecorsContext &ctx, bool update_both_directions)
{
	// This method should be called only on "consistent" lines because it is using data from
	// the frgr painting list items. This means that they should be already in an "up to date" state.
	assert(m_invalid_scr_items == FALSE);

	RECT rc_decor;
	for (TListIter<TDFVRectDecoration> iter(m_bkgr_decors); iter; ++iter)
	{
		TDFVRectDecoration &decor = iter.CurrItem();

		if (update_both_directions == FALSE)
		{
			// Pick up the existing position. Only the horz position will be used in the code below.
			decor.GetBaseRect(rc_decor);
		}
		else
		{
			// Use the curr horz rendering info from the main object. It is ok to use this info because this method is called
			// with the second param equal to TRUE only when this info describes the current line. This is a sort of a trick.
			TDFVLineRenderCharPlaceInfo &info_beg = ctx.owner->m_line_rendering_info[decor.char_beg];
			TDFVLineRenderCharPlaceInfo *ptr_info_end = &(ctx.owner->m_line_rendering_info[decor.char_beg+decor.num_chars-1]);
			long left_char_border_offs = info_beg.m_painting_list_item_offs;
			long right_char_border_offs = ptr_info_end[1].m_painting_list_item_offs;
			assert(left_char_border_offs >= 0 && right_char_border_offs >= 0);

			// Set the left and right sides of the rect to the borders of the frgr painting list items.
			rc_decor.left = ctx.text_x_pos+left_char_border_offs;
			rc_decor.right = ctx.text_x_pos+right_char_border_offs;

			// Update the left and right sides of the rect only when the values of the inner margins are non negative.
			// When the inner margins are negative there is no need to update anything because the bkgr color should
			// cover the frgr painting list item right to its border.
			const TBasicStyleProps &props = decor.Style()->m_props;
			if (props.inner_margin_left >= 0)
				rc_decor.left += info_beg.m_horz_spc_front_ext+info_beg.m_bkgr_max_inner_front_ext-props.inner_margin_left;
			if (props.inner_margin_right >= 0)
				rc_decor.right -= ptr_info_end->m_horz_spc_back_ext+ptr_info_end->m_bkgr_max_inner_back_ext-props.inner_margin_right;
		}

		long max_font_above = 0, max_font_below = 0;
		if (decor.upper_ext >= 0 || decor.lower_ext >= 0)
		{
			// Collect information about the heights of the fonts that are used inside the decorated area.
			int curr_char = 0;
			bool looking_for_beg = TRUE;
			for (TListIter<TDFVFrgrPaintListItem> iter(m_frgr_painting_list); (iter == TRUE && iter.CurrItem().pix_len > 0); ++iter)
			{
				if (looking_for_beg == TRUE && curr_char >= decor.char_beg)
				{
					// Current item starts the requested area.
					looking_for_beg = FALSE;
				}

				TDFVFrgrPaintListItem &item = iter.CurrItem();
				if (looking_for_beg == FALSE && item.screen_item->GetItemTypeId() == TTextItem::type_ID)
				{
					// Current item is the text item that belongs to the area of the passed decor.
					const TTextStyle *frgr_item_style = ((TTextItem*)item.screen_item)->Style();

					if (frgr_item_style->HeightAbove() > max_font_above)
						max_font_above = frgr_item_style->HeightAbove();
					if (frgr_item_style->HeightBelow() > max_font_below)
						max_font_below = frgr_item_style->HeightBelow();
				}

				curr_char += item.GetNumCoveredChars();
				if (curr_char >= decor.char_beg+decor.num_chars)
				{
					// Current item ends the requested area.
					break;
				}
			}

			// Ensure that the loop above has found something.
			assert(looking_for_beg == FALSE);
		}

		if (decor.upper_ext >= 0)
			rc_decor.top = m_baseline-max_font_above-decor.upper_ext;
		else rc_decor.top = m_baseline-GetWlbkHeightAbove();

		if (decor.lower_ext >= 0)
			rc_decor.bottom = m_baseline+max_font_below+decor.lower_ext;
		else rc_decor.bottom = m_baseline+GetWlbkHeightBelow();

		// Setup the resulting rect.
		decor.SetBaseRect(rc_decor);
	}
}

void TDecoratedFileViewLineInfo::PositionImposedDecor(TDFVAddRemoveDecorsContext &ctx, TDFVInlineImposedDecoration &decor)
{
	assert(decor.insert_decor == FALSE);

	//  Update position of the imposed decor according to the current state of the line foreground.
	TScreenItem *scr_item = decor.scr_item;
	RECT rc_text_area;
	GetHorzLocationInfo(ctx, decor.char_inx, 0, rc_text_area);

	long pos_x = rc_text_area.left;
	scr_item->SetHotSpotPos(pos_x+decor.sym_border_horz_offs, m_baseline+decor.baseline_vert_offs);
	ctx.owner->AddToInvalidRect(*(scr_item->GetBoundingRect()));
}

void TDecoratedFileViewLineInfo::UpdateWholeLineBkgrDecors(TDFVAddRemoveDecorsContext &ctx)
{
	for (TListIter<TDFVRectDecoration> iter(m_whole_line_bkgrs); iter; ++iter)
		PositionWholeLineBkgrDecor(ctx, iter.CurrItem());

	if (m_indirect_hgt_bkgrs == TRUE && m_invalid_scr_items == FALSE)
	{
		// There are some backgound decors that require update.
		UpdateInlineBkgrDecors(ctx, FALSE);
	}
}

void TDecoratedFileViewLineInfo::PositionWholeLineBkgrDecor(TDFVAddRemoveDecorsContext &ctx, TDFVRectDecoration &decor)
{
	assert(decor.cover_mask != 0);

	// Find out the left side of the bypassed decor.
	RECT rc_left;
	if ((decor.cover_mask & wlbk_nums) != 0)
	{
		ctx.owner->m_nums_rect_scr_item.GetBaseRect(rc_left);
	}
	else if ((decor.cover_mask & wlbk_marks) != 0)
	{
		ctx.owner->m_marks_rect_scr_item.GetBaseRect(rc_left);
	}
	else if ((decor.cover_mask & wlbk_ltext) != 0)
	{
		ctx.owner->m_text_rect_scr_item.GetBaseRect(rc_left);
	}
	else
	{
		ctx.owner->m_text_rect_scr_item.GetBaseRect(rc_left);
		rc_left.left += ctx.owner->m_dfv_style->m_props.text_left_offs;

		if ((decor.cover_mask & wlbk_mtext) == 0)
		{
			if (m_invalid_scr_items == TRUE)
				return;

			// Left side starts from the end of the non empty text.
			rc_left.left += GetFrgrPaintingListWidth();
		}
	}

	// Find out the right side of the bypassed decor.
	RECT rc_right;
	if ((decor.cover_mask & wlbk_rtext) != 0)
	{
		ctx.owner->m_text_rect_scr_item.GetBaseRect(rc_right);
	}
	else if ((decor.cover_mask & (wlbk_ltext | wlbk_mtext)) != 0)
	{
		ctx.owner->m_text_rect_scr_item.GetBaseRect(rc_right);
		rc_right.right = rc_right.left+ctx.owner->m_dfv_style->m_props.text_left_offs;

		if ((decor.cover_mask & wlbk_mtext) != 0)
		{
			if (m_invalid_scr_items == TRUE)
				return;

			// Right side starts from the end of the non empty text.
			rc_right.right += GetFrgrPaintingListWidth();
		}
	}
	else if ((decor.cover_mask & wlbk_marks) != 0)
	{
		ctx.owner->m_marks_rect_scr_item.GetBaseRect(rc_right);
	}
	else
	{
		// Finish at the right side of the nums area.
		ctx.owner->m_nums_rect_scr_item.GetBaseRect(rc_right);
	}

	// Prepare the rect and set position to the decor.
	assert(rc_left.left <= rc_right.right);
	RECT rc = { rc_left.left, m_baseline-GetWlbkHeightAbove(), rc_right.right, m_baseline+GetWlbkHeightBelow() };
	decor.SetBaseRect(rc);
	ctx.owner->AddToInvalidRect(*(decor.GetBoundingRect()));
}

void TDecoratedFileViewLineInfo::GetHorzLocationInfo(TDFVAddRemoveDecorsContext &ctx, long char_beg, long num_chars, RECT &dest_rect)
{
	// Check the validity of params.
	assert(m_invalid_scr_items == FALSE);
	long char_end = char_beg+num_chars;
	if (char_beg < 0 || char_beg > m_default_text.num_chars || num_chars < 0 || char_end > m_default_text.num_chars)
	{
		// Passed range of chars sticks out of the data of the current line.
		assert(FALSE);
		dest_rect.left = dest_rect.right = 0;
		return;
	}

	// Loop over the elements of the frgr painting list.
	long curr_char = 0;
	long curr_pix = ctx.text_x_pos;
	bool looking_for_beg = TRUE;
	for (TListIter<TDFVFrgrPaintListItem> iter(m_frgr_painting_list); iter; ++iter)
	{
		TDFVFrgrPaintListItem &item = iter.CurrItem();
		if (item.pix_len == 0)
		{
			// This is a special line overflow item.
			assert(item.screen_item->GetItemTypeId() == TTextItem::type_ID);
			TTextItem *scr_item = (TTextItem*)item.screen_item;
			if (looking_for_beg == TRUE)
				dest_rect.left = scr_item->GetBoundingRect()->left;

			dest_rect.right = scr_item->GetBoundingRect()->right;
			return;
		}

		// Figure out how many characters the current frgr item covers.
		int nch = item.GetNumCoveredChars();

		if (looking_for_beg == TRUE && curr_char+nch >= char_beg)
		{
			// Beginning of the requested area belongs to the current item.
			assert(curr_char <= char_beg);
			if (curr_char == char_beg)
			{
				dest_rect.left = curr_pix;
			}
			else if (curr_char+nch > char_beg)
			{
				// The procedure is complex because the starting char is in the middle of the chars of the curr item.
				assert(nch > 1);
				if (item.screen_item->GetItemTypeId() == TTextItem::type_ID)
				{
					TTextItem *tx = (TTextItem*)item.screen_item;
					long substr_width = tx->Style()->GetStringWidth(ctx.GetControlDC(), tx->GetBody(), char_beg-curr_char, NULL, TRUE, TRUE);
					dest_rect.left = tx->GetBoundingRect()->left + substr_width;
				}
				else
				{
					// This should be a multispace element. Pick up the left side of the target rect as a proportinal
					// part of the covered area.
					assert(item.screen_item->GetItemTypeId() == TLinesChainItem::type_ID);
					dest_rect.left = curr_pix+(char_beg-curr_char*item.pix_len)/nch;
				}
			}
			else
			{
				// Current painting list item ends right in front of the requested char. Use the right border of the item
				// as the left border of the requested rect.
				dest_rect.left = curr_pix+item.pix_len;
			}

			looking_for_beg = FALSE;
		}

		if (looking_for_beg == FALSE && curr_char+nch >= char_end)
		{
			// Ending of the requested area belongs to the current item.
			assert(curr_char <= char_end);
			if (curr_char == char_end)
			{
				dest_rect.right = curr_pix;
			}
			else if (curr_char+nch > char_end)
			{
				// The procedure is complex because the beginning char is in the middle of the chars of the curr item.
				assert(nch > 1);
				if (item.screen_item->GetItemTypeId() == TTextItem::type_ID)
				{
					TTextItem *tx = (TTextItem*)item.screen_item;
					long substr_width = tx->Style()->GetStringWidth(ctx.GetControlDC(), tx->GetBody(), char_end-curr_char, NULL, TRUE, TRUE);
					dest_rect.right = tx->GetBoundingRect()->left + substr_width;
				}
				else
				{
					// This should be a multispace element. Pick up the right side as the proportinal part of the covered area.
					assert(item.screen_item->GetItemTypeId() == TLinesChainItem::type_ID);
					dest_rect.right = curr_pix+(char_beg-curr_char*item.pix_len)/nch;
				}
			}
			else
			{
				dest_rect.right = curr_pix+item.pix_len;
			}

			// The chars area was properly mapped to the frgr painting objects.
			return;
		}

		// Update the loop state variables.
		curr_char += nch;
		curr_pix += item.pix_len;
	}

	// For some reason the area was not mapped.
	if (looking_for_beg == TRUE)
		dest_rect.left = curr_pix;
	dest_rect.right = curr_pix+((looking_for_beg == TRUE) ? 5 : 0);
}

bool TDecoratedFileViewLineInfo::ProcessTextStyleVertImpact(TDFVAddRemoveDecorsContext &ctx, TTextStyle *style)
{
	bool impact_detected = FALSE;

	if (style != NULL)
	{
		if (style->HeightAbove() > m_text_above)
		{
			long baseline_shift = style->HeightAbove()-m_text_above;
			ShiftY(ctx.owner, baseline_shift);
			ctx.owner->ReportVertDelta(baseline_shift, ctx.line_num);
			m_text_above = (short)style->HeightAbove();
			impact_detected = TRUE;
		}

		if (style->HeightBelow() > m_text_below)
		{
			ctx.owner->ReportVertDelta(style->HeightBelow()-m_text_below, ctx.line_num);
			m_text_below = (short)style->HeightBelow();
			impact_detected = TRUE;
		}
	}

	return(impact_detected);
}

void TDecoratedFileViewLineInfo::SetLineNumScreenItemStyleAndPosition(TDFVAddRemoveDecorsContext &ctx, long nums_x_pos, TTextStyle *style)
{
	// Make an initial left aligned placement starting from the left side of the nums area.
	TDecoratedFileViewStyleProps &props = ctx.owner->m_dfv_style->m_props;
	m_line_num_scr_item.SetStyle(style);
	m_line_num_scr_item.PlaceObject(ctx.GetControlDC(), nums_x_pos, m_baseline);

	// Apply alignment and shift from the style properties.
	long width = m_line_num_scr_item.GetWidth();
	long shift = props.line_nums_offs;
	if (props.line_nums_alignment == align_right)
	{
		shift = props.line_nums_width - shift - width;
	}
	else if (props.line_nums_alignment == align_center)
	{
		shift += (props.line_nums_width - width)/2;
	}

	if (shift != 0)
		m_line_num_scr_item.ShiftItem(shift, 0);
}

void TDecoratedFileViewLineInfo::CalcVerticalTextExtents(TDecoratedFileViewScreenItem *dfv)
{
	// Setup some initial non empty values.
	m_text_above = 3;
	m_text_below = 1;

	// Process the foreground decorations list.
	bool no_frgr_decors = TRUE;
	bool non_full_cover_decors = FALSE;
	for (TListIter<TDFVFrgrDecoration> iter(m_frgr_decors); iter; ++iter)
	{
		TDFVFrgrDecoration &decor = iter.CurrItem();
		EvaluateTextStyleExtents((TTextStyle*)decor.Style());
		no_frgr_decors = FALSE;

		if (decor.char_beg > 0 || decor.num_chars != m_default_text.num_chars)
			non_full_cover_decors = TRUE;
	}

	// Style of the default text is evaluated only when there is at least one decoration that is not covering the whole line.
	if (no_frgr_decors == TRUE || non_full_cover_decors == TRUE)
		EvaluateTextStyleExtents(dfv->m_dfv_style->m_default_text_style);

	// Process the current style of the line number if any.
	if (m_line_num_scr_item.GetBasicStyle() != NULL)
		EvaluateTextStyleExtents((TTextStyle*)m_line_num_scr_item.Style());
}

bool TDecoratedFileViewLineInfo::ProcessTextExtentsVertImpact(TDFVAddRemoveDecorsContext &ctx)
{
	// This method is using simplified approach when all fonts on the line contribute to its height even if they belong
	// to completely obscured decorations.
	bool impact_detected = FALSE;
	long old_text_above = m_text_above;
	long old_text_below = m_text_below;
	CalcVerticalTextExtents(ctx.owner);

	if (old_text_above != m_text_above)
	{
		long baseline_shift = m_text_above-old_text_above;
		ShiftY(ctx.owner, baseline_shift);
		ctx.owner->ReportVertDelta(baseline_shift, ctx.line_num);
		impact_detected = TRUE;
	}

	if (old_text_below != m_text_below)
	{
		ctx.owner->ReportVertDelta(m_text_below-old_text_below, ctx.line_num);
		impact_detected = TRUE;
	}

	return(impact_detected);
}

void TDecoratedFileViewLineInfo::EvaluateTextStyleExtents(TTextStyle *stl)
{
	if (stl->HeightAbove() > m_text_above)
		m_text_above = (short)stl->HeightAbove();
	if (stl->HeightBelow() > m_text_below)
		m_text_below = (short)stl->HeightBelow();
}

void TDecoratedFileViewLineInfo::EvaluateImposedExtents(TDecoratedFileViewScreenItem *dfv, TScreenItem *imposed_item)
{
	RECT *bnd_rect = imposed_item->GetBoundingRect();

	if (bnd_rect->top < m_baseline-m_imposed_above)
		m_imposed_above = (short)(m_baseline-bnd_rect->top);
	if (bnd_rect->bottom > m_baseline+m_imposed_below)
		m_imposed_below = (short)(bnd_rect->bottom-m_baseline);
}

void TDecoratedFileViewLineInfo::ReportTextAreaInvalid(TDecoratedFileViewScreenItem *owner)
{
	RECT rc =
	{
		owner->m_text_rect_scr_item.GetBoundingRect()->left, m_baseline-m_text_above,
		owner->m_text_rect_scr_item.GetBoundingRect()->right, m_baseline+m_text_below
	};

	owner->AddToInvalidRect(rc);
}

void TDecoratedFileViewLineInfo::ReportFullAreaInvalid(TDecoratedFileViewScreenItem *owner)
{
	RECT rc =
	{
		owner->m_bounding_rect.left, m_baseline-GetFullHeightAbove(),
		owner->m_bounding_rect.right, m_baseline+GetFullHeightBelow()
	};

	owner->AddToInvalidRect(rc);
}

bool TDecoratedFileViewLineInfo::RemoveVertSpacingDecors(TExtraSpcDecorsArray &info, void *decor_owner, short zorder_code,
															short &new_ext, short &new_wlbk_ext)
{
	short max_ext = 0, max_wlbk_ext = 0;
	bool smth_removed = FALSE;

	// Iterate the whole list.
	for (int i=0; i<info.NumItems(); ++i)
	{
		if ((decor_owner == NULL || info[i].spc_owner == decor_owner) && (zorder_code == ANY_DECOR_ZORDER || info[i].spc_zorder == zorder_code))
		{
			info.ReleaseItem(i--);
			smth_removed = TRUE;
		}
		else
		{
			// Decor survived. Evaluate its spacing.
			if (info[i].spc_wlbk_value > max_ext)
				max_ext = info[i].spc_wlbk_value;
			if (info[i].spc_wlbk_use_wlbk_color == TRUE && info[i].spc_wlbk_value > max_wlbk_ext)
				max_wlbk_ext = info[i].spc_wlbk_value;
		}
	}

	// Give out the new biggest values found.
	new_ext = max_ext;
	new_wlbk_ext = max_wlbk_ext;
	return(smth_removed);
}

long TDecoratedFileViewLineInfo::GetFrgrPaintingListItemCharPos(TDecoratedFileViewScreenItem *dfv, TScreenItem *frgr_list_scr_item, long app_pt_x)
{
	switch (frgr_list_scr_item->GetItemTypeId())
	{
		case TTextItem::type_ID:
				{
					TDFVFrgrDecoration &scr_item = *((TDFVFrgrDecoration*)frgr_list_scr_item);
					assert(scr_item.m_text_length > 0);
					long left_side = scr_item.GetBoundingRect()->left;

					if (app_pt_x < left_side)
						return(scr_item.char_beg);

					HDC hDC = ::GetDC(dfv->m_view->GetControl());
					if (hDC == NULL)
					{
						// The situation is bogus. Return at least someting.
						return(scr_item.char_beg);
					}

					// Try all possible sublengths starting from one.
					for (int sublen=1; sublen<scr_item.m_text_length; ++sublen)
					{
						long width = scr_item.Style()->GetStringWidth(hDC, scr_item.GetBody(), sublen, NULL, TRUE, TRUE);
						if (app_pt_x < left_side+width)
						{
							::ReleaseDC(dfv->m_view->GetControl(), hDC);
							return(scr_item.char_beg+sublen-1);
						}
					}

					// clicked point stays after the last character. Return the index of the last character.
					::ReleaseDC(dfv->m_view->GetControl(), hDC);
					return(scr_item.char_beg+scr_item.m_text_length-1);
				}

		case TBitmapItem::type_ID:
				{
					// This is a bitmap that is used for non ASCII substs. Such bitmaps always represent one char.
					return(((TDFVNonAsciiSubstItem*)frgr_list_scr_item)->char_inx);
				}

		case TLinesChainItem::type_ID:
				{
					TDFVTabOrSpaceItem *tbsp_item = (TDFVTabOrSpaceItem*)frgr_list_scr_item;
					if (tbsp_item->num_characters <= 1)
						return(tbsp_item->char_inx);

					// The situation is more complex. This object repesents several chars on the line.
					RECT *rc_item = tbsp_item->GetBoundingRect();
					if (app_pt_x < rc_item->left || rc_item->left >= rc_item->right)
						return(tbsp_item->char_inx);
					else if (app_pt_x >= rc_item->right)
						return(tbsp_item->char_inx+tbsp_item->num_characters-1);

					// The click point is inside the rect of the frgr item. Use proprtion to detect the char.
					long ch_offs = ((app_pt_x-rc_item->left)*tbsp_item->num_characters)/(rc_item->right-rc_item->left);
					return(tbsp_item->char_inx+ch_offs);
				}
	}

	// The passed point does not belong to the screen item.
	assert(FALSE);
	return(0);
}

bool TDecoratedFileViewLineInfo::IsSimpleData(TDecoratedFileViewScreenItem *dfv)
{
	TTabsSpacesViewMode &mode = dfv->m_dfv_style->m_tabs_mode;

	if (m_tabs_or_substs == TRUE)
	{
		// Data on the line is complex because it contains tabs symbols, bitmap symbols or XML escapes.
		return(FALSE);
	}
	else if (mode.visible_tabs == TRUE && (m_space_chars == TRUE || m_space_seqs == TRUE))
	{
		// Line contains spaces that should be painted as visible spaces.
		return(FALSE);
	}
	else if (mode.implicit_multisp_tabs == TRUE && m_space_seqs == TRUE)
	{
		// Line contains space sequences that should be displayed as implicit tabs.
		return(FALSE);
	}

	// Data of the line can be painted as single line of text.
	return(TRUE);
}

wchar_t TDecoratedFileViewLineInfo::CheckAssembleXmlSeq(const wchar_t *pdata, long data_len, long &seq_len)
{
	assert(*pdata == L'&' && data_len > 0);

	if (data_len >= 4)
	{
		if (data_len >= 5 && wcsncmp(pdata, L"&amp;", 5) == 0)
		{
			seq_len = 5;
			return(L'&');
		}
		else if (wcsncmp(pdata, L"&lt;", 4) == 0)
		{
			seq_len = 4;
			return(L'<');
		}
		else if (wcsncmp(pdata, L"&gt;", 4) == 0)
		{
			seq_len = 4;
			return(L'>');
		}
		else if (data_len >= 6 && wcsncmp(pdata, L"&quot;", 6) == 0)
		{
			seq_len = 6;
			return(L'\"');
		}
	}

	// This is not an XML escape sequence.
	return(0);
}

// -------------------------------------------------------------------------------
//  =====================  TDFVSpecTabsArea  ========================
// -------------------------------------------------------------------------------

long TDFVSpecTabsArea::GetSpecTabOffset(TDFVPrepareScreenItemsContext &ctx, int ispec, long proposed_offs)
{
	assert(ispec >= 0 && ispec < MAX_SUBCOLUMNS-1);

	if (m_spec_tab_offsets[ispec] < 0)
	{
		// This spec tab column was not used before.
		m_spec_tab_offsets[ispec] = proposed_offs;
		return(proposed_offs);
	}
	else if (m_spec_tab_offsets[ispec] >= proposed_offs)
	{
		// There is no need to change the layout.
		return(m_spec_tab_offsets[ispec]);
	}
	else
	{
		// The layout should be changed. This invalidates all lines of the range above the current line.
		if (ctx.line_num > m_line_num_start)
		{
			// This call will setup the update of the lines but will not reset the tab stops layout.
			ctx.owner->RequestFrgrUpdateForLinesRange(m_line_num_start, ctx.line_num-m_line_num_start);
		}

		// Update the layout itself.
		long diff = proposed_offs-m_spec_tab_offsets[ispec];
		for (int isp=ispec; isp<MAX_SUBCOLUMNS-1; ++isp)
			m_spec_tab_offsets[isp] += diff;

		return(proposed_offs);
	}
}

// -------------------------------------------------------------------------------
//  ================  TDecoratedFileViewScreenItem  ======================
// -------------------------------------------------------------------------------

TDecoratedFileViewScreenItem::TDecoratedFileViewScreenItem()
{
	SetOwner(this);					// This owner will be never given out. It should be not NULL to force screen items
									// core iteration procedure to include this object into processing.

	// Setup non NULL style to the m_style field of the base class to make the object paintable.
	m_default_bkgr_style.Setup(cs_transparent);
	m_style = &m_default_bkgr_style;

	// Clear only those vars that store the resorce ownership.
	m_file_data_owned = FALSE;
	m_lines_info_owned = FALSE;
	m_display_lines = NULL;
	m_line_rendering_info = NULL;

	// This method will clear the rest of the data fields.
	Reset();
}

void TDecoratedFileViewScreenItem::Reset()
{
	// Reset view object and styles.
	m_view = NULL;
	m_dfv_style = NULL;
	m_max_text_size.cx = m_max_text_size.cy = 0;
	m_min_text_size.cx = m_min_text_size.cy = 0;

	// Delete the file data if any.
	if (m_file_data_owned == TRUE)
	{
		assert(m_file_data != NULL);
		free(m_file_data);
		m_file_data_owned = FALSE;
	}

	// Reset the file data fields.
	m_file_data = NULL;
	m_file_data_len = 0;
	m_file_data_src_offs = 0;
	m_file_data_first_line = 0;

	// Delete the lines info if any.
	if (m_lines_info_owned == TRUE)
	{
		assert(m_lines_info != NULL);
		free(m_lines_info);
		m_lines_info_owned = FALSE;
	}

	// Reset the line info fields.
	m_lines_info = NULL;
	m_lines_info_len = 0;

	// Delete the derived info.
	if (m_display_lines != NULL)
	{
		delete[] m_display_lines;
		m_display_lines = NULL;
	}

	// Reset info about the displayes lines.
	m_display_lines_len = 0;
	m_display_lines_first_line = 0;

	// Clear the intersection fields.
	m_first_isect_line = 0;
	m_num_isect_lines = 0;

	// Clear current text size related info.
	m_text_size.cx = m_text_size.cy = 0;
	m_num_visible_lines = 0;
	m_visible_lines_height = 0;

	// Base rects should be invisible for now.
	m_nums_rect_scr_item.SetStyle(NULL);
	m_marks_rect_scr_item.SetStyle(NULL);
	m_text_rect_scr_item.SetStyle(NULL);

	for (int iarea=0; iarea<3; ++iarea)
	{
		m_top_area_wlbk_decors[iarea].CheckCancelStaticDecor(NULL, ANY_DECOR_ZORDER);
		m_bottom_area_wlbk_decors[iarea].CheckCancelStaticDecor(NULL, ANY_DECOR_ZORDER);
	}

	// Clear invalid areas related fields.
	m_frgr_update_wanted = FALSE;
	m_min_invalid_frgr_line = NULL;
	m_max_invalid_frgr_line = NULL;
	m_vert_delta_pix_value = 0;
	m_vert_delta_line_num = 0;
	m_allow_unupdated_painting = FALSE;
	m_smth_invalid = FALSE;

	// Clear the OOM support fields.
	m_out_of_mem_message.paint_link.Clear();
	m_out_of_mem_msg_line = NULL;

	// Release painting list building support.
	if (m_line_rendering_info != NULL)
	{
		free(m_line_rendering_info);
		m_line_rendering_info = NULL;
	}
}

void TDecoratedFileViewScreenItem::SetHotSpotPos(long pos_x, long pos_y)
{
	assert(IsInited() == TRUE);

	// The whole rect will be invalidated later. Ensure that object is in the updated state.
	assert(m_frgr_update_wanted == FALSE);
	assert(m_vert_delta_pix_value == 0);

	// Move lines of text one by one.
	TDecoratedFileViewStyleProps &props = m_dfv_style->m_props;
	long lev_y = pos_y+m_dfv_style->m_props.text_top_offs;
	long base_text_x = pos_x+props.line_nums_width+props.marks_area_width+props.text_left_offs;

	TDecoratedFileViewLineInfo *li1 = m_display_lines;
	for (int i1=0; i1<m_display_lines_len; ++i1, ++li1)
	{
		lev_y += li1->GetFullHeightAbove();
		li1->SetPosition(this, base_text_x, lev_y);
		lev_y += li1->GetFullHeightBelow();
	}

	// Shift nums rect and top bottom decors.
	long base_rect_x = pos_x;
	m_nums_rect_scr_item.SetHotSpotPos(base_rect_x, pos_y);
	m_top_area_wlbk_decors[0].SetHotSpotPos(base_rect_x, pos_y);
	m_bottom_area_wlbk_decors[0].SetHotSpotPos(base_rect_x, lev_y);

	// Shift marks rect and top bottom decors.
	base_rect_x += props.line_nums_width;
	m_marks_rect_scr_item.SetHotSpotPos(base_rect_x, pos_y);
	m_top_area_wlbk_decors[1].SetHotSpotPos(base_rect_x, pos_y);
	m_bottom_area_wlbk_decors[1].SetHotSpotPos(base_rect_x, lev_y);

	// Shift text rect and top bottom decors.
	base_rect_x += props.marks_area_width;
	m_text_rect_scr_item.SetHotSpotPos(base_rect_x, pos_y);
	m_top_area_wlbk_decors[2].SetHotSpotPos(base_rect_x, pos_y);
	m_bottom_area_wlbk_decors[2].SetHotSpotPos(base_rect_x, lev_y);

	// Invalidate the whole object and modify its rect.
	m_smth_invalid = FALSE;
	AddToInvalidRect(*m_nums_rect_scr_item.GetBoundingRect());
	AddToInvalidRect(*m_marks_rect_scr_item.GetBoundingRect());
	AddToInvalidRect(*m_text_rect_scr_item.GetBoundingRect());
	m_view->InvalidateAppSpaceRect(m_rc_invalid);
	m_smth_invalid = FALSE;
	m_bounding_rect = m_rc_invalid;
}

void TDecoratedFileViewScreenItem::OnDraw(HDC hDC, RECT &rc_invalid)
{

	// External methods of this class should be called only when object is in the "correct" state.
	assert(IsInited() == TRUE);

	if (m_frgr_update_wanted == TRUE || m_vert_delta_pix_value != 0)
	{
		// The object is not updated. Check the debug safeguard flag that allows painting without explicitly
		// updating the control. This is allowed sometimes when the message boxes pop up during the process
		// of applying decorations.
		assert(m_allow_unupdated_painting == TRUE);
		UpdateControl();
	}

	// Paint the background rects.
	if (m_nums_rect_scr_item.GetBasicStyle() != NULL)
		m_nums_rect_scr_item.OnDraw(hDC, rc_invalid);
	if (m_marks_rect_scr_item.GetBasicStyle() != NULL)
		m_marks_rect_scr_item.OnDraw(hDC, rc_invalid);
	if (m_text_rect_scr_item.GetBasicStyle() != NULL)
		m_text_rect_scr_item.OnDraw(hDC, rc_invalid);

	// Paint the rect decorations that stay above the lines of the file after painting the main bkgr.
	for (int ita=0; ita<3; ++ita)
	{
		if (m_top_area_wlbk_decors[ita].GetBasicStyle() != NULL)
			m_top_area_wlbk_decors[ita].OnDraw(hDC, rc_invalid);
	}

	// Paint the lines of the file.
	if (m_num_visible_lines >= 0)
	{
		// Paint only those lines, that intersect with the passed invalid rect. This dramatically speeds up painting big files.
		long lev_y = m_display_lines->m_baseline-m_display_lines->GetFullHeightAbove();
		long ibeg = -1, iend = m_num_visible_lines;
		TDecoratedFileViewLineInfo *li1 = m_display_lines;
		for (int i1=0; i1<m_num_visible_lines; ++i1, ++li1)
		{
			if (lev_y >= rc_invalid.bottom)
			{
				// Top of the current line stays below the bottom of the invalid area.
				// Variable iend marks the first line that stays outside of the paining area.
				iend = i1;
				break;
			}

			lev_y += li1->GetFullHeight();
			if (lev_y > rc_invalid.top)
			{
				// Bottom of the current line stays below the top of the invalid area.
				// This means that the line has intersection with the invalid area.
				if (ibeg < 0)
					ibeg = i1;

				// Do partial painting of the line right now.
				li1->PaintBkgrFrgrObjects(hDC, rc_invalid);
			}
		}

		if (ibeg >= 0)
		{
			// The loop above discovered lines that should be painted and even partially painted them.
			// Iterate same range of lines again and do the second part of the paining process.
			for (int i2=ibeg; i2<iend; ++i2)
				m_display_lines[i2].PaintImposedObjects(hDC, rc_invalid);
		}
	}

	// Paint rect decorations that stay below the lines of the file.
	for (int iba=0; iba<3; ++iba)
	{
		if (m_bottom_area_wlbk_decors[iba].GetBasicStyle() != NULL)
			m_bottom_area_wlbk_decors[iba].OnDraw(hDC, rc_invalid);
	}
}

TScreenItem *TDecoratedFileViewScreenItem::CheckClick(POINT &app_pt)
{
	assert(IsInited() == TRUE);

	// Pick up the line that stays under the passed point and after that let the line object figure out
	// whether there is anything at this place or not.
	TDecoratedFileViewLineInfo *li = GetDFVLineInfoFromVertPos(app_pt.y);
	if (li != NULL)
		return(li->CheckClickOrSummarize(this, app_pt, NULL));

	// The clicked position does not belong to any line. Check if it belongs to the top or bottom areas or not.
	for (int iarea=0; iarea<3; ++iarea)
	{
		if (m_top_area_wlbk_decors[iarea].GetOwner() != NULL)
		{
			TScreenItem *lc_item1 = m_top_area_wlbk_decors[iarea].CheckClick(app_pt);
			if (lc_item1 != NULL)
				return(lc_item1);
		}

		if (m_bottom_area_wlbk_decors[iarea].GetOwner() != NULL)
		{
			TScreenItem *lc_item2 = m_bottom_area_wlbk_decors[iarea].CheckClick(app_pt);
			if (lc_item2 != NULL)
				return(lc_item2);
		}
	}

	// No success.
	return(NULL);
}

void TDecoratedFileViewScreenItem::SummarizeScreenItems(TScreenItemSummarizeInfo &info, bool bounding_rect_mode)
{
	assert(IsInited() == TRUE);

	// This feature is not supported for the zoomed out views.
	if (bounding_rect_mode == TRUE)
		return;

	// Items from only one line can be present in the summary. There is no need to check other lines.
	TDecoratedFileViewLineInfo *li = GetDFVLineInfoFromVertPos(info.app_pt.y);
	if (li != NULL)
		li->CheckClickOrSummarize(this, info.app_pt, &info);

	// Check, if the clicked position belongs to the top or bottom areas or not.
	for (int iarea=0; iarea<3; ++iarea)
	{
		if (m_top_area_wlbk_decors[iarea].GetOwner() != NULL)
			m_top_area_wlbk_decors[iarea].SummarizeScreenItems(info, bounding_rect_mode);

		if (m_bottom_area_wlbk_decors[iarea].GetOwner() != NULL)
			m_bottom_area_wlbk_decors[iarea].SummarizeScreenItems(info, bounding_rect_mode);
	}
}

bool TDecoratedFileViewScreenItem::Setup(TScreenItemsViewObject *view_object, TDecoratedFileViewStyle *style_object,
										const wchar_t *fdata, long fdata_len, TDataBypassMode fd_bp_mode,
										long fdata_src_offs, long fdata_first_line,
										TSimpleLineInfo *linfo, long linfo_len, TDataBypassMode li_bp_mode,
										long first_visib_line, long num_visib_lines,
										long position_x, long position_y)
{
	assert(view_object != NULL);
	assert(style_object != NULL);

	// Bring all data members to the state of an empty file.
	Reset();

	// Verify the file data and its length.
	if (fdata != NULL)
	{
		// The file data is not missing. Although its length can be zero.
		if (fdata_len < 0)
			fdata_len = (long)wcslen(fdata);
	}
	else
	{
		// This is the case of a missing file.
		fdata_len = 0;

		assert(linfo == NULL && linfo_len == 0);
		linfo = NULL;
		linfo_len = 0;
	}

	// Check the lines info.
	bool local_fdata = FALSE;
	bool local_linfo = FALSE;
	long longest_line_len = 0;
	if (linfo != NULL)
	{
		assert(fdata != NULL);
		assert(linfo_len > 0);

		// Caller has provided the lines info. Check that this info conforms to the file data. All areas that are
		// described as the line contents should steadily increase and they should be inside the file data.
		long prev_lim = fdata_src_offs;
		for (long ii=0; ii<linfo_len; ++ii)
		{
			assert(linfo[ii].line_offs >= prev_lim);
			assert(linfo[ii].line_offs+linfo[ii].line_len <= fdata_len);

			prev_lim = linfo[ii].line_offs+linfo[ii].line_len;
			if (linfo[ii].line_len > longest_line_len)
				longest_line_len = linfo[ii].line_len;
		}
	}
	else
	{
		assert(linfo_len == 0);
		if (fdata != NULL)
		{
			// Non empty file was passed without lines info.
			linfo = TSimpleLineInfo::BuildLinesInfoFromData(fdata, fdata_len, fdata_src_offs, linfo_len, longest_line_len);
			if (linfo == NULL)
				return(FALSE);

			assert(linfo_len > 0);
			li_bp_mode = dbm_take_ownsh;
			local_linfo = TRUE;
		}
	}

	if (fdata != NULL)
	{
		//
		//  The passed file is not missing. Allocate various data structures.
		//

		// Duplicate the file data if needed.
		if (fd_bp_mode == dbm_duplicate)
		{
			if (fdata_len > 0)
			{
				wchar_t *nfd = (wchar_t*)malloc(fdata_len*sizeof(wchar_t));
				if (nfd == NULL)
				{
					if (local_linfo == TRUE)
						free(linfo);
					return(FALSE);
				}

				memcpy(nfd, fdata, fdata_len*sizeof(wchar_t));
				fdata = nfd;
			}
			else
			{
				// The file is empty. Use a const pointer to an empty line as the file data.
				fdata = L"";
			}

			fd_bp_mode = dbm_take_ownsh;
			local_fdata = TRUE;
		}

		// Duplicate the lines info if needed.
		if (li_bp_mode == dbm_duplicate)
		{
			TSimpleLineInfo *nli = (TSimpleLineInfo*)malloc(linfo_len*sizeof(TSimpleLineInfo));
			if (nli == NULL)
			{
				if (local_fdata == TRUE)
					free((wchar_t*)fdata);
				return(FALSE);
			}

			memcpy(nli, linfo, linfo_len*sizeof(TSimpleLineInfo));
			linfo = nli;
			li_bp_mode = dbm_take_ownsh;
			local_linfo = TRUE;
		}

		// Verify the range of lines to display.
		if (num_visib_lines <= 0)
		{
			// The number of lines to display is not passed directly. Find out an appropriate number of lines.
			if (first_visib_line <= fdata_first_line+linfo_len-1)
			{
				// At least the last line of the file data will be visible. Maybe there will be line overflow lines
				// before the lines with the file data.
				num_visib_lines = fdata_first_line+linfo_len-first_visib_line;
			}
			else
			{
				// The intersection is empty. Show one dummy line.
				num_visib_lines = 1;
			}
		}

		assert(num_visib_lines > 0);

		// Allocate an array of the line descriptors. Plus allocate additional buffers that will be used inside the TDecoratedFileViewLineInfo class.
		// Add +1 to the lengths of these arrays just to avoid having allocations with the zero length for the files that contain only empty lines.
		TDecoratedFileViewLineInfo *lines_array = new TDecoratedFileViewLineInfo[num_visib_lines];
		TDFVLineRenderCharPlaceInfo *horz_rendering_info = (TDFVLineRenderCharPlaceInfo*)malloc((longest_line_len+1)*sizeof(TDFVLineRenderCharPlaceInfo));

		if (lines_array == NULL || horz_rendering_info == NULL)
		{
			// At least one allocation failed. Release everything.
			if (lines_array != NULL)
				delete[] lines_array;
			if (horz_rendering_info != NULL)
				delete[] horz_rendering_info;
			if (local_fdata == TRUE)
				free((wchar_t*)fdata);
			if (local_linfo == TRUE)
				free(linfo);

			// The whole setup failed.
			return(FALSE);
		}

		//
		//  (1a) All allocations succeeded. Accept the data. Note, that only those fields are filled that have meaning
		//  for non empty files. Rest of the fields will be filled in later.
		//

		// File data.
		m_file_data = (wchar_t*)fdata;
		m_file_data_len = fdata_len;
		m_file_data_owned = (fd_bp_mode == dbm_take_ownsh) ? TRUE : FALSE;

		m_file_data_src_offs = fdata_src_offs;
		m_file_data_first_line = fdata_first_line;

		// Lines info.
		m_lines_info = linfo;
		m_lines_info_len = linfo_len;
		m_lines_info_owned = (li_bp_mode == dbm_take_ownsh) ? TRUE : FALSE;

		// The range of lines to display.
		m_display_lines = lines_array;
		m_display_lines_len = num_visib_lines;
		m_display_lines_first_line = first_visib_line;

		// Determine the meaningful intersection between the file data and the range of lines to display.
		long max_beg = __max(fdata_first_line, first_visib_line);
		long min_end = __min(fdata_first_line+linfo_len, first_visib_line+num_visib_lines);
		if (max_beg < min_end)
		{
			// The intersection is non empty.
			m_first_isect_line = max_beg;
			m_num_isect_lines = min_end-max_beg;
		}
		else
		{
			// There is no chance to show anything useful.
			m_first_isect_line = -10000;
			m_num_isect_lines = 0;
		}

		// (1b) Stpre the decoration overlap state buffer.
		m_line_rendering_info = horz_rendering_info;
	}

	// (2a) Assign those fields that are needed both for empty and non empty files.
	m_view = view_object;
	m_dfv_style = style_object;
	long def_font_heigth;
	SetupMaxTextAreaSize(def_font_heigth);
	TDecoratedFileViewStyleProps &props = style_object->m_props;
	SetupMinTextAreaSize(props.bounding_rect_min.cx, props.bounding_rect_min.cy);

	// (2b) Clear the pending request fields.
	m_frgr_update_wanted = FALSE;
	m_vert_delta_pix_value = 0;

	// Set up the initial state of displayable lines. This will also form the height of the object.
	long text_area_width = 0;
	long text_area_height = 0;
	if (fdata != NULL)
	{
		//
		//  The file is not missing.
		//

		// (3a) Setup the line objects.
		long lines_top = position_y+props.text_top_offs;
		TDFVDecoratedLineSetupContext ctx(this);
		ctx.line_num = first_visib_line;
		ctx.text_x_pos = position_x+props.line_nums_width+props.marks_area_width+props.text_left_offs;
		ctx.line_top = lines_top;
		ctx.nums_x_pos = position_x;

		for (int i1=0; i1<m_display_lines_len; ++i1)
		{
			if (ctx.line_num >= m_file_data_first_line && ctx.line_num < m_file_data_first_line+m_lines_info_len)
			{
				// There is some data in the file for this line number.
				TSimpleLineInfo &li = m_lines_info[ctx.line_num-m_file_data_first_line];
				m_display_lines[i1].Setup(ctx, m_file_data+li.line_offs, li.line_len);
			}
			else
			{
				// Display the line substitute.
				m_display_lines[i1].Setup(ctx, NULL, 0);
			}

			ctx.line_num++;

			if (m_max_text_size.cy > 0)
			{
				// Check if the latest line should be added to the set of the visible lines or not.
				if (ctx.line_top-lines_top <= m_max_text_size.cy)
				{
					m_num_visible_lines++;
					m_visible_lines_height = ctx.line_top-lines_top;
				}
			}
		}

		// Ensure that preparation of the lines went smooth.
		assert(m_frgr_update_wanted == FALSE && m_vert_delta_pix_value == 0);

		// (3b) Setup current text area size and visible lines info. Width of the text area was reported from lines setup.
		m_text_size.cy = ctx.line_top-lines_top;
		if (m_max_text_size.cy <= 0)
		{
			m_num_visible_lines = m_display_lines_len;
			m_visible_lines_height = m_text_size.cy;
		}

		// Prepare info about the text area.
		text_area_width = (m_text_size.cx > 0) ? m_text_size.cx : 4;
		text_area_height = m_visible_lines_height;
	}
	else
	{
		// Passed file is missing. Reserve some space for one dummy line.
		text_area_width = 10;
		text_area_height = def_font_heigth;
	}

	// Prepare the bottom mark of bkgr base rects.
	long bottom_mark = position_y+props.text_top_offs;
	bottom_mark += __max(text_area_height, m_min_text_size.cy);
	bottom_mark += props.text_bottom_offs;

	//
	// Prepare the background rect screen items if needed. The text rect is prepared all the times
	// regardless of the style props.
	//

	// Calls from inside the line setup could generate wrong invalid rect. Reset current invalid area
	// and regenerate it again later.
	m_smth_invalid = FALSE;

	long pos_x = position_x;
	RECT rc_nums = { pos_x, position_y, pos_x+props.line_nums_width, bottom_mark };
	m_nums_rect_scr_item.SetStyle(NULL);
	m_nums_rect_scr_item.SetBaseRect(rc_nums);
	if (props.line_nums_width > 0)
	{
		// Props say that object has non empty line nums area.
		m_nums_rect_scr_item.SetStyle((TScreenItemStyle*)m_dfv_style->LineNumsBkgr());
		AddToInvalidRect(*m_nums_rect_scr_item.GetBoundingRect());
		pos_x += props.line_nums_width;
	}

	RECT rc_top_nums = rc_nums;
	rc_top_nums.bottom = position_y+props.text_top_offs;
	m_top_area_wlbk_decors[0].SetStyle(NULL);
	m_top_area_wlbk_decors[0].SetBaseRect(rc_top_nums);

	RECT rc_bottom_nums = rc_nums;
	rc_bottom_nums.top = bottom_mark-props.text_bottom_offs;
	m_bottom_area_wlbk_decors[0].SetStyle(NULL);
	m_bottom_area_wlbk_decors[0].SetBaseRect(rc_bottom_nums);

	RECT rc_marks = { pos_x, position_y, pos_x+props.marks_area_width, bottom_mark };
	m_marks_rect_scr_item.SetStyle(NULL);
	m_marks_rect_scr_item.SetBaseRect(rc_marks);
	if (props.marks_area_width > 0)
	{
		// Props require to reserve space for the marks area.
		m_marks_rect_scr_item.SetStyle((TScreenItemStyle*)m_dfv_style->MarksAreaBkgr());
		AddToInvalidRect(*m_marks_rect_scr_item.GetBoundingRect());
		pos_x += props.marks_area_width;
	}

	RECT rc_top_marks = rc_marks;
	rc_top_marks.bottom = position_y+props.text_top_offs;
	m_top_area_wlbk_decors[1].SetStyle(NULL);
	m_top_area_wlbk_decors[1].SetBaseRect(rc_top_marks);

	RECT rc_bottom_marks = rc_marks;
	rc_bottom_marks.top = bottom_mark-props.text_bottom_offs;
	m_bottom_area_wlbk_decors[1].SetStyle(NULL);
	m_bottom_area_wlbk_decors[1].SetBaseRect(rc_bottom_marks);

	long text_rect_width = props.text_left_offs+__max(text_area_width, m_min_text_size.cx)+props.text_right_offs;
	RECT rc_text = { pos_x, position_y, pos_x+text_rect_width, bottom_mark };
	m_text_rect_scr_item.SetStyle(&(m_dfv_style->m_text_area_bkgr_style));
	m_text_rect_scr_item.SetBaseRect(rc_text);
	AddToInvalidRect(*m_text_rect_scr_item.GetBoundingRect());

	RECT rc_top_text = rc_text;
	rc_top_text.bottom = position_y+props.text_top_offs;
	m_top_area_wlbk_decors[2].SetStyle(NULL);
	m_top_area_wlbk_decors[2].SetBaseRect(rc_top_text);

	RECT rc_bottom_text = rc_text;
	rc_bottom_text.top = bottom_mark-props.text_bottom_offs;
	m_bottom_area_wlbk_decors[2].SetStyle(NULL);
	m_bottom_area_wlbk_decors[2].SetBaseRect(rc_bottom_text);

	// Bounding rect of the whole object is the current invalid rect.
	m_bounding_rect = m_rc_invalid;
	m_old_bounding_rect = m_rc_invalid;
	m_view->InvalidateAppSpaceRect(m_rc_invalid);
	m_smth_invalid = FALSE;

	// Full success.
	return(TRUE);
}

bool TDecoratedFileViewScreenItem::SetupTabsSpacesViewMode(bool visib_tabs, bool visib_spaces)
{
	assert(IsInited() == TRUE);

	// Accept the new props.
	m_dfv_style->SetTabsSpacesVisibMode(visib_tabs, visib_spaces);

	// Invalidate frgr painting lists in all text lines.
	TDecoratedFileViewLineInfo *dfv_line = m_display_lines;
	for (int inx=0; inx<m_display_lines_len; ++inx, ++dfv_line)
	{
		dfv_line->m_invalid_scr_items = TRUE;
		ReportFrgrUpdateWanted(dfv_line);
	}

	// Rebuild the foreground everywhere.
	return(UpdateControl());
}

bool TDecoratedFileViewScreenItem::SetupMinBoundingRectSize(long new_min_size_x, long new_min_size_y)
{
	assert(IsInited() == TRUE);

	// Update control if it has some pending work.
	bool res = UpdateControl();

	// Convert bounding rect size into text area size.
	SetupMinTextAreaSize(new_min_size_x, new_min_size_y);

	// Update bkgr rects and flush the invalid area.
	UpdateObjectWidth();
	UpdateObjectHeight();
	if (m_smth_invalid == TRUE)
	{
		ReportInvalidRect();
		m_view->UpdateControl();
	}

	// Overall result was figured out earlier.
	return(res);
}

// - - - - - - - - - Decoration manipulations - - - - - - - - - - -

bool TDecoratedFileViewScreenItem::AddBkgrDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
													long area_beg, long area_len,
													TBasicStyle *style, short upper_ext, short lower_ext, bool *smth_applied)
{
	assert(IsInited() == TRUE);

	bool full_success = TRUE;
	if (smth_applied != NULL)
		*smth_applied = FALSE;

	if (area_len <= 0)
		return(TRUE);

	// Iterate lines that intersect with the passed file area.
	TDFVAddRemoveDecorsContext ctx(this);
	TFileSpaceIterationInfo iter_info(area_beg, area_len, m_lines_info, m_lines_info_len);
	while (TDecoratedFileViewLineInfo *li = IterateVisibleFileSpace(iter_info))
	{
		if (iter_info.num_chars == 0)
		{
			// Inline bkgr decor is not applicable to the line borders or empty lines.
			continue;
		}

		if (smth_applied != NULL)
			*smth_applied = TRUE;

		ctx.line_num = iter_info.line_inx+m_file_data_first_line;
		full_success = li->AddBkgrDecor(ctx, dcr_owner, owner_data, zorder_code,
									iter_info.char_beg, iter_info.num_chars,
									style, upper_ext, lower_ext);
		if (full_success == FALSE)
			break;
	}

	// Even if result is success some work may be still pending.
	return(full_success == TRUE);
}

bool TDecoratedFileViewScreenItem::AddFrgrDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
													long area_beg, long area_len,
													TTextStyle *style, bool *smth_applied)
{
	assert(IsInited() == TRUE);

	bool full_success = TRUE;
	if (smth_applied != NULL)
		*smth_applied = FALSE;

	if (area_len <= 0)
		return(TRUE);

	TDFVAddRemoveDecorsContext ctx(this);
	TBasicStyle *bkgr_style = NULL;
	TTextStyle *frgr_style = style;

	// Split the style if needed outside of the loop.
	TTextStyleProps &stpr = style->m_props;
	if (stpr.background_color != cs_transparent || stpr.frame_color != cs_transparent && stpr.frame_corner_style != fcs_none)
	{
		// Passed style should be split into the background style and modified foreground style.
		TColor bkgr_frame_color = (stpr.frame_color != cs_transparent && stpr.frame_corner_style != fcs_none) ? stpr.frame_color : cs_transparent;
		bkgr_style = m_dfv_style->LookupOrCreateBkgrStyle(ctx.GetControlDC(), stpr.background_color, bkgr_frame_color);
		if (bkgr_style == NULL)
			return(FALSE);
		frgr_style = m_dfv_style->LookupOrCreateTextStyle(ctx.GetControlDC(), style, stpr.foreground_color, TRUE, FALSE);
		if (frgr_style == NULL)
			return(FALSE);
	}

	// Iterate lines that intersect with the passed file area.
	TFileSpaceIterationInfo iter_info(area_beg, area_len, m_lines_info, m_lines_info_len);
	while (TDecoratedFileViewLineInfo *li = IterateVisibleFileSpace(iter_info))
	{
		if (iter_info.num_chars == 0)
		{
			// Frgr decor is not applicable to the line borders or empty lines.
			continue;
		}

		if (smth_applied != NULL)
			*smth_applied = TRUE;

		ctx.line_num = iter_info.line_inx+m_file_data_first_line;
		full_success = li->AddFrgrDecor(ctx, dcr_owner, owner_data, zorder_code,
									iter_info.char_beg, iter_info.num_chars,
									frgr_style, bkgr_style);
		if (full_success == FALSE)
			break;
	}

	// Even if result is success some work may be still pending.
	return(full_success == TRUE);
}

bool TDecoratedFileViewScreenItem::AddSelectionDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
														long area_beg, long area_len,
														TColor bkgr_color, TColor frgr_color, short beg_end_line_ext, bool *smth_applied)
{
	assert(IsInited() == TRUE);
	if (frgr_color == cs_transparent)
	{
		assert(FALSE);
		return(FALSE);
	}

	bool full_success = TRUE;
	if (smth_applied != NULL)
		*smth_applied = FALSE;

	if (area_len <= 0)
		return(TRUE);

	// Pick up the bkgr style for this selection.
	TDFVAddRemoveDecorsContext ctx(this);
	TBasicStyle *bkgr_style = NULL;
	if (bkgr_color != cs_transparent)
	{
		// The color is not transparent. Non NULL style is needed.
		bkgr_style = m_dfv_style->LookupOrCreateBkgrStyle(ctx.GetControlDC(), bkgr_color);
		if (bkgr_style == NULL)
			return(FALSE);
	}

	// Iterate lines that intersect with the passed file area.
	TFileSpaceIterationInfo iter_info(area_beg, area_len, m_lines_info, m_lines_info_len);
	while (TDecoratedFileViewLineInfo *li = IterateVisibleFileSpace(iter_info))
	{
		CheckAndApplyVertDelta(iter_info.line_inx+m_file_data_first_line);
		if (iter_info.num_chars == 0 && beg_end_line_ext <= 0)
		{
			// Slct decor is not applicable to the line borders or empty lines when there is no exts.
			continue;
		}

		if (smth_applied != NULL)
			*smth_applied = TRUE;

		ctx.line_num = iter_info.line_inx+m_file_data_first_line;
		full_success = li->AddSlctDecor(ctx, dcr_owner, owner_data, zorder_code,
									iter_info.char_beg, iter_info.num_chars,
									bkgr_style, frgr_color, beg_end_line_ext);
		if (full_success == FALSE)
		{
			// There is no need to recycle the bkgr_style. All derived styles are owned by the file view style.
			break;
		}
	}

	// Even if result is success some work may be still pending.
	return(full_success == TRUE);
}

bool TDecoratedFileViewScreenItem::AddInsertedDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
														long file_offset, TScreenItem *scr_item, short decor_horz_order, long baseline_offs, bool *smth_applied)
{
	// Bottom of the passed screen item is placed on the baseline.
	assert(IsInited() == TRUE);
	assert(scr_item != NULL);

	if (smth_applied != NULL)
		*smth_applied = FALSE;

	// Inserted decoration can be added somewhere only once.
	TFileSpaceIterationInfo iter_info(file_offset, 0, m_lines_info, m_lines_info_len);
	TDecoratedFileViewLineInfo *li = IterateVisibleFileSpace(iter_info);
	if (li == NULL)
		return(FALSE);

	if (smth_applied != NULL)
		*smth_applied = TRUE;

	// Fix the vertical position of this line.
	TDFVAddRemoveDecorsContext ctx(this);
	ctx.line_num = iter_info.line_inx+m_file_data_first_line;
	CheckAndApplyVertDelta(ctx.line_num);
	bool res = li->AddInsertedDecor(ctx, dcr_owner, owner_data, zorder_code, iter_info.char_beg, scr_item, decor_horz_order, baseline_offs);
	return(res == TRUE);
}


bool TDecoratedFileViewScreenItem::AddImposedDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
														long file_offset, TScreenItem *scr_item, long horz_offs, long baseline_offs, bool *smth_applied)
{
	assert(IsInited() == TRUE);
	assert(scr_item != NULL);

	if (smth_applied != NULL)
		*smth_applied = FALSE;

	// Imposed decoration can be added somewhere only once.
	TFileSpaceIterationInfo iter_info(file_offset, 0, m_lines_info, m_lines_info_len);
	TDecoratedFileViewLineInfo *li = IterateVisibleFileSpace(iter_info);
	if (li == NULL)
		return(FALSE);

	if (smth_applied != NULL)
		*smth_applied = TRUE;

	// Fix the vertical position of this line.
	TDFVAddRemoveDecorsContext ctx(this);
	ctx.line_num = iter_info.line_inx+m_file_data_first_line;
	CheckAndApplyVertDelta(ctx.line_num);
	bool res = li->AddImposedDecor(ctx, dcr_owner, owner_data, zorder_code, iter_info.char_beg, scr_item, horz_offs, baseline_offs);
	return(res == TRUE);
}

bool TDecoratedFileViewScreenItem::AddHorzSpacingDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
															long file_offset, long spc_val_left, long spc_val_right, bool *smth_applied)
{
	assert(IsInited() == TRUE);
	assert(spc_val_left > 0 || spc_val_right > 0);

	if (smth_applied != NULL)
		*smth_applied = FALSE;

	// The horz spacing decoration cannot affect more than one line.
	TFileSpaceIterationInfo iter_info(file_offset, 0, m_lines_info, m_lines_info_len);
	TDecoratedFileViewLineInfo *li = IterateVisibleFileSpace(iter_info);
	if (li == NULL)
		return(FALSE);

	if (smth_applied != NULL)
		*smth_applied = TRUE;

	// Prepare context and update the corresponding line info object.
	TDFVAddRemoveDecorsContext ctx(this);
	ctx.line_num = iter_info.line_inx+m_file_data_first_line;
	bool res = li->AddHorzSpcDecor(ctx, dcr_owner, owner_data, zorder_code, iter_info.char_beg, spc_val_left, spc_val_right);
	return(res == TRUE);
}

bool TDecoratedFileViewScreenItem::AddSpecTabDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
															long file_offset, short decor_horz_order, bool *smth_applied)
{
	assert(IsInited() == TRUE);

	if (smth_applied != NULL)
		*smth_applied = FALSE;

	// The spec tab decoration cannot affect more than one line.
	TFileSpaceIterationInfo iter_info(file_offset, 0, m_lines_info, m_lines_info_len);
	TDecoratedFileViewLineInfo *li = IterateVisibleFileSpace(iter_info);
	if (li == NULL)
		return(FALSE);

	if (smth_applied != NULL)
		*smth_applied = TRUE;

	// Prepare context and update the corresponding line info object.
	TDFVAddRemoveDecorsContext ctx(this);
	ctx.line_num = iter_info.line_inx+m_file_data_first_line;
	bool res = li->AddSpecTabDecor(ctx, dcr_owner, owner_data, zorder_code, iter_info.char_beg, decor_horz_order);
	return(res == TRUE);
}

bool TDecoratedFileViewScreenItem::AddWholeLineBkgrDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
															long line_beg, long num_lines,
															TBasicStyle *style, WORD cover_mask, bool *smth_applied)
{
	assert(IsInited() == TRUE);

	bool full_success = TRUE;
	if (smth_applied != NULL)
		*smth_applied = FALSE;

	if (cover_mask == 0)
		return(TRUE);

	// Cover mask is the set of TDecoratedFileWlbkAreaFlags enum members.
	WORD mask_fragments[3];
	WORD cnt_fragms = SplitWlbkCoverMask(cover_mask, mask_fragments);
	assert(cnt_fragms > 0);

	// Iterate the lines that are affected.
	TDFVAddRemoveDecorsContext ctx(this);
	while (TDecoratedFileViewLineInfo *li = IterateVisibleLinesSpace(line_beg, num_lines))
	{
		// At this point the line_beg always contains the next line number.
		CheckAndApplyVertDelta(line_beg-1);
		ctx.line_num = line_beg-1;

		if (smth_applied != NULL)
			*smth_applied = TRUE;

		for (int ifr=0; ifr<cnt_fragms; ++ifr)
		{
			full_success = li->AddWholeLineBkgrDecor(ctx, dcr_owner, owner_data, zorder_code, style, mask_fragments[ifr]);
			if (full_success == FALSE)
				break;
		}
	}

	// Even if the result is success some work may be still pending.
	return(full_success == TRUE);
}

bool TDecoratedFileViewScreenItem::AddLineNumFrgrDecoration(void *dcr_owner, DWORD owner_data, short zorder_code,
															long line_beg, long num_lines,
															TTextStyle *style, bool *smth_applied)
{
	assert(IsInited() == TRUE);

	bool full_success = TRUE;
	if (smth_applied != NULL)
		*smth_applied = FALSE;

	// Iterate the lines that might be substantially affected.
	TDFVAddRemoveDecorsContext ctx(this);
	while (TDecoratedFileViewLineInfo *li = IterateVisibleLinesSpace(line_beg, num_lines))
	{
		// At this point line_beg always contains the next line number.
		CheckAndApplyVertDelta(line_beg-1);
		ctx.line_num = line_beg-1;

		if (smth_applied != NULL)
			*smth_applied = TRUE;

		full_success = li->AddLineNumFrgrDecor(ctx, dcr_owner, owner_data, zorder_code, style);
		if (full_success == FALSE)
			break;
	}

	// Even if the result is success some work may be still pending.
	return(full_success == TRUE);
}

bool TDecoratedFileViewScreenItem::AddSpecTabsAreaDecoration(void *decoration_owner, DWORD owner_data, short zorder_code, long line_beg, long num_lines,
														bool *smth_applied)
{
	bool full_success = TRUE;
	if (smth_applied != NULL)
		*smth_applied = FALSE;
	return(full_success == TRUE);
}

bool TDecoratedFileViewScreenItem::AddWholeLineImposedDecoration(void *dcr_owner, DWORD owner_data, short zorder_code, long line_number,
																TScreenItem *scr_item, long horz_offs, long baseline_offs, bool *smth_applied)
{
	assert(IsInited() == TRUE);
	assert(scr_item != NULL);

	if (smth_applied != NULL)
		*smth_applied = FALSE;

	TDecoratedFileViewLineInfo *li = GetDFVLineInfo(line_number);
	if (li == NULL)
		return(FALSE);

	if (smth_applied != NULL)
		*smth_applied = TRUE;

	// Line with the passed number is existing.
	TDFVAddRemoveDecorsContext ctx(this);
	ctx.line_num = line_number;
	CheckAndApplyVertDelta(line_number);
	bool res = li->AddWholeLineImposedDecor(ctx, dcr_owner, owner_data, zorder_code,
										scr_item, horz_offs, baseline_offs);
	return(res == TRUE);
}

bool TDecoratedFileViewScreenItem::AddWholeLineSpacingDecoration(void *dcr_owner, DWORD owner_data, short zorder_code, long line_number,
																bool above_spacing, short new_spc_val, bool accept_wlbk_color, bool *smth_applied)
{
	assert(IsInited() == TRUE);

	if (smth_applied != NULL)
		*smth_applied = FALSE;

	TDecoratedFileViewLineInfo *li = GetDFVLineInfo(line_number);
	if (li == NULL)
		return(FALSE);

	if (smth_applied != NULL)
		*smth_applied = TRUE;

	// Line with the passed number is existing.
	TDFVAddRemoveDecorsContext ctx(this);
	ctx.line_num = line_number;
	CheckAndApplyVertDelta(line_number);
	bool res = li->AddWholeLineSpacingDecor(ctx, dcr_owner, owner_data, zorder_code,
										above_spacing, new_spc_val, accept_wlbk_color);
	return(res == TRUE);
}

bool TDecoratedFileViewScreenItem::AddTopBottomAreaBkgrDecoration(void *dcr_owner, DWORD owner_data, short zorder_code, bool cover_top_area, bool cover_bottom_area,
																TBasicStyle *style, WORD cover_mask, bool *smth_applied)
{
	assert(IsInited() == TRUE);

	bool decor_applied = FALSE;
	if (smth_applied != NULL)
		*smth_applied = FALSE;

	if (cover_top_area == TRUE && m_dfv_style->m_props.text_top_offs > 0)
	{
		if ((cover_mask & wlbk_nums) != 0 && m_dfv_style->m_props.line_nums_width > 0)
			m_top_area_wlbk_decors[0].SetupSpecial(dcr_owner, owner_data, zorder_code, style);
		if ((cover_mask & wlbk_marks) != 0 && m_dfv_style->m_props.marks_area_width > 0)
			m_top_area_wlbk_decors[1].SetupSpecial(dcr_owner, owner_data, zorder_code, style);
		if ((cover_mask & wlbk_text) != 0)
			m_top_area_wlbk_decors[2].SetupSpecial(dcr_owner, owner_data, zorder_code, style);
	}

	if (cover_bottom_area == TRUE && m_dfv_style->m_props.text_bottom_offs > 0)
	{
		if ((cover_mask & wlbk_nums) != 0 && m_dfv_style->m_props.line_nums_width > 0)
			m_bottom_area_wlbk_decors[0].SetupSpecial(dcr_owner, owner_data, zorder_code, style);
		if ((cover_mask & wlbk_marks) != 0 && m_dfv_style->m_props.marks_area_width > 0)
			m_bottom_area_wlbk_decors[1].SetupSpecial(dcr_owner, owner_data, zorder_code, style);
		if ((cover_mask & wlbk_text) != 0)
			m_bottom_area_wlbk_decors[2].SetupSpecial(dcr_owner, owner_data, zorder_code, style);
	}

	if (smth_applied != NULL)
		*smth_applied = TRUE;

	return(TRUE);
}

void TDecoratedFileViewScreenItem::RemoveDecoration(void *dcr_owner, short zorder_code)
{
	assert(IsInited() == TRUE);

	if (m_num_isect_lines == 0)
	{
		// This file cannot have decorations.
		return;
	}

	// Get rid of the pending vert shift if any.
	ApplyAndDismissVertDelta();

	// Iterate all visible screen lines of the file. Decorations with passed owner can be present on any line.
	long inx_beg = m_first_isect_line-m_display_lines_first_line;
	TDecoratedFileViewLineInfo *li = m_display_lines+inx_beg;

	long line_num = m_first_isect_line;
	for (int inx=inx_beg; inx<inx_beg+m_num_isect_lines; ++inx, ++li, ++line_num)
	{
		if (m_vert_delta_pix_value != 0)
			li->ShiftY(this, m_vert_delta_pix_value);

		m_vert_delta_line_num = line_num;
		li->RemoveDecor(this, dcr_owner, zorder_code);
	}

	for (int iarea=0; iarea<3; ++iarea)
	{
		m_top_area_wlbk_decors[iarea].CheckCancelStaticDecor(dcr_owner, zorder_code);
		m_bottom_area_wlbk_decors[iarea].CheckCancelStaticDecor(dcr_owner, zorder_code);
	}
}

bool TDecoratedFileViewScreenItem::UpdateControl()
{
	assert(IsInited() == TRUE);

	// First step. Get rid of the vertical shift if any.
	if (m_vert_delta_pix_value != 0)
	{
		ApplyAndDismissVertDelta();
	}

	// Second step. Prepare all unprepared foreground painting lists.
	bool res = TRUE;
	if (m_frgr_update_wanted == TRUE)
	{
		res = PrepareForegroundScreenItems();
		UpdateObjectWidth();
	}

	// Third step. Check if there are unreported invalid areas or not.
	if (m_smth_invalid == TRUE)
	{
		ReportInvalidRect();
		m_view->UpdateControl();
	}

	// Return the overall result.
	return(res);
}

// - - - - - - - - -  Get information group  - - - - - - - - - - -

bool TDecoratedFileViewScreenItem::GetSelectionPosition(TColor bkgr_color, TColor frgr_color, long &sel_beg, long &sel_len)
{
	assert(IsInited() == TRUE);

	assert(FALSE);  // TODO
	return(FALSE);
}

bool TDecoratedFileViewScreenItem::GetLineNumberFromFileOffs(long file_offs, long &line_num, long &char_pos)
{
	assert(IsInited() == TRUE);

	TSimpleLineInfo *linfo = TSimpleLineInfo::FindLineInfo(m_lines_info, m_lines_info_len, file_offs);
	if (linfo == NULL)
		return(FALSE);

	// Char offset cannot be bigger than the line length because otherwise linfo would be NULL.
	line_num = m_file_data_first_line+(linfo-m_lines_info);
	char_pos = file_offs-linfo->line_offs+1;
	return(TRUE);
}

long TDecoratedFileViewScreenItem::GetLineNumberFromPoint(POINT &app_pt)
{
	assert(IsInited() == TRUE);

	// Pick up the line object that corresponds to the passed place.
	TDecoratedFileViewLineInfo *li = GetDFVLineInfoFromVertPos(app_pt.y);
	if (li == NULL)
		return(-1);

	// Return the line number of this visible line.
	return((long)(li-m_display_lines+m_display_lines_first_line));
}

bool TDecoratedFileViewScreenItem::GetFileAreaFromPoint(POINT &app_pt, long &area_beg, long &area_len)
{
	assert(IsInited() == TRUE);

	// Clear the output vars.
	area_beg = area_len = 0;

	// Find out what is the line object under this place.
	TDecoratedFileViewLineInfo *li = GetDFVLineInfoFromVertPos(app_pt.y);
	if (li == NULL || li->m_default_text.GetBody() == g_DFV_OutOfRangeLine)
		return(FALSE);

	// Some line that belongs tot he file body is found. Pick up its offset in the file.
	long offs = (long)(li->m_default_text.GetBody()-m_file_data);
	assert(offs >= 0 && offs <= m_file_data_len);

	// Check the horizontal direction.
	long text_x = m_text_rect_scr_item.GetBaseLeft()+m_dfv_style->m_props.text_left_offs;
	if (app_pt.x < text_x)
	{
		// Return the beginning of the line.
		area_beg = offs;
		return(TRUE);
	}

	// Iterate all foreground screen items to find out what item is good for the passed horz position.
	for (TListIter<TDFVFrgrPaintListItem> iter(li->m_frgr_painting_list); iter; ++iter)
	{
		TDFVFrgrPaintListItem &item = iter.CurrItem();
		text_x += item.pix_len;
		if (app_pt.x < text_x)
		{
			// Passed point is on the territory of the current frgr painting list item. Most likely this frgr item
			// covers several char places. Figure out the exact charplace that belongs to the clicked point.
			area_beg = offs+li->GetFrgrPaintingListItemCharPos(this, item.screen_item, app_pt.x);
			area_len = 1;
			return(TRUE);
		}
	}

	// None of the screen items matched. Return the end of the line.
	area_beg = offs+li->m_default_text.num_chars;
	return(TRUE);
}

TDecoratedFileWlbkAreaFlags TDecoratedFileViewScreenItem::GetHorzAreaTypeFromPoint(POINT &app_pt)
{
	assert(IsInited() == TRUE);

	// Check common areas that are not related to any line.
	if (m_dfv_style->m_props.line_nums_width > 0)
	{
		RECT rc;
		m_nums_rect_scr_item.GetBaseRect(rc);
		if (app_pt.x >= rc.left && app_pt.x < rc.right)
			return(wlbk_nums);
	}

	if (m_dfv_style->m_props.marks_area_width > 0)
	{
		RECT rc;
		m_marks_rect_scr_item.GetBaseRect(rc);
		if (app_pt.x >= rc.left && app_pt.x < rc.right)
			return(wlbk_marks);
	}

	// Check relation to the big text rect.
	RECT rc;
	m_text_rect_scr_item.GetBaseRect(rc);
	if (app_pt.x < rc.left || app_pt.x >= rc.right)
		return(wlbk_none);

	// Find out relation to the text on the specific line.
	TDecoratedFileViewLineInfo *li = GetDFVLineInfoFromVertPos(app_pt.y);
	if (li == NULL)
		return(wlbk_none);

	long text_left = rc.left+m_dfv_style->m_props.text_left_offs;
	if (app_pt.x < text_left)
		return(wlbk_ltext);

	// Give result based on the length of all frgr objects on this line.
	return((app_pt.x < text_left+li->GetFrgrPaintingListWidth()) ? wlbk_mtext : wlbk_rtext);
}

bool TDecoratedFileViewScreenItem::GetLineHeightInfo(long line_number, long &baseline, long &hi_ext, long &lo_ext, TLineHeightType line_measure_type)
{
	assert(IsInited() == TRUE);

	if (m_display_lines_len == 0 || line_number < m_display_lines_first_line || line_number >= m_display_lines_first_line+m_display_lines_len)
	{
		// Bogus line number.
		return(FALSE);
	}

	// Pick up information about the requested  line.
	TDecoratedFileViewLineInfo &linfo = m_display_lines[line_number-m_display_lines_first_line];
	assert(linfo.m_invalid_scr_items == FALSE);

	baseline = linfo.m_baseline;
	hi_ext = linfo.m_text_above;
	lo_ext = linfo.m_text_below;

	// Check the inline bkgrs impact.
	if (line_measure_type >= dfvlh_bkgr_exts)
	{
		hi_ext += linfo.m_ext_bkgr_above;
		lo_ext += linfo.m_ext_bkgr_below;
	}

	// Check the whole line bkgr and vert spacing decors.
	if (line_measure_type == dfvlh_wlbk_colored_exts)
	{
		hi_ext += linfo.m_upper_wlbk_ext;
		lo_ext += linfo.m_lower_wlbk_ext;
	}
	else if (line_measure_type >= dfvlh_full_exts)
	{
		hi_ext += linfo.m_upper_extent;
		lo_ext += linfo.m_lower_extent;
	}

	// Compare line height with the imposed decors info.
	if (line_measure_type == dfvlh_complete_exts)
	{
		if (linfo.m_imposed_above > hi_ext)
			hi_ext = linfo.m_imposed_above;
		if (linfo.m_imposed_below > lo_ext)
			lo_ext = linfo.m_imposed_below;
	}

	// Success.
	return(TRUE);
}

long TDecoratedFileViewScreenItem::GetHorzScrollForFirstLineOfFileArea(long area_beg, long area_len, long curr_scroll, long window_width,
																		long left_pixels, long right_pixels)
{
	assert(IsInited() == TRUE);

	// Find out the first line of the area.
	TSimpleLineInfo *li = TSimpleLineInfo::FindLineInfo(m_lines_info, m_lines_info_len, area_beg);
	if (li == NULL)
		return(curr_scroll);

	TDecoratedFileViewLineInfo *dfv_line = GetDFVLineInfo((long)(li-m_lines_info)+m_file_data_first_line);
	if (dfv_line == NULL)
		return(curr_scroll);

	// Reduce the size of the passed area to the part that belongs to its first line.
	area_len = __min(li->LineEnd()-area_beg, area_len);

	// Convert character offsets into the screen pixels.
	TDFVAddRemoveDecorsContext ctx(this);
	RECT area_rect;
	dfv_line->GetHorzLocationInfo(ctx, area_beg-li->line_offs, area_len, area_rect);

	// Call the worker function.
	long file_left = m_nums_rect_scr_item.GetBoundingRect()->left;
	long file_width = m_text_rect_scr_item.GetBoundingRect()->right-file_left;
	return(GetScrollPos(area_rect.left, area_rect.right, curr_scroll, window_width, left_pixels, right_pixels, file_left, file_width, FALSE, TRUE));
}

long TDecoratedFileViewScreenItem::GetScrollPosForLinesRange(long line_beg, long line_end, long curr_scroll, long window_height,
																long pixels_above, long pixels_below, bool inititial_frame_placement)
{
	assert(IsInited() == TRUE);

	// Find out information about the lines. GetLineHeightInfo will also take care of the case when the file is empty. (It will return FALSE).
	long base_beg, hi_ext_beg, lo_ext_beg;
	if (GetLineHeightInfo(line_beg, base_beg, hi_ext_beg, lo_ext_beg, dfvlh_wlbk_colored_exts) == FALSE)
		return(curr_scroll);

	long base_end, hi_ext_end, lo_ext_end;
	if (GetLineHeightInfo(line_end, base_end, hi_ext_end, lo_ext_end, dfvlh_wlbk_colored_exts) == FALSE)
		return(curr_scroll);

	long req_beg = base_beg-hi_ext_beg;
	long req_end = base_end+lo_ext_end;
	assert(req_beg < req_end);

	long file_top = m_text_rect_scr_item.GetBoundingRect()->top;
	long file_height = m_text_rect_scr_item.GetBoundingRect()->bottom-file_top;
	return(GetScrollPos(req_beg, req_end, curr_scroll, window_height, pixels_above, pixels_below, file_top, file_height, inititial_frame_placement, FALSE));
}

bool TDecoratedFileViewScreenItem::IntersectWithVisibleFileArea(long &area_beg, long &area_len, bool allow_zero_length_area)
{
	if (m_num_isect_lines <= 0 || area_len < 0 || (allow_zero_length_area == FALSE && area_len == 0))
		return(FALSE);

	long visible_beg = m_lines_info[m_first_isect_line-m_file_data_first_line].LineBeg();
	long visible_end = m_lines_info[m_first_isect_line+m_num_isect_lines-1-m_file_data_first_line].LineEnd();

	if (area_beg < visible_beg)
	{
		// Passed area starts before the current visible area.
		area_len -= (visible_beg-area_beg);
		if (area_len < 0 || (allow_zero_length_area == FALSE && area_len == 0))
			return(FALSE);

		area_beg = visible_beg;
	}

	if (area_beg > visible_end || (allow_zero_length_area == FALSE && area_beg == visible_end))
	{
		// Passed area starts after the current visible area.
		return(FALSE);
	}

	if (area_beg+area_len > visible_end)
	{
		area_len = visible_end-area_beg;
	}

	// The intersection is not empty or the caller allowed a zero length area.
	assert(area_len > 0 || (allow_zero_length_area == TRUE && area_len == 0));
	return(TRUE);
}

bool TDecoratedFileViewScreenItem::IntersectWithVisibleLinesRange(long &line_beg, long &num_lines)
{
	if (m_num_isect_lines <= 0 || num_lines <= 0)
		return(FALSE);

	if (line_beg < m_first_isect_line)
	{
		num_lines -= (m_first_isect_line-line_beg);
		if (num_lines <= 0)
			return(FALSE);

		line_beg = m_first_isect_line;
	}

	if (line_beg >= m_first_isect_line+m_num_isect_lines)
		return(FALSE);

	if (line_beg+num_lines > m_first_isect_line+m_num_isect_lines)
	{
		num_lines = m_first_isect_line+m_num_isect_lines-line_beg;
	}

	// The intersection is not empty.
	assert(num_lines > 0);
	return(TRUE);
}

bool TDecoratedFileViewScreenItem::GenerateBasicHtmlPage(THtmlDocument &rprt, const wchar_t *page_title, const wchar_t *css_dir_or_url, const wchar_t **css_file_names, int line_from, int num_lines_to_process)
{
	if (IsInited() == FALSE || rprt.IsWritingState() == FALSE)
		return(FALSE);

	// Emit the top of the HTML page.
	rprt.OpenDocumentHead();
	rprt.AddCssFileReferences(css_dir_or_url, css_file_names);
	rprt.WriteLine();

	if (page_title != NULL)
		rprt.AddPageTitle(page_title);

	rprt.OpenDocumentBody();
	rprt.SetupIndentDelta(-1);
	rprt.WriteLine();

	// Emit the middle of the page. The return value shows if all allocations went fine or not.
	bool overall_success = GenerateHtmlTable(rprt, line_from, num_lines_to_process);
	rprt.WriteLine();

	// Emit the bottom of the HTML page.
	rprt.SetupIndentDelta(0);
	rprt.CloseDocument();
	rprt.Close();
	return(overall_success);
}

bool TDecoratedFileViewScreenItem::GenerateHtmlTable(THtmlDocument &rprt, int line_from, int num_lines_to_process)
{
	if (IsInited() == FALSE || rprt.IsWritingState() == FALSE)
		return(FALSE);

	if (line_from < m_display_lines_first_line)
	{
		// Adjust the startup line to the beginning of the visible area.
		line_from = m_display_lines_first_line;
	}

	if (num_lines_to_process <= 0)
	{
		// The number of lines to emit is not given directly.
		if (m_num_isect_lines > 0 && line_from < m_first_isect_line+m_num_isect_lines)
		{
			// Pick up the end of the visible part of the file.
			num_lines_to_process = m_first_isect_line+m_num_isect_lines-line_from;
		}
	}

	bool overall_success = TRUE;
	if (m_num_isect_lines <= 0)
	{
		TSimpleHtmlElement msg1(L"div", L"The file does not contain displayable lines.");
		rprt.EmitElement(msg1);
	}
	else if (num_lines_to_process <= 0)
	{
		TSimpleHtmlElement msg2(L"div", L"The requested range of lines to process is empty.");
		rprt.EmitElement(msg2);
	}
	else
	{
		// Open the main structure and the HTML table.
		TSimpleHtmlElement main_div(L"div"), main_table(L"table", NULL, 0, m_dfv_style->Props().css_style_name, crlm_outline_object_frames);
		rprt.EmitElementBeg(main_div);
		rprt.EmitElementBeg(main_table);

		TDFVHtmlGenerationContext gen_ctx(this, rprt);
		gen_ctx.m_emt_hlpr.AddDataEmittingFlags(rdem_escape_space | rdem_escape_tab_char);
		if (m_dfv_style->AssembleXmlEscapes() == TRUE)
			gen_ctx.m_emt_hlpr.AddDataEmittingFlags(rdem_no_double_escaping);

		int last_line_to_process = line_from+num_lines_to_process-1;
		for (int nline = line_from; nline <= last_line_to_process; ++nline)
		{
			TDecoratedFileViewLineInfo *dli = GetDFVLineInfo(nline);

			if (dli != NULL)
			{
				// Ask the line to emit itself.
				if (dli->GenerateHtmlLine(gen_ctx, nline) == FALSE)
					overall_success = FALSE;
			}
			else
			{
				// Show the dummy line.
				gen_ctx.m_line_gen.ResetDecorations();
				gen_ctx.m_line_gen.EmitLine(gen_ctx.m_emt_hlpr, nline, L"The line is missing.");
			}
		}

		// Close the table and the main structure.
		rprt.EmitElementEnd(main_table);
		rprt.EmitElementEnd(main_div);
	}

	return(overall_success);
}

// /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
//        ----------- Protected Methods ----------
// \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

// This is static method.
long TDecoratedFileViewScreenItem::GetScrollPos(long req_beg, long req_end, long curr_scroll, long window_height,
												long pixels_above, long pixels_below, long obj_top, long obj_height,
												bool inititial_placement, bool use_smaller_shift)
{
	// (1) Check if the height of the file allows scrolling or not.
	if (obj_height <= window_height)
	{
		// View of the file is not scrollable.
		return(curr_scroll);
	}

	// (2) Compare requested area height with the height of the window.
	req_beg -= obj_top;
	req_end -= obj_top;
	curr_scroll -= obj_top;
	if (req_end-req_beg >= window_height)
	{
		// Requested area is bigger than the available window. Align the area to the top of the current window.
		return(req_beg+obj_top);
	}

	// (3) Check if the window is big enough to display requested extensions. If not, reduce the size of extensions.
	if (pixels_above > 0 || pixels_below > 0)
	{
		if (pixels_above+(req_end-req_beg)+pixels_below > window_height)
		{
			// Compute smaller upper and lower extensions.
			long pix_extra = window_height-(req_end-req_beg);
			if (pixels_below != 0)
			{
				pixels_above = (pixels_above*pix_extra)/(pixels_above+pixels_below);
				pixels_below = pix_extra-pixels_above;
			}
			else
			{
				pixels_above = pix_extra;
			}
		}
	}

	// Update top and bottom extensions taking into account if the file has anything to display in these
	// extenstions or not.
	if (pixels_above > req_beg)
		pixels_above = req_beg;
	if (req_end+pixels_below > obj_height)
		pixels_below = obj_height-req_end;

	// Enlarge the requested strip to cover the adjusted extensions.
	req_beg -= pixels_above;
	req_end += pixels_below;
	if (req_end-req_beg >= window_height)
	{
		// Adgusted area is bigger than the available window.
		return(req_beg+obj_top);
	}

	// (4) Check, if up or down shift is necessary.
	if (inititial_placement == TRUE || req_beg < curr_scroll)
	{
		long new_scroll = req_beg;

		// This check is redundant in case if therent scroll is correct. Keep it for now to allow future scroll pos
		// preparing with this function.
		if (obj_height-new_scroll < window_height)
			new_scroll = req_end-window_height;
		return(new_scroll+obj_top);
	}

	if (req_end > curr_scroll+window_height)
	{
		// Check if the top of the requested area is visible now or not.
		if (req_beg > curr_scroll+window_height)
		{
			// Requested area is not visible at all.
			if (use_smaller_shift == FALSE)
			{
				// Align area to the top of the window.
				long new_scroll = req_beg;
				if (obj_height-new_scroll < window_height)
					new_scroll = req_end-window_height;
				return(new_scroll+obj_top);
			}
		}

		// Align the bottom of requested area to the bottom of the window.
		long new_scroll = req_end-window_height;
		if (new_scroll < 0)
			new_scroll = 0;
		return(new_scroll+obj_top);
	}

	// Existing scroll position is still fine.
	return(curr_scroll+obj_top);
}

void TDecoratedFileViewScreenItem::SetupMaxTextAreaSize(long &dtf_height)
{
	// Retrieve props of the default text font.
	HDC hDC = ::GetDC(m_view->GetControl());
	TDecoratedFileViewStyleProps &props = m_dfv_style->m_props;
	TTextStyle *dts = m_dfv_style->m_default_text_style;
	long def_font_width_10 = dts->GetStringWidth(hDC, L"0123456789", 10, &dtf_height, TRUE, TRUE);
	::ReleaseDC(m_view->GetControl(), hDC);

	// Pick up values from the current props.
	long wa_width = props.text_area_max.cx;
	long wa_height = props.text_area_max.cy;

	// Check, if max width is specified in the number of chars or pixels.
	if (wa_width < 0)
		wa_width = (-wa_width*def_font_width_10)/10;

	// Non zero width should allow for at least 10 chars in default font.
	if (wa_width > 0 && wa_width < def_font_width_10)
		wa_width = def_font_width_10;

	// Check, if height is specified in the number of lines or pixels.
	if (wa_height < 0)
		wa_height = -wa_height*dtf_height;

	// Non zero height should allow for at least 1 line in default font.
	if (wa_height > 0 && wa_height < dtf_height)
		wa_height = dtf_height;

	// Accept an updated size of the area.
	m_max_text_size.cx = wa_width;
	m_max_text_size.cy = wa_height;
}

void TDecoratedFileViewScreenItem::SetupMinTextAreaSize(long bounding_rect_min_x, long bounding_rect_min_y)
{
	// Resolve negative values into the current state of the props.
	if (bounding_rect_min_x < 0)
		bounding_rect_min_x = m_min_text_size.cx;
	if (bounding_rect_min_y < 0)
		bounding_rect_min_y = m_min_text_size.cy;

	// Convert bounding rect size into the text area size.
	TDecoratedFileViewStyleProps &props = m_dfv_style->m_props;
	bounding_rect_min_x -= props.shadow_down_width;
	bounding_rect_min_x -= props.line_nums_width+props.marks_area_width;
	bounding_rect_min_x -= props.text_left_offs+props.text_right_offs;
	bounding_rect_min_y -= props.shadow_down_width;
	bounding_rect_min_y -= props.text_top_offs+props.text_bottom_offs;

	// Accept new values.
	m_min_text_size.cx = __max(bounding_rect_min_x, 0);
	m_min_text_size.cy = __max(bounding_rect_min_y, 0);
}

void TDecoratedFileViewScreenItem::RequestFrgrUpdateForLinesRange(long line_num_start, long num_lines)
{
	SetupFrgrUpdate(GetDFVLineInfo(line_num_start));
	if (num_lines > 1)
		SetupFrgrUpdate(GetDFVLineInfo(line_num_start+num_lines-1));
}

void TDecoratedFileViewScreenItem::ReportFrgrUpdateWanted(TDecoratedFileViewLineInfo *dfv_line)
{
	if (dfv_line->m_spec_tabs_area_index < 0)
	{
		// The situation is simple.
		SetupFrgrUpdate(dfv_line);
	}
	else
	{
		// Invaldate the whole range of lines and reset the layout of the lines area.
		assert(dfv_line->m_spec_tabs_area_index < m_spec_tab_areas.NumItems());
		TDFVSpecTabsArea &sta = m_spec_tab_areas[dfv_line->m_spec_tabs_area_index];
		RequestFrgrUpdateForLinesRange(sta.m_line_num_start, sta.m_num_lines);
		sta.ClearLayout();
	}
}

void TDecoratedFileViewScreenItem::SetupFrgrUpdate(TDecoratedFileViewLineInfo *dfv_line)
{
	if (m_frgr_update_wanted == FALSE)
	{
		// Start a new frgr update area.
		m_frgr_update_wanted = TRUE;
		m_min_invalid_frgr_line = m_max_invalid_frgr_line = dfv_line;
	}
	else
	{
		// Check if existing update area needs enlargment or not.
		if (dfv_line < m_min_invalid_frgr_line)
			m_min_invalid_frgr_line = dfv_line;
		if (dfv_line > m_max_invalid_frgr_line)
			m_max_invalid_frgr_line = dfv_line;
	}
}

bool TDecoratedFileViewScreenItem::PrepareForegroundScreenItems()
{
	bool full_success = TRUE;

	int cnt_rotations = 0;
	while (m_frgr_update_wanted == TRUE)
	{
		// Ensure that the horz update request pointers have reazonable values.
		assert(m_min_invalid_frgr_line != NULL && m_min_invalid_frgr_line >= m_display_lines);
		assert(m_max_invalid_frgr_line != NULL && m_max_invalid_frgr_line >= m_min_invalid_frgr_line);
		assert(m_max_invalid_frgr_line < m_display_lines+m_display_lines_len);

		// Pick up the pointers and clear the request flag.
		TDecoratedFileViewLineInfo *li = m_min_invalid_frgr_line;
		TDecoratedFileViewLineInfo *max_invalid_line = m_max_invalid_frgr_line;
		m_frgr_update_wanted = FALSE;

		// Iterate _all_ affected lines even if updating some of the lines will fail. This loop can increase/decrease
		// the m_text_size.cx. The upper layer should check this and modify the width of the text bkgr rect.
		TDFVPrepareScreenItemsContext ctx(this);
		ctx.line_num = GetLineNumFromLineInfo(li);

		while (li <= max_invalid_line)
		{
			full_success &= li->PrepareScreenItems(ctx);
			if (li->m_frgrw_sens_wlbks == TRUE)
			{
				// This line contains the wlbk decors that depend on the right side of the line foreground.
				// Width of the frgr was recalculated right now. Update the wlbk decors.
				li->UpdateWholeLineBkgrDecors(ctx);
			}

			li++;
			ctx.line_num++;
		}

		if (++cnt_rotations > 4)
		{
			assert(FALSE);
			break;
		}
	}

	return(full_success);
}

void TDecoratedFileViewScreenItem::UpdateObjectWidth()
{
	// Recalculate the width of the text bkgr rect.
	long bkgr_rect_width = __max(m_text_size.cx, m_min_text_size.cx);
	bkgr_rect_width += m_dfv_style->m_props.text_left_offs;
	bkgr_rect_width += m_dfv_style->m_props.text_right_offs;
	bkgr_rect_width += m_dfv_style->m_props.shadow_down_width;

	if (bkgr_rect_width != m_text_rect_scr_item.GetWidth())
	{
		// Text area width has changed.
		long horz_delta = bkgr_rect_width-m_text_rect_scr_item.GetWidth();
		RECT rc_text;
		m_text_rect_scr_item.GetBaseRect(rc_text);
		rc_text.right += horz_delta;
		m_text_rect_scr_item.SetBaseRect(rc_text);

		m_top_area_wlbk_decors[2].GetBaseRect(rc_text);
		rc_text.right += horz_delta;
		m_top_area_wlbk_decors[2].SetBaseRect(rc_text);

		m_bottom_area_wlbk_decors[2].GetBaseRect(rc_text);
		rc_text.right += horz_delta;
		m_bottom_area_wlbk_decors[2].SetBaseRect(rc_text);

		if (m_num_isect_lines > 0)
		{
			// Iterate all lines again. It is necessary to update the whole line bkgrs because the width
			// of all lines has changed.
			TDFVPrepareScreenItemsContext ctx(this);
			long inx_beg = m_first_isect_line-m_display_lines_first_line;
			TDecoratedFileViewLineInfo *li = m_display_lines+inx_beg;
			for (int inx=inx_beg; inx<inx_beg+m_num_isect_lines; ++inx, ++li)
			{
				li->UpdateWholeLineBkgrDecors(ctx);
			}
		}

		// Invalidate the difference area.
		RECT new_inv = *(m_text_rect_scr_item.GetBoundingRect());
		if (horz_delta > 0)
		{
			new_inv.left = new_inv.right;
			new_inv.right += horz_delta;
		}
		else
		{
			new_inv.left = new_inv.right+horz_delta;
		}
		AddToInvalidRect(new_inv);

		// Update rect of the whole object.
		m_bounding_rect.right += horz_delta;
	}
}

void TDecoratedFileViewScreenItem::ReportVertDelta(long new_delta, long line_num)
{
	if (m_vert_delta_pix_value == 0)
	{
		// Start the new vert update request.
		m_vert_delta_pix_value = new_delta;
		m_vert_delta_line_num = line_num;
	}
	else
	{
		if (line_num >= m_vert_delta_line_num)
		{
			// Push the correct line number down and modify the offset after that.
			CheckAndApplyVertDelta(line_num);
			m_vert_delta_pix_value += new_delta;
		}
		else
		{
			// Get rid of the old delta and start the new one.
			ApplyAndDismissVertDelta();
			m_vert_delta_pix_value = new_delta;
			m_vert_delta_line_num = line_num;
		}
	}
}

void TDecoratedFileViewScreenItem::CheckAndApplyVertDelta(long line_num)
{
	//
	// Method  expects that it is called only for non empty files with correct line_num field.
	//
	if (m_vert_delta_pix_value == 0)
	{
		// There is no pending vert adjustment.
		return;
	}

	// Check relation between the passed line and adjusted line.
	if (line_num > m_vert_delta_line_num)
	{
		// Propagate the adjustment down up to and including the passed line.
		long line_num_end = __min(line_num, m_display_lines_first_line+m_display_lines_len-1);
		for (long ln=m_vert_delta_line_num+1; ln <= line_num_end; ++ln)
			m_display_lines[ln-m_display_lines_first_line].ShiftY(this, m_vert_delta_pix_value);

		m_vert_delta_line_num = line_num;
	}
}

void TDecoratedFileViewScreenItem::ApplyAndDismissVertDelta()
{
	if (m_vert_delta_pix_value == 0)
	{
		// There is nothing to do.
		return;
	}

	// In fact, m_vert_delta_line_num is the line number of the last CORRECT line. Pick up the index
	// of the first incorrect line that stays right after it.
	long dln_inx_beg = m_vert_delta_line_num+1-m_display_lines_first_line;

	// Shift all lines starting from this index up to the end.
	for (long inx=dln_inx_beg; inx<m_display_lines_len; ++inx)
		m_display_lines[inx].ShiftY(this, m_vert_delta_pix_value);

	// Modify the total height of all lines.
	m_text_size.cy += m_vert_delta_pix_value;

	// Recalculate the number of visible lines. Start fom the case when all lines are visible. This is possible
	// because their height is known even if some of them are not painted.
	m_num_visible_lines = m_display_lines_len;
	m_visible_lines_height = m_text_size.cy;

	if (m_max_text_size.cy > 0)
	{
		// Check if some of the lines should be hidden.
		while (m_visible_lines_height > m_max_text_size.cy)
		{
			// Hide the bottom line.
			long line_hgt = m_display_lines[m_num_visible_lines-1].GetFullHeight();
			m_num_visible_lines--;
			m_visible_lines_height -= line_hgt;
		}
	}

	// Dismiss the vert update request.
	m_vert_delta_pix_value = 0;
	UpdateObjectHeight();
}

void TDecoratedFileViewScreenItem::UpdateObjectHeight()
{
	// Recalculate the total height of the whole object.
	long obj_height = __max(m_visible_lines_height, m_min_text_size.cy);
	obj_height += m_dfv_style->m_props.text_top_offs;
	obj_height += m_dfv_style->m_props.text_bottom_offs;
	obj_height += m_dfv_style->m_props.shadow_down_width;

	if (obj_height != m_text_rect_scr_item.GetHeight())
	{
		long vert_diff = obj_height-m_text_rect_scr_item.GetHeight();

		// Update the big bkgr rect heights.
		RECT rc_rc;
		m_nums_rect_scr_item.GetBaseRect(rc_rc);
		rc_rc.bottom += vert_diff;
		m_nums_rect_scr_item.SetBaseRect(rc_rc);
		m_marks_rect_scr_item.GetBaseRect(rc_rc);
		rc_rc.bottom += vert_diff;
		m_marks_rect_scr_item.SetBaseRect(rc_rc);
		m_text_rect_scr_item.GetBaseRect(rc_rc);
		rc_rc.bottom += vert_diff;
		m_text_rect_scr_item.SetBaseRect(rc_rc);

		// Update bottom area decor rects.
		for (int iarea=0; iarea<3; ++iarea)
		{
			m_bottom_area_wlbk_decors[iarea].GetBaseRect(rc_rc);
			rc_rc.top += vert_diff;
			rc_rc.bottom += vert_diff;
			m_bottom_area_wlbk_decors[iarea].SetBaseRect(rc_rc);
		}

		// Invalidate the difference.
		RECT new_inv_rect = m_bounding_rect;
		if (vert_diff > 0)
		{
			new_inv_rect.top = new_inv_rect.bottom;
			new_inv_rect.bottom += vert_diff;
		}
		else
		{
			new_inv_rect.top = new_inv_rect.bottom+vert_diff;
		}

		AddToInvalidRect(new_inv_rect);

		// Update rect of the whole object.
		m_bounding_rect.bottom += vert_diff;
	}
}

void TDecoratedFileViewScreenItem::AddToInvalidRect(RECT &rc)
{
	if (m_smth_invalid == FALSE)
	{
		// Create new invalid area. Save current bounding rect. It will be used later
		// to restrict the invalid area.
		m_smth_invalid = TRUE;
		m_rc_invalid = rc;
		m_old_bounding_rect = m_bounding_rect;
	}
	else
	{
		// Enlarge the invalid rect if needed.
		if (rc.left < m_rc_invalid.left)
			m_rc_invalid.left = rc.left;
		if (rc.top < m_rc_invalid.top)
			m_rc_invalid.top = rc.top;
		if (rc.right > m_rc_invalid.right)
			m_rc_invalid.right = rc.right;
		if (rc.bottom > m_rc_invalid.bottom)
			m_rc_invalid.bottom = rc.bottom;
	}
}

void TDecoratedFileViewScreenItem::ReportInvalidRect()
{
	// Ensure that move operations if any updated invalid rects properly.
	assert(m_old_bounding_rect.left == m_bounding_rect.left);
	assert(m_old_bounding_rect.top == m_bounding_rect.top);

	// Pick up maximum of right and bottom bounding rect sizes.
	long max_right = __max(m_bounding_rect.right, m_old_bounding_rect.right);
	long max_bottom = __max(m_bounding_rect.bottom, m_old_bounding_rect.bottom);

	// Ensure that invalid rect does not have areas that stay outside of the current and previous bounding rect.
	m_rc_invalid.left = __max(m_rc_invalid.left, m_bounding_rect.left);
	m_rc_invalid.top = __max(m_rc_invalid.top, m_bounding_rect.top);
	m_rc_invalid.right = __min(m_rc_invalid.right, max_right);
	m_rc_invalid.bottom = __min(m_rc_invalid.bottom, max_bottom);

	// Finaly invalidate the app space.
	if (m_rc_invalid.left < m_rc_invalid.right && m_rc_invalid.top < m_rc_invalid.bottom)
		m_view->InvalidateAppSpaceRect(m_rc_invalid);

	m_smth_invalid = FALSE;
}

//  - - - - - - - - - File space iteration - - - - - - - - - - - -

TDecoratedFileViewLineInfo *TDecoratedFileViewScreenItem::IterateVisibleFileSpace(TFileSpaceIterationInfo &info)
{
	// Check that visble intersection exists and get intersection of the area with the file data.
	if (m_file_data == NULL || m_num_isect_lines <= 0 || info.StepIteration() == FALSE)
		return(NULL);

	if (info.line_inx+m_file_data_first_line < m_first_isect_line)
	{
		// Area begins in the invisible part of the file.
		if (info.line_inx+m_file_data_first_line < m_first_isect_line-1)
		{
			// Skip several lines in one step.
			long line_inx_to = m_first_isect_line-m_file_data_first_line-1;
			long new_beg = m_lines_info[line_inx_to].line_offs;
			info.area_len -= new_beg-info.area_beg;
			if (info.area_len < 0)
				return(NULL);

			info.line_inx = line_inx_to;
			info.area_beg = new_beg;
		}

		// File data space iteration stands before the first visible line. Step onto this line and get intersection with it.
		if (info.StepIteration() == FALSE)
			return(NULL);
	}

	if (info.line_inx+m_file_data_first_line >= m_first_isect_line+m_num_isect_lines)
	{
		// All or part of the area stays after the visible part of the file.
		info.area_len = -1;
		return(NULL);
	}

	// Area has intersection with the visible part of the file.
	return(m_display_lines+(info.line_inx+m_file_data_first_line-m_display_lines_first_line));
}

TDecoratedFileViewLineInfo *TDecoratedFileViewScreenItem::IterateVisibleLinesSpace(long &line_beg, long &num_lines)
{
	if (m_num_isect_lines <= 0 || num_lines <= 0)
		return(NULL);

	if (line_beg < m_first_isect_line)
	{
		// Update the number of lines first.
		num_lines -= m_first_isect_line-line_beg;
		line_beg = m_first_isect_line;
		if (num_lines <= 0)
			return(NULL);
	}

	if (line_beg >= m_first_isect_line+m_num_isect_lines)
	{
		num_lines = -1;
		return(NULL);
	}

	// A non empty intersection is present.
	long inx = line_beg-m_display_lines_first_line;
	line_beg++;
	num_lines--;
	return(m_display_lines+inx);
}

TDecoratedFileViewLineInfo *TDecoratedFileViewScreenItem::GetDFVLineInfo(long line_num)
{
	// Ensure that passed line num can be visible on the screen.
	if (m_num_isect_lines <= 0 || line_num < m_first_isect_line || line_num >= m_first_isect_line+m_num_isect_lines)
		return(NULL);

	// Requested line is present in the array.
	return(m_display_lines+(line_num-m_display_lines_first_line));
}

TDecoratedFileViewLineInfo *TDecoratedFileViewScreenItem::GetDFVLineInfoFromVertPos(long app_y)
{
	// This method should be called only when object is in updated state.
	assert(m_frgr_update_wanted == FALSE);
	assert(m_vert_delta_pix_value == 0);

	// Click can select only something on visible file data lines.
	long upper_nfd_lines = m_first_isect_line-m_display_lines_first_line;
	if (m_num_isect_lines <= 0 || m_num_visible_lines-upper_nfd_lines <= 0)
		return(NULL);

	// At least some part of the file data is visible. Iterate it.
	TDecoratedFileViewLineInfo *li1 = m_display_lines+upper_nfd_lines;
	long lev_y = li1->m_baseline-li1->GetFullHeightAbove();
	for (int i1=m_num_visible_lines-upper_nfd_lines; i1>0; --i1, ++li1)
	{
		if (app_y < lev_y)
		{
			// Current line is below the check point.
			break;
		}

		lev_y += li1->GetFullHeight();
		if (app_y < lev_y)
		{
			// Current line has intersection with the check point.
			return(li1);
		}
	}

	// Passed vertical level has no intersection with visible file data.
	return(NULL);
}

long TDecoratedFileViewScreenItem::GetLineNumFromLineInfo(TDecoratedFileViewLineInfo *dfv_line)
{
	assert(dfv_line != NULL);
	if (m_display_lines == NULL || dfv_line < m_display_lines || dfv_line >= m_display_lines+m_display_lines_len)
		return(-1);

	// Passed line info falls into the valid range.
	return((long)(dfv_line-m_display_lines)+m_display_lines_first_line);
}

static TDecoratedFileWlbkAreaFlags g_WlbkAreaBits[] =
{
	wlbk_nums, wlbk_marks, wlbk_ltext, wlbk_mtext, wlbk_rtext,
};

int TDecoratedFileViewScreenItem::SplitWlbkCoverMask(WORD cover_mask, WORD mask_fragments[3])
{
	int cnt_frags = 0;

	// Split the passed mask into sequence of solid fragments.
	int num_bits = sizeof(g_WlbkAreaBits)/sizeof(TDecoratedFileWlbkAreaFlags);
	for (int i1=0; i1<num_bits; ++i1)
	{
		if ((cover_mask & g_WlbkAreaBits[i1]) != 0)
		{
			// This is the beginning of the new fragment.
			WORD frag = g_WlbkAreaBits[i1];

			// Look for the end of the fragment.
			for (++i1; i1<num_bits; ++i1)
			{
				if ((cover_mask & g_WlbkAreaBits[i1]) == 0)
					break;

				frag |= g_WlbkAreaBits[i1];
			}

			// Store the accumulated mask.
			mask_fragments[cnt_frags++] = frag;
		}
	}

	// Return value is the number of simple fragments.
	return(cnt_frags);
}


