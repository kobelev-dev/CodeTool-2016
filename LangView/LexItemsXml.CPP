//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "LangView/LexItemsXml.H"

// -----------------------------------------------------------------------
//   =================  LexItem Xml TagsDict  ====================
// -----------------------------------------------------------------------

// <LexItemStyleInfo>
static const TXmlDictRecord xtags_LexItemStyleInfo[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Symbol"),					lixt_sym_index,			},

	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CnctIcon"),					lixt_cnct_icon,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"PcbIcon"),					lixt_pcb_icon,				},

	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"MainBkgrStyle"),				lixt_main_bkgr_style,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"LowerBkgrStyle"),				lixt_pcb_lower_style,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"LoBkgrTranspNonTerm"),		lixt_pcb_lo_trasp_nterm,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"LoBkgrOpaqueNonTerm"),		lixt_pcb_lo_opaq_nterm,	},

	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"MainLabelStyle"),				lixt_main_label_style,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"SymLabelStyle"),				lixt_symbol_label_style,		},

	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"CircleDiameter"),				lixt_cnct_circle_diam,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MainLabShiftX"),				lixt_cnct_main_lab_shx,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MainLabShiftY"),				lixt_cnct_main_lab_shy,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SymLabShiftX"),				lixt_cnct_sym_lab_shx,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SymLabShiftY"),				lixt_cnct_sym_lab_shy,		},

	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MaxRectWidth"),				lixt_pcb_max_rectw,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MainLabMargins"),				lixt_pcb_mlab_margins,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MainLabDownShift"),			lixt_pcb_mlab_down_sh,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"MainLabLoSpace"),			lixt_pcb_mlab_lo_space,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SymLabLeftMargin"),			lixt_pcb_symlab_lmarg,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"SymLabRightMargin"),			lixt_pcb_symlab_rmarg,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"LowerRectHeight"),			lixt_pcb_lo_rect_height,		},

	{ xdrt_eof }
};

// <LexItemStyles>
static const TXmlDictRecord xtags_LexItemsStyles[] =
{
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"StylePropsCollection"),			lixt_low_level_styles,		xtef_none,		xtags_StylePropsCollection	},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"LexemaTypeStyle"),			lixt_lexema_style,			xtef_none,		xtags_LexItemStyleInfo		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"KeywordTypeStyle"),			lixt_keyword_style,			xtef_none,		xtags_LexItemStyleInfo		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"MidLevObjectStyle"),			lixt_mid_lev_obj_style,		xtef_none,		xtags_LexItemStyleInfo		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"NonTerminalStyle"),			lixt_non_term_style,		xtef_none,		xtags_LexItemStyleInfo		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"SpecialItemStyle"),			lixt_spec_item_style,		xtef_none,		xtags_LexItemStyleInfo		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"VisualParserItemStyle"),		lixt_vpars_item_style,		xtef_none,		xtags_LexItemStyleInfo		},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"CppDatabaseItemStyle"),		lixt_cpp_item_style,		xtef_none,		xtags_LexItemStyleInfo		},
	{ xdrt_eof }
};

// -------------------------------------------------------------------------------------
//   =================  TLangViewLexItemStyleInfoXmlHandler  ====================
// -------------------------------------------------------------------------------------

TLangViewLexItemStyleInfoXmlHandler::TLangViewLexItemStyleInfoXmlHandler()
{
	m_owner = NULL;
	m_list_type = lixt_none;
}

void TLangViewLexItemStyleInfoXmlHandler::SetupHandler(TLangViewLexItemStylePropsXmlHandler *owner, TLexItemXmlTag list_type)
{
	assert(owner != NULL);
	assert(list_type >= lixt_lexema_style && list_type <= lixt_cpp_item_style);

	m_owner = owner;
	m_list_type = list_type;
}

void TLangViewLexItemStyleInfoXmlHandler::BeginNodeProcessing(WORD tagId)
{
	m_sym_value = -1;
	m_any_symbol = FALSE;

	m_buffer.Clear();
	m_buffer_flags.Clear();

	m_transp_nterm_lo_bkgr = NULL;
	m_opaque_nterm_lo_bkgr = NULL;
}

void TLangViewLexItemStyleInfoXmlHandler::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	if (nodeScanningProblems == FALSE)
	{
		// Scanning the container has fully succeeded.
		if (m_sym_value >= 0)
		{
			// This is single record case.
			ApplyExplicitFields(GetStyleInfoArrayPtr(m_list_type)+m_sym_value);
		}
		else if (m_any_symbol == TRUE)
		{
			// All slots of the current category should be filled.
			TLexItemStyleInfo *dest_info = GetStyleInfoArrayPtr(m_list_type);
			int array_len = GetStyleInfoArrayLen(m_list_type);

			// It is important that param of the loop is m_sym_value.
			for (m_sym_value = 0; m_sym_value<array_len; ++m_sym_value, ++dest_info)
				ApplyExplicitFields(dest_info);
		}
	}
}

void TLangViewLexItemStyleInfoXmlHandler::ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case lixt_sym_index:			ResolveSymbolName(strValue, tagName);
									break;

		case lixt_main_bkgr_style:		ResolveBasicStyleName(m_buffer.basic_props.main_bkgr_style, strValue, tagName);
									m_buffer_flags.basic_props.main_bkgr_style = (TBasicStyle*)-1;
									break;

		case lixt_pcb_lower_style:		ResolveBasicStyleName(m_buffer.basic_props.pcb_lower_bkgr_style, strValue, tagName);
									m_buffer_flags.basic_props.pcb_lower_bkgr_style = (TBasicStyle*)-1;
									break;

		case lixt_pcb_lo_trasp_nterm:	ResolveBasicStyleName(m_transp_nterm_lo_bkgr, strValue, tagName);
									break;

		case lixt_pcb_lo_opaq_nterm:	ResolveBasicStyleName(m_opaque_nterm_lo_bkgr, strValue, tagName);
									break;

		case lixt_main_label_style:		ResolveTextStyleName(m_buffer.basic_props.main_label_style, strValue, tagName);
									m_buffer_flags.basic_props.main_label_style = (TTextStyle*)-1;
									break;

		case lixt_symbol_label_style:	ResolveTextStyleName(m_buffer.basic_props.symbol_label_style, strValue, tagName);
									m_buffer_flags.basic_props.symbol_label_style = (TTextStyle*)-1;
									break;

		default:
			assert(FALSE);
			break;
	}
}

void TLangViewLexItemStyleInfoXmlHandler::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case lixt_cnct_icon:			StoreShortValue(tagValue, 100, 4999, m_buffer.basic_props.app_cnct_icon, tagName);
									m_buffer_flags.basic_props.app_cnct_icon = -1;
									break;
		case lixt_pcb_icon:			StoreShortValue(tagValue, 100, 4999, m_buffer.basic_props.app_pcb_icon, tagName);
									m_buffer_flags.basic_props.app_pcb_icon = -1;
									break;

		case lixt_cnct_circle_diam:		StoreShortValue(tagValue, 1, 500, m_buffer.basic_props.cnct_circle_diameter, tagName);
									m_buffer_flags.basic_props.cnct_circle_diameter = -1;
									break;
		case lixt_cnct_main_lab_shx:	StoreCharValue(tagValue, -100, 100, m_buffer.basic_props.cnct_main_label_shift_x, tagName);
									m_buffer_flags.basic_props.cnct_main_label_shift_x = -1;
									break;
		case lixt_cnct_main_lab_shy:	StoreCharValue(tagValue, -100, 100, m_buffer.basic_props.cnct_main_label_shift_y, tagName);
									m_buffer_flags.basic_props.cnct_main_label_shift_y = -1;
									break;
		case lixt_cnct_sym_lab_shx:		StoreCharValue(tagValue, -100, 100, m_buffer.basic_props.cnct_symbol_label_shift_x, tagName);
									m_buffer_flags.basic_props.cnct_symbol_label_shift_x = -1;
									break;
		case lixt_cnct_sym_lab_shy:		StoreCharValue(tagValue, -100, 100, m_buffer.basic_props.cnct_symbol_label_shift_y, tagName);
									m_buffer_flags.basic_props.cnct_symbol_label_shift_y = -1;
									break;

		case lixt_pcb_max_rectw:		StoreShortValue(tagValue, 1, 500, m_buffer.basic_props.pcb_max_rect_width, tagName);
									m_buffer_flags.basic_props.pcb_max_rect_width = -1;
									break;
		case lixt_pcb_mlab_margins:	StoreCharValue(tagValue, 1, 100, m_buffer.basic_props.pcb_main_label_margins, tagName);
									m_buffer_flags.basic_props.pcb_main_label_margins = -1;
									break;
		case lixt_pcb_mlab_down_sh:	StoreCharValue(tagValue, 0, 100, m_buffer.basic_props.pcb_main_label_down_shift, tagName);
									m_buffer_flags.basic_props.pcb_main_label_down_shift = -1;
									break;
		case lixt_pcb_mlab_lo_space:	StoreCharValue(tagValue, 0, 100, m_buffer.basic_props.pcb_main_label_lo_space, tagName);
									m_buffer_flags.basic_props.pcb_main_label_lo_space = -1;
									break;
		case lixt_pcb_symlab_lmarg:	StoreCharValue(tagValue, 1, 100, m_buffer.basic_props.pcb_symlab_left_margin, tagName);
									m_buffer_flags.basic_props.pcb_symlab_left_margin = -1;
									break;
		case lixt_pcb_symlab_rmarg:	StoreCharValue(tagValue, 1, 100, m_buffer.basic_props.pcb_symlab_right_margin, tagName);
									m_buffer_flags.basic_props.pcb_symlab_right_margin = -1;
									break;
		case lixt_pcb_lo_rect_height:	StoreCharValue(tagValue, 1, 100, m_buffer.basic_props.pcb_lower_rect_height, tagName);
									m_buffer_flags.basic_props.pcb_lower_rect_height = -1;
									break;

		default:
			assert(FALSE);
			break;
	}
}

void TLangViewLexItemStyleInfoXmlHandler::ResolveSymbolName(TStringPtr &strValue, TStringPtr &tagName)
{
	if (strValue.IsEmpty() == TRUE)
	{
		m_parser->ReportXmlAppError(L"Name of the symbol cannot be empty.");
	}
	else if (strValue == L"any" || strValue == L"Any" || strValue == L"ANY" || strValue == L"all" || strValue == L"All" || strValue == L"ALL")
	{
		m_any_symbol = TRUE;
	}
	else
	{
		// Create zero terminated symbol name.
		wchar_t buffer[80];
		if (StoreStringValue(strValue, buffer, 80, tagName) == FALSE)
			return;

		switch (m_list_type)
		{
			case lixt_lexema_style:			m_sym_value = m_owner->m_dest_style->ResolveLexemaTypeName(buffer);
										break;
			case lixt_keyword_style:		m_sym_value = m_owner->m_dest_style->ResolveKeywordName(buffer);
										break;
			case lixt_mid_lev_obj_style:		m_sym_value = m_owner->m_dest_style->ResolveMidLevEventTypeName(buffer);
										break;

			case lixt_non_term_style:		if (m_owner->m_dest_style->m_non_terminal_styles_len <= 0)
										{
											// The grammar is missing or it has no non terminals or allocation of the non term infos failed.
											// Silently ignore this record.
											return;
										}

										// Resolve name of the non terminal according to the current grammar.
										m_sym_value = m_owner->m_dest_style->ResolveNonTerminalName(buffer);
										break;

			case lixt_spec_item_style:		m_sym_value = m_owner->m_dest_style->ResolveSpecItemName(buffer);
										break;
			case lixt_vpars_item_style:		m_sym_value = m_owner->m_dest_style->ResolveVisualParserItemName(buffer);
										break;
			case lixt_cpp_item_style:		m_sym_value = m_owner->m_dest_style->ResolveCppDatabaseItemName(buffer);
										break;

			default:
				assert(FALSE);
				break;
		}

		if (m_sym_value < 0)
		{
			m_parser->ReportXmlAppError(L"Name of the symbol is bogus.");
		}
	}
}

void TLangViewLexItemStyleInfoXmlHandler::ResolveBasicStyleName(TBasicStyle *&style, TStringPtr &strValue, TStringPtr &tagName)
{
	wchar_t style_name[80];
	if (strValue.IsEmpty() == TRUE)
	{
		m_parser->ReportXmlAppError(L"Name of the BasicStyleProps cannot be empty.");
	}
	else if (StoreStringValue(strValue, style_name, 80, tagName) == TRUE)
	{
		if (style_name[0] == L'@')
		{
			if (m_owner->m_dest_style->ResolveBuiltInBasicStyleName(style, style_name+1) == FALSE)
			{
				m_parser->ReportXmlAppError(L"Name of the built in BasicStyle is bogus.");
			}
		}
		else
		{
			TNamedScreenItemStyleProps *sp_item = m_owner->m_low_level_styles.FindPropsItem(style_name, TBasicStyle::type_ID);
			if (sp_item == NULL)
			{
				m_parser->ReportXmlAppError(L"Basic style props \"%s\" for <%.*s> are missing.", style_name, tagName.GetLength(), tagName.GetBodyPtr());
			}
			else if (m_owner->m_dest_style->FindOrAllocateDynamicBasicStyle(style, sp_item, m_owner->m_dim_grade) == FALSE)
			{
				m_parser->ReportXmlAppError(L"Allocating the BasicStyle failed.");
			}
		}
	}
}

void TLangViewLexItemStyleInfoXmlHandler::ResolveTextStyleName(TTextStyle *&style, TStringPtr &strValue, TStringPtr &tagName)
{
	wchar_t style_name[80];
	if (strValue.IsEmpty() == TRUE)
	{
		m_parser->ReportXmlAppError(L"Name of the TextStyleProps cannot be empty.");
		return;
	}
	else if (StoreStringValue(strValue, style_name, 80, tagName) == TRUE)
	{
		if (style_name[0] == L'@')
		{
			if (m_owner->m_dest_style->ResolveBuiltInTextStyleName(style, style_name+1) == FALSE)
			{
				m_parser->ReportXmlAppError(L"Name of the built in TextStyle is bogus.");
			}
		}
		else
		{
			TNamedScreenItemStyleProps *sp_item = m_owner->m_low_level_styles.FindPropsItem(style_name, TTextStyle::type_ID);
			if (sp_item == NULL)
			{
				m_parser->ReportXmlAppError(L"Text style props \"%s\" for <%.*s> are missing.", style_name, tagName.GetLength(), tagName.GetBodyPtr());
			}
			else if (m_owner->m_dest_style->FindOrAllocateDynamicTextStyle(style, sp_item, m_owner->m_dim_grade) == FALSE)
			{
				m_parser->ReportXmlAppError(L"Allocating the TextStyle failed.");
			}
		}
	}
}

TLexItemStyleInfo *TLangViewLexItemStyleInfoXmlHandler::GetStyleInfoArrayPtr(TLexItemXmlTag list_type)
{
	TLangViewLexItemStyle *stl = m_owner->m_dest_style;
	switch (list_type)
	{
		case lixt_lexema_style:			return(stl->m_lexema_type_styles);
		case lixt_keyword_style:		return(stl->m_keyword_styles);
		case lixt_mid_lev_obj_style:		return(stl->m_mid_lev_event_styles);

		case lixt_non_term_style:
				{
					// Note that result of non terminal infos allocation is checked here rather than presence of the goood grammar.
					// This avoid crashing in rare cases plus this is consistent with implementation of the GetNonTerminalStyleInfo().
					return((stl->m_non_terminal_styles_len <= 0) ? &(stl->m_def_non_terminal_style) : stl->m_non_terminal_styles);
				}

		case lixt_spec_item_style:		return(stl->m_spec_item_styles);
		case lixt_vpars_item_style:		return(stl->m_vpars_item_styles);
		case lixt_cpp_item_style:		return(stl->m_cpp_item_styles);

		default:
			assert(FALSE);
			return(NULL);
	}
}

int TLangViewLexItemStyleInfoXmlHandler::GetStyleInfoArrayLen(TLexItemXmlTag list_type)
{
	TLangViewLexItemStyle *stl = m_owner->m_dest_style;
	switch (list_type)
	{
		case lixt_lexema_style:			return(ltx_num_lexema_types);
		case lixt_keyword_style:		return(ltkn_num_keyword_vals);
		case lixt_mid_lev_obj_style:		return(TLangViewLexItemStyle::mid_lev_object_num_styles);

		case lixt_non_term_style:
				{
					// Note that result of non terminal infos allocation is checked here rather than presence of the goood grammar.
					// This avoids crashing in rare cases plus this is consistent with implementation of the GetNonTerminalStyleInfo().
					return((stl->m_non_terminal_styles_len <= 0) ? 1 : stl->m_non_terminal_styles_len);
				}

		case lixt_spec_item_style:		return(slit_num_types);
		case lixt_vpars_item_style:		return(vplit_num_types);
		case lixt_cpp_item_style:		return(TLangViewLexItemStyle::cpp_item_num_styles);

		default:
			assert(FALSE);
			return(-1);
	}
}

void  TLangViewLexItemStyleInfoXmlHandler::ApplyExplicitFields(TLexItemStyleInfo *dest)
{
	// App icons.
	if (m_buffer_flags.basic_props.app_cnct_icon != 0)
		dest->basic_props.app_cnct_icon = m_buffer.basic_props.app_cnct_icon;
	if (m_buffer_flags.basic_props.app_pcb_icon != 0)
		dest->basic_props.app_pcb_icon = m_buffer.basic_props.app_pcb_icon;

	// Main background.
	if (m_buffer_flags.basic_props.main_bkgr_style != NULL)
		dest->basic_props.main_bkgr_style = m_buffer.basic_props.main_bkgr_style;

	// Rect lines placing mode lower rect background.
	if (m_list_type != lixt_non_term_style || m_owner->m_highlight_non_terms_transp == FALSE || m_owner->m_dest_style->m_non_terminal_styles_len <= 0)
	{
		// This is simple case. Use default lower bkgr style.
		if (m_buffer_flags.basic_props.pcb_lower_bkgr_style != NULL)
			dest->basic_props.pcb_lower_bkgr_style = m_buffer.basic_props.pcb_lower_bkgr_style;
	}
	else
	{
		// Situation is more complex. Use specific bkgr to assign the lower bkgr style.
		TNonTerminalSymbol &non_term_info = m_owner->m_dest_style->m_grammar->non_terminals[m_sym_value];
		TBasicStyle *stl = (non_term_info.solid_symbol == FALSE) ? m_transp_nterm_lo_bkgr : m_opaque_nterm_lo_bkgr;
		if (stl != NULL)
			dest->basic_props.pcb_lower_bkgr_style = stl;
	}

	// Main and symbol labels style.
	if (m_buffer_flags.basic_props.main_label_style != NULL)
		dest->basic_props.main_label_style = m_buffer.basic_props.main_label_style;
	if (m_buffer_flags.basic_props.symbol_label_style != NULL)
		dest->basic_props.symbol_label_style = m_buffer.basic_props.symbol_label_style;

	// Concentric placing mode specific params group.
	if (m_buffer_flags.basic_props.cnct_circle_diameter != 0)
		dest->basic_props.cnct_circle_diameter = m_buffer.basic_props.cnct_circle_diameter;
	if (m_buffer_flags.basic_props.cnct_main_label_shift_x != 0)
		dest->basic_props.cnct_main_label_shift_x = m_buffer.basic_props.cnct_main_label_shift_x;
	if (m_buffer_flags.basic_props.cnct_main_label_shift_y != 0)
		dest->basic_props.cnct_main_label_shift_y = m_buffer.basic_props.cnct_main_label_shift_y;
	if (m_buffer_flags.basic_props.cnct_symbol_label_shift_x != 0)
		dest->basic_props.cnct_symbol_label_shift_x = m_buffer.basic_props.cnct_symbol_label_shift_x;
	if (m_buffer_flags.basic_props.cnct_symbol_label_shift_y != 0)
		dest->basic_props.cnct_symbol_label_shift_y = m_buffer.basic_props.cnct_symbol_label_shift_y;

	// Rect lines placing mode specific params group.
	if (m_buffer_flags.basic_props.pcb_max_rect_width != 0)
		dest->basic_props.pcb_max_rect_width = m_buffer.basic_props.pcb_max_rect_width;
	if (m_buffer_flags.basic_props.pcb_main_label_margins != 0)
		dest->basic_props.pcb_main_label_margins = m_buffer.basic_props.pcb_main_label_margins;
	if (m_buffer_flags.basic_props.pcb_main_label_down_shift != 0)
		dest->basic_props.pcb_main_label_down_shift = m_buffer.basic_props.pcb_main_label_down_shift;
	if (m_buffer_flags.basic_props.pcb_main_label_lo_space != 0)
		dest->basic_props.pcb_main_label_lo_space = m_buffer.basic_props.pcb_main_label_lo_space;
	if (m_buffer_flags.basic_props.pcb_symlab_left_margin != 0)
		dest->basic_props.pcb_symlab_left_margin = m_buffer.basic_props.pcb_symlab_left_margin;
	if (m_buffer_flags.basic_props.pcb_symlab_right_margin != 0)
		dest->basic_props.pcb_symlab_right_margin = m_buffer.basic_props.pcb_symlab_right_margin;
	if (m_buffer_flags.basic_props.pcb_lower_rect_height != 0)
		dest->basic_props.pcb_lower_rect_height = m_buffer.basic_props.pcb_lower_rect_height;
}

// --------------------------------------------------------------------------------------
//   =================  TLangViewLexItemStylePropsXmlHandler  ====================
// --------------------------------------------------------------------------------------

TLangViewLexItemStylePropsXmlHandler::TLangViewLexItemStylePropsXmlHandler(TLangViewLexItemStyle *dest, int dim_grade, bool highlight_non_terms_transp)
{
	m_dest_style = dest;
	m_dim_grade = dim_grade;
	m_highlight_non_terms_transp = highlight_non_terms_transp;
}

bool TLangViewLexItemStylePropsXmlHandler::LoadFromExeDirectory(const wchar_t *short_fname, TGenericConsole *cons, TDateTime *latest_scanned_file_date)
{
	// Do the scanning.
	return(TXmlContainerHandler::LoadFromExeDirectory(xtags_LexItemsStyles, L"LexItemStyles", short_fname, cons, latest_scanned_file_date));
}

bool TLangViewLexItemStylePropsXmlHandler::ResolveFixedBasicStyleName(TBasicStyle *&style, const wchar_t *style_name)
{
	TNamedScreenItemStyleProps *sp_item = m_low_level_styles.FindPropsItem(style_name, TBasicStyle::type_ID);
	if (sp_item == NULL)
	{
		// This may look strange, but this is ok.
		return(TRUE);
	}

	// Style props are present in the collection.
	return(m_dest_style->FindOrAllocateDynamicBasicStyle(style, sp_item, m_dim_grade));
}

bool TLangViewLexItemStylePropsXmlHandler::ResolveFixedTextStyleName(TTextStyle *&style, const wchar_t *style_name)
{
	TNamedScreenItemStyleProps *sp_item = m_low_level_styles.FindPropsItem(style_name, TTextStyle::type_ID);
	if (sp_item == NULL)
	{
		// This may look strange, but this is ok.
		return(TRUE);
	}

	// Style props are present in the collection.
	return(m_dest_style->FindOrAllocateDynamicTextStyle(style, sp_item, m_dim_grade));
}

bool TLangViewLexItemStylePropsXmlHandler::ResolveFixedLineStyleName(TLineStyle *&style, const wchar_t *style_name)
{
	TNamedScreenItemStyleProps *sp_item = m_low_level_styles.FindPropsItem(style_name, TLineStyle::type_ID);
	if (sp_item == NULL)
	{
		// This may look strange, but this is ok.
		return(TRUE);
	}

	// Style props are present in the collection.
	return(m_dest_style->FindOrAllocateDynamicLineStyle(style, sp_item, m_dim_grade));
}

TXmlContainerHandler *TLangViewLexItemStylePropsXmlHandler::OpenSubContainer(WORD tagId, TStringPtr &tagName)
{
	switch (tagId)
	{
		case lixt_low_level_styles:
				return(&m_low_level_styles);

		case lixt_lexema_style:
		case lixt_keyword_style:
		case lixt_mid_lev_obj_style:
		case lixt_non_term_style:
		case lixt_spec_item_style:
		case lixt_vpars_item_style:
		case lixt_cpp_item_style:
				m_style_info_handler.SetupHandler(this, (TLexItemXmlTag)tagId);
				return(&m_style_info_handler);

		default:
			assert(FALSE);
			return(NULL);
	}
}

void TLangViewLexItemStylePropsXmlHandler::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	assert(tagId == ROOT_CONTAINER_TAG_ID);

	if (nodeScanningProblems == FALSE)
	{
		// Note that warnings will be really emitted only if reader was inited with non NULL console.
		TFileNameBuffer buffer;
		TPathHelper::ExtractShortName(buffer, m_parser->GetXmlStreamFileName());
		m_low_level_styles.ReportUnusedStyles(m_parser->GetReaderConsole(), buffer);
	}
}


