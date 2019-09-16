//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//
//	Second version of the HTML generator.
//

#ifndef	Xml_HtmlGenDecors_H
#define	Xml_HtmlGenDecors_H

#ifndef   Common_HashTable_H
#include  "Common/HashTable.H"
#endif
#ifndef   Xml_HtmlGenCore_H
#include  "Xml/HtmlGenCore.H"
#endif

class THtmlGenAppDelegate : public TListItem
{
public:		//
			// This object is responsible for emitting only the application layer attributes. All other parts of
			// the HTML element like its name (<span>, <img>, etc.), CSS class, crlf mode, contents, etc.
			// are provided by the layer of the decorated file generator.
			//
			// Note that this class is not derived from the raw HTML element.
			//

	virtual const wchar_t	*GetIdAttr(THtmlCrLfMode &crlf_mode) { return(NULL); }
	virtual void			EmitSpecificAttrs(THtmlDocument &doc) { }
};

class THtmlGenDecorOwnersContext
{
public:

	THtmlGenDecorOwnersContext(int &cnt_owner_creation_errors) : m_owner_object_creation_errors(cnt_owner_creation_errors) { }
	~THtmlGenDecorOwnersContext() { ResetContext(); }

	void		ResetContext();
	void		ReleaseOtherOwners() { m_other_owners.DeleteAll(); }
				// These methods are not resetting the count of errors.
protected:

	struct TSimpleDecorOwner : public TRawHtmlElement
	{
		TSimpleDecorOwner(const wchar_t *css_class, const wchar_t *css_style, const wchar_t *img_src);
			// The name of the HTML element is either <span> or <img> depending on the value
			// of the img_src parameter, i.e. if it is NULL or not.

		virtual const wchar_t	*GetIdAttr(THtmlCrLfMode &crlf_mode) { return((m_elem_app_delegate != NULL) ? m_elem_app_delegate->GetIdAttr(crlf_mode) : NULL); }
		virtual const wchar_t	*GetClassAttr(THtmlCrLfMode &crlf_mode) { return(m_elem_css_class); }
		virtual const wchar_t	*GetStyleAttr(THtmlCrLfMode &crlf_mode) { return(m_elem_css_style); }

		virtual bool	SpecAttrsPresent() { return(m_elem_img_src != NULL || m_elem_app_delegate != NULL); }
		virtual bool	IsBodyPresent() { return(m_elem_img_src == NULL); }
		virtual void	EmitSpecificAttrs(THtmlDocument &doc);

		bool		CompareProps(const wchar_t *css_class, const wchar_t *css_style, const wchar_t *img_src);

	public:

		wchar_t					*m_elem_css_class;
		wchar_t					*m_elem_css_style;

		wchar_t					*m_elem_img_src;
		THtmlGenAppDelegate		*m_elem_app_delegate;
	};

	struct TAppAttrsDecorOwner : public TRawHtmlElement
	{
		TAppAttrsDecorOwner(const wchar_t *element_name, const wchar_t *attr1_name, const wchar_t *attr1_value, const wchar_t *attr2_name = NULL, const wchar_t *attr2_value = NULL)
			: TRawHtmlElement(element_name) { m_attr1_name = (wchar_t*)attr1_name; m_attr1_value = (wchar_t*)attr1_value; m_attr2_name = (wchar_t*)attr2_name; m_attr2_value = (wchar_t*)attr2_value; }

		virtual bool	SpecAttrsPresent() { return(TRUE); }
		virtual void	EmitSpecificAttrs(THtmlDocument &doc);

	protected:

		wchar_t					*m_attr1_name;
		wchar_t					*m_attr1_value;

		wchar_t					*m_attr2_name;
		wchar_t					*m_attr2_value;
	};

public:

	TRawHtmlElement		*FindOrCreateSimpleOwner(const wchar_t *css_class, const wchar_t *css_style = NULL, const wchar_t *img_src = NULL);
	TRawHtmlElement		*CreateAppDelegateProxy(THtmlGenAppDelegate *app_delegate, const wchar_t *css_class, const wchar_t *css_style = NULL, const wchar_t *img_src = NULL);
							// The names of the CSS classes, the styles and the names of the image files are expected to be static strings.

	TRawHtmlElement		*CreateHyperLinkOwner(const wchar_t *href_value);
	TRawHtmlElement		*CreateMarksAreaLabel(const wchar_t *label_value, const wchar_t *css_class);
	TRawHtmlElement		*CreateAppAttrsOwner(const wchar_t *attr1_name, const wchar_t *attr1_value, const wchar_t *attr2_name = NULL, const wchar_t *attr2_value = NULL);
							// The href value, the label value and the values of the custom attrs are all cached in the local strings dict.
protected:

	int								&m_owner_object_creation_errors;
										// This counter reflects the owner object allocation errors and misc strings dict errors.
	enum { NUM_BUCKETS = 32 };

	TList								m_simple_owners[NUM_BUCKETS];
	TList								m_other_owners;

	TStringsDict						m_misc_app_strings;
};

//
//  =================  Decorated Area  ===================
//

// This struct is not inteneded to be used by the app layer.
struct THtmlBasicDecor
{
	TRawHtmlElement			*m_basic_owner;			// The owner object provides attributes of the element if this element will be
														// rendered. Contents of the element are always taken from the data that is
														// being decorated. If the owner is NULL, the area is not rendered.
	short					m_zorder;

	long						m_decor_area_beg;			// File offset of the beginning of the area in the source file.
	long						m_decor_area_len;			// Length of the area in WCHARs. Length of the area cannot be zero.
														// Decors with empty areas should be filtered out by the upper layer.

	static int CompareAsBkgrDecors(THtmlBasicDecor &dcr1, THtmlBasicDecor &dcr2)
	{
		if (dcr1.m_decor_area_beg < dcr2.m_decor_area_beg)
			return(-1);
		else if (dcr1.m_decor_area_beg > dcr2.m_decor_area_beg)
			return(1);

		// Both decorations start on the same place.
		if (dcr1.m_zorder < dcr2.m_zorder)
			return(-1);
		else if (dcr1.m_zorder > dcr2.m_zorder)
			return(1);

		// The props are identical.
		return(0);
	}
};

// This struct is not inteneded to be used by the app layer.
struct THtmlSpacelessDecor
{
	TRawHtmlElement			*m_spaceless_owner;		// The owner object provides attributes and contents of the element if this
														// element will be rendered. If the owner is NULL, the decor is considered
														// to be a special tab.
	long						m_decor_area_offs;
	short					m_horz_order;				// Keep two short fields together for memory footprint reasons.

	short					m_spec_tab_clmns;
	wchar_t					*m_spec_tab_new_bkgr;
								// These fields are relevant for spec tab decorations only. When the value of the first field is greater
								// or equal than 2, the "colspan" attr is generated using special rules that are not the same to the last
								// spec tab object. When the value of second field is not NULL, css style of the line.
	enum
	{
		BKGR_COVER_LEFT	= -200,		// Decors with horz order equal or less than this value are covered with bkgr decor if any.
		BKGR_COVER_RIGHT	= 200,		// Decors with horz order equal or greater than this value are covered with bkgr decor if any.
	};

public:

	bool IsSpecialTab() const
	{
		return(m_spaceless_owner == NULL);
	}

	static int CompareHorzPositions(THtmlSpacelessDecor &dcr1, THtmlSpacelessDecor &dcr2)
	{
		if (dcr1.m_decor_area_offs < dcr2.m_decor_area_offs)
			return(-1);
		else if (dcr1.m_decor_area_offs > dcr2.m_decor_area_offs)
			return(1);

		// Both decorations are located on the same place.
		if (dcr1.m_horz_order < dcr2.m_horz_order)
			return(-1);
		else if (dcr1.m_horz_order > dcr2.m_horz_order)
			return(1);

		// The horz positions are identical.
		return(0);
	}
};

typedef TStructsArray<THtmlBasicDecor, 24, 256> THtmlBasicDecorsArray;
typedef TStructsArray<THtmlBasicDecor*, 256, 4096> THtmlBasicDecorPtrsArray;
typedef TStructsArray<THtmlSpacelessDecor, 8, 32> THtmlSpacelessDecorsArray;

struct THtmlObjectEmittingHelper
{
		THtmlObjectEmittingHelper(THtmlDocument &doc)
				: m_doc(doc) { m_data_emit_mode = rdem_default; m_line_nums_id_prefix = NULL; m_num_subcolumns = 0; }

		void PreprareForRendering(const wchar_t *initial_css_class, int num_curr_line_subcolumns)
		{
			m_opened_bkgr_decors.Clear();
			m_curr_css_class = (wchar_t*)initial_css_class;
			m_curr_line_subcolumns = (m_num_subcolumns >= 2) ? num_curr_line_subcolumns : 0;
			m_inx_subcolumn = 0;
		}

public:

	THtmlDocument				&m_doc;

	THtmlDataEmitMode			m_data_emit_mode;
									// This field contains raw data emitting flags like the "rdem_no_std_escaping" that should be set
									// when the data in the area is already HTML escaped.

	wchar_t						*m_line_nums_id_prefix;
									// Each line number <td> element is marked with an id attribute. The value of the id is generated
									// using either a "%sL%ld"  format simple "L%ld" format if the value of this field is NULL.

	int							m_num_subcolumns;
									// When the number of subcolumns is zero, the spec tab decors are ignored.

	THtmlBasicDecorPtrsArray		m_overlap_buffer;
									// This buffer is used for evaluating the state of the inline frgr decorations.

	THtmlBasicDecorPtrsArray		m_opened_bkgr_decors;
									// This bufffer is used for keeping track the state of rendering of the inline bkgr decors.

	wchar_t						*m_curr_css_class;

	int							m_curr_line_subcolumns;
	int							m_inx_subcolumn;
									// Information about the emitting state on the current line.

public:

	void AddDataEmittingFlags(DWORD new_flags)
	{
		m_data_emit_mode = (THtmlDataEmitMode)(m_data_emit_mode | new_flags);
	}

	int FindInsBeforeIndex(short new_object_zorder)
	{
		for (int istk=m_opened_bkgr_decors.NumItems()-1; istk >= 0; --istk)
		{
			if (m_opened_bkgr_decors[istk]->m_zorder <= new_object_zorder)
				return(istk+1);
		}

		// Object with the passed zorder should be inserted into the beginning of the array.
		return(0);
	}

	int FindBkgrEndLayerAt(int start_from_layer, int char_index)
	{
		// Negative staritng layers are ok for this function.
		assert(start_from_layer < m_opened_bkgr_decors.NumItems());

		for (int istk=start_from_layer; istk >= 0; --istk)
		{
			THtmlBasicDecor *dcr = m_opened_bkgr_decors[istk];
			if (dcr->m_decor_area_beg+dcr->m_decor_area_len == char_index)
				return(istk);
		}

		// None of the opened decors end right before the passed char place.
		return(-1);
	}
};

class THtmlGenericDecoratedArea : public TRawHtmlElement
{
public:
		THtmlGenericDecoratedArea(const wchar_t *element_name, const wchar_t *line_start_css_class = NULL)
				: TRawHtmlElement(element_name) { m_initial_css_style = (wchar_t*)line_start_css_class; m_first_subclmn_width = 1; }

	void		ResetDecorations();

	bool		SetBkgrDecor(TRawHtmlElement *owner, short zorder, long area_beg, long area_len);
	bool		SetFrgrDecor(TRawHtmlElement *owner, short zorder, long area_beg, long area_len);
	bool		SetSpacelessDecor(TRawHtmlElement *owner, short horz_order, long decor_offs);
				// Note that ownership on the decoration owners is NOT bypassed. The caller is responsible for
				// releasing these objects if this is necessary.

	bool		SetSpecialTab(short horz_order, long decor_offs, const wchar_t *spec_tab_bkgr = NULL, short spec_tab_num_clmns = 1);
				// Horz order of the special tabs can be used for placing the spaceless decors either in front of
				// the spec tab or after the spec tab.

	bool		EmitArea(THtmlObjectEmittingHelper &emt_ctx, const wchar_t *body_data_ptr, long body_data_len, long area_beg, long area_len);
				// Note that the name of the element is passed as the parameter of the ctor. The element is always emitted
				// in the full form: <name> ... </name> even if the body of the element is empty. This turns out to be good
				// because browsers testing (Apr 2012) shows that they tend to work unstabe with short forms of the element
				// like <name/>. The number of subcolumns is passed in the object emitting context.
protected:

	bool		AddSpacelessDecorInternal(THtmlSpacelessDecor &decor_struct);

	bool	CheckBkgrBegAt(int bkgr_inx, int char_index)
	{
		return(bkgr_inx < m_bkgr_decors.NumItems() && m_bkgr_decors[bkgr_inx].m_decor_area_beg <= char_index);
	}

	bool	CheckSpacelessAt(int spaceless_inx, int char_index)
	{
		return(spaceless_inx < m_spaceless_decors.NumItems() && m_spaceless_decors[spaceless_inx].m_decor_area_offs <= char_index);
	}

	bool	CheckSpacelessAt(int spaceless_inx, int char_index, short horz_order_limit)
	{
		if (spaceless_inx >= m_spaceless_decors.NumItems())
			return(FALSE);

		THtmlSpacelessDecor &decor = m_spaceless_decors[spaceless_inx];
		return(decor.m_decor_area_offs <= char_index && decor.m_horz_order <= horz_order_limit);
	}

	int NumSubColumns() const
	{
		int num_subcolumns = m_first_subclmn_width;

		for (int isp1=0; isp1<m_spaceless_decors.NumItems(); ++isp1)
		{
			if (m_spaceless_decors[isp1].IsSpecialTab() == TRUE)
			{
				short nclmns = m_spaceless_decors[isp1].m_spec_tab_clmns;
				num_subcolumns += (nclmns >= 2) ? nclmns : 1;
			}
		}

		return(num_subcolumns);
	}

protected:

	struct TDecoratedAreaProxy : public TRawHtmlElement
	{
			TDecoratedAreaProxy(TRawHtmlElement *base_element) : TRawHtmlElement(base_element->ElementName())
					{ m_base_element = base_element; m_css_class_attr = m_css_style_attr = NULL; m_colspan_buff[0] = 0; }

		virtual const wchar_t	*GetIdAttr(THtmlCrLfMode &crlf_mode) { return(m_base_element->GetIdAttr(crlf_mode)); }
		virtual const wchar_t	*GetClassAttr(THtmlCrLfMode &crlf_mode) { return(m_css_class_attr); }
		virtual const wchar_t	*GetStyleAttr(THtmlCrLfMode &crlf_mode) { return(m_css_style_attr); }

		virtual THtmlCrLfMode	GetCrLfMode() { return(crlm_before_and_after_object); };
		virtual bool			SpecAttrsPresent() { return(m_colspan_buff[0] != 0 || m_base_element->SpecAttrsPresent()); };
		virtual bool			IsBodyPresent() { return(TRUE); };

		virtual void	EmitSpecificAttrs(THtmlDocument &doc);

		void			PrepareExtAttrs(THtmlObjectEmittingHelper &hlpr, const wchar_t *spec_tab_css_style, short spec_tab_num_clmns);

		TRawHtmlElement		*m_base_element;

		wchar_t				*m_css_class_attr;
		wchar_t				*m_css_style_attr;

		wchar_t				m_colspan_buff[16];
	};

	void		ProcessSpacelessDecor(THtmlObjectEmittingHelper &emt_ctx, TDecoratedAreaProxy &element_proxy, int spaceless_inx);

protected:

	THtmlBasicDecorsArray				m_bkgr_decors;
										// Inline background decorations are ordered by the area beg as the primary key and by the zorder as the secondary key.

	THtmlBasicDecorsArray				m_frgr_decors;
										// Inline foreground decorations are ordered by their zorder. Offsets inside the same zorder are not ordered.

	THtmlSpacelessDecorsArray			m_spaceless_decors;
										// Spaceless decorations are ordered by the area beg as the primary key and by the horz order as the secondary key.

	wchar_t					*m_initial_css_style;
	short					m_first_subclmn_width;

	friend class THtmlDecoratedLineInfo;
	friend class THtmlDecoratedFile;
};

//
//  ===================  Decorated File  ===================
//

class THtmlDecoratedLineInfo
{
public:		//
			//	This class generates html element with the fixed type: <tr>.
			//

		THtmlDecoratedLineInfo() : m_marks_area(L"td", NULL, crlm_before_and_after_object), m_line_decors(L"td")
					{ m_nums_css_style = NULL; m_line_num_frgr_decor = NULL; }

	void		ResetDecorations();

	void		SetWholeLineStyles(const wchar_t *nums_css_style, const wchar_t *marks_css_style, const wchar_t *data_css_style);
	void		SetLineNumberStyle(TRawHtmlElement *frgr_decor) { m_line_num_frgr_decor = frgr_decor; }
	void		SetFirstSubcolumnWidth(int num_subclmns) { m_line_decors.m_first_subclmn_width = num_subclmns; }

	void		ResetMarksArea() { m_marks_area.ResetContainer(); }
	bool		MarksPresent() { return(m_marks_area.IsBodyPresent()); }
	bool		AppendMark(TRawHtmlElement *new_mark) { return(m_marks_area.AppendElement(new_mark)); }
				// The ownership on the mark object is NOT passed.

	THtmlGenericDecoratedArea		&LineDecors() { return(m_line_decors); }

	bool		EmitLine(THtmlObjectEmittingHelper &emt_ctx, long line_number, const wchar_t *body_data_ptr, long body_data_len = -1, long line_area_beg = 0, long line_area_len = -1);
				// Note that the number of subcolumns is passed in the emitting context object.
protected:

	wchar_t						*m_nums_css_style;

	TRawHtmlElement				*m_line_num_frgr_decor;		// This element is used as a wrapper for the line number, i.e. when the line number is emitted,
																// a beg for this object is emitted, then the line number as a raw data and then an element end.
																// This means that this object can provide any name of the element and any attrs, but not
																// the body of the element.
	TArrayBasedHtmlContainer		m_marks_area;
	THtmlGenericDecoratedArea		m_line_decors;

	friend class THtmlDecoratedFile;
};

struct THtmlDecoratedFileProps
{
	wchar_t						*m_def_nums_css_style;
	wchar_t						*m_def_marks_css_style;
									// Style of the line nums area and style the marks areas are expected to have fixed width.

	wchar_t						*m_def_line_data_css_style;
									// The line data style should not have fixed width.

	int							m_top_area_height;
	int							m_bottom_area_height;

	THtmlDataEmitMode			m_file_data_emit_mode;

	inline void Clear() { memset(this, 0, sizeof(THtmlDecoratedFileProps)); }
};

class THtmlDecoratedFile : public TRawHtmlElement
{
public:		//
			//	This class generates html element with the fixed type: <div>.
			//
			//	The CSS class, CSS style and the CrLf mode are used exactly as they are passed into the ctor.
			//	These attrs are never changed by the code inside and they cannot be changed using methods
			//	of the class.
			//

		THtmlDecoratedFile(const wchar_t *css_class = NULL, const wchar_t *css_style = NULL, THtmlCrLfMode crlf_mode = crlm_outline_object_frames);
		~THtmlDecoratedFile() { ResetDecoratedFile(); }

	virtual const wchar_t	*GetIdAttr(THtmlCrLfMode &crlf_mode) { return((m_dfl_element_id_div[0] != 0) ? m_dfl_element_id_div : NULL); }
	virtual const wchar_t	*GetClassAttr(THtmlCrLfMode &crlf_mode) { return(m_dfl_css_class_div); }
	virtual const wchar_t	*GetStyleAttr(THtmlCrLfMode &crlf_mode) { return(m_dfl_css_style_div); }
	virtual THtmlCrLfMode	GetCrLfMode() { return(m_dfl_crlf_mode_div); }

	virtual bool	IsBodyPresent() { return(TRUE); };
					// Derived classes should not overwrite this method.

	virtual void	EmitElementBody(THtmlDocument &doc);
					// This method emits a sequence of one ro more <table>...</table> elements if the setup
					// is fine or otherwise a singe <p> element with an error message.

	void			EmitLinesRange(THtmlDocument &doc, long line_beg, long num_lines, bool want_top_area = TRUE, bool want_bottom_area = TRUE);
					// This method emits either a sequence of one ro more <table>...</table> elements if the params
					// are fine or a singe <p> element with an error message otherwise.

	void			EmitSpecialInsert(THtmlDocument &doc, long line_num_after, const wchar_t *msg_css_class, const wchar_t *msg_body);

	void		ResetDecoratedFile();

	void		SetupElementId(const wchar_t *element_id);
	void		SetupCssClassForTable(const wchar_t *css_class) { m_dfl_css_class_table = (wchar_t*)css_class; }
				// These methods should be called after calling the main Setup() method.

	bool		Setup(THtmlDecoratedFileProps &props,							// The values, passed in the props cannot be changed later using the methods
																		// of the class.
					const wchar_t *line_numbers_id_prefix,					// Once this prefix is not empty, id attr of each line number element is generated
																		// using the format "%sL%ld". Otherwise simple "L%ld" is used.
					const wchar_t *fdata, long fdata_len_in_wchars = -1,
																		// The file data can be NULL. The ownership on the file data is not bypassed.
					long		fdata_src_offs = 0,							// Source offset of the first byte of the passed file data.
																		// Passed value is not changed by the Setup() method in any way.
					long		fdata_first_line = 1,							// Line number of the first line of the passed file data.
																		// Passed value is not changed by the Setup() method in any way.
					TSimpleLineInfo *linfo = NULL, long linfo_len = 0,			// The lines info should be either missing or it should describe the file data.
																		// The ownership on the lines info is not bypassed.
					long	first_visib_line = 1,								// Param value can be bigger and smaller than the fisrt_fdata_line.
																		// Passed value is not changed by the Setup() method in any way.
					long	num_visib_lines = 0);							// The value of the param can be 0. This means the file should be displayed
																		// up to its end. In any case at least one line will be displayed.

	bool		SetBkgrDecoration(TRawHtmlElement *owner, short zorder, long area_beg, long area_len);
	bool		SetFrgrDecoration(TRawHtmlElement *owner, short zorder, long area_beg, long area_len);
	bool		SetSpacelessDecoration(TRawHtmlElement *element, short horz_order, long area_offs);
	bool		SetSpecialTabDecoration(short horz_order, long area_offs);

	bool		SetWholeLineBkgrDecoration(long line_beg, long num_lines, const wchar_t *nums_css_style, const wchar_t *marks_css_style, const wchar_t *line_data_css_style);
	bool		SetLineNumberFrgrDecoration(long line_beg, long num_lines, TRawHtmlElement *owner);
	bool		SetSpecTabsAreaDecoration(long line_beg, long num_lines);
	bool		SetMarksAreaDecoration(long line_number, short horz_order, TRawHtmlElement *element);

	bool		SetTopBottomAreaDecoration(bool cover_top, bool cover_bottom, const wchar_t *nums_css_style, const wchar_t *marks_css_style, const wchar_t *line_data_css_style);

				//
				// Notes on using the HTML elements that are passed into the methods above.
				//
				//	Bkgr, Frgr and LineNumber -- these objects are wrappers. They provide the name of the element and they
				//								provide the attributes. They cannot provide the body of the element.
				//
				//	Spaceless and MarksSpaceless -- these objects are full HTML elements. They can have any nature and
				//								complexity.
				//
				//	Ownership on all these objects is not bypassed. Application layer should allocate and destroy them.
				//

	bool		GetIntersectionWithVisibleFileArea(long &area_beg, long &area_len, bool allow_zero_length_area);
	bool		GetIntersectionWithTheVisibleLinesRange(long &line_beg, long &num_lines);

	bool		IsMarksAeaEmpty(long line_number) { THtmlDecoratedLineInfo *linfo = GetDcrtLineInfo(line_number); return(linfo == NULL || linfo->MarksPresent() == FALSE); }

	long				GetLineNumber(long src_offset);
	TSimpleLineInfo	*GetLineInfo(long line_number);

protected:

	THtmlDecoratedLineInfo		*IterateVisibleFileSpace(TFileSpaceIterationInfo &info);
	THtmlDecoratedLineInfo		*IterateVisibleLinesSpace(long &line_beg, long &num_lines);
	THtmlDecoratedLineInfo		*GetDcrtLineInfo(long line_number);

	struct TLinesRangeEmtCtx
	{
			TLinesRangeEmtCtx(const wchar_t *css_class) : m_table_element(L"table", NULL, -1, css_class)
					{ m_first_range = TRUE; m_table_element.SetCssStyle(L"width:100%"); }

		bool						m_first_range;
		TAdvancedHtmlElement		m_table_element;
	};

	void		EmitTopBottmArea(THtmlDocument &doc, wchar_t **area_decors, int area_height);
	void		EmitLinesRangeLow(THtmlObjectEmittingHelper &hlpr, TLinesRangeEmtCtx &emt_ctx, int spec_tabs_area_inx, long line_beg, long num_lines);

	struct TSpecTabsAreaInfo
	{
		long			m_line_start;
		long			m_num_lines;
	};

	typedef TStructsArray<TSpecTabsAreaInfo, 64, 256> TSpecTabsAreaInfosArray;

protected:

	THtmlDecoratedFileProps			m_props;						// The props are not changing between the calls to the method setup.
	wchar_t							m_line_numbers_id_prefix[40];

	wchar_t							*m_file_data;					// Can be NULL. The file data is not owned.
	long								m_file_data_len;					// Length of the file data in wchars. The file data may contail NULL chars inside it.

	long								m_file_data_src_offs;			// The source offset of the first byte in the file data. This field is used for processing
																	// offsets of the areas that are occupied with decorations.
	long								m_file_data_first_line;			// Line number of the first line of the file data. Even when the length of the file data
																	// is zero, this file still contains one line and this line has a line number.

	TSimpleLineInfo					*m_lines_info;					// The value of this field is NULL only when the file is missing.
	long								m_lines_info_len;				// Number of lines in the file data. The value can be zero only when the file is missing.
	bool								m_lines_info_owned;

	THtmlDecoratedLineInfo				*m_display_lines;				// The value of the field is NULL only when the file is missing.
	long								m_display_lines_len;				// Number of elements in the m_display_lines array.
	long								m_display_lines_first_line;		// Line number of the first displayable line. Note that the value of this field is always
																	// exacly equal to the first_visib_line as it was passed into Setup() and this value
																	// has nothing to do with lines that are available in the file data.
	long								m_first_isect_line;
	long								m_num_isect_lines;
										// Information about the intersection between the file data and the requested range of lines
										// to display. This intersection can be empty. In this case the m_num_isect_lines is zero.

	TSpecTabsAreaInfosArray			m_subcolumns_info;				// Array of line ranges of lines that share the horz positions of the spec tabs. Each line
																	// that belongs to the certain subrange has its spec tabs on the same place as any other
																	// line that belongs to that subrange.
	wchar_t							*m_top_area_decors[3];
	wchar_t							*m_bottom_area_decors[3];

private:

	wchar_t							m_dfl_element_id_div[40];
	wchar_t							*m_dfl_css_class_table;

	wchar_t							*m_dfl_css_class_div;
	wchar_t							*m_dfl_css_style_div;
	THtmlCrLfMode					m_dfl_crlf_mode_div;
};

#endif	// Xml_HtmlGenDecors_H


