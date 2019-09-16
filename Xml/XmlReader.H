//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	Common_XmlReader_H
#define	Common_XmlReader_H

#ifndef   Common_FileDataHelper_H
#include  "Common/FileDataHelper.H"
#endif
#ifndef   Common_GenericConsole_H
#include  "Common/GenericConsole.H"
#endif
#ifndef   Common_StringPointer_H
#include  "Common/StringPointer.H"
#endif
#ifndef   Common_XmlTagsDict_H
#include  "Xml/XmlTagsDict.H"
#endif

#ifndef   WinUi_DialogControlHelpers_H			// This header is needed here for picking up definition of the TColor.
#include  "WinUi/DialogControlHelpers.H"			// In fact, the TColor is a higher level definition than the layer, that is defined
#endif										// in this file. Nevertheless, importing this type directly simplifies things.

enum TXmlReaderErrorCode
{
	// Errors from the XML stream.
	xrde_syntax_erros_base = 1000,

	xrde_xhdr_e1,		xrde_xhdr_e2,		xrde_xhdr_e3,		xrde_xhdr_e4,

	xrde_fldr_e1,
	xrde_lftg_e1,		xrde_lftg_e2,		xrde_lftg_e3,
	xrde_pitg_e1,		xrde_pitg_e2,

	xrde_snth_e1,		xrde_snth_e2,
	xrde_sncl_e1,		xrde_sncl_e2,

	xrde_sksp_e1,
	xrde_scnm_e1,
	xrde_scqs_e1,		xrde_scqs_e2,
	xrde_ueic_e1,
	xrde_bllt_e1,

	// Errors from the basic container handler object.
	xrde_std_handlers_erros_base = 1100,

	xrde_ssvl_e1,
	xrde_nmvl_e1,		xrde_nmvl_e2,		xrde_nmvl_e3,

	// Errors from the XML reader.
	xrde_xml_schema_errors_base = 1200,

	xrde_shlt_e1,		xrde_shlt_e2,		xrde_shlt_e3,		xrde_shlt_e4,

	xrde_mprs_e1,		xrde_mprs_e2,		xrde_mprs_e3,		xrde_mprs_e4,
	xrde_mprs_e5,		xrde_mprs_e6,		xrde_mprs_e7,		xrde_mprs_e8,

	xrde_chtg_e1,
	xrde_chdr_e1,		xrde_chdr_e2,		xrde_chdr_e3,		xrde_chdr_e4,

	xrde_enns_e1,		xrde_enns_e2,
	xrde_exns_e1,		xrde_exns_e2,		xrde_exns_e3,

	xrde_bbvl_e1,
	xrde_bnvl_e1,
	xrde_bgvl_e1,
	xrde_clvl_e1,		xrde_clvl_e2,		xrde_clvl_e3,

	// Application level can also report app level problems in the parsed data.
	xrde_app_errorss_base  = 4000,
};

//
//  Types of lexemas that can be retrieved from the XML stream.
//
//  Tag with attributes is treated as beginning of a container on the XML stream level.
//
//  Current implementation does not allow to add comments inside the leaf tags, because this
//  changes them into containers.
//
enum TXmlLexemaType
{
	xlt_none,

	xlt_tag_attr		= 3,		// Tag attribute like EventId="65382" or EventId='65382' (single or double quotes).
	xlt_leaf_tag		= 4,		// Simple tag like <Name>Value</Name> or <Name>Value</> or <Name/>.
							// XML comments are allowed only at the beginning or at the end of the tag value.
	xlt_cnr_beg		= 5,		// Beginning of the container. When this lexema is returned, it is already known that
							// this is not beginning of a simple field tag. Any tag with attrs is treated as container.
	xlt_cnr_body	= 6,		// The XML stream reader understands the container body and reports these lexemas
							// to the XML parser. XML parser simply ignores these lexemas.
	xlt_cnr_end		= 7,		// End of the container or end of the tag header that is also an end of the tag itself.
							// Note that any tag with attrs is treated as a container.
	xlt_eof			= 8,		// End of the XML stream.
	xlt_error		= 9,		// Scanner was not able to find the correct lexema. Nevertheless the parser should
							// continue parsing. It is known for sure that the stream pointer was shifted forward
							// at least for one character.
};

class TXmlReader;

//
//  XML stream implements scanner that breaks the input data into definitions of individual tags, tag attributes,
//  openings and closings of containers, etc.
//
class TXmlStream
{
public:

	TXmlStream(const wchar_t *fname, const wchar_t *buffer = NULL, int buff_len = -1);
		// If the buffer is not passed in the ctor, the XmlParser will attempt to load the file with the specified name
		// at the beginning of the parsing. In fact, the stream is not functional after calling the ctor. It is neccessary
		// to set the parser into it to make it functional.

	~TXmlStream();

	void					SetParser(TXmlReader *parser);
							// Establish a bidirectional link between the XML stream and the XML parser. Parser is used for
							// reporting syntax errors and low level areas like comments and location of delimiters.

	const wchar_t			*GetStreamFileName() const { return(m_stream_name); }
	TDateTime			GetStreamFileDate() const;
	bool					CheckDataBufferPresence() const { return(m_buff != NULL); }

	bool					SkipXmlHeader();
							// Return value is FALSE when there is an error inside the XML header or when there is no beginning
							// of the root container opening tag.

	TXmlLexemaType		GetXmlLexema(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end);
							// Note: The name of the tag and its value are both NOT zero terminated. This method simply
							// returns pointers into the stream data and lengths of the tag name and body respectedly.
							// When this method returns xlt_error, an appropriate error is already emitted.
protected:

	// Low level lexemas in the tag header.
	enum TXmlTagHeaderLex
	{
		xhl_attr_name,		// "abcd"
		xhl_equ,			// "="
		xhl_attr_val,		// qouted "ab cd"
		xhl_end1,			// ">"
		xhl_end2,			// "/>"
		xhl_eof,
		xhl_error,
	};

	// Low level lexemas outside of the tag header.
	enum TXmlContainerLex
	{
		xcl_tag_open,		// "<xyz "
		xcl_inner_tag,		// "<xyz>"
		xcl_empty_tag,		// "<xyz/>"
		xcl_short_end,		// "</>"
		xcl_named_end,		// "</xyz>"
		xcl_body_text,		// "abcd efgh"
		xcl_eof,
		xcl_error,
	};

	bool					LoadXmlFileFromDisk();
							// Method tries to load the disk file using the name that is set in the ctor. In case of error
							// is reports this error to the XmlParser.

	TXmlLexemaType		GetLexFromTagHeader(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end);
	TXmlLexemaType		GetLexFromContainer(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end);

	TXmlLexemaType		ProcessTagHdrOpen(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end);
	TXmlLexemaType		ProcessInnerTag(TStringPtr &tag_name, TStringPtr &tag_value, TStringPtr &tag_area_ex, int &line_num_at_lex_end);

	TXmlTagHeaderLex		GetTagHeaderLex(TStringPtr &lex_value, int &lex_end_line_num);
	TXmlContainerLex		GetContainerLex(TStringPtr &lex_value, int &lex_end_line_num);
							// Methods return either buffered presanned lexemas or pick up new lexemas from the source.
							// When these methods return xhl_error/xcl_error, an appropriate error is already reported.

	void		BufferLowLevelTagHeaderLex(TXmlTagHeaderLex lex_type, TStringPtr &lex_value, int &lex_end_line_num);
	void		BufferLowLevelContainerLex(TXmlContainerLex lex_type, TStringPtr &lex_value, int &lex_end_line_num);

	bool		SkipXmlSpaces(bool allowComments);
				// Return value is FALSE when the stream does not contain non empty chars after skipping the spaces.

	bool		ScanName(TStringPtr &name_area, bool tag_hdr_context);
	bool		ScanQuotedString(wchar_t frame_char, TStringPtr &string_body_area);
	bool		ScanContainerBody(TStringPtr &body_area, int &body_end_line_num);
				// When return value is TRUE, the output param contains the area of the discovered name or string body or
				// container body fragment. Otherwise methods issue an error. Methods (except for the container body) shift
				// the current position forward at least for one character, except for the EOF case.

	TXmlContainerLex		UnexpectedEofInsideTheContainer(int offs_beg, TStringPtr &lex_value);
	TXmlLexemaType		ReportBadLowLevelLexemaType(int llt, TStringPtr &tag_name);

protected:

	struct TBufferedLowLevelLex
	{
		union
		{
			TXmlTagHeaderLex		m_hdr_lex_type;
			TXmlContainerLex		m_cnr_lex_type;
		};

		TStringPtr				m_lex_value;				// For quoted strings this is body of the string without quotes.
		int						m_lex_line_num;			// Line number at the end of the lexema body (if it has the body).
	};

protected:

	wchar_t						*m_stream_name;			// Name of the stream is never owned.

	wchar_t						*m_buff;
	int							m_buff_len;
	bool							m_buff_owned;				// Buffer is owned when the file was loaded from the disk.
															// The buffer, that was passed as param of the ctor, is newer owned.
	int							m_offs;
	int							m_line_num_at_offs;

	bool							m_tag_hdr_state;
	int							m_cnt_buffered_lexemas;
	TBufferedLowLevelLex			m_buffered_lexemas[2];

	TXmlReader					*m_parser;					// Parser object is used by the stream class only for logging errors and
															// reporting low level areas. It is not used for anything else in this class.
	friend class TXmlReader;
};

//
//  Basic node is an abstract class that receives requests for tags processing.
//  Application should derive its own class or classes that will be used for parsing the XML file.
//
class TXmlContainerHandler
{
public:

	TXmlContainerHandler() { m_parser = NULL; }

	virtual void	SetParser(TXmlReader *parser) { m_parser = parser; }

	enum { ROOT_CONTAINER_TAG_ID = 0 };
					// This id is used as a pseudo tag id in the calls to BeginNodeProcessing() and FinalizeNodeProcessing()
					// when the root container is entered and exited. Root container does not belong to any context. This means
					// that there is no dict entry that describes this call.

	virtual void	BeginNodeProcessing(WORD cnrTagId) { }
	virtual void	FinalizeNodeProcessing(WORD cnrTagId, bool nodeScanningProblems) { }
					// First parameter is an id of the tag in the dictionary of the layer that has called the container.
					// Finalize method is called all the times regardless of how container is exited. The passed param shows
					// if abort is signalled or the count of errors increased while the node was scanned or not. This param
					// describes both the scanner level errors and the app level errors.

	virtual void	ProcessBooleanField(WORD tagId, bool tagValue, TStringPtr &tagName) { assert(FALSE); }
	virtual void	ProcessIntegerField(WORD tagId, __int64 tagValue, TStringPtr &tagName) { assert(FALSE); }
	virtual void	ProcessStringField(WORD tagId, TStringPtr &strValue, TStringPtr &tagName) { assert(FALSE); }
	virtual void	ProcessGuidField(WORD tagId, GUID &tagValue, TStringPtr &tagName) { assert(FALSE); }
	virtual void	ProcessColorField(WORD tagId, TColor tagValue, TStringPtr &tagName) { assert(FALSE); }
					//
					// Tag name is passed only for error reporting purposes. Applications should not check the passed name.
					// Contents of the strings are passed exactly as they are present in the source document. The XML escape
					// sequencies if any are not decoded.
					//

	virtual TXmlContainerHandler *OpenSubContainer(WORD tagId, TStringPtr &tagName) { return(NULL); }
					//
					//  It is not necessary to allocate separate container handler for each subcontainer. For example, implementation
					//  may return all the times the "this" pointer.
					//
					//  When OpenSubContainer() method returns NULL, this means that subcontainer is not accepted.
					//  Nevertheless this will not abort the parsing. To abort the parsing it is necesary to do this explicitly by calling
					//  the AbortXmlParsing() method in the parser.
					//
					//  Tag name is passed only for error reporting purposes. Applications should not check the passed name.
					//

	bool		LoadFromExeDirectory(const TXmlDictRecord *rootContainerTags, const wchar_t *xpct_root_container_name, const wchar_t *short_file_name,
										TGenericConsole *cons = NULL, TDateTime *latest_scanned_file_date = NULL, bool *new_file_detected_flag = NULL);
					// It is ok to pass the NULL console when the error messages are not needed. When the pointer to the date of the last
					// scanned file is NULL, the scanning errors are sent to the console all the times. Otherwise errors are sent only when
					// the date of the file differs from the passed date of the last scan. The new file detected flag is updated only if
					// the latest_scanned_file_date pointer is not NULL.

protected:

	bool		StoreStringValue(TStringPtr &strValue, wchar_t *location, int locationLen, TStringPtr &tagName, bool emitErrorOnOverflow = TRUE);
	void		StoreStringValueAllowTruncation(TStringPtr &strValue, wchar_t *location, int locationLen);
					// Simple helper functions that save passed string into the passed buffer.

	bool		StoreCharValue(__int64 tagValue, int min_value, int max_value, char &destination, TStringPtr &tagName);
	bool		StoreShortValue(__int64 tagValue, short min_value, short max_value, short &destination, TStringPtr &tagName);
	bool		StoreIntValue(__int64 tagValue, int min_value, int max_value, int &destination, TStringPtr &tagName);

	TXmlReader			*m_parser;			// TXmlReader sets up this field in the root node handler and in other node
											// handlers that are returned from the OpenSubContainer(). This field is NULL
											// only in the s_skip_subtree_handler.
	friend class TXmlReader;
};

// File area types are needed ONLY for syntax highlighting. Thety are not used during the regular parsing.
enum TXmlFileAreaType
{
	xfat_xml_header_frame,					// "<?" and "?>".
	xfat_xml_header_body,					// Contents of the XML header whatever it is.
	xfat_xml_comment_frame,				// "<!--" and "-->".
	xfat_xml_comment_body,				// Contents of the XML comment whatever it is.

	xfat_delim_tag_frame,					// "<", ">", "</", "/>" and "</>".
	xfat_delim_tag_hdr_equ,					// "=".

	xfat_tag_attr_name,
	xfat_tag_name_open,
	xfat_tag_name_close,
	xfat_container_name_open,
	xfat_container_name_close,

	xfat_tag_attr_value,						// Area of the tag attr value includes the qoutes around it.
	xfat_leaf_tag_value,						// Area of the leaf tag value does not include the leading and trailing spaces.
	xfat_container_body,

	xfat_xml_syntax_error,
	xfat_xml_schema_error,
	xfat_app_layer_error,

	xfat_num_area_types,
};

//
//  XML reader does not understand the meaning of the presentation props.
//
struct TXmlItemPresentationProps
{
	BYTE		m_tag_open_style_inx;
	BYTE		m_tag_body_style_inx;
	BYTE		m_tag_close_style_inx;
					// Style indexes are zero based. Zero index is a valid index style. When the value of the index
					// is 0xFF, that means that element should be painted using the default settings.
					// These indexes are used as params to TXmlFileViewStyle::GetAppFrgrAreaStyle().

	bool			m_show_leaf_tag_as_container;
					// This flag is checked even if the value of some/all of the indexes above are 0xFF.
};

struct TXmlItemPresentationRecord
{
	short							m_next_same_hash_record;				// Index of the next record in the array plus one. Zero value
																			// means that the current record is the last record of the list.
	short							m_xml_item_name_len;
	wchar_t							*m_xml_item_name;						// Record with NULL value of this field marks the end of array.

	const TXmlItemPresentationProps		*m_presentation_props;
};

struct TXmlFileAreaInfo
{
	TXmlFileAreaType					xml_area_type;

	int								xml_area_beg;
	int								xml_area_len;
										// Event areas of all event types except for errors are not empty. Error event areas
										// can be empty and non empty.
	union
	{
		TXmlItemPresentationRecord		*xml_tag_info;

		struct
		{
			int						xml_error_id;				// XML parsing errors are numbered sequentially starting from 1.
																// Each parsing session has its own space of parsing error ids.
			wchar_t					*xml_error_text;
		};
	};
};

typedef void  *XmlSyntaxHighlightCbkContext;
typedef void (*XmlSyntaxHighlightCbkHandler)(XmlSyntaxHighlightCbkContext ctx, TXmlFileAreaInfo &xml_area_info);

//
//	XML reader id not decoding the XML escape sequencies. If such seqs are present in the document,
//	they are simply passed to the callback handlers.
//
class TXmlReader
{
public:
			TXmlReader(const TXmlDictRecord *root_container_tags, const wchar_t *xpct_root_cnr_name, TGenericConsole *cons = NULL);
			TXmlReader(const TXmlDictRecord *root_container_tags, const wchar_t *xpct_root_cnr_name, TMessageBuffer &errors_buff);
				//
				// Current implementation works only with fixed XML schemas that are known before starting the scanning
				// process. This schema is passed as the first param of the ctor. If the passed name of the root container
				// is not empty, then the name of the root container in the source file should match the passed name.
				// Otherwise the root container in the input file can have any name.
				//

			TXmlReader(XmlSyntaxHighlightCbkHandler cbk_handler, XmlSyntaxHighlightCbkContext cbk_handler_context,
							TXmlItemPresentationRecord *special_tags_dict = NULL, TGenericConsole *cons = NULL);
				// This ctor creates reader that is used for displaying XML files with the highlighted syntax. It is ok to pass NULL
				// in the expected XML schema. In this case the scanner will treat all containers tags and all tag attributes as
				// expected objects that can have multiple instances.

	bool		Parse(TXmlStream &source, TXmlContainerHandler *root_node_handler);
				// This method should be used only with the first pair of ctors.

	bool		SyntaxHighlightParse(TXmlStream &source);
				// This method should be used only with the last ctor.

	void		AbortXmlParsing();
	void		ReportXmlError(WORD error_code, int error_area_offs, int error_area_len, int line_num, const wchar_t *format, ...);
	void		ReportXmlError(WORD error_code, TStringPtr &error_area, int line_num, const wchar_t *format, ...);
	void		ReportXmlAppError(const wchar_t *format, ...);
				// These methods can be called from inside the application defined callbacks. When the reported error does
				// not have relation to any particular place in the file, the error_area_offs should be passed as -1 and error_area
				// should be empty.

	static TXmlFileAreaType XmlAreaTypeFromXmlErrorCode(WORD error_code);

	enum { MAX_TAGS_DICT_LEN = 50, MAX_CNRS_NESTING = 20, MAX_ROOT_CNR_NAME_LEN = 80, };

	enum { MAX_ERRORS_IN_DOCUMENT = 40 };
				// After detecting this number of errors the parsing of the document is aborted.

	bool		CheckTagMentioned(WORD tagId);
				// This method is useful in the code of the container handlers when it is needed to check if some tag
				// is already processed or not. This method reports state of processing for the current container.

	const wchar_t		*GetXmlStreamFileName() const { return((m_input != NULL) ? m_input->GetStreamFileName() : NULL); }
	TGenericConsole	*GetReaderConsole() { return(m_console); }

	int		GetCurrentNesting() const { return(m_nesting_level); }
	int		GetLatestLexemaLineNumber() const { return(m_latest_lexema_line); }
	int		GetErrorsCount() const { return(m_errors_count); }

	static	TXmlContainerHandler		s_skip_subtree_handler;		// This static stub handler is used when it is necessary to skip
																// some arbitrary subtree. The field is decared as public because
																// its address can be returned by the callback handlers.
protected:

	void		Init(const TXmlDictRecord *root_container_tags = NULL, const wchar_t *xpct_root_cnr_name = NULL);
	void		SetStream(TXmlStream *source) { m_input = source; }

	void		PrependErrorsHeader();
	void		ReportErrorInternal(wchar_t *buffer_big_2maxpath_80, const wchar_t *msg_prefix, const wchar_t *format, va_list vargs);

	void		ReportXmlArea(TXmlFileAreaType area_type, int area_beg_offs, int area_len, TXmlItemPresentationRecord *area_tag_info = NULL);
	void		ReportXmlArea(TXmlFileAreaType area_type, TStringPtr &area, TXmlItemPresentationRecord *area_tag_info = NULL);

	void		ParseInternal(TXmlContainerHandler *rootNode);

	TXmlDictRecord	*CheckTagsDict(TStringPtr &tagName, int &tagDictInx);

	bool		CheckDictRecord(TXmlLexemaType lexType, TStringPtr &tagName, TXmlDictRecord *dictItem, int tagDictInx);

	bool		EnterNesting(TStringPtr &tagName, TXmlDictRecord *nodeDictItem, int tagDictInx, TXmlContainerHandler *forcedNode = NULL);
	bool		ExitNesting(TStringPtr &closingTagName, bool abortFlag = FALSE, bool special_unclosed_exit = FALSE);
				// Return value from the two methods above is the same to !m_abort_flag.

	bool		AssembleBoolean(TStringPtr &tagValue, bool &value);
	bool		AssembleInteger(TStringPtr &tagValue, __int64 &value);
	bool		AssembleGuid(TStringPtr &tagValue, GUID &value);
	bool		AssembleColor(TStringPtr &tagValue, TColor &value);

	static wchar_t *PluralSuff(int value) { return(((value % 10 == 1) && (value % 100 != 11)) ? L"" : L"s"); }

	bool		AssembleBooleanInternal(wchar_t *tagValue, int tagValueLen, bool &value);
	bool		AssembleGuidInternal(wchar_t *tagValue, int tagValueLen, GUID &value);

	wchar_t		*SkipColorComponent(wchar_t *tagValue, int &tagValueLen, int &componentValueLen);
	bool			AssembleColorComponent(wchar_t *componentValue, int componentValueLen, BYTE &value, const wchar_t *componentName);

private:

	enum { NUM_SHLT_TAG_BUCKETS = 16 };		// This should be strict power of 2.

	// Information about the entered layer.
	struct TXmlParsingLayerInfo
	{
		TXmlDictRecord			*m_layer_tags;
		TXmlContainerHandler		*m_layer_node;

		WORD					m_cnr_tag_id;				// Id of the container in the tags dict of the upper layer or 0.
		TStringPtr				m_cnr_name;				// Name of the current container as it was opened.
		int						m_cnr_beg_line;
		int						m_cnr_upper_dict_inx;		// Index into the tags dict of the upper layer or -1.

		int						m_tags_search_hint;			// Index of the local tag where it makes sense to start the next search.

		bool						m_tags_present[MAX_TAGS_DICT_LEN];
		int						m_initial_errs_count;		// Value of the errors count when the container was opened.
	};

private:

	TXmlStream							*m_input;

	wchar_t								m_expected_root_cnr_name[MAX_ROOT_CNR_NAME_LEN];
											// When this field is empty, the parser will accept any name of the root container.
											// Otherwise it will check the name and it may generate a parsing error.

	TXmlParsingLayerInfo					m_nesting_data[MAX_CNRS_NESTING];
	int									m_nesting_level;		// Number of the entered nesting levels.
	TXmlParsingLayerInfo					*m_curr_layer;			// Ptr to the current slot or NULL if no layer is currently entered.

	TXmlItemPresentationRecord				*m_shlt_tags_dict;
	short								m_shlt_tags_hash_table[NUM_SHLT_TAG_BUCKETS];
	XmlSyntaxHighlightCbkHandler			m_shlt_cbk_handler;
	XmlSyntaxHighlightCbkContext			m_shlt_cbk_context;
											// This group of fields is used in the syntax highlight parsing sessions.

	TGenericConsole						*m_console;
	TMessageBuffer						*m_errors_buffer;
											// When ptr to the errors buffer is not NULL, the console ptr is ignored. Message in the error
											// buffer is prepared primarily for displaying it in a popup message. Other use of this message
											// is possible, but it might not look as nice as it will look in a popup window.

	bool									m_abort_flag;
	int									m_latest_lexema_line;
	int									m_errors_count;

	wchar_t								*m_current_error_msg;
											// This is temp pointer that is valid only when the ReportXmlArea() method is called from
											// the ReportXmlError(). This simplifies the interface.
	friend class TXmlStream;
	friend class TXmlContainerHandler;
};

#endif	// Common_XmlReader_H


