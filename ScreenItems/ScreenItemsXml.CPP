//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "ScreenItems/ScreenItemsXml.H"

//-------------------------------------------------------------------------
//  ===============  Simple Screen Item Styles  =====================
//-------------------------------------------------------------------------

// ----- <BasicStyleProps> -----
const TXmlDictRecord xtags_BasicStyleProps[] =
{
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CssStyle"),					scxt_basic_css_style_name,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_color,		DefTag(L"BkgrColor"),					scxt_basic_background_color,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_color,		DefTag(L"ShadowUpColor"),				scxt_basic_shadow_up_color,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ShadowUpWidth"),			scxt_basic_shadow_up_width,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_color,		DefTag(L"ShadowDownColor"),			scxt_basic_shadow_down_color,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ShadowDownWidth"),			scxt_basic_shadow_down_width,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_color,		DefTag(L"FrameColor"),				scxt_basic_frame_color,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"FrameWidth"),				scxt_basic_frame_width,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"FrameOffset"),				scxt_basic_frame_offset,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"FrameStyle"),				scxt_basic_frame_style,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_color,		DefTag(L"FrameShadowUpColor"),		scxt_basic_frame_shadow_up_color,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"FrameShadowUpWidth"),		scxt_basic_frame_shadow_up_width,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_color,		DefTag(L"FrameShadowDownColor"),		scxt_basic_frame_shadow_down_color,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"FrameShadowDownWidth"),		scxt_basic_frame_shadow_down_width,	},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"InnerMarginHorz"),			scxt_basic_frame_in_marg_horz,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"InnerMarginLeft"),				scxt_basic_frame_in_marg_left,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"InnerMarginRight"),			scxt_basic_frame_in_marg_right,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"InnerMarginVert"),			scxt_basic_frame_in_marg_vert,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"InnerMarginTop"),				scxt_basic_frame_in_marg_top,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"InnerMarginBottom"),			scxt_basic_frame_in_marg_bottom,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OuterPaddingHorz"),			scxt_basic_frame_out_pddn_horz,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OuterPaddingLeft"),			scxt_basic_frame_out_pddn_left,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OuterPaddingRight"),			scxt_basic_frame_out_pddn_right,		},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OuterPaddingVert"),			scxt_basic_frame_out_pddn_vert,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OuterPaddingTop"),			scxt_basic_frame_out_pddn_top,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OuterPaddingBottom"),			scxt_basic_frame_out_pddn_bottom,		},
	{ xdrt_eof }
};

// ----- <LineStyleProps> -----
const TXmlDictRecord xtags_LineStyleProps[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_color,		DefTag(L"FrgrColor"),					scxt_line_foreground_color,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"Width"),						scxt_line_line_width,					},
	{ xdrt_eof }
};

// ----- <TextstyleProps> -----
const TXmlDictRecord xtags_TextStyleProps[] =
{
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"CssStyle"),					scxt_text_css_style_name,				},
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"FontName"),					scxt_text_font_name,					},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"FontHeight"),					scxt_text_font_height,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_bool,		DefTag(L"IsBold"),					scxt_text_is_bold,						},
	{ xdrt_field,	xrdm_req_si,	  xtbt_color,		DefTag(L"FrgrColor"),					scxt_text_foreground_color,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_color,		DefTag(L"BkgrColor"),					scxt_text_background_color,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_color,		DefTag(L"FrameColor"),				scxt_text_frame_color,					},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_string,		DefTag(L"FrameCornerStyle"),			scxt_text_frame_corner_style,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_color,		DefTag(L"ShadowDownColor"),			scxt_text_shadow_down_color,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"ShadowDownWidth"),			scxt_text_shadow_down_width,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"UpSideAdjust"),				scxt_text_up_side_adjust,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"BaselineAdjust"),				scxt_text_baseline_adjust,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"DownSideAdjust"),			scxt_text_down_side_adjust,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"FrontSideExt"),				scxt_text_front_side_ext,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"BackSideExt"),				scxt_text_back_side_ext,				},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OuterFrontPadding"),			scxt_text_outer_front_padding,			},
	{ xdrt_field,	xrdm_opt_si,	  xtbt_number,	DefTag(L"OuterBackPadding"),			scxt_text_outer_back_padding,			},
	{ xdrt_eof }
};

// ----- <NumericParam> -----
const TXmlDictRecord xtags_NumericParam[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						scxt_cln_style_name,					},
	{ xdrt_field,	xrdm_req_si,	  xtbt_number,	DefTag(L"Value"),						scxt_cln_num_value,					},
	{ xdrt_eof }
};

// ----- <BasicStyle> -----
const TXmlDictRecord xtags_BasicStyle[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						scxt_cln_style_name,					},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"BasicStyleProps"),				scxt_cln_style_props,	  xtef_none,		xtags_BasicStyleProps		},
	{ xdrt_eof }
};

// ----- <LineStyle> -----
const TXmlDictRecord xtags_LineStyle[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						scxt_cln_style_name,					},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"LineStyleProps"),				scxt_cln_style_props,	  xtef_none,		xtags_LineStyleProps		},
	{ xdrt_eof }
};

// ----- <TextStyle> -----
const TXmlDictRecord xtags_TextStyle[] =
{
	{ xdrt_field,	xrdm_req_si,	  xtbt_string,		DefTag(L"Name"),						scxt_cln_style_name,					},
	{ xdrt_cnr,	xrdm_req_si,	  xtbt_none,		DefTag(L"TextStyleProps"),				scxt_cln_style_props,	  xtef_none,		xtags_TextStyleProps		},
	{ xdrt_eof }
};

// ----- <StylePropsCollection> -----
const TXmlDictRecord xtags_StylePropsCollection[] =
{
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"NumericParam"),				scxt_cln_num_param,	  xtef_none,		xtags_NumericParam			},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"BasicStyle"),					scxt_cln_basic_style,	  xtef_none,		xtags_BasicStyle				},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"LineStyle"),					scxt_cln_line_style,		  xtef_none,		xtags_LineStyle				},
	{ xdrt_cnr,	xrdm_opt_mi,	  xtbt_none,		DefTag(L"TextStyle"),					scxt_cln_text_style,	  xtef_none,		xtags_TextStyle				},
	{ xdrt_eof }
};

// ----------------   Root Containers Directory   -------------------------

const TXmlDictRecord xtags_ScreenItemStyleRootContainersDirectory[] =
{
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"BasicStyleProps"),				scxt_basic_props,		  xtef_none,		xtags_BasicStyleProps		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"LineStyleProps"),				scxt_line_props,		  xtef_none,		xtags_LineStyleProps		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"TextStyleProps"),				scxt_text_props,		  xtef_none,		xtags_TextStyleProps		},
	{ xdrt_cnr,	xrdm_opt_si,	  xtbt_none,		DefTag(L"StylePropsCollection"),			scxt_props_collection,	  xtef_none,		xtags_StylePropsCollection	},
	{ xdrt_eof }
};

//---------------------------------------------------------------------------
//  ===============  TScreenItemsStyleXmlHelper  =====================
//---------------------------------------------------------------------------

void TScreenItemsStyleXmlHelper::DumpBasicStyleProps(TXmlWriter &doc, WORD tag_id, TBasicStyleProps &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, scxt_basic_props, xtags_ScreenItemStyleRootContainersDirectory);

	if (data.css_style_name != NULL)
		doc.WriteStringTag(scxt_basic_css_style_name, data.css_style_name);

	doc.WriteColorTag(scxt_basic_background_color, data.background_color);

	if (data.shadow_up_width != 0)
	{
		if (data.shadow_up_color != cs_transparent)
			doc.WriteColorTag(scxt_basic_shadow_up_color, data.shadow_up_color);

		doc.WriteNumTag(scxt_basic_shadow_up_width, data.shadow_up_width);
	}

	if (data.shadow_down_width != 0)
	{
		if (data.shadow_down_color != cs_transparent)
			doc.WriteColorTag(scxt_basic_shadow_down_color, data.shadow_down_color);

		doc.WriteNumTag(scxt_basic_shadow_down_width, data.shadow_down_width);
	}

	if (data.frame_color != cs_transparent)
		doc.WriteColorTag(scxt_basic_frame_color, data.frame_color);
	if (data.frame_width != 0)
		doc.WriteNumTag(scxt_basic_frame_width, data.frame_width);
	if (data.frame_offset != 0)
		doc.WriteNumTag(scxt_basic_frame_offset, data.frame_offset);

	if (data.frame_style == PS_DASH)
		doc.WriteStringTag(scxt_basic_frame_style, L"PS_DASH");
	else if (data.frame_style == PS_DOT)
		doc.WriteStringTag(scxt_basic_frame_style, L"PS_DOT");

	if (data.frame_shadow_up_width != 0)
	{
		if (data.frame_shadow_up_color != cs_transparent)
			doc.WriteColorTag(scxt_basic_frame_shadow_up_color, data.frame_shadow_up_color);

		doc.WriteNumTag(scxt_basic_frame_shadow_up_width, data.frame_shadow_up_width);
	}

	if (data.frame_shadow_down_width != 0)
	{
		if (data.frame_shadow_down_color != cs_transparent)
			doc.WriteColorTag(scxt_basic_frame_shadow_down_color, data.frame_shadow_down_color);

		doc.WriteNumTag(scxt_basic_frame_shadow_down_width, data.frame_shadow_down_width);
	}

	if (data.inner_margin_left == data.inner_margin_right)
	{
		if (data.inner_margin_left != 0)
			doc.WriteNumTag(scxt_basic_frame_in_marg_horz, data.inner_margin_left);
	}
	else
	{
		 if (data.inner_margin_left != 0)
			doc.WriteNumTag(scxt_basic_frame_in_marg_left, data.inner_margin_left);
		if (data.inner_margin_right != 0)
			doc.WriteNumTag(scxt_basic_frame_in_marg_right, data.inner_margin_right);
	}

	if (data.inner_margin_top == data.inner_margin_bottom)
	{
		if (data.inner_margin_top != 0)
			doc.WriteNumTag(scxt_basic_frame_in_marg_vert, data.inner_margin_top);
	}
	else
	{
		if (data.inner_margin_top != 0)
			doc.WriteNumTag(scxt_basic_frame_in_marg_top, data.inner_margin_top);
		if (data.inner_margin_bottom != 0)
			doc.WriteNumTag(scxt_basic_frame_in_marg_bottom, data.inner_margin_bottom);
	}

	if (data.outer_padding_left == data.outer_padding_right)
	{
		if (data.outer_padding_left != 0)
			doc.WriteNumTag(scxt_basic_frame_out_pddn_horz, data.outer_padding_left);
	}
	else
	{
		 if (data.outer_padding_left != 0)
			doc.WriteNumTag(scxt_basic_frame_out_pddn_left, data.outer_padding_left);
		if (data.outer_padding_right != 0)
			doc.WriteNumTag(scxt_basic_frame_out_pddn_right, data.outer_padding_right);
	}

	if (data.outer_padding_top == data.outer_padding_bottom)
	{
		if (data.outer_padding_top != 0)
			doc.WriteNumTag(scxt_basic_frame_out_pddn_vert, data.outer_padding_top);
	}
	else
	{
		if (data.outer_padding_top != 0)
			doc.WriteNumTag(scxt_basic_frame_out_pddn_top, data.outer_padding_top);
		if (data.outer_padding_bottom != 0)
			doc.WriteNumTag(scxt_basic_frame_out_pddn_bottom, data.outer_padding_bottom);
	}

	doc.CloseContainerEx(rcnr_dct);
}

void TScreenItemsStyleXmlHelper::DumpLineStyleProps(TXmlWriter &doc, WORD tag_id, TLineStyleProps &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, scxt_line_props, xtags_ScreenItemStyleRootContainersDirectory);

	doc.WriteColorTag(scxt_line_foreground_color, data.foreground_color);
	if (data.line_width != 0)
		doc.WriteNumTag(scxt_line_line_width, data.line_width);

	doc.CloseContainerEx(rcnr_dct);
}

void TScreenItemsStyleXmlHelper::DumpTextStyleProps(TXmlWriter &doc, WORD tag_id, TTextStyleProps &data)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, scxt_text_props, xtags_ScreenItemStyleRootContainersDirectory);

	if (data.css_style_name != NULL)
		doc.WriteStringTag(scxt_text_css_style_name, data.css_style_name);

	doc.WriteStringTag(scxt_text_font_name, data.font_name);
	doc.WriteNumTag(scxt_text_font_height, data.font_height);
	if (data.is_bold != FALSE)
		doc.WriteBoolTag(scxt_text_is_bold, data.is_bold);

	doc.WriteColorTag(scxt_text_foreground_color, data.foreground_color);
	if (data.background_color != cs_transparent)
		doc.WriteColorTag(scxt_text_background_color, data.background_color);

	if (data.frame_color != cs_transparent && data.frame_corner_style != fcs_none)
	{
		doc.WriteColorTag(scxt_text_frame_color, data.frame_color);

		wchar_t buff_40_chars[40];
		doc.WriteStringTag(scxt_text_frame_corner_style, TTextStyleProps::GetFrameCornerStyleText(data.frame_corner_style, buff_40_chars));
	}

	if (data.shadow_down_width != 0)
	{
		if (data.shadow_down_color != cs_transparent)
			doc.WriteColorTag(scxt_text_shadow_down_color, data.shadow_down_color);

		doc.WriteNumTag(scxt_text_shadow_down_width, data.shadow_down_width);
	}

	if (data.up_side_adjust != 0)
		doc.WriteNumTag(scxt_text_up_side_adjust, data.up_side_adjust);
	if (data.baseline_adjust != 0)
		doc.WriteNumTag(scxt_text_baseline_adjust, data.baseline_adjust);
	if (data.down_side_adjust != 0)
		doc.WriteNumTag(scxt_text_down_side_adjust, data.down_side_adjust);

	if (data.front_side_ext != 0)
		doc.WriteNumTag(scxt_text_front_side_ext, data.front_side_ext);
	if (data.back_side_ext != 0)
		doc.WriteNumTag(scxt_text_back_side_ext, data.back_side_ext);
	if (data.outer_front_padding != 0)
		doc.WriteNumTag(scxt_text_outer_front_padding, data.outer_front_padding);
	if (data.outer_back_padding != 0)
		doc.WriteNumTag(scxt_text_outer_back_padding, data.outer_back_padding);

	doc.CloseContainerEx(rcnr_dct);
}

//------------------------------------------------------------------------------------------
//	===============  TBasicScreenItemStyleXmlContainerHandler  =====================
//------------------------------------------------------------------------------------------

void TBasicScreenItemStyleXmlContainerHandler::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case scxt_basic_shadow_up_width:			StoreIntValue(tagValue, 0, 100, m_pdata->shadow_up_width, tagName);				break;
		case scxt_basic_shadow_down_width:			StoreIntValue(tagValue, 0, 100, m_pdata->shadow_down_width, tagName);			break;

		case scxt_basic_frame_width:				StoreShortValue(tagValue, 0, 100, m_pdata->frame_width, tagName);				break;
		case scxt_basic_frame_offset:				StoreShortValue(tagValue, 0, 100, m_pdata->frame_offset, tagName);				break;
		case scxt_basic_frame_shadow_up_width:		StoreIntValue(tagValue, 0, 100, m_pdata->frame_shadow_up_width, tagName);		break;
		case scxt_basic_frame_shadow_down_width:	StoreIntValue(tagValue, 0, 100, m_pdata->frame_shadow_down_width, tagName);		break;

		case scxt_basic_frame_in_marg_left:			StoreShortValue(tagValue, 0, 100, m_pdata->inner_margin_left, tagName);			break;
		case scxt_basic_frame_in_marg_right:		StoreShortValue(tagValue, 0, 100, m_pdata->inner_margin_right, tagName);			break;
		case scxt_basic_frame_in_marg_top:			StoreShortValue(tagValue, 0, 100, m_pdata->inner_margin_top, tagName);			break;
		case scxt_basic_frame_in_marg_bottom:		StoreShortValue(tagValue, 0, 100, m_pdata->inner_margin_bottom, tagName);			break;

		case scxt_basic_frame_in_marg_horz:		StoreShortValue(tagValue, 0, 100, m_pdata->inner_margin_left, tagName);
												StoreShortValue(tagValue, 0, 100, m_pdata->inner_margin_right, tagName);
												break;

		case scxt_basic_frame_in_marg_vert:			StoreShortValue(tagValue, 0, 100, m_pdata->inner_margin_top, tagName);
												StoreShortValue(tagValue, 0, 100, m_pdata->inner_margin_bottom, tagName);
												break;

		case scxt_basic_frame_out_pddn_left:		StoreShortValue(tagValue, 0, 100, (short&)m_pdata->outer_padding_left, tagName);		break;
		case scxt_basic_frame_out_pddn_right:		StoreShortValue(tagValue, 0, 100, (short&)m_pdata->outer_padding_right, tagName);		break;
		case scxt_basic_frame_out_pddn_top:		StoreShortValue(tagValue, 0, 100, (short&)m_pdata->outer_padding_top, tagName);		break;
		case scxt_basic_frame_out_pddn_bottom:		StoreShortValue(tagValue, 0, 100, (short&)m_pdata->outer_padding_bottom, tagName);		break;

		case scxt_basic_frame_out_pddn_horz:		StoreShortValue(tagValue, 0, 100, (short&)m_pdata->outer_padding_left, tagName);
												StoreShortValue(tagValue, 0, 100, (short&)m_pdata->outer_padding_right, tagName);
												break;

		case scxt_basic_frame_out_pddn_vert:		StoreShortValue(tagValue, 0, 100, (short&)m_pdata->outer_padding_top, tagName);
												StoreShortValue(tagValue, 0, 100, (short&)m_pdata->outer_padding_bottom, tagName);
												break;
	}
}

void TBasicScreenItemStyleXmlContainerHandler::ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case scxt_basic_frame_style:
				{
					if (strValue == L"PS_DASH")
						m_pdata->frame_style = PS_DASH;
					else if (strValue == L"PS_DOT")
						m_pdata->frame_style = PS_DOT;
				}
				break;
	}
}

void TBasicScreenItemStyleXmlContainerHandler::ProcessColorField(WORD tagId, TColor tagValue, TStringPtr &tagName)
{
	// Value of the color does not need additional verification.
	switch (tagId)
	{
		case scxt_basic_background_color:			m_pdata->background_color = tagValue;			break;
		case scxt_basic_shadow_up_color:			m_pdata->shadow_up_color = tagValue;			break;
		case scxt_basic_shadow_down_color:			m_pdata->shadow_down_color = tagValue;		break;
		case scxt_basic_frame_color:				m_pdata->frame_color = tagValue;				break;
		case scxt_basic_frame_shadow_up_color:		m_pdata->frame_shadow_up_color = tagValue;	break;
		case scxt_basic_frame_shadow_down_color:	m_pdata->frame_shadow_down_color = tagValue;	break;
	}
}

//------------------------------------------------------------------------------------------
//  ================  TLineScreenItemStyleXmlContainerHandler  ======================
//------------------------------------------------------------------------------------------

void TLineScreenItemStyleXmlContainerHandler::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case scxt_line_line_width:			StoreIntValue(tagValue, 0, 100, m_pdata->line_width, tagName); break;
	}
}

void TLineScreenItemStyleXmlContainerHandler::ProcessColorField(WORD tagId, TColor tagValue, TStringPtr &tagName)
{
	// Value of the color does not need additional verification.
	switch (tagId)
	{
		case scxt_line_foreground_color:		m_pdata->foreground_color = tagValue; break;
	}
}

//----------------------------------------------------------------------------------------
//  ===============  TTextScreenItemStyleXmlContainerHandler  =====================
//----------------------------------------------------------------------------------------

void TTextScreenItemStyleXmlContainerHandler::ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case scxt_text_is_bold:				m_pdata->is_bold = tagValue; break;
	}
}

void TTextScreenItemStyleXmlContainerHandler::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case scxt_text_font_height:				StoreShortValue(tagValue, 1, 160, m_pdata->font_height, tagName);					break;
		case scxt_text_shadow_down_width:		StoreShortValue(tagValue, 0, 100, m_pdata->shadow_down_width, tagName);			break;
		case scxt_text_up_side_adjust:			StoreShortValue(tagValue, -50, 50, m_pdata->up_side_adjust, tagName);				break;
		case scxt_text_baseline_adjust:			StoreShortValue(tagValue, -50, 50, m_pdata->baseline_adjust, tagName);				break;
		case scxt_text_down_side_adjust:		StoreShortValue(tagValue, -50, 50, m_pdata->down_side_adjust, tagName);			break;
		case scxt_text_front_side_ext:			StoreCharValue(tagValue, 0, 80, (char&)m_pdata->front_side_ext, tagName);			break;
		case scxt_text_back_side_ext:			StoreCharValue(tagValue, 0, 80, (char&)m_pdata->back_side_ext, tagName);			break;
		case scxt_text_outer_front_padding:		StoreCharValue(tagValue, 0, 80, (char&)m_pdata->outer_front_padding, tagName);		break;
		case scxt_text_outer_back_padding:		StoreCharValue(tagValue, 0, 80, (char&)m_pdata->outer_back_padding, tagName);		break;
	}
}

void TTextScreenItemStyleXmlContainerHandler::ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName)
{
	switch (tagId)
	{
		case scxt_text_font_name:
				{
					StoreStringValue(strValue, m_pdata->font_name, 80, tagName);
				}
				break;

		case scxt_text_frame_corner_style:
				{
					TFrameCornerStyle style;
					if (TTextStyleProps::GetFrameCornerStyleFromText(strValue, style) == FALSE)
					{
						m_parser->ReportXmlAppError(L"Error assembling style of the frame corner.");
						break;
					}

					// The string was recognized.
					m_pdata->frame_corner_style = style;
				}
				break;
	}
}

void TTextScreenItemStyleXmlContainerHandler::ProcessColorField(WORD tagId, TColor tagValue, TStringPtr &tagName)
{
	// Value of the color does not need additional verification.
	switch (tagId)
	{
		case scxt_text_foreground_color:	m_pdata->foreground_color = tagValue;		break;
		case scxt_text_background_color:	m_pdata->background_color = tagValue;		break;
		case scxt_text_frame_color:			m_pdata->frame_color = tagValue;			break;
		case scxt_text_shadow_down_color:	m_pdata->shadow_down_color = tagValue;	break;
	}
}

//--------------------------------------------------------------------------------------
//  ==================  TScreenItemStylePropsCollection  ========================
//--------------------------------------------------------------------------------------

TNamedScreenItemStyleProps *TScreenItemStylePropsCollection::FindPropsItem(const wchar_t *props_name, short props_type, TGenericConsole *cons, const wchar_t *cons_prefix)
{
	for (TListIter<TNamedScreenItemStyleProps> iter(m_data); iter; ++iter)
	{
		if (wcscmp(iter.CurrItem().m_style_name, props_name) == 0)
		{
			// Props with the matching name are available.
			if (iter.CurrItem().m_style_type_id != props_type)
			{
				// Props type mismatch.
				if (cons != NULL)
				{
					wchar_t buff1[240];
					swprintf(buff1, 240, L"*****%s%s%s Style \"%s\" is present but it belongs to the wrong type.",
										(cons_prefix != NULL) ? L" " : L"", (cons_prefix != NULL) ? cons_prefix : L"", (cons_prefix != NULL) ? L":" : L"",
										props_name);
					cons->ShowRunTimeError(buff1);
				}

				// Return failure.
				return(NULL);
			}

			// Return the non empty data.
			iter.CurrItem().m_cnt_use++;
			return(&(iter.CurrItem()));
		}
	}

	// Props name is unknown.
	if (cons != NULL)
	{
		wchar_t buff2[240];
		swprintf(buff2, 240, L"*****%s%s%s Style \"%s\" is missing.",
							(cons_prefix != NULL) ? L" " : L"", (cons_prefix != NULL) ? cons_prefix : L"", (cons_prefix != NULL) ? L":" : L"",
							props_name);
		cons->ShowRunTimeError(buff2);
	}

	// Return failure.
	return(NULL);
}

bool TScreenItemStylePropsCollection::PrepareNumParam(__int64 &param_value, const wchar_t *param_name, TGenericConsole *cons, const wchar_t *cons_error_prefix)
{
	TNamedScreenItemStyleProps *item = FindPropsItem(param_name, NumParam_type_ID, cons, cons_error_prefix);
	if (item == NULL)
		return(FALSE);

	// Requested data is available.
	param_value = item->m_num_value;
	return(TRUE);
}

bool TScreenItemStylePropsCollection::PrepareBasicProps(TBasicStyleProps &style_props, const wchar_t *props_name, TGenericConsole *cons, const wchar_t *cons_error_prefix)
{
	TNamedScreenItemStyleProps *item = FindPropsItem(props_name, TBasicStyle::type_ID, cons, cons_error_prefix);
	if (item == NULL)
		return(FALSE);

	// Requested data is available.
	style_props = item->m_basic_props;
	return(TRUE);
}

bool TScreenItemStylePropsCollection::PrepareLineProps(TLineStyleProps &style_props, const wchar_t *props_name, TGenericConsole *cons, const wchar_t *cons_error_prefix)
{
	TNamedScreenItemStyleProps *item = FindPropsItem(props_name, TLineStyle::type_ID, cons, cons_error_prefix);
	if (item == NULL)
		return(FALSE);

	// Requested data is available.
	style_props = item->m_line_props;
	return(TRUE);
}

bool TScreenItemStylePropsCollection::PrepareTextProps(TTextStyleProps &style_props, const wchar_t *props_name, TGenericConsole *cons, const wchar_t *cons_error_prefix)
{
	TNamedScreenItemStyleProps *item = FindPropsItem(props_name, TTextStyle::type_ID, cons, cons_error_prefix);
	if (item == NULL)
		return(FALSE);

	// Requested data is available.
	style_props = item->m_text_props;
	return(TRUE);
}

void TScreenItemStylePropsCollection::ReportUnusedStyles(TGenericConsole *cons, const wchar_t *cons_prefix)
{
	if (cons == NULL)
		return;

	for (TListIter<TNamedScreenItemStyleProps> iter(m_data); iter; ++iter)
	{
		if (iter.CurrItem().m_cnt_use == 0)
		{
			// The current set of props was never queried.
			wchar_t buff3[240];
			swprintf(buff3, 240, L"*****%s%s%s Style \"%s\" (line %d) was never used.",
								(cons_prefix != NULL) ? L" " : L"", (cons_prefix != NULL) ? cons_prefix : L"", (cons_prefix != NULL) ? L":" : L"",
								iter.CurrItem().m_style_name, iter.CurrItem().m_src_line_num);
			cons->ShowRunTimeError(buff3);
		}
	}
}

void TScreenItemStylePropsCollection::DumpPropsCollection(TXmlWriter &doc, WORD tag_id)
{
	TXmlDictRecord *rcnr_dct = doc.OpenContainerEx(tag_id, scxt_props_collection, xtags_ScreenItemStyleRootContainersDirectory);

	for (TListIter<TNamedScreenItemStyleProps> iter(m_data); iter; ++iter)
	{
		switch (iter.CurrItem().m_style_type_id)
		{
			case TBasicStyle::type_ID:
					{
						doc.OpenContainer(scxt_cln_basic_style);
						doc.WriteStringTag(scxt_cln_style_name, iter.CurrItem().m_style_name);
						TScreenItemsStyleXmlHelper::DumpBasicStyleProps(doc, scxt_cln_style_props, iter.CurrItem().m_basic_props);
						doc.CloseContainer();
					}
					break;

			case TLineStyle::type_ID:
					{
						doc.OpenContainer(scxt_cln_line_style);
						doc.WriteStringTag(scxt_cln_style_name, iter.CurrItem().m_style_name);
						TScreenItemsStyleXmlHelper::DumpLineStyleProps(doc, scxt_cln_style_props, iter.CurrItem().m_line_props);
						doc.CloseContainer();
					}
					break;

			case TTextStyle::type_ID:
					{
						doc.OpenContainer(scxt_cln_text_style);
						doc.WriteStringTag(scxt_cln_style_name, iter.CurrItem().m_style_name);
						TScreenItemsStyleXmlHelper::DumpTextStyleProps(doc, scxt_cln_style_props, iter.CurrItem().m_text_props);
						doc.CloseContainer();
					}
					break;

			case NumParam_type_ID:
					{
						doc.OpenContainer(scxt_cln_num_param);
						doc.WriteStringTag(scxt_cln_style_name, iter.CurrItem().m_style_name);
						doc.WriteNumTag(scxt_cln_num_value, iter.CurrItem().m_num_value);
						doc.CloseContainer();
					}
					break;

			default:
				{
					// Unsupported props type or the data corruption.
					assert(FALSE);
				}
				break;
		}
	}

	doc.CloseContainerEx(rcnr_dct);
}

bool TScreenItemStylePropsCollection::LoadFromExeDirectory(const wchar_t *short_fname, TGenericConsole *cons, TDateTime *latest_scanned_file_date, bool *new_file_detected_flag)
{
	// Do the scanning.
	return(TXmlContainerHandler::LoadFromExeDirectory(xtags_StylePropsCollection, L"ScreenItemStylesCollection", short_fname, cons, latest_scanned_file_date, new_file_detected_flag));
}

TXmlContainerHandler *TScreenItemStylePropsCollection::OpenSubContainer(WORD tagId, TStringPtr &tagName)
{
	if (m_current_style == NULL)
	{
		// Parsing is on the collection items level. Create new data structure for handling the new record.
		m_current_style = new TNamedScreenItemStyleProps();
		if (m_current_style == NULL)
		{
			m_parser->ReportXmlAppError(L"Low on memory while creating the style props record.");
			return(&TXmlReader::s_skip_subtree_handler);
		}

		// The record was created. Do the app level preparation.
		switch (tagId)
		{
			case scxt_cln_basic_style:	m_current_style->m_style_type_id = TBasicStyle::type_ID;	break;
			case scxt_cln_line_style:	m_current_style->m_style_type_id = TLineStyle::type_ID;	break;
			case scxt_cln_text_style:	m_current_style->m_style_type_id = TTextStyle::type_ID;	break;
			case scxt_cln_num_param:	m_current_style->m_style_type_id = NumParam_type_ID;	break;

			default:
				assert(FALSE);
				return(&TXmlReader::s_skip_subtree_handler);
		}

		// Store the line number of the beginning of the container for future use.
		m_current_style->m_src_line_num = m_parser->GetLatestLexemaLineNumber();

		// Handler for the style level is still this object.
		return(this);
	}
	else
	{
		// Parsing is on the single style level.
		switch (m_current_style->m_style_type_id)
		{
			case TBasicStyle::type_ID:	  m_basic_handler.SetDestination(&(m_current_style->m_basic_props));	return(&m_basic_handler);
			case TLineStyle::type_ID:	  m_line_handler.SetDestination(&(m_current_style->m_line_props));		return(&m_line_handler);
			case TTextStyle::type_ID:	  m_text_handler.SetDestination(&(m_current_style->m_text_props));		return(&m_text_handler);

			default:
				assert(FALSE);
				return(&TXmlReader::s_skip_subtree_handler);
		}
	}
}

void TScreenItemStylePropsCollection::FinalizeNodeProcessing(WORD tagId, bool nodeScanningProblems)
{
	if (m_current_style != NULL)
	{
		if (nodeScanningProblems == FALSE)
		{
			// Scanning of the style went fine. Ensure that the name of the new record is unique.
			for (TListIter<TNamedScreenItemStyleProps> iter(m_data); iter; ++iter)
			{
				if (wcscmp(iter.CurrItem().m_style_name, m_current_style->m_style_name) == 0)
				{
					m_parser->ReportXmlAppError(L"Duplication of the style name \"%s\". Previous definition is on the line %d.",
												m_current_style->m_style_name, iter.CurrItem().m_src_line_num);
					delete m_current_style;
					m_current_style = NULL;
					break;
				}
			}

			if (m_current_style != NULL)
				m_data.AppendItem(m_current_style);
		}
		else
		{
			// There were scanning problems. Get rid of the bogus style.
			delete m_current_style;
		}

		m_current_style = NULL;

		m_basic_handler.SetDestination(NULL);
		m_line_handler.SetDestination(NULL);
		m_text_handler.SetDestination(NULL);
	}
}

void TScreenItemStylePropsCollection::ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName)
{
	// String field can appear only when scanning is on the single style level.
	assert(m_current_style != NULL);
	assert(tagId == scxt_cln_style_name);

	StoreStringValue(strValue, m_current_style->m_style_name, 40, tagName);
}

void TScreenItemStylePropsCollection::ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName)
{
	// Integer field can appear only when scanning is on the single style level.
	assert(m_current_style != NULL);
	assert(tagId == scxt_cln_num_value);

	m_current_style->m_num_value = tagValue;
}


