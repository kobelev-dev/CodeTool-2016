//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>
#include  <stdarg.h>

#pragma hdrstop

#include  "Common/FormatNumber.H"
#include  "Xml/XmlReader.H"

// #define DEBUG_XML_GET_LEXEMA

// -----------------------------------------------------------------
//   ==================  TXmlStream  =====================
// -----------------------------------------------------------------

#ifdef DEBUG_XML_GET_LEXEMA
static wchar_t *g_XmlStream_LexTypeNames[] =
{
	L"empty",
	L"xml_header",
	L"xml_comment",
	L"tag_attr",
	L"data_tag",
	L"cnr_beg",
	L"cnr_body",
	L"cnr_end",
	L"EOF",
	L"xml_error",
};
#endif

TXmlStream::TXmlStream(const wchar_t *fname, const wchar_t *buffer, int buff_len)
{
	assert(fname != NULL && fname[0] != 0);

	m_stream_name = (wchar_t*)fname;
	m_buff = (wchar_t*)buffer;
	m_buff_len = (buffer != NULL) ? ((buff_len >= 0) ? buff_len : (int)wcslen(buffer)) : 0;
	m_buff_owned = FALSE;

	m_offs = 0;
	m_line_num_at_offs = 1;

	m_tag_hdr_state = FALSE;
	m_cnt_buffered_lexemas = 0;
	m_parser = NULL;
}

TXmlStream::~TXmlStream()
{
	if (m_buff != NULL && m_buff_owned == TRUE)
	{
		free(m_buff);
	}
}

void TXmlStream::SetParser(TXmlReader *parser)
{
	// Establish the bidirectional link.
	m_parser = parser;
	m_parser->SetStream(this);
}

TDateTime TXmlStream::GetStreamFileDate() const
{
	WIN32_FILE_ATTRIBUTE_DATA file_attrs;
	if (::GetFileAttributesExW(m_stream_name, GetFileExInfoStandard, &file_attrs) == 0)
	{
		// The object was instantiated with the memory stream or the file on the disk is missing or there is
		// some other problem. For simplicity reduce all these numerous cases to just the file missing case.
		DWORD error = ::GetLastError();
		return(-1);
	}

	// The date of the file was retrieved.
	return(MakeDateTime(file_attrs.ftLastWriteTime));
}

bool TXmlStream::SkipXmlHeader()
{
	// Skip the white spaces if any first.
	if (SkipXmlSpaces(TRUE) == FALSE)
	{
		// The file is empty.
		m_parser->ReportXmlError(xrde_xhdr_e1, m_offs, 0, m_line_num_at_offs, L"The Xml start tag is missing. (The file is empty).");
		return(FALSE);
	}

	if ((m_buff_len-m_offs) >= 2 && wcsncmp(m_buff+m_offs, L"<?", 2) == 0)
	{
		// This is beginning of the XML header.
		m_parser->ReportXmlArea(xfat_xml_header_frame, m_offs, 2);
		m_offs += 2;

		int header_body_beg_offs = m_offs, header_body_end_offs = -1;
		while (m_offs < m_buff_len)
		{
			if (m_buff[m_offs] == L'?' && (m_buff_len-m_offs) >= 2 && wcsncmp(m_buff+m_offs, L"?>", 2) == 0)
			{
				// The end of the XML header is found.
				header_body_end_offs = m_offs;
				m_offs += 2;
				break;
			}

			if (m_buff[m_offs] == L'\n')
				m_line_num_at_offs++;

			// Accept the current char as part of the XML header.
			m_offs++;
		}

		int actual_body_end_offs = (header_body_end_offs > 0) ? header_body_end_offs : m_offs;
		if (header_body_beg_offs < actual_body_end_offs)
		{
			// The body of the XML header is not empty.
			m_parser->ReportXmlArea(xfat_xml_header_body, header_body_beg_offs, actual_body_end_offs-header_body_beg_offs);
		}

		// Check presence of the XML header closing sequence.
		if (header_body_end_offs > 0)
		{
			m_parser->ReportXmlArea(xfat_xml_header_frame, header_body_end_offs, 2);
		}
		else
		{
			m_parser->ReportXmlError(xrde_xhdr_e2, header_body_beg_offs, m_offs-header_body_beg_offs, m_line_num_at_offs, L"End of file inside the XML header.");
			return(FALSE);
		}

		// Skip spaces after the XML header.
		if (SkipXmlSpaces(TRUE) == FALSE)
		{
			m_parser->ReportXmlError(xrde_xhdr_e3, m_offs, 0, m_line_num_at_offs, L"The Xml body start tag is missing. (The file is empty).");
			return(FALSE);
		}
	}

	// Check if data in the file starts with the beginning of the tag or not.
	if (m_buff[m_offs] != L'<')
	{
		m_parser->ReportXmlError(xrde_xhdr_e4, m_offs, 1, m_line_num_at_offs, L"Data in the file does not start with the opening of XML tag.");
		return(FALSE);
	}

	// Everything is fine with the XML header.
	return(TRUE);
}

TXmlLexemaType TXmlStream::GetXmlLexema(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end)
{
#ifdef DEBUG_XML_GET_LEXEMA
	int ltx_beg_offs = m_offs;
#endif

	// The whole work is done in one of the 2 major functions.
	TXmlLexemaType lt = (m_tag_hdr_state == TRUE) ?
							GetLexFromTagHeader(tag_name, tag_value, tag_area_ex, line_num_at_lex_end) :
							GetLexFromContainer(tag_name, tag_value, tag_area_ex, line_num_at_lex_end);

#ifdef DEBUG_XML_GET_LEXEMA
	// Debug trace.
	wprintf(L"=+Lex+= lex_type=(%s), \tname=\"%.*s\", \tvalue=\"%.*s\", \tstt_after=%s, ltx_beg_offs=0x%x.\n",
			g_XmlStream_LexTypeNames[lt], tag_name.GetLength(), (tag_name.GetLength() > 0) ? tag_name.GetBodyPtr() : L"",
			tag_value.GetLength(), (tag_value.GetLength() > 0) ? tag_value.GetBodyPtr() : L"",
			(m_tag_hdr_state == TRUE) ? L"(tag_hdr)" : L"(norm)", ltx_beg_offs);
#endif

	return(lt);
}

bool TXmlStream::LoadXmlFileFromDisk()
{
	if (m_buff != NULL && m_buff_owned == TRUE)
	{
		// Release the old buffer.
		free(m_buff);
	}

	m_buff = NULL;
	m_buff_len = 0;
	m_buff_owned = FALSE;

	TBasicFileInfo info;
	TLoadSaveResult res = TFileBodyHelper::LoadAsciiFileBody(info,				// Structure to fill.
											m_stream_name, NULL,	TRUE,	// Name of the file to load, use_real_disk_fname == TRUE.
											FALSE);						// Do not allow empty files loading (This is NON DEFAULT VALUE).
	if (res != ldres_success)
	{
		// This error is not related to any place in the source file.
		m_parser->ReportXmlError(xrde_fldr_e1, -1, -1, -1, L"Error loading the source file: %s.", TFileBodyHelper::GetLoadSaveResultText(res));
		return(FALSE);
	}

	// Success.
	m_buff = info.file_body;
	m_buff_len = info.file_len;
	m_buff_owned = TRUE;
	info.file_body = NULL;

	info.FreeBasicFileInfo();
	return(TRUE);
}

TXmlLexemaType TXmlStream::GetLexFromTagHeader(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end)
{
	assert(m_tag_hdr_state == TRUE);

	// Pick up the low level lexema.
	long lex_beg_offs = m_offs;
	TXmlTagHeaderLex llt = GetTagHeaderLex(tag_name, line_num_at_lex_end);
	switch (llt)
	{
		case xhl_attr_name:
				{
					// This should be the beginning of the tag attr.
					TStringPtr lex_value;
					if (GetTagHeaderLex(lex_value, line_num_at_lex_end) != xhl_equ)
					{
						m_parser->ReportXmlError(xrde_lftg_e1, lex_value, line_num_at_lex_end, L"Equal sign after the name of the tag attribute is missing.");
						tag_name = lex_value;
						return(xlt_error);
					}
					else if (GetTagHeaderLex(tag_value, line_num_at_lex_end) != xhl_attr_val)
					{
						m_parser->ReportXmlError(xrde_lftg_e2, tag_value, line_num_at_lex_end, L"Tag attribute value is missing after the attr name and the equal sign.");
						tag_name = tag_value;
						return(xlt_error);
					}

					// All parts of the tag attribute were retrieved.
					tag_area_ex.SetData(tag_value.GetBodyPtr()-1, tag_value.GetLength()+2);
					return(xlt_tag_attr);
				}

		case xhl_equ:
		case xhl_attr_val:
				{
					// Wrong syntax in the tag header. The tag attribute cannot begin with the equal sign or the qouted string.
					m_parser->ReportXmlError(xrde_lftg_e3, tag_name, line_num_at_lex_end, L"The name of the tag attribute is missing.");
					return(xlt_error);
				}

		case xhl_end1:
				{
					// End of the tag header. Switch to the container body mode.
					m_tag_hdr_state = FALSE;
					return(GetLexFromContainer(tag_name, tag_value, tag_area_ex, line_num_at_lex_end));
				}

		case xhl_end2:
				{
					// End of the tag header and an end of the tag at the same time. Note that each tag with attributes
					// is always a container. Due to this this is an end of the container, not just an end of the tag header.
					m_tag_hdr_state = FALSE;
					tag_name.AdjustToStringEnd();
					return(xlt_cnr_end);
				}

		case xhl_eof:
				{
					// Current pos should be equal to the end of the stream. Ensure this.
					assert(m_offs == m_buff_len);
					return(xlt_eof);
				}

		case xhl_error:
				{
					// The low level scanner should advance the offset for at least one char. Ensure this.
					assert(m_offs > lex_beg_offs);
					return(xlt_error);
				}
	}

	// Unknown type of the low level lexema.
	return(ReportBadLowLevelLexemaType(llt, tag_name));
}

TXmlLexemaType TXmlStream::GetLexFromContainer(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end)
{
	assert(m_tag_hdr_state == FALSE);

	// Pick up the next lexema from the stream.
	long lex_beg_offs = m_offs;
	TXmlContainerLex llt = GetContainerLex(tag_name, line_num_at_lex_end);
	switch (llt)
	{
		case xcl_tag_open:
				// This case needs further investigation.
				return(ProcessTagHdrOpen(tag_name, tag_value, tag_area_ex, line_num_at_lex_end));

		case xcl_inner_tag:
				// This case needs further investigation.
				return(ProcessInnerTag(tag_name, tag_value, tag_area_ex, line_num_at_lex_end));

		case xcl_empty_tag:
				// Convert this low level lex into the higher level lexema.
				tag_value.Clear();
				tag_area_ex.Clear();
				return(xlt_leaf_tag);

		case xcl_short_end:
				// Convert this low level lex into the higher level lexema.
				tag_name.AdjustToStringEnd();
				return(xlt_cnr_end);

		case xcl_named_end:
				// Convert this low level lex into the higher level lexema.
				return(xlt_cnr_end);

		case xcl_body_text:
				// Convert this low level lex into the higher level lexema.
				tag_value = tag_name;
				return(xlt_cnr_body);

		case xcl_eof:
				// End of the object should be equal to the end of the stream.
				assert(m_offs == m_buff_len);
				return(xlt_eof);

		case xcl_error:
				// Low level scanner should advance ptr at least for one char.
				return(xlt_error);
	}

	// Unknown type of the low level lexema.
	return(ReportBadLowLevelLexemaType(llt, tag_name));
}

TXmlLexemaType TXmlStream::ProcessTagHdrOpen(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end)
{
	// This is beginning of a complex tag. Name of the container is passed in the tag_name param. Check the next lexema.
	m_tag_hdr_state = TRUE;

	TStringPtr lex_value;
	int lex_value_line_num;
	TXmlTagHeaderLex llt = GetTagHeaderLex(lex_value, lex_value_line_num);
	switch (llt)
	{
		case xhl_end1:
				// End of the tag header. Tag attributes and nested tags are considered to be on the same nesting layer
				// in the current implementation. This means that end of the tag header is not changing the nesting layer.
				// It only changes the formatting rules. Switch to the container body mode.
				m_tag_hdr_state = FALSE;
				return(ProcessInnerTag(tag_name, tag_value, tag_area_ex, line_num_at_lex_end));

		case xhl_end2:
				// End of the header and end of the tag at the same time.
				m_tag_hdr_state = FALSE;
				tag_value.Clear();
				tag_area_ex.Clear();
				line_num_at_lex_end = lex_value_line_num;
				return(xlt_leaf_tag);
	}

	// Accept just the beginning of the container.
	BufferLowLevelTagHeaderLex(llt, lex_value, lex_value_line_num);
	return(xlt_cnr_beg);
}

TXmlLexemaType TXmlStream::ProcessInnerTag(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end)
{
	// It is necessary to figure out if this is a simple tag or a beginning of the container.
	TStringPtr lex_value;
	int lex_value_line_num;
	TXmlContainerLex llt = GetContainerLex(lex_value, lex_value_line_num);
	switch (llt)
	{
		case xcl_short_end:
				{
					// This is an empty field lexema.
					tag_value.Clear();
					tag_area_ex.Clear();
					line_num_at_lex_end = lex_value_line_num;
					return(xlt_leaf_tag);
				}

		case xcl_named_end:
				{
					// Check if the opening and closing names are the same.
					line_num_at_lex_end = lex_value_line_num;
					if (tag_name != lex_value)
					{
						// Names are different.
						m_parser->ReportXmlError(xrde_pitg_e1, lex_value, lex_value_line_num, L"The name of the closing tag is different from the name of the opening tag.");
						tag_name = lex_value;
						return(xlt_error);
					}

					// This is an empty field lexema.
					tag_value.Clear();
					tag_area_ex = lex_value;
					return(xlt_leaf_tag);
				}

		case xcl_body_text:
				{
					// Tag body is found after the tag name. Check, if there is a closing tag after the body or not.
					TStringPtr next_lex_value;
					int next_lex_value_line_num;
					TXmlContainerLex lltn = GetContainerLex(next_lex_value, next_lex_value_line_num);
					if (lltn == xcl_short_end)
					{
						tag_value = lex_value;
						tag_area_ex.Clear();
						line_num_at_lex_end = next_lex_value_line_num;
						return(xlt_leaf_tag);
					}
					else if (lltn == xcl_named_end)
					{
						line_num_at_lex_end = next_lex_value_line_num;
						if (tag_name != next_lex_value)
						{
							// The names are different.
							m_parser->ReportXmlError(xrde_pitg_e2, next_lex_value, next_lex_value_line_num, L"The name of the closing tag differs from the name of the opening tag.");
							tag_name = next_lex_value;
							return(xlt_error);
						}

						// Closing tag is found after the body. Save the beginning of the tag value for future use.
						tag_value = lex_value;
						tag_area_ex = next_lex_value;
						return(xlt_leaf_tag);
					}

					// Accept just the beginning of the container.
					BufferLowLevelContainerLex(llt, lex_value, lex_value_line_num);
					BufferLowLevelContainerLex(lltn, next_lex_value, next_lex_value_line_num);
					return(xlt_cnr_beg);
				}
	}

	// Accept just the beginning of the container.
	BufferLowLevelContainerLex(llt, lex_value, lex_value_line_num);
	return(xlt_cnr_beg);
}

TXmlStream::TXmlTagHeaderLex TXmlStream::GetTagHeaderLex(TStringPtr &lex_value, int &lex_end_line_num)
{
	assert(m_tag_hdr_state == TRUE);
	if (m_cnt_buffered_lexemas > 0)
	{
		// Prescanned lexema is available. Current design expects that only one lexema can be buffered.
		assert(m_cnt_buffered_lexemas == 1);
		m_cnt_buffered_lexemas = 0;

		lex_value = m_buffered_lexemas[0].m_lex_value;
		lex_end_line_num = m_buffered_lexemas[0].m_lex_line_num;
		return(m_buffered_lexemas[0].m_hdr_lex_type);
	}

	// Skip the spaces if any.
	bool data_present = SkipXmlSpaces(FALSE);

	// Tag header lexemas cannot span over several lines. This means that the line number is already known.
	lex_end_line_num = m_line_num_at_offs;

	if (data_present == FALSE)
	{
		// Nothing was found before the end of the file.
		lex_value.SetData(m_buff+m_buff_len, 0);
		return(xhl_eof);
	}

	// Something is still present in the input buffer.
	long lex_beg_offs = m_offs;
	wchar_t ch = m_buff[m_offs];
	if (ch == L'>')
	{
		// This is an end of the tag header.
		lex_value.SetData(m_buff+lex_beg_offs, 1);
		m_parser->ReportXmlArea(xfat_delim_tag_frame, lex_value);
		m_offs++;
		return(xhl_end1);
	}
	else if (ch == L'/')
	{
		if (++m_offs >= m_buff_len)
		{
			lex_value.SetData(m_buff+m_buff_len, 0);
			m_parser->ReportXmlError(xrde_snth_e1, lex_value, m_line_num_at_offs, L"Unexpected EOF while scanning the tag header.");
			return(xhl_error);
		}
		else if (m_buff[m_offs] == L'>')
		{
			// End of the tag header and tag.
			lex_value.SetData(m_buff+lex_beg_offs, 2);
			m_parser->ReportXmlArea(xfat_delim_tag_frame, lex_value);
			m_offs++;
			return(xhl_end2);
		}
		else
		{
			// Bogus situation.
			lex_value.SetData(m_buff+lex_beg_offs, 1);
			m_parser->ReportXmlError(xrde_snth_e2, lex_value, m_line_num_at_offs, L"The slash symbol (\"/\") is not immediately followed by the \">\" symbol in the tag header.");
			return(xhl_error);
		}
	}
	else if (ch == L'=')
	{
		// Equal sign the as the low level lexema.
		lex_value.SetData(m_buff+lex_beg_offs, 1);
		m_parser->ReportXmlArea(xfat_delim_tag_hdr_equ, lex_value);
		m_offs++;
		return(xhl_equ);
	}
	else if (ch == L'\'' || ch == L'\"')
	{
		// This is beginning of a quoted string.
		m_offs++;
		if (ScanQuotedString(ch, lex_value) == FALSE)
			return(xhl_error);

		// Quoted string was successfully extracted.
		return(xhl_attr_val);
	}
	else
	{
		// This should be the name of the tag attribute.
		if (ScanName(lex_value, TRUE) == FALSE)
			return(xhl_error);

		// Name of the attr was succesfully taken out.
		return(xhl_attr_name);
	}
}

TXmlStream::TXmlContainerLex TXmlStream::GetContainerLex(TStringPtr &lex_value, int &lex_end_line_num)
{
	assert(m_tag_hdr_state == FALSE);
	if (m_cnt_buffered_lexemas > 0)
	{
		// Prescanned lexema is available. Only one or two lexemas can be buffered.
		assert(m_cnt_buffered_lexemas == 1 || m_cnt_buffered_lexemas == 2);
		m_cnt_buffered_lexemas--;

		lex_value = m_buffered_lexemas[0].m_lex_value;
		lex_end_line_num = m_buffered_lexemas[0].m_lex_line_num;
		TXmlContainerLex lext = m_buffered_lexemas[0].m_cnr_lex_type;

		if (m_cnt_buffered_lexemas > 0)
			m_buffered_lexemas[0] = m_buffered_lexemas[1];

		return(lext);
	}

	// Skip the spaces if any.
	bool data_present = SkipXmlSpaces(TRUE);

	// In many cases lexema do not span over several lines. Prefill the line number.
	lex_end_line_num = m_line_num_at_offs;

	if (data_present == FALSE)
	{
		// Nothing was found before the end of the file.
		lex_value.SetData(m_buff+m_buff_len, 0);
		return(xcl_eof);
	}

	// Something is still present in the stream.
	int offs_beg = m_offs;
	if (m_buff[m_offs] == L'<')
	{
		//
		//  This is beginnig of the leaf tag or container.
		//
		if (++m_offs >= m_buff_len)
		{
			return(UnexpectedEofInsideTheContainer(offs_beg, lex_value));
		}

		if (m_buff[m_offs] != L'/')
		{
			// This is some sort of an opening tag.
			if (ScanName(lex_value, FALSE) == FALSE)
				return(xcl_error);

			// Name was succesfully taken out.
			if (m_offs >= m_buff_len)
				return(UnexpectedEofInsideTheContainer(offs_beg, lex_value));

			wchar_t ch = m_buff[m_offs];
			if (ch == L'>')
			{
				// Simple tag.
				m_parser->ReportXmlArea(xfat_delim_tag_frame, offs_beg, 1);
				m_parser->ReportXmlArea(xfat_delim_tag_frame, m_offs, 1);
				m_offs++;
				return(xcl_inner_tag);
			}
			else if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n')
			{
				// This is beginning of the tag with tag attributes.
				m_parser->ReportXmlArea(xfat_delim_tag_frame, offs_beg, 1);
				m_offs++;
				return(xcl_tag_open);
			}
			else if (ch == L'/')
			{
				// This looks to be an empty tag.
				if (++m_offs >= m_buff_len)
				{
					return(UnexpectedEofInsideTheContainer(offs_beg, lex_value));
				}
				else if (m_buff[m_offs] == L'>')
				{
					// This is simple closing tag.
					m_parser->ReportXmlArea(xfat_delim_tag_frame, offs_beg, 1);
					m_parser->ReportXmlArea(xfat_delim_tag_frame, m_offs-1, 2);
					m_offs++;
					return(xcl_empty_tag);
				}
			}

			// Bogus combination of chars.
			m_parser->ReportXmlError(xrde_sncl_e1, m_offs, 1, m_line_num_at_offs, L"Wrong character after the tag name in the opening tag.");
			m_offs++;
			return(xcl_error);
		}
		else
		{
			// This is some sort of the closing tag.
			if (++m_offs >= m_buff_len)
			{
				return(UnexpectedEofInsideTheContainer(offs_beg, lex_value));
			}
			else if (m_buff[m_offs] == L'>')
			{
				// This is empty closing tag.
				m_parser->ReportXmlArea(xfat_delim_tag_frame, offs_beg, 3);
				m_offs++;
				return(xcl_short_end);
			}

			// This is closing tag with a name.
			if (ScanName(lex_value, FALSE) == FALSE)
				return(xcl_error);

			// Name was succesfully taken out.
			if (m_offs >= m_buff_len)
			{
				return(UnexpectedEofInsideTheContainer(offs_beg, lex_value));
			}
			else if (m_buff[m_offs] == L'>')
			{
				// Named closing tag.
				m_parser->ReportXmlArea(xfat_delim_tag_frame, offs_beg, 2);
				m_parser->ReportXmlArea(xfat_delim_tag_frame, m_offs, 1);
				m_offs++;
				return(xcl_named_end);
			}

			// Bogus combination of characters.
			m_parser->ReportXmlError(xrde_sncl_e2, m_offs, 1, m_line_num_at_offs, L"Wrong character after the tag name in the closing tag.");
			m_offs++;
			return(xcl_error);
		}
	}

	// The tag body or container body is present in the stream.
	if (ScanContainerBody(lex_value, lex_end_line_num) == FALSE)
		return(xcl_error);

	// The body was retrieved.
	return(xcl_body_text);
}

void TXmlStream::BufferLowLevelTagHeaderLex(TXmlTagHeaderLex lext, TStringPtr &lex_value, int &lex_end_line_num)
{
	assert(m_cnt_buffered_lexemas == 0);

	m_buffered_lexemas[0].m_hdr_lex_type = lext;
	m_buffered_lexemas[0].m_lex_value = lex_value;
	m_buffered_lexemas[0].m_lex_line_num = lex_end_line_num;

	m_cnt_buffered_lexemas = 1;
}

void TXmlStream::BufferLowLevelContainerLex(TXmlContainerLex lext, TStringPtr &lex_value, int &lex_end_line_num)
{
	assert(m_cnt_buffered_lexemas == 0 || m_cnt_buffered_lexemas == 1);

	m_buffered_lexemas[m_cnt_buffered_lexemas].m_cnr_lex_type = lext;
	m_buffered_lexemas[m_cnt_buffered_lexemas].m_lex_value = lex_value;
	m_buffered_lexemas[m_cnt_buffered_lexemas].m_lex_line_num = lex_end_line_num;

	if (++m_cnt_buffered_lexemas > 2)
		m_cnt_buffered_lexemas = 2;
}

bool TXmlStream::SkipXmlSpaces(bool allow_comments)
{
	while (m_offs < m_buff_len)
	{
		wchar_t ch = m_buff[m_offs];
		if (ch == L' ' || ch == L'\t' || ch == L'\r')
		{
			m_offs++;
			continue;
		}
		else if (ch == L'\n')
		{
			m_offs++;
			m_line_num_at_offs++;
			continue;
		}

		// The current character is not space.
		if (ch == L'<' && allow_comments == TRUE)
		{
			int len_frame = 0;
			if ((m_buff_len-m_offs) >= 4 && wcsncmp(m_buff+m_offs, L"<!--", 4) == 0)
				len_frame = 4;
			else if ((m_buff_len-m_offs) >= 3 && wcsncmp(m_buff+m_offs, L"<--", 3) == 0)
				len_frame = 3;

			if (len_frame > 0)
			{
				// This is beginning of an XML comment.
				m_parser->ReportXmlArea(xfat_xml_comment_frame, m_offs, len_frame);
				m_offs += len_frame;

				int cmt_body_beg = m_offs, cmt_body_end = -1;
				while (m_offs < m_buff_len)
				{
					if (m_buff[m_offs] == L'-')
					{
						len_frame = 0;
						if ((m_buff_len-m_offs) >= 3 && wcsncmp(m_buff+m_offs, L"-->", 3) == 0)
							len_frame = 3;
						else if ((m_buff_len-m_offs) >= 4 && wcsncmp(m_buff+m_offs, L"--!>", 4) == 0)
							len_frame = 4;

						if (len_frame != 0)
						{
							// An end of the comment is found.
							cmt_body_end = m_offs;
							m_offs += len_frame;
							break;
						}
					}

					if (m_buff[m_offs] == L'\n')
						m_line_num_at_offs++;

					m_offs++;
				}

				int actual_cmt_body_end = (cmt_body_end > 0) ? cmt_body_end : m_offs;
				if (actual_cmt_body_end > cmt_body_beg)
					m_parser->ReportXmlArea(xfat_xml_comment_body, cmt_body_beg, actual_cmt_body_end-cmt_body_beg);

				if (cmt_body_end > 0)
				{
					m_parser->ReportXmlArea(xfat_xml_comment_frame, cmt_body_end, len_frame);
				}
				else
				{
					// The end of the comment is missing.
					m_parser->ReportXmlError(xrde_sksp_e1, cmt_body_beg, m_buff_len-cmt_body_beg, m_line_num_at_offs, L"End of file inside the XML comment.");
					return(FALSE);
				}

				continue;
			}
		}

		// Some non space character is present.
		return(TRUE);
	}

	// There are only spaces in front of the EOF.
	return(FALSE);
}

bool TXmlStream::ScanName(TStringPtr &name_area, bool tag_hdr_context)
{
	// The name cannot span over several lines.
	int offs_beg = m_offs;
	while (m_offs < m_buff_len)
	{
		wchar_t ch = m_buff[m_offs];
		if (ch >= L'A' && ch <= L'Z' || ch >= L'a' && ch <= L'z' || ch >= L'0' && ch <= L'9')
		{
			// These are regular letters and digits. Accept them.
			m_offs++;
			continue;
		}
		else if (ch == L':' || ch == L'_' || ch == L'.')
		{
			// These are additional allowed chars. Accept them also.
			m_offs++;
			continue;
		}

		// An end of the name is found.
		break;
	}

	// Check if something was discovered or not.
	if (offs_beg == m_offs)
	{
		int error_area_len = 0;
		if (m_offs < m_buff_len)
		{
			if (m_buff[m_offs] != '\r' && m_buff[m_offs] != '\n')
				error_area_len = 1;

			m_offs++;
		}

		name_area.SetData(m_buff+offs_beg, error_area_len);
		const wchar_t *object_name = (tag_hdr_context == TRUE) ? L"tag attribute" : L"tag or container";
		m_parser->ReportXmlError(xrde_scnm_e1, name_area, m_line_num_at_offs, L"The name of the %s is missing.", object_name);
		return(FALSE);
	}

	// The name is not empty.
	name_area.SetData(m_buff+offs_beg, m_offs-offs_beg);
	return(TRUE);
}

bool TXmlStream::ScanQuotedString(wchar_t frame_char, TStringPtr &string_body_area)
{
	// When this function is called the opening quote is already taken out of the source and it is passed
	// as the first param. This allows processing single and double quoted strings.
	int offs_beg = m_offs;
	while (m_offs < m_buff_len)
	{
		wchar_t ch = m_buff[m_offs];
		if (ch == L'\r' || ch == L'\n')
		{
			// End of line inside the string.
			string_body_area.SetData(m_buff+offs_beg, m_offs-offs_beg);
			m_parser->ReportXmlError(xrde_scqs_e1, string_body_area, m_line_num_at_offs, L"End of line inside the quoted string.");

			m_offs++;
			if (m_offs < m_buff_len && m_buff[m_offs] == L'\n')
				m_offs++;

			return(FALSE);
		}
		else if (ch == frame_char)
		{
			// End of the string is successfully found. Note that the string may be empty.
			string_body_area.SetData(m_buff+offs_beg, m_offs-offs_beg);
			m_offs++;
			return(TRUE);
		}

		// Accept the char.
		m_offs++;
	}

	// The closing quote is missing.
	string_body_area.SetData(m_buff+m_buff_len, 0);
	m_parser->ReportXmlError(xrde_scqs_e2, string_body_area, m_line_num_at_offs, L"End of file inside the quoted string.");
	return(FALSE);
}

bool TXmlStream::ScanContainerBody(TStringPtr &body_area, int &body_end_line_num)
{
	// When this method is called, potential spaces, including the XML coments, are expected to be already skipped.
	int latest_space_beg = -1;
	int latest_space_beg_line_num = -1;

	int offs_beg = m_offs;
	while (m_offs < m_buff_len)
	{
		wchar_t ch = m_buff[m_offs];

		if (ch == L'<')
		{
			// This is beginning of either a tag or an XML comment. In any case this is end of
			// the current fragment of the container body.
			break;
		}
		else if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n')
		{
			if (latest_space_beg == -1)
			{
				// This is the beginning of the area of white spaces.
				latest_space_beg = m_offs;
				latest_space_beg_line_num = m_line_num_at_offs;
			}

			if (ch == L'\n')
			{
				m_line_num_at_offs++;
			}
		}
		else
		{
			// This is the beginning of the non white spaces area.
			latest_space_beg = -1;
		}

		// Shift the current position forward.
		m_offs++;
	}

	// This is an end of the container body fragment or EOF. Note that container body can be empty.
	int body_end_offs = (latest_space_beg >= 0) ? latest_space_beg : m_offs;
	body_area.SetData(m_buff+offs_beg, body_end_offs-offs_beg);
	body_end_line_num = (latest_space_beg >= 0) ? latest_space_beg_line_num : m_line_num_at_offs;
	SkipXmlSpaces(TRUE);
	return(TRUE);
}

TXmlStream::TXmlContainerLex TXmlStream::UnexpectedEofInsideTheContainer(int offs_beg, TStringPtr &lex_value)
{
	assert(m_offs == m_buff_len);
	lex_value.SetData(m_buff+offs_beg, m_buff_len-offs_beg);
	m_parser->ReportXmlError(xrde_ueic_e1, lex_value, m_line_num_at_offs, L"Unexpected EOF.");
	return(xcl_error);
}

TXmlLexemaType TXmlStream::ReportBadLowLevelLexemaType(int llt, TStringPtr &tag_name)
{
	// The code below is a bit paranoic, but still makes sense.
	tag_name.SetData(m_buff+m_offs, 0);
	m_parser->ReportXmlError(xrde_bllt_e1, m_offs, 0, -1, L"Unknown low level lexema type: %d.", llt);
	return(xlt_error);
}

// ---------------------------------------------------------------------------
//   =================  TXmlContainerHandler  =======================
// ---------------------------------------------------------------------------

bool TXmlContainerHandler::LoadFromExeDirectory(const TXmlDictRecord *rootContainerTags, const wchar_t *xpct_root_container_name,
												const wchar_t *short_fname, TGenericConsole *cons, TDateTime *latest_scanned_file_date, bool *new_file_detected_flag)
{
	// Prepare the full name of the file.
	wchar_t profile_fname[2*MAX_PATH+80];
	TPathHelper::GetExeDirectoryName(profile_fname, 2*MAX_PATH);
	wcscat(profile_fname, short_fname);

	// Allocate low level scanner that will read the data from the file with the name that was just prepared.
	TXmlStream xmlSource(profile_fname);

	// Check, if the date of file from the latest scan was passed or not.
	if (latest_scanned_file_date != NULL)
	{
		TDateTime curr_file_date = xmlSource.GetStreamFileDate();
		if (curr_file_date == *latest_scanned_file_date)
		{
			// This is rescan of the file that was already scanned before. Do not emit the scanning errors if any.
			cons = NULL;

			if (new_file_detected_flag != NULL)
				*new_file_detected_flag = FALSE;
		}
		else
		{
			// Return the file date to the app layer for future use.
			*latest_scanned_file_date = curr_file_date;

			if (new_file_detected_flag != NULL)
				*new_file_detected_flag = TRUE;
		}
	}

	// Allocate reader with the fixed name of the root container. Note that console can be NULL.
	TXmlReader reader(rootContainerTags, xpct_root_container_name, cons);

	// Do the scanning.
	return(reader.Parse(xmlSource, this));
}

bool TXmlContainerHandler::StoreStringValue(TStringPtr &strValue, wchar_t *location, int locationLen, TStringPtr &tagName, bool emitError)
{
	int valueLen = strValue.GetLength();
	assert(locationLen > 0);
	assert(valueLen >= 0);

	if (valueLen >= locationLen)
	{
		// The destination buffer is too small.
		if (emitError == TRUE)
			m_parser->ReportXmlError(xrde_ssvl_e1, strValue, m_parser->GetLatestLexemaLineNumber(),
								L"Destination buffer with the length %d is too small to store the string \"%.*s\" as a value of the tag \"%.*s\".",
								locationLen, valueLen, strValue.GetBodyPtr(), tagName.GetLength(), tagName.GetBodyPtr());
		return(FALSE);
	}

	// Save the data.
	wcsncpy(location, strValue.GetBodyPtr(), valueLen);
	location[valueLen] = 0;
	return(TRUE);
}

void TXmlContainerHandler::StoreStringValueAllowTruncation(TStringPtr &strValue, wchar_t *location, int location_len)
{
	int valueLen = strValue.GetLength();
	assert(location_len > 0);
	assert(valueLen >= 0);

	int len = __min(valueLen, location_len-1);
	if (len < 0)
		len = 0;

	if (len > 0)
	{
		// There is some data to copy.
		wcsncpy(location, strValue.GetBodyPtr(), len);
	}

	location[len] = 0;
}

bool TXmlContainerHandler::StoreCharValue(__int64 tagValue, int min_value, int max_value, char &destination, TStringPtr &tagName)
{
	if (tagValue < min_value || tagValue > max_value)
	{
		m_parser->ReportXmlError(xrde_nmvl_e1, tagName, m_parser->GetLatestLexemaLineNumber(),
							L"The value of the numeric tag is out of range. The range for \"%.*s\" is from %d to %d.",
							tagName.GetLength(), tagName.GetBodyPtr(), min_value, max_value);
		return(FALSE);
	}

	destination = (char)tagValue;
	return(TRUE);
}

bool TXmlContainerHandler::StoreShortValue(__int64 tagValue, short min_value, short max_value, short &destination, TStringPtr &tagName)
{
	if (tagValue < min_value || tagValue > max_value)
	{
		m_parser->ReportXmlError(xrde_nmvl_e2, tagName, m_parser->GetLatestLexemaLineNumber(),
							L"The value of the numeric tag is out of range. The range for \"%.*s\" is from %hd to %hd.",
							tagName.GetLength(), tagName.GetBodyPtr(), min_value, max_value);
		return(FALSE);
	}

	destination = (short)tagValue;
	return(TRUE);
}

bool TXmlContainerHandler::StoreIntValue(__int64 tagValue, int min_value, int max_value, int &destination, TStringPtr &tagName)
{
	if (tagValue < min_value || tagValue > max_value)
	{
		m_parser->ReportXmlError(xrde_nmvl_e3, tagName, m_parser->GetLatestLexemaLineNumber(),
							L"The value of the numeric tag is out of range. The range for \"%.*s\" is from %d to %d.",
							tagName.GetLength(), tagName.GetBodyPtr(), min_value, max_value);
		return(FALSE);
	}

	destination = (int)tagValue;
	return(TRUE);
}

// ------------------------------------------------------------------------------
//   ======================  TXmlReader  ===========================
// ------------------------------------------------------------------------------

static TXmlDictRecord g_XmlReader_EmptyTagsDict[1] =
{
	{ xdrt_eof, xrdm_opt_si, xtbt_none },
};

TXmlContainerHandler  TXmlReader::s_skip_subtree_handler;

TXmlReader::TXmlReader(const TXmlDictRecord *root_container_tags, const wchar_t *xpct_root_cnr_name, TGenericConsole *cons)
{
	assert(root_container_tags != NULL);
	Init(root_container_tags, xpct_root_cnr_name);

	m_console = cons;
}

TXmlReader::TXmlReader(const TXmlDictRecord *root_container_tags, const wchar_t *xpct_root_cnr_name, TMessageBuffer &errors_buff)
{
	assert(root_container_tags != NULL);
	Init(root_container_tags, xpct_root_cnr_name);

	m_errors_buffer = &errors_buff;
	m_errors_buffer->ClearBuffer();
}

TXmlReader::TXmlReader(XmlSyntaxHighlightCbkHandler cbk_handler, XmlSyntaxHighlightCbkContext cbk_handler_context, TXmlItemPresentationRecord *special_tags_dict, TGenericConsole *cons)
{
	assert(cbk_handler != NULL);
	Init(NULL, NULL);

	if (special_tags_dict != NULL)
	{
		// Save the pointer before running the loop.
		m_shlt_tags_dict = special_tags_dict;

		// Prepare the hash table for better access to the array of tag infos.
		int record_pseudo_inx = 1;
		while (special_tags_dict->m_xml_item_name != NULL)
		{
			assert(special_tags_dict->m_xml_item_name_len > 0);
			short hash_index = special_tags_dict->m_xml_item_name_len & (NUM_SHLT_TAG_BUCKETS-1);
			special_tags_dict->m_next_same_hash_record = m_shlt_tags_hash_table[hash_index];
			m_shlt_tags_hash_table[hash_index] = record_pseudo_inx;

			// Shift to the next record.
			special_tags_dict++;
			record_pseudo_inx++;
		}
	}

	m_shlt_cbk_handler = cbk_handler;
	m_shlt_cbk_context = cbk_handler_context;
	m_console = cons;
}

bool TXmlReader::Parse(TXmlStream &source, TXmlContainerHandler *rootNode)
{
	assert(m_nesting_data[0].m_layer_tags != NULL && m_shlt_cbk_handler == NULL);
	assert(rootNode != NULL && rootNode != &s_skip_subtree_handler);

	// Setup the parsing.
	m_abort_flag = FALSE;
	m_latest_lexema_line = 0;
	m_errors_count = 0;

	if (m_errors_buffer != NULL)
		m_errors_buffer->ClearBuffer();

	source.SetParser(this);
	rootNode->SetParser(this);

	// Do the job.
	ParseInternal(rootNode);

	// Check, if all nesting layers were exited or not.
	if (m_nesting_level > 0)
	{
		// Exit all remaining layers in an abort mode.
		TStringPtr emptyTagName;
		while (m_nesting_level > 0)
			ExitNesting(emptyTagName, TRUE);
	}

	// Check the overall results.
	if (m_abort_flag == TRUE)
	{
		if (m_errors_buffer != NULL)
		{
			// Append the error buffer specific message that should not go to console.
			wchar_t buffer_big[2*MAX_PATH+80], buffer[120];
			swprintf(buffer, 120, L"\r\nParsing was abnormally terminated after %d error%s.", m_errors_count, PluralSuff(m_errors_count));
			ReportErrorInternal(buffer_big, buffer, NULL, NULL);
		}

		// Return the parsing failure.
		return(FALSE);
	}
	else if (m_errors_count > 0)
	{
		if (m_errors_buffer != NULL)
		{
			// Append the error buffer specific message that should not go to console.
			wchar_t buffer_big[2*MAX_PATH+80], buffer[120];
			swprintf(buffer, 120, L"\r\nEnd of the source file is reached with %d error%s.", m_errors_count, PluralSuff(m_errors_count));
			ReportErrorInternal(buffer_big, buffer, NULL, NULL);
		}

		// Return the parsing failure.
		return(FALSE);
	}

	// Full success.
	return(TRUE);
}

bool TXmlReader::SyntaxHighlightParse(TXmlStream &source)
{
	// Ensure that object was created using the right ctor.
	assert(m_nesting_data[0].m_layer_tags == NULL && m_shlt_cbk_handler != NULL);

	// Setup the parsing.
	m_abort_flag = FALSE;
	m_latest_lexema_line = 0;
	m_errors_count = 0;
	source.SetParser(this);

	if (m_input->SkipXmlHeader() == TRUE)
	{
		TStringPtr tag_name, tag_value, tag_area_ex;
		for(;;)
		{
			// Pick up the next lexema.
			TXmlLexemaType lex_type = m_input->GetXmlLexema(tag_name, tag_value, tag_area_ex, m_latest_lexema_line);
			if (lex_type == xlt_eof)
				break;

			if (lex_type == xlt_cnr_beg)
			{
				if (m_nesting_level >= MAX_CNRS_NESTING)
				{
					ReportXmlError(xrde_shlt_e1, tag_name, m_latest_lexema_line, L"The  nesting of the subcontainers is too deep. The max allowed depth is %d.", MAX_CNRS_NESTING);
					AbortXmlParsing();
					return(FALSE);
				}

				// Occupy the nesting info slot.
				m_nesting_data[m_nesting_level].m_cnr_name = tag_name;
				m_nesting_data[m_nesting_level].m_cnr_beg_line = m_latest_lexema_line;
				m_nesting_level++;
			}
			else if (lex_type == xlt_cnr_end)
			{
				if (m_nesting_level <= 0)
				{
					ReportXmlError(xrde_shlt_e2, tag_name, m_latest_lexema_line, L"Unbalannced subcontainer closing.");
					AbortXmlParsing();
					return(FALSE);
				}

				// Exit the nesting slot.
				m_nesting_level--;

				// Compare opening and closing names.
				if (tag_name.IsEmpty() == FALSE && m_nesting_data[m_nesting_level].m_cnr_name != tag_name)
				{
					TStringPtr &opn_tag_name = m_nesting_data[m_nesting_level].m_cnr_name;
					ReportXmlError(xrde_shlt_e3, tag_name, m_latest_lexema_line, L"The closing tag \"%.*s\" on the line %d differs from the opening tag \"%.*s\" on the line %d.",
								tag_name.GetLength(), tag_name.GetBodyPtr(), m_latest_lexema_line,
								opn_tag_name.GetLength(), opn_tag_name.GetBodyPtr(), m_nesting_data[m_nesting_level].m_cnr_beg_line);
				}
			}

			if (lex_type >= xlt_tag_attr && lex_type <= xlt_cnr_end)
			{
				// Current lexema needs highlighting on the reader level.
				TXmlItemPresentationRecord *tag_info = NULL;

				if (m_shlt_tags_dict != NULL && tag_name.GetLength() > 0)
				{
					int rc_ps_inx = m_shlt_tags_hash_table[tag_name.GetLength() & (NUM_SHLT_TAG_BUCKETS-1)];
					while (rc_ps_inx > 0)
					{
						TXmlItemPresentationRecord &rec = m_shlt_tags_dict[rc_ps_inx-1];
						if (wcsncmp(rec.m_xml_item_name, tag_name.GetBodyPtr(), tag_name.GetLength()) == 0)
						{
							tag_info = &rec;
							break;
						}

						// Shift to the next record that describes tag with the same length.
						rc_ps_inx = rec.m_next_same_hash_record;
					}
				}

				switch (lex_type)
				{
					case xlt_tag_attr:
							{
								// Name and quoted value areas should always be non empty.
								assert(tag_name.GetLength() > 0 && tag_area_ex.GetLength() > 0);
								ReportXmlArea(xfat_tag_attr_name, tag_name, tag_info);
								ReportXmlArea(xfat_tag_attr_value, tag_area_ex, tag_info);
							}
							break;

					case xlt_leaf_tag:
							{
								// Name of the leaf tag should be present while the tag body and the closing name can be missing.
								assert(tag_name.GetLength() > 0);
								if (tag_info != NULL && tag_info->m_presentation_props != NULL && tag_info->m_presentation_props->m_show_leaf_tag_as_container == TRUE)
								{
									// This is special case of pseudo container.
									ReportXmlArea(xfat_container_name_open, tag_name, tag_info);
									if (tag_value.GetLength() > 0)
										ReportXmlArea(xfat_container_body, tag_value, tag_info);
									if (tag_area_ex.GetLength() > 0)
										ReportXmlArea(xfat_container_name_close, tag_area_ex, tag_info);
								}
								else
								{
									// Regular case.
									ReportXmlArea(xfat_tag_name_open, tag_name, tag_info);
									if (tag_value.GetLength() > 0)
										ReportXmlArea(xfat_leaf_tag_value, tag_value, tag_info);
									if (tag_area_ex.GetLength() > 0)
										ReportXmlArea(xfat_tag_name_close, tag_area_ex, tag_info);
								}
							}
							break;

					case xlt_cnr_beg:
							{
								// Container opening should always contain the name.
								assert(tag_name.GetLength() > 0);
								ReportXmlArea(xfat_container_name_open, tag_name, tag_info);
							}
							break;

					case xlt_cnr_body:
							{
								// Report the body area only if it is not empty.
								if (tag_value.GetLength() > 0)
									ReportXmlArea(xfat_container_body, tag_value, tag_info);
							}
							break;

					case xlt_cnr_end:
							{
								// Container end can be named and nameless.
								if (tag_name.GetLength() > 0)
									ReportXmlArea(xfat_container_name_close, tag_name, tag_info);
							}
							break;
				}
			}
		}

		if (m_nesting_level > 0)
		{
			// Issue errors for all unclosed containers.
			for (int unkl_lev=m_nesting_level-1; unkl_lev >= 0; --unkl_lev)
			{
				TStringPtr &opn_tag_name = m_nesting_data[unkl_lev].m_cnr_name;
				ReportXmlError(xrde_shlt_e4, tag_name, m_latest_lexema_line, L"The closing tag of the container \"%.*s\" that starts on the line %d is missing.",
							opn_tag_name.GetLength(), opn_tag_name.GetBodyPtr(), m_nesting_data[unkl_lev].m_cnr_beg_line);
			}
		}
	}

	return(m_abort_flag == FALSE && m_errors_count == 0);
}

void TXmlReader::AbortXmlParsing()
{
	m_abort_flag = TRUE;
	if (m_console != NULL)
	{
		m_console->SetAbortFlag(TRUE);
	}
}

void TXmlReader::ReportXmlError(WORD err_code, int error_area_offs, int error_area_len, int line_num, const wchar_t *format, ...)
{
	assert(m_input != NULL);
	if (m_errors_count == 0)
		PrependErrorsHeader();

	// Format the error message.
	wchar_t buffer_big[2*MAX_PATH+80], buffer[80];
	swprintf(buffer, 80, L"%sLine %d, E%04hu: ", ((m_errors_buffer != NULL) ? L"        " : L""), line_num, err_code);

	va_list vargs;
	va_start(vargs, format);
	ReportErrorInternal(buffer_big, buffer, format, vargs);
	va_end(vargs);

	m_errors_count++;

	if (error_area_offs >= 0)
	{
		m_current_error_msg = buffer_big;
		ReportXmlArea(XmlAreaTypeFromXmlErrorCode(err_code), error_area_offs, error_area_len);
		m_current_error_msg = NULL;
	}
}

void TXmlReader::ReportXmlError(WORD err_code, TStringPtr &error_area, int line_num, const wchar_t *format, ...)
{
	assert(m_input != NULL);
	if (m_errors_count == 0)
		PrependErrorsHeader();

	// Format the error message.
	wchar_t buffer_big[2*MAX_PATH+80], buffer[80];
	swprintf(buffer, 80, L"%sLine %d, E%04hu: ", ((m_errors_buffer != NULL) ? L"        " : L""), line_num, err_code);

	va_list vargs;
	va_start(vargs, format);
	ReportErrorInternal(buffer_big, buffer, format, vargs);
	va_end(vargs);

	m_errors_count++;

	if (error_area.IsEmpty() == FALSE)
	{
		m_current_error_msg = buffer_big;
		ReportXmlArea(XmlAreaTypeFromXmlErrorCode(err_code), error_area);
		m_current_error_msg = NULL;
	}
}

void TXmlReader::ReportXmlAppError(const wchar_t *format, ...)
{
	assert(m_input != NULL);
	if (m_errors_count == 0)
		PrependErrorsHeader();

	// Format the message prefix.
	wchar_t buffer_big[2*MAX_PATH+80], buffer[80];
	swprintf(buffer, 80, L"%sLine %d: ", ((m_errors_buffer != NULL) ? L"        " : L""), m_latest_lexema_line);

	va_list vargs;
	va_start(vargs, format);
	ReportErrorInternal(buffer_big, buffer, format, vargs);
	va_end(vargs);

	m_errors_count++;
}

TXmlFileAreaType TXmlReader::XmlAreaTypeFromXmlErrorCode(WORD error_code)
{
	if (error_code < xrde_std_handlers_erros_base)
		return(xfat_xml_syntax_error);
	else if (error_code < xrde_xml_schema_errors_base)
		return(xfat_app_layer_error);
	else if (error_code < xrde_app_errorss_base)
		return(xfat_xml_schema_error);

	// The rest of the error codes are expected to come from the app layer.
	return(xfat_app_layer_error);
}

bool TXmlReader::CheckTagMentioned(WORD tagId)
{
	assert(m_curr_layer != NULL);

	// Look for the tag with the passed id in the local tags dictionary.
	TXmlDictRecord *di = m_curr_layer->m_layer_tags;
	int tag_inx = 0;
	while (di->m_rec_type != xdrt_eof && tag_inx < TXmlReader::MAX_TAGS_DICT_LEN)
	{
		if (di->m_tag_ident == tagId)
		{
			// The requested tag is known.
			return(m_curr_layer->m_tags_present[tag_inx]);
		}

		di++;
		tag_inx++;
	}

	// The tag is unknown or the tags dictionary is too long.
	return(FALSE);
}

void TXmlReader::Init(const TXmlDictRecord *root_container_tags, const wchar_t *xpct_root_cnr_name)
{
	// Do a neutral init that will work fine for all available ctors.
	m_input = NULL;

	m_expected_root_cnr_name[0] = 0;
	if (xpct_root_cnr_name != NULL && xpct_root_cnr_name[0] != 0)
	{
		wcsncpy(m_expected_root_cnr_name, xpct_root_cnr_name, MAX_ROOT_CNR_NAME_LEN);
		m_expected_root_cnr_name[MAX_ROOT_CNR_NAME_LEN-1] = 0;
	}

	// None of the layers are entered.
	memset(m_nesting_data, 0, sizeof(m_nesting_data));
	m_nesting_level = 0;
	m_curr_layer = NULL;

	// Store the tags dict pointer if any into the first nesting level info for a while.
	m_nesting_data[0].m_layer_tags = (TXmlDictRecord*)root_container_tags;

	// Put NULLs in these fields for now.
	m_shlt_tags_dict = NULL;
	memset(m_shlt_tags_hash_table, 0, sizeof(m_shlt_tags_hash_table));
	m_shlt_cbk_handler = NULL;
	m_shlt_cbk_context = NULL;

	// Init empty errors reporting and support stuffs.
	m_console = NULL;
	m_errors_buffer = NULL;

	m_abort_flag = FALSE;
	m_latest_lexema_line = -1;
	m_errors_count = 0;
	m_current_error_msg = NULL;
}

void TXmlReader::PrependErrorsHeader()
{
	wchar_t *space_str = (m_errors_buffer != NULL) ? L"  " : L"";
	wchar_t *suffix_str = (m_errors_buffer != NULL) ? L"\r\n" : L"";

	// This is the first error. Show the name of the file.
	wchar_t buffer[2*MAX_PATH+80];
	swprintf(buffer, 2*MAX_PATH+80, L"Error parsing XML file: %s\"%s\".%s", space_str, m_input->GetStreamFileName(), suffix_str);

	wchar_t buffer_big[2*MAX_PATH+80];
	ReportErrorInternal(buffer_big, buffer, NULL, NULL);
}

void TXmlReader::ReportErrorInternal(wchar_t *buffer_big, const wchar_t *msgPrefix, const wchar_t *format, va_list vargs)
{
	wchar_t *pb = buffer_big;

	if (msgPrefix != NULL)
	{
		// Add the prefix.
		wcscpy(buffer_big, msgPrefix);
		pb = buffer_big+wcslen(buffer_big);
	}

	if (format != NULL)
	{
		// Assemble the main message.
		vswprintf(pb, 2*MAX_PATH, format, vargs);
	}

	// Process the error buffer case first becasue in any case message is already prepared
	// in the error buffer style it the error buffer field is not NULL.
	if (m_errors_buffer != NULL)
	{
		// Write this line to the buffer.
		m_errors_buffer->Append(buffer_big);
		m_errors_buffer->Append(L"\r\n");
	}
	else if (m_console != NULL)
	{
		// Send the string to the callback.
		m_console->ShowRunTimeError(buffer_big);
	}
}

void TXmlReader::ReportXmlArea(TXmlFileAreaType area_type, int area_beg_offs, int area_len, TXmlItemPresentationRecord *tag_info)
{
	if (m_shlt_cbk_handler != NULL)
	{
		assert(area_beg_offs >= 0 && area_len >= 0);
		TXmlFileAreaInfo info = { area_type, area_beg_offs, area_len, tag_info };
		if (area_type >= xfat_xml_syntax_error && area_type <= xfat_app_layer_error)
		{
			info.xml_error_id = m_errors_count;
			info.xml_error_text = m_current_error_msg;
		}

		// Call the callback function.
		m_shlt_cbk_handler(m_shlt_cbk_context, info);
	}
}

void TXmlReader::ReportXmlArea(TXmlFileAreaType area_type, TStringPtr &area, TXmlItemPresentationRecord *tag_info)
{
	if (m_shlt_cbk_handler != NULL)
	{
		assert(area.GetLength() >= 0);
		TXmlFileAreaInfo info = { area_type, area.GetBodyPtr()-m_input->m_buff, area.GetLength(), tag_info };
		if (area_type >= xfat_xml_syntax_error && area_type <= xfat_app_layer_error)
		{
			info.xml_error_id = m_errors_count;
			info.xml_error_text = m_current_error_msg;
		}

		// Call the callback function.
		m_shlt_cbk_handler(m_shlt_cbk_context, info);
	}
}

void TXmlReader::ParseInternal(TXmlContainerHandler *rootNode)
{
	// Ensure that the source file is present.
	if (m_input->m_buff == NULL)
	{
		// The in memory buffer is missing.
		if (m_input->LoadXmlFileFromDisk() == FALSE)
		{
			// An error was already emitted.
			return;
		}
	}

	// Skip the XML header if any.
	if (m_input->SkipXmlHeader() == FALSE)
		return;

	// Pick up the first lexema after the XML header.
	TStringPtr ftlxn, ftlxv, ftlex;
	TXmlLexemaType root_cnr_lex_type = m_input->GetXmlLexema(ftlxn, ftlxv, ftlex, m_latest_lexema_line);

	// Enter the nesting level 1. This actually implies that the file should begin with a container open tag with the expected name.
	if (root_cnr_lex_type != xlt_cnr_beg && root_cnr_lex_type != xlt_leaf_tag)
	{
		// There is no open container lexema.
		ReportXmlError(xrde_mprs_e1, ftlxn, m_latest_lexema_line, L"The file should start with the beginning of the XML container.");
		return;
	}
	else if (m_expected_root_cnr_name[0] != 0 && ftlxn != m_expected_root_cnr_name)
	{
		// Expected container name mismatch.
		ReportXmlError(xrde_mprs_e2, ftlxv, m_latest_lexema_line, L"The name of the root container is wrong. Application expects container \"%s\".", m_expected_root_cnr_name);
		return;
	}

	// Enter the root container.
	if (EnterNesting(ftlxn, NULL, -1, rootNode) == FALSE)
	{
		// This failure is unlikely to happen, neverteless it happened.
		return;
	}

	if (root_cnr_lex_type == xlt_leaf_tag)
	{
		// This is special case when the XML document is empty.
		if (ExitNesting(ftlxn, m_abort_flag) == FALSE)
			return;

		if (m_input->GetXmlLexema(ftlxn, ftlxv, ftlex, m_latest_lexema_line) != xlt_eof)
			ReportXmlError(xrde_mprs_e3, ftlxn, m_latest_lexema_line, L"Extra data after the empty root container.");

		return;
	}

	// Loop over lexemas in the source file. Allocate vars outside the loop for perf reasons.
	TStringPtr tag_name, tag_value, tag_area_ex;
	for(;;)
	{
		// Pick up the next lexema.
		TXmlLexemaType lex_type = m_input->GetXmlLexema(tag_name, tag_value, tag_area_ex, m_latest_lexema_line);

		if (lex_type == xlt_error)
		{
			// Error is already reported from inside GetXmlLexema().
			if (m_errors_count >= MAX_ERRORS_IN_DOCUMENT)
			{
				ReportXmlError(xrde_mprs_e4, tag_name, m_latest_lexema_line, L"Too many errors. Aboring...");
				AbortXmlParsing();
				break;
			}

			// Continue parsing.
			continue;
		}
		else if (lex_type == xlt_eof)
		{
			// This is an end of file in the middle of everything. The nesting should be not empty.
			assert(m_nesting_level > 0);
			ReportXmlError(xrde_mprs_e5, tag_name, m_latest_lexema_line, L"End of file inside the containers hierarchy. Current nesting level is %d.", m_nesting_level);
			break;
		}
		else if (lex_type == xlt_cnr_end)
		{
			// An end of container is found. Exit the current nesting.
			if (ExitNesting(tag_name) == FALSE)
			{
				// Abort was signalled.
				break;
			}

			if (m_nesting_level <= 0)
			{
				// The root level container was exited. There should be nothing else in the file up to its end.
				if (m_input->GetXmlLexema(tag_name, tag_value, tag_area_ex, m_latest_lexema_line) != xlt_eof)
				{
					ReportXmlError(xrde_mprs_e6, tag_name, m_latest_lexema_line, L"Extra data after the end of the root container.");
					AbortXmlParsing();
				}

				// An end of the source stream is reached. Break out of the loop.
				break;
			}
			continue;
		}
		else if (lex_type == xlt_cnr_body)
		{
			// Ignore the container body.
			continue;
		}
		else if (lex_type != xlt_cnr_beg && lex_type != xlt_leaf_tag && lex_type != xlt_tag_attr)
		{
			// For some unclear reason the lex type is bogus.
			ReportXmlError(xrde_mprs_e7, tag_name, m_latest_lexema_line, L"Unexpected XML lexema type: %d.", lex_type);
			continue;
		}

		// All remainig types of lexemas have name of the tag.
		// Look in the dictionary of the current node for the record that has this name.
		assert(tag_name.GetLength() > 0);
		int tag_dict_inx = -1;
		TXmlDictRecord *dict_record = NULL;
		if (m_curr_layer->m_layer_node != &s_skip_subtree_handler)
		{
			dict_record = CheckTagsDict(tag_name, tag_dict_inx);
			if (dict_record == NULL)
			{
				ReportXmlError(xrde_mprs_e8, tag_name, m_latest_lexema_line, L"The tag or tag attr \"%.*s\" is not defined in the schema of the \"%.*s\" container.",
							tag_name.GetLength(), tag_name.GetBodyPtr(), m_curr_layer->m_cnr_name.GetLength(), m_curr_layer->m_cnr_name.GetBodyPtr());
			}
		}

		if (dict_record == NULL || CheckDictRecord(lex_type, tag_name, dict_record, tag_dict_inx) == FALSE)
		{
			// This tag is not allowed in the current container.
			if (m_abort_flag == TRUE)
				break;

			if (lex_type == xlt_cnr_beg)
			{
				// In fact, the tag, that opens a subcontainer should not be simply ignored. Instead of breaking the tree
				// structure it is much better to enter the "skip everything" mode and ignore the whole subtree.
				if (EnterNesting(tag_name, NULL, -1, &s_skip_subtree_handler) == FALSE)
				{
					// Abort was signalled.
					break;
				}
			}

			// Lexemas of other types can be simply ignored.
			continue;
		}

		// Process entering container.
		if (lex_type == xlt_cnr_beg)
		{
			// Try to enter new level of nesting.
			if (EnterNesting(tag_name, dict_record, tag_dict_inx) == FALSE)
			{
				// Abort was signalled.
				break;
			}

			continue;
		}
		else if (lex_type == xlt_leaf_tag && dict_record->m_rec_type == xdrt_cnr)
		{
			// This is more complex situation. Container appears as a simple tag. And even more.
			// Schema explictly allows this. Implement this as a sort of small script.
			if (EnterNesting(tag_name, dict_record, tag_dict_inx) == FALSE || ExitNesting(tag_name) == FALSE)
			{
				// Abort was signalled.
				break;
			}

			continue;
		}

		//
		// At this point an object can only be either the tag or the tag attr.
		//

		// This tag has id and it has type of the body. Process the body of the tag according to its type.
		bool bool_value;
		__int64  int_value;
		GUID guid_value;
		TColor color_value;
		WORD tag_id = dict_record->m_tag_ident;
		switch (dict_record->m_body_type)
		{
			case xtbt_bool:
					{
						// Assemble and process the bool value.
						if (AssembleBoolean(tag_value, bool_value) == FALSE)
							break;
						m_curr_layer->m_layer_node->ProcessBooleanField(tag_id, bool_value, tag_name);
					}
					break;

			case xtbt_number:
					{
						// Assemble and process the numeric value.
						if (AssembleInteger(tag_value, int_value) == FALSE)
							break;
						m_curr_layer->m_layer_node->ProcessIntegerField(tag_id, int_value, tag_name);
					}
					break;

			case xtbt_string:
					{
						// A string does not need any assembling. Just process it.
						m_curr_layer->m_layer_node->ProcessStringField(tag_id, tag_value, tag_name);
					}
					break;

			case xtbt_guid:
					{
						// Assemble and process the guid value.
						if (AssembleGuid(tag_value, guid_value) == FALSE)
							break;
						m_curr_layer->m_layer_node->ProcessGuidField(tag_id, guid_value, tag_name);
					}
					break;

			case xtbt_color:
					{
						// Assemble and process the color value.
						if (AssembleColor(tag_value, color_value) == FALSE)
							break;
						m_curr_layer->m_layer_node->ProcessColorField(tag_id, color_value, tag_name);
					}
					break;

			default:
				assert(FALSE);
				break;
		}

		// Check, if abort was signalled or not.
		if (m_abort_flag == TRUE)
			break;

		// Mark the tag as present in the current container.
		m_curr_layer->m_tags_present[dict_record-m_curr_layer->m_layer_tags] = TRUE;
	}
}

TXmlDictRecord *TXmlReader::CheckTagsDict(TStringPtr &tagName, int &tag_dict_inx)
{
	// Look for the tag in the current dictionary.
	int tagNameLen = tagName.GetLength();
	TXmlDictRecord *dict_rec = m_curr_layer->m_layer_tags+m_curr_layer->m_tags_search_hint;
	int tag_inx = m_curr_layer->m_tags_search_hint;

	int cnt_tested_records = 0;
	while (cnt_tested_records < MAX_TAGS_DICT_LEN)
	{
		assert(dict_rec->m_tag_name_len > 0 && dict_rec->m_tag_name != NULL);
		if (tagNameLen == dict_rec->m_tag_name_len && _wcsnicmp(tagName.GetBodyPtr(), dict_rec->m_tag_name, tagNameLen) == 0)
		{
			// Dictionary contains tag with exactly the same name.
			assert(dict_rec->m_tag_ident != 0);
			m_curr_layer->m_tags_search_hint = tag_inx;
			tag_dict_inx = tag_inx;
			return(dict_rec);
		}

		// Shift to the next record.
		dict_rec++;
		tag_inx++;
		if (dict_rec->m_rec_type == xdrt_eof)
		{
			dict_rec = m_curr_layer->m_layer_tags;
			tag_inx = 0;
		}

		if (tag_inx == m_curr_layer->m_tags_search_hint)
		{
			// The whole dictionary was checked.
			break;
		}

		cnt_tested_records++;
	}

	// Check the loop termination conditions.
	if (cnt_tested_records >= MAX_TAGS_DICT_LEN)
	{
		ReportXmlError(xrde_chtg_e1, tagName, m_latest_lexema_line, L"The tags dictionary for \"%.*s\" is too long. The max allowed length is %d.",
					m_curr_layer->m_cnr_name.GetLength(), m_curr_layer->m_cnr_name.GetBodyPtr(), MAX_TAGS_DICT_LEN);
	}

	// There is no definition for this tag in the tags dict.
	return(NULL);
}

bool TXmlReader::CheckDictRecord(TXmlLexemaType lex_type, TStringPtr &tag_name_ptr, TXmlDictRecord *dict_item, int tag_inx)
{
	assert(lex_type >= xlt_tag_attr && lex_type <= xlt_cnr_beg);
	assert(dict_item->m_tag_name_len > 0 && dict_item->m_tag_name != NULL);

	wchar_t *tag_name = tag_name_ptr.GetBodyPtr();
	int tag_name_len = tag_name_ptr.GetLength();

	// This is an ordinary tag with a name. Compare type of lexema with the expected nature of the tag.
	if (lex_type == xlt_tag_attr)
	{
		// Tag descriptor should be Attr or Attr/Field.
		if (dict_item->m_rec_type != xdrt_attr && dict_item->m_rec_type != xdrt_atrfl)
		{
			ReportXmlError(xrde_chdr_e1, tag_name_ptr, m_latest_lexema_line, L"The name \"%.*s\" is defined in the current container as a tag, not as a tag attr.", tag_name_len, tag_name);
			return(FALSE);
		}
	}
	else if (lex_type == xlt_leaf_tag)
	{
		// Any tag descriptor except Attr and Cnr.
		if (dict_item->m_rec_type == xdrt_attr)
		{
			ReportXmlError(xrde_chdr_e2, tag_name_ptr, m_latest_lexema_line, L"The name \"%.*s\" is defined in the current container as a tag attribute.", tag_name_len, tag_name);
			return(FALSE);
		}
	}
	else
	{
		// For the beginning of container tag descriptor should be the Cnr or Cnr/Field.
		assert(lex_type == xlt_cnr_beg);
		if (dict_item->m_rec_type != xdrt_cnr)
		{
			ReportXmlError(xrde_chdr_e3, tag_name_ptr, m_latest_lexema_line, L"The name \"%.*s\" is defined as a simple tag, not as a nesting container.", tag_name_len, tag_name);
			return(FALSE);
		}
	}

	// Type of the lexema fits to the type of the tag in the schema. Check situation with the tags duplication.
	if (m_curr_layer->m_tags_present[tag_inx] == TRUE && (dict_item->m_req_dup_mode == xrdm_opt_si || dict_item->m_req_dup_mode == xrdm_req_si))
	{
		ReportXmlError(xrde_chdr_e4, tag_name_ptr, m_latest_lexema_line, L"The %s \"%.*s\" should not be present more than once in \"%.*s\".",
					(lex_type == xlt_tag_attr) ? L"attribute" : L"tag", tag_name_len, tag_name,
					m_curr_layer->m_cnr_name.GetLength(), m_curr_layer->m_cnr_name.GetBodyPtr());
		return(FALSE);
	}

	// This lexema can be used with this tag definition.
	return(TRUE);
}

bool TXmlReader::EnterNesting(TStringPtr &tag_name, TXmlDictRecord *node_dict_item, int tag_dict_inx, TXmlContainerHandler *forcedNode)
{
	// Either the tag dictionary should be passed or node should be explictly forced.
	assert((node_dict_item != NULL && forcedNode == NULL) || (node_dict_item == NULL && forcedNode != NULL));

	// Check if there are still free slots in the stack.
	if (m_nesting_level >= MAX_CNRS_NESTING)
	{
		ReportXmlError(xrde_enns_e1, tag_name, m_latest_lexema_line, L"Nesting of the subcontainers is too deep. The max allowed depth is %d.", MAX_CNRS_NESTING);
		AbortXmlParsing();
		return(FALSE);
	}

	// Prepare the new node. Selecting the new node is not so obvious here.
	TXmlContainerHandler *inner_node = NULL;
	TXmlDictRecord *inner_tags_dict = NULL;
	WORD cnr_tag_id = 0;
	if (forcedNode != NULL)
	{
		if (forcedNode == &s_skip_subtree_handler)
		{
			// Use the same object once again.
			inner_node = &s_skip_subtree_handler;
			inner_tags_dict = g_XmlReader_EmptyTagsDict;
		}
		else
		{
			// Unknown forced node should be used only when the parsing is started.
			assert(m_nesting_level == 0);
			assert(m_nesting_data[0].m_layer_tags != NULL);

			inner_node = forcedNode;
			inner_tags_dict = m_nesting_data[0].m_layer_tags;
			cnr_tag_id = TXmlContainerHandler::ROOT_CONTAINER_TAG_ID;
		}
	}
	else
	{
		// Retrieve the subcontainer handler from the handler of the current container.
		cnr_tag_id = node_dict_item->m_tag_ident;
		inner_node = m_curr_layer->m_layer_node->OpenSubContainer(cnr_tag_id, tag_name);
		if (inner_node != NULL)
		{
			if (inner_node != &s_skip_subtree_handler)
			{
				// Some non dummy handler was retrieved.
				inner_node->SetParser(this);
				inner_tags_dict = (TXmlDictRecord*)node_dict_item->m_inner_schema;
			}
			else
			{
				// App layer decided to skip this subtree without checking its contents.
				inner_tags_dict = g_XmlReader_EmptyTagsDict;
			}
		}
		else
		{
			// Entering the target node failed. Enter the dummy node instead.
			ReportXmlError(xrde_enns_e2, tag_name, m_latest_lexema_line, L"Error getting handler for the subcontainer \"%s\" that resides in the container \"%.*s\".",
						node_dict_item->m_tag_name, m_curr_layer->m_cnr_name.GetLength(), m_curr_layer->m_cnr_name.GetBodyPtr());
			inner_node = &s_skip_subtree_handler;
			inner_tags_dict = g_XmlReader_EmptyTagsDict;
		}
	}

	if (m_abort_flag == TRUE)
	{
		// The error should be already reported.
		return(FALSE);
	}

	// Occupy the slot.
	m_curr_layer = m_nesting_data+m_nesting_level;
	m_nesting_level++;

	m_curr_layer->m_layer_tags = inner_tags_dict;
	m_curr_layer->m_layer_node = inner_node;
	m_curr_layer->m_cnr_tag_id = cnr_tag_id;
	m_curr_layer->m_cnr_name = tag_name;
	m_curr_layer->m_cnr_beg_line = GetLatestLexemaLineNumber();
	m_curr_layer->m_cnr_upper_dict_inx = tag_dict_inx;
	memset(m_curr_layer->m_tags_present, 0, MAX_TAGS_DICT_LEN*sizeof(bool));
	m_curr_layer->m_initial_errs_count = m_errors_count;

	// Notify the new node that it has entered the service. Note that in the "skip subtree" case the app layer will not
	// be called because the handler is a base class object with empty implementaition of this method.
	m_curr_layer->m_layer_node->BeginNodeProcessing(cnr_tag_id);
	return(TRUE);
}

bool TXmlReader::ExitNesting(TStringPtr &closingTagName, bool abort_flag, bool special_unclosed_exit)
{
	assert(m_nesting_level > 0);

	// Compare the opening and closing tag names. Note that name of the closing tag can be not available
	// if the closing tag looks like </>.
	if (special_unclosed_exit == FALSE && closingTagName.GetLength() > 0 && closingTagName != m_curr_layer->m_cnr_name)
	{
		// Names are different. Look for the passed name up in the nesting stack.
		bool missing_closing_tag = FALSE;
		for (int ilv1=m_nesting_level-2; ilv1 >= 0; --ilv1)
		{
			if (closingTagName == m_nesting_data[ilv1].m_cnr_name)
			{
				// The passed name is present on the upper layer. It seems that the closing tag is missing.
				// Exit these unclosed layers. This should reduce the total count of errors.
				missing_closing_tag = TRUE;
				for (int ilv2=m_nesting_level-1; ilv2>ilv1; --ilv2)
				{
					ReportXmlError(xrde_exns_e1, closingTagName, m_latest_lexema_line, L"The closing tag of the container \"%.*s\" that starts on the line %d is missing.",
								m_curr_layer->m_cnr_name.GetLength(), m_curr_layer->m_cnr_name.GetBodyPtr(), m_curr_layer->m_cnr_beg_line);
					ExitNesting(closingTagName, abort_flag, TRUE);
				}
			}
		}

		if (missing_closing_tag == FALSE)
		{
			// Report this issue but do not abort the process.
			ReportXmlError(xrde_exns_e2, closingTagName, m_latest_lexema_line, L"The closing tag \"%.*s\" on the line %d differs from the opening tag \"%.*s\" on the line %d.",
						closingTagName.GetLength(), closingTagName.GetBodyPtr(), m_latest_lexema_line,
						m_curr_layer->m_cnr_name.GetLength(), m_curr_layer->m_cnr_name.GetBodyPtr(), m_curr_layer->m_cnr_beg_line);
		}
	}

	// Check if all required tags are present or not.
	TXmlDictRecord *di = m_curr_layer->m_layer_tags;
	int tag_inx = 0;
	while (di->m_rec_type != xdrt_eof && tag_inx < MAX_TAGS_DICT_LEN)
	{
		if ((di->m_req_dup_mode == xrdm_req_si || di->m_req_dup_mode == xrdm_req_mi) && m_curr_layer->m_tags_present[tag_inx] == FALSE)
		{
			ReportXmlError(xrde_exns_e3, closingTagName, m_latest_lexema_line, L"The required %s \"%s\" is missing in the container \"%.*s\".",
						(di->m_rec_type != xdrt_attr) ? L"tag" :  L"tag attribute", di->m_tag_name,
						m_curr_layer->m_cnr_name.GetLength(), m_curr_layer->m_cnr_name.GetBodyPtr());
		}

		di++;
		tag_inx++;
	}

	// Give a chance to the app layer to tell what it thinks. The app layer is responsible for destroying the node
	// object if this is needed. Note that in the "skip subtree" case of the app layer will not be called because handler
	// is a base class object that has empty implementaition of this method.
	bool node_success = (abort_flag == FALSE && m_curr_layer->m_initial_errs_count == m_errors_count);
	m_curr_layer->m_layer_node->FinalizeNodeProcessing(m_curr_layer->m_cnr_tag_id, (node_success == TRUE) ? FALSE : TRUE);
	int cnr_inx = m_curr_layer->m_cnr_upper_dict_inx;

	// Free the slot.
	m_nesting_level--;
	m_curr_layer--;
	if (m_nesting_level <= 0)
	{
		m_curr_layer = NULL;
	}

	// Put the tag present mark if possible.
	if (cnr_inx >= 0 && m_nesting_level > 0)
		m_curr_layer->m_tags_present[cnr_inx] = TRUE;

	// Return the current state of the abort flag.
	return(m_abort_flag == FALSE);
}

bool TXmlReader::AssembleBoolean(TStringPtr &tagValue, bool &value)
{
	if (AssembleBooleanInternal(tagValue.GetBodyPtr(), tagValue.GetLength(), value) == FALSE)
	{
		ReportXmlError(xrde_bbvl_e1, tagValue, m_latest_lexema_line, L"Error getting a bool value from the string \"%.*s\".", tagValue.GetLength(), tagValue.GetBodyPtr());
		return(FALSE);
	}

	return(TRUE);
}

bool TXmlReader::AssembleInteger(TStringPtr &tagValue, __int64 &value)
{
	wchar_t buffer[120];
	if (tagValue.GetLength() < 120)
	{
		tagValue.CopyToVerifiedBuffer(buffer);
		if (AssembleInt64(buffer, value, anxvt_signed, L"_.:") == TRUE)
		{
			// Conversion succeeded.
			return(TRUE);
		}
	}

	ReportXmlError(xrde_bnvl_e1, tagValue, m_latest_lexema_line, L"Error assembling a signed 64 bit integer from \"%.*s\".", tagValue.GetLength(), tagValue.GetBodyPtr());
	return(FALSE);
}

bool TXmlReader::AssembleGuid(TStringPtr &tagValue, GUID &value)
{
	if (AssembleGuidInternal(tagValue.GetBodyPtr(), tagValue.GetLength(), value) == FALSE)
	{
		ReportXmlError(xrde_bgvl_e1, tagValue, m_latest_lexema_line, L"Error assembling guid from \"%.*s\".", tagValue.GetLength(), tagValue.GetBodyPtr());
		return(FALSE);
	}
	return(TRUE);
}

bool TXmlReader::AssembleColor(TStringPtr &tagValue, TColor &value)
{
	if (tagValue == L"transp" || tagValue == L"Transp" || tagValue == L"TRANSP")
	{
		value = cs_transparent;
		return(TRUE);
	}
	else if (tagValue == L"transparent" || tagValue == L"Transparent" || tagValue == L"TRANSPARENT")
	{
		value = cs_transparent;
		return(TRUE);
	}

	wchar_t *tagValuePtr = tagValue.GetBodyPtr();
	int tagValueLen = tagValue.GetLength();

	wchar_t *red_ptr, *green_ptr, *blue_ptr;
	int red_len, green_len, blue_len;
	BYTE red_value, green_value, blue_value;

	// Extract the RED string.
	red_ptr = tagValuePtr;
	tagValuePtr = SkipColorComponent(tagValuePtr, tagValueLen, red_len);
	if (tagValuePtr == NULL)
	{
		ReportXmlError(xrde_clvl_e1, tagValue, m_latest_lexema_line, L"The comma separator is missing after the RED color component.");
		return(FALSE);
	}

	// Extract the GREEN string.
	green_ptr = tagValuePtr;
	tagValuePtr = SkipColorComponent(tagValuePtr, tagValueLen, green_len);
	if (tagValuePtr == NULL)
	{
		ReportXmlError(xrde_clvl_e2, tagValue, m_latest_lexema_line, L"The comma separator is missing after the GREEN color component.");
		return(FALSE);
	}

	// The rest of the tag value should be a BLUE substring.
	blue_ptr = tagValuePtr;
	blue_len = tagValueLen;

	// Convert contents of the strings into numbers.
	if (AssembleColorComponent(red_ptr, red_len, red_value, L"RED") == FALSE)
		return(FALSE);
	else if (AssembleColorComponent(green_ptr, green_len, green_value, L"GREEN") == FALSE)
		return(FALSE);
	else if (AssembleColorComponent(blue_ptr, blue_len, blue_value, L"BLUE") == FALSE)
		return(FALSE);

	// Success.
	value = RGB(red_value, green_value, blue_value);
	return(TRUE);
}

bool TXmlReader::AssembleBooleanInternal(wchar_t *val_repr, int val_repr_len, bool &value)
{
	if (val_repr_len == 1)
	{
		if (*val_repr == L'0' || *val_repr == L'N' || *val_repr == L'n' || *val_repr == L'F' || *val_repr == L'f')
		{
			value = FALSE;
			return(TRUE);
		}
		else if (*val_repr == L'1' || *val_repr == L'Y' || *val_repr == L'y' || *val_repr == L'T' || *val_repr == L't')
		{
			value = TRUE;
			return(TRUE);
		}
	}
	else if (val_repr_len == 2)
	{
		if (_wcsnicmp(val_repr, L"No", 2) == 0)
		{
			value = FALSE;
			return(TRUE);
		}
		else if (_wcsnicmp(val_repr, L"On", 2) == 0)
		{
			value = TRUE;
			return(TRUE);
		}
	}
	else if (val_repr_len == 3)
	{
		if (_wcsnicmp(val_repr, L"Yes", 3) == 0)
		{
			value = TRUE;
			return(TRUE);
		}
		else if (_wcsnicmp(val_repr, L"Off", 3) == 0)
		{
			value = FALSE;
			return(TRUE);
		}
	}
	else if (val_repr_len == 4)
	{
		if (_wcsnicmp(val_repr, L"True", 4) == 0)
		{
			value = TRUE;
			return(TRUE);
		}
	}
	else if (val_repr_len == 5)
	{
		if (_wcsnicmp(val_repr, L"False", 5) == 0)
		{
			value = FALSE;
			return(TRUE);
		}
	}

	// None of the checks succeeded. Return the conversion failure.
	return(FALSE);
}

bool TXmlReader::AssembleGuidInternal(wchar_t *val_repr, int val_repr_len, GUID &value)
{
	int cnt_hex_digits = 0;
	DWORD val = 0;
	DWORD hex_val;

	for (int inx=0; inx<val_repr_len; ++inx)
	{
		wchar_t ch = val_repr[inx];
		if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n')
			continue;
		if (ch == L'-' || ch == L'{' || ch == L'}')
			continue;

		if (ch >= L'0' && ch <= L'9')
		{
			hex_val = ch - L'0';
		}
		else if (ch >= L'A' && ch <= L'F')
		{
			hex_val = ch - L'A' + 10;
		}
		else if (ch >= L'a' && ch <= L'f')
		{
			hex_val = ch - L'a' + 10;
		}
		else
		{
			// This is an illegal character.
			return(FALSE);
		}

		// The hex digit was successfully retrieved.
		val = (val << 4) | hex_val;
		cnt_hex_digits++;

		if (cnt_hex_digits == 8)
		{
			value.Data1 = val;
			val = 0;
		}
		else if (cnt_hex_digits == 12)
		{
			value.Data2 = (WORD)val;
			val = 0;
		}
		else if (cnt_hex_digits == 16)
		{
			value.Data3 = (WORD)val;
			val = 0;
		}
		else if (cnt_hex_digits > 16 && (cnt_hex_digits & 1) == 0)
		{
			int ib = (cnt_hex_digits-18) >> 1;
			if (ib >= 8)
			{
				// Too many hex digits.
				return(FALSE);
			}

			value.Data4[ib] = (BYTE)val;
			val = 0;
		}
	}

	if (cnt_hex_digits != 32)
		return(FALSE);

	// The guid was successfully assembled.
	return(TRUE);
}

wchar_t *TXmlReader::SkipColorComponent(wchar_t *val_repr, int &val_repr_len, int &componentValueLen)
{
	// Skip chars up to the comma.
	componentValueLen = 0;
	while (val_repr_len > 0 && *val_repr != L',')
	{
		val_repr++;
		val_repr_len--;
		componentValueLen++;
	}

	if (val_repr_len == 0)
	{
		// The comma is missing.
		return(NULL);
	}

	// Success.
	val_repr++;
	val_repr_len--;
	return(val_repr);
}

bool TXmlReader::AssembleColorComponent(wchar_t *componentValue, int componentValueLen, BYTE &value, const wchar_t *componentName)
{
	if (componentValueLen < 40)
	{
		wchar_t buffer[40];
		wcsncpy(buffer, componentValue, 40);
		buffer[componentValueLen] = 0;

		__int64 val;
		if (AssembleInt64(buffer, val, anxvt_signed, L"_.:") == TRUE)
		{
			if (val >= 0 && val <= 255)
			{
				value = (BYTE)val;
				return(TRUE);
			}
		}
	}

	TStringPtr error_area(componentValue, componentValueLen);
	ReportXmlError(xrde_clvl_e3, error_area, m_latest_lexema_line, L"Error assembling the \"%s\" color component from \"%.*s\".",
					componentName, componentValueLen, componentValue);

	return(FALSE);
}


