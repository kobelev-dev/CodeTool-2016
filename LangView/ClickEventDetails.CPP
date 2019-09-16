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

#include  "Common/FormatNumber.H"
#include  "TextScan/MidStoreXml.H"
#include  "TextScan/CppDatabaseXml.H"
#include  "LangView/ClickEventDetails.H"
#include  "LangView/LangViewResources.H"

static TIconsGridStyleProps g_LangViewClickEventDetails_IconsProps =
{
	NULL,						// No specific CSS style.
	IDBMP_EVENT_DETAILS,		// Resource ID.
	TRUE,						// Transparent pixels are present.
	TRUE,						// Individual boundaries for each icon.
	75,							// Resolution.
	19, 12,						// Left-top position of the cell 100.
	22, 24,						// Step between the cells.
	19, 21,						// Size of the cell.
};

//
//	Icons on the "Bitmaps/Event-Details-Icons.BMP" icons grid.
//
#define	GRM_DTLS_CODE_AREA			200
#define	GRM_DTLS_RAW_LEXEMA			201
#define	GRM_DTLS_MIDLEV_EVENT		202

// ----------------------------------------------------------------------
//    ==============  TLangViewFileIdDetailsHelper  =================
// ----------------------------------------------------------------------

void TLangViewFileIdDetailsHelper::Prepare(POINT &se_point, TMidLevStoreReader *db, ID pars_id, ID file_id)
{
	if (file_id == 0 || db == NULL)
	{
		// Display only the file ids without anything else.
		PrepareInternal(se_point, pars_id, file_id, FALSE);
	}
	else
	{
		bool file_vers_mismatch = FALSE;
		if (db->GetFileInformation(pars_id, file_id, FALSE, &file_vers_mismatch, TRUE) == TRUE)
		{
			// The database contians info about this file.
			PrepareInternal(se_point, &(db->FileData()), file_vers_mismatch);
		}
		else
		{
			// Display the numeric ids with a comment.
			PrepareInternal(se_point, pars_id, file_id, TRUE);
		}
	}
}

void TLangViewFileIdDetailsHelper::PrepareInternal(POINT &se_point, TFileInfo *fi, bool file_vers_mismatch)
{
	assert(fi != NULL);
	PrepareFormatting(se_point);
	PrepareColumnOffsets(1, 1, 13);

	SetSecondaryHeader(0, L"File Info");

	SetTitle(1, 1, L"FileId:");
	SetTitle(2, 1, L"FileName:");
	SetTitle(3, 1, L"FileLength:");
	SetTitle(4, 1, L"FileDate:");
	SetTitle(5, 1, L"FileCrc:");

	wchar_t *mism_suff = (file_vers_mismatch == TRUE) ? L" (*)" : L"";

	SetValue(1, 1, fi->PrepareFileId(m_file_id_buff, 40));
	SetValue(2, 1, FormatName(m_fname_buff, 280, fi->info.file_name));
	SetValue(3, 1, FormatLong(m_file_len_buff, 40, fi->info.file_len, mism_suff));

	if (fi->info.file_date != 0)
		FormatDateTime(fi->info.file_date, m_fdate_buff, 80, FALSE);
	else wcscpy(m_fdate_buff, L"0");

	wcscat(m_fdate_buff, mism_suff);
	SetValue(4, 1, m_fdate_buff);

	swprintf(m_fcrc_buff, 40, L"0x%0X%s", fi->info.file_crc, mism_suff);
	SetValue(5, 1, m_fcrc_buff);

	if (file_vers_mismatch == TRUE)
	{
		ShiftRowsDown(6, 1);
		SetTitleValue(6, 1, L"(*) Version of the file on disk differs from the database file info.");
	}
}

void TLangViewFileIdDetailsHelper::PrepareInternal(POINT &se_point, ID pars_id, ID file_id, bool display_no_info_comment)
{
	PrepareFormatting(se_point);
	PrepareColumnOffsets(1, 1, 7);

	SetSecondaryHeader(0, L"File Info");

	SetTitle(1, 1, L"FileId:");
	SetValue(1, 1, TFileInfo::PrepareFileId(pars_id, file_id, m_file_id_buff, 40));

	if (display_no_info_comment == TRUE)
	{
		ShiftRowsDown(2, 1);
		SetTitleValue(2, 1, L"(*) Other properties of this file are unknown.");
	}
}

// -------------------------------------------------------------------------
//    =============  TLangViewCodeLocationDetailsHelper  ================
// -------------------------------------------------------------------------

void TLangViewCodeLocationDetailsHelper::Prepare(POINT &se_point, TMidLevStoreReader *db, TSourceLocation &area, const wchar_t *secondary_section_name)
{
	PrepareFormatting(se_point);
	PrepareColumnOffsets(1, 1, 13);

	if (secondary_section_name == NULL)
		SetMainHeader(GRM_DTLS_CODE_AREA, L"Code Location Info");
	else SetSecondaryHeader(0, secondary_section_name);

	int cline = 1;
	if (area.parsing_id != 0)
	{
		// Source location belongs to some parsing session.
		SetTitle(1, 1, L"ParsingId:");
		SetTitle(2, 1, L"LinearArea:");
		SetTitle(3, 1, L"SourceRef:");
		SetValue(1, 1, FormatId(m_pars_id_buff, 40, area.parsing_id));
		SetValue(2, 1, FormatLinearArea(m_linear_area_buff, 40, area.lin_area));
		SetValue(3, 1, FormatId(m_src_ref_buff, 40, area.source_ref, L"none"));
		cline = 4;
	}

	// The code area info is displayed all the times.
	SetTitle(cline, 1, L"SourceArea:");
	wchar_t *partial_suff = (area.parsing_id != 0 && area.partial_src_area == TRUE) ? L" (*)" : L"";
	SetValue(cline, 1, FormatSourceAreaInfo(m_src_area_buff, 80, area.src_area, partial_suff));
	cline++;

	if (area.src_area.file_id != 0 && db != NULL)
	{
		TSourceAreaBodyInfo src_info;
		if (db->GetSourceAreaBodyInfo(src_info, area.src_area, NULL, TRUE) == TRUE)
		{
			// Info about the file body is available. Although it may belong to different file revision.
			// Show info about the line or lines range based on the lines info of the disk file.
			if (src_info.first_line_num == src_info.last_line_num)
			{
				SetTitle(cline, 1, L"Line:");
				SetValue(cline++, 1, FormatLong(m_lrange_buff, 40, src_info.first_line_num, partial_suff));
			}
			else
			{
				SetTitle(cline, 1, L"LinesRange:");
				swprintf(m_lrange_buff, 40, L"%ld ... %ld%s", src_info.first_line_num, src_info.last_line_num, partial_suff);
				SetValue(cline++, 1, m_lrange_buff);
			}
		}
	}

	if (area.parsing_id != 0 && area.partial_src_area == TRUE)
	{
		ShiftRowsDown(cline, 1);
		SetTitleValue(cline++, 1, L"(*) SourceArea is smaller than the LinearArea.");
	}
}

// --------------------------------------------------------------------------
//    ============  TLangViewCmdlLexemaDetailsHelper  =================
// --------------------------------------------------------------------------

void TLangViewCmdlLexemaDetailsHelper::Prepare(POINT &se_point, TCmdlLexemaInfo &lex_info)
{
	// Setup the title.
	PrepareFormatting(se_point);
	SetMainHeader(0, L"Command Line Lexema");
	PrepareColumnOffsets(1, 1, 13);

	// Section 1. Show the lexema type.
	SetTitle(1, 1, L"LexType:");
	SetValue(1, 1, TCmdlLexema::GetLexTypeName(lex_info.type, TRUE));
}

// --------------------------------------------------------------------------
//    ============  TLangViewClangLexemaDetailsHelper  =================
// --------------------------------------------------------------------------

void TLangViewClangLexemaDetailsHelper::Prepare(POINT &se_point, TLexemaInfo &lex_info)
{
	// Setup the title.
	PrepareFormatting(se_point);
	SetMainHeader(GRM_DTLS_RAW_LEXEMA, L"Raw Lexema");
	PrepareColumnOffsets(1, 1, 13);

	// Section 1. Show the type and subtype for all types of lexemas.
	SetTitle(1, 1, L"LexType:");
	SetTitle(2, 1, L"Subtype:");
	TLexSubtype subt = { lex_info.subtype };
	SetValue(1, 1, TLexema::GetLexTypeName(lex_info.type, TRUE));
	SetValue(2, 1, TLexema::GetLexSubtypeName(lex_info.type, subt));

	// Section 2. Show type/subtype specific props.
	wchar_t buff1[80], buff2[80], buff3[80];
	switch (lex_info.type)
	{
		case ltx_number:
				{
					// Show one type specific prop on the third line.
					SetTitle(3, 1, L"Value:");

					// Show dec and hex representation of the value. Formatting function will add the "0x" prefix.
					FormatInt64(lex_info.num_value, buff1, 80, fnms_dec_signed, (lex_info.num_value >= 100000 || lex_info.num_value <= -100000) ? L' ' : 0);
					FormatInt64(lex_info.num_value, buff2, 80, fnms_hex, ((unsigned __int64)lex_info.num_value >= 0x10000) ? L' ' : 0);

					// Display the unsigned form only if the value is negative.
					if (lex_info.num_value < 0)
					{
						wchar_t buff4[80];
						FormatInt64(lex_info.num_value, buff4, 80, fnms_dec_unsigned, ((unsigned __int64)lex_info.num_value >= 0x100000) ? L' ' : 0);
						swprintf(buff3, 80, L", %s (unsigned)", buff4);
					}
					else
					{
						buff3[0] = 0;
					}

					// Assemble the parts together.
					swprintf(m_buff, 200, L"%s, %s%s", buff1, buff2, buff3);
					SetValue(3, 1, m_buff);
				}
				break;

		case ltx_charconst:
				{
					// Show one type specific prop on the third line.
					SetTitle(3, 1, L"Value:");

					// Show the num value of the charconst in hex.
					FormatInt64(lex_info.num_value, m_buff, 200, fnms_hex, ((unsigned __int64)lex_info.num_value >= 0x10000) ? L' ' : 0);
					SetValue(3, 1, m_buff);
				}
				break;

		case ltx_string:
				{
					// Do not show the string itself.
					if (lex_info.str_value != NULL)
					{
						// Display the length of the string.
						SetTitle(3, 1, L"Length:");
						swprintf(m_buff, 200, L"%d character%s without trailing zero.", wcslen(lex_info.str_value), PluralSuffix(wcslen(lex_info.str_value)));
						SetValue(3, 1, m_buff);
					}
					else
					{
						SetTitle(3, 1, L"Info:");
						SetValue(3, 1, L"NULL string");
					}
				}
				break;

		case ltx_keyword:
				{
					// Show one type specific prop on the third line.
					SetTitle(3, 1, L"TokenId:");

					// Show the name of the token as it was recognized by the TLexema class.
					swprintf(m_buff, 200, L"%s (%I64d)", TLexema::GetKeywordIdEnumName((int)lex_info.num_value), lex_info.num_value);
					SetValue(3, 1, m_buff);
				}
				break;

		case ltx_name:
				{
					// Display this name in a quoted form.
					SetTitle(3, 1, L"Name:");
					SetValue(3, 1, FormatName(m_buff, 200, lex_info.str_value));
				}
				break;

		case ltx_error:
				{
					// Show the error code and the error message on the third and the forth lines.
					SetTitle(3, 1, L"ErrorCode:");
					SetTitle(4, 1, L"Message:");
					SetValue(3, 1, FormatLong(m_buff, 200, (DWORD)lex_info.num_value));
					SetValue(4, 1, lex_info.str_value);
				}
				break;
	}
}

// --------------------------------------------------------------------------
//    =============  TLangViewXmlBasedDetailsHelper  ===================
// --------------------------------------------------------------------------

void TLangViewXmlBasedDetailsHelper::PrepareParsingSessionInfo(POINT &se_point, CtvCompilationInfo &sess_header)
{
	PrepareFormatting(se_point);
	SetMainHeader(0, L"Parsing Session Info");
	PrepareColumnOffsets(1, 1, 10);

	SetTitle(1, 1, L"ParsingId:");
	SetValue(1, 1, FormatId(m_misc_buff1, 80, sess_header.parsing_id));

	// Generate the XML representation of the data.
	TXmlWriter xml_worker(L"sess_header.xml");
	xml_worker.PrepareInMemoryStream();
	TMidLevXmlHelper::DumpCtvCompilationInfo(xml_worker, 0, sess_header);

	// Display the XML representation.
	SetupXmlDescription(1, xml_worker);
}

void TLangViewXmlBasedDetailsHelper::PrepareDirectoryInfoFileInfo(POINT &se_point, TFileInfo &file_data, bool file_info_mode)
{
	PrepareFormatting(se_point);
	SetMainHeader(0, (file_info_mode == FALSE) ? L"File System Directory" : L"File Object");
	PrepareColumnOffsets(1, 1, (file_info_mode == FALSE) ? 11 : 7);

	SetTitle(1, 1, (file_info_mode == FALSE) ? L"DirectoryId:" : L"FileId:");
	SetValue(1, 1, file_data.PrepareFileId(m_misc_buff1, 80));

	// Generate the XML representation of the data.
	TXmlWriter xml_worker(L"directory_info_or_file_info.xml");
	xml_worker.PrepareInMemoryStream();
	TMidLevXmlHelper::DumpFileInfo(xml_worker, 0, file_data, file_info_mode);

	// Display the XML representation.
	SetupXmlDescription(1, xml_worker);
}

void TLangViewXmlBasedDetailsHelper::PrepareIndexingKeyInfo(POINT &se_point, TKeyInfo &key_data)
{
	PrepareFormatting(se_point);
	SetMainHeader(0, L"Indexing Key");
	PrepareColumnOffsets(1, 1, 7);

	SetTitle(1, 1, L"KeyId:");
	SetValue(1, 1, FormatId(m_misc_buff1, 80, key_data.key_id));

	// Generate the XML representation of the data.
	TXmlWriter xml_worker(L"indexing_key.xml");
	xml_worker.PrepareInMemoryStream();
	TMidLevXmlHelper::DumpIndexingKeyInfo(xml_worker, 0, key_data);

	// Display the XML representation.
	SetupXmlDescription(1, xml_worker);
}

void TLangViewXmlBasedDetailsHelper::PrepareIndexingKeyUseInfo(POINT &se_point, TKeyUseInfo &key_use_data)
{
	PrepareFormatting(se_point);
	SetMainHeader(0, L"Indexing Key Use");
	PrepareColumnOffsets(1, 1, 7);

	SetTitle(1, 1, L"KeyId:");
	SetValue(1, 1, FormatId(m_misc_buff1, 80, key_use_data.key.key_id));

	// Generate the XML representation of the data.
	TXmlWriter xml_worker(L"indexing_key_use.xml");
	xml_worker.PrepareInMemoryStream();
	TMidLevXmlHelper::DumpIndexingKeyUseInfo(xml_worker, 0, key_use_data);

	// Display the XML representation.
	SetupXmlDescription(1, xml_worker);
}

void TLangViewXmlBasedDetailsHelper::PrepareParsingEventInfo(POINT &se_point, TMidLevStoreReader *db, CtvParsingEventInfo &event)
{
	FormatEventId(m_misc_buff1, 80, event.hdr);
	int ev_tx_len = (int)(wcslen(m_misc_buff1)+7) & ~7;
	int clmn2_titles_offs = 11;

	PrepareFormatting(se_point);
	SetMainHeader(GRM_DTLS_MIDLEV_EVENT, L"Parsing Event");
	PrepareColumnOffsets(1, 1, 9);
	PrepareColumnOffsets(2, clmn2_titles_offs+ev_tx_len, clmn2_titles_offs+10+ev_tx_len);

	SetTitle(1, 1, L"EventId:");
	SetTitle(1, 2, L"EventType:");
	SetValue(1, 1, m_misc_buff1);
	SetValue(1, 2, event.GetEventTypeNameForListBox());

	// Generate the XML event representation.
	TXmlWriter xml_worker(L"parsing_event.xml");
	xml_worker.PrepareInMemoryStream();
	TMidLevXmlHelper::DumpParsingEvent(xml_worker, 0, event);

	// Display the XML representation.
	if (SetupXmlDescription(1, xml_worker) == FALSE)
		return;

	// XML data was mapped. Comment some of the events.
	if (event.hdr.event_type == pset_cond_param_use)
	{
		// Give some details on how this conditional parameter was used.
		PrepareColumnOffsets(1, 1, 10);
		SetTitle(2, 1, L"UseType:");

		CtvCondParamUseEvent &cprmu = event.cond_param;
		switch (event.cond_param.cond_param_type)
		{
			case cecit_value:		SetValue(2, 1, L"Computed value.");
								break;
			case cecit_constant:
					{
						ID cond_stmt_src_ref = 0, cond_prm_use_src_ref = 0;
						if (db != NULL)
						{
							if (db->GetSourceRefEvent(event.hdr.parsing_id, event.cond_param.cond_stmt_id) == TRUE)
								cond_stmt_src_ref = db->EventData().hdr.event_id;
							if (db->GetSourceRefEvent(event.hdr.parsing_id, event.hdr.event_id) == TRUE)
								cond_prm_use_src_ref = db->EventData().hdr.event_id;
						}

						if (cond_stmt_src_ref == 0 || cond_prm_use_src_ref == 0)
						{
							SetValue(2, 1, L"Literal constant from an unclear lex source.");
						}
						else if (cond_stmt_src_ref == cond_prm_use_src_ref)
						{
							SetValue(2, 1, L"Literal constant from the cond parsing stmt.");
						}
						else
						{
							SetValue(2, 1, L"Literal constant from the macro that was called");
							SetValue(3, 1, L"from the conditional parsing statement.");
						}
					}
					break;

			case cecit_decorated:	SetValue(2, 1, L"Value of the conditional expression parameter.");
								break;

			case cecit_name_def:	SetValue(2, 1, L"Test of the macro name definition status.");
								SetValue(3, 1, L"Information about this name is available.");
								break;
			case cecit_variable:
					{
						SetValue(2, 1, L"Value of the conditional expression parameter.");
						if (event.cond_param.decor_mdef_id == 0)
							SetValue(3, 1, L"There is no information about this name.");
						else SetValue(3, 1, L"The name is known, but it is currently undefined.");
					}
					break;

			case cecit_variab_def:	SetValue(2, 1, L"Test of the macro name definition status.");
								SetValue(3, 1, L"There is no information about this name.");
								break;

			case cecit_operation:	SetValue(2, 1, L"Operation in the conditional expression.");
								break;

			default:	SetValue(2, 1, L"Bogus conditional param type value.");
					break;
		}
	}
}

static TXmlItemPresentationRecord g_LangViewClickEventDetails_DumpCppItem_CustomTagsArray[] =
{
	{ 0, DefTag(L"Children"),	&g_XmlView_ShowLeafTagAsContainer },
	{ 0, 0, NULL, NULL },
};

void TLangViewXmlBasedDetailsHelper::PrepareCppItemInfo(POINT &se_point, TCppItemBase *item)
{
	PrepareFormatting(se_point);
	SetMainHeader(0, L"Cpp Object");
	PrepareColumnOffsets(1, 1, 8);

	if (item == NULL)
	{
		// The item is not passed.
		SetTitleValue(1, 1, L"(*) NULL Cpp item.");
		return;
	}

	if (item->ItemId() < bltin_ids_base)
		swprintf(m_misc_buff1, 80, L"%lu", item->ItemId());
	else swprintf(m_misc_buff1, 80, L"BuiltInIdsBase+%lu", item->ItemId()-bltin_ids_base);

	// Display the definition Id.
	SetTitle(1, 1, L"ItemId:");
	SetValue(1, 1, m_misc_buff1);

	// Generate the XML representation of the data.
	TXmlWriter xml_worker(L"cpp_item_info.xml");
	xml_worker.PrepareInMemoryStream();
	TCppDatabaseXmlHelper::DumpCppItem(xml_worker, 0, *item, cxdcm_none);

	// Display the XML representation.
	SetupXmlDescription(1, xml_worker, g_LangViewClickEventDetails_DumpCppItem_CustomTagsArray);
}

bool TLangViewXmlBasedDetailsHelper::SetupXmlDescription(int lines_above_xml_item, TDestinationFile &mem_buffer, TXmlItemPresentationRecord *special_tags_dict)
{
	// This class is a template instantiation. Template params, that were used during the instantiation
	// are not directly accesible. Use length of the array in the data field to get the number of rows.
	int num_rows = sizeof(m_baseline_offs)/sizeof(long);
	assert(lines_above_xml_item >= 0 && lines_above_xml_item <= num_rows);

	mem_buffer.Close();
	if (mem_buffer.GetErrorFlag() == TRUE)
	{
		// This is either an initial memory allocation error or a data buffering error.
		ShiftRowsDown(lines_above_xml_item+1, 1);
		SetTitleValue(lines_above_xml_item+1, 1, L"(*) Error preparing XML data.");
		return(FALSE);
	}

	// The data buffer is fine.
	long body_len = mem_buffer.GetCurrLen();
	wchar_t *body = mem_buffer.ExtractMemBody();
	assert(body != NULL && body_len >= 0);

	// Get rid of the extra CrLf at the end of the XML text if any.
	if (body_len >= 2 && body[body_len-1] == L'\n' && body[body_len-2] == L'\r')
		body_len -= 2;

	long y_pos = m_fields_ref.y+6;
	if (lines_above_xml_item > 0)
	{
		y_pos += m_baseline_offs[lines_above_xml_item-1];
	}

	// Setup and display the XML data.
	m_xml_repr.SetStyle(&m_stls->m_xml_repr_style);
	m_xml_repr.SetupXmlFile(&m_stls->m_scr_items_view, special_tags_dict, body, body_len, dbm_take_ownsh);
	m_xml_repr.SetHotSpotPos(m_fields_ref.x, y_pos+3);
	m_stls->m_scr_items_view.AddToInternalDataSet(m_xml_repr);

	// Map one pixel high rect on top of the XML view.
	RECT xml_repr_rect = *m_xml_repr.GetBoundingRect();
	xml_repr_rect.right -= m_xml_repr.Style()->Props().shadow_down_width;
	xml_repr_rect.bottom = xml_repr_rect.top+1;
	m_upper_delim.SetStyle(&m_stls->m_xml_upper_border_style);
	m_upper_delim.SetBaseRect(xml_repr_rect);
	m_stls->m_scr_items_view.AddToInternalDataSet(m_upper_delim);

	// Shift the rows below the XML view down.
	ShiftRowsDown(lines_above_xml_item+1, m_xml_repr.GetHeight()+10);

	// Update the vert marker.
	y_pos += m_xml_repr.GetHeight()+12;
	if (y_pos > *m_ymax)
		*m_ymax = y_pos;

	m_helper_prepared = TRUE;
	return(TRUE);
}

// ----------------------------------------------------------------------------
//    ==============  TLangViewClickEventDetailsDialog  ===================
// ----------------------------------------------------------------------------

static const TTextStyleProps g_LangViewClickEventDetails_FontProps[4] =
{
	{  NULL,	HELVETICA,			12,	TRUE,	RGB(80, 80, 80),		STD_TXT },		// Section title.
	{  NULL,	HELVETICA,			10,	TRUE,	RGB(80, 80, 80),		STD_TXT },		// Section subtitle.
	{  NULL,	MS_SANS_SERIF,		 8,	FALSE,	RGB(128, 128, 128),	STD_TXT },		// Field title.
	{  NULL,	VERDANA,			 8,	FALSE,	RGB(0, 0, 0),			STD_TXT },		// Field value.
};

static const TTextStyleSymbolAdjustInfo g_LangViewClickEventDetails_PrimaryHeader_Adjustments[] =
{
	{ L't', 0, 1, 2 }, { L'w', 0, 1, 1 }, { 0, 0, 0, 0 },
};

static const TTextStyleSymbolAdjustInfo g_LangViewClickEventDetails_SecondaryHeader_Adjustments[] =
{
	{ L's', 0, 1, 2 }, { 0, 0, 0, 0 },
};

static const TTextStyleSymbolAdjustInfo g_LangViewClickEventDetails_FieldValue_Adjustments[] =
{
	{ L'.', 0, 0, 1 }, { 0, 0, 0, 0 },
};

static const TControlPosInfo g_LangViewClickEventDetails_DialogCtrls[] =
{
	IDC_LVEDT_TITLE,			cpm_fix_top_side,
	IDC_LVEDT_BODY,			cpm_fix_all_corners,
	0, cpm_none,
};

static const TMenuItemInfo g_LangViewClickEventDetails_ContextMenu[] =
{
	{ 0, 2740,	L"Copy to clipboard as XML Text"		},	// 0
	{ 0, 2741,	L"Copy to clipboard as Html Page"	},	// 1
	// { 0, 2742,	L"Copy to clipboard as Html <div>"	},	// 2		<-- After uncommenting this line it is necessary to update the full/short menu selection code.
	{ 0, 2743,	L"Copy to clipboard as Bitmap"		},	// 3
	{ 0, 0, NULL },									// 4
};

static const wchar_t *g_LangViewClickEventDetails_CssStyleFiles[] =
{
	L"Wbs_XmlDataHighlight.CSS",
	NULL
};

TLangViewClickEventDetailsDialog::TLangViewClickEventDetailsDialog(TMidLevStoreReader *db_link)
		: TLangViewChildBrowser(db_link, cbrt_event_details, IDD_LANGV_SVF_EVENT_DETAILS)
{
	m_contents.Clear();
	m_bogus_evt_type_buff[0] = 0;
}

LRESULT TLangViewClickEventDetailsDialog::OnInitDialog(HWND hDefaultFocusControl)
{
	AdjustControlRightSide(IDC_LVEDT_BODY);
	SetupControlsListPositioning(g_LangViewClickEventDetails_DialogCtrls);
	SetMinTrackingSize(50, 20);

	// Prepare the screen items control. Use special frame props that do not exclude the left-top
	// empty space but add small vertical margin at the bottom.
	m_scr_items_view.SetWindow(::GetDlgItem(m_hDlg, IDC_LVEDT_BODY));
	TScreenItemsViewFrameProps props;
	props.SetStndDocStyleProps(FALSE, TRUE, RGB(252, 255, 252));
	props.vert_margin = 10;
	m_scr_items_view.SetProperties(props, FALSE);

	// Prepare styles for all smaller dtls helpers.
	if (PrepareStyles() == FALSE)
		return(-1);

	// Pass the styles info into the helpers.
	m_hdr_only_dtls.SetStyles(this);
	m_file_info_dtls.SetStyles(this);
	m_code_loc_dtls.SetStyles(this);
	m_cmdl_lexema_dtls.SetStyles(this);
	m_clang_lexema_dtls.SetStyles(this);
	m_xml_based_dtls.SetStyles(this);

	SetFocusControl(IDC_LVEDT_BODY);
	return(FALSE);
}

void TLangViewClickEventDetailsDialog::OnWmCommand(short controlId, short notificationCode, HWND hWndControl)
{
	if (controlId == IDC_LVEDT_BODY && notificationCode == xiv_left_btn_down)
	{
		// Save color under the mouse pointer to support the color editor.
		m_scr_items_view.SaveClickedPointColor((TScreenItemsViewMouseNotify*)hWndControl);
	}
	else if (controlId == IDC_LVEDT_BODY && notificationCode == xiv_right_btn_down)
	{
		if (m_contents.type != clet_none)
		{
			// The viewer is not empty. Show the context menu.
			const TMenuItemInfo *menu_info = g_LangViewClickEventDetails_ContextMenu;
			if (m_xml_based_dtls.m_helper_prepared == FALSE)
				menu_info += 2;

			TScreenItemsViewMouseNotify *mntf = (TScreenItemsViewMouseNotify*)hWndControl;
			DWORD res = m_scr_items_view.ShowAndTrackPopupMenu(menu_info, mntf->click_place.x, mntf->click_place.y);
			if (res == 2740)
			{
				// Pick up the contents of the decorated file viewer as a simple text.
				TDestinationFile rprt(L"event_details_info1");
				rprt.PrepareInMemoryStream();
				bool gen_res = m_xml_based_dtls.m_xml_repr.GetXmlData(rprt);
				ProcessReportData(gen_res, rprt);
			}
			else if (res == 2741)
			{
				// Get HTML representation as a complete page.
				THtmlDocument rprt(L"event_details_info2", FALSE);
				rprt.PrepareInMemoryStream();
				bool gen_res = m_xml_based_dtls.m_xml_repr.GenerateBasicHtmlPage(rprt, NULL, L"C:\\Ballerup\\Calvados\\Grammars\\CSS", g_LangViewClickEventDetails_CssStyleFiles);
				ProcessReportData(gen_res, rprt);
			}
			else if (res == 2742)
			{
				// Get HTML representation as a <div> .. </div> fragment.
				THtmlDocument rprt(L"event_details_info3", TRUE);
				rprt.PrepareInMemoryStream();
				bool gen_res = m_xml_based_dtls.m_xml_repr.GenerateHtmlTable(rprt);
				ProcessReportData(gen_res, rprt);
			}
			else if (res == 2743)
			{
				// It is necessary to generate the bitmap image.
				RECT margins = { 12, 8, 12, 14 };
				PutScreenShotToClipboard(m_scr_items_view, &margins);
			}
		}
	}
}

void TLangViewClickEventDetailsDialog::LoadContents(TLangViewClickEvent &event)
{
	ResetContents();
	POINT cursor = { 8, 3 };

	// Show the event specific info.
	switch (event.type)
	{
		case clet_none:
				m_hdr_only_dtls.Prepare(cursor, L"  Empty Click Event  ");
				break;

		//
		//	Group 1.
		//

		case clet_pars_session:
				m_xml_based_dtls.PrepareParsingSessionInfo(cursor, event.pars_sess_data);
				break;

		case clet_filesys_directory:
				m_xml_based_dtls.PrepareDirectoryInfoFileInfo(cursor, event.file_data, FALSE);
				break;

		case clet_file_object:
				m_xml_based_dtls.PrepareDirectoryInfoFileInfo(cursor, event.file_data, TRUE);
				break;

		case clet_indexing_key:
				m_xml_based_dtls.PrepareIndexingKeyInfo(cursor, event.key_data);
				break;

		//
		//	Group 2.
		//

		case clet_code_area:
				m_code_loc_dtls.Prepare(cursor, m_db, event.code_area);
				m_file_info_dtls.Prepare(cursor, m_db, event.code_area.src_area.pars_id, event.code_area.src_area.file_id);
				break;

		case clet_indexing_key_use:
				m_xml_based_dtls.PrepareIndexingKeyUseInfo(cursor, event.key_use_data);
				break;

		case clet_cmdl_lexema:
				m_cmdl_lexema_dtls.Prepare(cursor, event.cmdl_lexema);
				break;

		case clet_clang_lexema:
				m_clang_lexema_dtls.Prepare(cursor, event.clang_lexema);
				m_code_loc_dtls.Prepare(cursor, m_db, event.clang_lexema.origin, L"Lexema origin");
				break;

		//
		//	Group 3.
		//

		case clet_mid_lev_event:
				m_xml_based_dtls.PrepareParsingEventInfo(cursor, m_db, event.mldb_event_data);
				break;

		case clet_grbr_object:
				m_hdr_only_dtls.Prepare(cursor, L"Grammar Object");
				break;

		case clet_grm_pars_symbol:
				m_hdr_only_dtls.Prepare(cursor, L"Grammar Based Parsing Symbol");
				break;

		case clet_cpp_object:
				m_xml_based_dtls.PrepareCppItemInfo(cursor, event.cpp_item);
				break;

		//
		//	Group 4.
		//

		default:
			swprintf(m_bogus_evt_type_buff, 80, L"Unsupported Click Event Type (%d)", (int)event.type);
			m_hdr_only_dtls.Prepare(cursor, m_bogus_evt_type_buff);
			break;
	}

	// Copy in the data.
	m_contents = event;

	// Display the results.
	m_scr_items_view.ShowInternalDataSet(FALSE);
}

void TLangViewClickEventDetailsDialog::ResetContents()
{
	// Clear contents of the screen items view if any.
	if (m_scr_items_view.CurrDataSet() != NULL)
		m_scr_items_view.CurrDataSet()->Clear();

	m_scr_items_view.RepaintControl();

	m_contents.Clear();
	m_bogus_evt_type_buff[0] = 0;

	m_xml_based_dtls.m_helper_prepared = FALSE;
}

bool TLangViewClickEventDetailsDialog::PrepareStyles()
{
	HDC hDC = ::GetDC(m_hDlg);
	assert(hDC != NULL);
	bool fsucc = TRUE;

	// Section header icons.
	fsucc &= m_icon_picts.Setup(NULL, g_LangViewClickEventDetails_IconsProps);
	fsucc &= m_icon_picts.SetupForDeviceContext(hDC);

	// Styles for rect delimiters.
	TBasicStyleProps bs_props;
	bs_props.Init(RGB(96, 96, 96));
	m_sect_delim_style.Setup(bs_props);
	bs_props.Init(RGB(236, 236, 236));
	m_xml_upper_border_style.Setup(bs_props);

	fsucc &= m_sect_delim_style.SetupForDeviceContext(hDC);
	fsucc &= m_xml_upper_border_style.SetupForDeviceContext(hDC);

	// Text fonts.
	m_section_title_style.Setup(g_LangViewClickEventDetails_FontProps[0], g_LangViewClickEventDetails_PrimaryHeader_Adjustments);
	m_section_subtitle_style.Setup(g_LangViewClickEventDetails_FontProps[1], g_LangViewClickEventDetails_SecondaryHeader_Adjustments);
	m_field_title_style.Setup(g_LangViewClickEventDetails_FontProps[2], NULL);
	m_field_value_style.Setup(g_LangViewClickEventDetails_FontProps[3], g_LangViewClickEventDetails_FieldValue_Adjustments);

	fsucc &= m_section_title_style.SetupForDeviceContext(hDC);
	fsucc &= m_section_subtitle_style.SetupForDeviceContext(hDC);
	fsucc &= m_field_title_style.SetupForDeviceContext(hDC);
	fsucc &= m_field_value_style.SetupForDeviceContext(hDC);

	// XML representation.
	if (m_xml_repr_style.CheckXmlStylePrepared() == FALSE)
		fsucc &= m_xml_repr_style.SetupDefaultStyles();
	fsucc &= m_xml_repr_style.SetupForDeviceContext(hDC);

	::ReleaseDC(m_hDlg, hDC);
	return(fsucc);
}

void TLangViewClickEventDetailsDialog::ProcessReportData(bool gen_res, TDestinationFile &rprt)
{
	rprt.Close();
	if (gen_res == FALSE || rprt.GetErrorFlag() == TRUE)
	{
		MsgBox(L"Error generating the text representation.");
	}
	else
	{
		// The generated data is fine.
		DWORD err_code = TControlHelper::PutStringToClipboard(rprt.GetBodyPtr());
		if (err_code != ERROR_SUCCESS)
			MsgBoxFmt(MB_OK, L"Error putting text data into the clipboard (Win32 error: %d).", err_code);
	}
}


