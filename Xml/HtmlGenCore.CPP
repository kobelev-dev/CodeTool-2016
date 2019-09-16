//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//	Second version of HTML generation.
//

#include  <stdio.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "Xml/HtmlGenCore.H"

//----------------------------------------------------------------------------
//	====================  TSimpleHtmlElement  =====================
//----------------------------------------------------------------------------

TSimpleHtmlElement::TSimpleHtmlElement(const wchar_t *element_name, const wchar_t *element_body, int element_body_len, const wchar_t *css_class, THtmlCrLfMode crlf_mode)
		: TRawHtmlElement(element_name)
{
	SetElementBody(element_body, element_body_len);
	m_css_class = (wchar_t*)css_class;

	if (crlf_mode != crlm_default_param_value)
	{
		// The crlf mode is given explicitly.
		m_crlf_mode = crlf_mode;
	}
	else
	{
		// Expand the default value of the param.
		if (element_name != NULL)
			m_crlf_mode = crlm_before_and_after_object;
		else m_crlf_mode = crlm_none;
	}
}

void TSimpleHtmlElement::EmitElementBody(THtmlDocument &doc)
{
	if (m_element_body != NULL && m_element_body_len > 0)
	{
		// Emit the body of the element using the default mode.
		doc.EmitRawData(m_element_body, m_element_body_len);
	}
}

void TSimpleHtmlElement::SetElementBody(const wchar_t *element_body, int element_body_len)
{
	m_element_body = (wchar_t*)element_body;

	if (element_body == NULL)
	{
		// The element has no body.
		m_element_body_len = 0;
	}
	else if (element_body_len < 0)
	{
		// The body is a NULL terminated string.
		m_element_body_len = wcslen(element_body);
	}
	else
	{
		// The length of the element body is given explicitly.
		m_element_body_len = element_body_len;
	}
}

//----------------------------------------------------------------------------
//	==================  TAdvancedHtmlElement  =====================
//----------------------------------------------------------------------------

void TAdvancedHtmlElement::EmitSpecificAttrs(THtmlDocument &doc)
{
	if (m_app_attr1_name != NULL && m_app_attr1_name[0] != 0)
		doc.EmitAttribute(m_app_attr1_name, m_app_attr1_value);

	if (m_app_attr2_name != NULL && m_app_attr2_name[0] != 0)
		doc.EmitAttribute(m_app_attr2_name, m_app_attr2_value);
}

void TAdvancedHtmlElement::EmitElementBody(THtmlDocument &doc)
{
	if (m_element_body != NULL && m_element_body_len > 0)
	{
		// Emit the body of the element using the current emitting mode.
		doc.EmitRawData(m_element_body, m_element_body_len, 0, 0, m_body_emit_mode);
	}
}

//----------------------------------------------------------------------------
//	==================  THyperLinkHtmlElement  =====================
//----------------------------------------------------------------------------

void THyperLinkHtmlElement::EmitSpecificAttrs(THtmlDocument &doc)
{
	// This method is not defined as inline because it uses method of the HTML document that
	// is not yet defined when the definition of this class is compiled.
	doc.EmitAttribute(L"href", m_link_href);

	if (m_link_target != NULL)
		doc.EmitAttribute(L"target", m_link_target);
}

//----------------------------------------------------------------------------
//	===================  TImageHtmlElement  ======================
//----------------------------------------------------------------------------

void TImageHtmlElement::EmitSpecificAttrs(THtmlDocument &doc)
{
	// This method is not defined as inline because it uses method of the HTML document that
	// is not yet defined when the definition of this class is compiled.
	doc.EmitAttribute(L"src", m_image_src_file);
}

//----------------------------------------------------------------------------
//	==================  TListBasedHtmlContainer  ====================
//----------------------------------------------------------------------------

void TListBasedHtmlContainer::EmitElementBody(THtmlDocument &doc)
{
	// Emit child elements of the container if any.
	for (TListIter<TRawHtmlElement> iter(m_child_elements); iter; ++iter)
	{
		doc.EmitElement(iter.CurrItem());
	}
}

//----------------------------------------------------------------------------
//	==================  TArrayBasedHtmlContainer  ===================
//----------------------------------------------------------------------------

void TArrayBasedHtmlContainer::EmitSpecificAttrs(THtmlDocument &doc)
{
	if (m_app_attr1_name != NULL && m_app_attr1_name[0] != 0)
		doc.EmitAttribute(m_app_attr1_name, m_app_attr1_value);
}

void TArrayBasedHtmlContainer::EmitElementBody(THtmlDocument &doc)
{
	// Emit all child elements if any.
	for (int inx=0; inx<m_elements.NumItems(); ++inx)
	{
		doc.EmitElement(*m_elements[inx]);
	}
}

bool TArrayBasedHtmlContainer::AppendElements(TRawHtmlElement *nel1, TRawHtmlElement *nel2, TRawHtmlElement *nel3, TRawHtmlElement *nel4, TRawHtmlElement *nel5, TRawHtmlElement *nel6, TRawHtmlElement *nel7, TRawHtmlElement *nel8)
{
	if (nel1 != NULL && m_elements.AppendItem(nel1) == FALSE)
		return(FALSE);
	else if (nel2 != NULL && m_elements.AppendItem(nel2) == FALSE)
		return(FALSE);
	else if (nel3 != NULL && m_elements.AppendItem(nel3) == FALSE)
		return(FALSE);
	else if (nel4 != NULL && m_elements.AppendItem(nel4) == FALSE)
		return(FALSE);
	else if (nel5 != NULL && m_elements.AppendItem(nel5) == FALSE)
		return(FALSE);
	else if (nel6 != NULL && m_elements.AppendItem(nel6) == FALSE)
		return(FALSE);
	else if (nel7 != NULL && m_elements.AppendItem(nel7) == FALSE)
		return(FALSE);
	else if (nel8 != NULL && m_elements.AppendItem(nel8) == FALSE)
		return(FALSE);

	// All passed elements were successfully appended.
	return(TRUE);
}

//----------------------------------------------------------------------------
//	=====================  THtmlDocument  =======================
//----------------------------------------------------------------------------

static const wchar_t g_HtmlGenCore_20TabsLine[] = L"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static const wchar_t g_HtmlGenCore_40SpacesLine[] = L"                                        ";

THtmlDocument::THtmlDocument(const wchar_t *stream_name, bool document_fragment_mode, int line_indent)
		: TDestinationFile(stream_name)
{
	assert(wcslen(g_HtmlGenCore_20TabsLine) == 20 && wcslen(g_HtmlGenCore_40SpacesLine) == 40);

	m_doc_fragment_mode = document_fragment_mode;

	m_page_header_opened = FALSE;
	m_page_body_opened = FALSE;
	m_tag_header_opened = FALSE;
	m_cnt_opened_layers = 0;

	m_indent_delta = 0;

	// Max indentation should not exceed the length of the g_HtmlGenCore_40SpacesLine array.
	m_line_indent = (line_indent < 40) ? ((line_indent >= 0) ? line_indent : -1) : 40;
	m_line_dirty = FALSE;
}

THtmlDocument::~THtmlDocument()
{
	assert(m_page_header_opened == FALSE && m_page_body_opened == FALSE);
	assert(m_tag_header_opened == FALSE && m_cnt_opened_layers == 0);
	THtmlDocument::Close();
}

bool THtmlDocument::Close()
{
	if (IsWritingState() == TRUE)
	{
		if (m_tag_header_opened == TRUE)
		{
			// Close the unclosed tag header.
			EmitRawData(L">", 1, TRUE, TRUE, rdem_no_std_escaping);
			m_cnt_opened_layers -= 2;
		}

		if (m_cnt_opened_layers > 0)
		{
			// Close all unclosed container layers.
			while (m_cnt_opened_layers > 0)
			{
				m_cnt_opened_layers--;
				EmitRawData(L"</>", 3, TRUE, TRUE, rdem_no_std_escaping);
			}

			if (m_doc_fragment_mode == FALSE)
				EmitRawData(L"</>", 3, TRUE, TRUE, rdem_no_std_escaping);
		}
	}

	// Call the method of the base class.
	return(TDestinationFile::Close());
}
void THtmlDocument::OpenDocumentHead(THtmlDocumentType doc_type)
{
	assert(m_doc_fragment_mode == FALSE);
	assert(m_page_header_opened == FALSE && m_page_body_opened == FALSE);
	assert(m_tag_header_opened == FALSE && m_cnt_opened_layers == 0);

	// Emit the document type if any.
	switch (doc_type)
	{
		case dctp_html_401_transitional:
				WriteLine(L"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">");
				break;

		case dctp_html_401_strict:
				WriteLine(L"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">");
				break;

		case dctp_html_401_frameset:
				WriteLine(L"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\">");
				break;

		case dctp_xhtml_10_transitional:
				WriteLine(L"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\">");
				break;

		case dctp_xhtml_10_strict:
				WriteLine(L"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\">");
				break;

		case dctp_xhtml_10_frameset:
				WriteLine(L"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\">");
				break;
	}

	// Emit the std opening sequence.
	EmitRawData(L"<html>", 6, TRUE, TRUE, rdem_no_std_escaping);
	EmitRawData(L"<head>", 6, TRUE, TRUE, rdem_no_std_escaping);

	// Set the count of opened layers to 1 and not to 2 for nicer indentation.
	m_page_header_opened = TRUE;
	m_cnt_opened_layers = 1;
}

void THtmlDocument::OpenDocumentBody()
{
	assert(m_doc_fragment_mode == FALSE);
	assert(m_page_header_opened == TRUE && m_page_body_opened == FALSE);
	assert(m_tag_header_opened == FALSE && m_cnt_opened_layers == 1);

	// Emit the close/open sequence.
	m_cnt_opened_layers--;
	EmitRawData(L"</head>", 7, TRUE, TRUE, rdem_no_std_escaping);
	EmitRawData(L"<body>", 6, TRUE, TRUE, rdem_no_std_escaping);

	m_page_header_opened = FALSE;
	m_page_body_opened = TRUE;
	m_cnt_opened_layers++;
}

void THtmlDocument::CloseDocument()
{
	assert(m_doc_fragment_mode == FALSE);
	assert(m_page_header_opened == FALSE && m_page_body_opened == TRUE);
	assert(m_tag_header_opened == FALSE && m_cnt_opened_layers == 1);

	// Decrease the count of layers before sending the closing sequence.
	m_cnt_opened_layers--;
	m_page_body_opened = FALSE;

	// Emit the std closing sequence.
	EmitRawData(L"</body>", 7, TRUE, TRUE, rdem_no_std_escaping);
	EmitRawData(L"</html>", 7, TRUE, TRUE, rdem_no_std_escaping);
}

void THtmlDocument::AddHtmlComment(const wchar_t *comment_body, THtmlCrLfMode comment_crlf_mode)
{
	assert(m_tag_header_opened == FALSE);

	EmitRawData(L"<!-- ", 5, (comment_crlf_mode & crlm_before_whole_obj), (comment_crlf_mode & crlm_before_first_child), rdem_no_std_escaping);
	m_cnt_opened_layers++;

	if (comment_body != NULL && comment_body[0] != 0)
		EmitRawData(comment_body, wcslen(comment_body), FALSE, FALSE, rdem_no_std_escaping);

	m_cnt_opened_layers--;
	EmitRawData(L" -->", 4, (comment_crlf_mode & crlm_after_last_child), (comment_crlf_mode & crlm_after_whole_obj), rdem_no_std_escaping);
}

void THtmlDocument::AddPageTitle(const wchar_t *page_title)
{
	assert(m_page_header_opened == TRUE && m_page_body_opened == FALSE);
	assert(m_tag_header_opened == FALSE && m_cnt_opened_layers == 1);

	EmitRawData(L"<title>", 7, TRUE, FALSE, rdem_no_std_escaping);

	if (page_title != NULL && page_title[0] != 0)
		EmitRawData(page_title, wcslen(page_title), FALSE, FALSE, rdem_ignore_crlf);

	EmitRawData(L"</title>", 8, FALSE, TRUE, rdem_no_std_escaping);
}

void THtmlDocument::AddInlineCssStyle(const wchar_t *css_style)
{
	assert(m_page_header_opened == TRUE && m_page_body_opened == FALSE);
	assert(m_tag_header_opened == FALSE && m_cnt_opened_layers == 1);

	const wchar_t *starting_data = L"<style type=\"text/css\">";
	EmitRawData(starting_data, wcslen(starting_data), TRUE, FALSE, rdem_no_std_escaping);

	if (css_style != NULL && css_style[0] != 0)
		EmitRawData(css_style, wcslen(css_style), FALSE, FALSE, rdem_ignore_crlf);

	EmitRawData(L"</style>", 8, FALSE, TRUE, rdem_no_std_escaping);
}

void THtmlDocument::AddCssFileRef(const wchar_t *dir_or_url, const wchar_t *file_name)
{
	assert(m_page_header_opened == TRUE && m_page_body_opened == FALSE);
	assert(m_tag_header_opened == FALSE && m_cnt_opened_layers == 1);

	EmitRawData(L"<link href=\"", 12, TRUE, FALSE, rdem_no_std_escaping);

	if (dir_or_url != NULL && dir_or_url[0] != 0)
	{
		// The CSS file comes not from the current directory of the document.
		EmitRawData(dir_or_url, wcslen(dir_or_url), FALSE, FALSE, rdem_ignore_crlf);
		EmitRawData(L"/", 1, FALSE, FALSE, rdem_no_std_escaping);
	}

	// Emit the name of the CSS file.
	assert(file_name != NULL && file_name[0] != 0);
	EmitRawData(file_name, wcslen(file_name), FALSE, FALSE, rdem_ignore_crlf);

	const wchar_t *ending_data = L"\" type=\"text/css\" rel=\"stylesheet\"/>";
	EmitRawData(ending_data, wcslen(ending_data), FALSE, TRUE, rdem_no_std_escaping);
}

void THtmlDocument::AddCssFileReferences(const wchar_t *dir_or_url, const wchar_t **file_names)
{
	if (dir_or_url != NULL && file_names != NULL)
	{
		while (*file_names != NULL)
			AddCssFileRef(dir_or_url, *file_names++);
	}
}

void THtmlDocument::AddJavaScriptFileRef(const wchar_t *dir_or_url, const wchar_t *file_name)
{
	assert(m_page_header_opened == TRUE && m_page_body_opened == FALSE);
	assert(m_tag_header_opened == FALSE && m_cnt_opened_layers == 1);

	EmitRawData(L"<script src=\"", 13, TRUE, FALSE, rdem_no_std_escaping);

	if (dir_or_url != NULL && dir_or_url[0] != 0)
	{
		// Java Script file comes not from the current directory of the document.
		EmitRawData(dir_or_url, wcslen(dir_or_url), FALSE, FALSE, rdem_ignore_crlf);
		EmitRawData(L"/", 1, FALSE, FALSE, rdem_no_std_escaping);
	}

	// Emit the name of the Java Script file.
	assert(file_name != NULL && file_name[0] != 0);
	EmitRawData(file_name, wcslen(file_name), FALSE, FALSE, rdem_ignore_crlf);

	// For unclear reason browsers do not like the short form of the tag. Emit the tag close explicitly.
	const wchar_t *ending_data = L"\" type=\"text/javascript\" defer=\"defer\"></script>";
	EmitRawData(ending_data, wcslen(ending_data), FALSE, TRUE, rdem_no_std_escaping);
}

void THtmlDocument::EmitElement(TRawHtmlElement &element)
{
	if (element.m_element_name != NULL && element.m_element_name[0] != 0)
	{
		// This is regular case of a named element. Use the full procedure.
		THtmlCrLfMode element_crlf_mode = element.GetCrLfMode();
		bool element_body_expected = element.IsBodyPresent();

		EmitElementBegInternal(element, element_body_expected, element_crlf_mode);
		if (element_body_expected == TRUE)
		{
			element.EmitElementBody(*this);
			EmitElementEndInternal(element, element_crlf_mode);
		}
	}
	else
	{
		// The is a special case of the nameless element. Check for the element body.
		if (element.IsBodyPresent() == TRUE)
		{
			element.EmitElementBody(*this);
		}
	}
}

void THtmlDocument::EmitElementBeg(TRawHtmlElement &element)
{
	// The name of the element should be present. Ensure this.
	assert(element.m_element_name != NULL && element.m_element_name[0] != 0);

	// Emit the opening tag, regardless of what the object thinks about itself.
	EmitElementBegInternal(element, TRUE, element.GetCrLfMode());
}

void THtmlDocument::EmitElementEnd(TRawHtmlElement &element)
{
	// The name of the element should be present.
	assert(element.m_element_name != NULL && element.m_element_name[0] != 0);
	EmitElementEndInternal(element, element.GetCrLfMode());
}

void THtmlDocument::EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2)
{
	EmitElement(el1);
	EmitElement(el2);
}

void THtmlDocument::EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3)
{
	EmitElement(el1);
	EmitElement(el2);
	EmitElement(el3);
}

void THtmlDocument::EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4)
{
	EmitElement(el1);
	EmitElement(el2);
	EmitElement(el3);
	EmitElement(el4);
}

void THtmlDocument::EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4, TRawHtmlElement &el5)
{
	EmitElement(el1);
	EmitElement(el2);
	EmitElement(el3);
	EmitElement(el4);
	EmitElement(el5);
}

void THtmlDocument::EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4, TRawHtmlElement &el5, TRawHtmlElement &el6)
{
	EmitElement(el1);
	EmitElement(el2);
	EmitElement(el3);
	EmitElement(el4);
	EmitElement(el5);
	EmitElement(el6);
}

void THtmlDocument::EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4, TRawHtmlElement &el5, TRawHtmlElement &el6, TRawHtmlElement &el7)
{
	EmitElement(el1);
	EmitElement(el2);
	EmitElement(el3);
	EmitElement(el4);
	EmitElement(el5);
	EmitElement(el6);
	EmitElement(el7);
}

void THtmlDocument::EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4, TRawHtmlElement &el5, TRawHtmlElement &el6, TRawHtmlElement &el7, TRawHtmlElement &el8)
{
	EmitElement(el1);
	EmitElement(el2);
	EmitElement(el3);
	EmitElement(el4);
	EmitElement(el5);
	EmitElement(el6);
	EmitElement(el7);
	EmitElement(el8);
}

void THtmlDocument::EmitAttribute(const wchar_t *attr_name, const wchar_t *attr_value, THtmlCrLfMode attr_crlf_mode)
{
	// The name of the attribute should be not empty.
	assert(attr_name != NULL && attr_name[0] != 0);
	assert(m_tag_header_opened == TRUE);

	if ((attr_crlf_mode & crlm_before_whole_obj) == 0)
	{
		// Check if the space in front of the attribute is needed or not.
		if (m_line_dirty == TRUE)
			EmitRawData(L" ", 1, FALSE, FALSE, rdem_no_std_escaping);
	}
	else
	{
		EmitRawData(L"", 0, TRUE, FALSE, rdem_no_std_escaping);
	}

	EmitRawData(attr_name, wcslen(attr_name), FALSE, FALSE, rdem_ignore_crlf);
	EmitRawData(L"=\"", 2, FALSE, FALSE, rdem_no_std_escaping);

	if (attr_value != NULL && attr_value[0] != 0)
		EmitRawData(attr_value, wcslen(attr_value), FALSE, FALSE, rdem_ignore_crlf);

	EmitRawData(L"\"", 1, FALSE, (attr_crlf_mode & crlm_after_whole_obj), rdem_no_std_escaping);
}

void THtmlDocument::EmitRawData(const wchar_t *data_beg, long data_len, int front_crlf_flags, int trailing_crlf_flags, THtmlDataEmitMode emit_mode)
{
	assert(data_len >= 0);

	// Initialize writing.
	if (data_len > 0)
	{
		// The data is not empty. The line will become dirty inside the for loop in any case.
		if (front_crlf_flags != 0 && m_line_dirty == TRUE)
		{
			WriteLine();

			if ((emit_mode & rdem_ignore_indent) == 0)
				HtmlIndentLine();
		}
		else if (m_line_dirty == FALSE)
		{
			if ((emit_mode & rdem_ignore_indent) == 0)
				HtmlIndentLine();

			m_line_dirty = TRUE;
		}
	}
	else if (front_crlf_flags != 0 && m_line_dirty == TRUE)
	{
		WriteLine();
		m_line_dirty = FALSE;
	}

	// Process the passed data.
	for (int inx=0; inx<data_len; ++inx)
	{
		wchar_t ch = data_beg[inx];
		switch (ch)
		{
			case L'\r':
				{
					// Check if the next char is LF and fall threw in any case.
					if (inx+1<data_len && data_beg[inx+1] == L'\n')
						inx++;
				}

			case L'\n':
				{
					// Process the new line sequence.
					if ((emit_mode & rdem_ignore_crlf) == 0)
					{
						WriteLine();
						m_line_dirty = FALSE;

						if (inx+1<data_len && (emit_mode & rdem_ignore_indent) == 0)
						{
							HtmlIndentLine();
							m_line_dirty = TRUE;
						}

						continue;
					}

					// Replace the char with a space.
					ch = L' ';
				}
				break;

			// Check for characters that need an HTML escaping.
			case L'&':
				{
					if ((emit_mode & rdem_no_std_escaping) == 0)
					{
						// The data is not verbatim. Check for the XML escape sequencies.
						bool block_escaping = FALSE;
						if ((emit_mode & rdem_no_double_escaping) != 0)
						{
							if (data_len-inx >= 4)
							{
								const wchar_t *pseq = data_beg+inx;
								if (data_len-inx >= 5 && wcsncmp(pseq, L"&amp;", 5) == 0)
									block_escaping = TRUE;
								else if (wcsncmp(pseq, L"&lt;", 4) == 0)
									block_escaping = TRUE;
								else if (wcsncmp(pseq, L"&gt;", 4) == 0)
									block_escaping = TRUE;
								else if (data_len-inx >= 6 && wcsncmp(pseq, L"&quot;", 6) == 0)
									block_escaping = TRUE;
							}
						}

						if (block_escaping == FALSE)
						{
							Write(L"&amp;", 5);
							continue;
						}
					}
				}
				break;

			case L'<':
				{
					if ((emit_mode & rdem_no_std_escaping) == 0)
					{
						Write(L"&lt;", 4);
						continue;
					}
				}
				break;

			case L'>':
				{
					if ((emit_mode & rdem_no_std_escaping) == 0)
					{
						Write(L"&gt;", 4);
						continue;
					}
				}
				break;

			case L'\"':
				{
					if ((emit_mode & rdem_no_std_escaping) == 0)
					{
						Write(L"&quot;", 6);
						continue;
					}
				}
				break;

			case L' ':
				{
					if ((emit_mode & rdem_escape_space) != 0)
					{
						// Detect the length of the sequence of spaces if any.
						int ispce = inx+1;
						while (ispce < data_len && data_beg[ispce] == L' ')
							ispce++;

						int len_seq = ispce-inx;
						if (len_seq <= 1)
						{
							// There is only one space.
							Write(L"&nbsp;", 6);
						}
						else if (len_seq <= 2)
						{
							// There are two spaces.
							Write(L"&nbsp;&nbsp;", 12);
							inx++;
						}
						else
						{
							// Passed data contains a sequence of spaces.
							len_seq = (5*len_seq)/3;
							for (int isq=0; isq<len_seq; ++isq)
								Write(L"&nbsp;", 6);
							inx = ispce-1;
						}

						continue;
					}
				}
				break;

			case L'\t':
				{
					// HTML does not have direct support for the tab character.
					if ((emit_mode & rdem_escape_tab_char) != 0)
					{
						Write(L"&nbsp;&nbsp;&nbsp;&nbsp;", 24);
						continue;
					}
				}
				break;
		}

		// This is some ordinary character or its escaping was not requested.
		Write(&ch, 1);
	}

	// Finalize the writing.
	if (trailing_crlf_flags != 0 && m_line_dirty == TRUE)
	{
		WriteLine();
		m_line_dirty = FALSE;
	}
}

void THtmlDocument::EmitElementBegInternal(TRawHtmlElement &element, bool element_body_expected, THtmlCrLfMode element_crlf_mode)
{
	// The name of the element should be present.
	assert(element.m_element_name != NULL && element.m_element_name[0] != 0);
	assert(m_tag_header_opened == FALSE);

	//
	// Pick up info about the expected attributes.
	//

	THtmlCrLfMode crlf_m1 = crlm_none;
	const wchar_t *id_attr_val = element.GetIdAttr(crlf_m1);
	THtmlCrLfMode crlf_m2 = crlm_none;
	const wchar_t *class_attr_val = element.GetClassAttr(crlf_m2);
	THtmlCrLfMode crlf_m3 = crlm_none;
	const wchar_t *style_attr_val = element.GetStyleAttr(crlf_m3);

	bool spec_attrs_present = element.SpecAttrsPresent();
	bool attrs_present = (id_attr_val != NULL) || (class_attr_val != NULL) || (style_attr_val != NULL) || spec_attrs_present;
	bool crlf_after_tag_name = (attrs_present == TRUE) ? ((element_crlf_mode & crlm_before_first_attr) ? TRUE : FALSE) : FALSE;

	// Emit the name of the element.
	EmitRawData(L"<", 1, (element_crlf_mode & crlm_before_whole_obj), FALSE, rdem_no_std_escaping);
	EmitRawData(element.m_element_name, wcslen(element.m_element_name), FALSE, crlf_after_tag_name, rdem_ignore_crlf);

	if (attrs_present == TRUE)
	{
		m_tag_header_opened = TRUE;
		m_cnt_opened_layers += 2;

		if (id_attr_val != NULL)
			EmitAttribute(L"id", id_attr_val, crlf_m1);

		if (class_attr_val != NULL)
			EmitAttribute(L"class", class_attr_val, crlf_m2);

		if (style_attr_val != NULL)
			EmitAttribute(L"style", style_attr_val, crlf_m3);

		// Emit the element specific attrs if any.
		if (spec_attrs_present == TRUE)
			element.EmitSpecificAttrs(*this);

		m_tag_header_opened = FALSE;
	}

	bool crlf_before_closing_bracket = (attrs_present == TRUE) ? ((element_crlf_mode & crlm_after_last_attr) ? TRUE : FALSE) : FALSE;

	// Close the element header.
	if (element_body_expected == TRUE)
	{
		EmitRawData(L">", 1, crlf_before_closing_bracket, (element_crlf_mode & crlm_before_first_child), rdem_no_std_escaping);

		if (attrs_present == TRUE)
			m_cnt_opened_layers--;
		else m_cnt_opened_layers++;
	}
	else
	{
		EmitRawData(L"/>", 2, crlf_before_closing_bracket, (element_crlf_mode & crlm_after_whole_obj), rdem_no_std_escaping);

		if (attrs_present == TRUE)
			m_cnt_opened_layers -= 2;
	}
}

void THtmlDocument::EmitElementEndInternal(TRawHtmlElement &element, THtmlCrLfMode element_crlf_mode)
{
	// The name of the element should be present. Ensure this.
	assert(element.m_element_name != NULL && element.m_element_name[0] != 0);
	assert(m_tag_header_opened == FALSE);

	m_cnt_opened_layers--;
	EmitRawData(L"</", 2, (element_crlf_mode & crlm_after_last_child), FALSE, rdem_no_std_escaping);
	EmitRawData(element.m_element_name, wcslen(element.m_element_name), FALSE, FALSE, rdem_ignore_crlf);
	EmitRawData(L">", 1, FALSE, (element_crlf_mode & crlm_after_whole_obj), rdem_no_std_escaping);
}

void THtmlDocument::HtmlIndentLine()
{
	// Check if the indentation is needed or not.
	int effective_indent = m_cnt_opened_layers+m_indent_delta;

	if (effective_indent > 0)
	{
		// State of the layers require indentation. Check the props.
		if (m_line_indent < 0)
		{
			// Use the tab characters.
			if (effective_indent >= 20)
			{
				int cnt_big = effective_indent/20;
				for (int nbig=0; nbig<cnt_big; ++nbig)
					Write(g_HtmlGenCore_20TabsLine);
			}

			// Emit remaining part of required tabs.
			Write(g_HtmlGenCore_20TabsLine, effective_indent % 20);
		}
		else if (m_line_indent > 0)
		{
			// Emit spaces. Note that indentaion of one layer cannot exceed 40 because this is checked in the ctor.
			for (int ii=0; ii<effective_indent; ++ii)
				Write(g_HtmlGenCore_40SpacesLine, m_line_indent);
		}
	}
}


