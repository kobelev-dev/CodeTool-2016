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

#include  "ScreenItems/XmlFileView.H"

const TXmlItemPresentationProps g_XmlView_ShowLeafTagAsContainer = { 0xFF, 0xFF, 0xFF, TRUE, };

// -----------------------------------------------------------------------
//    ==================  TXmlFileViewStyle  =====================
// -----------------------------------------------------------------------

static const TDecoratedFileViewStyleProps g_XmlView_WholeFileProps_DefaultSet =
{
	L"xfat_table",

	40, 1,					// Nums area width, marks area width.
	7, 16, 8, 8,				// Text offsets: left, right, top, bottom.
	L"%d:", align_right, 3,		// Line numbers painting info.
	{ 0, 0 },					// Text area may have any size.
	{ 300, 0 },				// Min object size.

	RGB(238, 238, 238),		// Nums area background.				// RGB(209, 248, 226),
	RGB(167, 198, 248),		// Marks area background.
	RGB(244, 249, 244),		// Text area background.				// RGB(221, 255, 236),

	L"xfat_line_num",
	L"xfat_marks_area",
	L"xfat_line_body",

	2, RGB(128, 128, 128),		// Shadow information.
	RGB(80, 0, 0),				// Mouse selection bkgr color.
	RGB(160, 200, 160),		// Mouse selection frgr color.
	2,						// Mouse selection horz ext.
};

static const TTabsSpacesViewMode g_XmlView_WholeFileTabsMode_DefaultSet =
{
	FALSE,					// Visible tabs.
	FALSE,					// Visible spaces.
	FALSE,					// Implicit multispace tabs.
	6,						// Number of char places in one tab character.
	75, 110,					// Width of the tab symbol itself and width of the char place that are used in the tab
							// pos calculations in percents of the width of the basic char in the default text style.
	75,						// Width of the implicit multispace tab, similar to the width of the tab char.
};

static const TDecoratedFileViewTextProps g_XmlView_WholeFileTextProps_DefaultSet =
{
	{  NULL,	COURIER_NEW,	 8,	FALSE,	RGB(100, 100, 100),	STD_BKGR,  -1, 0, 1,  },		// Line num props.
	{  NULL,	COURIER_NEW,	 9,	FALSE,	RGB(0, 0, 0),			STD_BKGR,  -1, 0, 1,  },		// Default text props.
	{  NULL,	ARIAL,			 9,	TRUE,	RGB(60, 0, 0),			STD_TXT },					// Line overflow props.
};

static const TTextStyleProps  g_XmlView_StdAreasTextProps_DefaultSet[xfat_num_area_types] =
{
	{  L"xfat_xml_header",		SMALL_FONTS,	 6,	FALSE,	RGB(200, 140, 240),	STD_BKGR,	0, 0, 4  },	// xfat_xml_header_frame
	{  L"xfat_xml_header",		SMALL_FONTS,	 6,	FALSE,	RGB(200, 140, 140),	STD_BKGR,	0, 0, 4  },	// xfat_xml_header_body
	{  L"xfat_comment",		COURIER_NEW,	 8,	FALSE,	RGB(96, 96, 255),		STD_TXT },				// xfat_xml_comment_frame
	{  L"xfat_comment",		COURIER_NEW,	 8,	FALSE,	RGB(80, 80, 255),		STD_TXT },				// xfat_xml_comment_body

	{  L"xfat_delimiter",		MS_SANS_SERIF,	 8,	FALSE,	RGB(80, 80, 180),		STD_TXT },				// xfat_delim_tag_frame
	{  L"xfat_delimiter",		MS_SANS_SERIF,	 8,	FALSE,	RGB(80, 80, 180),		STD_TXT },				// xfat_delim_tag_hdr_equ

	{  L"xfat_attr_name",		COURIER_NEW,	 8,	FALSE,	RGB(120, 120, 120),	STD_TXT },				// xfat_tag_attr_name
	{  L"xfat_tag_open",		COURIER_NEW,	 8,	FALSE,	RGB(120, 120, 120),	STD_TXT },				// xfat_tag_name_open
	{  L"xfat_tag_close",		COURIER_NEW,	 8,	FALSE,	RGB(120, 120, 120),	STD_TXT },				// xfat_tag_name_close

	{  L"xfat_cnr_open",		COURIER_NEW,	 8,	FALSE,	RGB(105, 31, 27),		RGB(222, 234, 222),	STD_NO_FRSH,   0, 0, 0,   1, 1  },			// xfat_container_name_open
	{  L"xfat_cnr_close",		COURIER_NEW,	 8,	FALSE,	RGB(105, 31, 27),		RGB(222, 234, 222),	STD_NO_FRSH,   0, 0, 0,   1, 1  },			// xfat_container_name_close

	{  L"xfat_attr_value",		VERDANA,		 8,	FALSE,	RGB(20, 37, 186),		RGB(251, 237, 136),	STD_NO_FRSH,   0, 0, 0,   1, 1  },			// xfat_tag_attr_value
	{  L"xfat_tag_value",		VERDANA,		 9,	TRUE,	RGB(80, 80, 160),		STD_TXT },												// xfat_leaf_tag_value
	{  L"xfat_cnr_body",		VERDANA,		 8,	TRUE,	RGB(166, 41, 180),		STD_TXT },												// xfat_container_body

	{  L"xfat_syntax_error",		TAHOMA,			10,	FALSE,	RGB(24, 0, 0),			RGB(255, 41, 41),		STD_NO_FRSH,   0, 0, 1,   3, 2,   1, 1  },	// xfat_xml_syntax_error
	{  L"xfat_syntax_error",		TAHOMA,			10,	FALSE,	RGB(0, 24, 0),			RGB(255, 41, 41),		STD_NO_FRSH,   0, 0, 1,   3, 2,   1, 1  },	// xfat_xml_schema_error
	{  L"xfat_syntax_error",		TAHOMA,			10,	FALSE,	RGB(0, 0, 24),			RGB(255, 41, 41),		STD_NO_FRSH,   0, 0, 1,   3, 2,   1, 1  },	// xfat_app_layer_error
};

// Adjustments for xfat_delim_tag_frame stnd style.
static const TTextStyleSymbolAdjustInfo g_XmlView_TagDelimAdjustments[] =
{
	{ L'/', 1, 0, 1 }, { 0, 0, 0, 0 },
};

// - - - - - - - - - - -  Methods  - - - - - - - - - - - - -

bool TXmlFileViewStyle::SetupDefaultStyles()
{
	assert(m_xml_stls_prepared == FALSE);

	// Prepare text styles that will be used in the decorated file screen item style.
	m_line_nums_style.Setup(g_XmlView_WholeFileTextProps_DefaultSet.nums_props);
	m_default_text_style.Setup(g_XmlView_WholeFileTextProps_DefaultSet.text_props);
	m_line_overflow_style.Setup(g_XmlView_WholeFileTextProps_DefaultSet.ovfl_props);

	// Prepare style for decorated file screen item.
	m_file_view_style.Setup(g_XmlView_WholeFileProps_DefaultSet,
						g_XmlView_WholeFileTabsMode_DefaultSet,
						TRUE,															// The data in the view is expected to be XML escaped.
						&m_line_nums_style, &m_default_text_style, &m_line_overflow_style);

	// Setup styles for various types of areas in the XML file.
	for (int ifrgr=0; ifrgr<xfat_num_area_types; ++ifrgr)
	{
		const TTextStyleSymbolAdjustInfo *adj_info = NULL;
		if (ifrgr == xfat_delim_tag_frame)
			adj_info = g_XmlView_TagDelimAdjustments;

		m_stnd_xml_area_styles[ifrgr].Setup(g_XmlView_StdAreasTextProps_DefaultSet[ifrgr], adj_info);
	}

	m_xml_stls_prepared = TRUE;
	return(TRUE);
}

bool TXmlFileViewStyle::SetupAppTagStyles(const TTextStyleProps *props_array, int props_array_len)
{
	assert(props_array != NULL && props_array_len > 0);
	if (m_app_xml_area_styles != NULL)
	{
		// Get rid of the old non empty props info.
		 delete[] m_app_xml_area_styles;
		 m_app_xml_area_styles = NULL;
		 m_app_xml_area_styles_len = 0;
	}

	// Allocate space for the new array of styles.
	m_app_xml_area_styles = new TTextStyle[props_array_len];
	if (m_app_xml_area_styles == NULL)
		return(FALSE);

	// Process props that were passed by the application layer.
	for (int istyle=0; istyle<props_array_len; ++istyle)
		m_app_xml_area_styles[istyle].Setup(props_array[istyle]);

	// Success.
	m_app_xml_area_styles_len = props_array_len;
	return(TRUE);
}

bool TXmlFileViewStyle::SetupForDeviceContext(HDC hDC)
{
	assert(m_xml_stls_prepared == TRUE);
	bool fsucc = TScreenItemStyle::SetupForDeviceContext(hDC);

	// Whole file style.
	fsucc &= m_file_view_style.SetupForDeviceContext(hDC);

	// Stnd styles.
	for (int ifrgr=0; ifrgr<xfat_num_area_types; ++ifrgr)
		fsucc &= m_stnd_xml_area_styles[ifrgr].SetupForDeviceContext(hDC);

	if (m_app_xml_area_styles != NULL)
	{
		// App layer custom styles.
		for (int istyle=0; istyle<m_app_xml_area_styles_len; ++istyle)
			m_app_xml_area_styles[istyle].SetupForDeviceContext(hDC);
	}

	// Return the overall result.
	return(fsucc);
}

TTextStyle *TXmlFileViewStyle::GetStndFrgrAreaStyle(TXmlFileAreaType stnd_style_index)
{
	assert(stnd_style_index >= 0 && stnd_style_index < xfat_num_area_types);
	return(&(m_stnd_xml_area_styles[stnd_style_index]));
}

TTextStyle *TXmlFileViewStyle::GetAppFrgrAreaStyle(BYTE app_style_index)
{
	if (m_app_xml_area_styles == NULL || app_style_index == 0xFF || app_style_index >= m_app_xml_area_styles_len)
		return(NULL);

	return(&(m_app_xml_area_styles[app_style_index]));
}

// --------------------------------------------------------------------------
//   =================  TXmlFileViewScreenItem  =====================
// --------------------------------------------------------------------------

TXmlFileViewScreenItem::TXmlFileViewScreenItem()
{
	m_xml_decor_creation_errors = 0;
}

void TXmlFileViewScreenItem::SetStyle(TXmlFileViewStyle *new_style)
{
	if (new_style != NULL)
		assert(new_style->StyleTypeId() == TXmlFileViewStyle::type_ID);

	// Save the style in the data field of the base class.
	m_style = new_style;
}

void TXmlFileViewScreenItem::ResetSourceFile()
{
	// Reset the state of the base object.
	Reset();
	m_xml_decor_creation_errors = 0;
}

bool TXmlFileViewScreenItem::SetupXmlFile(TScreenItemsViewObject *view, TXmlItemPresentationRecord *special_tags_dict,
											const wchar_t *fdata, long fdata_len, TDataBypassMode dbp_mode,
											int line_from, int num_lines)
{
	// At this point the XmlView level style should be already set.
	assert(GetBasicStyle() != NULL && view != NULL);
	ResetSourceFile();

	bool r1 = Setup(view, XmlViewStyle()->GetWholeFileStyle(),
					fdata, fdata_len, dbp_mode,		// Body of the file.
					0, 1,								// File data src offset and the file data first line.
					NULL, NULL, dbm_use_only,			// Base class should create the lines linfo.
					line_from, num_lines,			// Range of lines to display.
					0, 0);							// Initial position of the screen item.

	if (r1 == TRUE && fdata != NULL)
	{
		// Run the parsing session. Real work will be done inside the callback handler.
		TXmlStream xml_stream(L"TXmlFileViewScreenItem::SetupXmlFile", fdata, fdata_len);
		TXmlReader reader(SyntaxHighlightCallbackHandler, this, special_tags_dict);
		reader.SyntaxHighlightParse(xml_stream);
		UpdateControl();
	}

	return(r1 == TRUE && m_xml_decor_creation_errors == 0);
}

bool TXmlFileViewScreenItem::GetXmlData(TDestinationFile &rprt) const
{
	if (m_file_data == NULL || m_file_data_len <= 0)
		return(FALSE);

	// File data is availaible in the base class.
	rprt.Write(m_file_data, m_file_data_len);
	return(TRUE);
}

void TXmlFileViewScreenItem::ApplySyntaxHighlightDecoration(TXmlFileAreaInfo &xml_area_info)
{
	TTextStyle *frgr_style = NULL;
	TXmlDecorsZorder xdz = xdz_normal;
	if (xml_area_info.xml_area_type >= xfat_tag_attr_name && xml_area_info.xml_area_type <= xfat_container_body)
	{
		// Current area can have application layer style.
		if (xml_area_info.xml_tag_info != NULL && xml_area_info.xml_tag_info->m_presentation_props != NULL)
		{
			// Props are present. Pick up the style index.
			BYTE app_style_index = 0xFF;
			switch (xml_area_info.xml_area_type)
			{
				case xfat_tag_attr_name:
				case xfat_tag_name_open:
				case xfat_container_name_open:
						app_style_index = xml_area_info.xml_tag_info->m_presentation_props->m_tag_open_style_inx;
						break;

				case xfat_tag_attr_value:
				case xfat_leaf_tag_value:
				case xfat_container_body:
						app_style_index = xml_area_info.xml_tag_info->m_presentation_props->m_tag_body_style_inx;
						break;

				case xfat_tag_name_close:
				case xfat_container_name_close:
						app_style_index = xml_area_info.xml_tag_info->m_presentation_props->m_tag_close_style_inx;
						break;
			}

			if (app_style_index != 0xFF)
			{
				// Style index indicates that the app specific props are present.
				frgr_style = XmlViewStyle()->GetAppFrgrAreaStyle(app_style_index);
			}
		}
	}

	if (frgr_style == NULL)
	{
		// Pick up the stnd style.
		frgr_style = XmlViewStyle()->GetStndFrgrAreaStyle(xml_area_info.xml_area_type);
	}

	if (xdz == xdz_normal)
	{
		if (xml_area_info.xml_area_type == xfat_xml_comment_frame || xml_area_info.xml_area_type == xfat_xml_comment_body)
			xdz = xdz_comment;
		else if (xml_area_info.xml_area_type >= xfat_xml_syntax_error && xml_area_info.xml_area_type <= xfat_app_layer_error)
			xdz = xdz_error;
	}

	if (frgr_style != NULL && xml_area_info.xml_area_len > 0)
	{
		// The style was retrieved and the area is not empty. Create decor without owner for now.
		bool r2 = AddFrgrDecoration(NULL, 0, xdz, xml_area_info.xml_area_beg, xml_area_info.xml_area_len, frgr_style);
		if (r2 == FALSE)
			m_xml_decor_creation_errors++;
	}
}


