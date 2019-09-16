//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_ClickEventDetails_H
#define	LangView_ClickEventDetails_H

#ifndef   ScreenItems_XmlFileView_H
#include  "ScreenItems/XmlFileView.H"
#endif
#ifndef   LangView_LangView_H
#include  "LangView/LangView.H"
#endif

class TLangViewClickEventDetailsDialog;

template <int _num_rows>
class TLangViewClickDetailsHelperBase
{
public:

	TLangViewClickDetailsHelperBase() { m_stls = NULL; m_ymax = NULL; }

	void		SetStyles(TLangViewClickEventDetailsDialog *stls) { m_stls = stls; }
				// This helper does not have style objects as data fields. It is using all necessary styles
				// from the passed dialog object.
protected:

	void		PrepareFormatting(POINT &start_end_point);
	void		PrepareColumnOffsets(int clmn, int chars_offs_title, int chars_offs_value);
	void		ShiftFieldsRef(long shift_x, long shift_y) { m_fields_ref.x += shift_x; m_fields_ref.y += shift_y; }
				// Columns numbering starts from 1. Char width is taken from the "field values" style.

	void ShiftRowsDown(int row_start, long shift_val)
	{
		for (int irow=row_start-1; irow<_num_rows; ++irow)
			m_baseline_offs[irow] += shift_val;
	}

	void		SetMainHeader(short header_icon, const wchar_t *header_text);
	void		SetSecondaryHeader(short header_icon, const wchar_t *header_text);
	void		SetTitle(int line, int clmn, const wchar_t *msg);
	void		SetTitleValue(int line, int clmn, const wchar_t *msg);
	void		SetValue(int line, int clmn, const wchar_t *msg);
				// Lines numbering in the helper starts from 1.

	static wchar_t		*FormatLinearArea(wchar_t *buff, long buff_len, TLinearArea &area);
	static wchar_t		*FormatSourceAreaInfo(wchar_t *buff, long buff_len, TSourceAreaInfo &area, const wchar_t *suffix = NULL);
	static wchar_t		*FormatEventId(wchar_t *buff, long buff_len, CtvEventHeader &hdr);
	static wchar_t		*FormatId(wchar_t *buff, long buff_len, ID value, const wchar_t *zero_subst = NULL);
	static wchar_t		*FormatLong(wchar_t *buff, long buff_len, long value, const wchar_t *suffix = NULL);
	static wchar_t		*FormatName(wchar_t *buff, long buff_len, const wchar_t *value);

	TLangViewClickEventDetailsDialog		*m_stls;

	enum { _num_clmns = 2 };

private:

	void		SetHdrInternal(short header_icon, const wchar_t *header_text, TTextStyle *header_text_style);
	void		SetTitleInternal(int line, int clmn, const wchar_t *msg, TTextStyle &tstl);

private:

	TBitmapItem							m_header_icon;
	TTextItem							m_header_text;
	TRectItem							m_header_delim;

	TTextItem							m_field_title[_num_rows*_num_clmns];
	TTextItem							m_field_value[_num_rows*_num_clmns];
											// This is matrix of fields that has _num_rows lines and _num_clmns columns.

	POINT								m_fields_ref;
											// This is reference point for placing the header and fields. Note that placing the header
											// and or the XML element is not moving the ref field but these actions change baseline
											// ofsets of the title/value elements.

	long									m_title_offs[_num_clmns];				// In pixels.
	long									m_value_offs[_num_clmns];			// In pixels.
	long									m_baseline_offs[_num_rows];		// In pixels.

	long									*m_ymax;

	friend class TLangViewXmlBasedDetailsHelper;
};

//
//  Bogus event, empty event and all not implemented cases.
//
class TLangViewHeaderOnlyDetailsHelper : public TLangViewClickDetailsHelperBase<1>
{
public:

	void		Prepare(POINT &start_end_point, const wchar_t *header_text) { PrepareFormatting(start_end_point); SetMainHeader(0, header_text); }
};

//
//  Details about the file.
//
class TLangViewFileIdDetailsHelper : public TLangViewClickDetailsHelperBase<6>
{
public:

	void		Prepare(POINT &start_end_point, TMidLevStoreReader *db, ID pars_id, ID file_id);
				// Note that this helper displays info based on the ids of the file ony. There is no way to pass
				// the name of the file directly. Database should contain info about this file.
protected:

	void		PrepareInternal(POINT &start_end_point, TFileInfo *cached_file_info, bool file_vers_mismatch);
				// This method prepares decsription that is 5 to 6 lines high.

	void		PrepareInternal(POINT &start_end_point, ID pars_id, ID file_id, bool display_no_info_comment);
				// This method prepares decsription that is 1 or 2 lines high.

	wchar_t				m_fname_buff[280];
	wchar_t				m_file_id_buff[40],			m_fdate_buff[40];
	wchar_t				m_file_len_buff[80],			m_fcrc_buff[40];
};

//
//  Details about the code location.
//
class TLangViewCodeLocationDetailsHelper : public TLangViewClickDetailsHelperBase<6>
{
public:

	void		Prepare(POINT &start_end_point, TMidLevStoreReader *db, TSourceLocation &area, const wchar_t *secondary_section_name = NULL);
				// Code loc description can consist of from 1 to 6 lines.
protected:

	wchar_t				m_pars_id_buff[40];
	wchar_t				m_linear_area_buff[40];
	wchar_t				m_src_ref_buff[40];
	wchar_t				m_src_area_buff[80];
	wchar_t				m_lrange_buff[40];
};

//
//  Command line raw lexema details.
//
class TLangViewCmdlLexemaDetailsHelper : public TLangViewClickDetailsHelperBase<1>
{
public:

	void		Prepare(POINT &start_end_point, TCmdlLexemaInfo &lex_info);
};

//
//  C/C++ raw lexema details.
//
class TLangViewClangLexemaDetailsHelper : public TLangViewClickDetailsHelperBase<4>
{
public:

	void		Prepare(POINT &start_end_point, TLexemaInfo &lex_info);
				//
				//	Lexema description contains from 2 to 4 lines:
				//
				//		empty_lexema, comment, eol, eof		  --	type and subtype on 2 lines.
				//		number, charconst, string, keyword, name	  --	type, subtype plus one additional prop. 3 lines in total.
				//		error								  --	type, subtype, error code and error msg. 4 lines in total.
				//
				//	NB: Lexema origin is not displayed by this helper at all.
				//
protected:

	wchar_t				m_buff[200];
	wchar_t				m_origin_buff[60];
};

//
//  Several different roles.
//
class TLangViewXmlBasedDetailsHelper : public TLangViewClickDetailsHelperBase<3>
{
public:		// Note:	This helper uses 3 lines. First line duplicates the most important props above
			//		the XML representation of the data while other two lines may contain additional
			//		explanations below the XML file viewer.

	TLangViewXmlBasedDetailsHelper() { m_helper_prepared = FALSE; }

	void		PrepareParsingSessionInfo(POINT &start_end_point, CtvCompilationInfo &sess_header);
	void		PrepareDirectoryInfoFileInfo(POINT &start_end_point, TFileInfo &file_data, bool file_info_mode);
	void		PrepareIndexingKeyInfo(POINT &start_end_point, TKeyInfo &key_data);
	void		PrepareIndexingKeyUseInfo(POINT &start_end_point, TKeyUseInfo &key_use_data);
	void		PrepareParsingEventInfo(POINT &start_end_point, TMidLevStoreReader *db, CtvParsingEventInfo &event);
	void		PrepareCppItemInfo(POINT &start_end_point, TCppItemBase *item);

protected:

	bool		SetupXmlDescription(int lines_above_xml_item, TDestinationFile &mem_buffer, TXmlItemPresentationRecord *special_tags_dict = NULL);

	bool						m_helper_prepared;
								// This flag indicates that xml viewer contains valid XML data.

	TRectItem				m_upper_delim;
	TXmlFileViewScreenItem	m_xml_repr;

	wchar_t					m_misc_buff1[80];

	friend class TLangViewClickEventDetailsDialog;
};

//
//  Template: IDD_LANGV_SVF_EVENT_DETAILS
//
class TLangViewClickEventDetailsDialog : public TLangViewChildBrowser
{
public:

	TLangViewClickEventDetailsDialog(TMidLevStoreReader *db_link);

	LRESULT		OnInitDialog(HWND hDefaultFocusControl);
	void			OnWmCommand(short controlId, short notificationCode, HWND hWndControl);

	void		LoadContents(TLangViewClickEvent &event);
	void		ResetContents();

	void		SetSelection(TLangViewClickEvent &event, bool show_margins) { }
	void		ResetSelection() { }
				// This viewer has no selection processing.

protected:

	bool		PrepareStyles();
	void		ProcessReportData(bool gen_res, TDestinationFile &rprt);

protected:

	TBitmapStyle								m_icon_picts;
	TBasicStyle								m_sect_delim_style;				// Style of the line under the sect header.
	TBasicStyle								m_xml_upper_border_style;		// Style of the line on top of the XML data viewer.

	TTextStyle								m_section_title_style;
	TTextStyle								m_section_subtitle_style;
	TTextStyle								m_field_title_style;
	TTextStyle								m_field_value_style;

	TXmlFileViewStyle							m_xml_repr_style;

	TScreenItemsViewObject					m_scr_items_view;

	TLangViewHeaderOnlyDetailsHelper			m_hdr_only_dtls;
	TLangViewFileIdDetailsHelper				m_file_info_dtls;
	TLangViewCodeLocationDetailsHelper			m_code_loc_dtls;
	TLangViewCmdlLexemaDetailsHelper			m_cmdl_lexema_dtls;
	TLangViewClangLexemaDetailsHelper			m_clang_lexema_dtls;
	TLangViewXmlBasedDetailsHelper				m_xml_based_dtls;

	TLangViewClickEvent						m_contents;
	wchar_t									m_bogus_evt_type_buff[80];

	template<int> friend class TLangViewClickDetailsHelperBase;
	friend class TLangViewXmlBasedDetailsHelper;
};

// -----------------  Template methods  ------------------

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::PrepareFormatting(POINT &start_end_point)
{
	assert(m_stls != NULL);

	// Clear TListItems in all objects to ensure that they are not setup more than once.
	for (int inx=0; inx<_num_rows*_num_clmns; ++inx)
	{
		m_field_title[inx].Clear();
		m_field_value[inx].Clear();
	}

	m_fields_ref = start_end_point;
	memset(m_title_offs, 0, sizeof(m_title_offs));
	memset(m_value_offs, 0, sizeof(m_value_offs));

	m_baseline_offs[0] = __max(m_stls->m_field_title_style.HeightAbove(), m_stls->m_field_value_style.HeightAbove());
	long lines_step = __max(m_stls->m_field_title_style.GetHeight(), m_stls->m_field_value_style.GetHeight())+2;
	for (int irow=1; irow<_num_rows; ++irow)
		m_baseline_offs[irow] = m_baseline_offs[irow-1]+lines_step;

	m_ymax = &(start_end_point.y);

	// Setup default offsets for the leftmost column.
	PrepareColumnOffsets(1, 1, 10);
}

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::PrepareColumnOffsets(int clmn, int chars_title, int chars_value)
{
	assert(clmn >= 1 && clmn <= _num_clmns);
	int char_width_in_pixels = m_stls->m_field_value_style.GetBasicWidth();

	m_title_offs[clmn-1] = char_width_in_pixels*chars_title;
	m_value_offs[clmn-1] = char_width_in_pixels*chars_value;
}

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::SetMainHeader(short header_icon, const wchar_t *header_text)
{
	// This method cannot be defined inline because it uses the data fields of the dialog.
	SetHdrInternal(header_icon, header_text, &m_stls->m_section_title_style);
}

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::SetSecondaryHeader(short header_icon, const wchar_t *header_text)
{
	// This method cannot be defined inline because it uses the data fields of the dialog.
	SetHdrInternal(header_icon, header_text, &m_stls->m_section_subtitle_style);
}

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::SetTitle(int line, int clmn, const wchar_t *msg)
{
	SetTitleInternal(line, clmn, msg, m_stls->m_field_title_style);
}

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::SetTitleValue(int line, int clmn, const wchar_t *msg)
{
	SetTitleInternal(line, clmn, msg, m_stls->m_field_value_style);
}

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::SetValue(int line, int clmn, const wchar_t *msg)
{
	if (clmn < 1)
		clmn = 1;
	if (clmn > _num_clmns)
		clmn = _num_clmns;

	TTextItem &scr_item = m_field_value[(line-1)*_num_clmns+(clmn-1)];
	assert(scr_item.IsInList() == FALSE);

	scr_item.SetStyle(&(m_stls->m_field_value_style));
	scr_item.SetContents(msg);

	long y_pos = m_fields_ref.y+m_baseline_offs[line-1];
	long rpos = scr_item.PlaceObject(m_stls->m_scr_items_view.GetControl(), m_fields_ref.x+m_value_offs[clmn-1], y_pos);
	m_stls->m_scr_items_view.AddToInternalDataSet(scr_item);

	// Update the vert marker.
	y_pos += m_stls->m_field_value_style.HeightBelow()+4;
	if (y_pos > *m_ymax)
		*m_ymax = y_pos;
}

// - - - - - - - - - -  Text Formatters  - - - - - - - - - - - -

template <int _num_rows>
wchar_t *TLangViewClickDetailsHelperBase<_num_rows>::FormatLinearArea(wchar_t *buff, long buff_len, TLinearArea &area)
{
	assert(buff_len >= 40);
	swprintf(buff, buff_len, L"%ld:%ld", area.linear_beg, area.linear_len);
	return(buff);
}

template <int _num_rows>
wchar_t *TLangViewClickDetailsHelperBase<_num_rows>::FormatSourceAreaInfo(wchar_t *buff, long buff_len, TSourceAreaInfo &area, const wchar_t *suffix)
{
	assert(buff_len >= 40);
	if (area.file_id != 0)
	{
		// The file info is present.
		wchar_t file_id_buff[40];
		swprintf(buff, buff_len, L"%s, %ld:%ld%s", area.PrepareFileIdShort(file_id_buff, 40), area.area_beg, area.area_len, (suffix != NULL) ? suffix : L"");
	}
	else
	{
		// The file ids are zero. Show just the offsets info.
		swprintf(buff, buff_len, L"%ld:%ld%s", area.area_beg, area.area_len, (suffix != NULL) ? suffix : L"");
	}

	return(buff);
}

template <int _num_rows>
wchar_t *TLangViewClickDetailsHelperBase<_num_rows>::FormatEventId(wchar_t *buff, long buff_len, CtvEventHeader &hdr)
{
	assert(buff_len >= 40);
	swprintf(buff, buff_len, L"%lu.%lu", hdr.parsing_id, hdr.event_id);
	return(buff);
}

template <int _num_rows>
wchar_t *TLangViewClickDetailsHelperBase<_num_rows>::FormatId(wchar_t *buff, long buff_len, DWORD value, const wchar_t *zero_subst)
{
	assert(buff_len >= 40);
	if (value == 0 && zero_subst != NULL)
		wcscpy(buff, zero_subst);
	else swprintf(buff, buff_len, L"%lu", value);
	return(buff);
}

template <int _num_rows>
wchar_t *TLangViewClickDetailsHelperBase<_num_rows>::FormatLong(wchar_t *buff, long buff_len, long value, const wchar_t *suffix)
{
	assert(buff_len >= 40);
	swprintf(buff, buff_len, L"%ld%s", value, (suffix != NULL) ? suffix : L"");
	return(buff);
}

template <int _num_rows>
wchar_t *TLangViewClickDetailsHelperBase<_num_rows>::FormatName(wchar_t *buff, long buff_len, const wchar_t *value)
{
	assert(buff_len >= 20);

	if (value == NULL)
	{
		wcscpy(buff, L"NULL");
	}
	else
	{
		buff[0] = L'\"';
		wcsncpy(buff+1, value, buff_len-5);
		buff[buff_len-4] = 0;
		wcscat(buff, ((long)wcslen(value) > buff_len-5) ? L"...\"" : L"\"");
	}

	return(buff);
}

// - - - - - - - - - -  Other Stuffs  - - - - - - - - - - - - -

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::SetHdrInternal(short header_icon, const wchar_t *header_text, TTextStyle *header_text_style)
{
	long pos_x = m_fields_ref.x+m_title_offs[0];
	long y_pos = m_fields_ref.y+6+header_text_style->HeightAbove();

	// Icon of the section if any.
	if (header_icon != 0)
	{
		m_header_icon.SetStyle(&(m_stls->m_icon_picts));
		m_header_icon.SetLeftAlignedPosition(pos_x, y_pos-6, header_icon);
		m_stls->m_scr_items_view.AddToInternalDataSet(m_header_icon);
		pos_x += m_header_icon.GetWidth()+7;
	}

	// Name of the section.
	m_header_text.SetStyle(header_text_style);
	m_header_text.SetContents(header_text);
	long rpos = m_header_text.PlaceObject(m_stls->m_scr_items_view.GetControl(), pos_x, y_pos);
	m_stls->m_scr_items_view.AddToInternalDataSet(m_header_text);
	y_pos += header_text_style->HeightBelow();

	// Dark line under the section name.
	rpos = __max(rpos+40, m_fields_ref.x+340);
	m_header_delim.SetStyle(&(m_stls->m_sect_delim_style));
	RECT rc = { m_fields_ref.x, y_pos, rpos, y_pos+2 };
	m_header_delim.SetBaseRect(rc);
	m_stls->m_scr_items_view.AddToInternalDataSet(m_header_delim);
	y_pos += 7;

	for (int irow=0; irow<_num_rows; ++irow)
		m_baseline_offs[irow] += y_pos-m_fields_ref.y;

	// Update the vert marker.
	if (y_pos > *m_ymax)
		*m_ymax = y_pos;
}

template <int _num_rows>
void TLangViewClickDetailsHelperBase<_num_rows>::SetTitleInternal(int line, int clmn, const wchar_t *msg, TTextStyle &tstl)
{
	if (clmn < 1)
		clmn = 1;
	if (clmn > _num_clmns)
		clmn = _num_clmns;

	TTextItem &scr_item = m_field_title[(line-1)*_num_clmns+(clmn-1)];
	assert(scr_item.IsInList() == FALSE);

	scr_item.SetStyle(&tstl);
	scr_item.SetContents(msg);

	long y_pos = m_fields_ref.y+m_baseline_offs[line-1];
	long rpos = scr_item.PlaceObject(m_stls->m_scr_items_view.GetControl(), m_fields_ref.x+m_title_offs[clmn-1], y_pos);
	m_stls->m_scr_items_view.AddToInternalDataSet(scr_item);

	// Update the vert marker.
	y_pos += tstl.HeightBelow()+4;
	if (y_pos > *m_ymax)
		*m_ymax = y_pos;
}

#endif	// LangView_ClickEventDetails_H


