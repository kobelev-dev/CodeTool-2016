//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	LangView_HtmlGenSource_H
#define	LangView_HtmlGenSource_H

#ifndef   Xml_HtmlGenDecors_H
#include  "Xml/HtmlGenDecors.H"
#endif
#ifndef   LangView_SyntaxHighlight_H
#include  "LangView/SyntaxHighlight.H"
#endif
#ifndef   TextScan_CppDatabase_H
#include  "TextScan/CppDatabase.H"
#endif

class THtmlSourceCodeStyle
{
public:			//
				//	When the returned style is NULL, this means that appropriate area should not be highlighted.
				//

			THtmlSourceCodeStyle(const wchar_t *icons_dir_name) { m_src_code_style_prepared = FALSE; m_src_icons_directory = (wchar_t*)icons_dir_name; }
	void		PrepareStdStyles() { m_src_code_style_prepared = TRUE; }

	void		GetWholeFileTopBottomExtents(int &top_area_height, int &bottom_area_height) { top_area_height = bottom_area_height = 4; }
				// The height of the areas above all lines of the file and below all lines of the file in pixels.

	void		GetPartialSrcEncloseFraming(int &num_top_rows, int &num_bottom_rows) { num_top_rows = 4; num_bottom_rows = 6; }
				// The number of lines to show above and below the active area of the file.

	const wchar_t		*GetHtmlTableCssStyle();
						// The source code element is always <div>. Its CSS class is provided by the upper layer and it
						// may differ from one case to another. Inside the <div> element resides one or several <table> elements.
						// The CSS class of the <table> element is picked up from the style object using the method above.

	void		GetWholeFileBkgrStyles(TSrcViewWholeFileStyleInx style_inx, wchar_t *&line_lums_style, wchar_t *&marks_area_style, wchar_t *&line_body_style);
	void		GetCodeStatusBkgrStyles(TConditionalStatus code_status, wchar_t *&line_lums_style, wchar_t *&marks_area_style, wchar_t *&line_body_style);

	const wchar_t		*GetCmdlLexemaFrgrStyle(TSrcViewCmdlLexemaStyleInx style_inx);
	const wchar_t		*GetCLangLexemaFrgrStyle(TSrcViewCLangLexemaStyleInx style_inx);

	const wchar_t		*GetMidLevObjectBkgrStyle(TSrcViewMidLevBkgrStyleInx style_inx);
	const wchar_t		*GetMidLevObjectFrgrStyle(TSrcViewMidLevFrgrStyleInx style_inx);

	const wchar_t		*GetCppObjectBkgrStyle();
	const wchar_t		*GetCppObjectFrgrStyle(TCppSourceAreaType area_type);

	void		GetSpacelessDecorationProps(TSrcViewSpacelessDecorInx style_inx, const wchar_t *&image_src_file, const wchar_t *&css_style, wchar_t *buff_80);

protected:

	bool					m_src_code_style_prepared;
	wchar_t				*m_src_icons_directory;
};

class THtmlSourceCodeFile : public THtmlDecoratedFile
{
public:			//
				//	This class generates html element with the fixed type: <div>.
				//
				//	The HTML id of the element is always identical to the prefix of the line numbers ids.
				//

			THtmlSourceCodeFile(const wchar_t *css_class = NULL, const wchar_t *css_style = NULL, THtmlCrLfMode crlf_mode = crlm_outline_object_frames);
			~THtmlSourceCodeFile() { ResetSourceFile(); }

	void		ResetSourceFile();
				// Method resets the underlying decorated file and releases all syntax related objects.

	bool		SetupSourceFile(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, const wchar_t *line_number_id_prefix,
								TFileInfo &file, bool bypass_file_info_data, int line_from = 1, int num_lines = 0);
				//
				// Once the ownership on the file data is not bypassed, the file data is expected to be persistent during
				// the whole lifetime of the object or until the next setup.
				//

	bool		SetupSourceFile(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, TMidLevStoreReader *db, TMidLevScanner *active_scanner,
								ID call_pars_id, ID call_event_id, int line_from = 0, int num_lines = 0);
	bool		SetupSourceFile(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, TMidLevStoreReader *db, TMidLevScanner *active_scanner,
								CtvLexSourceEnterEvent &call_event, int line_from = 0, int num_lines = 0);
				//
				// Note that when the "line_from" param is passed as zero, this means that only the active area that belongs
				// to the scope of the lex src call should be rendered. If this param is passed as 1, then the whole file will be
				// rendered regardless whether the active area covers the whole file or not.
				//

	const wchar_t	*SourceFileName() const { return((m_file.info.file_name != NULL) ? m_file.info.file_name : L"<null_fname>"); }
	inline long	  NumLines() const { return(m_file.NumLines()); }

	virtual void	SetupAdditionalDecors(TSyntaxHighlighter &hlpr) { }

protected:

	bool		AcceptFileData(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, const wchar_t *line_numbers_id_prefix,
								TFileInfo &file, bool bypass_file_info_data, int line_from, int num_lines);
	bool		FinalizeFileSetup(TGenericConsole &cons);

	void		ApplyDefnRangesLayout();

	class THtmlSourceCodeBuilder : public TSyntaxHighlightDocumentBuilder
	{
	public:
			THtmlSourceCodeBuilder(TGenericConsole &cons, THtmlSourceCodeStyle &code_style, THtmlSourceCodeFile *owner)
					: m_gen_console(cons), m_code_style(code_style), m_owner_file(*owner) { }

		virtual void	ApplyFileViewStyleToUpperArea(TSrcViewWholeFileStyleInx view_style_inx, long upper_area_end, bool whole_line_area_style);
		virtual void	ApplyFileViewStyleToLowerArea(TSrcViewWholeFileStyleInx view_style_inx, long lower_area_beg, bool whole_line_area_style);
		virtual void	ApplySpecialNumsMarksBackgrounds(long src_pars_beg_offs, long src_pars_end_offs, bool whole_line_area_style);

		virtual void	ApplyMidLevBkgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevBkgrStyleInx style_inx, TSourceAreaInfo &src_area);
		virtual void	ApplyMidLevFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceAreaInfo &src_area);
		virtual void	ApplyCppObjectFrgrDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder, TCppSourceAreaType area_type, TSourceAreaInfo &src_area);

		virtual void	ApplyCodeStatusDecors(TSourceViewDecorOwner *owner, bool whole_line_beg_style, bool whole_line_end_style);

		virtual void	ApplyCmdlRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCmdlLexemaStyleInx style_inx, TSourceArea &area);
		virtual void	ApplyCLangRawDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewCLangLexemaStyleInx style_inx, TSourceArea &area);
		virtual void	ApplyMacroHdrFrgrDecor(DWORD owner_data, TSrcViewDecorZorder zorder, TSrcViewMidLevFrgrStyleInx style_inx, TSourceArea &area);

		virtual void	ApplyInlineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder,
												TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long src_area_offs);
		virtual void	ApplyWholeLineSpaclessDecor(TSourceViewDecorOwner *decor_owner, TSrcViewDecorZorder zorder,
												TSrcViewSpacelessDecorInx decor_type_inx, short horz_order, long line_number);

		virtual bool	GetVisibleArea(long &visible_area_beg, long &visible_area_len);
		virtual void	ReportProcessingProblem(const wchar_t *message);

	protected:

		TRawHtmlElement	*FindOrCreateSpacelessDecorOwner(TSourceViewDecorOwner *decor_owner, TSrcViewSpacelessDecorInx decor_type_inx);

	protected:

		TGenericConsole				&m_gen_console;
		THtmlSourceCodeStyle			&m_code_style;
		THtmlSourceCodeFile			&m_owner_file;
	};

	static void	ShowConsoleError(TGenericConsole &cons, const wchar_t *format, ...);

protected:

	TFileInfo								m_file;
	bool									m_file_data_owned;

	THtmlGenDecorOwnersContext			m_low_level_decor_owners;
	TSourceViewDecorOwnersContext		m_syntax_level_decor_owners;

	int									m_decor_creation_errors;
	int									m_decor_owner_creation_errors;
};

class THtmlCppDatabaseReportStyle
{
public:
			THtmlCppDatabaseReportStyle(const wchar_t *icons_dir_name) { m_report_style_prepared = FALSE; m_cpp_list_view_icons_dir = (wchar_t*)icons_dir_name; }
	void		PrepareStdStyles() { m_report_style_prepared = TRUE; }

	const wchar_t		*GetReportTableCssStyle() const { return(L"crp_report_table"); }
	const wchar_t		*GetIndentCellCssStyle() const { return(L"crp_indent_cell"); }
	const wchar_t		*GetBodyCellCssStyle() const { return(L"crp_body_cell"); }

	int		WantTopRow() const { return(6); }
	int		WantBottomRow() const { return(7); }

	void		GetItemTypeBitmapProps(TCppItemBase &item, const wchar_t *&image_src_file, const wchar_t *&css_style, wchar_t *buff_80) const;
				// Not all Cpp item types have associated icons. Note that in some cases other props of the item
				// are used, not just its type.

	const wchar_t		*GetItemNameCssStyle(TCppItemBase &item) const;
	const wchar_t		*GetItemDetailsCssStyle(TCppItemBase &item) const;
	const wchar_t		*GetItemIdCssStyle() const { return(L"crp_item_id"); }

protected:

	bool			m_report_style_prepared;
	wchar_t		*m_cpp_list_view_icons_dir;
};

class THtmlCppDatabaseReport : public TAdvancedHtmlElement
{
public:			//
				//	This class generates html element with the fixed type: <table>.
				//

			THtmlCppDatabaseReport(const wchar_t *css_class = NULL, const wchar_t *css_style = NULL, THtmlCrLfMode crlf_mode = crlm_outline_object_frames);
				// When the css class is passed into the ctor explicitly, this css class will be used for generating the object.
				// Otherwise SetupReport() will setup the default css class that will be picked up from the report style.

	virtual bool	IsBodyPresent() { return(TRUE); };
	virtual void	EmitElementBody(THtmlDocument &doc);

	void		SetupReport(TGenericConsole &cons, THtmlCppDatabaseReportStyle &report_style, TCppDatabase &cdb,
							TCppItemBase *report_root = NULL, bool want_all_database_items = TRUE, bool want_item_details = TRUE, bool want_item_ids_clmn = FALSE);

	int		GetCntAppErrors() const { return(m_cnt_app_errors); }

protected:

	void		EmitItemsHier(THtmlDocument &doc, TCppItemBase &items_parent, int child_items_nest_level = 0);
	void		EmitSingleItem(THtmlDocument &doc, TCppItemBase &item, int item_nest_level);
	void		EmitPaddingRow(THtmlDocument &doc, int row_height);

	bool		CheckIncludeItem(TCppItemBase &item);

	const wchar_t *GetItemName(TCppItemBase &item)
	{
		if (item.IsDefinition() == TRUE && (item.ItemType() < cxx_block || item.ItemType() > cxx_light_block))
			return(((TCppDefnBase&)item).DefnName());
		else return(TCppItemHelper::GetItemTypeDisplayName(item.ItemType()));
	}

	bool		GenerateItemDetails(TMessageBuffer &buffer, TCppItemBase &item);

	enum { NUM_HIER_COLUMNS = 20 };

	TGenericConsole					*m_rprt_console;
	THtmlCppDatabaseReportStyle		*m_report_style;
	TCppDatabase					*m_cdb;

	TCppItemBase					*m_report_root;

	bool								m_show_all_items;
	bool								m_show_item_details;
	bool								m_show_item_ids_clmn;

	int								m_cnt_app_errors;
};

class THtmlSourceGenHelper
{
public:
			THtmlSourceGenHelper(TGenericConsole &cons) : m_src_gen_cons(cons) { m_css_dir_or_url = NULL; m_css_file_names = NULL; }

	void		SetCssFilesInfo(const wchar_t *css_dir_or_url, const wchar_t **css_file_names) { m_css_dir_or_url = css_dir_or_url; m_css_file_names = css_file_names; }

	bool		DumpParsingIndependentFile(THtmlDocument &rprt, THtmlSourceCodeStyle &code_style, const wchar_t *line_number_id_prefix,
											TFileInfo &file, int line_from = 1, int num_lines = 0);
	bool		DumpLexSourceEnclosure(THtmlDocument &rprt, THtmlSourceCodeStyle &code_style, TMidLevStoreReader *db, TMidLevScanner *active_scanner,
											ID call_pars_id, ID call_event_id, int line_from = 0, int num_lines = 0);
protected:

	TGenericConsole		&m_src_gen_cons;

	const wchar_t			*m_css_dir_or_url;
	const wchar_t			**m_css_file_names;
};

#endif	// LangView_HtmlGenSource_H


