//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Common_XmlWriter_H
#define	Common_XmlWriter_H

#ifndef   Common_FileDataHelper_H
#include  "Common/FileDataHelper.H"
#endif
#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif
#ifndef   Common_XmlTagsDict_H
#include  "Xml/XmlTagsDict.H"
#endif

#ifndef   WinUi_DialogControlHelpers_H			// This header is included here for picking up the TColor definition.
#include  "WinUi/DialogControlHelpers.H"
#endif

enum TXmlWriterErrorCode
{
	xwre_none = 0,

	xwre_dctr_e1		=  400,
	xwre_dctr_e2		=  401,
	xwre_clcn_e1		=  402,
	xwre_clcn_e2		=  403,
	xwre_chcl_e1		=  404,
	xwre_chst_e1		=  405,

	xwre_lkrc_e1		=  410,
	xwre_lkrc_e2		=  411,
	xwre_lkrc_e3		=  412,

	xwre_lktg_e1		=  420,
	xwre_lktg_e2		=  421,
	xwre_lktg_e3		=  422,
	xwre_lktg_e4		=  423,
	xwre_lktg_e5		=  424,
	xwre_lktg_e6		=  425,
	xwre_lktg_e7		=  426,
	xwre_lktg_e8		=  427,

	xwre_opcn_e1		=  430,
	xwre_opcn_e2		=  431,
	xwre_wrtg_e1		=  432,
	xwre_wrsc_e1		=  433,

	// App level may add its own error messages with ids starting from the value below.
	xwre_app_failure	=  700,
};

//
//  Instructions on adding/not adding the CR/LF when emitting the element.
//
enum TXmlItemCrLfMode
{
	//
	// These constants should be used in the function calls.
	//

	crlf_inherit				= 0,		// Use container default in front, use container default after the item.
	crlf_inherit_none,				// Use container default in front, no CR/LF after the item.
	crlf_inherit_add,					// Use container default in front, add CR/LF after the item.
	crlf_none_inherit		= 4,		// No CR/LF in front of the item, use container default after the item.
	crlf_none_none,					// No CR/LF in front of the item, no CR/LF after the item.
	crlf_none_add,					// No CR/LF in front of the item, add CR/LF after the item.
	crlf_add_inherit			= 8,		// Add CR/LF in front of the item, use container default after the item.
	crlf_add_none,					// Add CR/LF in front of the item, no CR/LF after the item.
	crlf_add_add,					// Add CR/LF in front of the item, add CR/LF after the item.

	//
	// Constants for internal use inside the XML writer.
	//

	crlf_prepend_inherit		=  0,
	crlf_prepend_none		=  4,
	crlf_prepend_add		=  8,
	crlf_prepend_MASK		= 12,

	crlf_append_inherit		=  0,
	crlf_append_none		=  1,
	crlf_append_add			=  2,
	crlf_append_MASK			=  3,
};

struct TXmlCnrWritingProps
{
	bool				m_use_short_tag_close;
						// When the value of this field is TRUE, the field tags are emitted like: <SomeName> Value </> instead
						// of <SomeName> Value </SomeName>. Tags with empty value are always emitted as <SomeName/>
						// and this cannot be controlled by props. This field affects only the field names. Names of the containers
						// are always emitted in the full form.

	bool				m_crlf_before_first_attr;
	bool				m_crlf_before_other_attr;
	bool				m_crlf_before_first_field;
	bool				m_crlf_before_other_field;
	bool				m_crlf_before_first_subcnr;
	bool				m_crlf_before_other_subcnr;

	bool				m_crlf_before_tag_hdr_close;
	bool				m_crlf_before_container_close;

	bool				m_crlf_after_attr;
	bool				m_crlf_after_field;
	bool				m_crlf_after_subcnr;
};

//
// Apps should only read from these structures. They should not good to modify them.
//
extern const TXmlCnrWritingProps g_XmlWriter_StdRootContainerProps;
extern const TXmlCnrWritingProps g_XmlWriter_StdSubContainerProps;

enum TXmlWriterBoolStyle
{
	xwbs_true_false,
	xwbs_yes_no,
	xwbs_on_off,
	xwbs_1_0,
};

//
// XML writer escapes the following characters:
//
//		L'&'		==>		L"&amp;"
//		L'<'		==>		L"&lt;"
//		L'>'		==>		L"&gt;"
//		L'\"'		==>		L"&quot;"
//
class TXmlWriter : public TDestinationFile
{
public:
			TXmlWriter(const wchar_t *streamName, TGenericConsole *cons = NULL, int lineIndent = 4);
			TXmlWriter(const wchar_t *streamName, TMessageBuffer &errorsBuffer, int lineIndent = 4);
				//
				//  When the indentation param is negative, then the tab character is used for indentation. When it is zero, there is no
				//  indentation at all. Otherwise the value of the param tells the number of spaces in one indentation unit.
				//
				//  Note that before calling OpenRootContainer() it is necessary to call PrepareDiskFile() or PrepareInMemoryStream()
				//  methods in the base class.
				//

			~TXmlWriter();

	bool		GetLineDirtyState() const		{ return(m_line_dirty); }
	int		GetCurrentNesting() const		{ return(m_nesting_level); }
	int		GetErrorsCount() const		{ return(m_errors_count); }

	void		SetupRootContainersDict(const TXmlDictRecord *tagsDict, const TXmlCnrWritingProps *rootContainerProps = NULL, const TXmlCnrWritingProps *subContainerProps = NULL);
	void		ResetRootContainersDict()	{ m_root_cnrs_dict = NULL; }

	TXmlDictRecord *GetRootContainersDict() const { return(m_root_cnrs_dict); }

	void		WriteXmlHeader(const wchar_t *headerContents = NULL, bool appendCrlf = TRUE);
	void		WriteXmlComment(const wchar_t *commentFrontDelim, const wchar_t *commentBody, TXmlItemCrLfMode crlfMode = crlf_none_none);
				// These are the only two methods that can be called when the root container is not opened.

	void		CloseTagHeaderIfNeeded();
	void		LineFeedAndIndent(bool soft_line_feed = TRUE);
				// Soft line feed is not doing anything if nothing is present on the current line.

	void		OpenContainer(WORD tagId);
	void		CloseContainer();
	void		CloseAllContainers();

	void		CheckAllContainersClosed();
				// This method is similar to closing all containers, except that it emits an error if something is still opened.

	TXmlDictRecord *OpenContainerEx(WORD passedTagId, WORD defaultTagId, const TXmlDictRecord *defRootContainersDict, const TXmlCnrWritingProps *defRootContainerProps = NULL, const TXmlCnrWritingProps *defSubContainerProps = NULL)
	{
		TXmlDictRecord *rcnr_dct = GetRootContainersDict();
		if (rcnr_dct == NULL)
		{
			// Information on the allowed root containers is currently not set.
			SetupRootContainersDict(defRootContainersDict, defRootContainerProps, defSubContainerProps);
		}

		OpenContainer((passedTagId != 0) ? passedTagId : defaultTagId);
		return(rcnr_dct);
	}

	void CloseContainerEx(const TXmlDictRecord *rcnr_dct)
	{
		CloseContainer();
		if (rcnr_dct == NULL)
			ResetRootContainersDict();
	}

	void		WriteBoolTag(WORD tagId, bool tagValue, TXmlWriterBoolStyle tagStyle = xwbs_true_false);
	void		WriteNumTag(WORD tagId, __int64 tagValue, wchar_t thousandsSeparatorChar = 0);
	void		WriteStringTag(WORD tagId, const wchar_t *tagValue, int maxStringLen = 256);
	void		WriteGuidTag(WORD tagId, GUID &tagValue);
	void		WriteColorTag(WORD tagId, TColor tagValue);
				// These methods allow to emit both tag attrs and leaf tags. Tags are emitted either as leaf tags or as tag attributes
				// according to the description of the tag in the current container schema.

	void		WriteNamedTag(const wchar_t *tagName, const wchar_t *tagValue, TXmlItemCrLfMode crlfMode = crlf_inherit);
	void		WriteNamedTagAttr(const wchar_t *attrName, const wchar_t *attrValue, TXmlItemCrLfMode crlfMode = crlf_inherit);
				// These methods write the leaf tag or tag attr regardless of contents of the current tags dict.

	void		WriteContainerBody(const wchar_t *bodyValue, TXmlItemCrLfMode crlfMode = crlf_none_none);

	enum
	{
		MAX_TAG_NAME_LEN = 80,
		MAX_TAGS_DICT_LEN = 200,
		MAX_CNRS_NESTING = 20,
	};

	void		ReportError(WORD errCode, const wchar_t *errMessage);

protected:

	TXmlDictRecord  *LookupRootContainerInfo(WORD tagId);
	TXmlDictRecord  *LookupTagInfo(WORD tagId, bool container_tag);

	void		OpenContainerInternal(const wchar_t *containerName, const TXmlDictRecord *tagsDict, TXmlCnrWritingProps *props, TXmlItemCrLfMode crlfMode);

	void		WriteTaggedObject(WORD tagId, TXmlItemBodyType expectedTagBodyType, const wchar_t *value);
	void		WriteScalarObject(TXmlDictRecordType objectType, const wchar_t *tagName, const wchar_t *tagValue, TXmlItemCrLfMode crlfMode);

	void		PrepareForWritingObject(TXmlDictRecordType objectType, TXmlItemCrLfMode prependCrlfMode);
				// Method closes the tag header if this is needed and adds a line feed with indenetaion if needed.

	void		WriteBodyData(const wchar_t *bodyData, bool preserve_st_eols);
	void		WriteRawData(const wchar_t *data_beg, long data_len);

	void		AddLineFeedIfNecessary(TXmlDictRecordType objectType, TXmlItemCrLfMode appendCrlfMode);
	void		AddLineFeedInternal(bool softLineFeed = TRUE);
	void		IndentLine();

protected:

	struct TXmlWriterLayerInfo
	{
		TXmlDictRecord			*m_curr_dict;
		int						m_latest_dict_inx;

		TXmlCnrWritingProps		*m_props;

		wchar_t					m_layer_name[MAX_TAG_NAME_LEN];

		bool						m_attrs_present;
		bool						m_tags_present;
		bool						m_header_closed;

		bool						m_tags_emitted[MAX_TAGS_DICT_LEN];

		TXmlItemCrLfMode			m_append_crlf;
	};

protected:

	TXmlDictRecord				*m_root_cnrs_dict;		// This is an array of containers that are allowed on the root level. Particular instance of
														// the XML writer does not allow generating document with any name of the root container.
														// It allows generating documents from a fixed set. Root container is always opened by its
														// id in this directory of allowed root containers.
	TXmlCnrWritingProps			m_root_cnr_props;
	TXmlCnrWritingProps			m_sub_cnr_props;

	TXmlWriterLayerInfo			m_nesting_data[MAX_CNRS_NESTING];
	int							m_nesting_level;		// This is the number of the entered levels.

	TXmlWriterLayerInfo			*m_curr_layer;			// Pointer to the current slot. This value is not NULL when the root container is entered and
														// there is no nesting overflow case.

	int							m_line_indent;			//   -1	- use TAB characters.
														//    0	- no indentation at all.
														//   >0	- use spaces for indenting the line.

	bool							m_line_dirty;			// Current line is considered to be dirty if it contains something besides the indentation.

private:

	TGenericConsole				*m_console;
	TMessageBuffer				*m_errors_buffer;
									// When the console pointer is not NULL, the errors buffer is ignored. Messages that are written
									// to the error buffer are identical to the messages, that are sent to console.

	int							m_errors_count;
};

#endif	// Common_XmlWriter_H


