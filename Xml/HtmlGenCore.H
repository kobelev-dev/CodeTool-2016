//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//	Second version of the HTML generator.
//

#ifndef	Xml_HtmlGenCore_H
#define	Xml_HtmlGenCore_H

#ifndef   Common_DoubleLinkedList_H
#include  "Common/DoubleLinkedList.H"
#endif
#ifndef   Common_FileDataHelper_H
#include  "Common/FileDataHelper.H"
#endif

class THtmlDocument;

enum THtmlCrLfMode
{
	crlm_none					=  0,

	crlm_before_whole_obj		=  1,
	crlm_after_whole_obj		=  2,

	crlm_before_first_attr		=  4,
	crlm_after_last_attr			=  8,

	crlm_before_first_child		= 16,
	crlm_after_last_child		= 32,

	crlm_before_and_after_object		= crlm_before_whole_obj | crlm_after_whole_obj,
	crlm_before_and_after_children		= crlm_before_first_child | crlm_after_last_child,
	crlm_outline_object_frames			= crlm_before_whole_obj | crlm_before_first_child | crlm_after_last_child | crlm_after_whole_obj,
	crlm_default_param_value			= -1,
};

enum THtmlDataEmitMode
{
	rdem_default				=  0,

	rdem_ignore_crlf			=  1,
	rdem_ignore_indent			=  2,
	rdem_no_std_escaping		=  4,		// Do not escape the "&", "<",">", "\"".
	rdem_no_double_escaping	=  8,		// Check for XML escape seqs and do not double escape the leading "&" there.

	rdem_escape_space			= 16,
	rdem_escape_tab_char		= 32,
	rdem_escape_tab_space		= 48,		// Escape both the tab and the space. Note that two bits are set to 1 in this mask.
};

class TRawHtmlElement : public TListItem
{
public:
		TRawHtmlElement(const wchar_t *element_name = NULL) { m_element_name = (wchar_t*)element_name; }

	const wchar_t			*ElementName() const { return(m_element_name); }

	void		SetElementName(const wchar_t *new_element_name) { m_element_name = (wchar_t*)new_element_name; }

	virtual const wchar_t	*GetIdAttr(THtmlCrLfMode &crlf_mode) { return(NULL); }
	virtual const wchar_t	*GetClassAttr(THtmlCrLfMode &crlf_mode) { return(NULL); }
	virtual const wchar_t	*GetStyleAttr(THtmlCrLfMode &crlf_mode) { return(NULL); }
							// Implementation can expect that before the call the param is always initted with the crlm_none.
							// This means that implementation should update the param only if it wants to emit the tag in non
							// default mode.

	virtual THtmlCrLfMode	GetCrLfMode() { return(crlm_none); }
	virtual bool			SpecAttrsPresent() { return(FALSE); }
	virtual bool			IsBodyPresent() { return(FALSE); }

	virtual void	EmitSpecificAttrs(THtmlDocument &doc) { }
	virtual void	EmitElementBody(THtmlDocument &doc) { }

protected:

	wchar_t				*m_element_name;
							// Name of the element can be NULL. In this case it describes the body of the container
							// or the body of some other element. The name of the element is not owned.

	friend class THtmlDocument;
};

class TSimpleHtmlElement : public TRawHtmlElement
{
public:
		TSimpleHtmlElement(const wchar_t *element_name = NULL, const wchar_t *element_body = NULL, int element_body_len = -1, const wchar_t *css_class = NULL, THtmlCrLfMode crlf_mode = crlm_default_param_value);

	virtual const wchar_t	*GetClassAttr(THtmlCrLfMode &crlf_mode) { return(m_css_class); }
	virtual THtmlCrLfMode	GetCrLfMode() { return(m_crlf_mode); }

	virtual bool			IsBodyPresent() { return(m_element_body != NULL && m_element_body_len > 0); }
	virtual void			EmitElementBody(THtmlDocument &doc);

	void		SetElementBody(const wchar_t *element_body = NULL, int element_body_len = -1);
	void		SetCssClass(const wchar_t *css_class) { m_css_class = (wchar_t*)css_class; }

protected:

	wchar_t				*m_element_body;
	int					m_element_body_len;
							// The body is not owned by the element. The string is expected to be an unescaped text.
							// This means that the stnd HTML eascaping will be applied to this data.

	wchar_t				*m_css_class;
	THtmlCrLfMode		m_crlf_mode;
};

class TAdvancedHtmlElement : public TSimpleHtmlElement
{
public:
		TAdvancedHtmlElement(const wchar_t *element_name = NULL, const wchar_t *element_body = NULL, int element_body_len = -1, const wchar_t *css_class = NULL, THtmlCrLfMode crlf_mode = crlm_default_param_value)
				: TSimpleHtmlElement(element_name, element_body, element_body_len, css_class, crlf_mode) { m_app_attr1_name = m_app_attr1_value = NULL; m_app_attr2_name = m_app_attr2_value = NULL; m_element_id[0] = 0; m_css_style = NULL; m_body_emit_mode = rdem_default; }

	virtual const wchar_t	*GetIdAttr(THtmlCrLfMode &crlf_mode) { return((m_element_id[0] != 0) ? m_element_id : NULL); }
	virtual const wchar_t	*GetStyleAttr(THtmlCrLfMode &crlf_mode) { return(m_css_style); }

	virtual bool	SpecAttrsPresent() { return(m_app_attr1_name != NULL && m_app_attr1_name[0] != 0 || m_app_attr2_name != NULL && m_app_attr2_name[0] != 0); }
	virtual void	EmitSpecificAttrs(THtmlDocument &doc);
	virtual void	EmitElementBody(THtmlDocument &doc);

	void		SetElementId(const wchar_t *id) { if (id != NULL) { wcsncpy(m_element_id, id, 60); m_element_id[59] = 0; } else { m_element_id[0] = 0; } }
	void		SetCssStyle(const wchar_t *css_style) { m_css_style = (wchar_t*)css_style; }
	void		SetFirstAppAttr(const wchar_t *attr_name, const wchar_t *attr_value) { m_app_attr1_name = (wchar_t*)attr_name; m_app_attr1_value = (wchar_t*)attr_value; }
	void		SetSecondAppAttr(const wchar_t *attr_name, const wchar_t *attr_value) { m_app_attr2_name = (wchar_t*)attr_name; m_app_attr2_value = (wchar_t*)attr_value; }
	void		SetBodyEmitMode(THtmlDataEmitMode emit_mode) { m_body_emit_mode = emit_mode; }

protected:

	wchar_t				*m_app_attr1_name;
	wchar_t				*m_app_attr1_value;

	wchar_t				*m_app_attr2_name;
	wchar_t				*m_app_attr2_value;

	wchar_t				m_element_id[60];
	wchar_t				*m_css_style;
	THtmlDataEmitMode	m_body_emit_mode;
};

class THyperLinkHtmlElement : public TSimpleHtmlElement
{
public:
		THyperLinkHtmlElement(const wchar_t *link_href, const wchar_t *element_body = NULL, int element_body_len = -1, const wchar_t *css_class = NULL, THtmlCrLfMode crlf_mode = crlm_default_param_value)
				: TSimpleHtmlElement(L"a", element_body, element_body_len, css_class, crlf_mode) { m_link_href = (wchar_t*)link_href; m_link_target = NULL; }

	virtual bool			SpecAttrsPresent() { return(TRUE); }
	virtual void			EmitSpecificAttrs(THtmlDocument &doc);

	void		SetLinkTarget(const wchar_t *link_target) { m_link_target = (wchar_t*)link_target; }

protected:

	wchar_t				*m_link_href;
	wchar_t				*m_link_target;
};

class TImageHtmlElement : public TRawHtmlElement
{
public:
		TImageHtmlElement(const wchar_t *image_src_file, const wchar_t *css_class = NULL, const wchar_t *css_style = NULL, THtmlCrLfMode crlf_mode = crlm_none)
				: TRawHtmlElement(L"img") { m_image_src_file = (wchar_t*)image_src_file; m_image_css_class = (wchar_t*)css_class; m_image_css_style = (wchar_t*)css_style; m_crlf_mode = crlf_mode; }

	virtual const wchar_t	*GetClassAttr(THtmlCrLfMode &crlf_mode) { return(m_image_css_class); }
	virtual const wchar_t	*GetStyleAttr(THtmlCrLfMode &crlf_mode) { return(m_image_css_style); }

	virtual THtmlCrLfMode	GetCrLfMode() { return(m_crlf_mode); }
	virtual bool			SpecAttrsPresent() { return(TRUE); }
	virtual void			EmitSpecificAttrs(THtmlDocument &doc);

protected:

	wchar_t				*m_image_src_file;

	wchar_t				*m_image_css_class;
	wchar_t				*m_image_css_style;
								// Css class and css style attrs of the image can be used for example to specify
								// the vertical shift of the image for small inline icons, like: L"vertical-align:-3px;".

	THtmlCrLfMode		m_crlf_mode;
};

class TListBasedHtmlContainer : public TRawHtmlElement
{
public:		//
			//  This container owns its elements.
			//

		TListBasedHtmlContainer(const wchar_t *element_name = NULL, const wchar_t *css_class = NULL, THtmlCrLfMode crlf_mode = crlm_outline_object_frames)
				: TRawHtmlElement(element_name) { m_css_class = (wchar_t*)css_class; m_crlf_mode = crlf_mode; }
		~TListBasedHtmlContainer() { m_child_elements.DeleteAll(); }

	virtual const wchar_t	*GetClassAttr(THtmlCrLfMode &crlf_mode) { return(m_css_class); }
	virtual THtmlCrLfMode	GetCrLfMode() { return(m_crlf_mode); }

	virtual bool	IsBodyPresent() { return(m_child_elements.IsEmpty() == FALSE); }
	virtual void	EmitElementBody(THtmlDocument &doc);

public:

	void			ResetContainer() { m_child_elements.DeleteAll(); }
	void			AppendElement(TRawHtmlElement *new_element) { m_child_elements.AppendItem(new_element); }
					// The ownership on the passed element IS bypassed.
protected:

	TList					m_child_elements;
							// Items are expected to be discendants of TRawHtmlElement. This list owns its items.

	wchar_t				*m_css_class;
	THtmlCrLfMode		m_crlf_mode;
};

class TArrayBasedHtmlContainer : public TRawHtmlElement
{
public:		//
			//  This container is not owning its elements. The same element may be present in this container
			//  more than once.
			//

		TArrayBasedHtmlContainer(const wchar_t *element_name = NULL, const wchar_t *css_class = NULL, THtmlCrLfMode crlf_mode = crlm_outline_object_frames)
				: TRawHtmlElement(element_name) { m_app_attr1_name = m_app_attr1_value = NULL; m_css_class = (wchar_t*)css_class; m_css_style = NULL; m_crlf_mode = crlf_mode; }

	virtual const wchar_t	*GetClassAttr(THtmlCrLfMode &crlf_mode) { return(m_css_class); }
	virtual const wchar_t	*GetStyleAttr(THtmlCrLfMode &crlf_mode) { return(m_css_style); }
	virtual THtmlCrLfMode	GetCrLfMode() { return(m_crlf_mode); }

	virtual bool	SpecAttrsPresent() { return(m_app_attr1_name != NULL && m_app_attr1_name[0] != 0); }
	virtual void	EmitSpecificAttrs(THtmlDocument &doc);

	virtual bool	IsBodyPresent() { return(m_elements.NumItems() > 0); }
	virtual void	EmitElementBody(THtmlDocument &doc);

public:

	void			SetCssClass(const wchar_t *css_class) { m_css_class = (wchar_t*)css_class; }
	void			SetCssStyle(const wchar_t *css_style) { m_css_style = (wchar_t*)css_style; }
	void			SetFirstAppAttr(const wchar_t *attr_name, const wchar_t *attr_value) { m_app_attr1_name = (wchar_t*)attr_name; m_app_attr1_value = (wchar_t*)attr_value; }

	void			ResetContainer() { m_elements.Clear(); m_app_attr1_name = m_app_attr1_value = NULL; }
	bool			AppendElement(TRawHtmlElement *new_element) { return(m_elements.AppendItem(new_element)); }
	bool			AppendElements(TRawHtmlElement *nel1, TRawHtmlElement *nel2, TRawHtmlElement *nel3 = NULL, TRawHtmlElement *nel4 = NULL, TRawHtmlElement *nel5 = NULL,
									TRawHtmlElement *nel6 = NULL, TRawHtmlElement *nel7 = NULL, TRawHtmlElement *nel8 = NULL);
					// The ownership on the elements is NOT passed.
protected:

	TStructsArray<TRawHtmlElement*, 16, 256>		m_elements;
													// Items are expected to be discendants of TRawHtmlElement.
													// Array is not owning its items.

	wchar_t				*m_app_attr1_name;
	wchar_t				*m_app_attr1_value;

	wchar_t				*m_css_class;
	wchar_t				*m_css_style;
	THtmlCrLfMode		m_crlf_mode;
};

enum THtmlDocumentType
{
	dctp_doctype_none,
	dctp_html_401_transitional,
	dctp_html_401_strict,
	dctp_html_401_frameset,
	dctp_xhtml_10_transitional,
	dctp_xhtml_10_strict,
	dctp_xhtml_10_frameset,
};

class THtmlDocument : public TDestinationFile
{
public:
		THtmlDocument(const wchar_t *stream_name, bool document_fragment_mode = FALSE, int line_indent = -1);
		~THtmlDocument();
			// This class is not automatically openning the destination file or preparing the in memory stream.
			// App layer should use methods of the base class for this.

	inline bool	IsDocFragmentMode() const { return(m_doc_fragment_mode); }

	bool		Close();
				// This method overwrites similar method of the base class. It attempts to close the unclosed layers
				// if possible and calls the method of the base class.

	void		OpenDocumentHead(THtmlDocumentType doc_type = dctp_html_401_strict);
	void		OpenDocumentBody();
	void		CloseDocument();
				// These methods can be used only when the object is instantiated in the whole document mode.
				// Current design expects that in the whole doc mode all three methods above should be called.

	void		AddHtmlComment(const wchar_t *comment_body, THtmlCrLfMode crlf_mode = crlm_before_and_after_object);
				// The comment can be added at any point except for when the tag header is opened. Note that W3C XML spec
				// tells that contents of the comment should not be escaped. This is done so to allow commenting out sections
				// of the document that contain markup inside.

	void		AddPageTitle(const wchar_t *page_title);
	void		AddInlineCssStyle(const wchar_t *css_style);
	void		AddCssFileRef(const wchar_t *dir_or_url, const wchar_t *file_name);
	void		AddCssFileReferences(const wchar_t *dir_or_url, const wchar_t **file_names);
	void		AddJavaScriptFileRef(const wchar_t *dir_or_url, const wchar_t *file_name);
				// All these statements are emitted on a separate line.

	void		AddSoftNewLine() { EmitRawData(NULL, 0, TRUE, FALSE); }
				// This method can be called at any time.

	void		SetupIndentDelta(int new_value) { m_indent_delta = new_value; }

	void		EmitElement(TRawHtmlElement &element);
	void		EmitElementBeg(TRawHtmlElement &element);
	void		EmitElementEnd(TRawHtmlElement &element);

	void		EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2);
	void		EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3);
	void		EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4);
	void		EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4, TRawHtmlElement &el5);
	void		EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4, TRawHtmlElement &el5, TRawHtmlElement &el6);
	void		EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4, TRawHtmlElement &el5, TRawHtmlElement &el6, TRawHtmlElement &el7);
	void		EmitElements(TRawHtmlElement &el1, TRawHtmlElement &el2, TRawHtmlElement &el3, TRawHtmlElement &el4, TRawHtmlElement &el5, TRawHtmlElement &el6, TRawHtmlElement &el7, TRawHtmlElement &el8);

	void		EmitAttribute(const wchar_t *attr_name, const wchar_t *attr_value, THtmlCrLfMode attr_crlf_mode = crlm_none);
	void		EmitRawData(const wchar_t *data_beg, long data_len, int front_crlf_flags = 0, int trailing_crlf_flags = 0, THtmlDataEmitMode emit_mode = rdem_default);

protected:

	void		EmitElementBegInternal(TRawHtmlElement &element, bool element_body_expected, THtmlCrLfMode element_crlf_mode);
	void		EmitElementEndInternal(TRawHtmlElement &element, THtmlCrLfMode element_crlf_mode);

	void		HtmlIndentLine();

protected:

	bool						m_doc_fragment_mode;

	bool						m_page_header_opened;
	bool						m_page_body_opened;

	bool						m_tag_header_opened;
	int						m_cnt_opened_layers;

	int						m_indent_delta;					// Application layer can setup its own delta to the current indentation.
															// This data vield is not used for any other purposes.

	int						m_line_indent;					//	 -1 - use TAB characters.
															//	  0  - no indentation at all.
															//	 >0 - use spaces for indenting the line.
	bool						m_line_dirty;

};

#endif	// Xml_HtmlGenCore_H


