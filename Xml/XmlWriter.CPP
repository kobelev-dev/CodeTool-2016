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
#include  "Xml/XmlWriter.H"

//-----------------------------------------------------------------------
//  ===================  TXmlWriter  =========================
//-----------------------------------------------------------------------

static TXmlDictRecord g_XmlWriter_DummyTagsDict[1] =
{
	{ xdrt_none, xrdm_opt_si, xtbt_none },
};

const TXmlCnrWritingProps g_XmlWriter_StdRootContainerProps =
{
	FALSE,					// Do not use short tag close form (n/a).
	FALSE, FALSE,				// Tag attrs are on one same line (n/a).
	TRUE, TRUE,				// All leaf tags start from the new line.
	TRUE, TRUE,				// Subcontainers start from the new line.
	FALSE,					// No CrLf before tag header close.
	FALSE,					// Do not add CrLf before cnr close. (n/a).
	FALSE,					// No crlf after tag attrs (n/a).
	TRUE, TRUE,				// Add crlf after leaf fields and subcnrs.
};

const TXmlCnrWritingProps g_XmlWriter_StdSubContainerProps =
{
	FALSE,					// Do not use short tag close form.
	FALSE, FALSE,				// Tag attrs are on one same line.
	TRUE, TRUE,				// All leaf tags start from the new line.
	TRUE, TRUE,				// Subcontainers start from the new line.
	FALSE,					// No CrLf before tag header close.
	TRUE,					// Add CrLf before container close.
	FALSE,					// No crlf after tag attrs.
	FALSE, FALSE,				// No crlf after leaf fields and subcnrs.
};

static const wchar_t g_20_tabs_line[] = L"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static const wchar_t g_40_spaces_line[] = L"                                        ";

TXmlWriter::TXmlWriter(const wchar_t *streamName, TGenericConsole *cons, int lineIndent)
	: TDestinationFile(streamName)
{
	m_root_cnrs_dict = NULL;

	m_nesting_level = 0;
	m_curr_layer = NULL;

	// Max indentation should not exceed the length of the g_40_spaces_line array.
	m_line_indent = (lineIndent < 40) ? ((lineIndent >= 0) ? lineIndent : -1) : 40;
	m_line_dirty = FALSE;

	m_console = cons;
	m_errors_buffer = NULL;
	m_errors_count = 0;
}

TXmlWriter::TXmlWriter(const wchar_t *streamName, TMessageBuffer &errorsBuffer, int lineIndent)
	: TDestinationFile(streamName)
{
	m_root_cnrs_dict = NULL;

	m_nesting_level = 0;
	m_curr_layer = NULL;

	// Max indentation should not exceed the length of the g_40_spaces_line array.
	m_line_indent = (lineIndent < 40) ? ((lineIndent >= 0) ? lineIndent : -1) : 40;
	m_line_dirty = FALSE;

	m_console = NULL;
	m_errors_buffer = &errorsBuffer;
	m_errors_count = 0;
}

TXmlWriter::~TXmlWriter()
{
	if (m_nesting_level > 0)
	{
		// One or more nesting layers were not closed.
		if (IsWritingState() == FALSE)
		{
			ReportError(xwre_dctr_e1, L"The low level writing is closed while some of the nesting levels are still opened.");
		}
		else
		{
			ReportError(xwre_dctr_e2, L"Destroying the XML writer when some of the nesting layers are still opened.");
		}

		if (IsWritingState() == TRUE)
			CloseAllContainers();
	}

	if (IsWritingState() == TRUE)
	{
		// Document is still in the writing state. It makes sense to close it.
		Close();
	}
}

void TXmlWriter::SetupRootContainersDict(const TXmlDictRecord *tagsDict, const TXmlCnrWritingProps *rootContainerProps, const TXmlCnrWritingProps *subContainerProps)
{
	assert(tagsDict != NULL);

	m_root_cnrs_dict = (TXmlDictRecord*)tagsDict;

	// Setup the container props. This is the only one place where they can be set.
	m_root_cnr_props = (rootContainerProps != NULL) ? *rootContainerProps : g_XmlWriter_StdRootContainerProps;
	m_sub_cnr_props = (subContainerProps != NULL) ? *subContainerProps : g_XmlWriter_StdSubContainerProps;
}

void TXmlWriter::WriteXmlHeader(const wchar_t *hdrContents, bool append_crlf)
{
	// Header can be written only when containers are not opened.
	assert(m_nesting_level == 0);

	if (hdrContents != NULL)
	{
		Write(L"<?xml ");
		WriteRawData(hdrContents, wcslen(hdrContents));
		Write(L" ?>");
	}
	else
	{
		Write(L"<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
	}

	m_line_dirty = TRUE;
	if (append_crlf == TRUE)
		AddLineFeedInternal();
}

void TXmlWriter::WriteXmlComment(const wchar_t *commentFrontDelim, const wchar_t *commentBody, TXmlItemCrLfMode crlfMode)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	if (commentBody == 0 || commentBody[0] == 0)
		return;

	CloseTagHeaderIfNeeded();
	if ((crlfMode & crlf_prepend_MASK) == crlf_prepend_add)
		LineFeedAndIndent();
	else if (m_line_dirty == FALSE)
		IndentLine();

	wchar_t delim_buff[sizeof(g_40_spaces_line)/sizeof(wchar_t)+1];
	if (commentFrontDelim == NULL)
	{
		// Caller has not passed the explicit delimiter.
		if (m_line_dirty == TRUE)
		{
			// Use current indentation as a delimiter.
			if (m_line_indent < 0)
			{
				commentFrontDelim = L"\t";
			}
			else if (m_line_indent == 0)
			{
				commentFrontDelim = L" ";
			}
			else
			{
				wcscpy(delim_buff, g_40_spaces_line);
				delim_buff[m_line_indent] = 0;
				commentFrontDelim = delim_buff;
			}
		}
		else
		{
			commentFrontDelim = L"";
		}
	}

	if (commentFrontDelim != NULL && commentFrontDelim[0] != 0)
		WriteRawData(commentFrontDelim, wcslen(commentFrontDelim));

	Write(L"<!-- ");
	m_line_dirty = TRUE;

	WriteBodyData(commentBody, TRUE);
	if (m_line_dirty == FALSE)
		IndentLine();

	Write((m_line_dirty == TRUE) ? L" -->" : L"-->");
	m_line_dirty = TRUE;

	if ((crlfMode & crlf_append_MASK) == crlf_append_add)
		AddLineFeedInternal();
}

void TXmlWriter::CloseTagHeaderIfNeeded()
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	if (m_curr_layer->m_header_closed == FALSE)
	{
		// Close the tag header.
		if (m_curr_layer->m_attrs_present == TRUE && m_curr_layer->m_props->m_crlf_before_tag_hdr_close == TRUE)
			LineFeedAndIndent();

		Write(L">");
		m_line_dirty = TRUE;

		// Put the mark that header is closed.
		m_curr_layer->m_header_closed = TRUE;
	}
}

void TXmlWriter::LineFeedAndIndent(bool soft_line_feed)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	AddLineFeedInternal(soft_line_feed);
	IndentLine();
}

void TXmlWriter::OpenContainer(WORD tag_id)
{
	TXmlDictRecord *cnrInfo = NULL;
	TXmlCnrWritingProps *props = &m_sub_cnr_props;

	if (m_nesting_level == 0)
	{
		// Look for this tag in the dictionary of the allowed root containers.
		cnrInfo = LookupRootContainerInfo(tag_id);
		props = &m_root_cnr_props;
	}
	else if (m_nesting_level < MAX_CNRS_NESTING)
	{
		// Look for this tag in the dictionary of the current nesting layer.
		cnrInfo = LookupTagInfo(tag_id, TRUE);
	}

	if (cnrInfo == NULL)
	{
		// The information about the passed tagId is missing. An error is already reported.
		// Nevertheless it is better to open the new layer to avoid breaking the nesting structure.
		wchar_t buffer[80];
		swprintf(buffer, 80, L"@@@-UnknCnr-TagId%d-@@@", tag_id);
		OpenContainerInternal(buffer, g_XmlWriter_DummyTagsDict, props, crlf_add_add);
	}
	else
	{
		// Use data from the dict item to call the protected method.
		OpenContainerInternal(cnrInfo->m_tag_name, cnrInfo->m_inner_schema, props, crlf_add_add);
	}
}

void TXmlWriter::CloseContainer()
{
	if (m_nesting_level <= 0)
	{
		ReportError(xwre_clcn_e1, L"Containers nesting underflow.");
		return;
	}
	else if (m_nesting_level > MAX_CNRS_NESTING)
	{
		// Do not emit anything in case of the nesting overflow. Nevertheless decrement the nesting level.
		m_nesting_level--;
		if (m_nesting_level <= MAX_CNRS_NESTING)
			m_curr_layer = m_nesting_data+MAX_CNRS_NESTING-1;
		return;
	}

	// Check if all required attrs, fields and subcontainers were emitted or not.
	TXmlDictRecord *pDict = m_curr_layer->m_curr_dict;
	for (int inx=0; inx<MAX_TAGS_DICT_LEN; ++inx, ++pDict)
	{
		if (pDict->m_rec_type == xdrt_eof)
			break;

		if (pDict->m_req_dup_mode == xrdm_req_si || pDict->m_req_dup_mode == xrdm_req_mi)
		{
			if (m_curr_layer->m_tags_emitted[inx] == FALSE)
			{
				wchar_t buffer[160];
				swprintf(buffer, 160, L"An object \"%s\" with the tagId %hu is required for the container \"%s\".", pDict->m_tag_name, pDict->m_tag_ident, m_curr_layer->m_layer_name);
				ReportError(xwre_clcn_e2, buffer);
			}
		}
	}

	// Find out if the crlf prepending is needed.
	bool crlf_before;
	if (m_curr_layer->m_header_closed == FALSE)
	{
		// The tag header is not closed yet.
		if (m_curr_layer->m_attrs_present == TRUE)
			crlf_before = m_curr_layer->m_props->m_crlf_before_tag_hdr_close;
		else
			crlf_before = FALSE;
	}
	else
	{
		// Tag header is closed or this is a nesting overflow case.
		crlf_before = m_curr_layer->m_props->m_crlf_before_container_close;
	}

	// Save pointer to the props of the current layer before dismissing it.
	TXmlWriterLayerInfo *prev_cli = m_curr_layer;

	// Modify the data structure.
	m_nesting_level--;
	m_curr_layer--;
	if (m_nesting_level == 0)
		m_curr_layer = NULL;

	// Process prepending.
	if (crlf_before == TRUE)
		LineFeedAndIndent();
	else if (m_line_dirty == FALSE)
		IndentLine();

	// Emit the closing sequence.
	if (prev_cli->m_header_closed == FALSE)
	{
		Write(L"/>");
	}
	else
	{
		WriteFmt(L"</%s>", prev_cli->m_layer_name);
	}

	m_line_dirty = TRUE;

	// Add final crlf if the stars tell to do this.
	AddLineFeedIfNecessary(xdrt_cnr, prev_cli->m_append_crlf);
}

void TXmlWriter::CloseAllContainers()
{
	while (m_nesting_level > 0)
		CloseContainer();
}

void TXmlWriter::CheckAllContainersClosed()
{
	if (m_nesting_level > 0)
	{
		wchar_t buffer[80];
		swprintf(buffer, 80, L"Some of the subcontainers (%d) were not closed.", m_nesting_level);
		ReportError(xwre_chcl_e1, buffer);

		// Since the names of containers are stored, this method will close subcontainers using their right names.
		CloseAllContainers();
	}
}

void TXmlWriter::WriteBoolTag(WORD tagId, bool value, TXmlWriterBoolStyle tagStyle)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	// Use the most generic procedure that treats everything that is not zero as TRUE.
	switch (tagStyle)
	{
		case xwbs_yes_no:
				WriteTaggedObject(tagId, xtbt_bool, (value != 0) ? L"Yes" : L"No");
				break;

		case xwbs_on_off:
				WriteTaggedObject(tagId, xtbt_bool, (value != 0) ? L"On" : L"Off");
				break;

		case xwbs_1_0:
				WriteTaggedObject(tagId, xtbt_bool, (value != 0) ? L"1" : L"0");
				break;

		default:
			WriteTaggedObject(tagId, xtbt_bool, (value != 0) ? L"True" : L"False");
			break;
	}
}

void TXmlWriter::WriteNumTag(WORD tagId, __int64 value, wchar_t thousandsSeparatorChar)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	wchar_t buffer[40];
	FormatInt64(value, buffer, 40, fnms_dec_signed, thousandsSeparatorChar);
	WriteTaggedObject(tagId, xtbt_number, buffer);
}

void TXmlWriter::WriteStringTag(WORD tagId, const wchar_t *value, int maxStringLen)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	if (value == NULL)
		value = L"";

	if (maxStringLen > 0)
	{
		// App layer asked to verify the length of the passed string.
		int inx = 0;
		while (inx < maxStringLen)
		{
			if (value[inx] == 0)
				break;
			inx++;
		}

		if (inx >= maxStringLen)
		{
			wchar_t buffer[160];
			swprintf(buffer, 160, L"String for the tag with id %hu in the container %s exceeds its max allowed length of %d.", tagId, m_curr_layer->m_layer_name, maxStringLen);
			ReportError(xwre_chst_e1, buffer);
			return;
		}
	}

	// Data preparation is not needed.
	WriteTaggedObject(tagId, xtbt_string, value);
}

void TXmlWriter::WriteGuidTag(WORD tagId, GUID &value)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	// Use stnd registry style formatting.
	wchar_t buffer[80];
	WriteTaggedObject(tagId, xtbt_guid, FormatGuid(value, buffer, 80));
}

void TXmlWriter::WriteColorTag(WORD tagId, TColor value)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	wchar_t buffer[80];
	if (value == cs_transparent)
	{
		wcscpy(buffer, L"Transp");
	}
	else
	{
		// Use decimal representation of the color components.
		swprintf(buffer, 80, L"%d,%d,%d", value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF);
	}

	WriteTaggedObject(tagId, xtbt_color, buffer);
}

void TXmlWriter::WriteNamedTag(const wchar_t *tagName, const wchar_t *tagValue, TXmlItemCrLfMode crlfMode)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	WriteScalarObject(xdrt_field, tagName, tagValue, crlfMode);
}

void TXmlWriter::WriteNamedTagAttr(const wchar_t *attrName, const wchar_t *attrValue, TXmlItemCrLfMode crlfMode)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	WriteScalarObject(xdrt_attr, attrName, attrValue, crlfMode);
}

void TXmlWriter::WriteContainerBody(const wchar_t *value, TXmlItemCrLfMode crlfMode)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	// Check the length of the body.
	if (value == NULL || value[0] == 0)
		return;

	if ((crlfMode & crlf_prepend_MASK) == crlf_prepend_add)
		AddLineFeedInternal();

	// Writing the body data will set/reset the m_line_dirty depending on the crlfs that may be present in the data.
	WriteBodyData(value, FALSE);

	if ((crlfMode & crlf_append_MASK) == crlf_append_add)
		AddLineFeedInternal();
}

void TXmlWriter::ReportError(WORD errCode, const wchar_t *message)
{
	wchar_t buffer[160];
	swprintf(buffer, 160, L"E%03hu: %s", errCode, message);

	if (m_console != NULL)
	{
		// Send the string to the callback.
		m_console->ShowRunTimeError(buffer);
	}
	else if (m_errors_buffer != NULL)
	{
		// Write this line to the buffer.
		m_errors_buffer->Append(buffer);
		m_errors_buffer->Append(L"\r\n");
	}

	m_errors_count++;
}

TXmlDictRecord *TXmlWriter::LookupRootContainerInfo(WORD tagId)
{
	if (tagId == 0)
	{
		ReportError(xwre_lkrc_e1, L"Root containers with the tagId equal to ZERO are not allowed.");
		return(NULL);
	}
	else if (m_root_cnrs_dict == NULL)
	{
		ReportError(xwre_lkrc_e2, L"The directory of the known root containers is not set.");
		return(NULL);
	}

	TXmlDictRecord *pDict = m_root_cnrs_dict;
	while (pDict->m_rec_type != xdrt_eof)
	{
		if (pDict->m_rec_type == xdrt_cnr && pDict->m_tag_ident == tagId)
		{
			assert(pDict->m_inner_schema != NULL);
			return(pDict);
		}

		pDict++;
	}

	wchar_t buffer[120];
	swprintf(buffer, 120, L"Container with id %d is not defined in the current directory of the root containers.", tagId);
	ReportError(xwre_lkrc_e3, buffer);
	return(NULL);
}

TXmlDictRecord *TXmlWriter::LookupTagInfo(WORD tag_id, bool container_tag)
{
	assert(m_nesting_level > 0 && m_nesting_level < MAX_CNRS_NESTING);

	if (m_curr_layer->m_curr_dict == g_XmlWriter_DummyTagsDict)
	{
		// This is special case when the tags dict is not a real dict, but it is a dummy list that is used when the unknown
		// tree is entered. In this case the null pointer should be returned without any error message.
		return(NULL);
	}

	if (tag_id == 0)
	{
		// Do not search for anything for this Id.
		ReportError(xwre_lktg_e1, L"Objects with the tagId equla to ZERO are not allowed.");
		return(NULL);
	}

	if (m_curr_layer->m_curr_dict->m_rec_type == xdrt_eof)
	{
		// Tags dictionary is empty.
		ReportError(xwre_lktg_e2, L"Attempt to search for tagId in the empty dictionary of tags.");
		return(NULL);
	}

	// Start either from the latest good tag or from the beginning of the dict if the container was just opened
	// or the tags were not used yet.
	int tag_desc_inx = m_curr_layer->m_latest_dict_inx;
	TXmlDictRecord *pDict = m_curr_layer->m_curr_dict+tag_desc_inx;
	for (int cntTags=0;  cntTags < MAX_TAGS_DICT_LEN; ++cntTags)
	{
		if (pDict->m_tag_ident == tag_id)
		{
			//
			// Id of the tag matched the field of the current record. Check properties of this tag definition.
			//
			if (container_tag == FALSE)
			{
				if (pDict->m_rec_type != xdrt_attr && pDict->m_rec_type != xdrt_atrfl && pDict->m_rec_type != xdrt_field)
				{
					// This tagId cannot be used for writing the scalar field.
					wchar_t buffer[120];
					swprintf(buffer, 120, L"The tag \"%s\" cannot be used for writing the tag attribute or the field tag.", pDict->m_tag_name);
					ReportError(xwre_lktg_e3, buffer);
					return(NULL);
				}
			}
			else
			{
				if (pDict->m_rec_type != xdrt_cnr)
				{
					// This tagId cannot be used for opening the subcontainer.
					wchar_t buffer[120];
					swprintf(buffer, 120, L"The tag \"%s\" cannot be used for opening the subcontainer.", pDict->m_tag_name);
					ReportError(xwre_lktg_e4, buffer);
					return(NULL);
				}
				else if (pDict->m_inner_schema == NULL)
				{
					// This tagId cannot be used for opening the subcontainer.
					wchar_t buffer[120];
					swprintf(buffer, 120, L"The tag \"%s\" describes subcontainer that does not have an inner tags dict.", pDict->m_tag_name);
					ReportError(xwre_lktg_e5, buffer);
					return(NULL);
				}
			}

			if (pDict->m_req_dup_mode == xrdm_req_si || pDict->m_req_dup_mode == xrdm_opt_si)
			{
				// This tag cannot be present more than once in the current container.
				if (m_curr_layer->m_tags_emitted[tag_desc_inx] == TRUE)
				{
					wchar_t buffer[120];
					swprintf(buffer, 120, ((container_tag == TRUE) ?
											L"The schema of the current container allows only one instance of the subcontainer \"%s\"." :
											L"The schema of the current container allows only one instance of the tag or tag attr \"%s\"."), pDict->m_tag_name);
					ReportError(xwre_lktg_e6, buffer);
					return(NULL);
				}
			}

			// Mark this tag as mentioned.
			m_curr_layer->m_tags_emitted[tag_desc_inx] = TRUE;

			// Return this tag as a valid tag for this container.
			m_curr_layer->m_latest_dict_inx = tag_desc_inx;
			return(pDict);
		}

		// Shift to the next tag description.
		tag_desc_inx++;
		pDict++;

		if (pDict->m_rec_type == xdrt_eof)
		{
			// Wrap the loop to the beginning of the dict.
			tag_desc_inx = 0;
			pDict = m_curr_layer->m_curr_dict;
		}

		// Check the loop termination condition.
		if (tag_desc_inx == m_curr_layer->m_latest_dict_inx)
		{
			// Tag definition for the passed id is not present in the current dict.
			wchar_t buffer[120];
			swprintf(buffer, 120, L"The tag with id %d is not defined in the schema of the container \"%s\".", tag_id, m_curr_layer->m_layer_name);
			ReportError(xwre_lktg_e7, buffer);
			return(NULL);
		}
	}

	// The tags dict is too long.
	wchar_t buffer[120];
	swprintf(buffer, 120, L"The tags dictionary of the \"%s\" is too long.", m_curr_layer->m_layer_name);
	ReportError(xwre_lktg_e8, buffer);
	return(NULL);
}

void TXmlWriter::OpenContainerInternal(const wchar_t *containerName, const TXmlDictRecord *tagsDict, TXmlCnrWritingProps *props, TXmlItemCrLfMode crlfMode)
{
	if (m_nesting_level >= MAX_CNRS_NESTING)
	{
		// Nesting layers overflow. Nevertheless coninue incrementing the levels count.
		m_nesting_level++;
		m_curr_layer = NULL;
		ReportError(xwre_opcn_e1, L"The nesting of the subcontainers is too deep.");
		return;
	}

	// Check the name of container.
	if (wcslen(containerName) >= MAX_TAG_NAME_LEN)
	{
		ReportError(xwre_opcn_e2, L"The name of the container is too long.");
		containerName = L"@@@-CnrNameLenghOverflow-@@@";
	}

	// Process prepending before changing the nesting levels count.
	PrepareForWritingObject(xdrt_cnr, crlfMode);

	// Modify the data structure.
	if (m_nesting_level == 0)
	{
		m_curr_layer = m_nesting_data;
		m_nesting_level = 1;
	}
	else
	{
		m_nesting_level++;
		m_curr_layer++;
	}

	// Fill in the layer info.
	memset(m_curr_layer, 0, sizeof(TXmlWriterLayerInfo));
	m_curr_layer->m_curr_dict = (TXmlDictRecord*)tagsDict;
	m_curr_layer->m_props = props;
	wcscpy(m_curr_layer->m_layer_name, containerName);
	m_curr_layer->m_append_crlf = crlfMode;

	// Emit the container header.
	WriteFmt(L"<%s", m_curr_layer->m_layer_name);
	m_line_dirty = TRUE;
}

void TXmlWriter::WriteTaggedObject(WORD tagId, TXmlItemBodyType expectedTagBodyType, const wchar_t *tagValue)
{
	//
	// This is a low level method. All typed tag writers come here.
	//
	TXmlDictRecord *tagInfo = LookupTagInfo(tagId, FALSE);
	if (tagInfo == NULL)
	{
		// Error is already reported.
		return;
	}

	if (tagInfo->m_body_type != expectedTagBodyType)
	{
		// Application is using the wrong access method to write the tag.
		wchar_t buffer[120];
		swprintf(buffer, 120, L"Container schema of the \"%s\" defines a different body type for the tag \"%s\".", m_curr_layer->m_layer_name, tagInfo->m_tag_name);
		ReportError(xwre_wrtg_e1, buffer);
		return;
	}

	// Everything went fine.
	WriteScalarObject(tagInfo->m_rec_type, tagInfo->m_tag_name, tagValue, crlf_inherit);
}

void TXmlWriter::WriteScalarObject(TXmlDictRecordType objectType, const wchar_t *tagName, const wchar_t *tagValue, TXmlItemCrLfMode crlfMode)
{
	assert(tagName != NULL && tagName[0] != 0);

	// Check the state. Do this before starting a new line.
	if ((objectType == xdrt_attr || objectType == xdrt_atrfl) && m_curr_layer->m_header_closed == TRUE)
	{
		// The tag header cannot be reopened back.
		wchar_t buffer[120];
		swprintf(buffer, 120, L"Attempt to write the tag attribute \"%s\" after writing a field or subcontainer.", tagName);
		ReportError(xwre_wrsc_e1, buffer);
		return;
	}

	// Close the tag header if needed and add CrLf is reqeusted.
	PrepareForWritingObject(objectType, crlfMode);

	if (objectType == xdrt_attr || objectType == xdrt_atrfl)
	{
		// Upper layer wants to write the tag attribute.
		WriteFmt(L"%s%s=\"", ((m_line_dirty == TRUE) ? L" " : L""), tagName);
		if (tagValue != NULL && tagValue[0] != 0)
			WriteRawData(tagValue, wcslen(tagValue));

		Write(L"\"");
		m_curr_layer->m_attrs_present = TRUE;
	}
	else if (objectType == xdrt_field)
	{
		// Upper layer wants to write the data field.
		if (tagValue != NULL && tagValue[0] != 0)
		{
			WriteFmt(L"<%s>", tagName);
			WriteRawData(tagValue, wcslen(tagValue));

			if (m_curr_layer->m_props->m_use_short_tag_close == TRUE)
				Write(L"</>");
			else WriteFmt(L"</%s>", tagName);
		}
		else
		{
			// This is an empty tag.
			WriteFmt(L"<%s/>", tagName);
		}

		m_curr_layer->m_tags_present = TRUE;
	}
	else
	{
		// Bogus object type.
		assert(FALSE);
	}

	m_line_dirty = TRUE;

	// Add final crlf if the stars ask to do this.
	AddLineFeedIfNecessary(objectType, crlfMode);
}

void TXmlWriter::PrepareForWritingObject(TXmlDictRecordType objectType, TXmlItemCrLfMode prependCrlfMode)
{
	if (m_nesting_level <= 0 || m_nesting_level > MAX_CNRS_NESTING)
		return;

	// Check the tag header state.
	if (objectType != xdrt_attr && objectType != xdrt_atrfl)
		CloseTagHeaderIfNeeded();

	// Add CrLf in front of the object if needed.
	prependCrlfMode = (TXmlItemCrLfMode)(prependCrlfMode & crlf_prepend_MASK);
	bool add_crlf;
	if (prependCrlfMode == crlf_prepend_inherit)
	{
		// Pick up the value from the props of the current container.
		if (objectType == xdrt_attr || objectType == xdrt_atrfl)
		{
			add_crlf = (m_curr_layer->m_attrs_present == TRUE) ?
								m_curr_layer->m_props->m_crlf_before_first_attr :
								m_curr_layer->m_props->m_crlf_before_other_attr;
		}
		else if (objectType == xdrt_field)
		{
			add_crlf = (m_curr_layer->m_tags_present == TRUE) ?
								m_curr_layer->m_props->m_crlf_before_first_field :
								m_curr_layer->m_props->m_crlf_before_other_field;
		}
		else
		{
			assert(objectType == xdrt_cnr);
			add_crlf = (m_curr_layer->m_tags_present == TRUE) ?
								m_curr_layer->m_props->m_crlf_before_first_subcnr :
								m_curr_layer->m_props->m_crlf_before_other_subcnr;
		}
	}
	else
	{
		// Use the param value.
		add_crlf = (prependCrlfMode == crlf_prepend_add) ? TRUE : FALSE;
	}

	if (add_crlf == TRUE)
	{
		LineFeedAndIndent();
	}
	else if (m_line_dirty == FALSE)
	{
		IndentLine();
	}
}

void TXmlWriter::AddLineFeedIfNecessary(TXmlDictRecordType objectTtype, TXmlItemCrLfMode appendCrlfMode)
{
	// Add CrLf after the object if needed.
	appendCrlfMode = (TXmlItemCrLfMode)(appendCrlfMode & crlf_append_MASK);

	bool add_crlf;
	if (appendCrlfMode == crlf_append_inherit)
	{
		// Pick up the value from the props of the current container.
		if (objectTtype == xdrt_attr || objectTtype == xdrt_atrfl)
		{
			add_crlf = m_curr_layer->m_props->m_crlf_after_attr;
		}
		else if (objectTtype == xdrt_field)
		{
			add_crlf = m_curr_layer->m_props->m_crlf_after_field;
		}
		else
		{
			assert(objectTtype == xdrt_cnr);
			add_crlf = m_curr_layer->m_props->m_crlf_after_subcnr;
		}
	}
	else
	{
		// Use the explicitly passed param value.
		add_crlf = (appendCrlfMode == crlf_append_add) ? TRUE : FALSE;
	}

	if (add_crlf == TRUE)
		AddLineFeedInternal();
}

void TXmlWriter::WriteBodyData(const wchar_t *body_data, bool preserve_st_eols)
{
	int body_data_len = (int)wcslen(body_data);
	bool non_emp_chars = FALSE;
	bool crlf_flag = FALSE;
	const wchar_t *plain_text_area = NULL;
	int plain_text_area_len = -1;

	while (body_data_len > 0)
	{
		// Pick up the next character.
		wchar_t ch = *body_data++;
		body_data_len--;

		if (ch == L'\r' || ch == L'\n')
		{
			// Check for plain text area in front of the eol chars and dump them, if any.
			if (plain_text_area != NULL)
			{
				if (m_line_dirty == FALSE)
					IndentLine();

				WriteRawData(plain_text_area, plain_text_area_len);
				m_line_dirty = TRUE;

				plain_text_area = NULL;
				plain_text_area_len = -1;
			}

			if (ch == L'\n')
				crlf_flag = TRUE;
		}
		else
		{
			// The rest of the chars are treated as terminals for now.
			if (plain_text_area == NULL)
			{
				// Start the new term area.
				if ((non_emp_chars == TRUE || preserve_st_eols == TRUE) && crlf_flag == TRUE)
				{
					// This term area is not the first one or the upper layer asked to preserve.
					AddLineFeedInternal();
					crlf_flag = FALSE;
				}

				plain_text_area = body_data-1;
				plain_text_area_len = 1;
			}
			else
			{
				// Just extend the length of existing plain text area.
				plain_text_area_len++;
			}

			non_emp_chars = TRUE;
		}
	}

	if (plain_text_area != NULL)
	{
		// Dump the final non empty area.
		if (m_line_dirty == FALSE)
			IndentLine();

		WriteRawData(plain_text_area, plain_text_area_len);
		m_line_dirty = TRUE;
	}

	if (preserve_st_eols == TRUE && crlf_flag == TRUE)
	{
		// Dump the final crlf.
		AddLineFeedInternal();
	}
}

void TXmlWriter::WriteRawData(const wchar_t *data_beg, long data_len)
{
	int ibeg = 0;
	for (int inx=0; inx<data_len; ++inx)
	{
		const wchar_t *esc_seq = NULL;
		int esc_seq_len;

		switch (data_beg[inx])
		{
			// Check for characters that need HTML escaping.
			case L'&':	esc_seq = L"&amp;";	esc_seq_len = 5;	  break;
			case L'<':	esc_seq = L"&lt;";		esc_seq_len = 4;	  break;
			case L'>':	esc_seq = L"&gt;";		esc_seq_len = 4;	  break;
			case L'\"':	esc_seq = L"&quot;";	esc_seq_len = 6;	  break;
		}

		if (esc_seq != NULL)
		{
			if (ibeg != inx)
				Write(data_beg+ibeg, inx-ibeg);

			Write(esc_seq, esc_seq_len);
			ibeg = inx+1;
		}
	}

	if (ibeg != data_len)
		Write(data_beg+ibeg, data_len-ibeg);
}

void TXmlWriter::AddLineFeedInternal(bool soft_line_feed)
{
	if (soft_line_feed == TRUE && m_line_dirty == FALSE)
	{
		// Do not do anything.
		return;
	}

	// Start a new line.
	WriteLine();
	m_line_dirty = FALSE;
}

void TXmlWriter::IndentLine()
{
	assert(m_line_dirty == FALSE);

	// Check if the indentation is needed or not.
	if (m_nesting_level > 0)
	{
		// Both the props and the current nesting level are asking for indentation.
		if (m_line_indent < 0)
		{
			// Use the tab characters.
			Write(g_20_tabs_line, (m_nesting_level < 20) ? m_nesting_level : 20);
		}
		else if (m_line_indent > 0)
		{
			// Write spaces. Note that indentaion of one layer cannot exceed 40 because this is checked in the ctor.
			for (int ii=0; ii<m_nesting_level; ++ii)
				Write(g_40_spaces_line, m_line_indent);
		}
	}
}


